/*
 * AVI demuxer
 * Copyright (c) 2001 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <inttypes.h>

#include "libavutil/avassert.h"
#include "libavutil/avstring.h"
#include "libavutil/opt.h"
#include "libavutil/dict.h"
#include "libavutil/internal.h"
#include "libavutil/intreadwrite.h"
#include "libavutil/mathematics.h"
#include "avformat.h"
#include "avi.h"
#include "dv.h"
#include "internal.h"
#include "isom.h"
#include "riff.h"
#include "libavcodec/bytestream.h"
#include "libavcodec/exif.h"
#include "libavcodec/internal.h"

typedef struct AVIStream {
    int64_t frame_offset;   /* current frame (video) or byte (audio) counter
                             * (used to compute the pts) */
    int remaining;
    int packet_size;

    uint32_t handler;
    uint32_t scale;
    uint32_t rate;
    int sample_size;        /* size of one sample (or packet)
                             * (in the rate/scale sense) in bytes */

    int64_t cum_len;        /* temporary storage (used during seek) */
    int prefix;             /* normally 'd'<<8 + 'c' or 'w'<<8 + 'b' */
    int prefix_count;
    uint32_t pal[256];
    int has_pal;
    int dshow_block_align;  /* block align variable used to emulate bugs in
                             * the MS dshow demuxer */

    AVFormatContext *sub_ctx;
    AVPacket *sub_pkt;
    AVBufferRef *sub_buffer;

    int64_t seek_pos;
} AVIStream;

typedef struct AVIContext {
    const AVClass *class;
    int64_t riff_end;
    int64_t movi_end;
    int64_t fsize;
    int64_t io_fsize;
    int64_t movi_list;
    int64_t last_pkt_pos;
    int index_loaded;
    int index_scaned;
    int is_odml;
    int non_interleaved;
    int stream_index;
    DVDemuxContext *dv_demux;
    int odml_depth;
    int use_odml;
    int odml_node_num;
    int64_t *odml_node_pos_list;
    int use_external_pts;
#define MAX_ODML_DEPTH 1000
    int64_t dts_max;
    int use_dmx_pts;
    int maybe_non_interleaved;
    int aindex_entries_invalid;
} AVIContext;

static const AVOption options[] = {
    { "use_odml", "use odml index", offsetof(AVIContext, use_odml), AV_OPT_TYPE_BOOL, {.i64 = 0}, -1, 1, AV_OPT_FLAG_DECODING_PARAM},
    { "maybe_non_interleaved", "maybe non_interleaved", offsetof(AVIContext, maybe_non_interleaved), AV_OPT_TYPE_BOOL, {.i64 = 0}, -1, 1, AV_OPT_FLAG_DECODING_PARAM},
    { NULL },
};

static const AVClass demuxer_class = {
    .class_name = "avi",
    .item_name  = av_default_item_name,
    .option     = options,
    .version    = LIBAVUTIL_VERSION_INT,
    .category   = AV_CLASS_CATEGORY_DEMUXER,
};


static const char avi_headers[][8] = {
    { 'R', 'I', 'F', 'F', 'A', 'V', 'I', ' '  },
    { 'R', 'I', 'F', 'F', 'A', 'V', 'I', 'X'  },
    { 'R', 'I', 'F', 'F', 'A', 'V', 'I', 0x19 },
    { 'O', 'N', '2', ' ', 'O', 'N', '2', 'f'  },
    { 'R', 'I', 'F', 'F', 'A', 'M', 'V', ' '  },
    { 0 }
};

static const AVMetadataConv avi_metadata_conv[] = {
    { "strn", "title" },
    { "isbj", "subject" },
    { "inam", "title" },
    { "iart", "artist" },
    { "icop", "copyright" },
    { "icmt", "comment" },
    { "ignr", "genre" },
    { "iprd", "product" },
    { "isft", "software" },

    { 0 },
};

static int avi_read_close(AVFormatContext *s);
static int avi_load_index(AVFormatContext *s);
static int guess_ni_flag(AVFormatContext *s);
static int avi_load_odml_index(AVFormatContext *s);

#define print_tag(s, str, tag, size)                                      \
    av_log(s, AV_LOG_TRACE, "pos:%"PRIX64" %s: tag=%s size=0x%x\n", \
           avio_tell(pb), str, av_fourcc2str(tag), size)                  \

static inline int get_duration(AVIStream *ast, int len)
{
    if (ast->sample_size)
        return len;
    else if (ast->dshow_block_align)
        return (len + (int64_t)ast->dshow_block_align - 1) / ast->dshow_block_align;
    else
        return 1;
}

static int get_riff(AVFormatContext *s, AVIOContext *pb)
{
    AVIContext *avi = s->priv_data;
    char header[8] = {0};
    int i;

    /* check RIFF header */
    avio_read(pb, header, 4);
    avi->riff_end  = avio_rl32(pb); /* RIFF chunk size */
    avi->riff_end += avio_tell(pb); /* RIFF chunk end */
    avio_read(pb, header + 4, 4);

    for (i = 0; avi_headers[i][0]; i++)
        if (!memcmp(header, avi_headers[i], 8))
            break;
    if (!avi_headers[i][0])
        return AVERROR_INVALIDDATA;

    if (header[7] == 0x19)
        av_log(s, AV_LOG_INFO,
               "This file has been generated by a totally broken muxer.\n");

    return 0;
}

static int read_odml_index(AVFormatContext *s, int frame_num)
{
    AVIContext *avi     = s->priv_data;
    AVIOContext *pb     = s->pb;
    int longs_per_entry = avio_rl16(pb);
    int index_sub_type  = avio_r8(pb);
    int index_type      = avio_r8(pb);
    int entries_in_use  = avio_rl32(pb);
    int chunk_id        = avio_rl32(pb);
    int64_t base        = avio_rl64(pb);
    int stream_id       = ((chunk_id      & 0xFF) - '0') * 10 +
                          ((chunk_id >> 8 & 0xFF) - '0');
    AVStream *st;
    AVIStream *ast;
    int i;
    int64_t last_pos = -1;
    int64_t filesize = avi->fsize;
    void *data = NULL;
    void *data_ori = NULL;
    int ret = 0;
    int64_t pos2;

    av_log(s, AV_LOG_TRACE,
            "longs_per_entry:%d index_type:%d entries_in_use:%d "
            "chunk_id:%X base:%16"PRIX64" frame_num:%d\n",
            longs_per_entry,
            index_type,
            entries_in_use,
            chunk_id,
            base,
            frame_num);

    if (stream_id >= s->nb_streams || stream_id < 0)
        return AVERROR_INVALIDDATA;
    st  = s->streams[stream_id];
    ast = st->priv_data;

    if (index_sub_type)
        return AVERROR_INVALIDDATA;

    avio_rl32(pb);

    if (index_type && longs_per_entry != 2)
        return AVERROR_INVALIDDATA;
    if (index_type > 1)
        return AVERROR_INVALIDDATA;

    if (filesize > 0 && base >= filesize) {
        av_log(s, AV_LOG_ERROR, "ODML index invalid\n");
        if (base >> 32 == (base & 0xFFFFFFFF) &&
            (base & 0xFFFFFFFF) < filesize    &&
            filesize <= 0xFFFFFFFF)
            base &= 0xFFFFFFFF;
        else
            return AVERROR_INVALIDDATA;
    }

    if (!index_type) {
        data_ori = data = malloc(entries_in_use * 16);
        if (!data)
            return AVERROR_UNKNOWN;
        avio_read(pb, data, entries_in_use * 16);
        if (avio_feof(pb))
            return AVERROR_INVALIDDATA;

        pos2 = avio_tell(pb);
    }

    for (i = 0; i < entries_in_use; i++) {
        if (index_type) {
            int64_t pos = avio_rl32(pb) + base - 8;
            int len     = avio_rl32(pb);
            int key     = len >= 0;
            len &= 0x7FFFFFFF;

            av_log(s, AV_LOG_TRACE, "pos:%"PRId64", len:%X\n", pos, len);


            if (last_pos == pos || pos == base - 8)
                avi->non_interleaved = 1;
            if (last_pos != pos && len)
                av_add_index_entry(st, pos, ast->cum_len, len, 0,
                                   key ? AVINDEX_KEYFRAME : 0);

            ast->cum_len += get_duration(ast, len);
            last_pos      = pos;
        } else {
            int64_t offset;
            int duration;
            offset = AV_RL64(data); data += 8;
            data += 4;       /* size */
            duration = AV_RL32(data); data += 4;

            if (avi->odml_depth > MAX_ODML_DEPTH) {
                av_log(s, AV_LOG_ERROR, "Too deeply nested ODML indexes\n");
                ret = AVERROR_INVALIDDATA;
                goto out;
            }

            if (avio_seek(pb, offset + 8, SEEK_SET) < 0) {
                ret = -1;
                goto out;
            }
            avi->odml_depth++;
            read_odml_index(s, frame_num);
            avi->odml_depth--;
            frame_num += duration;
        }
    }
    //avi->index_loaded = 2;

    if (!index_type) {
        if (avio_seek(pb, pos2, SEEK_SET) < 0) {
            av_log(s, AV_LOG_ERROR, "Failed to restore position after reading index\n");
            ret = -1;
            goto out;
        }
    }

out:
    if (data_ori) {
        free (data_ori);
    }
    return ret;
}

static void clean_index(AVFormatContext *s)
{
    int i;
    int64_t j;

    for (i = 0; i < s->nb_streams; i++) {
        AVStream *st   = s->streams[i];
        AVIStream *ast = st->priv_data;
        int n          = st->nb_index_entries;
        int max        = ast->sample_size;
        int64_t pos, size, ts;

        if (n != 1 || ast->sample_size == 0)
            continue;

        while (max < 1024)
            max += max;

        pos  = st->index_entries[0].pos;
        size = st->index_entries[0].size;
        ts   = st->index_entries[0].timestamp;

        for (j = 0; j < size; j += max)
            av_add_index_entry(st, pos + j, ts + j, FFMIN(max, size - j), 0,
                               AVINDEX_KEYFRAME);
    }
}

static int avi_read_tag(AVFormatContext *s, AVStream *st, uint32_t tag,
                        uint32_t size)
{
    AVIOContext *pb = s->pb;
    char key[5]     = { 0 };
    char *value;

    size += (size & 1);

    if (size == UINT_MAX)
        return AVERROR(EINVAL);
    value = av_malloc(size + 1);
    if (!value)
        return AVERROR(ENOMEM);
    if (avio_read(pb, value, size) != size) {
        av_freep(&value);
        return AVERROR_INVALIDDATA;
    }
    value[size] = 0;

    AV_WL32(key, tag);

    return av_dict_set(st ? &st->metadata : &s->metadata, key, value,
                       AV_DICT_DONT_STRDUP_VAL);
}

static const char months[12][4] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

static void avi_metadata_creation_time(AVDictionary **metadata, char *date)
{
    char month[4], time[9], buffer[64];
    int i, day, year;
    /* parse standard AVI date format (ie. "Mon Mar 10 15:04:43 2003") */
    if (sscanf(date, "%*3s%*[ ]%3s%*[ ]%2d%*[ ]%8s%*[ ]%4d",
               month, &day, time, &year) == 4) {
        for (i = 0; i < 12; i++)
            if (!av_strcasecmp(month, months[i])) {
                snprintf(buffer, sizeof(buffer), "%.4d-%.2d-%.2d %s",
                         year, i + 1, day, time);
                av_dict_set(metadata, "creation_time", buffer, 0);
            }
    } else if (date[4] == '/' && date[7] == '/') {
        date[4] = date[7] = '-';
        av_dict_set(metadata, "creation_time", date, 0);
    }
}

static void avi_read_nikon(AVFormatContext *s, uint64_t end)
{
    while (avio_tell(s->pb) < end && !avio_feof(s->pb)) {
        uint32_t tag  = avio_rl32(s->pb);
        uint32_t size = avio_rl32(s->pb);
        switch (tag) {
        case MKTAG('n', 'c', 't', 'g'):  /* Nikon Tags */
        {
            uint64_t tag_end = avio_tell(s->pb) + size;
            while (avio_tell(s->pb) < tag_end && !avio_feof(s->pb)) {
                uint16_t tag     = avio_rl16(s->pb);
                uint16_t size    = avio_rl16(s->pb);
                const char *name = NULL;
                char buffer[64]  = { 0 };
                size = FFMIN(size, tag_end - avio_tell(s->pb));
                size -= avio_read(s->pb, buffer,
                                  FFMIN(size, sizeof(buffer) - 1));
                switch (tag) {
                case 0x03:
                    name = "maker";
                    break;
                case 0x04:
                    name = "model";
                    break;
                case 0x13:
                    name = "creation_time";
                    if (buffer[4] == ':' && buffer[7] == ':')
                        buffer[4] = buffer[7] = '-';
                    break;
                }
                if (name)
                    av_dict_set(&s->metadata, name, buffer, 0);
                avio_skip(s->pb, size);
            }
            break;
        }
        default:
            avio_skip(s->pb, size);
            break;
        }
    }
}

static int avi_extract_stream_metadata(AVFormatContext *s, AVStream *st)
{
    GetByteContext gb;
    uint8_t *data = st->codecpar->extradata;
    int data_size = st->codecpar->extradata_size;
    int tag, offset;

    if (!data || data_size < 8) {
        return AVERROR_INVALIDDATA;
    }

    bytestream2_init(&gb, data, data_size);

    tag = bytestream2_get_le32(&gb);

    switch (tag) {
    case MKTAG('A', 'V', 'I', 'F'):
        // skip 4 byte padding
        bytestream2_skip(&gb, 4);
        offset = bytestream2_tell(&gb);

        // decode EXIF tags from IFD, AVI is always little-endian
        return avpriv_exif_decode_ifd(s, data + offset, data_size - offset,
                                      1, 0, &st->metadata);
        break;
    case MKTAG('C', 'A', 'S', 'I'):
        avpriv_request_sample(s, "RIFF stream data tag type CASI (%u)", tag);
        break;
    case MKTAG('Z', 'o', 'r', 'a'):
        avpriv_request_sample(s, "RIFF stream data tag type Zora (%u)", tag);
        break;
    default:
        break;
    }

    return 0;
}

static int calculate_bitrate(AVFormatContext *s)
{
    AVIContext *avi = s->priv_data;
    int i, j;
    int64_t lensum = 0;
    int64_t maxpos = 0;
    int64_t aduration = 0;
    int64_t vduration = 0;
    int64_t avdiff = 0;
    int64_t alen = 0;

    for (i = 0; i<s->nb_streams; i++) {
        int64_t len = 0;
        AVStream *st = s->streams[i];

        if (!st->nb_index_entries)
            continue;

        for (j = 0; j < st->nb_index_entries; j++)
            len += st->index_entries[j].size;
        maxpos = FFMAX(maxpos, st->index_entries[j-1].pos);
        lensum += len;
    }
    if (maxpos < av_rescale(avi->io_fsize, 9, 10)) // index does not cover the whole file
        return 0;
    if (lensum*9/10 > maxpos || lensum < maxpos*9/10) // frame sum and filesize mismatch
        return 0;

    for (i = 0; i<s->nb_streams; i++) {
        int64_t len = 0;
        AVStream *st = s->streams[i];
        int64_t duration;
        int64_t bitrate;

        for (j = 0; j < st->nb_index_entries; j++)
            len += st->index_entries[j].size;

        if (s->nb_streams == 2 && st->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            aduration = 1000 * (st->index_entries[j-1].timestamp - st->index_entries[0].timestamp) * av_q2d(st->time_base);
            alen = len;
        } else if (s->nb_streams == 2 && st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            vduration = 1000 * (st->index_entries[j-1].timestamp - st->index_entries[0].timestamp) * av_q2d(st->time_base);
        }

        // special case: one of the time anomalies, select small
        if (s->duration <= 0)
            s->duration = ((aduration > vduration) ? vduration : aduration) * 1000;

        if (st->nb_index_entries < 2 || st->codecpar->bit_rate > 0)
            continue;
        duration = st->index_entries[j-1].timestamp - st->index_entries[0].timestamp;
        if (duration > 0 && st->time_base.num > 0) {
            bitrate = av_rescale(8*len, st->time_base.den, duration * st->time_base.num);
            if (bitrate > 0) {
                st->codecpar->bit_rate = bitrate;
            }
        }
    }

    if (vduration >= 100 && aduration != 0 && alen) {
        avdiff = (vduration > aduration) ? vduration - aduration: aduration - vduration;
        if ((avdiff * 100 /vduration) >= 2) {//2%
            avi->use_external_pts = 1;
#if 0
            int64_t rate;
            for (i = 0; i<s->nb_streams; i++) {
                AVStream *st = s->streams[i];
                if (st->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                    AVIStream *ast = (AVIStream *)st->priv_data;
                    st->codecpar->bit_rate = 8 * alen * 1000 /vduration;
                    ast->rate = st->codecpar->bit_rate /8;
                    avpriv_set_pts_info(st, 64, ast->scale, ast->rate);
                }
            }
#endif
        }
    }


    return 1;
}

#define RETURN_ERROR(code) do { ret = (code); goto fail; } while (0)
static int avi_read_header(AVFormatContext *s)
{
    AVIContext *avi = s->priv_data;
    AVIOContext *pb = s->pb;
    unsigned int tag, tag1, handler;
    int codec_type, stream_index, frame_period;
    unsigned int size;
    int i;
    AVStream *st;
    AVIStream *ast      = NULL;
    int avih_width      = 0, avih_height = 0;
    int amv_file_format = 0;
    uint64_t list_end   = 0;
    int64_t pos;
    int ret;
    AVDictionaryEntry *dict_entry;
    int load_index = 1;
    avi->stream_index = -1;
    avi->use_dmx_pts = 0;

    ret = get_riff(s, pb);
    if (ret < 0)
        return ret;

    av_log(avi, AV_LOG_DEBUG, "use odml:%d\n", avi->use_odml);

    avi->io_fsize = avi->fsize = avio_size(pb);
    if (avi->fsize <= 0 || avi->fsize < avi->riff_end)
        avi->fsize = avi->riff_end == 8 ? INT64_MAX : avi->riff_end;

    /* first list tag */
    stream_index = -1;
    codec_type   = -1;
    frame_period = 0;
    for (;;) {
        if (avio_feof(pb))
            RETURN_ERROR(AVERROR_INVALIDDATA);
        tag  = avio_rl32(pb);
        size = avio_rl32(pb);

        print_tag(s, "tag", tag, size);

        switch (tag) {
        case MKTAG('L', 'I', 'S', 'T'):
            list_end = avio_tell(pb) + size;
            /* Ignored, except at start of video packets. */
            tag1 = avio_rl32(pb);

            print_tag(s, "list", tag1, 0);

            if (tag1 == MKTAG('m', 'o', 'v', 'i')) {
                avi->movi_list = avio_tell(pb) - 4;
                if (size)
                    avi->movi_end = avi->movi_list + size + (size & 1);
                else
                    avi->movi_end = avi->fsize;
                av_log(s, AV_LOG_TRACE, "movi end=%"PRIx64"\n", avi->movi_end);
                goto end_of_header;
            } else if (tag1 == MKTAG('I', 'N', 'F', 'O'))
                ff_read_riff_info(s, size - 4);
            else if (tag1 == MKTAG('n', 'c', 'd', 't'))
                avi_read_nikon(s, list_end);

            break;
        case MKTAG('I', 'D', 'I', 'T'):
        {
            unsigned char date[64] = { 0 };
            size += (size & 1);
            size -= avio_read(pb, date, FFMIN(size, sizeof(date) - 1));
            avio_skip(pb, size);
            avi_metadata_creation_time(&s->metadata, date);
            break;
        }
        case MKTAG('d', 'm', 'l', 'h'):
            avi->is_odml = 1;
            avio_skip(pb, size + (size & 1));
            break;
        case MKTAG('a', 'm', 'v', 'h'):
            amv_file_format = 1;
        case MKTAG('a', 'v', 'i', 'h'):
            /* AVI header */
            /* using frame_period is bad idea */
            frame_period = avio_rl32(pb);
            avio_rl32(pb); /* max. bytes per second */
            avio_rl32(pb);
            avi->non_interleaved |= avio_rl32(pb) & AVIF_MUSTUSEINDEX;

            avio_skip(pb, 2 * 4);
            avio_rl32(pb);
            avio_rl32(pb);
            avih_width  = avio_rl32(pb);
            avih_height = avio_rl32(pb);

            avio_skip(pb, size - 10 * 4);
            break;
        case MKTAG('s', 't', 'r', 'h'):
            /* stream header */

            tag1    = avio_rl32(pb);
            handler = avio_rl32(pb); /* codec tag */

            if (tag1 == MKTAG('p', 'a', 'd', 's')) {
                avio_skip(pb, size - 8);
                break;
            } else {
                stream_index++;
                st = avformat_new_stream(s, NULL);
                if (!st)
                    RETURN_ERROR(AVERROR(ENOMEM));

                st->id = stream_index;
                ast    = av_mallocz(sizeof(AVIStream));
                if (!ast)
                    RETURN_ERROR(AVERROR(ENOMEM));
                st->priv_data = ast;
            }
            if (amv_file_format)
                tag1 = stream_index ? MKTAG('a', 'u', 'd', 's')
                                    : MKTAG('v', 'i', 'd', 's');

            print_tag(s, "strh", tag1, -1);

            if (tag1 == MKTAG('i', 'a', 'v', 's') ||
                tag1 == MKTAG('i', 'v', 'a', 's')) {
                int64_t dv_dur;

                /* After some consideration -- I don't think we
                 * have to support anything but DV in type1 AVIs. */
                if (s->nb_streams != 1)
                    RETURN_ERROR(AVERROR_INVALIDDATA);

                if (handler != MKTAG('d', 'v', 's', 'd') &&
                    handler != MKTAG('d', 'v', 'h', 'd') &&
                    handler != MKTAG('d', 'v', 's', 'l'))
                    return AVERROR_INVALIDDATA;

                if (!CONFIG_DV_DEMUXER)
                    return AVERROR_DEMUXER_NOT_FOUND;

                ast = s->streams[0]->priv_data;
                st->priv_data = NULL;
                ff_free_stream(s, st);

                avi->dv_demux = avpriv_dv_init_demux(s);
                if (!avi->dv_demux) {
                    av_free(ast);
                    return AVERROR(ENOMEM);
                }

                s->streams[0]->priv_data = ast;
                avio_skip(pb, 3 * 4);
                ast->scale = avio_rl32(pb);
                ast->rate  = avio_rl32(pb);
                avio_skip(pb, 4);  /* start time */

                dv_dur = avio_rl32(pb);
                if (ast->scale > 0 && ast->rate > 0 && dv_dur > 0) {
                    dv_dur     *= AV_TIME_BASE;
                    s->duration = av_rescale(dv_dur, ast->scale, ast->rate);
                    if (s->duration > 0) {
                        load_index = 0;
                    }
                }
                /* else, leave duration alone; timing estimation in utils.c
                 * will make a guess based on bitrate. */

                stream_index = s->nb_streams - 1;
                avio_skip(pb, size - 9 * 4);
                break;
            }

            av_assert0(stream_index < s->nb_streams);
            ast->handler = handler;

            avio_rl32(pb); /* flags */
            avio_rl16(pb); /* priority */
            avio_rl16(pb); /* language */
            avio_rl32(pb); /* initial frame */
            ast->scale = avio_rl32(pb);
            ast->rate  = avio_rl32(pb);
            if (!(ast->scale && ast->rate)) {
                av_log(s, AV_LOG_WARNING,
                       "scale/rate is %"PRIu32"/%"PRIu32" which is invalid. "
                       "(This file has been generated by broken software.)\n",
                       ast->scale,
                       ast->rate);
                if (frame_period) {
                    ast->rate  = 1000000;
                    ast->scale = frame_period;
                } else {
                    ast->rate  = 25;
                    ast->scale = 1;
                }
            }
            avpriv_set_pts_info(st, 64, ast->scale, ast->rate);

            ast->cum_len  = avio_rl32(pb); /* start */
            st->nb_frames = avio_rl32(pb);

            st->start_time = 0;
            avio_rl32(pb); /* buffer size */
            avio_rl32(pb); /* quality */
            if (ast->cum_len > 3600LL * ast->rate / ast->scale) {
                av_log(s, AV_LOG_ERROR, "crazy start time, iam scared, giving up\n");
                ast->cum_len = 0;
            }
            ast->sample_size = avio_rl32(pb);
            ast->cum_len    *= FFMAX(1, ast->sample_size);
            av_log(s, AV_LOG_TRACE, "%"PRIu32" %"PRIu32" %d\n",
                    ast->rate, ast->scale, ast->sample_size);

            switch (tag1) {
            case MKTAG('v', 'i', 'd', 's'):
                codec_type = AVMEDIA_TYPE_VIDEO;

                ast->sample_size = 0;
                st->avg_frame_rate = av_inv_q(st->time_base);
                break;
            case MKTAG('a', 'u', 'd', 's'):
                codec_type = AVMEDIA_TYPE_AUDIO;
                break;
            case MKTAG('t', 'x', 't', 's'):
                codec_type = AVMEDIA_TYPE_SUBTITLE;
                break;
            case MKTAG('d', 'a', 't', 's'):
                codec_type = AVMEDIA_TYPE_DATA;
                break;
            default:
                av_log(s, AV_LOG_INFO, "unknown stream type %X\n", tag1);
            }

            if (ast->sample_size < 0) {
                if (s->error_recognition & AV_EF_EXPLODE) {
                    av_log(s, AV_LOG_ERROR,
                           "Invalid sample_size %d at stream %d\n",
                           ast->sample_size,
                           stream_index);
                    RETURN_ERROR(AVERROR_INVALIDDATA);
                }
                av_log(s, AV_LOG_WARNING,
                       "Invalid sample_size %d at stream %d "
                       "setting it to 0\n",
                       ast->sample_size,
                       stream_index);
                ast->sample_size = 0;
            }

            if (ast->sample_size == 0) {
                st->duration = st->nb_frames;
                if (st->duration > 0 && avi->io_fsize > 0 && avi->riff_end > avi->io_fsize) {
                    av_log(s, AV_LOG_DEBUG, "File is truncated adjusting duration\n");
                    st->duration = av_rescale(st->duration, avi->io_fsize, avi->riff_end);
                }
                if (st->duration > 0) {
                    load_index = 0;
                }
            }
            ast->frame_offset = ast->cum_len;
            avio_skip(pb, size - 12 * 4);
            break;
        case MKTAG('s', 't', 'r', 'f'):
            /* stream header */
            if (!size && (codec_type == AVMEDIA_TYPE_AUDIO ||
                          codec_type == AVMEDIA_TYPE_VIDEO))
                break;
            if (stream_index >= (unsigned)s->nb_streams || avi->dv_demux) {
                avio_skip(pb, size);
            } else {
                uint64_t cur_pos = avio_tell(pb);
                unsigned esize;
                if (cur_pos < list_end)
                    size = FFMIN(size, list_end - cur_pos);
                st = s->streams[stream_index];
                if (st->codecpar->codec_type != AVMEDIA_TYPE_UNKNOWN) {
                    avio_skip(pb, size);
                    break;
                }
                switch (codec_type) {
                case AVMEDIA_TYPE_VIDEO:
                    if (amv_file_format) {
                        st->codecpar->width      = avih_width;
                        st->codecpar->height     = avih_height;
                        st->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
                        st->codecpar->codec_id   = AV_CODEC_ID_AMV;
                        avio_skip(pb, size);
                        break;
                    }
                    tag1 = ff_get_bmp_header(pb, st, &esize);

                    if (tag1 == MKTAG('D', 'X', 'S', 'B') ||
                        tag1 == MKTAG('D', 'X', 'S', 'A')) {
                        st->codecpar->codec_type = AVMEDIA_TYPE_SUBTITLE;
                        st->codecpar->codec_tag  = tag1;
                        st->codecpar->codec_id   = AV_CODEC_ID_XSUB;
                        break;
                    }

                    if (size > 10 * 4 && size < (1 << 30) && size < avi->fsize) {
                        if (esize == size-1 && (esize&1)) {
                            st->codecpar->extradata_size = esize - 10 * 4;
                        } else
                            st->codecpar->extradata_size =  size - 10 * 4;
                        if (st->codecpar->extradata) {
                            av_log(s, AV_LOG_WARNING, "New extradata in strf chunk, freeing previous one.\n");
                        }
                        ret = ff_get_extradata(s, st->codecpar, pb,
                                               st->codecpar->extradata_size);
                        if (ret < 0)
                            return ret;
                    }

                    // FIXME: check if the encoder really did this correctly
                    if (st->codecpar->extradata_size & 1)
                        avio_r8(pb);

                    /* Extract palette from extradata if bpp <= 8.
                     * This code assumes that extradata contains only palette.
                     * This is true for all paletted codecs implemented in
                     * FFmpeg. */
                    if (st->codecpar->extradata_size &&
                        (st->codecpar->bits_per_coded_sample <= 8)) {
                        int pal_size = (1 << st->codecpar->bits_per_coded_sample) << 2;
                        const uint8_t *pal_src;

                        pal_size = FFMIN(pal_size, st->codecpar->extradata_size);
                        pal_src  = st->codecpar->extradata +
                                   st->codecpar->extradata_size - pal_size;
                        /* Exclude the "BottomUp" field from the palette */
                        if (pal_src - st->codecpar->extradata >= 9 &&
                            !memcmp(st->codecpar->extradata + st->codecpar->extradata_size - 9, "BottomUp", 9))
                            pal_src -= 9;
                        for (i = 0; i < pal_size / 4; i++)
                            ast->pal[i] = 0xFFU<<24 | AV_RL32(pal_src + 4 * i);
                        ast->has_pal = 1;
                    }

                    print_tag(s, "video", tag1, 0);

                    st->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
                    st->codecpar->codec_tag  = tag1;
                    st->codecpar->codec_id   = ff_codec_get_id(ff_codec_bmp_tags,
                                                            tag1);
                    /* If codec is not found yet, try with the mov tags. */
                    if (!st->codecpar->codec_id) {
                        st->codecpar->codec_id =
                            ff_codec_get_id(ff_codec_movvideo_tags, tag1);
                        if (st->codecpar->codec_id)
                           av_log(s, AV_LOG_WARNING,
                                  "mov tag found in avi (fourcc %s)\n",
                                  av_fourcc2str(tag1));
                    }
                    if (!st->codecpar->codec_id)
                        st->codecpar->codec_id = ff_codec_get_id(ff_codec_bmp_tags_unofficial, tag1);

                    /* This is needed to get the pict type which is necessary
                     * for generating correct pts. */
                    st->need_parsing = AVSTREAM_PARSE_HEADERS;

                    if (st->codecpar->codec_id == AV_CODEC_ID_MPEG4 &&
                        ast->handler == MKTAG('X', 'V', 'I', 'D'))
                        st->codecpar->codec_tag = MKTAG('X', 'V', 'I', 'D');

                    if (st->codecpar->codec_tag == MKTAG('V', 'S', 'S', 'H'))
                        st->need_parsing = AVSTREAM_PARSE_FULL;
                    if (st->codecpar->codec_id == AV_CODEC_ID_RV40)
                        st->need_parsing = AVSTREAM_PARSE_NONE;
                    if (st->codecpar->codec_id == AV_CODEC_ID_HEVC &&
                        st->codecpar->codec_tag == MKTAG('H', '2', '6', '5'))
                        st->need_parsing = AVSTREAM_PARSE_FULL;

                    if (st->codecpar->codec_id  == AV_CODEC_ID_AVRN &&
                        st->codecpar->codec_tag == MKTAG('A', 'V', 'R', 'n') &&
                        (st->codecpar->extradata_size < 31 ||
                          memcmp(&st->codecpar->extradata[28], "1:1", 3)))
                        st->codecpar->codec_id = AV_CODEC_ID_MJPEG;

                    if (st->codecpar->codec_tag == 0 && st->codecpar->height > 0 &&
                        st->codecpar->extradata_size < 1U << 30) {
                        st->codecpar->extradata_size += 9;
                        if ((ret = av_reallocp(&st->codecpar->extradata,
                                               st->codecpar->extradata_size +
                                               AV_INPUT_BUFFER_PADDING_SIZE)) < 0) {
                            st->codecpar->extradata_size = 0;
                            return ret;
                        } else
                            memcpy(st->codecpar->extradata + st->codecpar->extradata_size - 9,
                                   "BottomUp", 9);
                    }
                    st->codecpar->height = FFABS(st->codecpar->height);

//                    avio_skip(pb, size - 5 * 4);
                    break;
                case AVMEDIA_TYPE_AUDIO:
                    ret = ff_get_wav_header(s, pb, st->codecpar, size, 0);
                    if (ret < 0)
                        return ret;
                    ast->dshow_block_align = st->codecpar->block_align;
                    if (ast->sample_size && st->codecpar->block_align &&
                        ast->sample_size != st->codecpar->block_align) {
                        av_log(s,
                               AV_LOG_WARNING,
                               "sample size (%d) != block align (%d)\n",
                               ast->sample_size,
                               st->codecpar->block_align);
                        ast->sample_size = st->codecpar->block_align;
                    }
                    /* 2-aligned
                     * (fix for Stargate SG-1 - 3x18 - Shades of Grey.avi) */
                    if (size & 1)
                        avio_skip(pb, 1);
                    /* Force parsing as several audio frames can be in
                     * one packet and timestamps refer to packet start. */
                    st->need_parsing = AVSTREAM_PARSE_TIMESTAMPS;
                    /* ADTS header is in extradata, AAC without header must be
                     * stored as exact frames. Parser not needed and it will
                     * fail. */
                    if (st->codecpar->codec_id == AV_CODEC_ID_AAC &&
                        st->codecpar->extradata_size)
                        st->need_parsing = AVSTREAM_PARSE_NONE;
                    // The flac parser does not work with AVSTREAM_PARSE_TIMESTAMPS
                    if (st->codecpar->codec_id == AV_CODEC_ID_FLAC)
                        st->need_parsing = AVSTREAM_PARSE_NONE;
                    /* AVI files with Xan DPCM audio (wrongly) declare PCM
                     * audio in the header but have Axan as stream_code_tag. */
                    if (ast->handler == AV_RL32("Axan")) {
                        st->codecpar->codec_id  = AV_CODEC_ID_XAN_DPCM;
                        st->codecpar->codec_tag = 0;
                        ast->dshow_block_align = 0;
                    }
                    if (amv_file_format) {
                        st->codecpar->codec_id    = AV_CODEC_ID_ADPCM_IMA_AMV;
                        ast->dshow_block_align = 0;
                    }
                    if ((st->codecpar->codec_id == AV_CODEC_ID_AAC  ||
                         st->codecpar->codec_id == AV_CODEC_ID_FLAC ||
                         st->codecpar->codec_id == AV_CODEC_ID_MP2 ) && ast->dshow_block_align <= 4 && ast->dshow_block_align) {
                        av_log(s, AV_LOG_DEBUG, "overriding invalid dshow_block_align of %d\n", ast->dshow_block_align);
                        ast->dshow_block_align = 0;
                    }
                    if (st->codecpar->codec_id == AV_CODEC_ID_AAC && ast->dshow_block_align == 1024 && ast->sample_size == 1024 ||
                       st->codecpar->codec_id == AV_CODEC_ID_AAC && ast->dshow_block_align == 4096 && ast->sample_size == 4096 ||
                       st->codecpar->codec_id == AV_CODEC_ID_MP3 && ast->dshow_block_align == 1152 && ast->sample_size == 1152) {
                        av_log(s, AV_LOG_DEBUG, "overriding sample_size\n");
                        ast->sample_size = 0;
                    }
                    break;
                case AVMEDIA_TYPE_SUBTITLE:
                    st->codecpar->codec_type = AVMEDIA_TYPE_SUBTITLE;
                    st->internal->request_probe= 1;
                    avio_skip(pb, size);
                    break;
                default:
                    st->codecpar->codec_type = AVMEDIA_TYPE_DATA;
                    st->codecpar->codec_id   = AV_CODEC_ID_NONE;
                    st->codecpar->codec_tag  = 0;
                    avio_skip(pb, size);
                    break;
                }
            }
            break;
        case MKTAG('s', 't', 'r', 'd'):
            if (stream_index >= (unsigned)s->nb_streams
                || s->streams[stream_index]->codecpar->extradata_size
                || s->streams[stream_index]->codecpar->codec_tag == MKTAG('H','2','6','4')) {
                avio_skip(pb, size);
            } else {
                uint64_t cur_pos = avio_tell(pb);
                if (cur_pos < list_end)
                    size = FFMIN(size, list_end - cur_pos);
                st = s->streams[stream_index];

                if (size<(1<<30)) {
                    if (st->codecpar->extradata) {
                        av_log(s, AV_LOG_WARNING, "New extradata in strd chunk, freeing previous one.\n");
                    }
                    if ((ret = ff_get_extradata(s, st->codecpar, pb, size)) < 0)
                        goto fail;
                }

                if (st->codecpar->extradata_size & 1) //FIXME check if the encoder really did this correctly
                    avio_r8(pb);

                ret = avi_extract_stream_metadata(s, st);
                if (ret < 0) {
                    av_log(s, AV_LOG_WARNING, "could not decoding EXIF data in stream header.\n");
                }
            }
            break;
        case MKTAG('i', 'n', 'd', 'x'):
            pos = avio_tell(pb);
            if (!avi->use_odml) {
                if (avi->odml_node_pos_list) {
                    avi->odml_node_pos_list = av_realloc(avi->odml_node_pos_list,
                        (avi->odml_node_num + 1) * sizeof(int64_t));
                } else {
                    avi->odml_node_pos_list = av_malloc(sizeof(int64_t));
                }
                avi->odml_node_pos_list[avi->odml_node_num] = pos;
                avi->odml_node_num++;
            } else {
                if ((pb->seekable & AVIO_SEEKABLE_NORMAL) && !(s->flags & AVFMT_FLAG_IGNIDX) &&
                    avi->use_odml &&
                    read_odml_index(s, 0) < 0 &&
                    (s->error_recognition & AV_EF_EXPLODE))
                    RETURN_ERROR(AVERROR_INVALIDDATA);
            }
            avio_seek(pb, pos + size, SEEK_SET);
            break;
        case MKTAG('v', 'p', 'r', 'p'):
            if (stream_index < (unsigned)s->nb_streams && size > 9 * 4) {
                AVRational active, active_aspect;

                st = s->streams[stream_index];
                avio_rl32(pb);
                avio_rl32(pb);
                avio_rl32(pb);
                avio_rl32(pb);
                avio_rl32(pb);

                active_aspect.den = avio_rl16(pb);
                active_aspect.num = avio_rl16(pb);
                active.num        = avio_rl32(pb);
                active.den        = avio_rl32(pb);
                avio_rl32(pb); // nbFieldsPerFrame

                if (active_aspect.num && active_aspect.den &&
                    active.num && active.den) {
                    st->sample_aspect_ratio = av_div_q(active_aspect, active);
                    av_log(s, AV_LOG_TRACE, "vprp %d/%d %d/%d\n",
                            active_aspect.num, active_aspect.den,
                            active.num, active.den);
                }
                size -= 9 * 4;
            }
            avio_skip(pb, size);
            break;
        case MKTAG('s', 't', 'r', 'n'):
        case MKTAG('i', 's', 'b', 'j'):
        case MKTAG('i', 'n', 'a', 'm'):
        case MKTAG('i', 'a', 'r', 't'):
        case MKTAG('i', 'c', 'o', 'p'):
        case MKTAG('i', 'c', 'm', 't'):
        case MKTAG('i', 'g', 'n', 'r'):
        case MKTAG('i', 'p', 'o', 'd'):
        case MKTAG('i', 's', 'o', 'f'):
            if (s->nb_streams) {
                ret = avi_read_tag(s, s->streams[s->nb_streams - 1], tag, size);
                if (ret < 0)
                    goto fail;
                break;
            }
        default:
            if (size > 1000000) {
                av_log(s, AV_LOG_ERROR,
                       "Something went wrong during header parsing, "
                       "tag %s has size %u, "
                       "I will ignore it and try to continue anyway.\n",
                       av_fourcc2str(tag), size);
                if (s->error_recognition & AV_EF_EXPLODE)
                    RETURN_ERROR(AVERROR_INVALIDDATA);
                avi->movi_list = avio_tell(pb) - 4;
                avi->movi_end  = avi->fsize;
                goto end_of_header;
            }
        /* Do not fail for very large idx1 tags */
        case MKTAG('i', 'd', 'x', '1'):
            /* skip tag */
            size += (size & 1);
            avio_skip(pb, size);
            break;
        }
    }

end_of_header:
    /* check stream number */
    if (stream_index != s->nb_streams - 1) {
        RETURN_ERROR(AVERROR_INVALIDDATA);
    }

    if (load_index && !avi->index_loaded && (pb->seekable & AVIO_SEEKABLE_NORMAL)) {
        if (!avi->index_loaded && avi->odml_node_num) {
            if (avi_load_odml_index(s) >= 0)
                avi->index_loaded |= 1;
        }
        if (!avi->index_loaded) {
            if(avi_load_index(s) >= 0)
                avi->index_loaded |= 2;
        }
    }
    calculate_bitrate(s);

    //We will check this in the app's buffering. Do not do this check to optimize start play time.
    //if ((ret = guess_ni_flag(s)) < 0)
    //    goto fail;

    avi->non_interleaved |= ret | (s->flags & AVFMT_FLAG_SORT_DTS);
    dict_entry = av_dict_get(s->metadata, "ISFT", NULL, 0);
    if (dict_entry && !strcmp(dict_entry->value, "PotEncoder"))
        for (i = 0; i < s->nb_streams; i++) {
            AVStream *st = s->streams[i];
            if (   st->codecpar->codec_id == AV_CODEC_ID_MPEG1VIDEO
                || st->codecpar->codec_id == AV_CODEC_ID_MPEG2VIDEO)
                st->need_parsing = AVSTREAM_PARSE_FULL;
        }

    for (i = 0; i < s->nb_streams; i++) {
        AVStream *st = s->streams[i];
        if (st->nb_index_entries)
            break;
    }
    // DV-in-AVI cannot be non-interleaved, if set this must be
    // a mis-detection.
    if (avi->dv_demux)
        avi->non_interleaved = 0;
    if (i == s->nb_streams && avi->non_interleaved) {
        av_log(s, AV_LOG_WARNING,
               "Non-interleaved AVI without index, switching to interleaved\n");
        avi->non_interleaved = 0;
    }

    if (avi->non_interleaved) {
        av_log(s, AV_LOG_INFO, "non-interleaved AVI\n");
        clean_index(s);
    }

    ff_metadata_conv_ctx(s, NULL, avi_metadata_conv);
    ff_metadata_conv_ctx(s, NULL, ff_riff_info_conv);

    return 0;
fail:
    avi_read_close(s);
    return ret;
}

static int read_gab2_sub(AVFormatContext *s, AVStream *st, AVPacket *pkt)
{
    if (pkt->size >= 7 &&
        pkt->size < INT_MAX - AVPROBE_PADDING_SIZE &&
        !strcmp(pkt->data, "GAB2") && AV_RL16(pkt->data + 5) == 2) {
        uint8_t desc[256];
        int score      = AVPROBE_SCORE_EXTENSION, ret;
        AVIStream *ast = st->priv_data;
        ff_const59 AVInputFormat *sub_demuxer;
        AVRational time_base;
        int size;
        AVProbeData pd;
        unsigned int desc_len;
        AVIOContext *pb = avio_alloc_context(pkt->data + 7,
                                             pkt->size - 7,
                                             0, NULL, NULL, NULL, NULL);
        if (!pb)
            goto error;

        desc_len = avio_rl32(pb);

        if (desc_len > pb->buf_end - pb->buf_ptr)
            goto error;

        ret = avio_get_str16le(pb, desc_len, desc, sizeof(desc));
        avio_skip(pb, desc_len - ret);
        if (*desc)
            av_dict_set(&st->metadata, "title", desc, 0);

        avio_rl16(pb);   /* flags? */
        avio_rl32(pb);   /* data size */

        size = pb->buf_end - pb->buf_ptr;
        pd = (AVProbeData) { .buf      = av_mallocz(size + AVPROBE_PADDING_SIZE),
                             .buf_size = size };
        if (!pd.buf)
            goto error;
        memcpy(pd.buf, pb->buf_ptr, size);
        sub_demuxer = av_probe_input_format2(&pd, 1, &score);
        av_freep(&pd.buf);
        if (!sub_demuxer)
            goto error;

        if (strcmp(sub_demuxer->name, "srt") && strcmp(sub_demuxer->name, "ass"))
            goto error;

        if (!(ast->sub_pkt = av_packet_alloc()))
            goto error;

        if (!(ast->sub_ctx = avformat_alloc_context()))
            goto error;

        ast->sub_ctx->pb = pb;

        if (ff_copy_whiteblacklists(ast->sub_ctx, s) < 0)
            goto error;

        if (!avformat_open_input(&ast->sub_ctx, "", sub_demuxer, NULL)) {
            if (ast->sub_ctx->nb_streams != 1)
                goto error;
            ff_read_packet(ast->sub_ctx, ast->sub_pkt);
            avcodec_parameters_copy(st->codecpar, ast->sub_ctx->streams[0]->codecpar);
            time_base = ast->sub_ctx->streams[0]->time_base;
            avpriv_set_pts_info(st, 64, time_base.num, time_base.den);
        }
        ast->sub_buffer = pkt->buf;
        pkt->buf = NULL;
        av_packet_unref(pkt);
        return 1;

error:
        av_packet_free(&ast->sub_pkt);
        av_freep(&ast->sub_ctx);
        avio_context_free(&pb);
    }
    return 0;
}

static AVStream *get_subtitle_pkt(AVFormatContext *s, AVStream *next_st,
                                  AVPacket *pkt)
{
    AVIStream *ast, *next_ast = next_st->priv_data;
    int64_t ts, next_ts, ts_min = INT64_MAX;
    AVStream *st, *sub_st = NULL;
    int i;

    next_ts = av_rescale_q(next_ast->frame_offset, next_st->time_base,
                           AV_TIME_BASE_Q);

    for (i = 0; i < s->nb_streams; i++) {
        st  = s->streams[i];
        ast = st->priv_data;
        if (st->discard < AVDISCARD_ALL && ast && ast->sub_pkt && ast->sub_pkt->data) {
            ts = av_rescale_q(ast->sub_pkt->dts, st->time_base, AV_TIME_BASE_Q);
            if (ts <= next_ts && ts < ts_min) {
                ts_min = ts;
                sub_st = st;
            }
        }
    }

    if (sub_st) {
        ast               = sub_st->priv_data;
        av_packet_move_ref(pkt, ast->sub_pkt);
        pkt->stream_index = sub_st->index;

        if (ff_read_packet(ast->sub_ctx, ast->sub_pkt) < 0)
            ast->sub_pkt->data = NULL;
    }
    return sub_st;
}

static int get_stream_idx(const unsigned *d)
{
    if (d[0] >= '0' && d[0] <= '9' &&
        d[1] >= '0' && d[1] <= '9') {
        return (d[0] - '0') * 10 + (d[1] - '0');
    } else {
        return 100; // invalid stream ID
    }
}

/**
 *
 * @param exit_early set to 1 to just gather packet position without making the changes needed to actually read & return the packet
 */
static int avi_sync(AVFormatContext *s, int exit_early)
{
    AVIContext *avi = s->priv_data;
    AVIOContext *pb = s->pb;
    int n;
    unsigned int d[8];
    unsigned int size;
    int64_t i, sync;

start_sync:
    memset(d, -1, sizeof(d));
    for (i = sync = avio_tell(pb); !avio_feof(pb); i++) {
        int j;

        for (j = 0; j < 7; j++)
            d[j] = d[j + 1];
        d[7] = avio_r8(pb);

        size = d[4] + (d[5] << 8) + (d[6] << 16) + (d[7] << 24);

        n = get_stream_idx(d + 2);
        ff_tlog(s, "%X %X %X %X %X %X %X %X %"PRId64" %u %d\n",
                d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], i, size, n);
        if (i*(avi->io_fsize>0) + (uint64_t)size > avi->fsize || d[0] > 127)
            continue;
        if (size > (4 * 1024 * 1024))
            continue;

        // parse ix##
        if ((d[0] == 'i' && d[1] == 'x' && n < s->nb_streams) ||
            // parse JUNK
            (d[0] == 'J' && d[1] == 'U' && d[2] == 'N' && d[3] == 'K') ||
            (d[0] == 'i' && d[1] == 'd' && d[2] == 'x' && d[3] == '1') ||
            (d[0] == 'i' && d[1] == 'n' && d[2] == 'd' && d[3] == 'x')) {
            avio_skip(pb, size);
            goto start_sync;
        }

        // parse stray LIST
        if (d[0] == 'L' && d[1] == 'I' && d[2] == 'S' && d[3] == 'T') {
            avio_skip(pb, 4);
            goto start_sync;
        }

        n = get_stream_idx(d);

        if (!((i - avi->last_pkt_pos) & 1) &&
            get_stream_idx(d + 1) < s->nb_streams)
            continue;

        // detect ##ix chunk and skip
        if (d[2] == 'i' && d[3] == 'x' && n < s->nb_streams) {
            avio_skip(pb, size);
            goto start_sync;
        }

        if (d[2] == 'w' && d[3] == 'c' && n < s->nb_streams) {
            avio_skip(pb, 16 * 3 + 8);
            goto start_sync;
        }

        if (avi->dv_demux && n != 0)
            continue;

        // parse ##dc/##wb
        if (n < s->nb_streams) {
            AVStream *st;
            AVIStream *ast;
            st  = s->streams[n];
            ast = st->priv_data;

            if (!ast) {
                av_log(s, AV_LOG_WARNING, "Skipping foreign stream %d packet\n", n);
                continue;
            }

            if (s->nb_streams >= 2) {
                AVStream *st1   = s->streams[1];
                AVIStream *ast1 = st1->priv_data;
                // workaround for broken small-file-bug402.avi
                if (ast1 && d[2] == 'w' && d[3] == 'b'
                   && n == 0
                   && st ->codecpar->codec_type == AVMEDIA_TYPE_VIDEO
                   && st1->codecpar->codec_type == AVMEDIA_TYPE_AUDIO
                   && ast->prefix == 'd'*256+'c'
                   && (d[2]*256+d[3] == ast1->prefix || !ast1->prefix_count)
                  ) {
                    n   = 1;
                    st  = st1;
                    ast = ast1;
                    av_log(s, AV_LOG_WARNING,
                           "Invalid stream + prefix combination, assuming audio.\n");
                }
            }

            if (d[2] == 'p' && d[3] == 'c' && size <= 4 * 256 + 4) {
                int k    = avio_r8(pb);
                int last = (k + avio_r8(pb) - 1) & 0xFF;

                avio_rl16(pb); // flags

                // b + (g << 8) + (r << 16);
                for (; k <= last; k++)
                    ast->pal[k] = 0xFFU<<24 | avio_rb32(pb)>>8;

                ast->has_pal = 1;
                goto start_sync;
            } else if (((ast->prefix_count < 5 || sync + 9 > i) &&
                        d[2] < 128 && d[3] < 128) ||
                       d[2] * 256 + d[3] == ast->prefix /* ||
                       (d[2] == 'd' && d[3] == 'c') ||
                       (d[2] == 'w' && d[3] == 'b') */) {
                if (exit_early)
                    return 0;
                if (d[2] * 256 + d[3] == ast->prefix)
                    ast->prefix_count++;
                else {
                    ast->prefix       = d[2] * 256 + d[3];
                    ast->prefix_count = 0;
                }

                if (!avi->dv_demux &&
                    ((st->discard >= AVDISCARD_DEFAULT && size == 0) /* ||
                        // FIXME: needs a little reordering
                        (st->discard >= AVDISCARD_NONKEY &&
                        !(pkt->flags & AV_PKT_FLAG_KEY)) */
                    || st->discard >= AVDISCARD_ALL)) {

                    ast->frame_offset += get_duration(ast, size);
                    avio_skip(pb, size);
                    goto start_sync;
                }

                avi->stream_index = n;
                ast->packet_size  = size + 8;
                ast->remaining    = size;

                if (size) {
                    uint64_t pos = avio_tell(pb) - 8;
                    if (!st->index_entries || !st->nb_index_entries ||
                        st->index_entries[st->nb_index_entries - 1].pos < pos) {
                        av_add_index_entry(st, pos, ast->frame_offset, size,
                                           0, AVINDEX_KEYFRAME);
                    }
                }
                return 0;
            }
        }
    }

    if (pb->error)
        return pb->error;
    return AVERROR_EOF;
}

static int ni_prepare_read(AVFormatContext *s)
{
    AVIContext *avi = s->priv_data;
    int best_stream_index = 0;
    AVStream *best_st     = NULL;
    AVIStream *best_ast;
    int64_t best_ts = INT64_MAX;
    int i;

    for (i = 0; i < s->nb_streams; i++) {
        AVStream *st   = s->streams[i];
        AVIStream *ast = st->priv_data;
        int64_t ts     = ast->frame_offset;
        int64_t last_ts;

        if (!st->nb_index_entries)
            continue;

        last_ts = st->index_entries[st->nb_index_entries - 1].timestamp;
        if (!ast->remaining && ts > last_ts)
            continue;

        ts = av_rescale_q(ts, st->time_base,
                          (AVRational) { FFMAX(1, ast->sample_size),
                                         AV_TIME_BASE });

        av_log(s, AV_LOG_TRACE, "%"PRId64" %d/%d %"PRId64"\n", ts,
                st->time_base.num, st->time_base.den, ast->frame_offset);
        //if (i != avi->stream_index) {
        //    ts += 500*1000;//do not do seek for every frame.
        //}
        if (ts < best_ts) {
            best_ts           = ts;
            best_st           = st;
            best_stream_index = i;
        }
    }
    if (!best_st)
        return AVERROR_EOF;

    best_ast = best_st->priv_data;
    best_ts  = best_ast->frame_offset;
    if (best_ast->remaining) {
        i = av_index_search_timestamp(best_st,
                                      best_ts,
                                      AVSEEK_FLAG_ANY |
                                      AVSEEK_FLAG_BACKWARD);
    } else {
        i = av_index_search_timestamp(best_st, best_ts, AVSEEK_FLAG_ANY);
        if (i >= 0)
            best_ast->frame_offset = best_st->index_entries[i].timestamp;
    }

    if (i >= 0) {
        int64_t pos = best_st->index_entries[i].pos;
        pos += best_ast->packet_size - best_ast->remaining;
        if (avio_seek(s->pb, pos + 8, SEEK_SET) < 0)
          return AVERROR_EOF;

        av_assert0(best_ast->remaining <= best_ast->packet_size);

        avi->stream_index = best_stream_index;
        if (!best_ast->remaining)
            best_ast->packet_size =
            best_ast->remaining   = best_st->index_entries[i].size;
    }
    else
        return AVERROR_EOF;

    return 0;
}

static int audio_index_entries_valid_check(AVFormatContext *s)
{
    int i, j;

    for (i = 0; i < s->nb_streams; i++) {
        AVStream *st = s->streams[i];
        AVIStream *ast = st->priv_data;

        printf("st->codecpar->codec_id 0x%x\n", st->codecpar->codec_id);
        //only check mp3/aac/ac3
        if (st->codecpar->codec_id < AV_CODEC_ID_MP2 || st->codecpar->codec_id < AV_CODEC_ID_AC3 ||
            st->nb_index_entries < 4 || st->time_base.den == 0) {
            continue;
        }

        printf("st->nb_index_entries %d\n", st->nb_index_entries);
        //check 3 pkts in the beginig
        for (j = 0; j < 3; j++) {
            int pkt_dur = (st->index_entries[j + 1].timestamp - st->index_entries[j].timestamp) * 1000
                * st->time_base.num / st->time_base.den;
            if (ast->sample_size > 0)
                pkt_dur /= ast->sample_size;
            if (pkt_dur < 50) {
                break;
            }
        }

        printf("j %d, ast->sample_size %d, s->duration %lld\n", j, ast->sample_size, s->duration);
        //duration of the begining pkts is abnormal, check total duration for double check.
        if (j == 3) {
            if (s->duration > 0) {
                int total_dur =  (st->index_entries[st->nb_index_entries - 1].timestamp
                    - st->index_entries[0].timestamp) * 1000 * st->time_base.num
                    / st->time_base.den;
                if (ast->sample_size > 0)
                    total_dur /= ast->sample_size;
                printf("total_dur %d, s->duration %lld\n", total_dur, s->duration);
                if (total_dur > (s->duration / 500)) {
                    return 1;
                }
            }
        }
    }

    return 0;
}

static int avi_read_packet(AVFormatContext *s, AVPacket *pkt)
{
    AVIContext *avi = s->priv_data;
    AVIOContext *pb = s->pb;
    int err;

    if (CONFIG_DV_DEMUXER && avi->dv_demux) {
        int size = avpriv_dv_get_packet(avi->dv_demux, pkt);
        if (size >= 0)
            return size;
        else
            goto resync;
    }

    if (avi->maybe_non_interleaved) {
        if (!avi->index_loaded) {
            if (avi_load_odml_index(s) >= 0)
                avi->index_loaded |= 1;
        }
        if (!avi->index_loaded) {
            /* we only load the index on demand */
            if (avi_load_index(s) >= 0)
                avi->index_loaded |= 2;
        }
        avi->maybe_non_interleaved = 0;
        if (avi->index_loaded) {
            avi->non_interleaved = guess_ni_flag(s);
            if (avi->non_interleaved) {
                avi->aindex_entries_invalid = audio_index_entries_valid_check(s);
                av_log(s, AV_LOG_INFO, "avi->aindex_entries_invalid %d\n", avi->aindex_entries_invalid);
            }
        }
    }

    if (avi->non_interleaved > 0 && !avi->aindex_entries_invalid) {
        err = ni_prepare_read(s);
        if (err < 0)
            return err;
    }

resync:
    if (avi->stream_index >= 0) {
        AVStream *st   = s->streams[avi->stream_index];
        AVIStream *ast = st->priv_data;
        int dv_demux = CONFIG_DV_DEMUXER && avi->dv_demux;
        int size, err;

        if (get_subtitle_pkt(s, st, pkt))
            return 0;

        // minorityreport.AVI block_align=1024 sample_size=1 IMA-ADPCM
        if (ast->sample_size <= 1)
            size = INT_MAX;
        else if (ast->sample_size < 32)
            // arbitrary multiplier to avoid tiny packets for raw PCM data
            size = 1024 * ast->sample_size;
        else
            size = ast->sample_size;

        if (size > ast->remaining)
            size = ast->remaining;
        avi->last_pkt_pos = avio_tell(pb);
        err = av_get_packet(pb, pkt, size);
        if (err < 0)
            return err;
        size = err;

        if (ast->has_pal && pkt->size < (unsigned)INT_MAX / 2 && !dv_demux) {
            uint8_t *pal;
            pal = av_packet_new_side_data(pkt,
                                          AV_PKT_DATA_PALETTE,
                                          AVPALETTE_SIZE);
            if (!pal) {
                av_log(s, AV_LOG_ERROR,
                       "Failed to allocate data for palette\n");
            } else {
                memcpy(pal, ast->pal, AVPALETTE_SIZE);
                ast->has_pal = 0;
            }
        }

        if (CONFIG_DV_DEMUXER && dv_demux) {
            AVBufferRef *avbuf = pkt->buf;
            size = avpriv_dv_produce_packet(avi->dv_demux, pkt,
                                            pkt->data, pkt->size, pkt->pos);
            pkt->buf    = avbuf;
            pkt->flags |= AV_PKT_FLAG_KEY;
            if (size < 0)
                av_packet_unref(pkt);
        } else if (st->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE &&
                   !st->codecpar->codec_tag && read_gab2_sub(s, st, pkt)) {
            ast->frame_offset++;
            avi->stream_index = -1;
            ast->remaining    = 0;
            goto resync;
        } else {
            /* XXX: How to handle B-frames in AVI? */
            pkt->dts = ast->frame_offset;
//                pkt->dts += ast->start;
            if (ast->sample_size)
                pkt->dts /= ast->sample_size;
            pkt->stream_index = avi->stream_index;

            if (st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && st->index_entries) {
                AVIndexEntry *e;
                int index;

                index = av_index_search_timestamp(st, ast->frame_offset, AVSEEK_FLAG_ANY);
                e     = &st->index_entries[index];

                if (index >= 0 && e->timestamp == ast->frame_offset) {
                    if (index == st->nb_index_entries-1) {
                        int key=1;
                        uint32_t state=-1;
                        if (st->codecpar->codec_id == AV_CODEC_ID_MPEG4) {
                            const uint8_t *ptr = pkt->data, *end = ptr + FFMIN(size, 256);
                            while (ptr < end) {
                                ptr = avpriv_find_start_code(ptr, end, &state);
                                if (state == 0x1B6 && ptr < end) {
                                    key = !(*ptr & 0xC0);
                                    break;
                                }
                            }
                        }
                        if (!key)
                            e->flags &= ~AVINDEX_KEYFRAME;
                    }
                    if (e->flags & AVINDEX_KEYFRAME)
                        pkt->flags |= AV_PKT_FLAG_KEY;
                }
            } else {
                pkt->flags |= AV_PKT_FLAG_KEY;
            }
            ast->frame_offset += get_duration(ast, pkt->size);
        }
        ast->remaining -= err;
        if (!ast->remaining) {
            avi->stream_index = -1;
            ast->packet_size  = 0;
        }

        if (!avi->non_interleaved && pkt->pos >= 0 && ast->seek_pos > pkt->pos) {
            av_packet_unref(pkt);
            goto resync;
        }
        ast->seek_pos= 0;

        if (!avi->non_interleaved && st->nb_index_entries>1 && avi->index_loaded>1) {
            int64_t dts= av_rescale_q(pkt->dts, st->time_base, AV_TIME_BASE_Q);

            if (avi->dts_max < dts) {
                avi->dts_max = dts;
            } else if (avi->dts_max - (uint64_t)dts > 2*AV_TIME_BASE) {
                avi->non_interleaved = 1;
                av_log(s, AV_LOG_INFO, "Switching to NI mode, due to poor interleaving\n");
            }
        }

        //if (avi->use_external_pts) {
            if (st->codecpar->codec_id == AV_CODEC_ID_MP3 ||
                st->codecpar->codec_id == AV_CODEC_ID_MP2) {
                if (avi->use_dmx_pts) {
                    pkt->dts = pkt->pts = AV_NOPTS_VALUE;
                } else {
                    avi->use_dmx_pts = 1;
                }
            }
        //}

        return 0;
    }

    if ((err = avi_sync(s, 0)) < 0)
        return err;
    goto resync;
}

/* XXX: We make the implicit supposition that the positions are sorted
 * for each stream. */
static int avi_read_idx1(AVFormatContext *s, int size)
{
    AVIContext *avi = s->priv_data;
    AVIOContext *pb = s->pb;
    int nb_index_entries, i;
    AVStream *st;
    AVIStream *ast;
    int64_t pos;
    unsigned int index, tag, flags, len, first_packet = 1;
    int64_t last_pos = -1;
    unsigned last_idx = -1;
    int64_t idx1_pos, first_packet_pos = 0, data_offset = 0;
    int anykey = 0;

    nb_index_entries = size / 16;
    if (nb_index_entries <= 0)
        return AVERROR_INVALIDDATA;

    idx1_pos = avio_tell(pb);
    avio_seek(pb, avi->movi_list + 4, SEEK_SET);
    if (avi_sync(s, 1) == 0)
        first_packet_pos = avio_tell(pb) - 8;
    avi->stream_index = -1;
    avio_seek(pb, idx1_pos, SEEK_SET);

    if (s->nb_streams == 1 && s->streams[0]->codecpar->codec_tag == AV_RL32("MMES")) {
        first_packet_pos = 0;
        data_offset = avi->movi_list;
    }

    /* Read the entries and sort them in each stream component. */
    for (i = 0; i < nb_index_entries; i++) {
        if (avio_feof(pb))
            return -1;

        tag   = avio_rl32(pb);
        flags = avio_rl32(pb);
        pos   = avio_rl32(pb);
        len   = avio_rl32(pb);
        av_log(s, AV_LOG_TRACE, "%d: tag=0x%x flags=0x%x pos=0x%"PRIx64" len=%d/",
                i, tag, flags, pos, len);

        index  = ((tag      & 0xff) - '0') * 10;
        index +=  (tag >> 8 & 0xff) - '0';
        if (index >= s->nb_streams)
            continue;
        st  = s->streams[index];
        ast = st->priv_data;

        /* Skip 'xxpc' palette change entries in the index until a logic
         * to process these is properly implemented. */
        if ((tag >> 16 & 0xff) == 'p' && (tag >> 24 & 0xff) == 'c')
            continue;

        if (first_packet && first_packet_pos) {
            if (avi->movi_list + 4 != pos || pos + 500 > first_packet_pos)
                data_offset  = first_packet_pos - pos;
            first_packet = 0;
        }
        pos += data_offset;

        av_log(s, AV_LOG_TRACE, "%d cum_len=%"PRId64"\n", len, ast->cum_len);

        // even if we have only a single stream, we should
        // switch to non-interleaved to get correct timestamps
        if (last_pos == pos)
            avi->non_interleaved = 1;
        if (last_idx != pos && len) {
            av_add_index_entry(st, pos, ast->cum_len, len, 0,
                               (flags & AVIIF_INDEX) ? AVINDEX_KEYFRAME : 0);
            last_idx= pos;
        }
        ast->cum_len += get_duration(ast, len);
        last_pos      = pos;
        anykey       |= flags&AVIIF_INDEX;
    }
    if (!anykey) {
        for (index = 0; index < s->nb_streams; index++) {
            st = s->streams[index];
            if (st->nb_index_entries)
                st->index_entries[0].flags |= AVINDEX_KEYFRAME;
        }
    }
    return 0;
}

/* Scan the index and consider any file with streams more than
 * 2 seconds or 64MB apart non-interleaved. */
static int check_stream_max_drift(AVFormatContext *s)
{
    int64_t min_pos, pos;
    int i;
    int *idx = av_mallocz_array(s->nb_streams, sizeof(*idx));
    if (!idx)
        return AVERROR(ENOMEM);
    for (min_pos = pos = 0; min_pos != INT64_MAX; pos = min_pos + 1LU) {
        int64_t max_dts = INT64_MIN / 2;
        int64_t min_dts = INT64_MAX / 2;
        int64_t max_buffer = 0;

        min_pos = INT64_MAX;

        for (i = 0; i < s->nb_streams; i++) {
            AVStream *st = s->streams[i];
            AVIStream *ast = st->priv_data;
            //int n = st->nb_index_entries;
            int n;
            if (st->nb_index_entries > 100000)
                n = st->nb_index_entries / 100;
            else if (st->nb_index_entries > 10000)
                n = st->nb_index_entries / 10;
            else
                n = st->nb_index_entries;
            while (idx[i] < n && st->index_entries[idx[i]].pos < pos)
                idx[i]++;
            if (idx[i] < n) {
                int64_t dts;
                dts = av_rescale_q(st->index_entries[idx[i]].timestamp /
                                   FFMAX(ast->sample_size, 1),
                                   st->time_base, AV_TIME_BASE_Q);
                min_dts = FFMIN(min_dts, dts);
                min_pos = FFMIN(min_pos, st->index_entries[idx[i]].pos);
            }
        }
        for (i = 0; i < s->nb_streams; i++) {
            AVStream *st = s->streams[i];
            AVIStream *ast = st->priv_data;

            if (idx[i] && min_dts != INT64_MAX / 2) {
                int64_t dts, delta_dts;
                dts = av_rescale_q(st->index_entries[idx[i] - 1].timestamp /
                                   FFMAX(ast->sample_size, 1),
                                   st->time_base, AV_TIME_BASE_Q);
                delta_dts = av_sat_sub64(dts, min_dts);
                max_dts = FFMAX(max_dts, dts);
                max_buffer = FFMAX(max_buffer,
                                   av_rescale(delta_dts,
                                              st->codecpar->bit_rate,
                                              AV_TIME_BASE));
            }
        }
        if (av_sat_sub64(max_dts, min_dts) > 2 * AV_TIME_BASE ||
            max_buffer > 1024 * 1024 * 8) {
            av_free(idx);
            return 1;
        }
    }
    av_free(idx);
    return 0;
}

static int guess_ni_flag(AVFormatContext *s)
{
    int i;
    int64_t last_start = 0;
    int64_t first_end  = INT64_MAX;
    int64_t oldpos     = avio_tell(s->pb);

    for (i = 0; i < s->nb_streams; i++) {
        AVStream *st = s->streams[i];
        int n        = st->nb_index_entries;
        unsigned int size;

        if (n <= 0)
            continue;

        if (n >= 2) {
            int64_t pos = st->index_entries[0].pos;
            unsigned tag[2];
            avio_seek(s->pb, pos, SEEK_SET);
            tag[0] = avio_r8(s->pb);
            tag[1] = avio_r8(s->pb);
            avio_rl16(s->pb);
            size = avio_rl32(s->pb);
            if (get_stream_idx(tag) == i && pos + size > st->index_entries[1].pos)
                last_start = INT64_MAX;
            if (get_stream_idx(tag) == i && size == st->index_entries[0].size + 8)
                last_start = INT64_MAX;
        }

        if (st->index_entries[0].pos > last_start)
            last_start = st->index_entries[0].pos;
        if (st->index_entries[n - 1].pos < first_end)
            first_end = st->index_entries[n - 1].pos;
    }
    avio_seek(s->pb, oldpos, SEEK_SET);

    if (last_start > first_end)
        return 1;

    return check_stream_max_drift(s);
}

static int avi_load_odml_index(AVFormatContext *s)
{
    int ret = -1, n;
    AVIContext *avi = s->priv_data;
    AVIOContext *pb = s->pb;
    int64_t pos;

    if (avi->odml_node_num <= 0) {
        return ret;
    }

    s->not_interruptible = 1;
    pos = avio_tell(pb);
    for (n = 0; n < avi->odml_node_num; n++) {
        avio_seek(pb, avi->odml_node_pos_list[n], SEEK_SET);
        ret = read_odml_index(s, 0);
        if (ret < 0) {
            goto the_end;
        }
    }

    for (n = 0; n < s->nb_streams; n++) {
        if (s->streams[n]->codecpar && s->streams[n]->codecpar->codec_type != AVMEDIA_TYPE_VIDEO) {
            continue;
        }
        if (s->streams[n]->nb_frames > s->streams[n]->nb_index_entries) {
            ret = -1;
        }
    }

the_end:
    s->not_interruptible = 0;
    avio_seek(pb, pos, SEEK_SET);
    return ret;
}

static int avi_load_index(AVFormatContext *s)
{
    AVIContext *avi = s->priv_data;
    AVIOContext *pb = s->pb;
    uint32_t tag, size;
    uint32_t junknum = 0;
    int64_t pos = avio_tell(pb);
    int64_t next;
    int ret     = -1;

    if (avio_seek(pb, avi->movi_end, SEEK_SET) < 0)
        goto the_end; // maybe truncated file

    s->not_interruptible = 1;
    av_log(s, AV_LOG_TRACE, "movi_end=0x%"PRIx64"\n", avi->movi_end);
    for (;;) {
        tag  = avio_rl32(pb);
        size = avio_rl32(pb);
        if (avio_feof(pb))
            break;
        next = avio_tell(pb) + size + (size & 1);

        if (tag == MKTAG('i', 'd', 'x', '1') &&
            avi_read_idx1(s, size) >= 0) {
            ret = 0;
        } else if (tag == MKTAG('L', 'I', 'S', 'T')) {
            uint32_t tag1 = avio_rl32(pb);

            if (tag1 == MKTAG('I', 'N', 'F', 'O'))
                ff_read_riff_info(s, size - 4);
        } else if (!ret)
            break;

        if (tag == 0 && size == 0)
            junknum++;

        if (avio_seek(pb, next, SEEK_SET) < 0 || junknum >= 1310720)
            break; // something is wrong here
    }

the_end:
    s->not_interruptible = 0;
    avio_seek(pb, pos, SEEK_SET);
    return ret;
}

static void seek_subtitle(AVStream *st, AVStream *st2, int64_t timestamp)
{
    AVIStream *ast2 = st2->priv_data;
    int64_t ts2     = av_rescale_q(timestamp, st->time_base, st2->time_base);
    av_packet_unref(ast2->sub_pkt);
    if (avformat_seek_file(ast2->sub_ctx, 0, INT64_MIN, ts2, ts2, 0) >= 0 ||
        avformat_seek_file(ast2->sub_ctx, 0, ts2, ts2, INT64_MAX, 0) >= 0)
        ff_read_packet(ast2->sub_ctx, ast2->sub_pkt);
}

static int avi_read_seek_check(AVFormatContext *s)
{
    AVIContext *avi = s->priv_data;

    if (avi->index_scaned) {
        if (!avi->index_loaded) {
            return AVERROR_INVALIDDATA;
        }
        return 0;
    }

    if (!avi->index_loaded && avi->odml_node_num) {
        if (avi_load_odml_index(s) >= 0)
            avi->index_loaded |= 1;
    }

    if (!avi->index_loaded) {
        /* we only load the index on demand */
        if (avi_load_index(s) >= 0)
            avi->index_loaded |= 2;
    }

    if (avi->index_loaded)
        avi->non_interleaved |= guess_ni_flag(s);

    if (avi->non_interleaved) {
        avi->aindex_entries_invalid = audio_index_entries_valid_check(s);
        av_log(s, AV_LOG_INFO, "avi->aindex_entries_invalid %d\n", avi->aindex_entries_invalid);
    }

    avi->index_scaned = 1;

    if (!avi->index_loaded) {
        return AVERROR_INVALIDDATA;
    }

    return 0;
}

static int avi_read_seek(AVFormatContext *s, int stream_index,
                         int64_t timestamp, int flags)
{
    AVIContext *avi = s->priv_data;
    AVStream *st;
    int i, index;
    int64_t pos, pos_min;
    AVIStream *ast;

    /* Does not matter which stream is requested dv in avi has the
     * stream information in the first video stream.
     */
    if (avi->dv_demux)
        stream_index = 0;

    if (avi_read_seek_check(s) != 0) {
        return AVERROR_INVALIDDATA;
    }

    av_assert0(stream_index >= 0);

    st    = s->streams[stream_index];
    ast   = st->priv_data;
    index = av_index_search_timestamp(st,
                                      timestamp * FFMAX(ast->sample_size, 1),
                                      flags);
    if (index < 0) {
        if (st->nb_index_entries > 0)
            av_log(s, AV_LOG_DEBUG, "Failed to find timestamp %"PRId64 " in index %"PRId64 " .. %"PRId64 "\n",
                   timestamp * FFMAX(ast->sample_size, 1),
                   st->index_entries[0].timestamp,
                   st->index_entries[st->nb_index_entries - 1].timestamp);
        return AVERROR_INVALIDDATA;
    }

    /* find the position */
    pos       = st->index_entries[index].pos;
    timestamp = st->index_entries[index].timestamp / FFMAX(ast->sample_size, 1);

    av_log(s, AV_LOG_TRACE, "XX %"PRId64" %d %"PRId64"\n",
            timestamp, index, st->index_entries[index].timestamp);

    if (CONFIG_DV_DEMUXER && avi->dv_demux) {
        /* One and only one real stream for DV in AVI, and it has video  */
        /* offsets. Calling with other stream indexes should have failed */
        /* the av_index_search_timestamp call above.                     */

        if (avio_seek(s->pb, pos, SEEK_SET) < 0)
            return -1;

        /* Feed the DV video stream version of the timestamp to the */
        /* DV demux so it can synthesize correct timestamps.        */
        ff_dv_offset_reset(avi->dv_demux, timestamp);

        avi->stream_index = -1;
        avi->use_dmx_pts = 0;
        return 0;
    }

    pos_min = pos;
    for (i = 0; i < s->nb_streams; i++) {
        AVStream *st2   = s->streams[i];
        AVIStream *ast2 = st2->priv_data;

        if (st2->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && avi->aindex_entries_invalid)
            continue;

        ast2->packet_size =
        ast2->remaining   = 0;

        if (ast2->sub_ctx) {
            seek_subtitle(st, st2, timestamp);
            continue;
        }

        if (st2->nb_index_entries <= 0)
            continue;

//        av_assert1(st2->codecpar->block_align);
        index = av_index_search_timestamp(st2,
                                          av_rescale_q(timestamp,
                                                       st->time_base,
                                                       st2->time_base) *
                                          FFMAX(ast2->sample_size, 1),
                                          flags |
                                          AVSEEK_FLAG_BACKWARD |
                                          (st2->codecpar->codec_type != AVMEDIA_TYPE_VIDEO ? AVSEEK_FLAG_ANY : 0));
        if (index < 0)
            index = 0;
        ast2->seek_pos = st2->index_entries[index].pos;
        pos_min = FFMIN(pos_min,ast2->seek_pos);
    }
    for (i = 0; i < s->nb_streams; i++) {
        AVStream *st2 = s->streams[i];
        AVIStream *ast2 = st2->priv_data;

        if (st2->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && avi->aindex_entries_invalid)
            continue;

        if (ast2->sub_ctx || st2->nb_index_entries <= 0)
            continue;

        index = av_index_search_timestamp(
                st2,
                av_rescale_q(timestamp, st->time_base, st2->time_base) * FFMAX(ast2->sample_size, 1),
                flags | AVSEEK_FLAG_BACKWARD | (st2->codecpar->codec_type != AVMEDIA_TYPE_VIDEO ? AVSEEK_FLAG_ANY : 0));
        if (index < 0)
            index = 0;
        while (!avi->non_interleaved && index>0 && st2->index_entries[index-1].pos >= pos_min)
            index--;
        ast2->frame_offset = st2->index_entries[index].timestamp;
    }

    /* do the seek */
    if (avio_seek(s->pb, pos_min, SEEK_SET) < 0) {
        av_log(s, AV_LOG_ERROR, "Seek failed\n");
        return -1;
    }
    avi->stream_index = -1;
    avi->dts_max      = INT_MIN;
    avi->use_dmx_pts = 0;
    return 0;
}

static int avi_read_close(AVFormatContext *s)
{
    int i;
    AVIContext *avi = s->priv_data;

    for (i = 0; i < s->nb_streams; i++) {
        AVStream *st   = s->streams[i];
        AVIStream *ast = st->priv_data;
        if (ast) {
            if (ast->sub_ctx) {
                av_freep(&ast->sub_ctx->pb);
                avformat_close_input(&ast->sub_ctx);
            }
            av_buffer_unref(&ast->sub_buffer);
            av_packet_free(&ast->sub_pkt);
        }
    }

    if (avi->odml_node_pos_list)
        av_freep(&avi->odml_node_pos_list);
    av_freep(&avi->dv_demux);

    return 0;
}

static int avi_probe(const AVProbeData *p)
{
    int i;

    /* check file header */
    for (i = 0; avi_headers[i][0]; i++)
        if (AV_RL32(p->buf    ) == AV_RL32(avi_headers[i]    ) &&
            AV_RL32(p->buf + 8) == AV_RL32(avi_headers[i] + 4))
            return AVPROBE_SCORE_MAX;

    return 0;
}

AVInputFormat ff_avi_demuxer = {
    .name           = "avi",
    .long_name      = NULL_IF_CONFIG_SMALL("AVI (Audio Video Interleaved)"),
    .priv_data_size = sizeof(AVIContext),
    .extensions     = "avi",
    .read_probe     = avi_probe,
    .read_header    = avi_read_header,
    .read_packet    = avi_read_packet,
    .read_close     = avi_read_close,
    .read_seek      = avi_read_seek,
    .seek_check     = avi_read_seek_check,
    .priv_class = &demuxer_class,
};
