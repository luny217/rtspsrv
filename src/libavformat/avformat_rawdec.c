/*
 * RAW demuxers
 * Copyright (c) 2001 Fabrice Bellard
 * Copyright (c) 2005 Alex Beregszaszi
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

#include "avformat.h"
#include "avformat_internal.h"
#include "avformat_avio_internal.h"
#include "avformat_rawdec.h"
#include "libavutil/avutil_opt.h"
#include "libavutil/avutil_parseutils.h"
#include "libavutil/avutil_pixdesc.h"
#include "libavutil/avutil_avassert.h"
#include "libavutil/avutil_intreadwrite.h"

#define RAW_PACKET_SIZE 1024

int ff_raw_read_partial_packet(AVFormatContext *s, AVPacket *pkt)
{
    int ret, size;

    size = RAW_PACKET_SIZE;

    if (av_new_packet(pkt, size) < 0)
        return AVERROR(ENOMEM);

    pkt->pos= avio_tell(s->pb);
    pkt->stream_index = 0;
    ret = ffio_read_partial(s->pb, pkt->data, size);
    if (ret < 0) {
        av_free_packet(pkt);
        return ret;
    }
    av_shrink_packet(pkt, ret);
    return ret;
}

int ff_raw_audio_read_header(AVFormatContext *s)
{
    AVStream *st = avformat_new_stream(s, NULL);
    if (!st)
        return AVERROR(ENOMEM);
    st->codec->codec_type = AVMEDIA_TYPE_AUDIO;
    st->codec->codec_id = s->iformat->raw_codec_id;
    st->need_parsing = AVSTREAM_PARSE_FULL_RAW;
    st->start_time = 0;
    /* the parameters will be extracted from the compressed bitstream */

    return 0;
}

/* MPEG-1/H.263 input */
int ff_raw_video_read_header(AVFormatContext *s)
{
    AVStream *st;
    FFRawVideoDemuxerContext *s1 = s->priv_data;
    int ret = 0;


    st = avformat_new_stream(s, NULL);
    if (!st) {
        ret = AVERROR(ENOMEM);
        goto fail;
    }

    st->codec->codec_type = AVMEDIA_TYPE_VIDEO;
    st->codec->codec_id = s->iformat->raw_codec_id;
    st->need_parsing = AVSTREAM_PARSE_FULL_RAW;

    st->codec->time_base = av_inv_q(s1->framerate);
    avpriv_set_pts_info(st, 64, 1, 1200000);

fail:
    return ret;
}

static int ff_raw_data_read_header(AVFormatContext *s)
{
    AVStream *st = avformat_new_stream(s, NULL);
    if (!st)
        return AVERROR(ENOMEM);
    st->codec->codec_type = AVMEDIA_TYPE_DATA;
    st->codec->codec_id = s->iformat->raw_codec_id;
    st->start_time = 0;
    return 0;
}


/* Note: Do not forget to add new entries to the Makefile as well. */

#define OFFSET(x) offsetof(FFRawVideoDemuxerContext, x)
#define DEC AV_OPT_FLAG_DECODING_PARAM
const AVOption ff_rawvideo_options[] = {
    { "framerate", "", OFFSET(framerate), AV_OPT_TYPE_VIDEO_RATE, {.str = "25"}, 0, 0, DEC},
    { NULL },
};