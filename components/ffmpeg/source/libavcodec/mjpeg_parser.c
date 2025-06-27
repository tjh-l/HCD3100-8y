/*
 * MJPEG parser
 * Copyright (c) 2000, 2001 Fabrice Bellard
 * Copyright (c) 2003 Alex Beregszaszi
 * Copyright (c) 2003-2004 Michael Niedermayer
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
 * MJPEG parser.
 */

#include "parser.h"
#include "mjpeg.h"
#include "get_bits.h"
#include "tiff.h"
#include "exif.h"
#include "bytestream.h"
#include "libavutil/imgutils.h"

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef ROUNDUP
#define ROUNDUP(a, b)	(((a) + ((b) - 1)) & ~((b) - 1))
#endif

typedef struct MJPEGParserContext{
    ParseContext pc;
    int size;

    int ls;
    int lossless;
    int progressive;
    int cs_itu601;
    int adobe_transform;
    int pix_fmt_id;
    int rgb;
    int palette_index;

    /* Number of components in image (1 for gray, 3 for YUV, etc.) */
    uint8_t nb_components;
    /* precision (in bits) for the samples */
    uint8_t bits;
    /* unique value identifying each component */
    uint8_t component_id[4];
    /* quantization table ID to use for this comp */
    uint8_t quant_index[4];
    /* Array[numComponents] giving the number of blocks (horiz) in this component */
    uint8_t h_count[4];
    int h_max;
    /* Same for the vertical part of this component */
    uint8_t v_count[4];
    /* max val in v_count*/
    int v_max;
    int pix_fmt;
}MJPEGParserContext;

typedef enum MJPG_SCAN_TYPE {
    YUV444_YH1V1,
    YUV422_YH2V1,
    YUV420_YH1V2,
    YUV420_YH2V2,
    YUV411_YH4V1,
} mjpg_scan_type_e;

/**
 * Find the end of the current frame in the bitstream.
 * @return the position of the first byte of the next frame, or -1
 */
static int find_frame_end(MJPEGParserContext *m, const uint8_t *buf, int buf_size){
    ParseContext *pc= &m->pc;
    int vop_found, i;
    uint32_t state;

    vop_found= pc->frame_start_found;
    state= pc->state;

    i=0;
    if(!vop_found){
        for(i=0; i<buf_size;){
            state= (state<<8) | buf[i];
            if(state>=0xFFC00000 && state<=0xFFFEFFFF){
                if(state>=0xFFD8FFC0 && state<=0xFFD8FFFF){
                    i++;
                    vop_found=1;
                    break;
                }else if(state<0xFFD00000 || state>0xFFD9FFFF){
                    m->size= (state&0xFFFF)-1;
                }
            }
            if(m->size>0){
                int size= FFMIN(buf_size-i, m->size);
                i+=size;
                m->size-=size;
                state=0;
                continue;
            }else
                i++;
        }
    }

    if(vop_found){
        /* EOF considered as end of frame */
        if (buf_size == 0)
            return 0;
        for(; i<buf_size;){
            state= (state<<8) | buf[i];
            if(state>=0xFFC00000 && state<=0xFFFEFFFF){
                if(state>=0xFFD8FFC0 && state<=0xFFD8FFFF){
                    pc->frame_start_found=0;
                    pc->state=0;
                    return i-3;
                } else if(state<0xFFD00000 || state>0xFFD9FFFF){
                    m->size= (state&0xFFFF)-1;
                    if (m->size >= 0xF000)
                        m->size = 0;
                }
            }
            if(m->size>0){
                int size= FFMIN(buf_size-i, m->size);
                i+=size;
                m->size-=size;
                state=0;
                continue;
            }else
                i++;
        }
    }
    pc->frame_start_found= vop_found;
    pc->state= state;
    return END_NOT_FOUND;
}

static inline int mjpeg_parse_dri(GetBitContext *gb, AVCodecContext *avctx)
{
    int restart_interval = 0;
    if (get_bits_left(gb) < 32) {
        return AVERROR_INVALIDDATA;
    }

    if (get_bits(gb, 16) != 4)
        return AVERROR_INVALIDDATA;
    restart_interval = get_bits(gb, 16);
    av_log(NULL, AV_LOG_DEBUG, "restart_interval: %d\n", restart_interval);

    return 0;
}

static inline int mjpeg_parse_com(GetBitContext *gb, AVCodecContext *avctx, MJPEGParserContext *m)
{
    int len;

    if (get_bits_left(gb) < 16) {
        return AVERROR_INVALIDDATA;
    }

    len = get_bits(gb, 16);
    len -= 2;
    if (len >= 0 && 8 * len <= get_bits_left(gb)) {
        int i;
        char *cbuf = av_malloc(len - 1);
        if (!cbuf)
            return AVERROR(ENOMEM);

        for (i = 0; i < len - 2; i++) {
            cbuf[i] = get_bits(gb, 8);
            len--;
        }
        if (i > 0 && cbuf[i - 1] == '\n')
            cbuf[i - 1] = 0;
        else
            cbuf[i] = 0;

        if (!strcmp(cbuf, "CS=ITU601"))
            m->cs_itu601 = 1;

        av_log(NULL, AV_LOG_INFO, "comment: '%s'\n", cbuf);
        av_free(cbuf);
        if (len > 0)
            skip_bits(gb, 8 * len);
        return 0;
    }

    return AVERROR_INVALIDDATA;
}

static inline int mjpeg_parse_app(GetBitContext *gb, AVCodecContext *avctx, AVCodecParserContext *s)
{
    int len, id;
    MJPEGParserContext *m = (MJPEGParserContext *)s->priv_data;

    if (get_bits_left(gb) < 16) {
        return AVERROR_INVALIDDATA;
    }

    len = get_bits(gb, 16);
    len -= 2;
    if (8 * len > get_bits_left(gb))
        return AVERROR_INVALIDDATA;

    if (len < 4) {
        skip_bits(gb, len * 8);
        return 0;
    }

    id= get_bits_long(gb, 32);
    len -= 4;

    if (id == AV_RB32("Exif") && len >= 2) {
        const uint8_t *aligned;
        GetByteContext gbytes;
        int ret, le, ifd_offset;

        skip_bits(gb, 16); // skip padding
        len -= 2;

        // init byte wise reading
        aligned = align_get_bits(gb);
        bytestream2_init(&gbytes, aligned, len);

        ret = ff_tdecode_header(&gbytes, &le, &ifd_offset);
        if (ret) {
            av_log(NULL, AV_LOG_WARNING, "mjpeg: invalid TIFF header in EXIF data\n");
        } else {
            while (ifd_offset > 0) {
                bytestream2_seek(&gbytes, ifd_offset, SEEK_SET);
                ifd_offset = ff_exif_decode_ifd(avctx, &gbytes, le, 0, &s->metadata);
            }
        }
    }

    if (id == AV_RB32("Adob")
        && len >= 7
        && show_bits(gb, 8) == 'e'
        && show_bits_long(gb, 32) != AV_RB32("e_CM")) {
        skip_bits(gb,  8); /* 'e' */
        skip_bits(gb, 16); /* version */
        skip_bits(gb, 16); /* flags0 */
        skip_bits(gb, 16); /* flags1 */
        m->adobe_transform = get_bits(gb,  8);
        len -= 7;
    }

    if (len > 0)
        skip_bits(gb, 8 * len);
    return 0;
}

static inline int mjpeg_parse_skip_marker(GetBitContext *gb, AVCodecContext *avctx)
{
    int len;

    if (get_bits_left(gb) < 16) {
        return AVERROR_INVALIDDATA;
    }
    len = get_bits(gb, 16);
    len -= 2;
    if (len >= 0 && 8 * len <= get_bits_left(gb)) {
        if (len > 0)
            skip_bits(gb, 8 * len);
        return 0;
    } else {
        return AVERROR_INVALIDDATA;
    }
}

static void mjpeg_parse_estimate_lowres(AVCodecContext *avctx)
{
    int width = avctx->coded_width;
    int height = avctx->coded_height;
    while ((width > 1920 || height > 1080) && avctx->lowres < 3) {
        width >>= 1;
        height >>= 1;
        ++avctx->lowres;
    }
    if ((width * height <  1280 * 720) && avctx->lowres > 0) {
        --avctx->lowres;
    }
    return;
}

static int mjpeg_parse_calc_memory_required(AVCodecContext *avctx, MJPEGParserContext *s)
{
    int block_size, size;
    int bw = (avctx->coded_width + s->h_max * 8 - 1) / (s->h_max * 8);
    int bh = (avctx->coded_height + s->v_max * 8 - 1) / (s->v_max * 8);
    int bwxbh = bw * bh;
    int size_required = 0;
    int width, height;
    int i, ret;
    int linesize[4];
    int sizes[4] = {0};

    block_size = 64 >> (avctx->lowres << 1);
    for (i = 0; i < s->nb_components; i++) {
        size = bwxbh * s->h_count[i] * s->v_count[i];
        size_required += size * 2 * block_size;
        if (avctx->lowres > 0)
            size_required += size * 8;
        size_required += size;
    }

    width = ROUNDUP(avctx->coded_width >> avctx->lowres, 32);
    height = ROUNDUP(avctx->coded_height >> avctx->lowres, 32);

    ret = av_image_fill_linesizes(linesize, s->pix_fmt, width);
    if (ret >= 0) {
        ret = av_image_fill_plane_sizes(sizes, s->pix_fmt, height, linesize);
    }
    if (ret < 0) {
        size_required += width * height * 4;
    } else {
        size_required += sizes[0] + sizes[1] + sizes[2] + sizes[3];
    }

    av_log(NULL, AV_LOG_VERBOSE, "sizes %d, %d, %d, %d\n", sizes[0], sizes[1], sizes[2], sizes[3]);
    
    return size_required;
}

static int mjpeg_parse_memory_check(int size_required)
{
    int mem_avail;
    #ifdef __linux__
        mem_avail = av_get_mem_available();
        if (size_required > (int)(mem_avail * 0.85)) {
    #else
        #include <hcuapi/mmz.h>
        mem_avail = mmz_available(MMZ_ID_SYSMEM);
        if (size_required > (int)(mem_avail * 0.9)) {
    #endif
        av_log(NULL, AV_LOG_VERBOSE, "memory is not enough, size_required %d, mem_avail %d, lowes++\n",
            size_required, mem_avail);
        return -1;
    } else {
        av_log(NULL, AV_LOG_VERBOSE, "size_required %d, mem_avail %d\n", size_required, mem_avail);
    }

    return 0;
}

static int mjpeg_parse_get_pixfmt(MJPEGParserContext *s)
{
    int pix_fmt = AV_PIX_FMT_NONE;
    switch (s->pix_fmt_id) {
        case 0x11110000: /* for bayer-encoded huffman lossless JPEGs embedded in DNGs */
            pix_fmt = AV_PIX_FMT_GRAY16LE;
            break;
        case 0x11111100:
            if (s->rgb) {
                pix_fmt = s->bits <= 9 ? AV_PIX_FMT_BGR24 : AV_PIX_FMT_BGR48;
            } else {
                if (   s->adobe_transform == 0
                    || s->component_id[0] == 'R' - 1 && s->component_id[1] == 'G' - 1 && s->component_id[2] == 'B' - 1) {
                    pix_fmt = s->bits <= 8 ? AV_PIX_FMT_GBRP : AV_PIX_FMT_GBRP16;
                } else {
                    if (s->bits <= 8) pix_fmt = s->cs_itu601 ? AV_PIX_FMT_YUV444P : AV_PIX_FMT_YUVJ444P;
                    else              pix_fmt = AV_PIX_FMT_YUV444P16;
                }
            }
            break;
        case 0x11111111:
            if (s->rgb) {
                pix_fmt = s->bits <= 9 ? AV_PIX_FMT_ABGR : AV_PIX_FMT_RGBA64;
            } else {
                if (s->adobe_transform == 0 && s->bits <= 8) {
                    pix_fmt = AV_PIX_FMT_GBRAP;
                } else {
                    pix_fmt = s->bits <= 8 ? AV_PIX_FMT_YUVA444P : AV_PIX_FMT_YUVA444P16;
                }
            }
            break;
        case 0x22111122:
        case 0x22111111:
            if (s->adobe_transform == 0 && s->bits <= 8) {
                pix_fmt = AV_PIX_FMT_GBRAP;
            } else if (s->adobe_transform == 2 && s->bits <= 8) {
                pix_fmt = AV_PIX_FMT_YUVA444P;
            } else {
                if (s->bits <= 8) pix_fmt = AV_PIX_FMT_YUVA420P;
                else              pix_fmt = AV_PIX_FMT_YUVA420P16;
            }
            break;
        case 0x12121100:
        case 0x22122100:
        case 0x21211100:
        case 0x22211200:
        case 0x22221100:
        case 0x22112200:
        case 0x11222200:
            if (s->bits <= 8) pix_fmt = s->cs_itu601 ? AV_PIX_FMT_YUV444P : AV_PIX_FMT_YUVJ444P;
            else goto unk_pixfmt;
            break;
        case 0x11000000:
        case 0x13000000:
        case 0x14000000:
        case 0x31000000:
        case 0x33000000:
        case 0x34000000:
        case 0x41000000:
        case 0x43000000:
        case 0x44000000:
            if(s->bits <= 8)
                pix_fmt = AV_PIX_FMT_GRAY8;
            else
                pix_fmt = AV_PIX_FMT_GRAY16;
            break;
        case 0x12111100:
        case 0x14121200:
        case 0x14111100:
        case 0x22211100:
        case 0x22112100:
            if (s->component_id[0] == 'Q' && s->component_id[1] == 'F' && s->component_id[2] == 'A') {
                if (s->bits <= 8) pix_fmt = AV_PIX_FMT_GBRP;
                else goto unk_pixfmt;
            } else {
                if (s->bits <= 8) pix_fmt = s->cs_itu601 ? AV_PIX_FMT_YUV440P : AV_PIX_FMT_YUVJ440P;
                else goto unk_pixfmt;
            }
            break;
        case 0x21111100:
            if (s->component_id[0] == 'Q' && s->component_id[1] == 'F' && s->component_id[2] == 'A') {
                if (s->bits <= 8) pix_fmt = AV_PIX_FMT_GBRP;
                else goto unk_pixfmt;
            } else {
                if (s->bits <= 8) pix_fmt = s->cs_itu601 ? AV_PIX_FMT_YUV422P : AV_PIX_FMT_YUVJ422P;
                else              pix_fmt = AV_PIX_FMT_YUV422P16;
            }
            break;
        case 0x31111100:
            if (s->bits > 8)
                goto unk_pixfmt;
            pix_fmt = s->cs_itu601 ? AV_PIX_FMT_YUV444P : AV_PIX_FMT_YUVJ444P;
            break;
        case 0x22121100:
        case 0x22111200:
            if (s->bits <= 8) pix_fmt = s->cs_itu601 ? AV_PIX_FMT_YUV422P : AV_PIX_FMT_YUVJ422P;
            else goto unk_pixfmt;
            break;
        case 0x22111100:
        case 0x23111100:
        case 0x42111100:
        case 0x24111100:
            if (s->bits <= 8) pix_fmt = s->cs_itu601 ? AV_PIX_FMT_YUV420P : AV_PIX_FMT_YUVJ420P;
            else              pix_fmt = AV_PIX_FMT_YUV420P16;
            break;
        case 0x41111100:
            if (s->bits <= 8) pix_fmt = s->cs_itu601 ? AV_PIX_FMT_YUV411P : AV_PIX_FMT_YUVJ411P;
            else goto unk_pixfmt;
            break;
        default:
unk_pixfmt:
            av_log(NULL, AV_LOG_WARNING, "unknow pixfmt\n");
            break;
    }
    if (s->ls) {
        if (s->nb_components == 3) {
            pix_fmt = AV_PIX_FMT_RGB24;
        } else if (s->nb_components != 1) {
            av_log(NULL, AV_LOG_WARNING, "unknow pixfmt2\n");
        } else if (s->palette_index && s->bits <= 8) {
            pix_fmt = AV_PIX_FMT_PAL8;
        } else if (s->bits <= 8) {
            pix_fmt = AV_PIX_FMT_GRAY8;
        } else {
            pix_fmt = AV_PIX_FMT_GRAY16;
        }
    }

    return pix_fmt;
}

static int mjpeg_parse_sof (GetBitContext *gb, AVCodecContext *avctx, MJPEGParserContext *s)
{
    int i;
    uint8_t value = 0;
    uint16_t len = 0;
    int hw_support = 1;

    if (get_bits_left(gb) < 16) {
        return AVERROR_INVALIDDATA;
    }

    len = get_bits(gb, 16);
    len -= 2;
    if (len < 4 || 8 * len > get_bits_left(gb)) {
        return AVERROR_INVALIDDATA;
    }

    /* Get sample precision */
    s->bits = get_bits(gb, 8);len--;
    avctx->coded_height = avctx->height = get_bits(gb, 16);len -= 2;
    avctx->coded_width = avctx->width = get_bits(gb, 16);len -= 2;
    av_log(NULL, AV_LOG_VERBOSE,
        "precision: %d, width %d, height %d\n",
        s->bits, avctx->height, avctx->width);

    /* Get number of components */
    s->nb_components = get_bits(gb, 8);len--;
    if (s->nb_components > 4 || len < 3 * s->nb_components) {
        return AVERROR_INVALIDDATA;
    }

    /* Get decimation and quantization table id for each component */
    s->h_max = s->v_max = 1;
    for (i = 0; i < s->nb_components; i++) {
        /* Get component ID number */
        s->component_id[i] = get_bits(gb, 8) - 1;len--;

        /* Get decimation */
        value = get_bits(gb, 8);len--;
        s->h_count[i] = (value & 0xf0) >> 4;
        s->h_max = max(s->h_count[i], s->h_max);
        s->v_count[i] = (value & 0x0f);
        s->v_max = max(s->v_count[i], s->v_max);

        /* Get quantization table id */
        s->quant_index[i] = get_bits(gb, 8);len--;
        av_log(NULL, AV_LOG_VERBOSE,
            "component_id[%d]: %d, quant_index[%d]: %d,"
            "h_count[%d]: %d, v_count[%d]: %d\n",
            i, s->component_id[i], i, s->quant_index[i], i, s->h_count[i], i, s->v_count[i]);
    }

    if (s->progressive) {
        av_log(NULL, AV_LOG_VERBOSE, "hw do not support progressive\n");
        hw_support = 0;
    }

    if (s->nb_components == 1) {
        //GRAYSCALE/monochrome support hw huffman.
    }

    if (s->v_max == 1 && s->h_max == 1 && s->lossless == 1 &&
        (s->nb_components==3 || s->nb_components == 4))
        s->rgb = 1;
    else if (!s->lossless)
        s->rgb = 0;

    av_log(NULL, AV_LOG_DEBUG, "rgb %d\n", s->rgb);
    if (!s->rgb){
        //hw huffman only support: Y 1x1,2x1,1x2,2x2, UV 1x1
        if(s->h_count[1] == 2 || s->v_count[1] == 2
            || s->h_count[2]== 2 || s->v_count[2] == 2) {
            av_log(NULL, AV_LOG_VERBOSE, "hw only support Y 1x1,2x1,1x2,2x2, UV 1x1,"
                "but currrent U %dx%d, current V %dx%d\n",
                s->h_count[1], s->v_count[1], s->h_count[2], s->v_count[2]);
            hw_support = 0;
        }
    }

    if (s->nb_components == 4 && s->component_id[0] == 'C' - 1 && s->component_id[1] == 'M' - 1
        && s->component_id[2] == 'Y' - 1 && s->component_id[3] == 'K' - 1) {
        av_log(NULL, AV_LOG_VERBOSE, "hw do not support CMYK\n");
        hw_support = 0;
        s->adobe_transform = 0;
    }

    s->pix_fmt_id= ((unsigned)s->h_count[0] << 28) | (s->v_count[0] << 24) |
        (s->h_count[1] << 20) | (s->v_count[1] << 16) |
        (s->h_count[2] << 12) | (s->v_count[2] << 8) |
        (s->h_count[3] << 4) | s->v_count[3];

    if (0x11111100 == s->pix_fmt_id &&
        s->component_id[0] == 'R' - 1 && s->component_id[1] == 'G' - 1 && 
        s->component_id[2] == 'B' - 1) {
        av_log(NULL, AV_LOG_VERBOSE, "hw do not support rgb\n");
        hw_support = 0;
    }


    if (0x11111111 == s->pix_fmt_id && s->rgb) {
        av_log(NULL, AV_LOG_VERBOSE, "hw do not support rgba\n");
        hw_support = 0;
    }

    if (hw_support) {
        avctx->pic_hw_support = 1;
        if (1 == s->h_count[0] && 1 == s->v_count[0]) {
            avctx->scan_type = YUV444_YH1V1;
        } else if (2 == s->h_count[0] && 1 == s->v_count[0]) {
            avctx->scan_type = YUV422_YH2V1;
        } else if (1 == s->h_count[0] && 2 == s->v_count[0]) {
            avctx->scan_type = YUV420_YH1V2;
        } else if (2 == s->h_count[0] && 2 == s->v_count[0]) {
            avctx->scan_type = YUV420_YH2V2;
        } else if (4 == s->h_count[0] && 1 == s->v_count[0]) {
            avctx->scan_type = YUV411_YH4V1;
        }
    } else {
        int size_required, ret;
        mjpeg_parse_estimate_lowres(avctx);
        s->pix_fmt = mjpeg_parse_get_pixfmt(s);
        size_required = mjpeg_parse_calc_memory_required(avctx, s);
        ret = mjpeg_parse_memory_check(size_required);
        if (ret < 0) {
            if (avctx->lowres < 3) {
                avctx->lowres++;
            } else {
                av_log(NULL, AV_LOG_WARNING, "avctx->lowres has already setted to 3\n");
            }
        }
        av_log(NULL, AV_LOG_VERBOSE, "avctx->lowres %d\n", avctx->lowres);
    }
    av_log(NULL, AV_LOG_VERBOSE, "hw_support %d\n", hw_support);

    if (len > 0)
        skip_bits(gb, 8 * len);
    return 0;
}

static int jpeg_parse_info(AVCodecParserContext *s, AVCodecContext *avctx, const uint8_t *buf, int buf_size)
{
    int ret;
    GetBitContext gb;
    uint8_t marker;
    int foundSOF = 0;
    MJPEGParserContext *m = (MJPEGParserContext *)s->priv_data;

    if (!buf || buf_size <= 0) {
        return AVERROR_INVALIDDATA;
    }

    av_log(NULL, AV_LOG_TRACE, "avctx->coded_width %d\n", avctx->coded_width);
    if (avctx->coded_width != 0 && avctx->profile != FF_PROFILE_UNKNOWN) {
        return 0;
    }

    ret = init_get_bits8(&gb, buf, buf_size);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "jpeg parser invalid buffer\n");
        return -1;
    }

    m->adobe_transform = -1;

    marker = get_bits(&gb, 8);
    while(marker == 0xff) {
        marker = get_bits(&gb, 8);

        av_log(NULL, AV_LOG_TRACE, "marker = %x\n", marker);

        switch (marker) {
            case SOS:
                /* start of scan (begins compressed data) */
                goto done;

            case SOI:
                break;

            case DRI: {
                if(mjpeg_parse_dri(&gb, avctx))
                    return AVERROR_INVALIDDATA;
                break;
            }

            case COM: {
                if(mjpeg_parse_com(&gb, avctx, m))
                    return AVERROR_INVALIDDATA;
                break;
            }

            case DHT:
            case DQT:
                /* Ignore these codes */
                if (mjpeg_parse_skip_marker (&gb, avctx))
                    return AVERROR_INVALIDDATA;
                break;

            case SOF0:
                avctx->profile = FF_PROFILE_MJPEG_HUFFMAN_BASELINE_DCT;
                foundSOF = 1;
                if (mjpeg_parse_sof (&gb, avctx, m))
                    return AVERROR_INVALIDDATA;
                break;

            case SOF1:
                avctx->profile = FF_PROFILE_MJPEG_HUFFMAN_EXTENDED_SEQUENTIAL_DCT;
                foundSOF = 1;
                if (mjpeg_parse_sof (&gb, avctx, m))
                    return AVERROR_INVALIDDATA;
                break;

            case SOF2:
                avctx->profile = FF_PROFILE_MJPEG_HUFFMAN_PROGRESSIVE_DCT;
                foundSOF = 1;
                m->progressive = 1;
                if (mjpeg_parse_sof (&gb, avctx, m))
                    return AVERROR_INVALIDDATA;
                break;

            case SOF3:
                avctx->profile = FF_PROFILE_MJPEG_HUFFMAN_LOSSLESS;
                foundSOF = 1;
                m->lossless = 1;
                if (mjpeg_parse_sof (&gb, avctx, m))
                    return AVERROR_INVALIDDATA;
                break;

            case SOF48:
                avctx->profile = FF_PROFILE_MJPEG_HUFFMAN_LOSSLESS;
                foundSOF = 1;
                m->lossless = 1;
                m->ls = 1;
                if (mjpeg_parse_sof (&gb, avctx, m))
                    return AVERROR_INVALIDDATA;
                break;

            case APP1:
            case APP13:
                if (mjpeg_parse_app (&gb, avctx, s))
                    return AVERROR_INVALIDDATA;
                break;

            default:
                if (marker == JPG || (marker >= JPG0 && marker <= JPG13) ||
                    (marker >= APP0 && marker <= APP15)) {
                    if (mjpeg_parse_skip_marker (&gb, avctx))
                        return AVERROR_INVALIDDATA;
                } else {
                    av_log(NULL, AV_LOG_ERROR, "unknow marker = %x\n", marker);
                    if (mjpeg_parse_skip_marker (&gb, avctx))
                        return AVERROR_INVALIDDATA;
                }
        }

        if (get_bits_left(&gb) < 8) {
            return AVERROR_INVALIDDATA;
        }

        marker = get_bits(&gb, 8);
    }

done:
    return foundSOF ? 0 : AVERROR_INVALIDDATA;
}

static int jpeg_parse(AVCodecParserContext *s,
                      AVCodecContext *avctx,
                      const uint8_t **poutbuf, int *poutbuf_size,
                      const uint8_t *buf, int buf_size)
{
    MJPEGParserContext *m = (MJPEGParserContext *)s->priv_data;
    ParseContext *pc = &m->pc;
    int next = buf_size;

    if (poutbuf && poutbuf_size) {
        if(s->flags & PARSER_FLAG_COMPLETE_FRAMES){
            next= buf_size;
        }else{
            next= find_frame_end(m, buf, buf_size);

            if (ff_combine_frame(pc, next, &buf, &buf_size) < 0) {
                *poutbuf = NULL;
                *poutbuf_size = 0;
                return buf_size;
            }
        }

        *poutbuf = buf;
        *poutbuf_size = buf_size;
    }

    jpeg_parse_info(s, avctx, buf, buf_size);

    return next;
}


AVCodecParser ff_mjpeg_parser = {
    .codec_ids      = { AV_CODEC_ID_MJPEG, AV_CODEC_ID_JPEGLS },
    .priv_data_size = sizeof(MJPEGParserContext),
    .parser_parse   = jpeg_parse,
    .parser_close   = ff_parse_close,
};
