/*
 * RTP input format
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

#include <time.h>
#include "libavutil/avutil_mathematics.h"
#include "libavutil/avutil_string.h"
#include "libavutil/avutil_time.h"
#include "libavcodec/avcodec_get_bits.h"
#include "avformat.h"
#include "avformat_network.h"
#include "avformat_url.h"
#include "avformat_rtpdec.h"
#include "avformat_rtpdec_formats.h"
#include "avformat_rtp_h264_decoder.h"
#include "avformat_rtsp.h"

#define MIN_FEEDBACK_INTERVAL 200000 /* 200 ms in us */


static RTPDynamicProtocolHandler * rtp_first_dynamic_payload_handler = NULL;

void ff_register_dynamic_payload_handler(RTPDynamicProtocolHandler * handler)
{
    handler->next = rtp_first_dynamic_payload_handler;
    rtp_first_dynamic_payload_handler = handler;
}

void ff_register_rtp_dynamic_payload_handlers(void)
{
    ff_register_dynamic_payload_handler(&ff_h264_dynamic_handler);
    ff_register_dynamic_payload_handler(&ff_hevc_dynamic_handler);
}

RTPDynamicProtocolHandler * ff_rtp_handler_find_by_name(const char * name,
        enum AVMediaType codec_type)
{
    RTPDynamicProtocolHandler * handler;
    for (handler = rtp_first_dynamic_payload_handler;
            handler; handler = handler->next)
        if (!av_strcasecmp(name, handler->enc_name) &&
                codec_type == handler->codec_type)
            return handler;
    return NULL;
}

RTPDynamicProtocolHandler * ff_rtp_handler_find_by_id(int id,
        enum AVMediaType codec_type)
{
    RTPDynamicProtocolHandler * handler;
    for (handler = rtp_first_dynamic_payload_handler;
            handler; handler = handler->next)
        if (handler->static_payload_id && handler->static_payload_id == id &&
                codec_type == handler->codec_type)
            return handler;
    return NULL;
}

static int rtcp_parse_packet(RTPDemuxContext * s, const unsigned char * buf, int len)
{
    int payload_len;
    while (len >= 4)
    {
        payload_len = FFMIN(len, (AV_RB16(buf + 2) + 1) * 4);

        switch (buf[1])
        {
            case RTCP_SR:
                if (payload_len < 20)
                {
                    av_log(NULL, AV_LOG_ERROR,
                           "Invalid length for RTCP SR packet\n");
                    return AVERROR_INVALIDDATA;
                }

                s->last_rtcp_reception_time = av_gettime();
                s->last_rtcp_ntp_time  = AV_RB64(buf + 8);
                s->last_rtcp_timestamp = AV_RB32(buf + 16);
                if ((int64_t)s->first_rtcp_ntp_time == AV_NOPTS_VALUE)
                {
                    s->first_rtcp_ntp_time = s->last_rtcp_ntp_time;
                    if (!s->base_timestamp)
                        s->base_timestamp = s->last_rtcp_timestamp;
                    s->rtcp_ts_offset = s->last_rtcp_timestamp - s->base_timestamp;
                }
                break;
            case RTCP_BYE:
                return -RTCP_BYE;
        }

        buf += payload_len;
        len -= payload_len;
    }
    return -1;
}

#define RTP_SEQ_MOD (1 << 16)

static void rtp_init_statistics(RTPStatistics * s, uint16_t base_sequence)
{
    memset(s, 0, sizeof(RTPStatistics));
    s->max_seq   = base_sequence;
    s->probation = 1;
}

/*
 * Called whenever there is a large jump in sequence numbers,
 * or when they get out of probation...
 */
static void rtp_init_sequence(RTPStatistics * s, uint16_t seq)
{
    s->max_seq        = seq;
    s->cycles         = 0;
    s->base_seq       = seq - 1;
    s->bad_seq        = RTP_SEQ_MOD + 1;
    s->received       = 0;
    s->expected_prior = 0;
    s->received_prior = 0;
    s->jitter         = 0;
    s->transit        = 0;
}

/* Returns 1 if we should handle this packet. */
int rtp_valid_packet_in_sequence(RTPStatistics * s, uint16_t seq)
{
    uint16_t udelta = seq - s->max_seq;
    const int MAX_DROPOUT    = 3000;
    const int MAX_MISORDER   = 100;
    const int MIN_SEQUENTIAL = 2;

    /* source not valid until MIN_SEQUENTIAL packets with sequence
     * seq. numbers have been received */
    if (s->probation)
    {
        if (seq == s->max_seq + 1)
        {
            s->probation--;
            s->max_seq = seq;
            if (s->probation == 0)
            {
                rtp_init_sequence(s, seq);
                s->received++;
                return 1;
            }
        }
        else
        {
            s->probation = MIN_SEQUENTIAL - 1;
            s->max_seq   = seq;
        }
    }
    else if (udelta < MAX_DROPOUT)
    {
        /*  in order, with permissible gap */
        if (seq < s->max_seq)
        {
            /*  sequence number wrapped; count another 64k cycles */
            s->cycles += RTP_SEQ_MOD;
        }
        s->max_seq = seq;
    }
    else if (udelta <= RTP_SEQ_MOD - MAX_MISORDER)
    {
        /*  sequence made a large jump... */
        if (seq == s->bad_seq)
        {
            /* two sequential packets -- assume that the other side
             * restarted without telling us; just resync. */
            rtp_init_sequence(s, seq);
        }
        else
        {
            s->bad_seq = (seq + 1) & (RTP_SEQ_MOD - 1);
            return 0;
        }
    }
    else
    {
        /*  duplicate or reordered packet... */
    }
    s->received++;
    return 1;
}

static void rtcp_update_jitter(RTPStatistics * s, uint32_t sent_timestamp,
                               uint32_t arrival_timestamp)
{
    /*  Most of this is pretty straight from RFC 3550 appendix A.8 */
    uint32_t transit = arrival_timestamp - sent_timestamp;
    uint32_t prev_transit = s->transit;
    int32_t d = transit - prev_transit;
    /*  Doing the FFABS() call directly on the "transit - prev_transit" */
    /*  expression doesn't work, since it's an unsigned expression. Doing the */
    /*  transit calculation in unsigned is desired though, since it most */
    /*  probably will need to wrap around. */
    d = FFABS(d);
    s->transit = transit;
    if (!prev_transit)
        return;
    s->jitter += d - (int32_t)((s->jitter + 8) >> 4);
}

int ff_rtp_check_and_send_back_rr(RTPDemuxContext * s, URLContext * fd, int id, int count)
{
    AVIOContext * pb;
    uint8_t * buf;
    int len;
    int rtcp_bytes;
    RTPStatistics * stats = &s->statistics;
    uint32_t lost;
    uint32_t extended_max;
    uint32_t expected_interval;
    uint32_t received_interval;
    int32_t  lost_interval;
    uint32_t expected;
    uint32_t fraction;

    /* TODO: I think this is way too often; RFC 1889 has algorithm for this */
    /* XXX: MPEG pts hardcoded. RTCP send every 0.5 seconds */
    s->octet_count += count;
#if 0
    rtcp_bytes = ((s->octet_count - s->last_octet_count) * RTCP_TX_RATIO_NUM) /
                 RTCP_TX_RATIO_DEN;
    rtcp_bytes /= 50; /*  mmu_man: that's enough for me... VLC sends much less btw !? */
    if (rtcp_bytes < 28)
        return -1;
#endif
    s->last_octet_count = s->octet_count;

    /* av_log(NULL, AV_LOG_ERROR,  "rtcp 3\n"); */

    if (avio_open_dyn_buf(&pb) < 0)
        return -1;

    /* av_log(NULL, AV_LOG_ERROR,  "rtcp 4\n"); */

    /*  Receiver Report */
    avio_w8(pb, 0x24);
    avio_w8(pb, id);
    avio_wb16(pb, 0x34);

    avio_w8(pb, (RTP_VERSION << 6) + 1); /* 1 report block */
    avio_w8(pb, RTCP_RR);
    avio_wb16(pb, 7); /* length in words - 1 */
    /*  our own SSRC: we use the server's SSRC + 1 to avoid conflicts */
    avio_wb32(pb, s->ssrc + 1);
    avio_wb32(pb, s->ssrc); /*  server SSRC */
    /*  some placeholders we should really fill... */
    /*  RFC 1889/p64 */
    extended_max          = stats->cycles + stats->max_seq;
    expected              = extended_max - stats->base_seq;
    lost                  = expected - stats->received;
    lost                  = FFMIN(lost, 0xffffff); /*  clamp it since it's only 24 bits... */
    expected_interval     = expected - stats->expected_prior;
    stats->expected_prior = expected;
    received_interval     = stats->received - stats->received_prior;
    stats->received_prior = stats->received;
    lost_interval         = expected_interval - received_interval;
    if (expected_interval == 0 || lost_interval <= 0)
        fraction = 0;
    else
        fraction = (lost_interval << 8) / expected_interval;

    fraction = (fraction << 24) | lost;

    avio_wb32(pb, fraction); /* 8 bits of fraction, 24 bits of total packets lost */
    avio_wb32(pb, extended_max); /* max sequence received */
    avio_wb32(pb, stats->jitter >> 4); /* jitter */

    if ((int64_t)s->last_rtcp_ntp_time == AV_NOPTS_VALUE)
    {
        avio_wb32(pb, 0); /* last SR timestamp */
        avio_wb32(pb, 0); /* delay since last SR */
    }
    else
    {
        uint32_t middle_32_bits   = s->last_rtcp_ntp_time >> 16; /*  this is valid, right? do we need to handle 64 bit values special? */
        uint32_t delay_since_last = av_rescale(av_gettime() - s->last_rtcp_reception_time,
                                               65536, AV_TIME_BASE);

        avio_wb32(pb, middle_32_bits); /* last SR timestamp */
        avio_wb32(pb, delay_since_last); /* delay since last SR */
    }

    /*  CNAME */
    avio_w8(pb, (RTP_VERSION << 6) + 1); /* 1 report block */
    avio_w8(pb, RTCP_SDES);
    len = strlen(s->hostname);
    avio_wb16(pb, (7 + len + 3) / 4); /* length in words - 1 */
    avio_wb32(pb, s->ssrc + 1);
    avio_w8(pb, 0x01);
    avio_w8(pb, len);
    avio_write(pb, (const unsigned char *)s->hostname, len);
    avio_w8(pb, 0); /* END */
    /*  padding */
    for (len = (7 + len) % 4; len % 4; len++)
        avio_w8(pb, 0);

    avio_flush(pb);
    if (!fd)
        return 0;
    len = avio_close_dyn_buf(pb, &buf);
    if ((len > 0) && buf)
    {
        int av_unused result;
        /* av_log(s->ic,AV_LOG_WARNING, "sending %d bytes of RR\n", len); */
        result = ffurl_write(fd, buf, len);
        av_log(s->ic, AV_LOG_WARNING, "result from ffurl_write: %d\n", result);
        av_free(buf);
    }
    return 0;
}

#define RTCP_RR_LEN 52

int ff_rtp_check_and_send_back_rr_1(RTPDemuxContext * s, URLContext * fd, int id, int count)
{
    AVIOContext * pb;
    uint8_t * buf;
    int len, result;
    int rtcp_bytes;
    RTPStatistics * stats = &s->statistics;
    uint32_t lost;
    uint32_t extended_max;
    uint32_t expected_interval;
    uint32_t received_interval;
    int32_t  lost_interval;
    uint32_t expected;
    uint32_t fraction;
    char rtcp_buf[128] = {0};
    unsigned int val;

    /* TODO: I think this is way too often; RFC 1889 has algorithm for this */
    /* XXX: MPEG pts hardcoded. RTCP send every 0.5 seconds */
    s->octet_count += count;

    s->last_octet_count = s->octet_count;

    /* av_log(NULL, AV_LOG_ERROR,  "rtcp 3\n"); */

    /*if (avio_open_dyn_buf(&pb) < 0)
        return -1;*/

    /* av_log(NULL, AV_LOG_ERROR,  "rtcp 4\n"); */

    /*  Receiver Report */

    rtcp_buf[0] = 0x24;
    rtcp_buf[1] = (uint8_t)id;
    rtcp_buf[2] = RTCP_RR_LEN >> 8;
    rtcp_buf[3] = (uint8_t)RTCP_RR_LEN;

    rtcp_buf[4] = (uint8_t)((RTP_VERSION << 6) + 1); /* avio_w8(pb, (RTP_VERSION << 6) + 1); *//* 1 report block */
    rtcp_buf[5] = (uint8_t)(RTCP_RR); /* avio_w8(pb, RTCP_RR); */
    rtcp_buf[6] = (uint8_t)(0x07 >> 8); /* avio_wb16(pb, 7); length in words - 1 */
    rtcp_buf[7] = (uint8_t)(0x07);

    /*  our own SSRC: we use the server's SSRC + 1 to avoid conflicts */
    val = s->ssrc + 1;
    rtcp_buf[8] = val >> 24;
    rtcp_buf[9] = (uint8_t)val >> 16;
    rtcp_buf[10] = (uint8_t)val >> 8;
    rtcp_buf[11] = (uint8_t)val; /* avio_wb32(pb, s->ssrc + 1); */

    val = s->ssrc;
    rtcp_buf[12] = val >> 24;
    rtcp_buf[13] = (uint8_t)val >> 16;
    rtcp_buf[14] = (uint8_t)val >> 8;
    rtcp_buf[15] = (uint8_t)val; /* avio_wb32(pb, s->ssrc); // server SSRC */

    /*  some placeholders we should really fill... */
    /*  RFC 1889/p64 */
    extended_max          = stats->cycles + stats->max_seq;
    expected              = extended_max - stats->base_seq;
    lost                  = expected - stats->received;
    lost                  = FFMIN(lost, 0xffffff); /*  clamp it since it's only 24 bits... */
    expected_interval     = expected - stats->expected_prior;
    stats->expected_prior = expected;
    received_interval     = stats->received - stats->received_prior;
    stats->received_prior = stats->received;
    lost_interval         = expected_interval - received_interval;
    if (expected_interval == 0 || lost_interval <= 0)
        fraction = 0;
    else
        fraction = (lost_interval << 8) / expected_interval;

    fraction = (fraction << 24) | lost;

    val = fraction;
    rtcp_buf[16] = val >> 24;
    rtcp_buf[17] = (uint8_t)val >> 16;
    rtcp_buf[18] = (uint8_t)val >> 8;
    rtcp_buf[19] = (uint8_t)val;  /* avio_wb32(pb, fraction);  8 bits of fraction, 24 bits of total packets lost */

    val = extended_max;
    rtcp_buf[20] = val >> 24;
    rtcp_buf[21] = (uint8_t)val >> 16;
    rtcp_buf[22] = (uint8_t)val >> 8;
    rtcp_buf[23] = (uint8_t)val;    /* avio_wb32(pb, extended_max);  max sequence received */

    val = stats->jitter >> 4;
    rtcp_buf[24] = val >> 24;
    rtcp_buf[25] = (uint8_t)val >> 16;
    rtcp_buf[26] = (uint8_t)val >> 8;
    rtcp_buf[27] = (uint8_t)val;   /* avio_wb32(pb, stats->jitter >> 4); jitter */

    if ((int64_t)s->last_rtcp_ntp_time == AV_NOPTS_VALUE)
    {

        val = 0;
        rtcp_buf[28] = val >> 24;
        rtcp_buf[29] = (uint8_t)val >> 16;
        rtcp_buf[30] = (uint8_t)val >> 8;
        rtcp_buf[31] = (uint8_t)val; /* avio_wb32(pb, 0);  last SR timestamp */

        rtcp_buf[32] = val >> 24;
        rtcp_buf[33] = (uint8_t)val >> 16;
        rtcp_buf[34] = (uint8_t)val >> 8;
        rtcp_buf[35] = (uint8_t)val; /* avio_wb32(pb, 0); delay since last SR */
    }
    else
    {
        uint32_t middle_32_bits   = s->last_rtcp_ntp_time >> 16; /*  this is valid, right? do we need to handle 64 bit values special? */
        uint32_t delay_since_last = av_rescale(av_gettime() - s->last_rtcp_reception_time,
                                               65536, AV_TIME_BASE);

        val = middle_32_bits;
        rtcp_buf[28] = val >> 24;
        rtcp_buf[29] = (uint8_t)val >> 16;
        rtcp_buf[30] = (uint8_t)val >> 8;
        rtcp_buf[31] = (uint8_t)val;    /* avio_wb32(pb, middle_32_bits);  last SR timestamp */

        val = delay_since_last;
        rtcp_buf[32] = val >> 24;
        rtcp_buf[33] = (uint8_t)val >> 16;
        rtcp_buf[34] = (uint8_t)val >> 8;
        rtcp_buf[35] = (uint8_t)val;    /* avio_wb32(pb, delay_since_last);  delay since last SR */
    }

    /*  CNAME */
    rtcp_buf[36] = (uint8_t)((RTP_VERSION << 6) + 1); /* avio_w8(pb, (RTP_VERSION << 6) + 1); = 1 report block */
    rtcp_buf[37] = (uint8_t)(RTCP_SDES); /* avio_w8(pb, RTCP_SDES); */

    rtcp_buf[38] = (uint8_t)(0x04 >> 8); /* avio_wb16(pb, 7);  length in words - 1 */
    rtcp_buf[39] = (uint8_t)(0x07); /* len = strlen(s->hostname);  avio_wb16(pb, (7 + len + 3) / 4); = length in words - 1 */

    val = s->ssrc + 1;
    rtcp_buf[40] = val >> 24;
    rtcp_buf[41] = (uint8_t)val >> 16;
    rtcp_buf[42] = (uint8_t)val >> 8;
    rtcp_buf[43] = (uint8_t)val;  /* avio_wb32(pb, s->ssrc + 1); */

    rtcp_buf[44] = (uint8_t)0x01; /* avio_w8(pb, 0x01); */
    rtcp_buf[45] = (uint8_t)0x08; /* avio_w8(pb, len); */
    rtcp_buf[46] = 'H';
    rtcp_buf[47] = 'B';
    rtcp_buf[48] = 'G';
    rtcp_buf[49] = 'K';
    rtcp_buf[50] = '-';
    rtcp_buf[51] = 'N';
    rtcp_buf[52] = 'V';
    rtcp_buf[53] = 'R'; /* avio_write(pb, s->hostname, len); */

    rtcp_buf[54] = (uint8_t)0x00; /* avio_w8(pb, 0);  END */
    rtcp_buf[55] = (uint8_t)0x00; /*  padding */

    if (!fd)
        return 0;
    
    /* av_log(s->ic,AV_LOG_WARNING, "sending %d bytes of RR\n", len); */
    result = ffurl_write(fd, (const unsigned char *)rtcp_buf, RTCP_RR_LEN + 4);
    /* av_log(s->ic, AV_LOG_WARNING, "result from ffurl_write: %d\n", result); */

    return 0;
}


int ff_rtp_check_and_send_back_rr_custom(RTPDemuxContext * s, URLContext * fd,
        AVIOContext * avio, int count)
{
    AVIOContext * pb;
    uint8_t * buf;
    int len;
    int rtcp_bytes;
    RTPStatistics * stats = &s->statistics;

    if ((!fd && !avio) || (count < 1))
    {
         av_log(s->ic, AV_LOG_WARNING, "[%s] Failed AT:%d\n", __FUNCTION__, __LINE__);
        return -1;
    }

    /* TODO: I think this is way too often; RFC 1889 has algorithm for this */
    /* XXX: MPEG pts hardcoded. RTCP send every 0.5 seconds */
    s->octet_count += count;
    rtcp_bytes = ((s->octet_count - s->last_octet_count) * RTCP_TX_RATIO_NUM) /
                 RTCP_TX_RATIO_DEN;
    rtcp_bytes /= 50; /*  mmu_man: that's enough for me... VLC sends much less btw !? */
    if (rtcp_bytes < 120)
    {
        return -1;
    }
    s->last_octet_count = s->octet_count;

    char tmp_buf[128];
    /*  memset(tmp_buf,0,sizeof(tmp_buf)); //eagle 1219 */

#if 0
    tmp_buf[0] = 0x24;
    tmp_buf[1] = 0x01;
    tmp_buf[2] = 0; /* length; */
    tmp_buf[3] = 0x52; /* length; */

    tmp_buf[4] = (RTP_VERSION << 6) + 1;
    tmp_buf[5] = RTCP_RR;

    unsigned int length = 0;
    tmp_buf[6] = (int)length >> 8;
    tmp_buf[7] = (uint8_t)length;

    unsigned int ssrc_plus1 = s->ssrc + 1;
    tmp_buf[8] = ssrc_plus1 >> 24;
    tmp_buf[9] = (uint8_t)(ssrc_plus1 >> 16);
    tmp_buf[10] = (uint8_t)(ssrc_plus1 >> 8);
    tmp_buf[11] = (uint8_t) ssrc_plus1;

    unsigned int ssrc = s->ssrc;
    tmp_buf[12] = ssrc >> 24;
    tmp_buf[13] = (uint8_t)(ssrc >> 16);
    tmp_buf[14] = (uint8_t)(ssrc >> 8);
    tmp_buf[15] = (uint8_t) ssrc;

/* fraction */
    tmp_buf[16] = (uint8_t) 0;
    tmp_buf[17] = (uint8_t) 0;
    tmp_buf[18] = (uint8_t) 0;
    tmp_buf[19] = (uint8_t) 0;

/* max sequence received */
    uint32_t extended_max;
    extended_max          = stats->cycles + stats->max_seq;
    tmp_buf[20] = extended_max >> 24;
    tmp_buf[21] = (uint8_t)(extended_max >> 16);
    tmp_buf[22] = (uint8_t)(extended_max >> 8);
    tmp_buf[23] = (uint8_t) extended_max;

/* jitter */
    tmp_buf[24] = (uint8_t) 0;
    tmp_buf[25] = (uint8_t) 0;
    tmp_buf[26] = (uint8_t) 0;
    tmp_buf[27] = (uint8_t) 0;

/* last SR timestamp */
    tmp_buf[28] = (uint8_t) 0;
    tmp_buf[29] = (uint8_t) 0;
    tmp_buf[30] = (uint8_t) 0;
    tmp_buf[31] = (uint8_t) 0;

/* delay since last SR */
    tmp_buf[32] = (uint8_t) 0;
    tmp_buf[33] = (uint8_t) 0;
    tmp_buf[34] = (uint8_t) 0;
    tmp_buf[35] = (uint8_t) 0;

    tmp_buf[36] = (RTP_VERSION << 6) + 1;
    tmp_buf[37] = RTCP_SDES;

    len = strlen(s->hostname);
    length = (7 + len + 3) / 4;
    tmp_buf[38] = (int)length >> 8;
    tmp_buf[39] = (uint8_t)length;

    tmp_buf[40] = ssrc_plus1 >> 24;
    tmp_buf[41] = (uint8_t)(ssrc_plus1 >> 16);
    tmp_buf[42] = (uint8_t)(ssrc_plus1 >> 8);
    tmp_buf[43] = (uint8_t) ssrc_plus1;

    tmp_buf[44] = 0x01;
    tmp_buf[45] = 4;
    tmp_buf[46] = 'h';
    tmp_buf[47] = 'b';
    tmp_buf[48] = 'g';
    tmp_buf[49] = 'k';
    tmp_buf[50] = 0;

    tmp_buf[3] = 51 - 3;
    int ret = fd->prot->url_write(fd, (uint8_t *)tmp_buf, 51);
#else
    tmp_buf[0] = 0x24;
    tmp_buf[1] = 0x01;
    tmp_buf[2] = 0; /* length; */
    tmp_buf[3] = 0x34; /* length; */

    tmp_buf[4] = (RTP_VERSION << 6) + 1;
    tmp_buf[5] = RTCP_RR;

    unsigned int length = 7;
    tmp_buf[6] = (int)length >> 8;
    tmp_buf[7] = (uint8_t)length;

    unsigned int ssrc_plus1 = s->ssrc + 1;
    tmp_buf[8] = ssrc_plus1 >> 24;
    tmp_buf[9] = (uint8_t)(ssrc_plus1 >> 16);
    tmp_buf[10] = (uint8_t)(ssrc_plus1 >> 8);
    tmp_buf[11] = (uint8_t) ssrc_plus1;

    unsigned int ssrc = s->ssrc;
    tmp_buf[12] = ssrc >> 24;
    tmp_buf[13] = (uint8_t)(ssrc >> 16);
    tmp_buf[14] = (uint8_t)(ssrc >> 8);
    tmp_buf[15] = (uint8_t) ssrc;

/* fraction */
    tmp_buf[16] = (uint8_t) 0;
    tmp_buf[17] = (uint8_t) 0xff;
    tmp_buf[18] = (uint8_t) 0xff;
    tmp_buf[19] = (uint8_t) 0xff;

/* max sequence received */
    uint32_t extended_max;
    extended_max          = stats->cycles + stats->max_seq;
    tmp_buf[20] = extended_max >> 24;
    tmp_buf[21] = (uint8_t)(extended_max >> 16);
    tmp_buf[22] = (uint8_t)(extended_max >> 8);
    tmp_buf[23] = (uint8_t) extended_max;

/* jitter */
    tmp_buf[24] = (uint8_t) 0;
    tmp_buf[25] = (uint8_t) 0;
    tmp_buf[26] = (uint8_t) 0;
    tmp_buf[27] = (uint8_t) 0x88;

/* last SR timestamp */
    tmp_buf[28] = (uint8_t) 0;
    tmp_buf[29] = (uint8_t) 0;
    tmp_buf[30] = (uint8_t) 0;
    tmp_buf[31] = (uint8_t) 0;

/* delay since last SR */
    tmp_buf[32] = (uint8_t) 0;
    tmp_buf[33] = (uint8_t) 0;
    tmp_buf[34] = (uint8_t) 0;
    tmp_buf[35] = (uint8_t) 0;

    tmp_buf[36] = (RTP_VERSION << 6) + 1;
    tmp_buf[37] = RTCP_SDES;

    length = 4;
    tmp_buf[38] = (int)length >> 8;
    tmp_buf[39] = (uint8_t)length;

    tmp_buf[40] = ssrc_plus1 >> 24;
    tmp_buf[41] = (uint8_t)(ssrc_plus1 >> 16);
    tmp_buf[42] = (uint8_t)(ssrc_plus1 >> 8);
    tmp_buf[43] = (uint8_t) ssrc_plus1;

    tmp_buf[44] = 0x01;
    tmp_buf[45] = 7;
    tmp_buf[46] = 'a';
    tmp_buf[47] = 'd';
    tmp_buf[48] = 'e';
    tmp_buf[49] = 'a';
    tmp_buf[50] = 'g';
    tmp_buf[51] = 'l';
    tmp_buf[52] = 'e';

    tmp_buf[53] = 0;
    tmp_buf[54] = 0;
    tmp_buf[55] = 0;

    int ret = fd->prot->url_write(fd, (uint8_t *)tmp_buf, 56);
#endif
    return 0;
}


void ff_rtp_send_punch_packets(URLContext * rtp_handle)
{
    AVIOContext * pb;
    uint8_t * buf;
    int len;

    /* Send a small RTP packet */
    if (avio_open_dyn_buf(&pb) < 0)
        return;

    avio_w8(pb, (RTP_VERSION << 6));
    avio_w8(pb, 0); /* Payload type */
    avio_wb16(pb, 0); /* Seq */
    avio_wb32(pb, 0); /* Timestamp */
    avio_wb32(pb, 0); /* SSRC */

    avio_flush(pb);
    len = avio_close_dyn_buf(pb, &buf);
    if ((len > 0) && buf)
        ffurl_write(rtp_handle, buf, len);
    av_free(buf);

    /* Send a minimal RTCP RR */
    if (avio_open_dyn_buf(&pb) < 0)
        return;

    avio_w8(pb, (RTP_VERSION << 6));
    avio_w8(pb, RTCP_RR); /* receiver report */
    avio_wb16(pb, 1); /* length in words - 1 */
    avio_wb32(pb, 0); /* our own SSRC */

    avio_flush(pb);
    len = avio_close_dyn_buf(pb, &buf);
    if ((len > 0) && buf)
        ffurl_write(rtp_handle, buf, len);
    av_free(buf);
}

static int find_missing_packets(RTPDemuxContext * s, uint16_t * first_missing,
                                uint16_t * missing_mask)
{
    int i;
    uint16_t next_seq = s->seq + 1;
    RTPPacket * pkt = s->queue;

    if (!pkt || pkt->seq == next_seq)
        return 0;

    *missing_mask = 0;
    for (i = 1; i <= 16; i++)
    {
        uint16_t missing_seq = next_seq + i;
        while (pkt)
        {
            int16_t diff = pkt->seq - missing_seq;
            if (diff >= 0)
                break;
            pkt = pkt->next;
        }
        if (!pkt)
            break;
        if (pkt->seq == missing_seq)
            continue;
        *missing_mask |= 1 << (i - 1);
    }

    *first_missing = next_seq;
    return 1;
}

int ff_rtp_send_rtcp_feedback(RTPDemuxContext * s, URLContext * fd,
                              AVIOContext * avio)
{
    int len, need_keyframe, missing_packets;
    AVIOContext * pb;
    uint8_t * buf;
    int64_t now;
    uint16_t first_missing = 0, missing_mask = 0;

    if (!fd && !avio)
        return -1;

    need_keyframe = s->handler && s->handler->need_keyframe &&
                    s->handler->need_keyframe(s->dynamic_protocol_context);
    missing_packets = find_missing_packets(s, &first_missing, &missing_mask);

    if (!need_keyframe && !missing_packets)
        return 0;

    /* Send new feedback if enough time has elapsed since the last
     * feedback packet. */

    now = av_gettime();
    if (s->last_feedback_time &&
            (now - s->last_feedback_time) < MIN_FEEDBACK_INTERVAL)
        return 0;
    s->last_feedback_time = now;

    if (!fd)
        pb = avio;
    else if (avio_open_dyn_buf(&pb) < 0)
        return -1;

    if (need_keyframe)
    {
        avio_w8(pb, (RTP_VERSION << 6) | 1); /* PLI */
        avio_w8(pb, RTCP_PSFB);
        avio_wb16(pb, 2); /* length in words - 1 */
        /*  our own SSRC: we use the server's SSRC + 1 to avoid conflicts */
        avio_wb32(pb, s->ssrc + 1);
        avio_wb32(pb, s->ssrc); /*  server SSRC */
    }

    if (missing_packets)
    {
        avio_w8(pb, (RTP_VERSION << 6) | 1); /* NACK */
        avio_w8(pb, RTCP_RTPFB);
        avio_wb16(pb, 3); /* length in words - 1 */
        avio_wb32(pb, s->ssrc + 1);
        avio_wb32(pb, s->ssrc); /*  server SSRC */

        avio_wb16(pb, first_missing);
        avio_wb16(pb, missing_mask);
    }

    avio_flush(pb);
    if (!fd)
        return 0;
    len = avio_close_dyn_buf(pb, &buf);
    if (len > 0 && buf)
    {
        ffurl_write(fd, buf, len);
        av_free(buf);
    }
    return 0;
}

/**
 * open a new RTP parse context for stream 'st'. 'st' can be NULL for
 * MPEG2-TS streams.
 */
RTPDemuxContext * ff_rtp_parse_open(AVFormatContext * s1, AVStream * st, int payload_type, int queue_size)
{
    RTPDemuxContext * s;

    s = av_mallocz(sizeof(RTPDemuxContext));
    if (!s)
        return NULL;
    s->payload_type        = payload_type;
    s->last_rtcp_ntp_time  = AV_NOPTS_VALUE;
    s->first_rtcp_ntp_time = AV_NOPTS_VALUE;
    s->ic                  = s1;
    s->st                  = st;
    s->queue_size          = queue_size;
    rtp_init_statistics(&s->statistics, 0);
    if (st)
    {
        switch (st->codec->codec_id)
        {
            default:
                break;
        }
    }
    /*  needed to send back RTCP RR in RTSP sessions */
    gethostname(s->hostname, sizeof(s->hostname));
    return s;
}

void ff_rtp_parse_set_dynamic_protocol(RTPDemuxContext * s, PayloadContext * ctx,
                                       RTPDynamicProtocolHandler * handler)
{
    s->dynamic_protocol_context = ctx;
    s->handler                  = handler;
}

#if 0
void ff_rtp_parse_set_crypto(RTPDemuxContext * s, const char * suite,
                             const char * params)
{
    if (!ff_srtp_set_crypto(&s->srtp, suite, params))
        s->srtp_enabled = 1;
}
#endif

/**
 * This was the second switch in rtp_parse packet.
 * Normalizes time, if required, sets stream_index, etc.
 */
static void finalize_packet(RTPDemuxContext * s, AVPacket * pkt, uint32_t timestamp)
{
    if (pkt->pts != AV_NOPTS_VALUE || pkt->dts != AV_NOPTS_VALUE)
        return; /* Timestamp already set by depacketizer */
    if (timestamp == RTP_NOTS_VALUE)
        return;
    /*Start of luny on 2014-9-24 15:19 1.0*/

#if 0
    if (s->last_rtcp_ntp_time != AV_NOPTS_VALUE && s->ic->nb_streams > 1)
    {
        int64_t addend;
        int delta_timestamp;

        /* compute pts from timestamp with received ntp_time */
        delta_timestamp = timestamp - s->last_rtcp_timestamp;
        /* convert to the PTS timebase */
        addend = av_rescale(s->last_rtcp_ntp_time - s->first_rtcp_ntp_time,
                            s->st->time_base.den,
                            (uint64_t) s->st->time_base.num << 32);
        pkt->pts = s->range_start_offset + s->rtcp_ts_offset + addend +
                   delta_timestamp;
        return;
    }
#endif

    if (!s->base_timestamp)
        s->base_timestamp = timestamp;
    /* assume that the difference is INT32_MIN < x < INT32_MAX,
     * but allow the first timestamp to exceed INT32_MAX */
    if (!s->timestamp)
        s->unwrapped_timestamp += timestamp;
    else
        s->unwrapped_timestamp += (int32_t)(timestamp - s->timestamp);
    /*Start of luny on 2014-9-2 9:46 */
    /* video(25fps): delta(3600) / 90000 * 1000000 = 40000 us */
    /* audio(8khz):  delta(320)  / 8000  * 1000000 = 40000 us */
    if (!s->timestamp)
    {
        pkt->delta = 0;
    }
    else
    {
        pkt->delta = ((int64_t)(timestamp - s->timestamp) * 1000000) / s->st->time_base.den;/*luny*/
        pkt->packet_ts = timestamp;

        /*if(pkt->delta >0)
        {
             av_log(s->ic, AV_LOG_WARNING, "++++++++[%s] delta = %d   %d   %d\n",name, pkt->delta, (int32_t)(timestamp - s->timestamp), s->st->time_base.den);
        }*/
    }



#if 0
    if (!pkt->average_delta)
    {
        pkt->average_delta = pkt->delta;
    }
    else if (pkt->delta > pkt->average_delta * 3)
    {
        av_log(s->ic, AV_LOG_WARNING, "---average_delta too long \n");
    }
    else if (pkt->delta > 0)
    {
        pkt->average_delta = (pkt->average_delta + pkt->delta) / 2;
    }
#endif
    /*End of luny on 2014-9-2 9:46 */
    s->timestamp = timestamp;
    pkt->pts     = s->unwrapped_timestamp + s->range_start_offset -
                   s->base_timestamp;
}

#if 0
static int rtp_parse_packet_internal(RTPDemuxContext * s, AVPacket * pkt,
                                     const uint8_t * buf, int len)
{
    unsigned int ssrc;
    int payload_type, seq, flags = 0;
    int ext, csrc;
    AVStream * st;
    uint32_t timestamp;
    int rv = 0;

    csrc         = buf[0] & 0x0f;
    ext          = buf[0] & 0x10;
    payload_type = buf[1] & 0x7f;
    if (buf[1] & 0x80)
        flags |= RTP_FLAG_MARKER;
    seq       = AV_RB16(buf + 2);
    timestamp = AV_RB32(buf + 4);
    ssrc      = AV_RB32(buf + 8);
    /* store the ssrc in the RTPDemuxContext */
    s->ssrc = ssrc;

    /* NOTE: we can handle only one payload type */
    if (s->payload_type != payload_type)
        return -1;

    st = s->st;
    /*  only do something with this if all the rtp checks pass... */
    if (!rtp_valid_packet_in_sequence(&s->statistics, seq))
    {
        av_log(st ? st->codec : NULL, AV_LOG_ERROR,
               "RTP: PT=%02x: bad cseq %04x expected=%04x\n",
               payload_type, seq, ((s->seq + 1) & 0xffff));
        return -1;
    }
#ifdef DEBUG_PACKET_FLOW
    av_log(st ? st->codec : NULL, AV_LOG_DEBUG, "Seq:%d mark:%d\n", seq, buf[1] & 0x80);
#endif
    if (buf[0] & 0x20)
    {
        int padding = buf[len - 1];
        if (len >= 12 + padding)
            len -= padding;
    }

    s->seq = seq;
    len   -= 12;
    buf   += 12;

    len   -= 4 * csrc;
    buf   += 4 * csrc;
    if (len < 0)
        return AVERROR_INVALIDDATA;

    /* RFC 3550 Section 5.3.1 RTP Header Extension handling */
    if (ext)
    {
        if (len < 4)
            return -1;
        /* calculate the header extension length (stored as number
         * of 32-bit words) */
        ext = (AV_RB16(buf + 2) + 1) << 2;

        if (len < ext)
            return -1;
        /*  skip past RTP header extension */
        len -= ext;
        buf += ext;
    }

    if (s->handler && s->handler->parse_packet)
    {
        rv = s->handler->parse_packet(s->ic, s->dynamic_protocol_context,
                                      s->st, pkt, &timestamp, buf, len, seq,
                                      flags);
        /* rv = h264_handle_packet(s->ic, s->dynamic_protocol_context, s->st, pkt, &timestamp, buf, len, seq, flags); */
    }
    else if (st)
    {
#ifdef REUSE_RECV_BUF
        if ((rv = av_new_packet_from_buffer(pkt, buf, len)) < 0)
        {
            return rv;
        }
#else
        if ((rv = av_new_packet(pkt, len)) < 0)
        {
            return rv;
        }
        memcpy(pkt->data, buf, len);
#endif
        pkt->stream_index = st->index;
    }
    else
    {
        return AVERROR(EINVAL);
    }

    /*  now perform timestamp things.... */
    finalize_packet(s, pkt, timestamp);

    return rv;
}
#endif

void ff_rtp_reset_packet_queue(RTPDemuxContext * s)
{
    while (s->queue)
    {
        RTPPacket * next = s->queue->next;
        av_free(s->queue->buf);
        av_free(s->queue);
        s->queue = next;
    }
    s->seq       = 0;
    s->queue_len = 0;
    s->prev_ret  = 0;
}

static int has_next_packet(RTPDemuxContext * s)
{
    return s->queue && s->queue->seq == (uint16_t)(s->seq + 1);
}

int64_t ff_rtp_queued_packet_time(RTPDemuxContext * s)
{
    return s->queue ? s->queue->recvtime : 0;
}

void ff_rtp_parse_close(RTPDemuxContext * s)
{
    ff_rtp_reset_packet_queue(s);
    av_free(s);
}

int ff_parse_fmtp(AVStream * stream, PayloadContext * data, const char * p,
                  int (*parse_fmtp)(AVStream * stream,
                                    PayloadContext * data,
                                    const char * attr, const char * value))
{
    char attr[256];
    char * value;
    int res;
    int value_size = strlen(p) + 1;

    if (!(value = av_malloc(value_size)))
    {
        av_log(NULL, AV_LOG_ERROR, "Failed to allocate data for FMTP.\n");
        return AVERROR(ENOMEM);
    }

    /*  remove protocol identifier */
    while (*p && *p == ' ')
        p++;                     /*  strip spaces */
    while (*p && *p != ' ')
        p++;                     /*  eat protocol identifier */
    while (*p && *p == ' ')
        p++;                     /*  strip trailing spaces */

    while (ff_rtsp_next_attr_and_value(&p,
                                       attr, sizeof(attr),
                                       value, value_size))
    {
        res = parse_fmtp(stream, data, attr, value);
        if (res < 0 && res != AVERROR_PATCHWELCOME)
        {
            av_free(value);
            return res;
        }
    }
    av_free(value);
    return 0;
}

