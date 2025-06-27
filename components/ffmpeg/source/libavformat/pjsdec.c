/*
 * Copyright (c) 2012 Clément Bœsch
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

/**
 * @file
 * PJS (Phoenix Japanimation Society) subtitles format demuxer
 *
 * @see http://subs.com.ru/page.php?al=pjs
 */

#include "avformat.h"
#include "internal.h"
#include "subtitles.h"
#include "avio_internal.h"

typedef struct {
    FFDemuxSubtitlesQueue q;
} PJSContext;

static int pjs_probe(const AVProbeData *p)
{
    char c;
    int64_t start, end;
    const unsigned char *ptr = p->buf;
    char *buf;
    int size = 0;
    char *dst_buf = NULL;
    character_encoding type = 0;
    int score = 0;

    if(!memcmp(ptr, "\xef\xbb\xbf", 3)) { // Skip UTF-8 BOM header
        ptr += 3;
    } else if (!memcmp(ptr, "\xff\xfe", 2)) {
        ptr += 2;  /* skip UTF-16 LE */
        type = CHAR_ENC_UTF_16_LE;
    } else if (!memcmp(ptr, "\xfe\xff", 2)) {
        ptr += 2;  /* skip UTF-16 BE */
        type = CHAR_ENC_UTF_16_BE;
    }

    if (type == CHAR_ENC_UTF_16_LE || type == CHAR_ENC_UTF_16_BE) {
        size = p->buf_size / 2;
        if (p->buf_size % 2) {
            size += 1;
        }
        dst_buf = malloc(size);
        if (!dst_buf) {
            printf("pjs probe:utf16 trans to 8 buff malloc fail\n");
            return 0;
        }
        ff_trans_char_enc_to_utf8(type, ptr, dst_buf, size);
        buf = dst_buf;
    } else {
        buf = ptr;
    }

    if (sscanf(buf, "%"SCNd64",%"SCNd64",%c", &start, &end, &c) == 3) {
        size_t q1pos = strcspn(buf, "\"");
        size_t q2pos = q1pos + strcspn(buf + q1pos + 1, "\"") + 1;
        if (strcspn(buf, "\r\n") > q2pos)
            score = AVPROBE_SCORE_MAX;
    } else {
        score = 0;
    }

    if (dst_buf) {
        free(dst_buf);
        dst_buf = NULL;
    }
    return score;
}

static int64_t read_ts(char **line, int *duration)
{
    int64_t start, end;

    if (sscanf(*line, "%"SCNd64",%"SCNd64, &start, &end) == 2) {
        *line += strcspn(*line, "\"");
        *line += !!**line;
        if (end < start || end - (uint64_t)start > INT_MAX)
            return AV_NOPTS_VALUE;
        *duration = end - start;
        return start;
    }
    return AV_NOPTS_VALUE;
}

static int pjs_read_header(AVFormatContext *s)
{
    AVIOContext *dst_pb = NULL;
    int ret;
    AVIOContext *pb;
    PJSContext *pjs = s->priv_data;
    AVStream *st = avformat_new_stream(s, NULL);

    if (!st)
        return AVERROR(ENOMEM);
    avpriv_set_pts_info(st, 64, 1, 10);
    st->codecpar->codec_type = AVMEDIA_TYPE_SUBTITLE;
    st->codecpar->codec_id   = AV_CODEC_ID_PJS;
    ret = ff_trans_utf16_to_utf8(s->pb, &dst_pb);
    if (ret && dst_pb) {
        pb = dst_pb;
    } else {
        pb = s->pb;
    }

    while (!avio_feof(pb)) {
        char line[4096];
        char *p = line;
        const int64_t pos = avio_tell(pb);
        int len = ff_get_line(s->pb, line, sizeof(line));
        int64_t pts_start;
        int duration;

        if (!len)
            break;

        line[strcspn(line, "\r\n")] = 0;

        pts_start = read_ts(&p, &duration);
        if (pts_start != AV_NOPTS_VALUE) {
            AVPacket *sub;

            p[strcspn(p, "\"")] = 0;
            sub = ff_subtitles_queue_insert(&pjs->q, p, strlen(p), 0);
            if (!sub) {
                ff_subtitles_queue_clean(&pjs->q);
                return AVERROR(ENOMEM);
            }
            sub->pos = pos;
            sub->pts = pts_start;
            sub->duration = duration;
        }
    }
    if (dst_pb) {
        ffio_free_dyn_buf(&dst_pb);
    }
    ff_subtitles_queue_finalize(s, &pjs->q);
    return 0;
}

static int pjs_read_packet(AVFormatContext *s, AVPacket *pkt)
{
    PJSContext *pjs = s->priv_data;
    return ff_subtitles_queue_read_packet(&pjs->q, pkt);
}

static int pjs_read_seek(AVFormatContext *s, int stream_index,
                         int64_t min_ts, int64_t ts, int64_t max_ts, int flags)
{
    PJSContext *pjs = s->priv_data;
    return ff_subtitles_queue_seek(&pjs->q, s, stream_index,
                                   min_ts, ts, max_ts, flags);
}

static int pjs_read_close(AVFormatContext *s)
{
    PJSContext *pjs = s->priv_data;
    ff_subtitles_queue_clean(&pjs->q);
    return 0;
}

AVInputFormat ff_pjs_demuxer = {
    .name           = "pjs",
    .long_name      = NULL_IF_CONFIG_SMALL("PJS (Phoenix Japanimation Society) subtitles"),
    .priv_data_size = sizeof(PJSContext),
    .read_probe     = pjs_probe,
    .read_header    = pjs_read_header,
    .read_packet    = pjs_read_packet,
    .read_seek2     = pjs_read_seek,
    .read_close     = pjs_read_close,
    .extensions     = "pjs",
};
