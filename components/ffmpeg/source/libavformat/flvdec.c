/*
 * FLV demuxer
 * Copyright (c) 2003 The FFmpeg Project
 *
 * This demuxer will generate a 1 byte extradata for VP6F content.
 * It is composed of:
 *  - upper 4 bits: difference between encoded width and visible width
 *  - lower 4 bits: difference between encoded height and visible height
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

#include "libavformat/avio_internal.h"
#include "libavutil/avstring.h"
#include "libavutil/channel_layout.h"
#include "libavutil/dict.h"
#include "libavutil/opt.h"
#include "libavutil/intfloat.h"
#include "libavutil/mathematics.h"
#include "libavutil/time_internal.h"
#include "libavcodec/bytestream.h"
#include "avformat.h"
#include "internal.h"
#include "avio_internal.h"
#include "flv.h"

#define VALIDATE_INDEX_TS_THRESH 2500

#define RESYNC_BUFFER_SIZE (1<<20)

#define MAX_DEPTH 16      ///< arbitrary limit to prevent unbounded recursion

typedef struct FLVContext {
    const AVClass *class; ///< Class for private options.
    int trust_metadata;   ///< configure streams according onMetaData
    int trust_datasize;   ///< trust data size of FLVTag
    int dump_full_metadata;   ///< Dump full metadata of the onMetadata
    int wrong_dts;        ///< wrong dts due to negative cts
    uint8_t *new_extradata[FLV_STREAM_TYPE_NB];
    int new_extradata_size[FLV_STREAM_TYPE_NB];
    int last_sample_rate;
    int last_channels;
    struct {
        int64_t dts;
        int64_t pos;
    } validate_index[2];
    int validate_next;
    int validate_count;
    int searched_for_end;

    uint8_t resync_buffer[2*RESYNC_BUFFER_SIZE];

    int broken_sizes;
    int sum_flv_tag_size;

    int last_keyframe_stream_index;
    int keyframe_count;
    int64_t video_bit_rate;
    int64_t audio_bit_rate;
    int64_t *keyframe_times;
    int64_t *keyframe_filepositions;
    int missing_streams;
    AVRational framerate;
    int64_t last_ts;
    int64_t time_offset;
    int64_t time_pos;
    int has_index_entries;
} FLVContext;

/* AMF date type */
typedef struct amf_date {
    double   milliseconds;
    int16_t  timezone;
} amf_date;

static int probe(const AVProbeData *p, int live)
{
    const uint8_t *d = p->buf;
    unsigned offset = AV_RB32(d + 5);

    if (d[0] == 'F' &&
        d[1] == 'L' &&
        d[2] == 'V' &&
        d[3] < 5 && d[5] == 0 &&
        offset + 100 < p->buf_size &&
        offset > 8) {
        int is_live = !memcmp(d + offset + 40, "NGINX RTMP", 10);

        if (live == is_live)
            return AVPROBE_SCORE_MAX;
    }
    return 0;
}

static int flv_probe(const AVProbeData *p)
{
    return probe(p, 0);
}

static int live_flv_probe(const AVProbeData *p)
{
    return probe(p, 1);
}

static int kux_probe(const AVProbeData *p)
{
    const uint8_t *d = p->buf;

    if (d[0] == 'K' &&
        d[1] == 'D' &&
        d[2] == 'K' &&
        d[3] == 0 &&
        d[4] == 0) {
        return AVPROBE_SCORE_EXTENSION + 1;
    }
    return 0;
}

static void add_keyframes_index(AVFormatContext *s)
{
    FLVContext *flv   = s->priv_data;
    AVStream *stream  = NULL;
    unsigned int i    = 0;

    if (flv->last_keyframe_stream_index < 0) {
        av_log(s, AV_LOG_DEBUG, "keyframe stream hasn't been created\n");
        return;
    }

    av_assert0(flv->last_keyframe_stream_index <= s->nb_streams);
    stream = s->streams[flv->last_keyframe_stream_index];

    if (stream->nb_index_entries == 0) {
        if (flv->keyframe_count && stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            flv->has_index_entries = 1;
        }
        for (i = 0; i < flv->keyframe_count; i++) {
            av_log(s, AV_LOG_TRACE, "keyframe filepositions = %"PRId64" times = %"PRId64"\n",
                   flv->keyframe_filepositions[i], flv->keyframe_times[i] * 1000);
            av_add_index_entry(stream, flv->keyframe_filepositions[i],
                flv->keyframe_times[i] * 1000, 0, 0, AVINDEX_KEYFRAME);
        }
    } else
        av_log(s, AV_LOG_WARNING, "Skipping duplicate index\n");

    if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        av_freep(&flv->keyframe_times);
        av_freep(&flv->keyframe_filepositions);
        flv->keyframe_count = 0;
    }
}

static AVStream *create_stream(AVFormatContext *s, int codec_type)
{
    FLVContext *flv   = s->priv_data;
    AVStream *st = avformat_new_stream(s, NULL);
    if (!st)
        return NULL;
    st->codecpar->codec_type = codec_type;
    if (s->nb_streams>=3 ||(   s->nb_streams==2
                           && s->streams[0]->codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE
                           && s->streams[1]->codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE
                           && s->streams[0]->codecpar->codec_type != AVMEDIA_TYPE_DATA
                           && s->streams[1]->codecpar->codec_type != AVMEDIA_TYPE_DATA))
        s->ctx_flags &= ~AVFMTCTX_NOHEADER;
    if (codec_type == AVMEDIA_TYPE_AUDIO) {
        st->codecpar->bit_rate = flv->audio_bit_rate;
        flv->missing_streams &= ~FLV_HEADER_FLAG_HASAUDIO;
    }
    if (codec_type == AVMEDIA_TYPE_VIDEO) {
        st->codecpar->bit_rate = flv->video_bit_rate;
        flv->missing_streams &= ~FLV_HEADER_FLAG_HASVIDEO;
        st->avg_frame_rate = flv->framerate;
    }


    avpriv_set_pts_info(st, 32, 1, 1000); /* 32 bit pts in ms */
    flv->last_keyframe_stream_index = s->nb_streams - 1;
    add_keyframes_index(s);
    return st;
}

static int flv_same_audio_codec(AVCodecParameters *apar, int flags)
{
    int bits_per_coded_sample = (flags & FLV_AUDIO_SAMPLESIZE_MASK) ? 16 : 8;
    int flv_codecid           = flags & FLV_AUDIO_CODECID_MASK;
    int codec_id;

    if (!apar->codec_id && !apar->codec_tag)
        return 1;

    if (apar->bits_per_coded_sample != bits_per_coded_sample)
        return 0;

    switch (flv_codecid) {
    // no distinction between S16 and S8 PCM codec flags
    case FLV_CODECID_PCM:
        codec_id = bits_per_coded_sample == 8
                   ? AV_CODEC_ID_PCM_U8
#if HAVE_BIGENDIAN
                   : AV_CODEC_ID_PCM_S16BE;
#else
                   : AV_CODEC_ID_PCM_S16LE;
#endif
        return codec_id == apar->codec_id;
    case FLV_CODECID_PCM_LE:
        codec_id = bits_per_coded_sample == 8
                   ? AV_CODEC_ID_PCM_U8
                   : AV_CODEC_ID_PCM_S16LE;
        return codec_id == apar->codec_id;
    case FLV_CODECID_AAC:
        return apar->codec_id == AV_CODEC_ID_AAC;
    case FLV_CODECID_ADPCM:
        return apar->codec_id == AV_CODEC_ID_ADPCM_SWF;
    case FLV_CODECID_SPEEX:
        return apar->codec_id == AV_CODEC_ID_SPEEX;
    case FLV_CODECID_MP3:
        return apar->codec_id == AV_CODEC_ID_MP3;
    case FLV_CODECID_NELLYMOSER_8KHZ_MONO:
    case FLV_CODECID_NELLYMOSER_16KHZ_MONO:
    case FLV_CODECID_NELLYMOSER:
        return apar->codec_id == AV_CODEC_ID_NELLYMOSER;
    case FLV_CODECID_PCM_MULAW:
        return apar->sample_rate == 8000 &&
               apar->codec_id    == AV_CODEC_ID_PCM_MULAW;
    case FLV_CODECID_PCM_ALAW:
        return apar->sample_rate == 8000 &&
               apar->codec_id    == AV_CODEC_ID_PCM_ALAW;
    default:
        return apar->codec_tag == (flv_codecid >> FLV_AUDIO_CODECID_OFFSET);
    }
}

static void flv_set_audio_codec(AVFormatContext *s, AVStream *astream,
                                AVCodecParameters *apar, int flv_codecid)
{
    switch (flv_codecid) {
    // no distinction between S16 and S8 PCM codec flags
    case FLV_CODECID_PCM:
        apar->codec_id = apar->bits_per_coded_sample == 8
                           ? AV_CODEC_ID_PCM_U8
#if HAVE_BIGENDIAN
                           : AV_CODEC_ID_PCM_S16BE;
#else
                           : AV_CODEC_ID_PCM_S16LE;
#endif
        break;
    case FLV_CODECID_PCM_LE:
        apar->codec_id = apar->bits_per_coded_sample == 8
                           ? AV_CODEC_ID_PCM_U8
                           : AV_CODEC_ID_PCM_S16LE;
        break;
    case FLV_CODECID_AAC:
        apar->codec_id = AV_CODEC_ID_AAC;
        break;
    case FLV_CODECID_ADPCM:
        apar->codec_id = AV_CODEC_ID_ADPCM_SWF;
        break;
    case FLV_CODECID_SPEEX:
        apar->codec_id    = AV_CODEC_ID_SPEEX;
        apar->sample_rate = 16000;
        break;
    case FLV_CODECID_MP3:
        apar->codec_id      = AV_CODEC_ID_MP3;
        astream->need_parsing = AVSTREAM_PARSE_FULL;
        break;
    case FLV_CODECID_NELLYMOSER_8KHZ_MONO:
        // in case metadata does not otherwise declare samplerate
        apar->sample_rate = 8000;
        apar->codec_id    = AV_CODEC_ID_NELLYMOSER;
        break;
    case FLV_CODECID_NELLYMOSER_16KHZ_MONO:
        apar->sample_rate = 16000;
        apar->codec_id    = AV_CODEC_ID_NELLYMOSER;
        break;
    case FLV_CODECID_NELLYMOSER:
        apar->codec_id = AV_CODEC_ID_NELLYMOSER;
        break;
    case FLV_CODECID_PCM_MULAW:
        apar->sample_rate = 8000;
        apar->codec_id    = AV_CODEC_ID_PCM_MULAW;
        break;
    case FLV_CODECID_PCM_ALAW:
        apar->sample_rate = 8000;
        apar->codec_id    = AV_CODEC_ID_PCM_ALAW;
        break;
    default:
        avpriv_request_sample(s, "Audio codec (%x)",
               flv_codecid >> FLV_AUDIO_CODECID_OFFSET);
        apar->codec_tag = flv_codecid >> FLV_AUDIO_CODECID_OFFSET;
    }
}

static int flv_same_video_codec(AVCodecParameters *vpar, int flags)
{
    int flv_codecid = flags & FLV_VIDEO_CODECID_MASK;

    if (!vpar->codec_id && !vpar->codec_tag)
        return 1;

    switch (flv_codecid) {
    case FLV_CODECID_H263:
        return vpar->codec_id == AV_CODEC_ID_FLV1;
    case FLV_CODECID_SCREEN:
        return vpar->codec_id == AV_CODEC_ID_FLASHSV;
    case FLV_CODECID_SCREEN2:
        return vpar->codec_id == AV_CODEC_ID_FLASHSV2;
    case FLV_CODECID_VP6:
        return vpar->codec_id == AV_CODEC_ID_VP6F;
    case FLV_CODECID_VP6A:
        return vpar->codec_id == AV_CODEC_ID_VP6A;
    case FLV_CODECID_H264:
        return vpar->codec_id == AV_CODEC_ID_H264;
    default:
        return vpar->codec_tag == flv_codecid;
    }
}

static int flv_set_video_codec(AVFormatContext *s, AVStream *vstream,
                               int flv_codecid, int read)
{
    int ret = 0;
    AVCodecParameters *par = vstream->codecpar;
    enum AVCodecID old_codec_id = vstream->codecpar->codec_id;
    switch (flv_codecid) {
    case FLV_CODECID_H263:
        par->codec_id = AV_CODEC_ID_FLV1;
        break;
    case FLV_CODECID_REALH263:
        par->codec_id = AV_CODEC_ID_H263;
        break; // Really mean it this time
    case FLV_CODECID_SCREEN:
        par->codec_id = AV_CODEC_ID_FLASHSV;
        break;
    case FLV_CODECID_SCREEN2:
        par->codec_id = AV_CODEC_ID_FLASHSV2;
        break;
    case FLV_CODECID_VP6:
        par->codec_id = AV_CODEC_ID_VP6F;
    case FLV_CODECID_VP6A:
        if (flv_codecid == FLV_CODECID_VP6A)
            par->codec_id = AV_CODEC_ID_VP6A;
        if (read) {
            if (par->extradata_size != 1) {
                ff_alloc_extradata(par, 1);
            }
            if (par->extradata)
                par->extradata[0] = avio_r8(s->pb);
            else
                avio_skip(s->pb, 1);
        }
        ret = 1;     // 1 byte body size adjustment for flv_read_packet()
        break;
    case FLV_CODECID_H264:
        par->codec_id = AV_CODEC_ID_H264;
        vstream->need_parsing = AVSTREAM_PARSE_HEADERS;
        ret = 3;     // not 4, reading packet type will consume one byte
        break;
    case FLV_CODECID_MPEG4:
        par->codec_id = AV_CODEC_ID_MPEG4;
        ret = 3;
        break;
    default:
        avpriv_request_sample(s, "Video codec (%x)", flv_codecid);
        par->codec_tag = flv_codecid;
    }

    if (!vstream->internal->need_context_update && par->codec_id != old_codec_id) {
        avpriv_request_sample(s, "Changing the codec id midstream");
        return AVERROR_PATCHWELCOME;
    }

    return ret;
}

static int amf_get_string(AVIOContext *ioc, char *buffer, int buffsize)
{
    int ret;
    int length = avio_rb16(ioc);
    if (length >= buffsize) {
        avio_skip(ioc, length);
        return -1;
    }

    ret = avio_read(ioc, buffer, length);
    if (ret < 0)
        return ret;
    if (ret < length)
        return AVERROR_INVALIDDATA;

    buffer[length] = '\0';

    return length;
}

static int parse_keyframes_index(AVFormatContext *s, AVIOContext *ioc, int64_t max_pos)
{
    FLVContext *flv       = s->priv_data;
    unsigned int timeslen = 0, fileposlen = 0, i;
    char str_val[256];
    int64_t *times         = NULL;
    int64_t *filepositions = NULL;
    int ret                = AVERROR(ENOSYS);
    int64_t initial_pos    = avio_tell(ioc);

    if (flv->keyframe_count > 0) {
        av_log(s, AV_LOG_DEBUG, "keyframes have been parsed\n");
        return 0;
    }
    av_assert0(!flv->keyframe_times);
    av_assert0(!flv->keyframe_filepositions);

    if (s->flags & AVFMT_FLAG_IGNIDX)
        return 0;

    while (avio_tell(ioc) < max_pos - 2 &&
           amf_get_string(ioc, str_val, sizeof(str_val)) > 0) {
        int64_t **current_array;
        unsigned int arraylen;

        // Expect array object in context
        if (avio_r8(ioc) != AMF_DATA_TYPE_ARRAY)
            break;

        arraylen = avio_rb32(ioc);
        if (arraylen>>28)
            break;

        if       (!strcmp(KEYFRAMES_TIMESTAMP_TAG , str_val) && !times) {
            current_array = &times;
            timeslen      = arraylen;
        } else if (!strcmp(KEYFRAMES_BYTEOFFSET_TAG, str_val) &&
                   !filepositions) {
            current_array = &filepositions;
            fileposlen    = arraylen;
        } else
            // unexpected metatag inside keyframes, will not use such
            // metadata for indexing
            break;

        if (!(*current_array = av_mallocz(sizeof(**current_array) * arraylen))) {
            ret = AVERROR(ENOMEM);
            goto finish;
        }

        for (i = 0; i < arraylen && avio_tell(ioc) < max_pos - 1; i++) {
            double d;
            if (avio_r8(ioc) != AMF_DATA_TYPE_NUMBER)
                goto invalid;
            d = av_int2double(avio_rb64(ioc));
            if (isnan(d) || d < INT64_MIN || d > INT64_MAX)
                goto invalid;
            current_array[0][i] = d;
        }
        if (times && filepositions) {
            // All done, exiting at a position allowing amf_parse_object
            // to finish parsing the object
            ret = 0;
            break;
        }
    }

    if (timeslen == fileposlen && fileposlen>1 && max_pos <= filepositions[0]) {
        for (i = 0; i < FFMIN(2,fileposlen); i++) {
            flv->validate_index[i].pos = filepositions[i];
            flv->validate_index[i].dts = times[i] * 1000;
            flv->validate_count        = i + 1;
        }
        flv->keyframe_times = times;
        flv->keyframe_filepositions = filepositions;
        flv->keyframe_count = timeslen;
        times = NULL;
        filepositions = NULL;
    } else {
invalid:
        av_log(s, AV_LOG_WARNING, "Invalid keyframes object, skipping.\n");
    }

finish:
    av_freep(&times);
    av_freep(&filepositions);
    avio_seek(ioc, initial_pos, SEEK_SET);
    return ret;
}

static int amf_parse_object(AVFormatContext *s, AVStream *astream,
                            AVStream *vstream, const char *key,
                            int64_t max_pos, int depth)
{
    AVCodecParameters *apar, *vpar;
    FLVContext *flv = s->priv_data;
    AVIOContext *ioc;
    AMFDataType amf_type;
    char str_val[1024];
    double num_val;
    amf_date date;

    if (depth > MAX_DEPTH)
        return AVERROR_PATCHWELCOME;

    num_val  = 0;
    ioc      = s->pb;
    if (avio_feof(ioc))
        return AVERROR_EOF;
    amf_type = avio_r8(ioc);

    switch (amf_type) {
    case AMF_DATA_TYPE_NUMBER:
        num_val = av_int2double(avio_rb64(ioc));
        break;
    case AMF_DATA_TYPE_BOOL:
        num_val = avio_r8(ioc);
        break;
    case AMF_DATA_TYPE_STRING:
        if (amf_get_string(ioc, str_val, sizeof(str_val)) < 0) {
            av_log(s, AV_LOG_ERROR, "AMF_DATA_TYPE_STRING parsing failed\n");
            return -1;
        }
        break;
    case AMF_DATA_TYPE_OBJECT:
        if (key &&
            (ioc->seekable & AVIO_SEEKABLE_NORMAL) &&
            !strcmp(KEYFRAMES_TAG, key) && depth == 1)
            if (parse_keyframes_index(s, ioc, max_pos) < 0)
                av_log(s, AV_LOG_ERROR, "Keyframe index parsing failed\n");
            else
                add_keyframes_index(s);
        while (avio_tell(ioc) < max_pos - 2 &&
               amf_get_string(ioc, str_val, sizeof(str_val)) > 0)
            if (amf_parse_object(s, astream, vstream, str_val, max_pos,
                                 depth + 1) < 0)
                return -1;     // if we couldn't skip, bomb out.
        if (avio_r8(ioc) != AMF_END_OF_OBJECT) {
            av_log(s, AV_LOG_ERROR, "Missing AMF_END_OF_OBJECT in AMF_DATA_TYPE_OBJECT\n");
            return -1;
        }
        break;
    case AMF_DATA_TYPE_NULL:
    case AMF_DATA_TYPE_UNDEFINED:
    case AMF_DATA_TYPE_UNSUPPORTED:
        break;     // these take up no additional space
    case AMF_DATA_TYPE_MIXEDARRAY:
    {
        unsigned v;
        avio_skip(ioc, 4);     // skip 32-bit max array index
        while (avio_tell(ioc) < max_pos - 2 &&
               amf_get_string(ioc, str_val, sizeof(str_val)) > 0)
            // this is the only case in which we would want a nested
            // parse to not skip over the object
            if (amf_parse_object(s, astream, vstream, str_val, max_pos,
                                 depth + 1) < 0)
                return -1;
        v = avio_r8(ioc);
        if (v != AMF_END_OF_OBJECT) {
            av_log(s, AV_LOG_ERROR, "Missing AMF_END_OF_OBJECT in AMF_DATA_TYPE_MIXEDARRAY, found %d\n", v);
            return -1;
        }
        break;
    }
    case AMF_DATA_TYPE_ARRAY:
    {
        unsigned int arraylen, i;

        arraylen = avio_rb32(ioc);
        for (i = 0; i < arraylen && avio_tell(ioc) < max_pos - 1; i++)
            if (amf_parse_object(s, NULL, NULL, NULL, max_pos,
                                 depth + 1) < 0)
                return -1;      // if we couldn't skip, bomb out.
    }
    break;
    case AMF_DATA_TYPE_DATE:
        // timestamp (double) and UTC offset (int16)
        date.milliseconds = av_int2double(avio_rb64(ioc));
        date.timezone = avio_rb16(ioc);
        break;
    default:                    // unsupported type, we couldn't skip
        av_log(s, AV_LOG_ERROR, "unsupported amf type %d\n", amf_type);
        return -1;
    }

    if (key) {
        apar = astream ? astream->codecpar : NULL;
        vpar = vstream ? vstream->codecpar : NULL;

        // stream info doesn't live any deeper than the first object
        if (depth == 1) {
            if (amf_type == AMF_DATA_TYPE_NUMBER ||
                amf_type == AMF_DATA_TYPE_BOOL) {
                if (!strcmp(key, "duration"))
                    s->duration = num_val * AV_TIME_BASE;
                else if (!strcmp(key, "videodatarate") &&
                         0 <= (int)(num_val * 1024.0))
                    flv->video_bit_rate = num_val * 1024.0;
                else if (!strcmp(key, "audiodatarate") &&
                         0 <= (int)(num_val * 1024.0))
                    flv->audio_bit_rate = num_val * 1024.0;
                else if (!strcmp(key, "datastream")) {
                    AVStream *st = create_stream(s, AVMEDIA_TYPE_SUBTITLE);
                    if (!st)
                        return AVERROR(ENOMEM);
                    st->codecpar->codec_id = AV_CODEC_ID_TEXT;
                } else if (!strcmp(key, "framerate")) {
                    flv->framerate = av_d2q(num_val, 1000);
                    if (vstream)
                        vstream->avg_frame_rate = flv->framerate;
                } else if (flv->trust_metadata) {
                    if (!strcmp(key, "videocodecid") && vpar) {
                        int ret = flv_set_video_codec(s, vstream, num_val, 0);
                        if (ret < 0)
                            return ret;
                    } else if (!strcmp(key, "audiocodecid") && apar) {
                        int id = ((int)num_val) << FLV_AUDIO_CODECID_OFFSET;
                        flv_set_audio_codec(s, astream, apar, id);
                    } else if (!strcmp(key, "audiosamplerate") && apar) {
                        apar->sample_rate = num_val;
                    } else if (!strcmp(key, "audiosamplesize") && apar) {
                        apar->bits_per_coded_sample = num_val;
                    } else if (!strcmp(key, "stereo") && apar) {
                        apar->channels       = num_val + 1;
                        apar->channel_layout = apar->channels == 2 ?
                                               AV_CH_LAYOUT_STEREO :
                                               AV_CH_LAYOUT_MONO;
                    } else if (!strcmp(key, "width") && vpar) {
                        vpar->width = num_val;
                    } else if (!strcmp(key, "height") && vpar) {
                        vpar->height = num_val;
                    }
                }
            }
            if (amf_type == AMF_DATA_TYPE_STRING) {
                if (!strcmp(key, "encoder")) {
                    int version = -1;
                    if (1 == sscanf(str_val, "Open Broadcaster Software v0.%d", &version)) {
                        if (version > 0 && version <= 655)
                            flv->broken_sizes = 1;
                    }
                } else if (!strcmp(key, "metadatacreator")) {
                    if (   !strcmp (str_val, "MEGA")
                        || !strncmp(str_val, "FlixEngine", 10))
                        flv->broken_sizes = 1;
                }
            }
        }

        if (amf_type == AMF_DATA_TYPE_OBJECT && s->nb_streams == 1 &&
           ((!apar && !strcmp(key, "audiocodecid")) ||
            (!vpar && !strcmp(key, "videocodecid"))))
                s->ctx_flags &= ~AVFMTCTX_NOHEADER; //If there is either audio/video missing, codecid will be an empty object

        if ((!strcmp(key, "duration")        ||
            !strcmp(key, "filesize")        ||
            !strcmp(key, "width")           ||
            !strcmp(key, "height")          ||
            !strcmp(key, "videodatarate")   ||
            !strcmp(key, "framerate")       ||
            !strcmp(key, "videocodecid")    ||
            !strcmp(key, "audiodatarate")   ||
            !strcmp(key, "audiosamplerate") ||
            !strcmp(key, "audiosamplesize") ||
            !strcmp(key, "stereo")          ||
            !strcmp(key, "audiocodecid")    ||
            !strcmp(key, "datastream")) && !flv->dump_full_metadata)
            return 0;

        s->event_flags |= AVFMT_EVENT_FLAG_METADATA_UPDATED;
        if (amf_type == AMF_DATA_TYPE_BOOL) {
            av_strlcpy(str_val, num_val > 0 ? "true" : "false",
                       sizeof(str_val));
            av_dict_set(&s->metadata, key, str_val, 0);
        } else if (amf_type == AMF_DATA_TYPE_NUMBER) {
            snprintf(str_val, sizeof(str_val), "%.f", num_val);
            av_dict_set(&s->metadata, key, str_val, 0);
        } else if (amf_type == AMF_DATA_TYPE_STRING) {
            av_dict_set(&s->metadata, key, str_val, 0);
        } else if (amf_type == AMF_DATA_TYPE_DATE) {
            time_t time;
            struct tm t;
            char datestr[128];
            time =  date.milliseconds / 1000; // to seconds
            localtime_r(&time, &t);
            strftime(datestr, sizeof(datestr), "%a, %d %b %Y %H:%M:%S %z", &t);

            av_dict_set(&s->metadata, key, datestr, 0);
        }
    }

    return 0;
}

#define TYPE_ONTEXTDATA 1
#define TYPE_ONCAPTION 2
#define TYPE_ONCAPTIONINFO 3
#define TYPE_UNKNOWN 9

static int flv_read_metabody(AVFormatContext *s, int64_t next_pos)
{
    FLVContext *flv = s->priv_data;
    AMFDataType type;
    AVStream *stream, *astream, *vstream;
    AVStream av_unused *dstream;
    AVIOContext *ioc;
    int i;
    char buffer[32];

    astream = NULL;
    vstream = NULL;
    dstream = NULL;
    ioc     = s->pb;

    // first object needs to be "onMetaData" string
    type = avio_r8(ioc);
    if (type != AMF_DATA_TYPE_STRING ||
        amf_get_string(ioc, buffer, sizeof(buffer)) < 0)
        return TYPE_UNKNOWN;

    if (!strcmp(buffer, "onTextData"))
        return TYPE_ONTEXTDATA;

    if (!strcmp(buffer, "onCaption"))
        return TYPE_ONCAPTION;

    if (!strcmp(buffer, "onCaptionInfo"))
        return TYPE_ONCAPTIONINFO;

    if (strcmp(buffer, "onMetaData") && strcmp(buffer, "onCuePoint") && strcmp(buffer, "|RtmpSampleAccess")) {
        av_log(s, AV_LOG_DEBUG, "Unknown type %s\n", buffer);
        return TYPE_UNKNOWN;
    }

    // find the streams now so that amf_parse_object doesn't need to do
    // the lookup every time it is called.
    for (i = 0; i < s->nb_streams; i++) {
        stream = s->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            vstream = stream;
            flv->last_keyframe_stream_index = i;
        } else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            astream = stream;
            if (flv->last_keyframe_stream_index == -1)
                flv->last_keyframe_stream_index = i;
        } else if (stream->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE)
            dstream = stream;
    }

    // parse the second object (we want a mixed array)
    if (amf_parse_object(s, astream, vstream, buffer, next_pos, 0) < 0)
        return -1;

    return 0;
}

static int flv_read_header(AVFormatContext *s)
{
    int flags;
    FLVContext *flv = s->priv_data;
    int offset;
    int pre_tag_size = 0;

    /* Actual FLV data at 0xe40000 in KUX file */
    if(!strcmp(s->iformat->name, "kux"))
        avio_skip(s->pb, 0xe40000);

    avio_skip(s->pb, 4);
    flags = avio_r8(s->pb);

    flv->missing_streams = flags & (FLV_HEADER_FLAG_HASVIDEO | FLV_HEADER_FLAG_HASAUDIO);

    s->ctx_flags |= AVFMTCTX_NOHEADER;

    offset = avio_rb32(s->pb);
    avio_seek(s->pb, offset, SEEK_SET);

    /* Annex E. The FLV File Format
     * E.3 TheFLVFileBody
     *     Field               Type    Comment
     *     PreviousTagSize0    UI32    Always 0
     * */
    pre_tag_size = avio_rb32(s->pb);
    if (pre_tag_size) {
        av_log(s, AV_LOG_WARNING, "Read FLV header error, input file is not a standard flv format, first PreviousTagSize0 always is 0\n");
    }

    s->start_time = 0;
    flv->sum_flv_tag_size = 0;
    flv->last_keyframe_stream_index = -1;

    return 0;
}

static int flv_read_close(AVFormatContext *s)
{
    int i;
    FLVContext *flv = s->priv_data;
    for (i=0; i<FLV_STREAM_TYPE_NB; i++)
        av_freep(&flv->new_extradata[i]);
    av_freep(&flv->keyframe_times);
    av_freep(&flv->keyframe_filepositions);
    return 0;
}

static int flv_get_extradata(AVFormatContext *s, AVStream *st, int size)
{
    int ret;
    if (!size)
        return 0;

    if ((ret = ff_get_extradata(s, st->codecpar, s->pb, size)) < 0)
        return ret;
    st->internal->need_context_update = 1;
    return 0;
}

static int flv_queue_extradata(FLVContext *flv, AVIOContext *pb, int stream,
                               int size)
{
    if (!size)
        return 0;

    av_free(flv->new_extradata[stream]);
    flv->new_extradata[stream] = av_mallocz(size +
                                            AV_INPUT_BUFFER_PADDING_SIZE);
    if (!flv->new_extradata[stream])
        return AVERROR(ENOMEM);
    flv->new_extradata_size[stream] = size;
    avio_read(pb, flv->new_extradata[stream], size);
    return 0;
}

static void clear_index_entries(AVFormatContext *s, int64_t pos)
{
    int i, j, out;
    av_log(s, AV_LOG_WARNING,
           "Found invalid index entries, clearing the index.\n");
    for (i = 0; i < s->nb_streams; i++) {
        AVStream *st = s->streams[i];
        /* Remove all index entries that point to >= pos */
        out = 0;
        for (j = 0; j < st->nb_index_entries; j++)
            if (st->index_entries[j].pos < pos)
                st->index_entries[out++] = st->index_entries[j];
        st->nb_index_entries = out;
    }
}

static int amf_skip_tag(AVIOContext *pb, AMFDataType type, int depth)
{
    int nb = -1, ret, parse_name = 1;

    if (depth > MAX_DEPTH)
        return AVERROR_PATCHWELCOME;

    if (avio_feof(pb))
        return AVERROR_EOF;

    switch (type) {
    case AMF_DATA_TYPE_NUMBER:
        avio_skip(pb, 8);
        break;
    case AMF_DATA_TYPE_BOOL:
        avio_skip(pb, 1);
        break;
    case AMF_DATA_TYPE_STRING:
        avio_skip(pb, avio_rb16(pb));
        break;
    case AMF_DATA_TYPE_ARRAY:
        parse_name = 0;
    case AMF_DATA_TYPE_MIXEDARRAY:
        nb = avio_rb32(pb);
        if (nb < 0)
            return AVERROR_INVALIDDATA;
    case AMF_DATA_TYPE_OBJECT:
        while(!pb->eof_reached && (nb-- > 0 || type != AMF_DATA_TYPE_ARRAY)) {
            if (parse_name) {
                int size = avio_rb16(pb);
                if (!size) {
                    avio_skip(pb, 1);
                    break;
                }
                avio_skip(pb, size);
            }
            if ((ret = amf_skip_tag(pb, avio_r8(pb), depth + 1)) < 0)
                return ret;
        }
        break;
    case AMF_DATA_TYPE_NULL:
    case AMF_DATA_TYPE_OBJECT_END:
        break;
    default:
        return AVERROR_INVALIDDATA;
    }
    return 0;
}

static int flv_data_packet(AVFormatContext *s, AVPacket *pkt,
                           int64_t dts, int64_t next)
{
    AVIOContext *pb = s->pb;
    AVStream *st    = NULL;
    char buf[20];
    int ret = AVERROR_INVALIDDATA;
    int i, length = -1;
    int array = 0;

    switch (avio_r8(pb)) {
    case AMF_DATA_TYPE_ARRAY:
        array = 1;
    case AMF_DATA_TYPE_MIXEDARRAY:
        avio_seek(pb, 4, SEEK_CUR);
    case AMF_DATA_TYPE_OBJECT:
        break;
    default:
        goto skip;
    }

    while (array || (ret = amf_get_string(pb, buf, sizeof(buf))) > 0) {
        AMFDataType type = avio_r8(pb);
        if (type == AMF_DATA_TYPE_STRING && (array || !strcmp(buf, "text"))) {
            length = avio_rb16(pb);
            ret    = av_get_packet(pb, pkt, length);
            if (ret < 0)
                goto skip;
            else
                break;
        } else {
            if ((ret = amf_skip_tag(pb, type, 0)) < 0)
                goto skip;
        }
    }

    if (length < 0) {
        ret = AVERROR_INVALIDDATA;
        goto skip;
    }

    for (i = 0; i < s->nb_streams; i++) {
        st = s->streams[i];
        if (st->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE)
            break;
    }

    if (i == s->nb_streams) {
        st = create_stream(s, AVMEDIA_TYPE_SUBTITLE);
        if (!st)
            return AVERROR(ENOMEM);
        st->codecpar->codec_id = AV_CODEC_ID_TEXT;
    }

    pkt->dts  = dts;
    pkt->pts  = dts;
    pkt->size = ret;

    pkt->stream_index = st->index;
    pkt->flags       |= AV_PKT_FLAG_KEY;

skip:
    avio_seek(s->pb, next + 4, SEEK_SET);

    return ret;
}

static int resync(AVFormatContext *s)
{
    FLVContext *flv = s->priv_data;
    int64_t i;
    int64_t pos = avio_tell(s->pb);

    for (i=0; !avio_feof(s->pb); i++) {
        int j  = i & (RESYNC_BUFFER_SIZE-1);
        int j1 = j + RESYNC_BUFFER_SIZE;
        flv->resync_buffer[j ] =
        flv->resync_buffer[j1] = avio_r8(s->pb);

        if (i >= 8 && pos) {
            uint8_t *d = flv->resync_buffer + j1 - 8;
            if (d[0] == 'F' &&
                d[1] == 'L' &&
                d[2] == 'V' &&
                d[3] < 5 && d[5] == 0) {
                av_log(s, AV_LOG_WARNING, "Concatenated FLV detected, might fail to demux, decode and seek %"PRId64"\n", flv->last_ts);
                flv->time_offset = flv->last_ts + 1;
                flv->time_pos    = avio_tell(s->pb);
            }
        }

        if (i > 22) {
            unsigned lsize2 = AV_RB32(flv->resync_buffer + j1 - 4);
            if (lsize2 >= 11 && lsize2 + 8LL < FFMIN(i, RESYNC_BUFFER_SIZE)) {
                unsigned  size2 = AV_RB24(flv->resync_buffer + j1 - lsize2 + 1 - 4);
                unsigned lsize1 = AV_RB32(flv->resync_buffer + j1 - lsize2 - 8);
                if (lsize1 >= 11 && lsize1 + 8LL + lsize2 < FFMIN(i, RESYNC_BUFFER_SIZE)) {
                    unsigned  size1 = AV_RB24(flv->resync_buffer + j1 - lsize1 + 1 - lsize2 - 8);
                    if (size1 == lsize1 - 11 && size2  == lsize2 - 11) {
                        avio_seek(s->pb, pos + i - lsize1 - lsize2 - 8, SEEK_SET);
                        return 1;
                    }
                }
            }
        }
    }
    return AVERROR_EOF;
}

static int flv_read_packet(AVFormatContext *s, AVPacket *pkt)
{
    FLVContext *flv = s->priv_data;
    int ret, i, size, flags;
    enum FlvTagType type;
    int stream_type=-1;
    int64_t next, pos, meta_pos;
    int64_t dts, pts = AV_NOPTS_VALUE;
    int av_uninit(channels);
    int av_uninit(sample_rate);
    AVStream *st    = NULL;
    int last = -1;
    int orig_size;

retry:
    /* pkt size is repeated at end. skip it */
    pos  = avio_tell(s->pb);
    type = (avio_r8(s->pb) & 0x1F);
    orig_size =
    size = avio_rb24(s->pb);
    flv->sum_flv_tag_size += size + 11;
    dts  = avio_rb24(s->pb);
    dts |= (unsigned)avio_r8(s->pb) << 24;
    av_log(s, AV_LOG_TRACE, "type:%d, size:%d, last:%d, dts:%"PRId64" pos:%"PRId64"\n", type, size, last, dts, avio_tell(s->pb));
    if (avio_feof(s->pb))
        return AVERROR_EOF;
    avio_skip(s->pb, 3); /* stream id, always 0 */
    flags = 0;

    if (flv->validate_next < flv->validate_count) {
        int64_t validate_pos = flv->validate_index[flv->validate_next].pos;
        if (pos == validate_pos) {
            if (FFABS(dts - flv->validate_index[flv->validate_next].dts) <=
                VALIDATE_INDEX_TS_THRESH) {
                flv->validate_next++;
            } else {
                clear_index_entries(s, validate_pos);
                flv->validate_count = 0;
            }
        } else if (pos > validate_pos) {
            clear_index_entries(s, validate_pos);
            flv->validate_count = 0;
        }
    }

    if (size == 0) {
        ret = FFERROR_REDO;
        goto leave;
    }

    next = size + avio_tell(s->pb);

    if (type == FLV_TAG_TYPE_AUDIO) {
        stream_type = FLV_STREAM_TYPE_AUDIO;
        flags    = avio_r8(s->pb);
        size--;
    } else if (type == FLV_TAG_TYPE_VIDEO) {
        stream_type = FLV_STREAM_TYPE_VIDEO;
        flags    = avio_r8(s->pb);
        size--;
        if ((flags & FLV_VIDEO_FRAMETYPE_MASK) == FLV_FRAME_VIDEO_INFO_CMD)
            goto skip;
    } else if (type == FLV_TAG_TYPE_META) {
        stream_type=FLV_STREAM_TYPE_SUBTITLE;
        if (size > 13 + 1 + 4) { // Header-type metadata stuff
            int type;
            meta_pos = avio_tell(s->pb);
            type = flv_read_metabody(s, next);
            if (type == 0 && dts == 0 || type < 0) {
                if (type < 0 && flv->validate_count &&
                    flv->validate_index[0].pos     > next &&
                    flv->validate_index[0].pos - 4 < next) {
                    av_log(s, AV_LOG_WARNING, "Adjusting next position due to index mismatch\n");
                    next = flv->validate_index[0].pos - 4;
                }
                goto skip;
            } else if (type == TYPE_ONTEXTDATA) {
                avpriv_request_sample(s, "OnTextData packet");
                return flv_data_packet(s, pkt, dts, next);
            } else if (type == TYPE_ONCAPTION) {
                return flv_data_packet(s, pkt, dts, next);
            } else if (type == TYPE_UNKNOWN) {
                stream_type = FLV_STREAM_TYPE_DATA;
            }
            avio_seek(s->pb, meta_pos, SEEK_SET);
        }
    } else {
        av_log(s, AV_LOG_DEBUG,
               "Skipping flv packet: type %d, size %d, flags %d.\n",
               type, size, flags);
skip:
        if (avio_seek(s->pb, next, SEEK_SET) != next) {
            // This can happen if flv_read_metabody above read past
            // next, on a non-seekable input, and the preceding data has
            // been flushed out from the IO buffer.
            av_log(s, AV_LOG_ERROR, "Unable to seek to the next packet\n");
            return AVERROR_INVALIDDATA;
        }
        ret = FFERROR_REDO;
        goto leave;
    }

    /* skip empty data packets */
    if (!size) {
        ret = FFERROR_REDO;
        goto leave;
    }

    /* now find stream */
    for (i = 0; i < s->nb_streams; i++) {
        st = s->streams[i];
        if (stream_type == FLV_STREAM_TYPE_AUDIO) {
            if (st->codecpar->codec_type == AVMEDIA_TYPE_AUDIO &&
                (s->audio_codec_id || flv_same_audio_codec(st->codecpar, flags)))
                break;
        } else if (stream_type == FLV_STREAM_TYPE_VIDEO) {
            if (st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO &&
                (s->video_codec_id || flv_same_video_codec(st->codecpar, flags)))
                break;
        } else if (stream_type == FLV_STREAM_TYPE_SUBTITLE) {
            if (st->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE)
                break;
        } else if (stream_type == FLV_STREAM_TYPE_DATA) {
            if (st->codecpar->codec_type == AVMEDIA_TYPE_DATA)
                break;
        }
    }
    if (i == s->nb_streams) {
        static const enum AVMediaType stream_types[] = {AVMEDIA_TYPE_VIDEO, AVMEDIA_TYPE_AUDIO, AVMEDIA_TYPE_SUBTITLE, AVMEDIA_TYPE_DATA};
        st = create_stream(s, stream_types[stream_type]);
        if (!st)
            return AVERROR(ENOMEM);
    }
    av_log(s, AV_LOG_TRACE, "%d %X %d \n", stream_type, flags, st->discard);

    if (flv->time_pos <= pos) {
        dts += flv->time_offset;
    }

    if ((s->pb->seekable & AVIO_SEEKABLE_NORMAL) &&
        ((flags & FLV_VIDEO_FRAMETYPE_MASK) == FLV_FRAME_KEY ||
         stream_type == FLV_STREAM_TYPE_AUDIO))
        av_add_index_entry(st, pos, dts, size, 0, AVINDEX_KEYFRAME);

    if ((st->discard >= AVDISCARD_NONKEY && !((flags & FLV_VIDEO_FRAMETYPE_MASK) == FLV_FRAME_KEY || stream_type == FLV_STREAM_TYPE_AUDIO)) ||
        (st->discard >= AVDISCARD_BIDIR && ((flags & FLV_VIDEO_FRAMETYPE_MASK) == FLV_FRAME_DISP_INTER && stream_type == FLV_STREAM_TYPE_VIDEO)) ||
         st->discard >= AVDISCARD_ALL) {
        avio_seek(s->pb, next, SEEK_SET);
        ret = FFERROR_REDO;
        goto leave;
    }

    // if not streamed and no duration from metadata then seek to end to find
    // the duration from the timestamps
    if ((s->pb->seekable & AVIO_SEEKABLE_NORMAL) &&
        (!s->duration || s->duration == AV_NOPTS_VALUE) &&
        !flv->searched_for_end) {
        int size;
        const int64_t pos   = avio_tell(s->pb);
        // Read the last 4 bytes of the file, this should be the size of the
        // previous FLV tag. Use the timestamp of its payload as duration.
        int64_t fsize       = avio_size(s->pb);
retry_duration:
        avio_seek(s->pb, fsize - 4, SEEK_SET);
        size = avio_rb32(s->pb);
        if (size > 0 && size < fsize) {
            // Seek to the start of the last FLV tag at position (fsize - 4 - size)
            // but skip the byte indicating the type.
            avio_seek(s->pb, fsize - 3 - size, SEEK_SET);
            if (size == avio_rb24(s->pb) + 11) {
                uint32_t ts = avio_rb24(s->pb);
                ts         |= (unsigned)avio_r8(s->pb) << 24;
                if (ts)
                    s->duration = ts * (int64_t)AV_TIME_BASE / 1000;
                else if (fsize >= 8 && fsize - 8 >= size) {
                    fsize -= size+4;
                    goto retry_duration;
                }
            }
        }

        avio_seek(s->pb, pos, SEEK_SET);
        flv->searched_for_end = 1;
    }

    if (stream_type == FLV_STREAM_TYPE_AUDIO) {
        int bits_per_coded_sample;
        channels = (flags & FLV_AUDIO_CHANNEL_MASK) == FLV_STEREO ? 2 : 1;
        sample_rate = 44100 << ((flags & FLV_AUDIO_SAMPLERATE_MASK) >>
                                FLV_AUDIO_SAMPLERATE_OFFSET) >> 3;
        bits_per_coded_sample = (flags & FLV_AUDIO_SAMPLESIZE_MASK) ? 16 : 8;
        if (!st->codecpar->channels || !st->codecpar->sample_rate ||
            !st->codecpar->bits_per_coded_sample) {
            st->codecpar->channels              = channels;
            st->codecpar->channel_layout        = channels == 1
                                               ? AV_CH_LAYOUT_MONO
                                               : AV_CH_LAYOUT_STEREO;
            st->codecpar->sample_rate           = sample_rate;
            st->codecpar->bits_per_coded_sample = bits_per_coded_sample;
        }
        if (!st->codecpar->codec_id) {
            flv_set_audio_codec(s, st, st->codecpar,
                                flags & FLV_AUDIO_CODECID_MASK);
            flv->last_sample_rate =
            sample_rate           = st->codecpar->sample_rate;
            flv->last_channels    =
            channels              = st->codecpar->channels;
        } else {
            AVCodecParameters *par = avcodec_parameters_alloc();
            if (!par) {
                ret = AVERROR(ENOMEM);
                goto leave;
            }
            par->sample_rate = sample_rate;
            par->bits_per_coded_sample = bits_per_coded_sample;
            flv_set_audio_codec(s, st, par, flags & FLV_AUDIO_CODECID_MASK);
            sample_rate = par->sample_rate;
            avcodec_parameters_free(&par);
        }
    } else if (stream_type == FLV_STREAM_TYPE_VIDEO) {
        int ret = flv_set_video_codec(s, st, flags & FLV_VIDEO_CODECID_MASK, 1);
        if (ret < 0)
            return ret;
        size -= ret;
    } else if (stream_type == FLV_STREAM_TYPE_SUBTITLE) {
        st->codecpar->codec_id = AV_CODEC_ID_TEXT;
    } else if (stream_type == FLV_STREAM_TYPE_DATA) {
        st->codecpar->codec_id = AV_CODEC_ID_NONE; // Opaque AMF data
    }

    if (st->codecpar->codec_id == AV_CODEC_ID_AAC ||
        st->codecpar->codec_id == AV_CODEC_ID_H264 ||
        st->codecpar->codec_id == AV_CODEC_ID_MPEG4) {
        int type = avio_r8(s->pb);
        size--;

        if (size < 0) {
            ret = AVERROR_INVALIDDATA;
            goto leave;
        }

        if (st->codecpar->codec_id == AV_CODEC_ID_H264 || st->codecpar->codec_id == AV_CODEC_ID_MPEG4) {
            // sign extension
            int32_t cts = (avio_rb24(s->pb) + 0xff800000) ^ 0xff800000;
            pts = av_sat_add64(dts, cts);
            if (cts < 0) { // dts might be wrong
                if (!flv->wrong_dts)
                    av_log(s, AV_LOG_WARNING,
                        "Negative cts, previous timestamps might be wrong.\n");
                flv->wrong_dts = 1;
            } else if (FFABS(dts - pts) > 1000*60*15) {
                av_log(s, AV_LOG_WARNING,
                       "invalid timestamps %"PRId64" %"PRId64"\n", dts, pts);
                dts = pts = AV_NOPTS_VALUE;
            }
        }
        if (type == 0 && (!st->codecpar->extradata || st->codecpar->codec_id == AV_CODEC_ID_AAC ||
            st->codecpar->codec_id == AV_CODEC_ID_H264)) {
            AVDictionaryEntry *t;

            if (st->codecpar->extradata) {
                if ((ret = flv_queue_extradata(flv, s->pb, stream_type, size)) < 0)
                    return ret;
                ret = FFERROR_REDO;
                goto leave;
            }
            if ((ret = flv_get_extradata(s, st, size)) < 0)
                return ret;

            /* Workaround for buggy Omnia A/XE encoder */
            t = av_dict_get(s->metadata, "Encoder", NULL, 0);
            if (st->codecpar->codec_id == AV_CODEC_ID_AAC && t && !strcmp(t->value, "Omnia A/XE"))
                st->codecpar->extradata_size = 2;

            ret = FFERROR_REDO;
            goto leave;
        }
    }

    /* skip empty data packets */
    if (!size) {
        ret = FFERROR_REDO;
        goto leave;
    }

    ret = av_get_packet(s->pb, pkt, size);
    if (ret < 0)
        return ret;
    pkt->dts          = dts;
    pkt->pts          = pts == AV_NOPTS_VALUE ? dts : pts;
    pkt->stream_index = st->index;
    pkt->pos          = pos;
    if (flv->new_extradata[stream_type]) {
        int ret = av_packet_add_side_data(pkt, AV_PKT_DATA_NEW_EXTRADATA,
                                          flv->new_extradata[stream_type],
                                          flv->new_extradata_size[stream_type]);
        if (ret >= 0) {
            flv->new_extradata[stream_type]      = NULL;
            flv->new_extradata_size[stream_type] = 0;
        }
    }
    if (stream_type == FLV_STREAM_TYPE_AUDIO &&
                    (sample_rate != flv->last_sample_rate ||
                     channels    != flv->last_channels)) {
        flv->last_sample_rate = sample_rate;
        flv->last_channels    = channels;
        ff_add_param_change(pkt, channels, 0, sample_rate, 0, 0);
    }

    if (stream_type == FLV_STREAM_TYPE_AUDIO ||
        (flags & FLV_VIDEO_FRAMETYPE_MASK) == FLV_FRAME_KEY ||
        stream_type == FLV_STREAM_TYPE_SUBTITLE ||
        stream_type == FLV_STREAM_TYPE_DATA)
        pkt->flags |= AV_PKT_FLAG_KEY;

leave:
    last = avio_rb32(s->pb);
    if (!flv->trust_datasize) {
        if (last != orig_size + 11 && last != orig_size + 10 &&
            !avio_feof(s->pb) &&
            (last != orig_size || !last) && last != flv->sum_flv_tag_size &&
            !flv->broken_sizes) {
            av_log(s, AV_LOG_ERROR, "Packet mismatch %d %d %d\n", last, orig_size + 11, flv->sum_flv_tag_size);
            avio_seek(s->pb, pos + 1, SEEK_SET);
            ret = resync(s);
            av_packet_unref(pkt);
            if (ret >= 0) {
                goto retry;
            }
        }
    }

    if (ret >= 0)
        flv->last_ts = pkt->dts;

    return ret;
}

/* 包含tag_header和tag_videodata的第一个字节(判断是否为key frame) */
typedef struct video_frame_header {
    unsigned char video_codec:4;
    unsigned char frame_type:3;
    unsigned char isexheader:1;
} video_frame_header;

typedef struct tag_header {
    unsigned char             tag_type;     // 1字节，Tag的类型
    unsigned char             data_size[3]; // 3字节，Tag Data 的大小，Little Endian
    unsigned char             time_stamp[4]; // 4字节，时间戳，Little Endian
    unsigned char             stream_id[3];  // 3字节，StreamID，通常为0
    struct video_frame_header frame_header;
} __attribute__((aligned(1))) tag_header;

static int64_t flv_find_keyframe(AVFormatContext* s, int64_t ts, int flags)
{
    int64_t      keyframe_pos;
    int64_t      previous_len;
    int64_t      datasize;
    unsigned int timestamp;
    unsigned char pre_tag_len[4] = {0}; // previous tag len = data_size + 11
    struct tag_header header = {0};

    av_log(s, AV_LOG_DEBUG, "flv_find_keyframe start pos %lld\n",avio_tell(s->pb));
    /* 读一个pkt查看timestamp */
    if (avio_read(s->pb, &header, 12) != 12) {
        return -1;
    }
    timestamp = header.time_stamp[0] << 16 |
                header.time_stamp[1] << 8 |
                header.time_stamp[2] |
                header.time_stamp[3] << 24;
    av_log(s, AV_LOG_DEBUG, "time stamp:%d\n", timestamp);
    if (timestamp <= ts) // 从这个pkt往后读到第一个key frame
    {
        do {
            if (header.frame_header.frame_type == 1 && timestamp >= ts) {
                return avio_seek(s->pb, -12, SEEK_CUR);
            } else {
                datasize = header.data_size[0] << 16 |
                           header.data_size[1] << 8  |
                           header.data_size[2];
                avio_seek(s->pb, datasize - 1 + 4, SEEK_CUR);
                if (avio_feof(s->pb)) {
                    return -1;
                }
                if (avio_read(s->pb, &header, 12) != 12) {
                    return -1;
                }
                timestamp = header.time_stamp[0] << 16 |
                            header.time_stamp[1] << 8  |
                            header.time_stamp[2]       |
                            header.time_stamp[3] << 24;
            }
        } while (1);
    } else { // 往前读到第一个key frame
        do {
            if (header.frame_header.frame_type == 1 && timestamp <= ts) {
                return avio_seek(s->pb, -12, SEEK_CUR);
            } else {
                avio_seek(s->pb, -16, SEEK_CUR);
                if (avio_read(s->pb, pre_tag_len, 4) != 4) {
                    return -1;
                }
                previous_len = pre_tag_len[0] << 24 |
                               pre_tag_len[1] << 16 |
                               pre_tag_len[2] << 8  |
                               pre_tag_len[3];
                if (previous_len + 4 > avio_tell(s->pb)) {
                    av_log(s, AV_LOG_WARNING, "previous len beyond,file:%s,line:%d\n", __FILE__, __LINE__);
                    return -1;
                }
                /* seek to tag header */
                avio_seek(s->pb, -previous_len - 4, SEEK_CUR);
                if (avio_read(s->pb, &header, 12) != 12)
                {
                    return -1;
                }
                timestamp = header.time_stamp[0] << 16 |
                            header.time_stamp[1] << 8  |
                            header.time_stamp[2]       |
                            header.time_stamp[3] << 24;
            }
        } while (1);
    }
    return -1;
}

static int flv_read_seek(AVFormatContext *s, int stream_index,
                         int64_t ts, int flags)
{
    FLVContext* flv = s->priv_data;
    int64_t cur_pos = avio_tell(s->pb);// 记录当前位置
    int64_t key_pos = 0;
    int64_t seek_pos;
    int64_t flvtime_sec;
    int64_t ts_sec;
    int ret;
    flv->validate_count = 0;

    do {
        if (flv->has_index_entries == 1) {
            break;
        }

        if (!s->streams[stream_index] || !s->streams[stream_index]->time_base.den) {
            break;
        }
        flvtime_sec = s->duration / 1000000;
        ts_sec = ts * s->streams[stream_index]->time_base.num / s->streams[stream_index]->time_base.den;

        if (ts_sec >= flvtime_sec || ts < 0) {
            av_log(s, AV_LOG_WARNING, "seek time error.\n");
            return avio_seek(s->pb, cur_pos, SEEK_SET);
        }

        seek_pos = avio_size(s->pb) * ts_sec / flvtime_sec;
        avio_seek(s->pb,seek_pos,SEEK_SET);
        ret = resync(s);
        if (ret < 0) {
            av_log(s, AV_LOG_WARNING, "seek do sync.\n");
            break;
        }
        key_pos  = flv_find_keyframe(s, ts, flags);
        if (key_pos <= 0) {
            av_log(s, AV_LOG_WARNING, "seek find key frame error.\n");
            break;
        }

        return avio_seek(s->pb, key_pos, SEEK_SET);
    } while (0);

    avio_seek(s->pb, cur_pos, SEEK_SET);
    return avio_seek_time(s->pb, stream_index, ts, flags);
}

#define OFFSET(x) offsetof(FLVContext, x)
#define VD AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_DECODING_PARAM
static const AVOption options[] = {
    { "flv_metadata", "Allocate streams according to the onMetaData array", OFFSET(trust_metadata), AV_OPT_TYPE_BOOL, { .i64 = 0 }, 0, 1, VD },
    { "flv_full_metadata", "Dump full metadata of the onMetadata", OFFSET(dump_full_metadata), AV_OPT_TYPE_BOOL, { .i64 = 0 }, 0, 1, VD },
    { "flv_ignore_prevtag", "Ignore the Size of previous tag", OFFSET(trust_datasize), AV_OPT_TYPE_BOOL, { .i64 = 0 }, 0, 1, VD },
    { "missing_streams", "", OFFSET(missing_streams), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 0xFF, VD | AV_OPT_FLAG_EXPORT | AV_OPT_FLAG_READONLY },
    { NULL }
};

static const AVClass flv_class = {
    .class_name = "flvdec",
    .item_name  = av_default_item_name,
    .option     = options,
    .version    = LIBAVUTIL_VERSION_INT,
};

AVInputFormat ff_flv_demuxer = {
    .name           = "flv",
    .long_name      = NULL_IF_CONFIG_SMALL("FLV (Flash Video)"),
    .priv_data_size = sizeof(FLVContext),
    .read_probe     = flv_probe,
    .read_header    = flv_read_header,
    .read_packet    = flv_read_packet,
    .read_seek      = flv_read_seek,
    .read_close     = flv_read_close,
    .extensions     = "flv",
    .priv_class     = &flv_class,
};

static const AVClass live_flv_class = {
    .class_name = "live_flvdec",
    .item_name  = av_default_item_name,
    .option     = options,
    .version    = LIBAVUTIL_VERSION_INT,
};

AVInputFormat ff_live_flv_demuxer = {
    .name           = "live_flv",
    .long_name      = NULL_IF_CONFIG_SMALL("live RTMP FLV (Flash Video)"),
    .priv_data_size = sizeof(FLVContext),
    .read_probe     = live_flv_probe,
    .read_header    = flv_read_header,
    .read_packet    = flv_read_packet,
    .read_seek      = flv_read_seek,
    .read_close     = flv_read_close,
    .extensions     = "flv",
    .priv_class     = &live_flv_class,
    .flags          = AVFMT_TS_DISCONT
};

static const AVClass kux_class = {
    .class_name = "kuxdec",
    .item_name  = av_default_item_name,
    .option     = options,
    .version    = LIBAVUTIL_VERSION_INT,
};

AVInputFormat ff_kux_demuxer = {
    .name           = "kux",
    .long_name      = NULL_IF_CONFIG_SMALL("KUX (YouKu)"),
    .priv_data_size = sizeof(FLVContext),
    .read_probe     = kux_probe,
    .read_header    = flv_read_header,
    .read_packet    = flv_read_packet,
    .read_seek      = flv_read_seek,
    .read_close     = flv_read_close,
    .extensions     = "kux",
    .priv_class     = &kux_class,
};
