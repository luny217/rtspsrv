/*
 * H.26L/H.264/AVC/JVT/14496-10/... encoder/decoder
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

#ifndef AVCODEC_H264_H
#define AVCODEC_H264_H

#include "libavutil/avutil_intreadwrite.h"
#include "avcodec_get_bits.h"
#include "avcodec_parser.h"
/* picture type */
#define PICT_TOP_FIELD     1
#define PICT_BOTTOM_FIELD  2
#define PICT_FRAME         3

#define H264_MAX_PICTURE_COUNT 36
#define H264_MAX_THREADS       32

#define MAX_SPS_COUNT          32
#define MAX_PPS_COUNT         256

#define MAX_MMCO_COUNT         66

#define MAX_DELAYED_PIC_COUNT  16

#define MAX_MBPAIR_SIZE (256*1024) // a tighter bound could be calculated if someone cares about a few bytes

#define EXTENDED_SAR       255

#define MB_TYPE_REF0       MB_TYPE_ACPRED // dirty but it fits in 16 bit

#define QP_MAX_NUM (51 + 6*6)           // The maximum supported qp

/* NAL unit types */
enum
{
    NAL_SLICE           = 1,
	NAL_DPA             = 2,
	NAL_DPB             = 3,
	NAL_DPC             = 4,
	NAL_IDR_SLICE       = 5,
	NAL_SEI             = 6,
	NAL_SPS             = 7,
	NAL_PPS             = 8,
	NAL_AUD             = 9,
	NAL_END_SEQUENCE    = 10,
	NAL_END_STREAM      = 11,
	NAL_FILLER_DATA     = 12,
	NAL_SPS_EXT         = 13,
	NAL_AUXILIARY_SLICE = 19,
	NAL_FF_IGNORE       = 0xff0f001,
};

/**
 * pic_struct in picture timing SEI message
 */
typedef enum
{
    SEI_PIC_STRUCT_FRAME             = 0, ///<  0: %frame
    SEI_PIC_STRUCT_TOP_FIELD         = 1, ///<  1: top field
    SEI_PIC_STRUCT_BOTTOM_FIELD      = 2, ///<  2: bottom field
    SEI_PIC_STRUCT_TOP_BOTTOM        = 3, ///<  3: top field, bottom field, in that order
    SEI_PIC_STRUCT_BOTTOM_TOP        = 4, ///<  4: bottom field, top field, in that order
    SEI_PIC_STRUCT_TOP_BOTTOM_TOP    = 5, ///<  5: top field, bottom field, top field repeated, in that order
    SEI_PIC_STRUCT_BOTTOM_TOP_BOTTOM = 6, ///<  6: bottom field, top field, bottom field repeated, in that order
    SEI_PIC_STRUCT_FRAME_DOUBLING    = 7, ///<  7: %frame doubling
    SEI_PIC_STRUCT_FRAME_TRIPLING    = 8  ///<  8: %frame tripling
} SEI_PicStructType;

/**
 * Sequence parameter set
 */
typedef struct SPS
{
    unsigned int sps_id;
    int profile_idc;
    int level_idc;
    int chroma_format_idc;
    int transform_bypass;              ///< qpprime_y_zero_transform_bypass_flag
    int log2_max_frame_num;            ///< log2_max_frame_num_minus4 + 4
    int poc_type;                      ///< pic_order_cnt_type
    int log2_max_poc_lsb;              ///< log2_max_pic_order_cnt_lsb_minus4
    int delta_pic_order_always_zero_flag;
    int offset_for_non_ref_pic;
    int offset_for_top_to_bottom_field;
    int poc_cycle_length;              ///< num_ref_frames_in_pic_order_cnt_cycle
    int ref_frame_count;               ///< num_ref_frames
    int gaps_in_frame_num_allowed_flag;
    int mb_width;                      ///< pic_width_in_mbs_minus1 + 1
    int mb_height;                     ///< pic_height_in_map_units_minus1 + 1
    int frame_mbs_only_flag;
    int mb_aff;                        ///< mb_adaptive_frame_field_flag
    int direct_8x8_inference_flag;
    int crop;                          ///< frame_cropping_flag

    /* those 4 are already in luma samples */
    unsigned int crop_left;            ///< frame_cropping_rect_left_offset
    unsigned int crop_right;           ///< frame_cropping_rect_right_offset
    unsigned int crop_top;             ///< frame_cropping_rect_top_offset
    unsigned int crop_bottom;          ///< frame_cropping_rect_bottom_offset
    int vui_parameters_present_flag;
    AVRational sar;
    int video_signal_type_present_flag;
    int full_range;
    int colour_description_present_flag;
    enum AVColorPrimaries color_primaries;
    enum AVColorTransferCharacteristic color_trc;
    enum AVColorSpace colorspace;
    int timing_info_present_flag;
    uint32_t num_units_in_tick;
    uint32_t time_scale;
    int fixed_frame_rate_flag;
    short offset_for_ref_frame[256]; // FIXME dyn aloc?
    int bitstream_restriction_flag;
    int num_reorder_frames;
    int scaling_matrix_present;
    uint8_t scaling_matrix4[6][16];
    uint8_t scaling_matrix8[6][64];
    int nal_hrd_parameters_present_flag;
    int vcl_hrd_parameters_present_flag;
    int pic_struct_present_flag;
    int time_offset_length;
    int cpb_cnt;                          ///< See H.264 E.1.2
    int initial_cpb_removal_delay_length; ///< initial_cpb_removal_delay_length_minus1 + 1
    int cpb_removal_delay_length;         ///< cpb_removal_delay_length_minus1 + 1
    int dpb_output_delay_length;          ///< dpb_output_delay_length_minus1 + 1
    int bit_depth_luma;                   ///< bit_depth_luma_minus8 + 8
    int bit_depth_chroma;                 ///< bit_depth_chroma_minus8 + 8
    int residual_color_transform_flag;    ///< residual_colour_transform_flag
    int constraint_set_flags;             ///< constraint_set[0-3]_flag
    int new;                              ///< flag to keep track if the decoder context needs re-init due to changed SPS
} SPS;

/**
 * Picture parameter set
 */
typedef struct PPS
{
    unsigned int sps_id;
    int cabac;                  ///< entropy_coding_mode_flag
    int pic_order_present;      ///< pic_order_present_flag
    int slice_group_count;      ///< num_slice_groups_minus1 + 1
    int mb_slice_group_map_type;
    unsigned int ref_count[2];  ///< num_ref_idx_l0/1_active_minus1 + 1
    int weighted_pred;          ///< weighted_pred_flag
    int weighted_bipred_idc;
    int init_qp;                ///< pic_init_qp_minus26 + 26
    int init_qs;                ///< pic_init_qs_minus26 + 26
    int chroma_qp_index_offset[2];
    int deblocking_filter_parameters_present; ///< deblocking_filter_parameters_present_flag
    int constrained_intra_pred;     ///< constrained_intra_pred_flag
    int redundant_pic_cnt_present;  ///< redundant_pic_cnt_present_flag
    int transform_8x8_mode;         ///< transform_8x8_mode_flag
    uint8_t scaling_matrix4[6][16];
    uint8_t scaling_matrix8[6][64];
    uint8_t chroma_qp_table[2][QP_MAX_NUM + 1]; ///< pre-scaled (with chroma_qp_index_offset) version of qp_table
    int chroma_qp_diff;
} PPS;

/**
 * Memory management control operation opcode.
 */
typedef enum MMCOOpcode
{
    MMCO_END = 0,
    MMCO_SHORT2UNUSED,
    MMCO_LONG2UNUSED,
    MMCO_SHORT2LONG,
    MMCO_SET_MAX_LONG,
    MMCO_RESET,
    MMCO_LONG,
} MMCOOpcode;

typedef struct H264Picture
{
    struct AVFrame f;

    AVBufferRef * qscale_table_buf;
    int8_t * qscale_table;

    AVBufferRef * motion_val_buf[2];
    int16_t (*motion_val[2])[2];

    AVBufferRef * mb_type_buf;
    uint32_t * mb_type;

    AVBufferRef * hwaccel_priv_buf;
    void * hwaccel_picture_private; ///< hardware accelerator private data

    AVBufferRef * ref_index_buf[2];
    int8_t * ref_index[2];

    int field_poc[2];       ///< top/bottom POC
    int poc;                ///< frame POC
    int frame_num;          ///< frame_num (raw frame_num from slice header)
    int mmco_reset;         /**< MMCO_RESET set this 1. Reordering code must
                                 not mix pictures before and after MMCO_RESET. */
    int pic_id;             /**< pic_num (short -> no wrap version of pic_num,
                                 pic_num & max_pic_num; long -> long_pic_num) */
    int long_ref;           ///< 1->long term reference 0->short term reference
    int ref_poc[2][2][32];  ///< POCs of the frames/fields used as reference (FIXME need per slice)
    int ref_count[2][2];    ///< number of entries in ref_poc         (FIXME need per slice)
    int mbaff;              ///< 1 -> MBAFF frame 0-> not MBAFF
    int field_picture;      ///< whether or not picture was encoded in separate fields

    int needs_realloc;      ///< picture needs to be reallocated (eg due to a frame size change)
    int reference;
    int recovered;          ///< picture at IDR or recovery point + recovery count
    int invalid_gap;
    int sei_recovery_frame_cnt;

    int crop;
    int crop_left;
    int crop_top;
} H264Picture;

/**
 * H264Context
 */
typedef struct H264Context
{
    AVCodecContext * avctx;   
    ParseContext parse_context;
    GetBitContext gb;
    H264Picture * cur_pic_ptr;
    H264Picture cur_pic;

    /* coded dimensions -- 16 * mb w/h */
    int width, height;

    unsigned current_sps_id; ///< id of the current SPS
    SPS sps; ///< current sps
    PPS pps; ///< current pps

    int au_pps_id; ///< pps_id of current access unit

    int slice_num;
    uint16_t * slice_table;     ///< slice_table_base + 2*mb_stride + 1
    int slice_type;
    int slice_type_nos;         ///< S free slice type (SI/SP are remapped to I/P)
    int slice_type_fixed;

    // interlacing specific flags
    int picture_structure;

    DECLARE_ALIGNED(8, uint16_t, sub_mb_type)[4];

    // Weighted pred stuff
    int use_weight;
    int use_weight_chroma;
    int luma_log2_weight_denom;
    int chroma_log2_weight_denom;
    // The following 2 can be changed to int8_t but that causes 10cpu cycles speedloss
    int luma_weight[48][2][2];
    int chroma_weight[48][2][2][2];
    int implicit_weight[48][48][2];

    /**
     * num_ref_idx_l0/1_active_minus1 + 1
     */
    unsigned int ref_count[2];          ///< counts frames or fields, depending on current mb mode
    unsigned int list_count;

    int nal_ref_idc;
    int nal_unit_type;
    uint8_t * rbsp_buffer[2];
    unsigned int rbsp_buffer_size[2];

    /**
     * Used to parse AVC variant of h264
     */
    int is_avc;           ///< this flag is != 0 if codec is avc1
    int nal_length_size;  ///< Number of bytes used for nal length (1, 2 or 4)
    int got_first;        ///< this flag is != 0 if we've parsed a frame

    int bit_depth_luma;         ///< luma bit depth from sps to detect changes
    int chroma_format_idc;      ///< chroma format from sps to detect changes

    SPS * sps_buffers[MAX_SPS_COUNT];
    PPS * pps_buffers[MAX_PPS_COUNT];

    int dequant_coeff_pps;      ///< reinit tables when pps changes

    uint16_t * slice_table_base;

    // POC stuff
    int poc_lsb;
    int poc_msb;
    int delta_poc_bottom;
    int delta_poc[2];
    int frame_num;
    int prev_poc_msb;           ///< poc_msb of the last reference pic for POC type 0
    int prev_poc_lsb;           ///< poc_lsb of the last reference pic for POC type 0
    int frame_num_offset;       ///< for POC type 2
    int prev_frame_num_offset;  ///< for POC type 2
    int prev_frame_num;         ///< frame_num of the last pic for POC type 1/2

    H264Picture * delayed_pic[MAX_DELAYED_PIC_COUNT + 2]; // FIXME size?

    enum AVPictureType pict_type;
    
    int luma_weight_flag[2];    ///< 7.4.3.2 luma_weight_lX_flag
    int chroma_weight_flag[2];  ///< 7.4.3.2 chroma_weight_lX_flag

    /**
     * pic_struct in picture timing SEI message
     */
    SEI_PicStructType sei_pic_struct;

    /**
     * Bit set of clock types for fields/frames in picture timing SEI message.
     * For each found ct_type, appropriate bit is set (e.g., bit 1 for
     * interlaced).
     */
    int sei_ct_type;

    /**
     * dpb_output_delay in picture timing SEI message, see H.264 C.2.2
     */
    int sei_dpb_output_delay;

    /**
     * cpb_removal_delay in picture timing SEI message, see H.264 C.1.2
     */
    int sei_cpb_removal_delay;

    /**
     * recovery_frame_cnt from SEI message
     *
     * Set to -1 if no recovery point SEI message found or to number of frames
     * before playback synchronizes. Frames having recovery point are key
     * frames.
     */
    int sei_recovery_frame_cnt;

    /**
     * Are the SEI recovery points looking valid.
     */
    int valid_recovery_point;

    /**
     * recovery_frame is the frame_num at which the next frame should
     * be fully constructed.
     *
     * Set to -1 when not expecting a recovery point.
     */
    int recovery_frame;

    /**
     * We have seen an IDR, so all the following frames in coded order are correctly
     * decodable.
     */
#define FRAME_RECOVERED_IDR  (1 << 0)
    /**
     * Sufficient number of frames have been decoded since a SEI recovery point,
     * so all the following frames in presentation order are correct.
     */
#define FRAME_RECOVERED_SEI  (1 << 1)

    int frame_recovered;    ///< Initial frame has been completely recovered

    int has_recovery_point;


    // Timestamp stuff
    int sei_buffering_period_present;   ///< Buffering period SEI flag

    uint8_t parse_history[6];
    int parse_history_count;
    int parse_last_mb;
} H264Context;

/**
 * Decode SPS
 */
int ff_h264_decode_seq_parameter_set(H264Context * h);

/**
 * compute profile from sps
 */
int ff_h264_get_profile(SPS * sps);

/**
 * Decode PPS
 */
int ff_h264_decode_picture_parameter_set(H264Context * h, int bit_length);

/**
 * Decode a network abstraction layer unit.
 * @param consumed is the number of bytes used as input
 * @param length is the length of the array
 * @param dst_length is the number of decoded bytes FIXME here
 *                   or a decode rbsp tailing?
 * @return decoded bytes, might be src+1 if no escapes
 */
const uint8_t * ff_h264_decode_nal(H264Context * h, const uint8_t * src,
                                   int * dst_length, int * consumed, int length);

/**
 * Free any data that may have been allocated in the H264 context
 * like SPS, PPS etc.
 */
void ff_h264_free_context(H264Context * h);

int ff_init_poc(H264Context * h, int pic_field_poc[2], int * pic_poc);
int ff_pred_weight_table(H264Context * h);
int ff_set_ref_count(H264Context * h);

void ff_h264_flush_change(H264Context * h);



#endif /* AVCODEC_H264_H */
