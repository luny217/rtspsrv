/*
 * H.26L/H.264/AVC/JVT/14496-10/... decoder
 * Copyright (c) 2003 Michael Niedermayer <michaelni@gmx.at>
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
 * H.264 / AVC / MPEG4 part10 codec.
 * @author Michael Niedermayer <michaelni@gmx.at>
 */

#define UNCHECKED_BITSTREAM_READER 1

#include "libavutil/avutil_avassert.h"
#include "libavutil/avutil_opt.h"
#include "libavutil/avutil_timer.h"
#include "avcodec_internal.h"
#include "avcodec.h"
#include "avcodec_h264.h"
#include "avcodec_golomb.h"

#include <assert.h>

const uint8_t * ff_h264_decode_nal(H264Context * h, const uint8_t * src,
                                   int * dst_length, int * consumed, int length)
{
    int i, si, di;
    uint8_t * dst;
    int bufidx;

    // src[0]&0x80; // forbidden bit
    h->nal_ref_idc   = src[0] >> 5;
    h->nal_unit_type = src[0] & 0x1F;

    src++;
    length--;

#define STARTCODE_TEST                                                  \
    if (i + 2 < length && src[i + 1] == 0 && src[i + 2] <= 3) {         \
        if (src[i + 2] != 3) {                                          \
            /* startcode, so we must be past the end */                 \
            length = i;                                                 \
        }                                                               \
        break;                                                          \
    }

#if HAVE_FAST_UNALIGNED
#define FIND_FIRST_ZERO                                                 \
    if (i > 0 && !src[i])                                               \
        i--;                                                            \
    while (src[i])                                                      \
        i++

#if HAVE_FAST_64BIT
    for (i = 0; i + 1 < length; i += 9)
    {
        if (!((~AV_RN64A(src + i) &
                (AV_RN64A(src + i) - 0x0100010001000101ULL)) &
                0x8000800080008080ULL))
            continue;
        FIND_FIRST_ZERO;
        STARTCODE_TEST;
        i -= 7;
    }
#else
    for (i = 0; i + 1 < length; i += 5)
    {
        if (!((~AV_RN32A(src + i) &
                (AV_RN32A(src + i) - 0x01000101U)) &
                0x80008080U))
            continue;
        FIND_FIRST_ZERO;
        STARTCODE_TEST;
        i -= 3;
    }
#endif
#else
    for (i = 0; i + 1 < length; i += 2)
    {
        if (src[i])
            continue;
        if (i > 0 && src[i - 1] == 0)
            i--;
        STARTCODE_TEST;
    }
#endif

    // use second escape buffer for inter data
    bufidx = h->nal_unit_type == NAL_DPC ? 1 : 0;

    si = h->rbsp_buffer_size[bufidx];
    av_fast_padded_malloc(&h->rbsp_buffer[bufidx], &h->rbsp_buffer_size[bufidx], length + MAX_MBPAIR_SIZE);
    dst = h->rbsp_buffer[bufidx];

    if (dst == NULL)
        return NULL;

    if (i >= length - 1) //no escaped 0
    {
        *dst_length = length;
        *consumed = length + 1; //+1 for the header
        if (h->avctx->flags2 & CODEC_FLAG2_FAST)
        {
            return src;
        }
        else
        {
            memcpy(dst, src, length);
            return dst;
        }
    }

    memcpy(dst, src, i);
    si = di = i;
    while (si + 2 < length)
    {
        // remove escapes (very rare 1:2^22)
        if (src[si + 2] > 3)
        {
            dst[di++] = src[si++];
            dst[di++] = src[si++];
        }
        else if (src[si] == 0 && src[si + 1] == 0)
        {
            if (src[si + 2] == 3)   // escape
            {
                dst[di++]  = 0;
                dst[di++]  = 0;
                si        += 3;
                continue;
            }
            else   // next start code
                goto nsc;
        }

        dst[di++] = src[si++];
    }
    while (si < length)
        dst[di++] = src[si++];

nsc:
    memset(dst + di, 0, FF_INPUT_BUFFER_PADDING_SIZE);

    *dst_length = di;
    *consumed   = si + 1; // +1 for the header
    /* FIXME store exact number of bits in the getbitcontext
     * (it is needed for decoding) */
    return dst;
}

int ff_pred_weight_table(H264Context * h)
{
    int list, i;
    int luma_def, chroma_def;

    h->use_weight             = 0;
    h->use_weight_chroma      = 0;
    h->luma_log2_weight_denom = get_ue_golomb(&h->gb);
    if (h->sps.chroma_format_idc)
        h->chroma_log2_weight_denom = get_ue_golomb(&h->gb);
    luma_def   = 1 << h->luma_log2_weight_denom;
    chroma_def = 1 << h->chroma_log2_weight_denom;

    for (list = 0; list < 2; list++)
    {
        h->luma_weight_flag[list]   = 0;
        h->chroma_weight_flag[list] = 0;
        for (i = 0; i < h->ref_count[list]; i++)
        {
            int luma_weight_flag, chroma_weight_flag;

            luma_weight_flag = get_bits1(&h->gb);
            if (luma_weight_flag)
            {
                h->luma_weight[i][list][0] = get_se_golomb(&h->gb);
                h->luma_weight[i][list][1] = get_se_golomb(&h->gb);
                if (h->luma_weight[i][list][0] != luma_def ||
                        h->luma_weight[i][list][1] != 0)
                {
                    h->use_weight             = 1;
                    h->luma_weight_flag[list] = 1;
                }
            }
            else
            {
                h->luma_weight[i][list][0] = luma_def;
                h->luma_weight[i][list][1] = 0;
            }

            if (h->sps.chroma_format_idc)
            {
                chroma_weight_flag = get_bits1(&h->gb);
                if (chroma_weight_flag)
                {
                    int j;
                    for (j = 0; j < 2; j++)
                    {
                        h->chroma_weight[i][list][j][0] = get_se_golomb(&h->gb);
                        h->chroma_weight[i][list][j][1] = get_se_golomb(&h->gb);
                        if (h->chroma_weight[i][list][j][0] != chroma_def ||
                                h->chroma_weight[i][list][j][1] != 0)
                        {
                            h->use_weight_chroma        = 1;
                            h->chroma_weight_flag[list] = 1;
                        }
                    }
                }
                else
                {
                    int j;
                    for (j = 0; j < 2; j++)
                    {
                        h->chroma_weight[i][list][j][0] = chroma_def;
                        h->chroma_weight[i][list][j][1] = 0;
                    }
                }
            }
        }
        if (h->slice_type_nos != AV_PICTURE_TYPE_B)
            break;
    }
    h->use_weight = h->use_weight || h->use_weight_chroma;
    return 0;
}

/* forget old pics after a seek */
void ff_h264_flush_change(H264Context * h)
{
    int i, j;
    
    h->prev_frame_num = -1;
    if (h->cur_pic_ptr)
    {
        h->cur_pic_ptr->reference = 0;
        for (j = i = 0; h->delayed_pic[i]; i++)
            if (h->delayed_pic[i] != h->cur_pic_ptr)
                h->delayed_pic[j++] = h->delayed_pic[i];
        h->delayed_pic[j] = NULL;
    }
    h->list_count = 0;
}

int ff_init_poc(H264Context * h, int pic_field_poc[2], int * pic_poc)
{
    const int max_frame_num = 1 << h->sps.log2_max_frame_num;
    int field_poc[2];

    h->frame_num_offset = h->prev_frame_num_offset;
    if (h->frame_num < h->prev_frame_num)
        h->frame_num_offset += max_frame_num;

    if (h->sps.poc_type == 0)
    {
        const int max_poc_lsb = 1 << h->sps.log2_max_poc_lsb;

        if (h->poc_lsb < h->prev_poc_lsb &&
                h->prev_poc_lsb - h->poc_lsb >= max_poc_lsb / 2)
            h->poc_msb = h->prev_poc_msb + max_poc_lsb;
        else if (h->poc_lsb > h->prev_poc_lsb &&
                 h->prev_poc_lsb - h->poc_lsb < -max_poc_lsb / 2)
            h->poc_msb = h->prev_poc_msb - max_poc_lsb;
        else
            h->poc_msb = h->prev_poc_msb;
        field_poc[0] =
            field_poc[1] = h->poc_msb + h->poc_lsb;
        if (h->picture_structure == PICT_FRAME)
            field_poc[1] += h->delta_poc_bottom;
    }
    else if (h->sps.poc_type == 1)
    {
        int abs_frame_num, expected_delta_per_poc_cycle, expectedpoc;
        int i;

        if (h->sps.poc_cycle_length != 0)
            abs_frame_num = h->frame_num_offset + h->frame_num;
        else
            abs_frame_num = 0;

        if (h->nal_ref_idc == 0 && abs_frame_num > 0)
            abs_frame_num--;

        expected_delta_per_poc_cycle = 0;
        for (i = 0; i < h->sps.poc_cycle_length; i++)
            // FIXME integrate during sps parse
            expected_delta_per_poc_cycle += h->sps.offset_for_ref_frame[i];

        if (abs_frame_num > 0)
        {
            int poc_cycle_cnt          = (abs_frame_num - 1) / h->sps.poc_cycle_length;
            int frame_num_in_poc_cycle = (abs_frame_num - 1) % h->sps.poc_cycle_length;

            expectedpoc = poc_cycle_cnt * expected_delta_per_poc_cycle;
            for (i = 0; i <= frame_num_in_poc_cycle; i++)
                expectedpoc = expectedpoc + h->sps.offset_for_ref_frame[i];
        }
        else
            expectedpoc = 0;

        if (h->nal_ref_idc == 0)
            expectedpoc = expectedpoc + h->sps.offset_for_non_ref_pic;

        field_poc[0] = expectedpoc + h->delta_poc[0];
        field_poc[1] = field_poc[0] + h->sps.offset_for_top_to_bottom_field;

        if (h->picture_structure == PICT_FRAME)
            field_poc[1] += h->delta_poc[1];
    }
    else
    {
        int poc = 2 * (h->frame_num_offset + h->frame_num);

        if (!h->nal_ref_idc)
            poc--;

        field_poc[0] = poc;
        field_poc[1] = poc;
    }

    if (h->picture_structure != PICT_BOTTOM_FIELD)
        pic_field_poc[0] = field_poc[0];
    if (h->picture_structure != PICT_TOP_FIELD)
        pic_field_poc[1] = field_poc[1];
    *pic_poc = FFMIN(pic_field_poc[0], pic_field_poc[1]);

    return 0;
}

/**
 * Compute profile from profile_idc and constraint_set?_flags.
 *
 * @param sps SPS
 *
 * @return profile as defined by FF_PROFILE_H264_*
 */
int ff_h264_get_profile(SPS * sps)
{
    int profile = sps->profile_idc;

    switch (sps->profile_idc)
    {
        case FF_PROFILE_H264_BASELINE:
            // constraint_set1_flag set to 1
            profile |= (sps->constraint_set_flags & 1 << 1) ? FF_PROFILE_H264_CONSTRAINED : 0;
            break;
        case FF_PROFILE_H264_HIGH_10:
        case FF_PROFILE_H264_HIGH_422:
        case FF_PROFILE_H264_HIGH_444_PREDICTIVE:
            // constraint_set3_flag set to 1
            profile |= (sps->constraint_set_flags & 1 << 3) ? FF_PROFILE_H264_INTRA : 0;
            break;
    }

    return profile;
}

int ff_set_ref_count(H264Context * h)
{
    int ref_count[2], list_count;
    int num_ref_idx_active_override_flag;

    // set defaults, might be overridden a few lines later
    ref_count[0] = h->pps.ref_count[0];
    ref_count[1] = h->pps.ref_count[1];

    if (h->slice_type_nos != AV_PICTURE_TYPE_I)
    {
        unsigned max[2];
        max[0] = max[1] = h->picture_structure == PICT_FRAME ? 15 : 31;

        num_ref_idx_active_override_flag = get_bits1(&h->gb);

        if (num_ref_idx_active_override_flag)
        {
            ref_count[0] = get_ue_golomb(&h->gb) + 1;
            if (h->slice_type_nos == AV_PICTURE_TYPE_B)
            {
                ref_count[1] = get_ue_golomb(&h->gb) + 1;
            }
            else
                // full range is spec-ok in this case, even for frames
                ref_count[1] = 1;
        }

        if (ref_count[0] - 1 > max[0] || ref_count[1] - 1 > max[1])
        {
            av_log(h->avctx, AV_LOG_ERROR, "reference overflow %u > %u or %u > %u\n", ref_count[0] - 1, max[0], ref_count[1] - 1, max[1]);
            h->ref_count[0] = h->ref_count[1] = 0;
            h->list_count   = 0;
            return AVERROR_INVALIDDATA;
        }

        if (h->slice_type_nos == AV_PICTURE_TYPE_B)
            list_count = 2;
        else
            list_count = 1;
    }
    else
    {
        list_count   = 0;
        ref_count[0] = ref_count[1] = 0;
    }

    if (list_count != h->list_count ||
            ref_count[0] != h->ref_count[0] ||
            ref_count[1] != h->ref_count[1])
    {
        h->ref_count[0] = ref_count[0];
        h->ref_count[1] = ref_count[1];
        h->list_count   = list_count;
        return 1;
    }

    return 0;
}

av_cold void ff_h264_free_context(H264Context * h)
{
    int i;

    //ff_h264_free_tables(h, 1); // FIXME cleanup init stuff perhaps

    for (i = 0; i < MAX_SPS_COUNT; i++)
        av_freep(h->sps_buffers + i);

    for (i = 0; i < MAX_PPS_COUNT; i++)
        av_freep(h->pps_buffers + i);
}

static const AVProfile profiles[] =
{
    { FF_PROFILE_H264_BASELINE,             "Baseline"              },
    { FF_PROFILE_H264_CONSTRAINED_BASELINE, "Constrained Baseline"  },
    { FF_PROFILE_H264_MAIN,                 "Main"                  },
    { FF_PROFILE_H264_EXTENDED,             "Extended"              },
    { FF_PROFILE_H264_HIGH,                 "High"                  },
    { FF_PROFILE_H264_HIGH_10,              "High 10"               },
    { FF_PROFILE_H264_HIGH_10_INTRA,        "High 10 Intra"         },
    { FF_PROFILE_H264_HIGH_422,             "High 4:2:2"            },
    { FF_PROFILE_H264_HIGH_422_INTRA,       "High 4:2:2 Intra"      },
    { FF_PROFILE_H264_HIGH_444,             "High 4:4:4"            },
    { FF_PROFILE_H264_HIGH_444_PREDICTIVE,  "High 4:4:4 Predictive" },
    { FF_PROFILE_H264_HIGH_444_INTRA,       "High 4:4:4 Intra"      },
    { FF_PROFILE_H264_CAVLC_444,            "CAVLC 4:4:4"           },
    { FF_PROFILE_UNKNOWN },
};

static const AVOption h264_options[] =
{
    {"is_avc", "is avc", offsetof(H264Context, is_avc), FF_OPT_TYPE_INT, {.i64 = 0}, 0, 1, 0},
    {"nal_length_size", "nal_length_size", offsetof(H264Context, nal_length_size), FF_OPT_TYPE_INT, {.i64 = 0}, 0, 4, 0},
    {NULL}
};

static const AVClass h264_class =
{
    .class_name = "H264 Decoder",
    .item_name  = av_default_item_name,
    .option     = h264_options,
    .version    = LIBAVUTIL_VERSION_INT,
};

AVCodec ff_h264_decoder =
{
    .name                  = "h264",
    .long_name             = "H.264 / AVC / MPEG-4 AVC / MPEG-4 part 10",
    .type                  = AVMEDIA_TYPE_VIDEO,
    .id                    = AV_CODEC_ID_H264,
    .priv_data_size        = sizeof(H264Context),
    .init                  = NULL,
    .close                 = NULL,
    .capabilities          = /*CODEC_CAP_DRAW_HORIZ_BAND |*/ CODEC_CAP_DR1 |
    CODEC_CAP_DELAY | CODEC_CAP_SLICE_THREADS |
    CODEC_CAP_FRAME_THREADS,
    .flush                 = NULL,
    .profiles              = profiles,
    .priv_class            = &h264_class,
};

