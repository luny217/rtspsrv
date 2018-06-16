/*
 * FileName:
 * Author:         luny  Version: 1.0  Date: 2017-7-6
 * Description:
 * Version:
 * Function List:
 *                 1.
 * History:
 *     <author>   <time>    <version >   <desc>
 */

#ifndef _WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#endif

#include "avutil_time.h"
#include "avformat.h"
#include "avformat_network.h"
#include "mshead.h"
#include "rtspsrv_utils.h"
#include "rtspsrv_handler.h"
#include "rtspsrv_stream.h"
#include "fifo.h"

#ifndef _RTSPSRV_TEST
#include <mem_manage.h>
#endif

#define SUPPORT_RTP_UDP_TRANSPORT 0
#define _DISCARD_DATA_SIZE     (2 << 20)

#define AV_TIME_BASE_VIDEO (AVRational){1, 90000}
#define AV_TIME_BASE_PAL (AVRational){1, 25}
#define AV_TIME_BASE_NTSC (AVRational){1, 30}

#define AV_TIME_BASE_AUDIO (AVRational){1, 8000}

#define NTP_TO_RTP_FORMAT(x) av_rescale((x), INT64_C(1) << 32, 1000000)

static int _process_rtcp_data(rtp_info_t * rinfo, int fd);

/******************************************************************************
* 函数介绍: 从一个NALU数据中获取一包RTP数据
* 输入参数: rtp_info: 构造一个rtp包所需要的信息
*			rtpHdr: RTP头
*			enc_type: 编码类型, H264 or H265
* 输出参数: p_packet_size: 获取到的RTP包的大小
* 返回值  : 获取到的RTP包的地址
*****************************************************************************/
uint8 * _get_video_rtp_pack(rtp_info_t * rtp_info, uint8 * rtpHdr, uint32 enc_type, int * p_packet_size)
{
    uint32 bytes_left = 0, max_size = 0;
    uint8 * cp = NULL, *cp2 = NULL;
    nal_msg_t * nal = NULL;
    rtp_hdr_t * header = NULL;

    if (rtp_info == NULL)
    {
        return NULL;
    }

    nal = &rtp_info->nal;
    header = &rtp_info->rtp_hdr;

    /* 判断是否到达结尾 */
    if (rtp_info->end == nal->end)
    {
        *p_packet_size = 0;
        return NULL;
    }

    /* 判断是否是一个NALU的开始 */
    if (rtp_info->nal_begin)
    {
        rtp_info->nal_begin = HB_FALSE;
    }
    else
    {
        /* 设置上一个RTP包的结束位置为此RTP包的开始位置 */
        rtp_info->start = rtp_info->end;/* continue with the next RTP-FU packet */
    }

    /* 计算剩余的字节数 */
    bytes_left = nal->end - rtp_info->start;

    /* 设置RTP包数据最大的字节数 */
    max_size = MAX_RTP_PACKET_SIZE; /* sizeof(basic rtp header) == 12 bytes */

    if (rtp_info->fu_flag)
    {
        if (enc_type == VIDEO_H264)
            max_size -= 2; /* FU分包的情况，RTP Header为14字节，RTP包数据最大的字节数需要减2 */
        else if (enc_type == VIDEO_H265)
            max_size -= 3; /* FU分包的情况，RTP Header为15字节，RTP包数据最大的字节数需要减3 */
    }

    /* 判断是否是最后一包数据 */
    if (bytes_left > max_size)
    {
        rtp_info->end = rtp_info->start + max_size; /* limit RTP packet size to 1472 bytes */
    }
    else
    {
        rtp_info->end = rtp_info->start + bytes_left;
    }
    /* 判断是否是FU分包 */
    if (rtp_info->fu_flag)
    {
        /* 判断是否到达结尾，设置FU Header的结束位 */
        if (rtp_info->end == nal->end)
        {
            rtp_info->e_bit = 1;
        }
    }

    /* 设置一帧数据最后一包数据的标识位*/
    rtp_info->rtp_hdr.marker = nal->eof ? 1 : 0;
    if (rtp_info->fu_flag && !rtp_info->e_bit)
    {
        rtp_info->rtp_hdr.marker = 0;
    }

    /* 设置RTP Header以及RTP数据的指针 */
    cp = rtpHdr;
    rtp_info->pRTP = rtp_info->start;

    if (enc_type == VIDEO_H264)
    {
        cp2 = (uint8 *)&rtp_info->rtp_hdr;
        cp[0] = cp2[0];
        cp[1] = cp2[1];
    }
    else if (enc_type == VIDEO_H265)
    {
        cp[0] = 0x80;
        cp[1] = (rtp_info->rtp_hdr.marker ? 0x80 : 0x00) | 96;
    }
    /* 设置序列号*/
    cp[2] = (header->seq_no >> 8) & 0xff;
    cp[3] = header->seq_no & 0xff;

    /* 设置时间戳*/
    cp[4] = (header->timestamp >> 24) & 0xff;
    cp[5] = (header->timestamp >> 16) & 0xff;
    cp[6] = (header->timestamp >> 8) & 0xff;
    cp[7] = header->timestamp & 0xff;

    /* 设置源标识*/
    cp[8] = (header->ssrc >> 24) & 0xff;
    cp[9] = (header->ssrc >> 16) & 0xff;
    cp[10] = (header->ssrc >> 8) & 0xff;
    cp[11] = header->ssrc & 0xff;

    /* 序列号累加*/
    header->seq_no++;

    if (enc_type == VIDEO_H264)
    {
        /*!
        * \n The FU indicator octet has the following format:
        * \n
        * \n      +---------------+
        * \n MSB  |0|1|2|3|4|5|6|7|  LSB
        * \n      +-+-+-+-+-+-+-+-+
        * \n      |F|NRI|  Type   |
        * \n      +---------------+
        * \n
        * \n The FU header has the following format:
        * \n
        * \n      +---------------+
        * \n      |0|1|2|3|4|5|6|7|
        * \n      +-+-+-+-+-+-+-+-+
        * \n      |S|E|R|  Type   |
        * \n      +---------------+
        */
        /* 设置FU Header */
        if (rtp_info->fu_flag)
        {
            /* FU indicator  F|NRI|Type*/
            cp[12] = cp[13] = 0;
            /* 设置RTP分包方式为FU_A，28*/
            cp[12] = (nal->type & 0xe0) | 28; /*Type is 28 for FU_A*/
            /* FU header S|E|R|Type*/
            cp[13] = (rtp_info->s_bit << 7)
                     | (rtp_info->e_bit << 6) | (nal->type & 0x1f);
            /* R = 0, must be ignored by receiver*/
            rtp_info->s_bit = rtp_info->e_bit = 0;

            rtp_info->hdr_len = 14; /*RTP Header的长度为14*/
        }
        else
        {
            rtp_info->hdr_len = 12; /* RTP Header的长度为12*/
        }
    }
    else if (enc_type == VIDEO_H265)
    {
        /*///////////////////////  RTP payload head  /////////////////////////////////

        码流类型type: 96
        FU type 	: 49
        nal type	: vps sps pps

        +---------------+---------------+
        |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |F|	 Type	 |	LayerId  | TID |
        +-------------+-----------------+
        1 0 0 0 0 0 0 1

        +---------------+
        |0|1|2|3|4|5|6|7|
        +-+-+-+-+-+-+-+-+
        |S|E|  FuType	|
        +---------------+
        ////////////////////////////////////////////////////////////////////////////*/

        /* 设置FU Header */
        if (rtp_info->fu_flag)
        {

            cp[12] = 49 << 1;
            cp[13] = 1;
            cp[14] = nal->type;
            cp[14] = nal->type | rtp_info->e_bit << 6 | rtp_info->s_bit << 7;
            rtp_info->s_bit = rtp_info->e_bit = 0;
            rtp_info->hdr_len = 15; /* RTP Header的长度为12+1 */
        }
        else
        {
            rtp_info->hdr_len = 12; /* RTP Header的长度为12 */
        }

    }
    /* 计算RTP包的长度 */
    *p_packet_size = (rtp_info->end - rtp_info->start);
    /* 返回RTP包的地址(Header + Payload) */
    return rtp_info->pRTP;
}

void _get_audio_rtp_pack(rtp_info_t * rtp_info, uint32 ts)
{
    uint8 * cp = NULL;
    rtp_hdr_t * rtp_header = NULL;

    /*RTP Header赋值*/
    rtp_header = (rtp_hdr_t *)(rtp_info->hdr_data + 4);
    cp = (uint8 *)rtp_header;

    rtp_header->version = rtp_info->rtp_hdr.version; /*设置版本号*/
    rtp_header->payload = rtp_info->rtp_hdr.payload; /*设置负载类型*/
    rtp_header->marker = 1; /*最后一包数据的标识位，设置为1*/

    /*设置序列号*/
    cp[2] = (rtp_info->rtp_hdr.seq_no >> 8) & 0xff;
    cp[3] = rtp_info->rtp_hdr.seq_no & 0xff;

    /*序列号累加*/
    rtp_info->rtp_hdr.seq_no++;

    /*设置时间戳*/
    cp[4] = (ts >> 24) & 0xff;
    cp[5] = (ts >> 16) & 0xff;
    cp[6] = (ts >> 8) & 0xff;
    cp[7] = ts & 0xff;

    /*设置源标识*/
    cp[8] = (rtp_info->rtp_hdr.ssrc >> 24) & 0xff;
    cp[9] = (rtp_info->rtp_hdr.ssrc >> 16) & 0xff;
    cp[10] = (rtp_info->rtp_hdr.ssrc >> 8) & 0xff;
    cp[11] = rtp_info->rtp_hdr.ssrc & 0xff;
    rtp_info->hdr_len = 12;
}

/******************************************************************************
* 函数介绍: 将NAL的信息填充进rtp信息的结构体中
* 输入参数: rtp_info: 构造一个rtp包所需要的信息
*			enc_type: 编码类型, H264 or H265
*           nal_buf: NAL数据缓冲区的指针
*           nal_size: NAL数据缓冲区的大小
*           time_stamp: rtp时间戳
*           end_of_frame: 是否是EOF的标识
* 输出参数: 无
* 返回值  : 成功返回0，出错返回-1
*****************************************************************************/
int set_rtp_pack(rtp_info_t * rtp_info, uint32 enc_type, uint8 * nal_buf, uint32 nal_size, uint32 time_stamp, uint32 end_of_frame)
{
    nal_msg_t * nal = NULL;

    if (rtp_info == NULL)
    {
        rsrv_log(AV_LOG_WARNING, "rtp_info is NULL\n");
        return ERR_INPUT_PARAM;
    }

    nal = &rtp_info->nal;

    /* 将一个NALU的信息填充进RTP信息结构体中 */
    nal->start = nal_buf;
    nal->size = nal_size;
    nal->eof = end_of_frame;
    if (enc_type == VIDEO_H264)
    {
        nal->type = nal->start[0];
    }
    else if (enc_type == VIDEO_H265)
    {
        nal->type = (nal->start[0] >> 1) & 0x3F;
    }
    nal->end = nal->start + nal->size;
    rtp_info->rtp_hdr.timestamp = time_stamp;

    /*判断是否需要进行RTP分包*/
    if (nal->size > MAX_RTP_PACKET_SIZE)
    {
        rtp_info->fu_flag = HB_TRUE;
        rtp_info->s_bit = 1; /*先将s_bit置1，get_rtp_pack后，会将s_bit置0*/
        rtp_info->e_bit = 0;

        if (enc_type == VIDEO_H264)
        {
            nal->start += 1; /*跳过NALU Type字节*/
        }
        else if (enc_type == VIDEO_H265)
        {
            nal->start += 2; /*跳过NALU Type字节*/
        }
    }
    else
    {
        rtp_info->fu_flag = HB_FALSE;
        rtp_info->s_bit = rtp_info->e_bit = 0;
    }

    rtp_info->start = rtp_info->end = nal->start;
    rtp_info->nal_begin = HB_TRUE;

    return 0;
}

int _rtcp_send_sr(rtp_info_t * rinfo, int fd, sint64 ntp_time)
{
    char rtcp_buf[128] = { 0 };
	uint32 rtp_ts = 0;
	sint32 val = 0;
    uint64 val64;

    /*  Sender Report */

    rinfo->last_rtcp_ntp_time = ntp_time;
    /*rtp_ts = av_rescale_q(ntp_time - rinfo->first_rtcp_ntp_time,
    	(AVRational){1, 1000000}, s1->streams[0]->time_base) + s->base_timestamp;*/

    rtcp_buf[0] = 0x24;
    rtcp_buf[1] = (uint8)rinfo->channel_id;
    rtcp_buf[2] = RTCP_SR_SIZE >> 8;
    rtcp_buf[3] = (uint8)RTCP_SR_SIZE;

    rtcp_buf[4] = (uint8)(RTP_VERSION << 6); /* avio_w8(s1->pb, RTP_VERSION << 6); */
    rtcp_buf[5] = (uint8)(RTCP_SR); /* avio_w8(s1->pb, RTCP_SR); */
    rtcp_buf[6] = (uint8)(0x06 >> 8); /* avio_wb16(s1->pb, 6); */
    rtcp_buf[7] = (uint8)(0x06);

    val = rinfo->ssrc;
    rtcp_buf[8] = val >> 24;
    rtcp_buf[9] = (uint8)(val >> 16);
    rtcp_buf[10] = (uint8)(val >> 8);
    rtcp_buf[11] = (uint8)val; /* avio_wb32(s1->pb, s->ssrc); */

    val64 = NTP_TO_RTP_FORMAT(ntp_time);
    rtcp_buf[12] = val64 >> 56;
    rtcp_buf[13] = (uint8)(val64 >> 48);
    rtcp_buf[14] = (uint8)(val64 >> 40);
    rtcp_buf[15] = (uint8)(val64 >> 32);
    rtcp_buf[16] = (uint8)(val64 >> 24);
    rtcp_buf[17] = (uint8)(val64 >> 16);
    rtcp_buf[18] = (uint8)(val64 >> 8);
    rtcp_buf[19] = (uint8)val64;	/*avio_wb64(s1->pb, NTP_TO_RTP_FORMAT(ntp_time));*/

    val = rtp_ts;
    rtcp_buf[20] = val >> 24;
    rtcp_buf[21] = (uint8)(val >> 16);
    rtcp_buf[22] = (uint8)(val >> 8);
    rtcp_buf[23] = (uint8)val; /* avio_wb32(s1->pb, rtp_ts); */

    val = rinfo->packet_count;
    rtcp_buf[24] = val >> 24;
    rtcp_buf[25] = (uint8)(val >> 16);
    rtcp_buf[26] = (uint8)(val >> 8);
    rtcp_buf[27] = (uint8)val; /* avio_wb32(s1->pb, s->packet_count); */

    val = rinfo->octet_count;
    rtcp_buf[28] = val >> 24;
    rtcp_buf[29] = (uint8)(val >> 16);
    rtcp_buf[30] = (uint8)(val >> 8);
    rtcp_buf[31] = (uint8)val; /* avio_wb32(s1->pb, s->octet_count); */

    val = send(fd, (const unsigned char *)rtcp_buf, RTCP_SR_SIZE + 4, 0);
    if(val >0 && val != RTCP_SR_SIZE + 4)
    {
        rsrv_log(AV_LOG_ERROR, "rtcp left[%d]\n", 32 - val);
    }

    return val;
}

int rsrv_reply_heart(rtsp_sessn_t * rss)
{
    if (rss->heart_beat && !rss->left_size)
    {
        rsrv_reply_options(rss);
        rss->heart_beat = 0;
        /*av_log(NULL, AV_LOG_WARNING, "[fd:%d]send heart beat Cseq:%d\n", rtsp->fd, rtsp->cseq);*/
        return 1;
    }
    return 0;
}

int _process_cache_data(rtp_trans_t * trans)
{
    writev_info_t * wv_info = &trans->wv_info;
    uint32 send_bytes = 0;
    int ret;

    if (trans->type != RTP_RTP_AVP_TCP)
        return 0; 

    if (wv_info->piov_size > 0)
    {
        ret = send(trans->tcp_fd, wv_info->buf + wv_info->send_size, wv_info->piov_size, 0);
        if (ret > 0)
        {
            send_bytes = (uint32)ret;
            if (send_bytes == wv_info->piov_size)
            {
                wv_info->send_size = 0;
                wv_info->piov_size = 0;
                return 0;
            }
            /*rsrv_log(AV_LOG_WARNING, "buf_len:%d send_bytes:%d\n",wv_info->piov_size, send_bytes);*/
            wv_info->send_size += send_bytes;
            wv_info->piov_size -= send_bytes;
            return send_bytes;
        }
        else
        {
            if (ff_neterrno() == AVERROR(EAGAIN) || ff_neterrno() == AVERROR(EINTR))
            {
                /*rsrv_log(AV_LOG_WARNING, "EAGAIN errno:%d \n", errno);*/
                return 1;
            }
            else
            {
                rsrv_log(AV_LOG_WARNING, "errno:%d close send\n", errno);
                return -1;
            }
        }
    }
    return 0;
}


/*
// 函数名：rtsp_add_interleaved_head
// 描  述：向RTP流中添加TCP的头
// 参  数：[in/out] rtp_buf     rtp数据缓冲区
//         [in]     rtp_size    rtp数据大小
//         [in]     channel_id  通道ID
// 返回值：无
// 说  明：
*/

void _add_interhead(uint8 * rtp_buf, uint16 rtp_size, uint32 channel_id)
{
    rtp_buf[0] = '$';
    rtp_buf[1] =  channel_id;
    rtp_buf[2] = (uint8)((rtp_size & 0xff00) >> 8);
    rtp_buf[3] = (uint8)((rtp_size & 0xff));
}

rtp_info_t * _get_rtpinfo(rtp_sessn_t * rtp_sessn, rtpmd_type_e type)
{
    if (M_VIDEO == type)
        return &(rtp_sessn->vinfo);
    else if (M_AUDIO == type)
        return &(rtp_sessn->ainfo);
    else if (M_TEXT == type)
        return &(rtp_sessn->tinfo);
    return NULL;
}


#ifdef _WIN32
int _send_udp_data(int fd, uint8 * header, uint32 header_size, uint8 * data, uint32 data_size)
{
    WSABUF wsabufs[2];
    sint32 snd_size = -1;
    int ret;

    wsabufs[0].buf = header;
    wsabufs[0].len = header_size;
    wsabufs[1].buf = data;
    wsabufs[1].len = data_size;

    ret = WSASend(fd, wsabufs, 2, &snd_size, 0, NULL, NULL);
    if (ret < 0)
    {
        if (ff_neterrno() == AVERROR(EAGAIN) || ff_neterrno() == AVERROR(EINTR))
        {
            return 0;
        }
        else
        {
            rsrv_log(AV_LOG_ERROR, "writeV[%d] actual send :%d header:%d data:%d needdata:%d\n",
                   fd, snd_size, header_size, data_size, (data_size + header_size));
            return -1;
        }
    }
    return 0;
}

int _send_tcp_data(rtp_trans_t * trans, uint8 * header, uint32 header_size, char * data, uint32 data_size)
{
    WSABUF wsabufs[2];
    sint32 snd_size = -1;
    uint32 left_size = 0;
    int tcp_fd, ret, try_time = MAX_TRY_TIME;
    writev_info_t * wv_info = NULL;

    if (trans == NULL || header == NULL || data == NULL)
    {
        return -1;
    }

    tcp_fd =  trans->tcp_fd;
    wv_info = &trans->wv_info;

    wsabufs[0].buf = header;
    wsabufs[0].len = header_size;
    wsabufs[1].buf = data;
    wsabufs[1].len = data_size;

    while (try_time > 0)
    {
        ret = _process_cache_data(trans);
        if (ret == 0)
        {
            if (try_time < MAX_TRY_TIME)
            {
                rsrv_log(AV_LOG_WARNING, "try_time:%d\n", try_time);
            }
            break;
        }
        else if (ret < 0)
        {
            rsrv_log(AV_LOG_WARNING, "_process_cache_data err:%d\n", ret);
            return ret;
        }
        try_time--;
    }

    if(!try_time)
    {
        rsrv_log(AV_LOG_WARNING, "MAX_TRY_TIME piov_size:%d\n", wv_info->piov_size);
        
        trans->blocked = 1;
        return AVERROR(EAGAIN);
    }

    ret = WSASend(tcp_fd, wsabufs, 2, &snd_size, 0, NULL, NULL);
    if (ret < 0)
    {
        if (ff_neterrno() == AVERROR(EAGAIN) || ff_neterrno() == AVERROR(EINTR))
        {
            /*if (data_size != 1446)
                av_log(NULL, AV_LOG_WARNING, "EAGAIN = %d data = %p\n", data_size, data);*/
            trans->blocked = 1;
            return AVERROR(EAGAIN);
        }
        else
        {
            rsrv_log(AV_LOG_ERROR, "WSASend[%d] actual send :%d header:%d data:%d needdata:%d\n",
                   tcp_fd, snd_size, header_size, data_size, (data_size + header_size));
            return -1;
        }
    }
    else if (snd_size != (header_size + data_size))
    {
        if (snd_size < wsabufs[0].len)
        {
            left_size = wsabufs[0].len - snd_size;
            memcpy(wv_info->buf, wsabufs[0].buf + (wsabufs[0].len - left_size), left_size);
            wv_info->piov_size += left_size;
            memcpy(wv_info->buf + wv_info->piov_size, wsabufs[1].buf, wsabufs[1].len);
            wv_info->piov_size += wsabufs[1].len;
        }
        else
        {
            left_size = wsabufs[1].len - (snd_size - wsabufs[0].len);
            if ((wv_info->piov_size + left_size) >= wv_info->buf_size)
            {
                rsrv_log(AV_LOG_ERROR, "malloc_buflen[%d] piov_len[%d] iLeft[%d]\n", wv_info->buf_size, wv_info->piov_size, left_size);
                return 0;
            }
            memcpy(wv_info->buf + wv_info->piov_size, wsabufs[1].buf + (wsabufs[1].len - left_size), left_size);
            wv_info->piov_size += left_size;
        }
    }
    return snd_size;
}
#else

int _send_udp_data(int fd, uint8 * header, uint32 header_size, uint8 * data, uint32 data_size)
{
    struct iovec iov[2];
    int ret;

    iov[0].iov_base = header;
    iov[0].iov_len = header_size;
    iov[1].iov_base = data;
    iov[1].iov_len = data_size;

    ret = writev(fd, iov, 2);
    if (ret < 0)
    {
        if ((errno == EAGAIN) || (errno == EINTR) || (errno == 0))
        {
            return 0;
        }
        else
        {
            /*rtp_sessn->started = 0;*/
            return -1;
        }
    }
    return 0;
}

int _send_tcp_data(rtp_trans_t * trans, uint8 * header, uint32 header_size, uint8 * data, uint32 data_size)
{
    struct iovec iov[2];
    uint32 snd_size = 0, left_size = 0;
    int tcp_fd, ret, try_time = MAX_TRY_TIME;
    writev_info_t * wv_info = NULL;

    if (trans == NULL || header == NULL || data == NULL)
    {
        return -1;
    }

    tcp_fd =  trans->tcp_fd;
    wv_info = &trans->wv_info;

    iov[0].iov_base = header;
    iov[0].iov_len = header_size;
    iov[1].iov_base = data;
    iov[1].iov_len = data_size;

    while (try_time > 0)
    {
        ret = _process_cache_data(trans);
        if (ret == 0)
        {
            if (try_time < MAX_TRY_TIME)
            {
                rsrv_log(AV_LOG_WARNING, "try_time:%d\n", try_time);
            }
            break;
        }
        else if (ret < 0)
        {
            rsrv_log(AV_LOG_WARNING, "_process_cache_data err:%d\n", ret);
            return ret;
        }
        try_time--;
    }

    if(!try_time)
    {
        /*rsrv_log(AV_LOG_WARNING, "MAX_TRY_TIME piov_size:%d\n", wv_info->piov_size);*/        
        trans->blocked = 1;
        return AVERROR(EAGAIN);
    }

    ret = writev(tcp_fd, iov, 2);

    if (ret < 0)
    {
        if (ff_neterrno() == AVERROR(EAGAIN) || ff_neterrno() == AVERROR(EINTR))
        {           
            trans->blocked = 1;
            return AVERROR(EAGAIN);
        }
        else
        {
            rsrv_log(AV_LOG_ERROR, "writev[%d] actual send :%d header:%d data:%d needdata:%d\n",
                   tcp_fd, snd_size, header_size, data_size, (data_size + header_size));
            return -1;
        }
    }
    else
    {
        snd_size = (uint32)ret;

        if (snd_size != (header_size + data_size))
        {
            if (snd_size < iov[0].iov_len)
            {
                left_size = iov[0].iov_len - snd_size;
                memcpy(wv_info->buf, iov[0].iov_base + (iov[0].iov_len - left_size), left_size);
                wv_info->piov_size += left_size;
                memcpy(wv_info->buf + wv_info->piov_size, iov[1].iov_base, iov[1].iov_len);
                wv_info->piov_size += iov[1].iov_len;
            }
            else
            {
                left_size = iov[1].iov_len - (snd_size - iov[0].iov_len);
                if ((wv_info->piov_size + left_size) >= wv_info->buf_size)
                {
                    rsrv_log(AV_LOG_ERROR, "buflen[%d] piov_len[%d] iLeft[%d]\n", wv_info->buf_size, wv_info->piov_size, left_size);
                    return 0;
                }
                memcpy(wv_info->buf + wv_info->piov_size, iov[1].iov_base + (iov[1].iov_len - left_size), left_size);
                wv_info->piov_size += left_size;
            }
        }
    }
    return (sint32)snd_size;
}

#endif

int _send_vstream(rtp_sessn_t * rtp_sessn, uint8 * buf, uint32 buf_size, strm_buf_t * strm_info)
{
    sint32 end_of_frame = 0;
    uint8 * rtp_buf = NULL, *nal_ptr = buf;
    int rtp_size = 0, nal_size = 0, ret = -1, nal_type;
    int nal_start = 0, nal_end = 0, deal_size = buf_size;
    rtp_info_t * rtp_info = NULL, last_rtp_info;
    rtp_trans_t * trans = NULL;

    if (rtp_sessn == NULL || buf == NULL || strm_info == NULL)
    {
        rsrv_log(AV_LOG_WARNING, "vstream buf is NULL\n");
        return ERR_INPUT_PARAM;
    }

    trans = &rtp_sessn->trans;

    rtp_info = _get_rtpinfo(rtp_sessn, M_VIDEO);
    if (rtp_info == NULL)
    {
        rsrv_log(AV_LOG_ERROR, "get video rtpinfo error!\n");
        return ERR_INPUT_PARAM;
    }

    if (trans->video_blocked > 0)
    {
        /*av_log(NULL, AV_LOG_WARNING, "[fd:%d last:%p buf:%p]\n", trans->tcp_fd, trans->last_buf_ptr, buf);*/

        if (trans->last_buf_size == buf_size && trans->last_buf_ptr == buf)
        {
            nal_ptr = trans->nal_ptr;
            nal_size = trans->nal_size;
            deal_size = trans->deal_size;
            nal_start = trans->nal_start;
            nal_end = trans->nal_end;
            end_of_frame = trans->end_frame;
            last_rtp_info = *rtp_info;
            trans->video_blocked = 0;
            goto RE_SEND;
        }
        else
        {
            trans->video_blocked = 0;
        }
    }

    while (nal_ptr + nal_size < buf + buf_size)
    {
        ret = rsrv_find_nal_unit(nal_ptr, deal_size, &nal_start, &nal_end, strm_info->enc_type);
        if (ret > 0)
        {
            nal_ptr += nal_start;
            nal_size = nal_end - nal_start;

            /*av_hex_dump(NULL, (uint8 *)nal_ptr, ret < 32 ? ret : 32);*/

            end_of_frame = rsrv_check_end_of_frame(strm_info->enc_type, nal_ptr[0], &nal_type);
            if (end_of_frame < 0)
            {
                rsrv_log(AV_LOG_ERROR, "rsrv_check_end_of_frame() err!");
                return ERR_INPUT_PARAM;
            }

            ret = set_rtp_pack(rtp_info, strm_info->enc_type, nal_ptr, nal_size, strm_info->ts, end_of_frame);
            if (ret < 0)
            {
                rsrv_log(AV_LOG_ERROR, "set_rtp_pack() err!");
                return ERR_INPUT_PARAM;
            }
            last_rtp_info = *rtp_info;
RE_SEND:
            while ((rtp_buf = _get_video_rtp_pack(rtp_info, (rtp_info->hdr_data + 4), strm_info->enc_type, &rtp_size)) != NULL)
            {
                if (trans->type == RTP_RTP_AVP)
                {
                    ret = _send_udp_data(trans->rtp_udp_vfd, (rtp_info->hdr_data + 4), rtp_info->hdr_len, rtp_buf, rtp_size);
                    if (ret < 0)
                    {
                        return -1;
                    }
                }
                else if (trans->type == RTP_RTP_AVP_TCP)
                {
                    _add_interhead(rtp_info->hdr_data, (rtp_size + rtp_info->hdr_len), VIDEO_RTP);
                    rtp_info->hdr_len += 4;                 

                    ret = _send_tcp_data(trans, rtp_info->hdr_data, rtp_info->hdr_len, rtp_buf, rtp_size);
                    if (ret < 0)
                    {
                        if (trans->blocked > 0)
                        {                            
                            trans->video_blocked = 1;
                            trans->blocked = 0;
                            trans->last_buf_ptr = buf;
                            trans->last_buf_size = buf_size;
                            trans->nal_ptr = nal_ptr;
                            trans->nal_size = nal_size;
                            trans->deal_size = deal_size;
                            trans->nal_start = nal_start;
                            trans->nal_end = nal_end;
                            trans->end_frame = end_of_frame;
                            *rtp_info = last_rtp_info;
                            rtp_sessn->again_count++;
                        }
                        return ret;
                    }
                    
                    last_rtp_info = *rtp_info;
                    rtp_sessn->vinfo.octet_count += ret;/* 计算rtcp包 */
                    rtp_sessn->octet_count += ret;/* 计算视频码率 */
                }
            }

            if (!end_of_frame && deal_size > 0 && nal_end != nal_start)
            {
                nal_ptr += (nal_end - nal_start);
                deal_size -= nal_end;
            }
        }
        else if (ret == 0)
        {
            rsrv_log(AV_LOG_WARNING, "nal_end == nal_start!\n");
            break;
        }
        else if (ret < 0)
        {
            rsrv_log(AV_LOG_ERROR, "not find nal !\n");
            break;
        }
    }
    return 0;
}

int _send_astream(rtp_sessn_t * rtp_sessn, uint8 * buf, uint32 buf_size, strm_buf_t * strm_info)
{
    rtp_info_t * rtp_info = NULL, last_rtp_info;
    rtp_trans_t * trans = NULL;
    int ret;

    if (rtp_sessn == NULL || buf == NULL || strm_info == NULL)
    {
        rsrv_log(AV_LOG_WARNING, "vstream buf is NULL\n");
        return ERR_INPUT_PARAM;
    }

    trans = &rtp_sessn->trans;

    rtp_info = _get_rtpinfo(rtp_sessn, M_AUDIO);
    if (rtp_info == NULL)
    {
        rsrv_log(AV_LOG_ERROR, "get audio rtpinfo error!\n");
        return ERR_INPUT_PARAM;
    }

    /*if (trans->audio_blocked > 0)
    {
        if (trans->last_buf_size == buf_size && trans->last_buf_ptr == buf)
        {
            last_rtp_info = *rtp_info;

        }
        trans->audio_blocked = 0;
    }*/

    last_rtp_info = *rtp_info;

    _get_audio_rtp_pack(rtp_info, strm_info->ts);

    if (trans->type == RTP_RTP_AVP)
    {
        _send_udp_data(rtp_sessn->trans.rtp_udp_afd, (rtp_info->hdr_data + 4), rtp_info->hdr_len, buf, buf_size);
    }
    else if (trans->type == RTP_RTP_AVP_TCP)
    {
        _add_interhead(rtp_info->hdr_data, (rtp_info->hdr_len + buf_size), AUDIO_RTP);
        rtp_info->hdr_len += 4;
        ret = _send_tcp_data(trans, rtp_info->hdr_data, rtp_info->hdr_len, buf, buf_size);
        if (ret < 0)
        {
            if (trans->blocked > 0)
            {
                rsrv_log(AV_LOG_WARNING, "[fd:%d buf:%p]\n", trans->tcp_fd, buf);
                trans->audio_blocked = 1;
                trans->blocked = 0;

                trans->last_buf_ptr = buf;
                trans->last_buf_size = buf_size;

                *rtp_info = last_rtp_info;
                rtp_sessn->again_count++;
            }
            return ret;
        }
        trans->audio_blocked = 0;
        rtp_sessn->ainfo.octet_count += ret;
    }
    return 0;
}

int _send_tstream(rtp_sessn_t * rtp_sessn, uint8 * buf, uint32 buf_size, strm_buf_t  strm_info)
{
    rtp_hdr_t rtp_header;
    uint8 * p = NULL;
    rtp_info_t * rtp_info = NULL;
    uint32 itimestamp;

    memset(&rtp_header, 0x0, sizeof(rtp_hdr_t));
    if (rtp_sessn->trans.type != RTP_RTP_AVP_TCP)
        return 0;

    rtp_info = _get_rtpinfo(rtp_sessn, M_TEXT);
    rtp_header.version = rtp_info->rtp_hdr.version;
    rtp_header.payload = rtp_info->rtp_hdr.payload;
    rtp_header.marker  = 1;
    p = (uint8 *)&rtp_header;

    p[2] = (rtp_info->rtp_hdr.seq_no >> 8) & 0xff;
    p[3] = rtp_info->rtp_hdr.seq_no & 0xff;


    itimestamp = strm_info.ts;
    p[4] = (itimestamp >> 24) & 0xff;
    p[5] = (itimestamp >> 16) & 0xff;
    p[6] = (itimestamp >> 8) & 0xff;
    p[7] = itimestamp & 0xff;

    p[8]  = (rtp_info->rtp_hdr.ssrc >> 24) & 0xff;
    p[9]  = (rtp_info->rtp_hdr.ssrc >> 16) & 0xff;
    p[10] = (rtp_info->rtp_hdr.ssrc >> 8) & 0xff;
    p[11] = rtp_info->rtp_hdr.ssrc & 0xff;

    _add_interhead((uint8 *)rtp_info->hdr_data, sizeof(rtp_hdr_t) + buf_size, rtp_sessn->trans.tcp_tinfo.interleaved.rtp);

    memcpy((rtp_info->hdr_data + 4), &rtp_header, sizeof(rtp_hdr_t));
    rtp_info->hdr_len = sizeof(rtp_hdr_t) + 4;

    if (rtp_sessn->trans.type == RTP_RTP_AVP_TCP)
    {
        _send_tcp_data(&rtp_sessn->trans, rtp_info->hdr_data, rtp_info->hdr_len, buf, buf_size);
    }
    rtp_info->rtp_hdr.seq_no++;
    return 0;
}

/* 数据回调前做发送统计 */
static sint32 _rstrm_data_cb(sint32 ch_idx, sint32 pri_data, char * full_data, sint32 size)
{
    rtsp_sessn_t * rss = (rtsp_sessn_t *)pri_data;
    MSHEAD * pmshead = NULL;
    uint8 * data = (uint8 *)full_data;
    rtp_sessn_t * rtp_sessn = NULL;
    sint32 ret = 0, data_size = 0, data_size_left = 0;
    sint32 frame_width, frame_height;
    sint32 enc_type, time_sec = 0, time_msec = 0;
    chstrm_type_e stm_type;
    strm_buf_t  strm_info = {0};
    rtp_trans_t * trans = NULL;
    sint64 snd_update_time;

    if ((NULL == rss) || (NULL == full_data) || size <= 0)
    {
        rsrv_log(AV_LOG_ERROR, "size <= 0\n");
        return ERR_INPUT_PARAM;
    }

    snd_update_time = (av_gettime_relative() - rss->last_snd_time) / 1000;
    rss->max_snd_time = (snd_update_time > rss->max_snd_time) ? snd_update_time : rss->max_snd_time;

    rtp_sessn = &rss->rtp_sessn;
    stm_type = rss->stm_type;
    trans = &rtp_sessn->trans;

    data_size_left = size;

    while (data_size_left > 0)
    {
        pmshead = (MSHEAD *)data;
        data_size = MSHEAD_GETMSDSIZE(pmshead);
        data = (uint8 *)MSHEAD_DATAP(pmshead);

        if (trans->audio_blocked > 0)
        {
            if ((data + data_size) < trans->last_buf_ptr)
            {
                /* 音频发送没有阻塞的情况下才发送视频，否则说明上次视频发送成功，音频发送阻塞，需要跳过视频帧*/
                goto SKIP_DATA;
            }
            else
            {
                rsrv_log(AV_LOG_WARNING, "data = %p data_size = %d last_buf_ptr = %p\n", data, data_size, trans->last_buf_ptr);
            }
        }

        if (ISVIDEOFRAME(pmshead))
        {
            rss->cvfr_cnt++;
            rss->cvfr_cnt_sec++;
            rss->serial = MSHEAD_GET_VIDEO_FCOUNT(pmshead);
            frame_width = MSHEAD_GET_VIDEO_WIDTH(pmshead);
            frame_height = MSHEAD_GET_VIDEO_HEIGHT(pmshead);

            if (pmshead->algorithm == MSHEAD_ALGORITHM_VIDEO_H265_HISILICON ||
                    pmshead->algorithm == MSHEAD_ALGORITHM_VIDEO_H265_GENERAL)
            {
                enc_type = VIDEO_H265;
            }
            else if (pmshead->algorithm == MSHEAD_ALGORITHM_VIDEO_H264_HISILICON ||
                     pmshead->algorithm == MSHEAD_ALGORITHM_VIDEO_H264_STANDARD)
            {
                enc_type = VIDEO_H264;
            }

            if (ISKEYFRAME(pmshead))
            {
                rss->cifr_cnt++;
            }

            if (rss->last_enc_type == -1)
            {
                rss->last_enc_type = enc_type;
            }
            else if (rss->last_enc_type != enc_type)
            {
                return ERR_EOF;
            }

            if (rss->last_frame_width == -1)
            {
                rss->last_frame_width = frame_width;
            }
            else if (rss->last_frame_width != frame_width)
            {
                return ERR_EOF;
            }

            if (rss->last_frame_height == -1)
            {
                rss->last_frame_height = frame_height;
            }
            else if (rss->last_frame_height != frame_height)
            {
                return ERR_EOF;
            }

            rss->video_pts = av_rescale_q(rss->cvfr_cnt, AV_TIME_BASE_PAL, AV_TIME_BASE_VIDEO);
            rss->video_cur_timestamp = rss->video_base_timestamp + rss->video_pts;
            trans->media_type = M_VIDEO;
            trans->video_seqno = MSHEAD_GET_VIDEO_FCOUNT(pmshead);
            MSHEAD_GET_TIMESTAMP(pmshead, time_sec, time_msec);

            /*time_gap = (time_sec * 1000 + time_msec) - (rss->time_sec * 1000 + rss->time_msec);
            av_log(NULL, AV_LOG_WARNING, "[fd:%d]time_gap = %d\n", rss->fd, time_gap);*/

            rss->time_sec = time_sec;
            rss->time_msec = time_msec;
            /*strm_info.ts = (uint32)(90000 * ((time_sec * 1000 + time_msec) / 1000.0) + 0.5);*/
            strm_info.ts = rss->video_cur_timestamp;
            strm_info.enc_type = enc_type;  /*rtspsrv_get_enc_type(ch_idx, stm_type);*/

            ret = _send_vstream(rtp_sessn, data, data_size, &strm_info);
            rss->left_size = trans->wv_info.piov_size;
            if (ret < 0)
            {
                return ret;
            }            
            rss->last_snd_time = av_gettime_relative();
            rss->update_time = (av_gettime_relative() - rss->update_time_start) / 1000;            
            rtp_sessn->vinfo.packet_count = rss->cvfr_cnt;
            if(!rss->left_size)
            {
                _process_rtcp_data(&rtp_sessn->vinfo, trans->tcp_fd);
            }
        }
        else if (ISAUDIOFRAME(pmshead))
        {
            if (trans->audio_blocked == 0)
            {
                rss->cafr_cnt++;
            }

            trans->media_type = M_AUDIO;
            /*strm_info.ts = rss->base_timestamp + rss->cafr_cnt * 320;*/
            strm_info.media_type = M_AUDIO;
            rss->audio_pts = av_rescale_q(rss->cafr_cnt * data_size, AV_TIME_BASE_AUDIO, AV_TIME_BASE_AUDIO);
            rss->audio_cur_timestamp = rss->audio_base_timestamp + rss->audio_pts;
            strm_info.ts = rss->audio_cur_timestamp;

            ret = _send_astream(rtp_sessn, data, data_size, &strm_info);
            rss->left_size = trans->wv_info.piov_size;
            if (ret < 0)
            {
                return ret;
            }
            rtp_sessn->ainfo.packet_count = rss->cafr_cnt;

            /*if(!rss->left_size)
            {
                _process_rtcp_data(&rtp_sessn->ainfo, trans->tcp_fd);
            }*/
        }
        else
        {
            rsrv_log(AV_LOG_WARNING, "text size = %d\n", data_size);

            rtp_sessn->trans.media_type = M_TEXT;

            if ((stm_type == MAIN_STREAM) && (rtp_sessn->media_type & 0x04))
            {
                ret = _send_tstream(rtp_sessn, data, size, strm_info);
                if (ret < 0)
                {
                    return ret;
                }
            }
        }
SKIP_DATA:
        data += data_size;
        data_size_left -= (data_size + MSHEAD_GETMSHSIZE(pmshead));
    }

    return 0;
}

static sint32 _rstrm_fifo_read(rtsp_sessn_t * rss, sint32 ch_idx)
{
    MSHEAD mshead = {0};
    sint32 link_stat = 0, size = 0, count = 0, data_size = 0;
    sint32 ret = 0, consumer, fifo;

    if (rss == NULL)
    {
        rsrv_log(AV_LOG_WARNING, "rss is NULL\n");
        return ERR_INPUT_PARAM;
    }

    fifo = rss->fifo;
    consumer = rss->consumer;

    while (count++ < 10)
    {
        if (rss->frame_len > 0 && rss->frame_buf != NULL)
        {
            size = rss->frame_len;
            goto PROCESS_BUF_FRAME;
        }

        if ((size = fifo_readmulti(fifo, consumer, (char *)&mshead, sizeof(MSHEAD), FIFO_PEEK_NO)) <= 0)
        {
            /*av_log(NULL, AV_LOG_WARNING, "fifo_readmulti %x\n", ret);*/
            break;
        }

        fifo_ioctrl(fifo, FIFO_CMD_GETDATASIZE, consumer, &data_size, sizeof(sint32));
        if (data_size < MSHEAD_GETFRAMESIZE(&mshead)) /* FIFO数据不足一帧,不再循环 */
        {
            break;
        }

        /*av_log(NULL, AV_LOG_WARNING, "[fd:%d id:%x con:%d size:%d]\n",rss->fd, rss->sessn_id, consumer, data_size);*/

        if (rss->ciframe && (0 == ISKEYFRAME(&mshead))) /* 需要发送I帧,但数据不是I帧,丢弃该帧数据 */
        {
            if ((size = fifo_readframemulti(fifo, consumer, NULL, MSHEAD_GETFRAMESIZE(&mshead), FIFO_PEEK_YES)) < 0)
            {
                rsrv_log(AV_LOG_WARNING, "fifo_readframemulti %x\n", ret);
            }
            /*av_log(NULL, AV_LOG_WARNING, "[fd:%d] drop-P size = %d\n", rss->fd, size);*/
            rss->drop_frame++;
        }
        else
        {
            if ((ret = fifo_frameprocessmulti(fifo, consumer, _rstrm_data_cb, ch_idx, (sint32)rss)) > 0)
            {
                rss->ciframe = HB_FALSE;
            }
            else if (ret == 0)
            {
                fifo_ioctrl(fifo, FIFO_CMD_GETLINKSTAT, consumer, &link_stat, sizeof(sint32));
                if (link_stat)
                {
                    rss->frame_len = MSHEAD_GETFRAMESIZE(&mshead);
#ifdef _RTSPSRV_TEST
                    rss->frame_buf = av_malloc(rss->frame_len);
#else
                    rss->frame_buf = mem_manage_malloc(COMP_RTSPSRV, rss->frame_len);
#endif
                    if (rss->frame_buf)
                    {
                        if ((size = fifo_readframemulti(fifo, consumer, rss->frame_buf, rss->frame_len, FIFO_PEEK_YES)) > 0)
                        {
PROCESS_BUF_FRAME:
                            ret = _rstrm_data_cb(ch_idx, (sint32)rss, rss->frame_buf, size);
                            if (ret == AVERROR(EAGAIN))
                            {
                                continue;
                            }
                            else if (ret < 0)
                            {
                                return ret;
                            }
                        }
#ifdef _RTSPSRV_TEST
                        av_free(rss->frame_buf);
#else
                        mem_manage_free(COMP_RTSPSRV, rss->frame_buf);
#endif
                        rss->frame_buf = NULL;
                        rss->frame_len = 0;
                    }
                    else
                    {
                        if ((size = fifo_readframemulti(fifo, consumer, NULL, MSHEAD_GETFRAMESIZE(&mshead), FIFO_PEEK_YES)) > 0)
                        {
                            rss->ciframe = HB_TRUE;
                        }
                        rss->frame_len = 0;
                    }
                }
            }
            else if (ret == AVERROR(EAGAIN))
            {
                continue;
            }
            else
            {
                rsrv_log(AV_LOG_WARNING, "[fd:%d] low_read frameprocess 0x%x\n", rss->fd, ret);
                return ret;
            }
        }
    }

    if (ERRNO(FIFO_ERROR_MSHEAD, COMP_FIFO) == (uint32)size)
    {
        rsrv_log(AV_LOG_ERROR, "ch(%d) fifo(0x%x) consumer(%d) FIFO RESET Because FIFO_ERROR_MSHEAD\n", ch_idx, fifo, consumer);
        if (0 > fifo_ioctrl(fifo, COMMON_CMD_RESET, consumer, NULL, 0))
        {
            rsrv_log(AV_LOG_ERROR, "ch(%d) fifo(0x%x) consumer(%d) FIFO RESET ERROR\n", ch_idx, fifo, consumer);
        }
    }

    return 0;
}

/* fifo正常低水位的情况下,所有帧都发送 */
static sint32 _rstrm_fifo_low_read(rtsp_sessn_t * rss, sint32 ch_idx)
{
    sint32 ret = 0;

    if ((ret = _rstrm_fifo_read(rss, ch_idx)) < 0)
    {
        rsrv_log(AV_LOG_WARNING, "_rstrm_fifo_low_read %x\n", ret);
        return ret;
    }

    return 0;
}

/* fifo中水位的情况下,所有帧都发送 */
static sint32 _rstrm_fifo_middle_read(rtsp_sessn_t * rss, sint32 ch_idx)
{
    sint32 ret = 0;

    if ((ret = _rstrm_fifo_read(rss, ch_idx)) < 0)
    {
        rsrv_log(AV_LOG_WARNING, "_rstrm_fifo_middle_read %x\n", ret);
        return ret;
    }

    return 0;
}

/* fifo高水位的情况下,只发I帧,其他帧丢弃,10s高水位比会增加 */
static sint32 _rstrm_fifo_high_read(rtsp_sessn_t * rss, sint32 ch_idx)
{
    MSHEAD mshead = {0};
    sint32 size = 0, ret = 0, link_stat = 0, water = 0, count = 0;
    sint32 consumer, fifo;

    if (rss == NULL)
    {
        rsrv_log(AV_LOG_WARNING, "high_read rss is null\n");
        return ERR_INPUT_PARAM;
    }

    fifo = rss->fifo;
    consumer = rss->consumer;

    while (count++ < 20)
    {
        if (rss->frame_len > 0 && rss->frame_buf != NULL)
        {
            size = rss->frame_len;
            goto PROCESS_BUF_FRAME;
        }

        if ((ret = fifo_readmulti(fifo, consumer, (char *)&mshead, sizeof(MSHEAD), FIFO_PEEK_NO)) < 0)
        {
            PERROR(ret);
            rsrv_log(AV_LOG_WARNING, "fifo_readmulti %x\n", ret);
            break;
        }
        if (ISKEYFRAME(&mshead))
        {
            if ((ret = fifo_frameprocessextmulti(fifo, consumer, _rstrm_data_cb, ch_idx, (sint32)rss, 0)) > 0)
            {
                rss->ciframe = HB_FALSE;
            }
            else if (0 == ret)
            {
                fifo_ioctrl(fifo, FIFO_CMD_GETLINKSTAT, consumer, &link_stat, sizeof(sint32));
                if (link_stat)
                {
                    rss->frame_len = MSHEAD_GETFRAMESIZE(&mshead);
#ifdef _RTSPSRV_TEST
                    rss->frame_buf = av_malloc(rss->frame_len);
#else
                    rss->frame_buf = mem_manage_malloc(COMP_RTSPSRV, rss->frame_len);
#endif

                    if (rss->frame_buf)
                    {
                        if ((size = fifo_readframemulti(fifo, consumer, rss->frame_buf, rss->frame_len, FIFO_PEEK_YES)) > 0)
                        {
PROCESS_BUF_FRAME:
                            ret = _rstrm_data_cb(ch_idx, (sint32)rss, rss->frame_buf, size);
                            if (ret == AVERROR(EAGAIN))
                            {
                                continue;
                            }
                            else if (ret < 0)
                            {
                                return ret;
                            }
                        }
#ifdef _RTSPSRV_TEST
                        av_free(rss->frame_buf);
#else
                        mem_manage_free(COMP_RTSPSRV, rss->frame_buf);
#endif
                        rss->frame_buf = NULL;
                        rss->frame_len = 0;
                    }
                    else
                    {
                        if ((size = fifo_readframemulti(fifo, consumer, NULL, MSHEAD_GETFRAMESIZE(&mshead), FIFO_PEEK_YES)) > 0)
                        {
                            rss->ciframe = HB_TRUE;
                        }
                        rss->frame_len = 0;
                    }
                }
            }
            else if (ret == AVERROR(EAGAIN))
            {
                continue;
            }
            else
            {
                rsrv_log(AV_LOG_WARNING, "[fd:%d] high_read frameprocess 0x%x\n", rss->fd, ret);
                return ret;
            }

            fifo_ioctrl(fifo, FIFO_CMD_GETCURWATER, consumer, &water, sizeof(sint32));
            if (water >= FIFO_WATER_HIGH) /* 如果还是高水位,把后面的P帧丢了 */
            {
                sint32 nexttype = 0;

                size = fifo_readextmulti(fifo, consumer, (char *)NULL, _DISCARD_DATA_SIZE, &nexttype);
                rss->ciframe = HB_TRUE;
                /*av_log(NULL, AV_LOG_WARNING, "[fd:%d] high_read %d\n", rss->fd, size);*/
            }
            break;
        }
        else
        {
            if ((size = fifo_readframemulti(fifo, consumer, (char *)NULL,
                                            MSHEAD_GETFRAMESIZE(&mshead), FIFO_PEEK_YES)) <= 0)
            {
                break;
            }
            rss->ciframe = HB_TRUE;
        }

    }

    return 0;
}

/* FIFO最高水位的情况下,为防止影响其他通道,直接丢到低水位 */
static sint32 _rstrm_fifo_highest_read(rtsp_sessn_t * rss, sint32 ch_idx)
{
    sint32 water = FIFO_WATER_LOW, size = 0, count = 0;
    sint32 consumer, fifo;

    if (rss == NULL)
    {
        rsrv_log(AV_LOG_WARNING, "highest_read rss is null\n");
        return ERR_INPUT_PARAM;
    }

    fifo = rss->fifo;
    consumer = rss->consumer;

    while (count++ < 30)
    {
        fifo_ioctrl(fifo, FIFO_CMD_GETCURWATER, consumer, &water, sizeof(sint32));
        if (water == FIFO_WATER_LOW)
        {
            break;
        }
        if ((size = fifo_readframemulti(fifo, consumer, (char *)NULL, _DISCARD_DATA_SIZE, FIFO_PEEK_YES)) <= 0)
        {
            break;
        }
        rss->ciframe = HB_TRUE;
        rss->high_water_cnt++;
        /*av_log(NULL, AV_LOG_WARNING, "[fd:%d] highest_read %d\n", rss->fd, size);*/
    }

    return 0;
}

static int _process_rtcp_data(rtp_info_t * rinfo, int fd)
{
    int rtcp_bytes, snd_bytes = 0;

    rtcp_bytes = ((rinfo->octet_count - rinfo->last_octet_count) * RTCP_TX_RATIO_NUM) / RTCP_TX_RATIO_DEN;

    if ((rinfo->first_packet || ((rtcp_bytes >= RTCP_SR_SIZE) &&
                                 (rsrv_ntp_time() - rinfo->last_rtcp_ntp_time > 5000000))))
    {
        snd_bytes = _rtcp_send_sr(rinfo, fd, rsrv_ntp_time());
        rinfo->last_octet_count = rinfo->octet_count;
        rinfo->first_packet = 0;
    }

    return snd_bytes;
}

int _process_fifo_data(rtsp_sessn_t * rss, sint32 ch_idx)
{
    sint32 ret = 0, consumer, fifo;
    sint32 fr_cnt = 0, water = 0;

    if (rss == NULL)
    {
        return -1;
    }

    fifo = rss->fifo;
    consumer = rss->consumer;

    rss->update_time_start = av_gettime_relative();

    ret = fifo_ioctrl(fifo, FIFO_CMD_GETVFRAMECOUNT, consumer, &fr_cnt, sizeof(sint32));
    if (ret < 0)
    {
        rsrv_log(AV_LOG_WARNING, "GETVFRAMECOUNT %x\n", ret);
        return ret;
    }

    ret = fifo_ioctrl(fifo, FIFO_CMD_GETCURWATER, consumer, &water, sizeof(sint32));
    if (ret < 0)
    {
        rsrv_log(AV_LOG_WARNING, "GETCURWATER %x\n", ret);
        return ret;
    }

    if (fr_cnt >= 1 || water >= FIFO_WATER_MIDDLE)
    {
        switch (water)
        {
            case FIFO_WATER_LOW:
                ret = _rstrm_fifo_low_read(rss, ch_idx);
                break;
            case FIFO_WATER_MIDDLE:
                ret = _rstrm_fifo_middle_read(rss, ch_idx);
                break;
            case FIFO_WATER_HIGH:
                ret = _rstrm_fifo_high_read(rss, ch_idx);
                break;
            case FIFO_WATER_HIGHEST:
                ret = _rstrm_fifo_highest_read(rss, ch_idx);
                break;
            default:
                rsrv_log(AV_LOG_ERROR, "get water err, water=%d\n", water);
                return ERR_NOT_FOUND;
        }
    }
    return 0;
}


int rstrm_send_data(rtsp_srv_t * rs_srv, rtsp_sessn_t * rss, int ch_idx)
{
    int ret = 0;
    sint64 average_time, relative_time;
    strm_buf_t strm_info;
    chstrm_type_e stm_type;
    rtp_sessn_t * rtp_sessn = NULL;
    /*sint32 data_size = 0;*/

    if (rs_srv == NULL || rss == NULL)
    {
        rsrv_log(AV_LOG_WARNING, "rstrm_send_data rss is null\n");
        return ERR_INPUT_PARAM;
    }

    rtp_sessn = &rss->rtp_sessn;

    stm_type = rss->stm_type;

    if (stm_type < 0 || stm_type > CH_STRM_MAX)
    {
        rsrv_log(AV_LOG_WARNING, "stm_type is illegal: %d\n", stm_type);
        return ERR_INPUT_PARAM;
    }

    relative_time = av_gettime_relative();
    
    if (!rss->calc_time)
    {
        rss->calc_time = relative_time;
        rtp_sessn->octet_count = 0;
        rss->cvfr_cnt_sec = 0;
    }
    else if ((relative_time - rss->calc_time) > TEN_SECONDS)
    {
        rss->cbitrate = (rtp_sessn->octet_count >> 10) * 8 / 10;
        rss->calc_time = relative_time;
        rtp_sessn->octet_count = 0;
        rss->fps = rss->cvfr_cnt_sec / 10;
        rss->cvfr_cnt_sec = 0;
    }

    rss->total_time += rss->update_time;
    rss->total_count++;

    if (rss->total_count >= 3000)
    {
        average_time = rss->total_time / rss->total_count;
        if (average_time > 50)
        {
            rsrv_log(AV_LOG_WARNING, "[fd:%d strm:%d] average_time:%lld exceed 50ms\n", rss->fd, rss->stm_type, average_time);
        }

        /*if (snd_update_time > 1000)
        {
            rsrv_log(AV_LOG_WARNING, "[fd:%d]rsrv frame update too late:%llds\n", rss->fd, snd_update_time / 1000);
        }*/
        rss->total_count = 0;
        rss->total_time = 0;
    }

    /*fifo = rs_srv->fifos[ch_idx][stm_type];
    fifo_ioctrl(rss->fifo, FIFO_CMD_GETDATASIZE, rss->consumer, &data_size, sizeof(sint32));
    av_log(NULL, AV_LOG_WARNING, "[fd:%d id:%x con:%d size:%d]\n",rss->fd, rss->sessn_id, rss->consumer, data_size);*/    

    if (rss->heart_beat == 1)
    {
        rsrv_reply_heart(rss);
        rss->heart_beat = 0;
    }

    memset(&strm_info, 0x0, sizeof(strm_info));

    strm_info.media_type = rtp_sessn->trans.media_type;

    ret = _process_fifo_data(rss, ch_idx);
    if (ret < 0)
    {
        return ret;
    }

    return 0;
}

