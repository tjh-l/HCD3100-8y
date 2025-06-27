/*
 * Register all the formats and protocols
 * Copyright (c) 2000, 2001, 2002 Fabrice Bellard
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

#include <stddef.h>
#include <stdint.h>
#include <malloc.h>

#include <hctinyplayer/registry.h>

/*************************protocol registry****************************/
extern const URLProtocol ff_file_protocol;

#include "protocol_list.c"

const URLProtocol **ffurl_get_protocols(void)
{
	const URLProtocol **ret;
	int i, ret_idx = 0;

	ret = calloc(sizeof(url_protocols) / sizeof((url_protocols)[0]), sizeof(*ret));
	if (!ret)
		return NULL;

	for (i = 0; url_protocols[i]; i++) {
		const URLProtocol *up = url_protocols[i];

		ret[ret_idx++] = up;
	}

	return ret;
}

/*************************demuxer registry****************************/
extern const AVInputFormat  ff_flv_demuxer;
extern const AVInputFormat  ff_kux_demuxer;
extern const AVInputFormat  ff_live_flv_demuxer;
extern const AVInputFormat  ff_mjpeg_demuxer;

#include "demuxer_list.c"

const AVInputFormat *av_demuxer_iterate(void **opaque)
{
    static const uintptr_t size = sizeof(demuxer_list)/sizeof(demuxer_list[0]) - 1;
    uintptr_t i = (uintptr_t)*opaque;
    const AVInputFormat *f = NULL;

    if (i < size) {
        f = demuxer_list[i];
        if (f)
            *opaque = (void*)(i + 1);
    }

    return f;
}

/*************************parser registry****************************/
extern const AVCodecParser ff_h264_parser;
extern const AVCodecParser ff_mpegaudio_parser;

#include "parser_list.c"

const AVCodecParser *av_parser_iterate(void **opaque)
{
    uintptr_t i = (uintptr_t)*opaque;
    const AVCodecParser *p = parser_list[i];

    if (p)
        *opaque = (void*)(i + 1);

    return p;
}

