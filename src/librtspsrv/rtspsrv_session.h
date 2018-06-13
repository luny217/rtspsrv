/*
 * FileName:     rtspsrv_session.h
 * Author:         luny  Version: 1.0  Date: 2017-5-13
 * Description:
 * Version:
 * Function List:
 *                 1.
 * History:
 *     <author>   <time>    <version >   <desc>
 */

#ifndef _RTSPSRV_SESSION_H
#define _RTSPSRV_SESSION_H

#include "defines.h"
#include "rtspsrv_nlist.h"
#include "rtspsrv_defines.h"
#include "rtspsrv_rtp.h"

#define RSESSION_STATE_IDLE             (0)
#define RSESSION_STATE_DISPATCHED       (1)
#define RSESSION_STATE_CLOSED           (2)
#define RSESSION_STATE_CLOSING          (3)
#define RSESSION_STATE_INTERUPT         (4)
#define RSESSION_STATE_INTERACTION      (5)
#define RSESSION_STATE_STREAMING        (6)

typedef enum _rtsp_state
{
    RTSP_STATE_NONE = 0,
    RTSP_STATE_OPTION = 1,
    RTSP_STATE_DESCRIBER = 2,
    RTSP_STATE_SETUP = 3,
    RTSP_STATE_PLAY = 4,
    RTSP_STATE_PAUSE = 5,
    RTSP_STATE_TEARDOWN = 6,
    RTSP_STATE_SERVER_ERROR = 7
} rtsp_state_e;

/* RTSP通道参数结构体 16bytes */
typedef struct _rtsp_param
{
    uint32 bitrate;
    uint32 resolution;
    uint32 profile_id;
    uint32 reserve;
} rtsp_param_t;

/* 编码配置参数 792bytes */
typedef struct _enc_param
{
    sint32 enc_type;
    sint32 profile_id;
    sint32 vps_len;
    sint32 sps_len;
    sint32 pps_len;
    uint32 reserve;
    uint8  vps_buf[256];
    uint8  sps_buf[256];
    uint8  pps_buf[256];
} enc_param_t;

/* RTSP服务器会话实例结构体 892bytes */
typedef struct _rtsp_session
{
    sint32 fd;   /* RTSP的网络文件描述符 */
    sint32 ep_fd;
    uint32 ch_idx;
    sint32 cli_id; /* cli调试信息id */

    sint32 fps;
    uint32 ssrc;
    sint32 first_iframe;
    sint32 left_size;
    sint32 max_send_size;
    sint32 mshead_id;

    sint32 fifo;        /* fifo句柄 */
    sint32 consumer;    /* fifo消费者编号 */

    sint32 ac_handle;   /* active_mode主动模式句柄 */
    sint32 err_code;

    uint32 video_base_timestamp;
    uint32 video_cur_timestamp;
    uint32 video_timestamp;

    uint32 audio_base_timestamp;
    uint32 audio_cur_timestamp;
    uint32 audio_timestamp;

    uint32 ciframe;      /* 等待发送I帧标志 */
    sint32 cbitrate;    /* NETMS_TIME_INTERVAL平均码率,bit/s */
    sint32 cvfr_cnt_sec;   /* 每秒数据量累计,为统计平均帧率提供数据 */
    uint32 serial;      /* 视频序列号 */

    sint32 cifr_cnt;    /* 回调的i帧总数 */
    sint32 cvfr_cnt;    /* 回调的p帧总数 */
    sint32 cafr_cnt;    /* 音频不好统计,音频都连在视频后面,结构保留 */

    char * in_buf;     //RTSP请求的缓冲
    sint32 in_size;     //RTSP请求的大小
    char * out_buf;    //RTSP应答的缓冲
    sint32 out_size;    //RTSP应答的大小

    char * frame_buf;
    sint32 frame_len;

    uint32 cseq;        //RTSP请求的序列号
    uint32 sessn_id;    //RTSP会话ID
    uint32 heart_beat;  //OPTION 心跳
    rtsp_state_e state;
    chstrm_type_e stm_type;

    uint32 timeout_cnt;
    sint32 try_snd_len;
    sint32 last_frame_width;    /* 上一帧宽度 */
    sint32 last_frame_height;   /* 上一帧高度 */
    sint32 last_enc_type;       /* 上一帧编码类型 */

    sint32 time_sec;    /* 最近一次的视频时间戳 */
    sint32 time_msec;

    sint32 low_water_cnt;   /* 没有数据计数 */
    sint32 high_water_cnt;  /* 数据溢出计数 */
    sint32 drop_frame;
    sint8 remote_ip_str[16];
    sint32 remote_ip;
    uint32 remote_port;
    uint32 reserve;

    sint64 video_pts;
    sint64 audio_pts;
    sint64 update_time;    
    sint64 update_time_start;
    sint64 last_snd_time;
    sint64 max_snd_time;
    sint64 total_time;
    sint64 total_count;
    sint64 calc_time;
    rtp_sessn_t rtp_sessn;  /* RTP会话结构体 608bytes */
} rtsp_sessn_t;

/* 服务器运行时通道相关信息 1280bytes */
typedef struct _rsrv_chinfo
{
    sint32 ep_fd;                  /* 服务器各个通道epoll句柄, 服务器监听epoll句柄使用ep_fd[0] */
    struct epoll_event * ev;		/* 服务器epoll事件集合指针, 按照通道管理 */
    n_slist_t * sessn_lst;         /* 会话单向链表, 按照对应的通道由多线程管理, 计数从0开始*/
    sint32 sessn_active;           /* 当前通道的活动会话数 */
    sint32 reserve;				/* 保留 */
    sint32 fifos[CH_STRM_MAX];

    rtsp_param_t ch_param[CH_STRM_MAX]; /**/
    rtp_chinfo_t rtp_chinfo[CH_STRM_MAX]; /* RTP通道信息 通道, 主子次码流 */
} rsrv_chinfo_t;

/* 服务器结构体 40bytes */
typedef struct _rtsp_server
{
    sint32 srv_fd;                          /* 服务器监听socket句柄 */
    uint32 srv_port;                        /* 服务器监听端口 */
    sint32 srv_ep_fd;                       /* 服务器监听epoll句柄*/
    sint32 chn_num_max;                      /*支持的最大通道数*/
    uint32 wkr_num;                          /*支持的工作线程数,按照CPU核心数分配*/
    sint32 sessn_max;                       /* 当前支持的最大会话数 */
    sint32 sessn_total;						/* 当前通道的活动会话数 */
    sint32 state;

    void * cb_func;
    rsrv_chinfo_t * chi;
} rtsp_srv_t;

////////////////////////////////////////////////////////////////////////////////
// 函数名：remove_rtsp_session
// 描  述：从RTSP链表中删除一个RTSP会话
// 参  数：[in] RTSP_session  RTSP会话
// 返回值：成功返回0，出错返回-1
// 说  明：
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// 函数名：rtsp_session_init
// 描  述：RTSP链表结点初始化
// 参  数：[in] socket_fd  RTSP头结点的网络文件描述符
// 返回值：成功返回0，出错返回-1
// 说  明：
////////////////////////////////////////////////////////////////////////////////

int rsrv_sessn_init(rtsp_sessn_t * rss, int fd, int ep_fd, int ch_idx);

////////////////////////////////////////////////////////////////////////////////
// 函数名：rtsp_session_free
// 描  述：释放申请的RTSP链表结点
// 参  数：[in] rtsp  RTSP会话
// 返回值：无
// 说  明：
////////////////////////////////////////////////////////////////////////////////

int rsrv_sessn_trip(rtsp_srv_t * rs_srv, int ch_idx);


#endif

