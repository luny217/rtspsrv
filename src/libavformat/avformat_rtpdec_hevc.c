/*
 * RTP parser for HEVC/H.265 payload format (draft version 6)
 * Copyright (c) 2014 Thomas Volkert <thomas@homer-conferencing.com>
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
 *
 */

/* #include "libavutil/avutil_avassert.h" */
#include "libavutil/avutil_string.h"
#include "libavutil/avutil_base64.h"
#include "libavcodec/avcodec_get_bits.h"
#include "libavcodec/avcodec.h"


#include "avformat.h"
#include "avformat_rtpdec.h"
#include "avformat_rtpdec_formats.h"
#include "avformat_rtp_h264_decoder.h"


#define RTP_HEVC_PAYLOAD_HEADER_SIZE       2
#define RTP_HEVC_FU_HEADER_SIZE            1
#define RTP_HEVC_DONL_FIELD_SIZE           2
#define RTP_HEVC_DOND_FIELD_SIZE           1
#define RTP_HEVC_AP_NALU_LENGTH_FIELD_SIZE 2
#define HEVC_SPECIFIED_NAL_UNIT_TYPES      48

/* SDP out-of-band signaling data */
struct PayloadContext
{
    int using_donl_field;
    int profile_id;
    uint8_t * sps, *pps, *vps, *sei;
    int sps_size, pps_size, vps_size, sei_size;
};

static const uint8_t start_sequence[] = { 0x00, 0x00, 0x00, 0x01 };

static int hevc_sdp_parse_fmtp_config(AVStream * stream, PayloadContext * hevc_data, const char * attr, const char * value)
{
    /* profile-space: 0-3 */
    /* profile-id: 0-31 */
    if (!strcmp(attr, "profile-id"))
    {
        hevc_data->profile_id = atoi(value);
        av_log(stream, AV_LOG_WARNING, "SDP: found profile-id: %d\n", hevc_data->profile_id);
    }

    /* tier-flag: 0-1 */
    /* level-id: 0-255 */
    /* interop-constraints: [base16] */
    /* profile-compatibility-indicator: [base16] */
    /* sprop-sub-layer-id: 0-6, defines highest possible value for TID, default: 6 */
    /* recv-sub-layer-id: 0-6 */
    /* max-recv-level-id: 0-255 */
    /* tx-mode: MSM,SSM */
    /* sprop-vps: [base64] */
    /* sprop-sps: [base64] */
    /* sprop-pps: [base64] */
    /* sprop-sei: [base64] */
    if (!strcmp(attr, "sprop-vps") || !strcmp(attr, "sprop-sps") ||
            !strcmp(attr, "sprop-pps") || !strcmp(attr, "sprop-sei"))
    {
        uint8_t ** data_ptr = NULL;
        int * size_ptr = NULL;
        if (!strcmp(attr, "sprop-vps"))
        {
            data_ptr = &hevc_data->vps;
            size_ptr = &hevc_data->vps_size;
        }
        else if (!strcmp(attr, "sprop-sps"))
        {
            data_ptr = &hevc_data->sps;
            size_ptr = &hevc_data->sps_size;
        }
        else if (!strcmp(attr, "sprop-pps"))
        {
            data_ptr = &hevc_data->pps;
            size_ptr = &hevc_data->pps_size;
        }
        else if (!strcmp(attr, "sprop-sei"))
        {
            data_ptr = &hevc_data->sei;
            size_ptr = &hevc_data->sei_size;
        }
        else
            av_assert0(0);

#if 0
        ff_h264_parse_sprop_parameter_sets(s, data_ptr,
                                           size_ptr, value);
#endif
    }

    /* max-lsr, max-lps, max-cpb, max-dpb, max-br, max-tr, max-tc */
    /* max-fps */

    /* sprop-max-don-diff: 0-32767

         When the RTP stream depends on one or more other RTP
         streams (in this case tx-mode MUST be equal to "MSM" and
         MSM is in use), this parameter MUST be present and the
         value MUST be greater than 0.
    */
    if (!strcmp(attr, "sprop-max-don-diff"))
    {
        if (atoi(value) > 0)
            hevc_data->using_donl_field = 1;
        av_dlog(stream, "Found sprop-max-don-diff in SDP, DON field usage is: %d\n",
                hevc_data->using_donl_field);
    }

    /* sprop-depack-buf-nalus: 0-32767 */
    if (!strcmp(attr, "sprop-depack-buf-nalus"))
    {
        if (atoi(value) > 0)
            hevc_data->using_donl_field = 1;
        av_dlog(stream, "Found sprop-depack-buf-nalus in SDP, DON field usage is: %d\n",
                hevc_data->using_donl_field);
    }

    /* sprop-depack-buf-bytes: 0-4294967295 */
    /* depack-buf-cap */
    /* sprop-segmentation-id: 0-3 */
    /* sprop-spatial-segmentation-idc: [base16] */
    /* dec-parallel-ca: */
    /* include-dph */

    return 0;
}

static av_cold int hevc_parse_sdp_line(AVFormatContext * ctx, int st_index,
                                       PayloadContext * hevc_data, const char * line)
{
    AVStream * current_stream;
    AVCodecContext * codec;
    const char * sdp_line_ptr = line;

    if (st_index < 0)
        return 0;

    current_stream = ctx->streams[st_index];
    codec  = current_stream->codec;

    if (av_strstart(sdp_line_ptr, "framesize:", &sdp_line_ptr))
    {
        /* ff_h264_parse_framesize(codec, sdp_line_ptr); */
    }
    else if (av_strstart(sdp_line_ptr, "fmtp:", &sdp_line_ptr))
    {
        int ret = ff_parse_fmtp(current_stream, hevc_data, sdp_line_ptr, hevc_sdp_parse_fmtp_config);
        if (hevc_data->vps_size || hevc_data->sps_size ||
                hevc_data->pps_size || hevc_data->sei_size)
        {
            av_freep(&codec->extradata);
            codec->extradata_size = hevc_data->vps_size + hevc_data->sps_size +
                                    hevc_data->pps_size + hevc_data->sei_size;
            codec->extradata = av_malloc(codec->extradata_size +
                                         FF_INPUT_BUFFER_PADDING_SIZE);
            if (!codec->extradata)
            {
                ret = AVERROR(ENOMEM);
                codec->extradata_size = 0;
            }
            else
            {
                int pos = 0;
                memcpy(codec->extradata + pos, hevc_data->vps, hevc_data->vps_size);
                pos += hevc_data->vps_size;
                memcpy(codec->extradata + pos, hevc_data->sps, hevc_data->sps_size);
                pos += hevc_data->sps_size;
                memcpy(codec->extradata + pos, hevc_data->pps, hevc_data->pps_size);
                pos += hevc_data->pps_size;
                memcpy(codec->extradata + pos, hevc_data->sei, hevc_data->sei_size);
                pos += hevc_data->sei_size;
                memset(codec->extradata + pos, 0, FF_INPUT_BUFFER_PADDING_SIZE);
            }

            av_freep(&hevc_data->vps);
            av_freep(&hevc_data->sps);
            av_freep(&hevc_data->pps);
            av_freep(&hevc_data->sei);
            hevc_data->vps_size = 0;
            hevc_data->sps_size = 0;
            hevc_data->pps_size = 0;
            hevc_data->sei_size = 0;
        }
        return ret;
    }

    return 0;
}


int ff_h264_handle_aggregated_packet(AVFormatContext * ctx, PayloadContext * data, AVPacket * pkt,
                                     const uint8_t * buf, int len,
                                     int skip_between, int * nal_counters,
                                     int nal_mask)
{
    int pass         = 0;
    int total_length = 0;
    uint8_t * dst     = NULL;
    int ret;

    /*  first we are going to figure out the total size */
    for (pass = 0; pass < 2; pass++)
    {
        const uint8_t * src = buf;
        int src_len        = len;

        while (src_len > 2)
        {
            uint16_t nal_size = AV_RB16(src);

            /*  consume the length of the aggregate */
            src     += 2;
            src_len -= 2;

            if (nal_size <= src_len)
            {
                if (pass == 0)
                {
                    /*  counting */
                    total_length += sizeof(start_sequence) + nal_size;
                }
                else
                {
                    /*  copying */
                    memcpy(dst, start_sequence, sizeof(start_sequence));
                    dst += sizeof(start_sequence);
                    memcpy(dst, src, nal_size);
                    if (nal_counters)
                        nal_counters[(*src) & nal_mask]++;
                    dst += nal_size;
                }
            }
            else
            {
                av_log(ctx, AV_LOG_ERROR,
                       "nal size exceeds length: %d %d\n", nal_size, src_len);
                return AVERROR_INVALIDDATA;
            }

            /*  eat what we handled */
            src     += nal_size + skip_between;
            src_len -= nal_size + skip_between;
        }

        if (pass == 0)
        {
            /* now we know the total size of the packet (with the
             * start sequences added) */
            if ((ret = av_new_packet(pkt, total_length)) < 0)
                return ret;
            dst = pkt->data;
        }
    }

    return 0;
}

int ff_h264_handle_frag_packet(AVPacket * pkt, const uint8_t * buf, int len,
                               int start_bit, const uint8_t * nal_header,
                               int nal_header_len)
{
    int ret;
    int tot_len = len;
    int pos = 0;
    if (start_bit)
        tot_len += sizeof(start_sequence) + nal_header_len;
    if ((ret = av_new_packet(pkt, tot_len)) < 0)
        return ret;
    if (start_bit)
    {
        memcpy(pkt->data + pos, start_sequence, sizeof(start_sequence));
        pos += sizeof(start_sequence);
        memcpy(pkt->data + pos, nal_header, nal_header_len);
        pos += nal_header_len;
    }
    memcpy(pkt->data + pos, buf, len);
    return 0;
}

static int hevc_handle_packet(AVFormatContext * ctx, PayloadContext * rtp_hevc_ctx,
                              AVStream * st, AVPacket * pkt, uint32_t * timestamp,
                              const uint8_t * buf, int len, uint16_t seq,
                              int flags)
{
    const uint8_t * rtp_pl = buf;
    int tid, lid, nal_type;
    int first_fragment, last_fragment, fu_type;
    uint8_t new_nal_header[2];
    int res = 0;

    /* sanity check for size of input packet: 1 byte payload at least */
    if (len < RTP_HEVC_PAYLOAD_HEADER_SIZE + 1)
    {
        av_log(ctx, AV_LOG_ERROR, "Too short RTP/HEVC packet, got %d bytes\n", len);
        return AVERROR_INVALIDDATA;
    }

    /*
     * decode the HEVC payload header according to section 4 of draft version 6:
     *
     *    0                   1
     *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
     *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     *   |F|   Type    |  LayerId  | TID |
     *   +-------------+-----------------+
     *
     *      Forbidden zero (F): 1 bit
     *      NAL unit type (Type): 6 bits
     *      NUH layer ID (LayerId): 6 bits
     *      NUH temporal ID plus 1 (TID): 3 bits
     */
    nal_type = (buf[0] >> 1) & 0x3f;
    lid  = ((buf[0] << 5) & 0x20) | ((buf[1] >> 3) & 0x1f);
    tid  =   buf[1] & 0x07;

    /* av_log(ctx, AV_LOG_WARNING, "HEVC NAL type (%d) len (%d)\n", nal_type,len); */

    /* sanity check for correct layer ID */
    if (lid)
    {
        /* future scalable or 3D video coding extensions */
        /* avpriv_report_missing_feature(ctx, "Multi-layer HEVC coding\n"); */
        return AVERROR_PATCHWELCOME;
    }

    /* sanity check for correct temporal ID */
    if (!tid)
    {
        av_log(ctx, AV_LOG_ERROR, "Illegal temporal ID in RTP/HEVC packet\n");
        return AVERROR_INVALIDDATA;
    }

    /* sanity check for correct NAL unit type */
    if (nal_type > 50)
    {
        av_log(ctx, AV_LOG_ERROR, "Unsupported (HEVC) NAL type (%d)\n", nal_type);
        return AVERROR_INVALIDDATA;
    }

    switch (nal_type)
    {
            /* video parameter set (VPS) */
        case 32:
            /* sequence parameter set (SPS) */
        case 33:
            /* picture parameter set (PPS) */
        case 34:
            /*  supplemental enhancement information (SEI) */
        case 39:
            /* single NAL unit packet */
        default:
            /* sanity check for size of input packet: 1 byte payload at least */
            if (len < 1)
            {
                av_log(ctx, AV_LOG_ERROR,
                       "Too short RTP/HEVC packet, got %d bytes of NAL unit type %d\n",
                       len, nal_type);
                return AVERROR_INVALIDDATA;
            }

            /* create A/V packet */
            if ((res = av_new_packet(pkt, sizeof(start_sequence) + len)) < 0)
                return res;
            /* A/V packet: copy start sequence */
            memcpy(pkt->data, start_sequence, sizeof(start_sequence));
            /* A/V packet: copy NAL unit data */
            memcpy(pkt->data + sizeof(start_sequence), buf, len);

            rtp_h264_set_start_end_bit(ctx, 1, 0);
            break;
            /* aggregated packet (AP) - with two or more NAL units */
        case 48:
            /* pass the HEVC payload header */
            buf += RTP_HEVC_PAYLOAD_HEADER_SIZE;
            len -= RTP_HEVC_PAYLOAD_HEADER_SIZE;

            /* pass the HEVC DONL field */
            if (rtp_hevc_ctx->using_donl_field)
            {
                buf += RTP_HEVC_DONL_FIELD_SIZE;
                len -= RTP_HEVC_DONL_FIELD_SIZE;
            }

#if 1
            res = ff_h264_handle_aggregated_packet(ctx, rtp_hevc_ctx, pkt, buf, len,
                                                   rtp_hevc_ctx->using_donl_field ?
                                                   RTP_HEVC_DOND_FIELD_SIZE : 0,
                                                   NULL, 0);
#endif
            if (res < 0)
                return res;
            break;
            /* fragmentation unit (FU) */
        case 49:
            /* pass the HEVC payload header */
            buf += RTP_HEVC_PAYLOAD_HEADER_SIZE;
            len -= RTP_HEVC_PAYLOAD_HEADER_SIZE;

            /*
             *    decode the FU header
             *
             *     0 1 2 3 4 5 6 7
             *    +-+-+-+-+-+-+-+-+
             *    |S|E|  FuType   |
             *    +---------------+
             *
             *       Start fragment (S): 1 bit
             *       End fragment (E): 1 bit
             *       FuType: 6 bits
             */
            first_fragment = buf[0] & 0x80;
            last_fragment  = buf[0] & 0x40;
            fu_type        = buf[0] & 0x3f;

            /* pass the HEVC FU header */
            buf += RTP_HEVC_FU_HEADER_SIZE;
            len -= RTP_HEVC_FU_HEADER_SIZE;

            /* pass the HEVC DONL field */
            if (rtp_hevc_ctx->using_donl_field)
            {
                buf += RTP_HEVC_DONL_FIELD_SIZE;
                len -= RTP_HEVC_DONL_FIELD_SIZE;
            }

            av_dlog(ctx, " FU type %d with %d bytes\n", fu_type, len);

            /* sanity check for size of input packet: 1 byte payload at least */
            if (len <= 0)
            {
                if (len < 0)
                {
                    av_log(ctx, AV_LOG_ERROR,
                           "Too short RTP/HEVC packet, got %d bytes of NAL unit type %d\n",
                           len, nal_type);
                    return AVERROR_INVALIDDATA;
                }
                else
                {
                    return AVERROR(EAGAIN);
                }
            }

            if (first_fragment && last_fragment)
            {
                av_log(ctx, AV_LOG_ERROR, "Illegal combination of S and E bit in RTP/HEVC packet\n");
                return AVERROR_INVALIDDATA;
            }

            new_nal_header[0] = (rtp_pl[0] & 0x81) | (fu_type << 1);
            new_nal_header[1] = rtp_pl[1];

#if 1
            res = ff_h264_handle_frag_packet(pkt, buf, len, first_fragment,
                                             new_nal_header, sizeof(new_nal_header));
#endif
            rtp_h264_set_start_end_bit(ctx, first_fragment != 0, last_fragment != 0);

            break;
            /* PACI packet */
        case 50:
            /* Temporal scalability control information (TSCI) */
            avpriv_report_missing_feature(ctx, "PACI packets for RTP/HEVC\n");
            res = AVERROR_PATCHWELCOME;
            break;
    }

    pkt->stream_index = st->index;

    return res;
}

static int hevc_init(AVFormatContext * s, int st_index, PayloadContext * data)
{
    if (st_index < 0)
        return 0;
    s->streams[st_index]->need_parsing = AVSTREAM_PARSE_FULL;
    return 0;
}

static PayloadContext * hevc_new_context(void)
{
    return av_mallocz(sizeof(PayloadContext) + FF_INPUT_BUFFER_PADDING_SIZE);
}

static void hevc_free_context(PayloadContext * data)
{
    av_free(data);
}

int rtp_hevc_set_start_end_bit(AVFormatContext *ctx, int start_bit, int end_bit)
{
    ctx->start_bit  = start_bit;
    ctx->end_bit    = end_bit;
    return 0;
}

int rtp_hevc_set_marker(AVFormatContext *ctx, int marker)
{
    ctx->marker = marker;
    return 0;
}

void print_fast_hevc_parse_flags(AVFormatContext * ctx)
{
    av_log(ctx, AV_LOG_DEBUG, "<s=%d e=%d m=%d>", ctx->start_bit, ctx->end_bit, ctx->marker);
}

int rtp_hevc_get_nalu_type(AVFormatContext *ctx, const char * buf, int len)
{
    int nal_unit_type = -1, temporal_id = -1;
    /*00 00 00 01 ??*/
    if (len < 5)
    {
        return -1;
    }
    if (buf[0] != 0 || buf[1] != 0 || buf[2] != 0 || buf[3] != 1)
    {
        return -1;
    }

    nal_unit_type = (buf[4] >> 1) & 0x3f;
    temporal_id   = (buf[5] & 0x07) - 1;
    
    return nal_unit_type;
}


RTPDynamicProtocolHandler ff_hevc_dynamic_handler =
{
    .enc_name         = "H265",
    .codec_type       = AVMEDIA_TYPE_VIDEO,
    .codec_id = AV_CODEC_ID_HEVC,
    .static_payload_id = 0x96,
    .init = hevc_init,
    .parse_sdp_a_line = hevc_parse_sdp_line,
    .alloc = hevc_new_context,
    .free = hevc_free_context,
    .parse_packet     = hevc_handle_packet,
};
