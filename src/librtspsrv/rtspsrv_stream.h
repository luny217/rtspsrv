//////////////////////////////////////////////////////////////////////////////
// ��Ȩ���У�2009-2014����������߿����ּ������޹�˾
// ���ļ���δ�����ģ������˺���߿ƵĻ��ܺ�ר������
////////////////////////////////////////////////////////////////////////////////
// �ļ����� send_av_data.h
// ���ߣ�   �����
// �汾��   1.0
// ���ڣ�   2014-07-15
// ������   ����Ƶ���ݷ��ͳ���ͷ�ļ�
// ��ʷ��¼��
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

/* ��Ҫע��������ͨ����������ͨ���Ĺ���ʽ������ */
/* ������ͨ���ṹ�� consumer channel */
typedef struct _strm_cch
{
    uint8 enable;       /* ͨ���Ƿ��� */
    uint8 expect_en;    /* ͨ�����������������,��ֹ��ͬ���������� */
    uint8 sync_flag;    /* ͬ����־,open_stream���غ�,���ܻص�����,���򷵻�ǰ�ص������ݻᱻ�ϲ㶪�� */
    uint8 ciframe;      /* �ȴ�����I֡��־ */
    sint32 count;       /* �������� */

    sint32 cbitrate;    /* NETMS_TIME_INTERVALƽ������,bit/s */
    sint32 cbyte_cnt;   /* ÿ���������ۼ�,Ϊͳ��ƽ�������ṩ���� */
    sint32 cbyte_ti;    /*  */
    sint32 cexp_br;     /* ͳ����ʱ���� */
    uint32 serial;      /* ��Ƶ���к� */

    sint32 cifr_cnt;    /* �ص���i֡���� */
    sint32 cvfr_cnt;    /* �ص���p֡���� */
    sint32 cafr_cnt;    /* ��Ƶ����ͳ��,��Ƶ��������Ƶ����,�ṹ���� */
} strm_cch_t;



void rtsp_add_interhead(uint8 * rtp_buf, uint16 rtp_size, uint32 channel_id);

///////////////////////////////////////////////////////////////////////////////
// RTP��ʽ��������ʱTCPͷ�е�ͨ��ID
///////////////////////////////////////////////////////////////////////////////
typedef enum _tagCHANNEL_ID
{
    VIDEO_RTP = 0, //RTP��Ƶͨ��ID
    VIDEO_RTCP,	   //RTCP��Ƶͨ��ID
    AUDIO_RTP,     //RTP��Ƶͨ��ID
    AUDIO_RTCP     //RTCP��Ƶͨ��ID
} CHANNEL_ID_E;


int rstrm_send_data(rtsp_srv_t * rs_srv, rtsp_sessn_t * rss, int ch_idx);

#endif

