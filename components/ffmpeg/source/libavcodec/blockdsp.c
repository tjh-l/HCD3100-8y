/*
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

#include <stdint.h>
#include <string.h>

#include "config.h"
#include "libavutil/attributes.h"
#include "avcodec.h"
#include "blockdsp.h"
#include "version.h"

static void clear_block_c(int16_t *block)
{
    memset(block, 0, sizeof(int16_t) * 64);
}

static void clear_blocks_c(int16_t *blocks)
{
    memset(blocks, 0, sizeof(int16_t) * 6 * 64);
}

static void fill_block16_c(uint8_t *block, uint8_t value, ptrdiff_t line_size,
                           int h)
{
    int i;

    for (i = 0; i < h; i++) {
        memset(block, value, 16);
        block += line_size;
    }
}

static void fill_block8_c(uint8_t *block, uint8_t value, ptrdiff_t line_size,
                          int h)
{
    int i;

    for (i = 0; i < h; i++) {
        memset(block, value, 8);
        block += line_size;
    }
}

static void ff_blockdsp_lowres_compact_init(BlockDSPContext *c, int lowres);
av_cold void ff_blockdsp_init(BlockDSPContext *c, AVCodecContext *avctx)
{
    if (avctx->lowres > 0 && (avctx->vdec_knobs & VDEC_KNOB_LOWRES_COMPACT)) {
        return ff_blockdsp_lowres_compact_init(c, avctx->lowres);
    }

    c->clear_block  = clear_block_c;
    c->clear_blocks = clear_blocks_c;

    c->fill_block_tab[0] = fill_block16_c;
    c->fill_block_tab[1] = fill_block8_c;

    if (ARCH_ALPHA)
        ff_blockdsp_init_alpha(c);
    if (ARCH_ARM)
        ff_blockdsp_init_arm(c);
    if (ARCH_PPC)
        ff_blockdsp_init_ppc(c);
    if (ARCH_X86)
        ff_blockdsp_init_x86(c, avctx);
    if (ARCH_MIPS)
        ff_blockdsp_init_mips(c);
}

static void clear_block_4x4_c(int16_t *block)
{
    memset(block, 0, sizeof(int16_t) * 16);
}

static void clear_block_2x2_c(int16_t *block)
{
    block[0] = 0;
    block[1] = 0;
    block[2] = 0;
    block[3] = 0;
}

static void clear_block_1x1_c(int16_t *block)
{
    *block = 0;
}

static void ff_blockdsp_lowres_compact_init(BlockDSPContext *c, int lowres)
{
    switch (lowres) {
        case 1: c->clear_block  = clear_block_4x4_c;break;
        case 2: c->clear_block  = clear_block_2x2_c;break;
        case 3: c->clear_block  = clear_block_1x1_c;break;
        default: c->clear_block = NULL;break;
    }
    c->clear_blocks = NULL;

    c->fill_block_tab[0] = NULL;
    c->fill_block_tab[1] = NULL;
}
