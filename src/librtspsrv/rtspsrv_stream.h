//////////////////////////////////////////////////////////////////////////////
// 版权所有，2009-2014，北京汉邦高科数字技术有限公司
// 本文件是未公开的，包含了汉邦高科的机密和专利内容
////////////////////////////////////////////////////////////////////////////////
// 文件名： send_av_data.h
// 作者：   杨朋辉
// 版本：   1.0
// 日期：   2014-07-15
// 描述：   音视频数据发送程序头文件
// 历史记录：
////////////////////////////////////////////////////////////////////////////////

#ifndef _SEND_AV_DATA_H
#define _SEND_AV_DATA_H

#include <defines.h>

#define TEMP_FRAME_MAX    1000

typedef struct
{
    char text_send_buff[512];
    int  size;
} TTEXT_DATA_INFO;

typedef struct _stream_buf_info
{
    uint32 frame_start;
    uint32 frame_len;
    uint32 frame_end;
    uint32 frame_type;
    uint32 frame_seqno;
    uint32 total_nal;
    uint32 nal_len[10];
    uint32 ts;
    uint32 enc_type;
    rtpmd_type_e media_type;
} strm_buf_t;

typedef struct _stream_media
{
    uint32 TEMP_BUF_SZ;  //streambuffer size
    uint32 SAFE_BUF_SZ;
    uint32 write;			//current write pos
    uint32 frame_index;
    uint32 frame_sqno;	//current frame sqno
    strm_buf_t strm_info[TEMP_FRAME_MAX];  //record gstreambuffer frame info content start,end,type
    uint8 * buf;		 //malloc buffer one stream
} strm_media_t;

/* 需要注意生产者通道与消费者通道的管理方式与区别 */
/* 消费者通道结构体 consumer channel */
typedef struct _strm_cch
{
    uint8 enable;       /* 通道是否开启 */
    uint8 expect_en;    /* 通道被期望开启或滚比,防止不同步带来问题 */
    uint8 sync_flag;    /* 同步标志,open_stream返回后,才能回调数据,否则返回前回调的数据会被上层丢掉 */
    uint8 ciframe;      /* 等待发送I帧标志 */
    sint32 count;       /* 开流计数 */

    sint32 cbitrate;    /* NETMS_TIME_INTERVAL平均码率,bit/s */
    sint32 cbyte_cnt;   /* 每秒数据量累计,为统计平均码率提供数据 */
    sint32 cbyte_ti;    /*  */
    sint32 cexp_br;     /* 统计临时数据 */
    uint32 serial;      /* 视频序列号 */

    sint32 cifr_cnt;    /* 回调的i帧总数 */
    sint32 cvfr_cnt;    /* 回调的p帧总数 */
    sint32 cafr_cnt;    /* 音频不好统计,音频都连在视频后面,结构保留 */
} strm_cch_t;



void rtsp_add_interhead(uint8 * rtp_buf, uint16 rtp_size, uint32 channel_id);

///////////////////////////////////////////////////////////////////////////////
// RTP方式发送数据时TCP头中的通道ID
///////////////////////////////////////////////////////////////////////////////
typedef enum _tagCHANNEL_ID
{
    VIDEO_RTP = 0, //RTP视频通道ID
    VIDEO_RTCP,	   //RTCP视频通道ID
    AUDIO_RTP,     //RTP音频通道ID
    AUDIO_RTCP     //RTCP音频通道ID
} CHANNEL_ID_E;


int rstrm_send_data(rtsp_srv_t * rs_srv, rtsp_sessn_t * rss, int ch_idx);

#endif

