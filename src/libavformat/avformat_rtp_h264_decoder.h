
#ifndef __RTP_H264_DECODER_H__
#define __RTP_H264_DECODER_H__

#if 0
/* NAL unit types (copied from libavcodec/h264.h)*/
enum {
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
#endif
int rtp_h264_get_nalu_type(AVFormatContext *ctx, const char * buf, int len);


int rtp_h264_set_start_end_bit(AVFormatContext *ctx, int start_bit, int end_bit);
int rtp_h264_set_marker(AVFormatContext *ctx, int marker);
void print_fast_h264_parse_flags(AVFormatContext * ctx);

#endif /* __RTP_H264_DECODER_H__ */
