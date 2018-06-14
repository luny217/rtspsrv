/*
 * RTP H264 Protocol (RFC3984)
 * Copyright (c) 2006 Ryan Martell
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
 * @brief H.264 / RTP Code (RFC3984)
 * @author Ryan Martell <rdm4@martellventures.com>
 *
 * @note Notes:
 * Notes:
 * This currently supports packetization mode:
 * Single Nal Unit Mode (0), or
 * Non-Interleaved Mode (1).  It currently does not support
 * Interleaved Mode (2). (This requires implementing STAP-B, MTAP16, MTAP24,
 *                        FU-B packet types)
 */

#include "libavutil/avutil_attributes.h"
#include "libavutil/avutil_base64.h"
#include "libavutil/avutil_string.h"
#include "libavcodec/avcodec_get_bits.h"
#include "avformat.h"

#include "avformat_network.h"
#include <assert.h>

#include "avformat_rtpdec.h"
#include "avformat_rtpdec_formats.h"
#include "avformat_rtp_h264_decoder.h"

/* #define DEBUG_PACKET_FLOW */

struct PayloadContext
{
    /*  sdp setup parameters */
    uint8_t profile_idc;
    uint8_t profile_iop;
    uint8_t level_idc;
    int packetization_mode;
#ifdef DEBUG
    int packet_types_received[32];
#endif
};

#ifdef DEBUG
#define COUNT_NAL_TYPE(data, nal) data->packet_types_received[(nal) & 0x1f]++
#else
#define COUNT_NAL_TYPE(data, nal) do { } while (0)
#endif

static const uint8_t start_sequence[] = { 0, 0, 0, 1 };

static int sdp_parse_fmtp_config_h264(AVStream * stream,
                                      PayloadContext * h264_data,
                                      const char * attr, const char * value)
{
    AVCodecContext * codec = stream->codec;
    assert(codec->codec_id == AV_CODEC_ID_H264);
    assert(h264_data != NULL);

    if (!strcmp(attr, "packetization-mode"))
    {
        av_log(codec, AV_LOG_DEBUG, "RTP Packetization Mode: %d\n", atoi(value));
        h264_data->packetization_mode = atoi(value);
        /*
         * Packetization Mode:
         * 0 or not present: Single NAL mode (Only nals from 1-23 are allowed)
         * 1: Non-interleaved Mode: 1-23, 24 (STAP-A), 28 (FU-A) are allowed.
         * 2: Interleaved Mode: 25 (STAP-B), 26 (MTAP16), 27 (MTAP24), 28 (FU-A),
         *                      and 29 (FU-B) are allowed.
         */
        if (h264_data->packetization_mode > 1)
            av_log(codec, AV_LOG_ERROR,
                   "Interleaved RTP mode is not supported yet.\n");
    }
    else if (!strcmp(attr, "profile-level-id"))
    {
        if (strlen(value) == 6)
        {
            char buffer[3];
            /*  6 characters=3 bytes, in hex. */
            uint8_t profile_idc;
            uint8_t profile_iop;
            uint8_t level_idc;

            buffer[0]   = value[0];
            buffer[1]   = value[1];
            buffer[2]   = '\0';
            profile_idc = strtol(buffer, NULL, 16);
            buffer[0]   = value[2];
            buffer[1]   = value[3];
            profile_iop = strtol(buffer, NULL, 16);
            buffer[0]   = value[4];
            buffer[1]   = value[5];
            level_idc   = strtol(buffer, NULL, 16);

            av_log(codec, AV_LOG_DEBUG,
                   "RTP Profile IDC: %x Profile IOP: %x Level: %x\n",
                   profile_idc, profile_iop, level_idc);
            h264_data->profile_idc = profile_idc;
            h264_data->profile_iop = profile_iop;
            h264_data->level_idc   = level_idc;
        }
    }
    else if (!strcmp(attr, "sprop-parameter-sets"))
    {
        codec->extradata_size = 0;
        av_freep(&codec->extradata);

        while (*value)
        {
            char base64packet[1024];
            uint8_t decoded_packet[1024];
            int packet_size;
            char * dst = base64packet;

            while (*value && *value != ','
                    && (unsigned int)(dst - base64packet) < sizeof(base64packet) - 1)
            {
                *dst++ = *value++;
            }
            *dst++ = '\0';

            if (*value == ',')
                value++;

            packet_size = av_base64_decode(decoded_packet, base64packet,
                                           sizeof(decoded_packet));
            if (packet_size > 0)
            {
                uint8_t nal_type = decoded_packet[0] & 0x1f;
                uint8_t * dest = av_malloc(packet_size + sizeof(start_sequence) +
                                           codec->extradata_size +
                                           FF_INPUT_BUFFER_PADDING_SIZE);
                if (!dest)
                {
                    av_log(codec, AV_LOG_ERROR,
                           "Unable to allocate memory for extradata!\n");
                    return AVERROR(ENOMEM);
                }                              
                
                /*if (codec->extradata_size)
                {
                    memcpy(dest, codec->extradata, codec->extradata_size);
                    av_free(codec->extradata);
                }*/

                /*memcpy(dest + codec->extradata_size, start_sequence,
                       sizeof(start_sequence));*/
                memcpy(dest, decoded_packet, packet_size);
                memset(dest + packet_size, 0, FF_INPUT_BUFFER_PADDING_SIZE);

                if(nal_type == NAL_SPS)
                {
                    codec->sps_data = dest;
                    codec->sps_data_size = packet_size;
                    /*av_log(codec, AV_LOG_WARNING, "sps set to %p (size: %d)!\n",
                            codec->sps_data, codec->sps_data_size);*/
                    /* av_hex_dump(NULL, codec->sps_data, codec->sps_data_size); */
                }
                else if(nal_type == NAL_PPS)
                {
                    codec->pps_data = dest;
                    codec->pps_data_size = packet_size;
                    /*av_log(codec, AV_LOG_WARNING, "pps set to %p (size: %d)!\n",
                            codec->pps_data, codec->pps_data_size);*/
                    /* av_hex_dump(NULL, codec->pps_data, codec->pps_data_size); */
                }
            }
        }
        
    }
    return 0;
}

static PayloadContext * h264_new_context(void)
{
    return av_mallocz(sizeof(PayloadContext) + FF_INPUT_BUFFER_PADDING_SIZE);
}

static void h264_free_context(PayloadContext * data)
{
#ifdef DEBUG
    int ii;

    for (ii = 0; ii < 32; ii++)
    {
        if (data->packet_types_received[ii])
            av_log(NULL, AV_LOG_DEBUG, "Received %d packets of type %d\n",
                   data->packet_types_received[ii], ii);
    }
#endif

    av_free(data);
}

static av_cold int h264_init(AVFormatContext * s, int st_index,
                             PayloadContext * data)
{
    if (st_index < 0)
        return 0;
    s->streams[st_index]->need_parsing = AVSTREAM_PARSE_FULL;
    return 0;
}

static int parse_h264_sdp_line(AVFormatContext * s, int st_index,
                               PayloadContext * h264_data, const char * line)
{
    AVStream * stream;
    AVCodecContext * codec;
    const char * p = line;

    if (st_index < 0)
        return 0;

    stream = s->streams[st_index];
    codec  = stream->codec;

    if (av_strstart(p, "framesize:", &p))
    {
        char buf1[50];
        char * dst = buf1;

        /*  remove the protocol identifier */
        while (*p && *p == ' ')
            p++;                     /*  strip spaces. */
        while (*p && *p != ' ')
            p++;                     /*  eat protocol identifier */
        while (*p && *p == ' ')
            p++;                     /*  strip trailing spaces. */
        while (*p && *p != '-' && (unsigned int)(dst - buf1) < sizeof(buf1) - 1)
            *dst++ = *p++;
        *dst = '\0';

        /*  a='framesize:96 320-240' */
        /*  set our parameters */
        codec->width   = atoi(buf1);
        codec->height  = atoi(p + 1); /*  skip the - */
    }
    else if (av_strstart(p, "fmtp:", &p))
    {
        return ff_parse_fmtp(stream, h264_data, p, sdp_parse_fmtp_config_h264);
    }
    else if (av_strstart(p, "cliprect:", &p))
    {
        /*  could use this if we wanted. */
    }

    return 0;
}

RTPDynamicProtocolHandler ff_h264_dynamic_handler =
{
    "H264",
    AVMEDIA_TYPE_VIDEO,
    AV_CODEC_ID_H264,
    0x96,
    h264_init,
    parse_h264_sdp_line,
    h264_new_context,
    h264_free_context,    
};
