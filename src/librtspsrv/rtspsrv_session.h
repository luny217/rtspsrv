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

/* RTSPͨ�������ṹ�� 16bytes */
typedef struct _rtsp_param
{
    uint32 bitrate;
    uint32 resolution;
    uint32 profile_id;
    uint32 reserve;
} rtsp_param_t;

/* �������ò��� 792bytes */
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

/* RTSP�������Ựʵ���ṹ�� 892bytes */
typedef struct _rtsp_session
{
    sint32 fd;   /* RTSP�������ļ������� */
    sint32 ep_fd;
    uint32 ch_idx;
    sint32 cli_id; /* cli������Ϣid */

    sint32 fps;
    uint32 ssrc;
    sint32 first_iframe;
    sint32 left_size;
    sint32 max_send_size;
    sint32 mshead_id;

    sint32 fifo;        /* fifo��� */
    sint32 consumer;    /* fifo�����߱�� */

    sint32 ac_handle;   /* active_mode����ģʽ��� */
    sint32 err_code;

    uint32 video_base_timestamp;
    uint32 video_cur_timestamp;
    uint32 video_timestamp;

    uint32 audio_base_timestamp;
    uint32 audio_cur_timestamp;
    uint32 audio_timestamp;

    uint32 ciframe;      /* �ȴ�����I֡��־ */
    sint32 cbitrate;    /* NETMS_TIME_INTERVALƽ������,bit/s */
    sint32 cvfr_cnt_sec;   /* ÿ���������ۼ�,Ϊͳ��ƽ��֡���ṩ���� */
    uint32 serial;      /* ��Ƶ���к� */

    sint32 cifr_cnt;    /* �ص���i֡���� */
    sint32 cvfr_cnt;    /* �ص���p֡���� */
    sint32 cafr_cnt;    /* ��Ƶ����ͳ��,��Ƶ��������Ƶ����,�ṹ���� */

    char * in_buf;     //RTSP����Ļ���
    sint32 in_size;     //RTSP����Ĵ�С
    char * out_buf;    //RTSPӦ��Ļ���
    sint32 out_size;    //RTSPӦ��Ĵ�С

    char * frame_buf;
    sint32 frame_len;

    uint32 cseq;        //RTSP��������к�
    uint32 sessn_id;    //RTSP�ỰID
    uint32 heart_beat;  //OPTION ����
    rtsp_state_e state;
    chstrm_type_e stm_type;

    uint32 timeout_cnt;
    sint32 try_snd_len;
    sint32 last_frame_width;    /* ��һ֡��� */
    sint32 last_frame_height;   /* ��һ֡�߶� */
    sint32 last_enc_type;       /* ��һ֡�������� */

    sint32 time_sec;    /* ���һ�ε���Ƶʱ��� */
    sint32 time_msec;

    sint32 low_water_cnt;   /* û�����ݼ��� */
    sint32 high_water_cnt;  /* ����������� */
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
    rtp_sessn_t rtp_sessn;  /* RTP�Ự�ṹ�� 608bytes */
} rtsp_sessn_t;

/* ����������ʱͨ�������Ϣ 1280bytes */
typedef struct _rsrv_chinfo
{
    sint32 ep_fd;                  /* ����������ͨ��epoll���, ����������epoll���ʹ��ep_fd[0] */
    struct epoll_event * ev;		/* ������epoll�¼�����ָ��, ����ͨ������ */
    n_slist_t * sessn_lst;         /* �Ự��������, ���ն�Ӧ��ͨ���ɶ��̹߳���, ������0��ʼ*/
    sint32 sessn_active;           /* ��ǰͨ���Ļ�Ự�� */
    sint32 reserve;				/* ���� */
    sint32 fifos[CH_STRM_MAX];

    rtsp_param_t ch_param[CH_STRM_MAX]; /**/
    rtp_chinfo_t rtp_chinfo[CH_STRM_MAX]; /* RTPͨ����Ϣ ͨ��, ���Ӵ����� */
} rsrv_chinfo_t;

/* �������ṹ�� 40bytes */
typedef struct _rtsp_server
{
    sint32 srv_fd;                          /* ����������socket��� */
    uint32 srv_port;                        /* �����������˿� */
    sint32 srv_ep_fd;                       /* ����������epoll���*/
    sint32 chn_num_max;                      /*֧�ֵ����ͨ����*/
    uint32 wkr_num;                          /*֧�ֵĹ����߳���,����CPU����������*/
    sint32 sessn_max;                       /* ��ǰ֧�ֵ����Ự�� */
    sint32 sessn_total;						/* ��ǰͨ���Ļ�Ự�� */
    sint32 state;

    void * cb_func;
    rsrv_chinfo_t * chi;
} rtsp_srv_t;

////////////////////////////////////////////////////////////////////////////////
// ��������remove_rtsp_session
// ��  ������RTSP������ɾ��һ��RTSP�Ự
// ��  ����[in] RTSP_session  RTSP�Ự
// ����ֵ���ɹ�����0��������-1
// ˵  ����
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// ��������rtsp_session_init
// ��  ����RTSP�������ʼ��
// ��  ����[in] socket_fd  RTSPͷ���������ļ�������
// ����ֵ���ɹ�����0��������-1
// ˵  ����
////////////////////////////////////////////////////////////////////////////////

int rsrv_sessn_init(rtsp_sessn_t * rss, int fd, int ep_fd, int ch_idx);

////////////////////////////////////////////////////////////////////////////////
// ��������rtsp_session_free
// ��  �����ͷ������RTSP������
// ��  ����[in] rtsp  RTSP�Ự
// ����ֵ����
// ˵  ����
////////////////////////////////////////////////////////////////////////////////

int rsrv_sessn_trip(rtsp_srv_t * rs_srv, int ch_idx);


#endif

