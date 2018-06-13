/*
 * FileName:       rtp.h
 * Author:         luny  Version: 1.0  Date: 2017-5-16
 * Description:
 * Version:
 * Function List:
 *                 1.
 * History:
 *     <author>   <time>    <version >   <desc>
 */

#ifndef _RTPH
#define _RTPH

#ifndef _WIN32
#include <sys/types.h>
#include <sys/socket.h>
#endif

#include <defines.h>
#include "rtspsrv_session.h"

#define RTP_HEADER_SIZE     	 (32)       //大于12的值
#define NAL_HEADER_SIZE          (4)        //00000001

#define READ_AUDIO_DATA_LEN      (320)     //每次读取的音频数据的长度
#define VIDEO_STREAM_BUFFER_SIZE (1024<<10) //视频数据缓冲长度，单个帧长度最大不超过此值
#define AUDIO_STREAM_BUFFER_SIZE (READ_AUDIO_DATA_LEN + RTP_HEADER_SIZE)     //音频数据缓冲的长度

#define RTP_VERSION       2
#define MAXRTPPACKSIZE    1460 //1448 //1460 //1444  1500(max) - 20(IP header) - 20(TCP header) - 12or14(RTP header)
#define PAYLOAD_TYPE_H264 96
#define PAYLOAD_TYPE_PCMA 8
#define PAYLOAD_TYPE_H265 96
#define PAYLOAD_TYPE_TEXT 98

#define RTCP_TX_RATIO_NUM 5
#define RTCP_TX_RATIO_DEN 1000
#define RTCP_SR_SIZE 28
#define RTCP_RR_LEN 52

#define MAX_RTP_PACKET_SIZE (1448) //1500(max) - 20(IP header) - 20(TCP header) - 12or14(RTP header) 

/*// RTP和RTCP协议的定义*/
typedef enum
{
    rtp_proto = 0,
    rtcp_proto
} rtp_protos;

/*// RTP传输类型的定义*/
typedef enum _rtp_type
{
    RTP_NO_TRANSPORT = 0,
    RTP_RTP_AVP,    //UDP
    RTP_RTP_AVP_TCP //TCP
} rtp_type_e;

/* RTP和RTCP端口对结构体 4bytes */
typedef struct
{
    uint16 rtp;
    uint16 rtcp;
} port_pair_t;

/* NAL信息结构体 16bytes */
typedef struct _nal_msg
{
    uint8 eof; /* 是否是EOF */
    uint8 type; /* NAL类型 */
    uint8 reserve[2];
    uint8 * start; /* 起始地址指针 */
    uint8 * end; /* 结束地址指针 */
    uint32 size; /* 数据的大小 */
} nal_msg_t;

/* RTP头定义结构体 16bytes */
typedef struct _rtp_header
{
    /* byte 0 */
    uint8 csrc_len: 4; /* CSRC计数 */
    uint8 extension: 1; /* 扩展位 */
    uint8 padding: 1;  /* 填充位 */
    uint8 version: 2;  /* 版本号 */

    /* byte 1 */
    uint8 payload: 7; /* 负载类型 */
    uint8 marker: 1; /* 最后一包数据的标识位 */

    /* bytes 2, 3 */
    uint16 seq_no;  /* 序列号 */
    /* bytes 4-7 */
    uint32 timestamp; /* 时间戳 */
    /* bytes 8-11 */
    uint32 ssrc; /* SSRC，区分是哪一路RTP流 */

    /* bytes 12-16 */
    uint32 reserve;
} rtp_hdr_t;

/* RTP信息定义结构体 128bytes */
typedef struct _rtp_info
{
    nal_msg_t nal;    /* NAL信息 16bytes */
    rtp_hdr_t rtp_hdr;/* RTP头 16bytes */
    uint32 hdr_len;       /* RTP头的长度 */
    uint8 hdr_data[20];

    uint32 s_bit;    /* 开始位 */
    uint32 e_bit;    /* 结束位 */
    uint32 fu_flag;  /* 是否是采用FU打包的标识 */
    uint32 nal_begin; /* 是否是一个NAL的开始的标识 */

    uint8 * pRTP; /* 指向RTP包地址的指针 */
    uint8 * start; /* 指向负载的起始地址的指针 */
    uint8 * end; /* 指向负载的结束地址的指针 */
    uint32 again_count;

    uint32 channel_id;
    uint32 ssrc;

    uint32 packet_count;
    uint32 octet_count;
    uint32 last_octet_count;
    sint32 first_packet;

    sint64 last_rtcp_ntp_time;	/* rtcp sender statistics */
    sint64 first_rtcp_ntp_time;
} rtp_info_t;

/* UDP信息定义结构体 24bytes */
typedef struct _udp_info
{
    struct sockaddr rem_addr;   //RTP客户端的网络地址
    port_pair_t cli_ports;      //客户端的端口对
    port_pair_t srv_ports;      //服务器端的端口对
} udp_info_t;

/* TCP信息定义结构体 4bytes */
typedef struct _tcp_info
{
    port_pair_t interleaved; //interleaved选项中指定的端口对
} tcp_info_t;

/* 发送缓存 24bytes */
typedef struct _writev_info
{
    uint32 piov_size; /* 缓冲区数据的长度 */
    uint32 send_size; /* 缓冲区已经发送的长度 */
    uint32 buf_size;
    uint8 * buf;
    uint32 frame_size;
    uint8 * frame_buf;
} writev_info_t;

/* RTP传输信息定义结构体 200bytes */
typedef struct _rtp_trans
{
    rtp_type_e type;      /* RTP传输类型 */
    uint32 resend_time; /* 重新发送的次数 */

    uint32 video_seqno;
    uint32 audio_seqno;
    uint32 media_type;

    uint32 blocked;       /* 上一次发送阻塞 */
    uint32 video_blocked;
    uint32 audio_blocked;
    uint32 text_blocked;
    uint32 end_frame;
    uint32 last_buf_size;
    uint8 * last_buf_ptr;
    sint32 deal_size;
    sint32 nal_size;
    sint32 nal_start;
    sint32 nal_end;
    uint8 * nal_ptr;

    sint32 tcp_fd;       /* TCP的网络文件描述符 */

    sint32 rtp_udp_vfd; /* UDP的视频网络文件描述符 */
    sint32 rtp_udp_afd; /* UDP的音频网络文件描述符 */
    sint32 rtp_udp_tfd; /* UDP的音频网络文件描述符 */

    sint32 rtcp_udp_vfd; /* UDP的视频网络文件描述符 */
    sint32 rtcp_udp_afd; /* UDP的音频网络文件描述符 */

    tcp_info_t tcp_info; /* TCP信息 */
    tcp_info_t tcp_tinfo; /* text_TCP信息 */

    udp_info_t udp_vinfo; /* 视频的UDP信息 */
    udp_info_t udp_ainfo; /* 音频的UDP信息 */
    udp_info_t udp_tinfo; /* 音频的UDP信息 */

    writev_info_t wv_info;
    uint32 reserve;
} rtp_trans_t;

/* RTP会话结构体 616bytes */
typedef struct _rtp_sessn
{
    rtp_trans_t trans;   /* RTP传输信息 200bytes*/
    rtp_info_t vinfo; /* 视频的RTP信息 128bytes*/
    rtp_info_t ainfo; /* 音频的RTP信息 */
    rtp_info_t tinfo; /* 音频的RTP信息 */

    uint32 media_type; /* 第一位代表视频，第二位代表音频，第三位代表文本，0代表不启用，1代表启用 */

    uint32 start_seq;     /* 起始的序列号 */
    uint32 start_rtptime; /* 起始的RTP时间戳 */

    uint32 started;	/* 是否开始音视频流传输的标识 */
    uint32 ssrc;	/* SSRC标识，区分是哪一路RTP流 */
    uint32 again_count;

    uint32 octet_count;
    uint32 reserve;
} rtp_sessn_t;


/* 通道RTP属性信息 400bytes */
typedef struct _rtp_ch_info
{
    uint32 en_audio; /* 音频使能 */
    audio_type_e atype; /* 音频编码类型 0:g711 */
    video_type_e vtype; /* 视频编码类型 0:h264 1:h265 */
    uint32 reserve;
    rtp_info_t mdstrms[MD_STRM_MAX]; /* media streams: 0 video 1 audio 2 text */
} rtp_chinfo_t;

/* RTCP packet types */
enum RTCPType
{
    RTCP_FIR = 192,
    RTCP_NACK, // 193
    RTCP_SMPTETC,// 194
    RTCP_IJ,   // 195
    RTCP_SR = 200,
    RTCP_RR,   // 201
    RTCP_SDES, // 202
    RTCP_BYE,  // 203
    RTCP_APP,  // 204
    RTCP_RTPFB,// 205
    RTCP_PSFB, // 206
    RTCP_XR,   // 207
    RTCP_AVB,  // 208
    RTCP_RSI,  // 209
    RTCP_TOKEN,// 210
};

/******************************************************************************
* 函数介绍: RTP信息初始化
* 输入参数: rs_srv: RTSP server 管理句柄
*				  ch_idx: 通道号
* 输出参数: 无
* 返回值  :
*****************************************************************************/
//int rtp_info_init(rtsp_srv_t * rs_srv, int ch_idx);

/******************************************************************************
* 函数介绍: 获取时间戳
* 输入参数: rs_srv: RTSP server 管理句柄
*			rss: RTSP session 实例
*			ch_idx: 通道号
* 输出参数: 无
* 返回值  : 返回获取到的时间戳
*****************************************************************************/
uint32 rtp_get_timestamp(int srv_fd, int type);

////////////////////////////////////////////////////////////////////////////////
// 函数名： rtp_set_timestamp
// 描  述： 设置时间戳
// 参  数： [in] _iType -- 主流、子流、文本流
//          [in] _iAVFlag -- 0视频 1音频
//          [in] time_usec -- 时间戳
// 返回值：
// 说  明：
////////////////////////////////////////////////////////////////////////////////
int rtp_set_timestamp(int srv_fd, int type, unsigned int time_usec);

uint8 * get_rtp_pack(rtp_info_t * rtp_info, uint8 * rtpHdr, uint32 data_type, uint32 * p_packet_size);

uint8 * get_audio_rtp_pack(rtp_info_t * rtp_info, uint32 ts);

int set_rtp_pack(rtp_info_t * rtp_info, uint32 enc_type, uint8 * nal_buf, uint32 nal_size, uint32 time_stamp, uint32 end_of_frame);


#define MAX_EAGAIN_TIMEOUT (30) //设置超时时间，单位秒

#endif


