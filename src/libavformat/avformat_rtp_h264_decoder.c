/*
 * Parse ipc h264 stream, pack NALUs to frame
 */

#include "avformat.h"
#include "avformat_rtp_h264_decoder.h"

int rtp_h264_set_start_end_bit(AVFormatContext *ctx, int start_bit, int end_bit)
{
#ifdef DISABLE_RTP_H264_PARSER
    ctx->start_bit  = start_bit;
    ctx->end_bit    = end_bit;
    /* av_log(ctx, AV_LOG_WARNING, "start_bit = %d end_bit = %d marker = %d\n", start_bit, end_bit, ctx->marker); */
#endif
    return 0;
}

int rtp_h264_set_marker(AVFormatContext *ctx, int marker)
{
#ifdef DISABLE_RTP_H264_PARSER
    ctx->marker = marker;
#endif
    return 0;
}

void print_fast_h264_parse_flags(AVFormatContext * ctx)
{
#ifdef DISABLE_RTP_H264_PARSER
    av_log(ctx, AV_LOG_DEBUG, "<s=%d e=%d m=%d>", ctx->start_bit, ctx->end_bit, ctx->marker);
#endif
}

int rtp_h264_get_nalu_type(AVFormatContext *ctx, const char * buf, int len)
{
#ifdef DISABLE_RTP_H264_PARSER
    int nalu_type = -1;
    /*00 00 00 01 ??*/
    if (len < 5)
    {
        return -1;
    }
    if (buf[0] != 0 || buf[1] != 0 || buf[2] != 0 || buf[3] != 1)
    {
        return -1;
    }

    nalu_type = buf[4] & 0x1F;
    return nalu_type;
#endif
    return 0;
}

