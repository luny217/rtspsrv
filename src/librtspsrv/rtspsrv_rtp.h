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

#define RTP_HEADER_SIZE     	 (32)       //����12��ֵ
#define NAL_HEADER_SIZE          (4)        //00000001

#define READ_AUDIO_DATA_LEN      (320)     //ÿ�ζ�ȡ����Ƶ���ݵĳ���
#define VIDEO_STREAM_BUFFER_SIZE (1024<<10) //��Ƶ���ݻ��峤�ȣ�����֡������󲻳�����ֵ
#define AUDIO_STREAM_BUFFER_SIZE (READ_AUDIO_DATA_LEN + RTP_HEADER_SIZE)     //��Ƶ���ݻ���ĳ���

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

/*// RTP��RTCPЭ��Ķ���*/
typedef enum
{
    rtp_proto = 0,
    rtcp_proto
} rtp_protos;

/*// RTP�������͵Ķ���*/
typedef enum _rtp_type
{
    RTP_NO_TRANSPORT = 0,
    RTP_RTP_AVP,    //UDP
    RTP_RTP_AVP_TCP //TCP
} rtp_type_e;

/* RTP��RTCP�˿ڶԽṹ�� 4bytes */
typedef struct
{
    uint16 rtp;
    uint16 rtcp;
} port_pair_t;

/* NAL��Ϣ�ṹ�� 16bytes */
typedef struct _nal_msg
{
    uint8 eof; /* �Ƿ���EOF */
    uint8 type; /* NAL���� */
    uint8 reserve[2];
    uint8 * start; /* ��ʼ��ַָ�� */
    uint8 * end; /* ������ַָ�� */
    uint32 size; /* ���ݵĴ�С */
} nal_msg_t;

/* RTPͷ����ṹ�� 16bytes */
typedef struct _rtp_header
{
    /* byte 0 */
    uint8 csrc_len: 4; /* CSRC���� */
    uint8 extension: 1; /* ��չλ */
    uint8 padding: 1;  /* ���λ */
    uint8 version: 2;  /* �汾�� */

    /* byte 1 */
    uint8 payload: 7; /* �������� */
    uint8 marker: 1; /* ���һ�����ݵı�ʶλ */

    /* bytes 2, 3 */
    uint16 seq_no;  /* ���к� */
    /* bytes 4-7 */
    uint32 timestamp; /* ʱ��� */
    /* bytes 8-11 */
    uint32 ssrc; /* SSRC����������һ·RTP�� */

    /* bytes 12-16 */
    uint32 reserve;
} rtp_hdr_t;

/* RTP��Ϣ����ṹ�� 128bytes */
typedef struct _rtp_info
{
    nal_msg_t nal;    /* NAL��Ϣ 16bytes */
    rtp_hdr_t rtp_hdr;/* RTPͷ 16bytes */
    uint32 hdr_len;       /* RTPͷ�ĳ��� */
    uint8 hdr_data[20];

    uint32 s_bit;    /* ��ʼλ */
    uint32 e_bit;    /* ����λ */
    uint32 fu_flag;  /* �Ƿ��ǲ���FU����ı�ʶ */
    uint32 nal_begin; /* �Ƿ���һ��NAL�Ŀ�ʼ�ı�ʶ */

    uint8 * pRTP; /* ָ��RTP����ַ��ָ�� */
    uint8 * start; /* ָ���ص���ʼ��ַ��ָ�� */
    uint8 * end; /* ָ���صĽ�����ַ��ָ�� */
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

/* UDP��Ϣ����ṹ�� 24bytes */
typedef struct _udp_info
{
    struct sockaddr rem_addr;   //RTP�ͻ��˵������ַ
    port_pair_t cli_ports;      //�ͻ��˵Ķ˿ڶ�
    port_pair_t srv_ports;      //�������˵Ķ˿ڶ�
} udp_info_t;

/* TCP��Ϣ����ṹ�� 4bytes */
typedef struct _tcp_info
{
    port_pair_t interleaved; //interleavedѡ����ָ���Ķ˿ڶ�
} tcp_info_t;

/* ���ͻ��� 24bytes */
typedef struct _writev_info
{
    uint32 piov_size; /* ���������ݵĳ��� */
    uint32 send_size; /* �������Ѿ����͵ĳ��� */
    uint32 buf_size;
    uint8 * buf;
    uint32 frame_size;
    uint8 * frame_buf;
} writev_info_t;

/* RTP������Ϣ����ṹ�� 200bytes */
typedef struct _rtp_trans
{
    rtp_type_e type;      /* RTP�������� */
    uint32 resend_time; /* ���·��͵Ĵ��� */

    uint32 video_seqno;
    uint32 audio_seqno;
    uint32 media_type;

    uint32 blocked;       /* ��һ�η������� */
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

    sint32 tcp_fd;       /* TCP�������ļ������� */

    sint32 rtp_udp_vfd; /* UDP����Ƶ�����ļ������� */
    sint32 rtp_udp_afd; /* UDP����Ƶ�����ļ������� */
    sint32 rtp_udp_tfd; /* UDP����Ƶ�����ļ������� */

    sint32 rtcp_udp_vfd; /* UDP����Ƶ�����ļ������� */
    sint32 rtcp_udp_afd; /* UDP����Ƶ�����ļ������� */

    tcp_info_t tcp_info; /* TCP��Ϣ */
    tcp_info_t tcp_tinfo; /* text_TCP��Ϣ */

    udp_info_t udp_vinfo; /* ��Ƶ��UDP��Ϣ */
    udp_info_t udp_ainfo; /* ��Ƶ��UDP��Ϣ */
    udp_info_t udp_tinfo; /* ��Ƶ��UDP��Ϣ */

    writev_info_t wv_info;
    uint32 reserve;
} rtp_trans_t;

/* RTP�Ự�ṹ�� 616bytes */
typedef struct _rtp_sessn
{
    rtp_trans_t trans;   /* RTP������Ϣ 200bytes*/
    rtp_info_t vinfo; /* ��Ƶ��RTP��Ϣ 128bytes*/
    rtp_info_t ainfo; /* ��Ƶ��RTP��Ϣ */
    rtp_info_t tinfo; /* ��Ƶ��RTP��Ϣ */

    uint32 media_type; /* ��һλ������Ƶ���ڶ�λ������Ƶ������λ�����ı���0�������ã�1�������� */

    uint32 start_seq;     /* ��ʼ�����к� */
    uint32 start_rtptime; /* ��ʼ��RTPʱ��� */

    uint32 started;	/* �Ƿ�ʼ����Ƶ������ı�ʶ */
    uint32 ssrc;	/* SSRC��ʶ����������һ·RTP�� */
    uint32 again_count;

    uint32 octet_count;
    uint32 reserve;
} rtp_sessn_t;


/* ͨ��RTP������Ϣ 400bytes */
typedef struct _rtp_ch_info
{
    uint32 en_audio; /* ��Ƶʹ�� */
    audio_type_e atype; /* ��Ƶ�������� 0:g711 */
    video_type_e vtype; /* ��Ƶ�������� 0:h264 1:h265 */
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
* ��������: RTP��Ϣ��ʼ��
* �������: rs_srv: RTSP server ������
*				  ch_idx: ͨ����
* �������: ��
* ����ֵ  :
*****************************************************************************/
//int rtp_info_init(rtsp_srv_t * rs_srv, int ch_idx);

/******************************************************************************
* ��������: ��ȡʱ���
* �������: rs_srv: RTSP server ������
*			rss: RTSP session ʵ��
*			ch_idx: ͨ����
* �������: ��
* ����ֵ  : ���ػ�ȡ����ʱ���
*****************************************************************************/
uint32 rtp_get_timestamp(int srv_fd, int type);

////////////////////////////////////////////////////////////////////////////////
// �������� rtp_set_timestamp
// ��  ���� ����ʱ���
// ��  ���� [in] _iType -- �������������ı���
//          [in] _iAVFlag -- 0��Ƶ 1��Ƶ
//          [in] time_usec -- ʱ���
// ����ֵ��
// ˵  ����
////////////////////////////////////////////////////////////////////////////////
int rtp_set_timestamp(int srv_fd, int type, unsigned int time_usec);

uint8 * get_rtp_pack(rtp_info_t * rtp_info, uint8 * rtpHdr, uint32 data_type, uint32 * p_packet_size);

uint8 * get_audio_rtp_pack(rtp_info_t * rtp_info, uint32 ts);

int set_rtp_pack(rtp_info_t * rtp_info, uint32 enc_type, uint8 * nal_buf, uint32 nal_size, uint32 time_stamp, uint32 end_of_frame);


#define MAX_EAGAIN_TIMEOUT (30) //���ó�ʱʱ�䣬��λ��

#endif


