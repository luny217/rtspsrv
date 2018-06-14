/*
 * RTP input/output format
 * Copyright (c) 2002 Fabrice Bellard
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

#include "libavutil/avutil_string.h"
#include "libavutil/avutil_opt.h"
#include "avformat.h"

#include "avformat_rtp.h"

/* from http://www.iana.org/assignments/rtp-parameters last updated 05 January 2005 */
/* payload types >= 96 are dynamic;
 * payload types between 72 and 76 are reserved for RTCP conflict avoidance;
 * all the other payload types not present in the table are unassigned or
 * reserved
 */
static const struct
{
    int pt;
    const char enc_name[32];
    enum AVMediaType codec_type;
    enum AVCodecID codec_id;
    int clock_rate;
    int audio_channels;
} rtp_payload_types[] =
{
    {0, "PCMU",        AVMEDIA_TYPE_AUDIO,   AV_CODEC_ID_PCM_MULAW, 8000, 1},
    {8, "PCMA",        AVMEDIA_TYPE_AUDIO,   AV_CODEC_ID_PCM_ALAW, 8000, 1},
    {98, "metadata",   AVMEDIA_TYPE_DATA,   AV_CODEC_ID_TEXT_XML, 90000, 0},
    {107, "vnd.onvif.metadata",   AVMEDIA_TYPE_DATA,   AV_CODEC_ID_APP_XML, 90000, 0},
    { -1, "",           AVMEDIA_TYPE_UNKNOWN, AV_CODEC_ID_NONE, -1, -1}
};

int ff_rtp_get_codec_info(AVCodecContext * codec, int payload_type)
{
    int i = 0;

    for (i = 0; rtp_payload_types[i].pt >= 0; i++)
        if (rtp_payload_types[i].pt == payload_type)
        {
            if (rtp_payload_types[i].codec_id != AV_CODEC_ID_NONE)
            {
                codec->codec_type = rtp_payload_types[i].codec_type;
                codec->codec_id = rtp_payload_types[i].codec_id;
                if (rtp_payload_types[i].audio_channels > 0)
                    codec->channels = rtp_payload_types[i].audio_channels;
                if (rtp_payload_types[i].clock_rate > 0)
                    codec->sample_rate = rtp_payload_types[i].clock_rate;
                return 0;
            }
        }
    return -1;
}

#if 0
int ff_rtp_get_payload_type(AVFormatContext * fmt,
                            AVCodecContext * codec, int idx)
{
    int i;
    AVOutputFormat * ofmt = fmt ? fmt->oformat : NULL;

    /* Was the payload type already specified for the RTP muxer? */
    if (ofmt && ofmt->priv_class && fmt->priv_data)
    {
        int64_t payload_type;
        if (av_opt_get_int(fmt->priv_data, "payload_type", 0, &payload_type) >= 0 &&
                payload_type >= 0)
            return (int)payload_type;
    }

    /* static payload type */
    for (i = 0; rtp_payload_types[i].pt >= 0; ++i)
        if (rtp_payload_types[i].codec_id == codec->codec_id)
        {            
            if (codec->codec_type == AVMEDIA_TYPE_AUDIO &&
                    ((rtp_payload_types[i].clock_rate > 0 &&
                      codec->sample_rate != rtp_payload_types[i].clock_rate) ||
                     (rtp_payload_types[i].audio_channels > 0 &&
                      codec->channels != rtp_payload_types[i].audio_channels)))
                continue;
            return rtp_payload_types[i].pt;
        }

    if (idx < 0)
        idx = codec->codec_type == AVMEDIA_TYPE_AUDIO;

    /* dynamic payload type */
    return RTP_PT_PRIVATE + idx;
}
#endif

const char * ff_rtp_enc_name(int payload_type)
{
    int i;

    for (i = 0; rtp_payload_types[i].pt >= 0; i++)
        if (rtp_payload_types[i].pt == payload_type)
            return rtp_payload_types[i].enc_name;

    return "";
}

enum AVCodecID ff_rtp_codec_id(const char * buf, enum AVMediaType codec_type)
{
    int i;

    for (i = 0; rtp_payload_types[i].pt >= 0; i++)
        if (!av_strcasecmp(buf, rtp_payload_types[i].enc_name) && (codec_type == rtp_payload_types[i].codec_type))
            return rtp_payload_types[i].codec_id;

    return AV_CODEC_ID_NONE;
}
