/*
 * FileName:     rtsp_handler.c
 * Author:         luny  Version: 1.0  Date: 2017-5-17
 * Description:
 * Version:
 * Function List:
 *                 1.
 * History:
 *     <author>   <time>    <version >   <desc>
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/epoll.h>
#ifndef _WIN32
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif
#include <sys/types.h>

#include "avformat_network.h"
#include "avutil_time.h"
#include "avutil_avstring.h"
#include "avutil_random_seed.h"

#include "sstring.h"
#include "fifo.h"
#include "mshead.h"
#include "rtspsrv_epoll.h"
#include "rtspsrv_handler.h"
#include "rtspsrv_adapter.h"
#include "rtspsrv_utils.h"

#ifndef _RTSPSRV_TEST
#include "mem_manage.h"
#include "sys_netapi.h"
#else
#include "libavutil/avutil_log.h"
#endif

/* RTSP������Ϣ */
#define PACKAGE "HanBang RTSP Server"
#define DESCRIPTION "RTSP/RTP stream from IPC"
#define VERSION "V1.0.1"
#define RTSP_VER "RTSP/1.0"  /* RTSP�汾��Ϣ */
#define RTSP_EL "\r\n"      /* �س����� */
#define RTSP_RTP_AVP "RTP/AVP"    /* UDP���䷽ʽ */

static int _find_sessnid(char * sessn_str)
{
    uint32 sessn_id = 0;
    int i = 0;
    int getstart = -1;
    char tmp_str[32] = {0};

    for (i = 0; i < sstrlen(sessn_str); i++)
    {
        /*if (av_isdigit(sessn_str[i]))*/
        if (sessn_str[i] >= '0' && sessn_str[i] <= '9')
        {
            if (getstart == -1)
                getstart = i;
            continue;
        }
        if (getstart != -1)
            break;
    }
    sessn_str = sessn_str + getstart;
    memcpy(tmp_str, sessn_str, (i - getstart));

    /*av_log(NULL, AV_LOG_ERROR, "format sessn id:%s len:%d %d i:%d start:%d\n",
           sessn_str, sstrlen(sessn_str), (i - getstart), i, getstart);*/
    sessn_id = (uint32)atoll(tmp_str);

    return sessn_id;
}

static int _sessn_set_strmtype(rtsp_sessn_t * rss, char * object)
{
    chstrm_type_e stm_type = MAIN_STREAM;

    if (object != NULL)
    {
        if (!av_strncasecmp(object, GET_MAIN_STREAM, sstrlen(GET_MAIN_STREAM)))
            stm_type = MAIN_STREAM;
        else if (!av_strncasecmp(object, GET_SUB_STREAM1, sstrlen(GET_SUB_STREAM1)))
            stm_type = SUB_STREAM1;
        else if (!av_strncasecmp(object, GET_SUB_STREAM2, sstrlen(GET_SUB_STREAM2)))
            stm_type = SUB_STREAM2;
    }
    else
    {
        return -1;
    }

    if (rss->stm_type != stm_type)
        rss->stm_type = stm_type;

    return stm_type;
}

static int _sessn_init_rtpinfo(rtsp_srv_t * rs_srv, rtsp_sessn_t * rss, int ch_idx)
{
    rtp_chinfo_t * ch_info = NULL;
    int stm_type;
    rtp_hdr_t * video_hdr = NULL;
    rtp_hdr_t * audio_hdr = NULL;
    rtp_hdr_t * text_hdr = NULL;
    rtp_info_t * vinfo = NULL;
    rtp_info_t * ainfo = NULL;
    rtp_info_t * tinfo = NULL;

    if (rs_srv == NULL || rss == NULL || ch_idx < 0 || ch_idx > CHN_MAX)
    {
        return -1;
    }

    stm_type = rss->stm_type;

    if (stm_type < 0 || stm_type > CH_STRM_MAX)
    {
        return -1;
    }

    ch_info = &rs_srv->chi[ch_idx].rtp_chinfo[stm_type];

    video_hdr = &ch_info->mdstrms[M_VIDEO].rtp_hdr;
    audio_hdr = &ch_info->mdstrms[M_AUDIO].rtp_hdr;
    text_hdr = &ch_info->mdstrms[M_TEXT].rtp_hdr;

    vinfo = &ch_info->mdstrms[M_VIDEO];
    ainfo = &ch_info->mdstrms[M_AUDIO];
    tinfo = &ch_info->mdstrms[M_TEXT];

    /* ��ʼ����ƵRTP��Ϣ */
    video_hdr->payload = PAYLOAD_TYPE_H264;
    vinfo->ssrc = video_hdr->ssrc = com_random32();
    video_hdr->version = RTP_VERSION;
    video_hdr->seq_no = (uint16)(com_rand() % 0xFFFF);
    video_hdr->timestamp = com_random32();
    vinfo->octet_count = 0;
    vinfo->packet_count = 0;
    vinfo->again_count = 0;
    vinfo->first_packet = 1;
    vinfo->last_octet_count = 0;
    vinfo->first_rtcp_ntp_time = rsrv_ntp_time();

    /* ��ʼ����ƵRTP��Ϣ */
    audio_hdr->payload = PAYLOAD_TYPE_PCMA;
    ainfo->ssrc = audio_hdr->ssrc = com_random32();
    audio_hdr->version = RTP_VERSION;
    audio_hdr->seq_no = (uint16)(com_rand() % 0xFFFF);
    audio_hdr->timestamp = com_random32();
    ainfo->octet_count = 0;
    ainfo->packet_count = 0;
    ainfo->first_packet = 1;
    ainfo->last_octet_count = 0;
    ainfo->first_rtcp_ntp_time = rsrv_ntp_time();

    /* ��ʼ���ı�RTP��Ϣ, ������Я���Զ����ı���Ϣ */
    text_hdr->payload = PAYLOAD_TYPE_TEXT;
    tinfo->ssrc = text_hdr->ssrc = com_random32();
    text_hdr->version = RTP_VERSION;
    text_hdr->seq_no = (uint16)(com_rand() % 0xFFFF);
    text_hdr->timestamp = com_random32();

    memcpy(&(rss->rtp_sessn.vinfo), &(ch_info->mdstrms[M_VIDEO]), sizeof(rtp_info_t));
    memcpy(&(rss->rtp_sessn.ainfo), &(ch_info->mdstrms[M_AUDIO]), sizeof(rtp_info_t));
    memcpy(&(rss->rtp_sessn.tinfo), &(ch_info->mdstrms[M_TEXT]), sizeof(rtp_info_t));

    return 0;
}

/******************************************************************************
* ��������: ��RTSP�����в���Ҫ������RTSP�Ự
* �������: rs_srv: RTSP server ������
*				  rss: RTSP sessionʵ��
*				  ch_idx: ͨ����
* �������: ��
* ����ֵ  : >0-д�����ݴ�С, <0-�������
*****************************************************************************/
static int _sessn_search_teardown(rtsp_srv_t * rs_srv, rtsp_sessn_t * rss, int ch_idx)
{
    rtsp_sessn_t * sessn = NULL;
    n_slist_t * sessn_list = NULL, *item = NULL;

    if (rs_srv == NULL || rss == NULL)
    {
        rsrv_log(AV_LOG_ERROR, "_search_teardown rs_srv is NULL\n");
        return -1;
    }

    sessn_list = rs_srv->chi[ch_idx].sessn_lst;

    if (sessn_list == NULL)
    {
        rsrv_log(AV_LOG_ERROR, "_search_teardown sessn_list is NULL\n");
        return -1;
    }

    for (item = sessn_list; item; item = item->next)
    {
        sessn = (rtsp_sessn_t *)item->data;
        if (sessn == NULL)
        {
            continue;
        }

        if (rss->sessn_id == sessn->sessn_id)
        {
            rsrv_log(AV_LOG_WARNING, "[id:%u fd:%d]sessn teardown = %u\n", rss->ch_idx, rss->fd, rss->sessn_id);
            rss->state = RTSP_STATE_TEARDOWN;
            return 0;
        }
    }

    return -1;
}

/*
** ��������_get_stat
** ��  ������ȡRTSP������Ķ�Ӧ��������Ϣ
** ��  ����[in] err RTSP������
** ����ֵ���ɹ����ش�����Ķ�Ӧ��������Ϣ��������NULL
** ˵  ����
*/
static char * _get_stat(int ecode)
{
    int i;

    static struct
    {
        char * token;
        int code;
    } status[] =
    {
        { "Continue", 100 },
        { "OK", 200 },
        { "Created", 201 },
        { "Accepted", 202 },
        { "Non-Authoritative Information", 203 },
        { "No Content", 204 },
        { "Reset Content", 205 },
        { "Partial Content", 206 },
        { "Multiple Choices", 300 },
        { "Moved Permanently", 301 },
        { "Moved Temporarily", 302 },
        { "Bad Request", 400 },
        { "Unauthorized", 401 },
        { "Payment Required", 402 },
        { "Forbidden", 403 },
        { "Stream Not Found", 404 },
        { "Method Not Allowed", 405 },
        { "Not Acceptable", 406 },
        { "Proxy Authentication Required", 407 },
        { "Request Time-out", 408 },
        { "Conflict", 409 },
        { "Gone", 410 },
        { "Length Required", 411 },
        { "Precondition Failed", 412 },
        { "Request Entity Too Large", 413 },
        { "Request-URI Too Large", 414 },
        { "Unsupported Media Type", 415 },
        { "Bad Extension", 420 },
        { "Invalid Parameter", 450 },
        { "Parameter Not Understood", 451 },
        { "Conference Not Found", 452 },
        { "Not Enough Bandwidth", 453 },
        { "Session Not Found", 454 },
        { "Method Not Valid In This State", 455 },
        { "Header Field Not Valid for Resource", 456 },
        { "Invalid Range", 457 },
        { "Parameter Is Read-Only", 458 },
        { "Unsupported transport", 461 },
        { "Internal Server Error", 500 },
        { "Not Implemented", 501 },
        { "Bad Gateway", 502 },
        { "Service Unavailable", 503 },
        { "Gateway Time-out", 504 },
        { "RTSP Version Not Supported", 505 },
        { "Option not supported", 551 },
        { "Extended Error:", 911 },
        { NULL, -1 }
    };

    for (i = 0; status[i].code != ecode && status[i].code != -1; ++i);
    return status[i].token;
}

/*
** ��������add_time_stamp
** ��  ������RTSPӦ�������ʱ����Ϣ
** ��  ����[in/out] buf  RTSPӦ������ָ��
**         [in]     crlf �Ƿ����CRLF(�س�����)�ı�ʶ
** ����ֵ���ɹ�����ERR_NOERROR��������ERR_GENERIC
** ˵  ����
*/
static void _add_timestamp(char * buf, int crlf)
{
    struct tm * t;
    time_t now;

    /* ��ȡ��ǰ��ʱ�� */
    now = time(NULL);
    t = gmtime(&now);

    /*
    * concatenates a null terminated string with a
    * time stamp in the format of "Date: 23 Jan 1997 15:35:06 GMT"
    */
    strftime(buf + sstrnlen(buf, RTSP_BUFSIZE), 38, "Date: %a, %d %b %Y %H:%M:%S GMT""\r\n", t);

    if (crlf)
        sstrncat(buf, "\r\n", RTSP_BUFSIZE); /* ����CRLF����β */
}

/*
** ��������parse_rtsp_url
** ��  ��������RTSP��URL
** ��  ����[in]   url        RTSP��URL
**         [in]   server_len RTSP��ַ�ĳ���
**         [in]   object_len RTSP���Ƶĳ���
**         [out]  server     RTSP��ַ
**         [out]  port       RTSP�˿ں�
**         [out]  object     RTSP����
** ����ֵ���ɹ�����0��������1��-1
** ˵  ����
**       ����0��ʾURL����Ч�ģ�����1��ʾURL����Ч�ģ�����-1��ʾ���ʹ��󣬱���:������̫С
**       �����ĸ�ʽ:rtsp://server[:port]/object
**       rtsp://192.168.111.71:8554/H264MainStream
*/
static int _parse_url(const char * url, char * server, int server_len, uint16 * port, char * object, int object_len)
{
    int not_valid_url = 1;
    char full[MAX_STR_LENGTH] = { 0 }, sub_str[MAX_STR_LENGTH] = {0};

    /* ��URL�����ݸ��Ʊ��� */
    sstrncpy(full, url, 128);

    /* �ж�ǰ7���ֽ��Ƿ���"rtsp://" */
    if (0 == sstrncmp(full, "rtsp://", 7))
    {
        char * token;
        int has_port = 0;
        /* ���ȣ���Ҫ�ж��Ƿ��ж˿���Ϣ ��"rtsp://"�Ժ�����ݸ��Ʊ��� */
        sstrncpy(sub_str, &full[7], 128);

        /* ���ҵ�һ�γ���'/'��λ�� */
        if (sstrchr(sub_str, '/'))
        {
            int len = 0;
            uint16 i = 0;
            char * end_ptr = NULL;

            /* ���ҵ�һ�γ���'/'��λ�� */
            end_ptr = sstrchr(sub_str, '/');
            len = end_ptr - sub_str;

            for (; (i < sstrnlen(url, 128)); i++)
                sub_str[i] = 0;

            /* ��������'/'�ַ�֮������� */
            sstrncpy(sub_str, &full[7], len);
        }

        /* �����Ƿ����':'�������ж��Ƿ��ж˿���Ϣ */
        if (sstrchr(sub_str, ':'))
        {
            has_port = 1; /* ���ж˿ںŵı�ʶ */
        }

        /* ����"rtsp://"�Ժ�����ݣ�server[:port]/object */
        token = sstrtok(&full[7], " :/\t\n");
        if (token != NULL)
        {
            /* ��ȡ��RTSP��ַ */
            sstrncpy(server, token, server_len);
            if (server[server_len - 1])
            {
                return -1;
            }

            if (has_port)
            {
                /* ��ȡ��RTSP�˿ں� */
                char * port_str = sstrtok(&full[sstrnlen(server, 128) + 7 + 1], " /\t\n");
                if (port_str != NULL)
                {
                    *port = (uint16)atol(port_str);
                }
                else
                {
                    return -1;
                }
            }
            else
            {
                *port = 554; /* ��ȡ�˿ں� */
            }

            /* ��Ҫ��������RTSP���� */
            not_valid_url = 0;
            token = sstrtok(NULL, " ");
            if (token != NULL) /* ����RTSP���� */
            {
                /* ��ȡ��RTSP���� */
                sstrncpy(object, token, object_len);
                if (object[object_len - 1])
                {
                    return -1;
                }
            }
            else /* ������RTSP���� */
            {
                object[0] = '\0';
            }
        }
    }
    else
    {
        rsrv_log(AV_LOG_ERROR, "url is invalid: %s", url);
        not_valid_url = 1;
    }

    return not_valid_url;
}

static int _get_rtp_port(char * trans_pt, port_pair_t * port_pt, port_pair_t * txt_port_pt, int _iFlag)
{
    char * tmp_pt = NULL;

    tmp_pt = sstrstr(trans_pt, "interleaved");
    if (tmp_pt != NULL)
    {
        /* ͨ������interleaved�ֶλ�ȡ��Ӧ��RTP��RTCP�˿ڶ� */
        tmp_pt = sstrstr(tmp_pt, "=");

        sscanf(tmp_pt + 1, "%2hd", &port_pt->rtp);
        if (M_TEXT == _iFlag)
        {
            sscanf(tmp_pt + 1, "%2hd", &txt_port_pt->rtp);
        }

        tmp_pt = sstrstr(tmp_pt, "-");
        if (tmp_pt != NULL)
        {
            sscanf(tmp_pt + 1, "%2hd", &port_pt->rtcp);
            if (M_TEXT == _iFlag)
            {
                sscanf(tmp_pt + 1, "%2hd", &txt_port_pt->rtcp);
            }
        }
        else
        {
            port_pt->rtcp = port_pt->rtp + 1;
            if (M_TEXT == _iFlag)
            {
                txt_port_pt->rtcp = txt_port_pt->rtp + 1;
            }
        }
    }
    return 0;
}

static void _reply_error(rtsp_sessn_t * rss, int ecode)
{
    if (ecode == 1)
    {
        rsrv_do_reply(rss, RTSP_STATUS_BAD_REQUEST);
    }
    else
    {
        rsrv_do_reply(rss, 500);
    }
}

/*
** ��������tcp_accept
** ��  ��������TCP���������
** ��  ����[in] fd  �����ļ�������
** ����ֵ���ɹ����ؼ����������ļ���������������ERR_GENERIC
** ˵  ����
*/
int rsrv_tcp_accept(int fd, char * ip, uint16 * port)
{
    struct sockaddr_in addr;
    char addr_p[16];    /* ��������ͻ���ip��Ϣ */
    int socket;
    socklen_t addrlen = sizeof(addr);

    memset(&addr, 0x00, sizeof(addr));
    addrlen = sizeof(addr);

    /* ���ܿͻ��˵����ӣ���ȡ�����ַ */
    socket = accept(fd, (struct sockaddr *)&addr, &addrlen);
    if (socket < 0)
    {
        perror("accept");
        return socket;
    }

    inet_ntop(AF_INET, &addr.sin_addr, addr_p, sizeof(addr_p));
    rsrv_log(AV_LOG_WARNING, "accept form [fd:%d ip:%s:%d]\n", socket, addr_p, ntohs(addr.sin_port));
    memcpy(ip, addr_p, 16);
    if (port) *port = addr.sin_port;

    return socket;
}

int rsrv_tcp_read(int fd, void * buffer, int nbytes)
{
    return recv(fd, buffer, nbytes, 0);
}

int rsrv_tcp_write(int fd, char * buf, int size)
{
    return send(fd, buf, size, 0);
}

static int _reply_describe(rtsp_sessn_t * rss, char * url, char * descr)
{
    int ret = -1;

    memset(rss->out_buf, 0x00, RTSP_BUFSIZE);

    /*describe*/
    /* RTSP/1.0 200 OK */
    snprintf(rss->out_buf, RTSP_BUFSIZE, "%s %d %s"RTSP_EL, RTSP_VER, 200, _get_stat(200));

    /* Cseq: */
    snprintf(rss->out_buf + sstrnlen(rss->out_buf, RTSP_BUFSIZE), RTSP_BUFSIZE, "CSeq: %d"RTSP_EL, rss->cseq);

    /* Server: */
    snprintf(rss->out_buf + sstrnlen(rss->out_buf, RTSP_BUFSIZE), RTSP_BUFSIZE, "Server: %s %s"RTSP_EL, PACKAGE, VERSION);

    /*Content-Type:*/
    sstrncat(rss->out_buf, "Content-Type: application/sdp"RTSP_EL, RTSP_BUFSIZE);

    /* Content-Base: */
    snprintf(rss->out_buf + sstrnlen(rss->out_buf, RTSP_BUFSIZE), RTSP_BUFSIZE, "Content-Base: %s"RTSP_EL, url);

    /* Content-Length: */
    snprintf(rss->out_buf + sstrnlen(rss->out_buf, RTSP_BUFSIZE), RTSP_BUFSIZE,
             "Content-Length: %d"RTSP_EL, sstrnlen(descr, RTSP_BUFSIZE));

    /* Date */
    _add_timestamp(rss->out_buf, 0);

    /* end of message */
    sstrncat(rss->out_buf, RTSP_EL, RTSP_BUFSIZE);

    /* ���SDP��Ϣ */
    sstrncat(rss->out_buf, descr, RTSP_BUFSIZE);

    /* RTSPӦ���ַ������ȼ��� */
    rss->out_size = sstrnlen(rss->out_buf, RTSP_BUFSIZE);

    /*av_log(NULL, AV_LOG_WARNING, "send DESCRIBE reply: \n%s", rss->out_buffer);*/

    /*av_log(NULL, AV_LOG_WARNING, "[id:%d fd:%d]send DESCRIBE reply %d bytes\n", rss->self_idx, rss->fd, rss->out_size);*/

    /* ����RTSPӦ�� */
    ret = rsrv_tcp_write(rss->fd, rss->out_buf, rss->out_size);
    if (ret < 0)
    {
        rsrv_log(AV_LOG_ERROR, "send DESCRIBE reply error");
        return ERR_GENERIC;
    }
    return ERR_NOERROR;
}

static int _reply_setup(rtsp_sessn_t * rss, rtp_sessn_t * rtp, int media_type)
{
    int w_pos = 0, ret = -1;

    memset(rss->out_buf, 0x00, RTSP_BUFSIZE);

    /* build a reply message */
    /*sprintf(rss->out_buffer, "%s %d %s"RTSP_EL"CSeq: %d"RTSP_EL"Server: %s %s"RTSP_EL,
     RTSP_VER, 200, rtsp_get_stat(200), rss->cseq, PACKAGE, VERSION);*/

    /* RTSP/1.0 200 OK */
    snprintf(rss->out_buf, RTSP_BUFSIZE, "%s %d %s"RTSP_EL, RTSP_VER, 200, _get_stat(200));

    /*Cseq:*/
    snprintf(rss->out_buf + sstrnlen(rss->out_buf, RTSP_BUFSIZE), RTSP_BUFSIZE, "CSeq: %d"RTSP_EL, rss->cseq);

    /*Server:*/
    snprintf(rss->out_buf + sstrnlen(rss->out_buf, RTSP_BUFSIZE), RTSP_BUFSIZE, "Server: %s %s"RTSP_EL, PACKAGE, VERSION);

    /*Session:*/
    w_pos = sstrnlen(rss->out_buf, RTSP_BUFSIZE);
    w_pos += snprintf(rss->out_buf + w_pos, RTSP_BUFSIZE, "Session: %u"RTSP_EL, rss->sessn_id);

    /*Transport:*/
    w_pos += snprintf(rss->out_buf + w_pos, RTSP_BUFSIZE, "Transport: ");
    switch (rtp->trans.type)
    {
        case RTP_RTP_AVP:
            if (AUDIO == media_type)
            {
                w_pos += snprintf(rss->out_buf + w_pos, RTSP_BUFSIZE, "RTP/AVP;unicast;client_port=%d-%d;source=%s;server_port=", \
                                  rtp->trans.udp_ainfo.cli_ports.rtp, rtp->trans.udp_ainfo.cli_ports.rtcp, rtspsrv_adpt_get_ip());

                w_pos += snprintf(rss->out_buf + w_pos, RTSP_BUFSIZE, "%d-%d", rtp->trans.udp_ainfo.srv_ports.rtp, rtp->trans.udp_ainfo.srv_ports.rtcp);
            }
            else
            {
                w_pos += snprintf(rss->out_buf + w_pos, RTSP_BUFSIZE, "RTP/AVP;unicast;client_port=%d-%d;source=%s;server_port=", \
                                  rtp->trans.udp_vinfo.cli_ports.rtp, rtp->trans.udp_vinfo.cli_ports.rtcp, rtspsrv_adpt_get_ip());

                w_pos += snprintf(rss->out_buf + w_pos, RTSP_BUFSIZE, "%d-%d", rtp->trans.udp_vinfo.srv_ports.rtp, rtp->trans.udp_vinfo.srv_ports.rtcp);
            }
            break;

        case RTP_RTP_AVP_TCP:
            w_pos += snprintf(rss->out_buf + w_pos, RTSP_BUFSIZE, "RTP/AVP/TCP;unicast;interleaved=%d-%d", \
                              rtp->trans.tcp_info.interleaved.rtp, rtp->trans.tcp_info.interleaved.rtcp);
            break;

        default:
            break;
    }
    w_pos += snprintf(rss->out_buf + w_pos, RTSP_BUFSIZE, ";ssrc=%u", rtp->ssrc);
    sstrncat(rss->out_buf, RTSP_EL, RTSP_BUFSIZE);

    /* Date:*/
    _add_timestamp(rss->out_buf, 0);
    sstrncat(rss->out_buf, RTSP_EL, RTSP_BUFSIZE);

    /* RTSPӦ���ַ������ȼ��� */
    rss->out_size = sstrnlen(rss->out_buf, RTSP_BUFSIZE);

    /*av_log(NULL, AV_LOG_WARNING, "send SETUP reply: \n%s", rss->out_buf);*/
    /*av_log(NULL, AV_LOG_WARNING, "[id:%d fd:%d]send SETUP reply %d bytes\n", rss->ch_idx, rss->fd, rss->out_size);*/

    /* ����RTSPӦ�� */
    ret = rsrv_tcp_write(rss->fd, rss->out_buf, rss->out_size);
    if (ret < 0)
    {
        rsrv_log(AV_LOG_ERROR, "send SETUP reply error\n");
        return ERR_GENERIC;
    }

    return ERR_NOERROR;
}

static int _reply_play(rtsp_srv_t * rs_srv, rtsp_sessn_t * rss, int ch_idx, char * url)
{
    char temp[32];
    int ret = -1, stm_type;
    rtp_chinfo_t * rtp_chinfo = NULL;

    if (rs_srv == NULL || rss == NULL)
    {
        return -1;
    }

    stm_type = rss->stm_type;

    if (stm_type < 0 || stm_type > CH_STRM_MAX)
    {
        return -1;
    }

    rtp_chinfo = &rs_srv->chi[ch_idx].rtp_chinfo[stm_type];

    memset(rss->out_buf, 0x00, RTSP_BUFSIZE);
    /* build a reply message */
    /* RTSP/1.0 200 OK */
    snprintf(rss->out_buf, RTSP_BUFSIZE, "%s %d %s"RTSP_EL, RTSP_VER, 200, _get_stat(200));

    /* CSeq: */
    snprintf(rss->out_buf + sstrnlen(rss->out_buf, RTSP_BUFSIZE), RTSP_BUFSIZE, "CSeq: %d"RTSP_EL, rss->cseq);

    /* Server: */
    snprintf(rss->out_buf + sstrnlen(rss->out_buf, RTSP_BUFSIZE), RTSP_BUFSIZE, "Server: %s %s"RTSP_EL, PACKAGE, VERSION);

    /* Session: */
    sstrncat(rss->out_buf, "Session: ", RTSP_BUFSIZE);
    snprintf(temp, 4, "%u", rss->sessn_id);
    sstrncat(rss->out_buf, temp, RTSP_BUFSIZE);
    sstrncat(rss->out_buf, RTSP_EL, RTSP_BUFSIZE);

    /* RTP-info: */
    sstrncat(rss->out_buf, "RTP-info: ", RTSP_BUFSIZE);
    sstrncat(rss->out_buf, "url=", RTSP_BUFSIZE);
    snprintf(rss->out_buf + sstrnlen(rss->out_buf, RTSP_BUFSIZE), RTSP_BUFSIZE, "%s/track1", url);
    sstrncat(rss->out_buf, ";", RTSP_BUFSIZE);


    snprintf(rss->out_buf + sstrnlen(rss->out_buf, RTSP_BUFSIZE), RTSP_BUFSIZE, "seq=%u;rtptime=%u",
             rtp_chinfo->mdstrms[VIDEO].rtp_hdr.seq_no, rtp_chinfo->mdstrms[VIDEO].rtp_hdr.timestamp);

    if (rtp_chinfo->en_audio == 1)
    {
        sstrncat(rss->out_buf, ",", RTSP_BUFSIZE);
        sstrncat(rss->out_buf, "url=", RTSP_BUFSIZE);
        snprintf(rss->out_buf + sstrnlen(rss->out_buf, RTSP_BUFSIZE), RTSP_BUFSIZE, "%s/track2", url);
        sstrncat(rss->out_buf, ";", RTSP_BUFSIZE);
        snprintf(rss->out_buf + sstrnlen(rss->out_buf, RTSP_BUFSIZE), RTSP_BUFSIZE, "seq=%u;rtptime=%u",
                 rtp_chinfo->mdstrms[AUDIO].rtp_hdr.seq_no, rtp_chinfo->mdstrms[AUDIO].rtp_hdr.timestamp);
    }

    sstrncat(rss->out_buf, RTSP_EL, RTSP_BUFSIZE);
    /* Date: */
    _add_timestamp(rss->out_buf, 0);
    sstrncat(rss->out_buf, RTSP_EL, RTSP_BUFSIZE);

    /* RTSPӦ���ַ������ȼ��� */
    rss->out_size = sstrnlen(rss->out_buf, RTSP_BUFSIZE);

    /*av_log(NULL, AV_LOG_WARNING, "send PLAY reply: \n%s", rss->out_buffer);*/
    /*av_log(NULL, AV_LOG_WARNING, "[id:%d fd:%d]send PLAY reply %d bytes\n", rss->ch_idx, rss->fd, rss->out_size);*/
    /* ����RTSPӦ�� */
    ret = rsrv_tcp_write(rss->fd, rss->out_buf, rss->out_size);
    if (ret < 0)
    {
        rsrv_log(AV_LOG_ERROR, "send PLAY reply error");
        return -1;
    }

    return 0;
}

static int _reply_pause(rtsp_sessn_t * rss)
{
    char temp[30];
    int ret = -1;

    memset(rss->out_buf, 0x00, RTSP_BUFSIZE);

    /* build a reply message */
    snprintf(rss->out_buf, RTSP_BUFSIZE, "%s %d %s"RTSP_EL"CSeq: %d"RTSP_EL"Server: %s/%s"RTSP_EL,
             RTSP_VER, 200, _get_stat(200), rss->cseq, PACKAGE, VERSION);
    sstrncat(rss->out_buf, "Session: ", RTSP_BUFSIZE);
    snprintf(temp, 4, "%u", rss->sessn_id);
    sstrncat(rss->out_buf, temp, RTSP_BUFSIZE);
    sstrncat(rss->out_buf, RTSP_EL, RTSP_BUFSIZE);
    _add_timestamp(rss->out_buf, 0);
    sstrncat(rss->out_buf, RTSP_EL, RTSP_BUFSIZE);

    /* RTSPӦ���ַ������ȼ��� */
    rss->out_size = sstrnlen(rss->out_buf, RTSP_BUFSIZE);

    rsrv_log(AV_LOG_WARNING, "send PAUSE reply: \n%s", rss->out_buf);
    /*����RTSPӦ��*/
    ret = rsrv_tcp_write(rss->fd, rss->out_buf, rss->out_size);
    if (ret < 0)
    {
        rsrv_log(AV_LOG_ERROR, "send PAUSE reply error");
        return ERR_GENERIC;
    }

    return ERR_NOERROR;

}

static int _reply_teardown(rtsp_sessn_t * rss)
{
    char temp[30];
    int ret = -1;
    struct sockaddr rtsp_peer;
    socklen_t namelen = sizeof(rtsp_peer);

    memset(rss->out_buf, 0x00, RTSP_BUFSIZE);

    /* build a reply message */
    snprintf(rss->out_buf, RTSP_BUFSIZE, "%s %d %s"RTSP_EL"CSeq: %d"RTSP_EL"Server: %s/%s"RTSP_EL,
             RTSP_VER, 200, _get_stat(200), rss->cseq, PACKAGE, VERSION);
    sstrncat(rss->out_buf, "Session: ", RTSP_BUFSIZE);
    snprintf(temp, 4, "%u", rss->sessn_id);
    sstrncat(rss->out_buf, temp, RTSP_BUFSIZE);
    sstrncat(rss->out_buf, RTSP_EL, RTSP_BUFSIZE);
    _add_timestamp(rss->out_buf, 0);
    sstrncat(rss->out_buf, RTSP_EL, RTSP_BUFSIZE);

    /* RTSPӦ���ַ������ȼ��� */
    rss->out_size = sstrnlen(rss->out_buf, RTSP_BUFSIZE);

    /*av_log(NULL, AV_LOG_WARNING, "send TEARDOWN reply: \n%s", rss->out_buffer);*/
    /*av_log(NULL, AV_LOG_WARNING, "[id:%d fd:%d]send TEARDOWN reply %d bytes\n", rss->ch_idx, rss->fd, rss->out_size);*/

    if (getpeername(rss->fd, &rtsp_peer, &namelen) != 0)
        return ERR_GENERIC;

    /* ����RTSPӦ�� */
    ret = rsrv_tcp_write(rss->fd, rss->out_buf, rss->out_size);
    if (ret < 0)
    {
        rsrv_log(AV_LOG_ERROR, "send TEARDOWN reply error\n");
        return ERR_GENERIC;
    }

    return ERR_NOERROR;
}

/******************************************************************************
* ��������: RTSP reply options
* �������: rss: RTSP session ʵ��
* �������: ��
* ����ֵ  : >0-д�����ݴ�С, <0-�������
*****************************************************************************/
int rsrv_reply_options(rtsp_sessn_t * rss)
{
    int ret = -1;

    memset(rss->out_buf, 0x00, RTSP_BUFSIZE);

    /* options */
    /* RTSP/1.0 200 OK */
    snprintf(rss->out_buf, RTSP_BUFSIZE, "%s %d %s"RTSP_EL, RTSP_VER, 200, _get_stat(200));

    /* Cseq: */
    snprintf(rss->out_buf + sstrnlen(rss->out_buf, RTSP_BUFSIZE), RTSP_BUFSIZE, "CSeq: %d"RTSP_EL, rss->cseq);

    /* Public: */
    sstrncat(rss->out_buf, "Public: OPTIONS, DESCRIBE, SETUP,PLAY, TEARDOWN"RTSP_EL, RTSP_BUFSIZE);
    sstrncat(rss->out_buf, RTSP_EL, RTSP_BUFSIZE);

    /* RTSPӦ���ַ������ȼ��� */
    rss->out_size = sstrnlen(rss->out_buf, RTSP_BUFSIZE);

    /*av_log(NULL, AV_LOG_WARNING, "send OPTIONS reply: \n%s", rss->out_buffer);*/

    /*av_log(NULL, AV_LOG_WARNING, "[id:%d fd:%d]send OPTIONS reply %d bytes\n", rss->ch_idx, rss->fd, rss->out_size);*/

    /* ����RTSPӦ�� */
    ret = rsrv_tcp_write(rss->fd, rss->out_buf, rss->out_size);
    if (ret < 0)
    {
        rsrv_log(AV_LOG_ERROR, "send OPTIONS reply error\n");
        return ERR_GENERIC;
    }
    else if (ret != rss->out_size)
    {
        rsrv_log(AV_LOG_ERROR, "send OPTIONS reply size:%d\n", ret);
    }

    return ERR_NOERROR;
}

/******************************************************************************
* ��������: RTSP reply
* �������: rss: RTSP session ʵ��
*				  ecode: RTSP������
* �������: ��
* ����ֵ  : >0-д�����ݴ�С, <0-�������
*****************************************************************************/
int rsrv_do_reply(rtsp_sessn_t * rss, int ecode)
{
    rsrv_cli_info_t * rss_cli = NULL;
    char buf[MAX_STR_LENGTH] = {0};
    int ret = -1;

    rsrv_log(AV_LOG_WARNING, "[fd:%d ip:%s:%d] reply: %s\n", rss->fd, rss->remote_ip_str, rss->remote_port, _get_stat(ecode));

    /* ����RTSP���������RTSPӦ����Ϣ */
    snprintf(buf, RTSP_BUFSIZE, "%s %d %s"RTSP_EL, RTSP_VER, ecode, _get_stat(ecode));

    /* Cseq: */
    snprintf(buf + sstrnlen(buf, MAX_STR_LENGTH), RTSP_BUFSIZE, "CSeq: %d"RTSP_EL, rss->cseq);

    /* Server: */
    snprintf(buf + sstrnlen(buf, MAX_STR_LENGTH), RTSP_BUFSIZE, "Server: %s %s"RTSP_EL, PACKAGE, VERSION);

    /* Date: */
    _add_timestamp(buf, 0);

    /* end of message */
    sstrncat(buf, RTSP_EL, MAX_STR_LENGTH);

    rss->err_code = ecode;

    /*av_log(NULL, AV_LOG_WARNING, "send [%d] reply to [%d]: \n%s", ecode, rss->fd, buf);*/

    rss_cli = rss_get_cli(rss->cli_id);
    if (rss_cli)
    {
        rss_cli->error_code = ecode;
    }

    /* ����RTSPӦ�� */
    ret = rsrv_tcp_write(rss->fd, buf, sstrnlen(buf, MAX_STR_LENGTH));
    if (ret < 0)
    {
        rsrv_log(AV_LOG_ERROR, "send %d reply error", ecode);
        return ERR_GENERIC;
    }

    return ERR_NOERROR;
}

/*
** ��������rtspsrv_do_options
** ��  ����RTSP OPTIONS��������
** ��  ����[in] rtsp RTSP�Ự�ṹ��
** ����ֵ���ɹ�����ERR_NOERROR��������ERR_GENERIC
** ˵  ����
*/
int rsrv_do_options(rtsp_sessn_t * rss)
{
    char url[MAX_STR_LENGTH] = { 0 }, object[MAX_STR_LENGTH] = { 0 }, server[MAX_STR_LENGTH] = { 0 };
    char method[MAX_STR_LENGTH] = { 0 }, ver[MAX_STR_LENGTH] = { 0 }, trash[MAX_STR_LENGTH] = { 0 };
    uint16 port = 0;
    char * p = NULL;
    int ret = 0;

    /* CSeq */
    p = sstrstr(rss->in_buf, HDR_CSEQ);
    if (NULL == p)
    {
        rsrv_do_reply(rss, RTSP_STATUS_BAD_REQUEST);	/* Bad Request */
        return ERR_NOERROR;
    }
    else
    {
        if (sscanf(p, "%254s %4d", trash, &(rss->cseq)) != 2)
        {
            rsrv_do_reply(rss, RTSP_STATUS_BAD_REQUEST);	/* Bad Request */
            return ERR_NOERROR;
        }
    }

    sscanf(rss->in_buf, " %31s %255s %31s ", method, url, ver);

    /* ����URL rtsp://server:port/object */
    ret = _parse_url(url, server, sizeof(server), &port, object, sizeof(object));
    if (1 == ret || -1 == ret)
    {
        _reply_error(rss, ret);
        return ERR_NOERROR;
    }

    /* δ����ֱ��OPTION,�����������,�ñ�־λ */

    if (rss->state != RTSP_STATE_PLAY)
        rsrv_reply_options(rss);
    else
        rss->heart_beat = 1;

    return ERR_NOERROR;
}

int _parse_iframe_nals(uint8 * buf, sint32 buf_size, enc_param_t * enc_param)
{
    uint8 * nal_ptr = buf;
    sint32 nal_size = 0, nal_type, ret, end_of_frame = 0;
    sint32 nal_start = 0, nal_end = 0, deal_size = buf_size;

    while (nal_ptr + nal_size < buf + buf_size)
    {
        ret = rsrv_find_nal_unit(nal_ptr, deal_size, &nal_start, &nal_end, enc_param->enc_type);
        if (ret > 0)
        {
            nal_ptr += nal_start;
            nal_size = nal_end - nal_start;

            end_of_frame = rsrv_check_end_of_frame(enc_param->enc_type, nal_ptr[0], &nal_type);
            if (end_of_frame < 0)
            {
                rsrv_log(AV_LOG_ERROR, "rsrv_check_end_of_frame() err!");
            }

            if (nal_size < MAX_SPS_LENGTH && enc_param->enc_type == VIDEO_H265)
            {
                switch (nal_type)
                {
                    case HEVC_NAL_VPS:
                        memcpy(enc_param->vps_buf, nal_ptr, nal_size);
                        enc_param->vps_len = nal_size;
                        break;
                    case HEVC_NAL_SPS:
                        memcpy(enc_param->sps_buf, nal_ptr, nal_size);
                        enc_param->sps_len = nal_size;
                        break;
                    case HEVC_NAL_PPS:
                        memcpy(enc_param->pps_buf, nal_ptr, nal_size);
                        enc_param->pps_len = nal_size;
                        break;
                }
            }
            else if (nal_size < MAX_SPS_LENGTH && enc_param->enc_type == VIDEO_H264)
            {
                switch (nal_type)
                {
                    case H264_NAL_SPS:
                        memcpy(enc_param->sps_buf, nal_ptr, nal_size);
                        enc_param->sps_len = nal_size;
                        break;
                    case H264_NAL_PPS:
                        memcpy(enc_param->pps_buf, nal_ptr, nal_size);
                        enc_param->pps_len = nal_size;
                        break;
                }
            }

            if (!end_of_frame && deal_size > 0 && nal_end != nal_start)
            {
                nal_ptr += (nal_end - nal_start);
                deal_size -= nal_end;
            }
        }
        else
        {
            return -1;
        }
    }

    return 0;
}

int _parse_iframe(int fifo, int consumer, enc_param_t * enc_param)
{
    MSHEAD mshead = {0}, * pmshead = NULL;
    sint32 size = 0, count = 0, data_size = 0;
    sint32 ret = 0, frame_len = 0, ciframe = HB_TRUE;
    uint8 * frame_buf = NULL, * data = NULL;


    while (count++ < 10)
    {
        if ((size = fifo_readmulti(fifo, consumer, (char *)&mshead, sizeof(MSHEAD), FIFO_PEEK_NO)) <= 0)
        {
            rsrv_log(AV_LOG_ERROR, "_parse_iframe no data read err %x\n", ret);
            return -1;
        }

        if (ISVIDEOFRAME(&mshead))
        {
            if (mshead.algorithm == MSHEAD_ALGORITHM_VIDEO_H265_HISILICON ||
                    mshead.algorithm == MSHEAD_ALGORITHM_VIDEO_H265_GENERAL)
            {
                enc_param->enc_type = VIDEO_H265;
            }
            else if (mshead.algorithm == MSHEAD_ALGORITHM_VIDEO_H264_HISILICON ||
                     mshead.algorithm == MSHEAD_ALGORITHM_VIDEO_H264_STANDARD)
            {
                enc_param->enc_type = VIDEO_H264;
            }
        }

        fifo_ioctrl(fifo, FIFO_CMD_GETDATASIZE, consumer, &data_size, sizeof(sint32));
        if (data_size < MSHEAD_GETFRAMESIZE(&mshead)) /* FIFO���ݲ���һ֡,����ѭ�� */
        {
            return -1;
        }

        if (ciframe && (ISKEYFRAME(&mshead) == 0)) /* ��ҪI֡,�����ݲ���I֡,������֡���� */
        {
            if ((size = fifo_readframemulti(fifo, consumer, NULL, MSHEAD_GETFRAMESIZE(&mshead), FIFO_PEEK_YES)) < 0)
            {
                rsrv_log(AV_LOG_WARNING, "fifo_readframemulti %x\n", ret);
            }
        }
        else
        {
            frame_len = MSHEAD_GETFRAMESIZE(&mshead);
#ifdef _RTSPSRV_TEST
            frame_buf = av_malloc(frame_len);
#else
            frame_buf = mem_manage_malloc(COMP_RTSPSRV, frame_len);
#endif
            if (frame_buf)
            {
                if ((size = fifo_readframemulti(fifo, consumer, (char *)frame_buf, frame_len, FIFO_PEEK_NO)) > 0)
                {
                    pmshead = (MSHEAD *)frame_buf;
                    data_size = MSHEAD_GETMSDSIZE(pmshead);
                    data = (uint8 *)MSHEAD_DATAP(pmshead);

                    ret = _parse_iframe_nals(data, data_size, enc_param);
                    if (ret < 0)
                    {
                        return -1;
                    }
                }
#ifdef _RTSPSRV_TEST
                av_free(frame_buf);
#else
                mem_manage_free(COMP_RTSPSRV, frame_buf);
#endif
                frame_buf = NULL;
                frame_len = 0;
                break;
            }
            else
            {
                rsrv_log(AV_LOG_ERROR, "_parse_iframe malloc err\n");
                return -1;
            }
        }
    }
    return 0;
}

int rsrv_do_describe(rtsp_srv_t * rs_srv, rtsp_sessn_t * rss, int ch_idx)
{
    chstrm_type_e stm_type;
    rtp_chinfo_t * rtp_chinfo = NULL;
    rtsp_param_t * ch_param = NULL;
    char url[MAX_STR_LENGTH] = { 0 }, object[MAX_STR_LENGTH] = { 0 };
    char server[MAX_STR_LENGTH] = { 0 }, trash[MAX_STR_LENGTH] = { 0 };
    char source_filter_line[MAX_SDP_LENGTH] = { 0 }, misc_sdp_lines[MAX_SDP_LENGTH] = { 0 };
    char video_sdp_lines[MAX_SDP_LENGTH] = { 0 }, audio_sdp_lines[MAX_SDP_LENGTH] = { 0 };
    char descr[MAX_DESCR_LENGTH], text_sdp_lines[MAX_SDP_LENGTH] = { 0 };
    uint8 vps_base64[MAX_SPS_LENGTH] = { 0 }, sps_base64[MAX_SPS_LENGTH] = { 0 }, pps_base64[MAX_SPS_LENGTH] = {0};
    int vps_len = 0, sps_len = 0, pps_len;
    uint32 is_ssm = 0;
    uint16 port;
    struct timeval creation_time;
    char * p = NULL;
    int ret = 0, fifo, err_code;;
#ifndef _RTSPSRV_TEST
    PT_STREAM_ACTIVE_S active = {0};
#endif
    rsrv_cli_info_t * rss_cli = NULL;
    enc_param_t enc_param = {0};

    if (rs_srv == NULL || rss == NULL)
    {
        return -1;
    }

    /*��ȡURL*/
    if (!sscanf(rss->in_buf, " %*8s %255s ", url))
    {
        err_code = RTSP_STATUS_BAD_REQUEST;
        goto REPLY_ERROR;
    }

    /*����URL [rtsp://server:port/object]*/

    ret = _parse_url(url, server, sizeof(server), &port, object, sizeof(object));
    if (1 == ret || -1 == ret)
    {
        err_code = ret;
        goto REPLY_ERROR;
    }

    _sessn_set_strmtype(rss, object);
    stm_type = rss->stm_type;

    rtp_chinfo = &rs_srv->chi[ch_idx].rtp_chinfo[stm_type];
    ch_param = &rs_srv->chi[ch_idx].ch_param[stm_type];

    /* ��ȡAccept������ */
    if (sstrstr(rss->in_buf, HDR_ACCEPT) != NULL)
    {
        if (sstrstr(rss->in_buf, "application/sdp") != NULL)
        {
            /*av_log(NULL, AV_LOG_WARNING, "Get application/sdp");*/
        }
        else
        {
            /* ��֧�� */
            err_code = RTSP_STATUS_UNSUPPORTED_OPTION;
            goto REPLY_ERROR;
        }
    }

    /* ��ȡCSeq */
    p = sstrstr(rss->in_buf, HDR_CSEQ);
    if (NULL == p)
    {
        err_code = RTSP_STATUS_BAD_REQUEST;
        goto REPLY_ERROR;
    }
    else
    {
        if (sscanf(p, "%254s %4d", trash, &(rss->cseq)) != 2)
        {
            err_code = RTSP_STATUS_BAD_REQUEST;
            goto REPLY_ERROR;
        }
    }

    /*����SSM�Ự�����, ������Ҫ��������:"a=source-filter: incl ..."*/
    if (is_ssm)
    {
        snprintf(source_filter_line, MAX_SDP_LENGTH,
                 "a=source-filter: incl IN IP4 * %s\r\n"
                 "a=rtcp-unicast: reflection\r\n", rtspsrv_adpt_get_ip());
    }
    else
    {
        sstrcpy(source_filter_line, "");
    }

    if ((rs_srv->sessn_total + 1) <= rs_srv->sessn_max)
    {
        rs_srv->sessn_total++;
        rs_srv->chi[ch_idx].sessn_active++;
    }
    else
    {
        /* Not Enough Bandwidth */
        err_code = RTSP_STATUS_BANDWIDTH;
        goto REPLY_ERROR;
    }


#ifdef _RTSPSRV_TEST
    fifo = rs_srv->chi[ch_idx].fifos[stm_type];
    rss->consumer = fifo_openmulti(fifo, -1, 0, FIFO_HTWATER_SIZE, FIFO_HIWATER_SIZE, FIFO_LOWATER_SIZE, 0);
    rss->fifo = fifo;

#else

    rss->ac_handle = sys_api_open_active_mode(stm_type, rss->remote_ip, "rtspsrv", &active);
    if (rss->ac_handle < 0)
    {
        rsrv_log(AV_LOG_ERROR, "sys_api_active_mode_open err = %x\n", rss->ac_handle);
        return -1;
    }

    rss->consumer = active.consumer;
    fifo = active.fifo;
    rss->fifo = fifo;

#endif

    rsrv_log(AV_LOG_WARNING, "[fd:%d consumer:%d fifo:0x%x]\n", rss->fd, rss->consumer, rss->fifo);

    ret = _parse_iframe(fifo, rss->consumer, &enc_param);
    if (ret < 0)
    {
        rsrv_log(AV_LOG_ERROR, "_parse_iframe err = %x\n", ret);
    }

    if (enc_param.enc_type == VIDEO_H265)
    {
        vps_len = rsrv_get_vps_base64(enc_param.vps_buf, enc_param.vps_len, vps_base64);
        if (vps_len < 0)
        {
            rsrv_log(AV_LOG_ERROR, "get_vps err!\n");
        }
    }

    sps_len = rsrv_get_sps_base64(enc_param.sps_buf, enc_param.sps_len, sps_base64);
    if (sps_len < 0)
    {
        rsrv_log(AV_LOG_ERROR, "get_sps err!\n");
    }

    pps_len = rsrv_get_pps_base64(enc_param.pps_buf, enc_param.pps_len, pps_base64);
    if (pps_len < 0)
    {
        rsrv_log(AV_LOG_ERROR, "get_pps err!\n");
    }

    ch_param->profile_id = 0x640029;
    ch_param->bitrate = 4096;

    /* ��Ƶ��ý��������Ϣ */

    if (enc_param.enc_type == VIDEO_H264)
    {
        snprintf(video_sdp_lines, MAX_SDP_LENGTH,
                 "m=video 0 RTP/AVP 96\r\n"
                 "c=IN IP4 0.0.0.0\r\n"
                 "b=AS:%d\r\n"
                 "a=rtpmap:96 H264/90000\r\n"
                 "a=fmtp:96 packetization-mode=1;profile-level-id=%06x;sprop-parameter-sets=%s,%s\r\n"
                 "a=control:%s/track1\r\n",
                 ch_param->bitrate, ch_param->profile_id, sps_base64, pps_base64, url);
    }
    else if (enc_param.enc_type == VIDEO_H265)
    {
        snprintf(video_sdp_lines, MAX_SDP_LENGTH,
                 "m=video 0 RTP/AVP 96\r\n"
                 "c=IN IP4 0.0.0.0\r\n"
                 "b=AS:%d\r\n"
                 "a=rtpmap:96 H265/90000\r\n"
                 "a=fmtp:96 packetization-mode=1\r\n"
                 "a=control:%s/track1\r\n",
                 ch_param->bitrate, url);
    }

    /* ��Ƶ��ý��������Ϣ */
    snprintf(audio_sdp_lines, MAX_SDP_LENGTH,
             "m=audio 0 RTP/AVP 8\r\n"
             "c=IN IP4 0.0.0.0\r\n"
             "b=AS:64\r\n"
             "a=rtpmap:8 PCMA/8000\r\n"
             "a=control:%s/track2\r\n", url);

    /* ���ܻط�������Ϣ */
    snprintf(text_sdp_lines, MAX_SDP_LENGTH,
             "m=text 0 RTP/AVP 98\r\n"
             "c=IN IP4 0.0.0.0\r\n"
             "a=rtpmap:98 metadata/90000\r\n"
             "a=control:%s/track3\r\n", url);
    rtp_chinfo->en_audio = 0;

    if (rtp_chinfo->en_audio == 1)
        snprintf(misc_sdp_lines, MAX_SDP_LENGTH, "%s%s%s", video_sdp_lines, audio_sdp_lines, text_sdp_lines);
    else
        snprintf(misc_sdp_lines, MAX_SDP_LENGTH, "%s%s", video_sdp_lines, text_sdp_lines);

    gettimeofday(&creation_time, NULL);

    /* ��ȡSDP��Ϣ */
    snprintf(descr, sizeof(descr),
             "v=0\r\n"
             "o=- %ld%06ld %d IN IP4 %s\r\n"
             "s=%s\r\n"
             "i=%s\r\n"
             "t=0 0\r\n"
             "a=tool:%s %s\r\n"
             "a=type:broadcast\r\n"
             "a=control:%s\r\n"
             "%s"
             "a=range:npt=0-\r\n"
             "a=x-qt-text-nam:%s\r\n"
             "a=x-qt-text-inf:%s\r\n"
             "%s",
             creation_time.tv_sec, creation_time.tv_usec, /* o= <session id> */
             1,
             rtspsrv_adpt_get_ip(),           /* o= <address>*/
             DESCRIPTION,              /* s= <description>*/
             object,                   /* i= <info>*/
             PACKAGE, VERSION,         /* a=tool:*/
             url,                      /* a=control:rtsp://server:port/object*/
             source_filter_line,     /* a=source-filter: incl (if a SSM session)*/
             DESCRIPTION,              /* a=x-qt-text-nam: line*/
             object,                   /* a=x-qt-text-inf: line*/
             misc_sdp_lines);         /* miscellaneous session SDP lines (if any)*/

    /*����DESCRIBEӦ��*/
    _reply_describe(rss, url, descr);
    rss->state = RTSP_STATE_DESCRIBER;

    rss_cli = rss_get_cli(rss->cli_id);
    if (rss_cli)
    {
        rss_cli->fifo = rss->fifo;
        rss_cli->consumer = rss->consumer;
        rss_cli->stm_type = rss->stm_type;
        rss_cli->enc_type = enc_param.enc_type;
        rss_cli->state = rss->state;
    }
    return ERR_NOERROR;

REPLY_ERROR:
    /* bad request */
    rsrv_do_reply(rss, err_code);
    return ERR_NOERROR;
}

static void _set_streamtype(rtsp_sessn_t * rss, char * object)
{
    chstrm_type_e stream_type = MAIN_STREAM;

    if (object != NULL && rss != NULL)
    {
        if (!av_strncasecmp(object, GET_MAIN_STREAM, sstrlen(GET_MAIN_STREAM)))
            stream_type = MAIN_STREAM;
        else if (!av_strncasecmp(object, GET_SUB_STREAM1, sstrlen(GET_SUB_STREAM1)))
            stream_type = SUB_STREAM1;
        else if (!av_strncasecmp(object, GET_SUB_STREAM2, sstrlen(GET_SUB_STREAM2)))
            stream_type = SUB_STREAM2;
        else
        {
            stream_type = STREAM_NUM;
        }
    }
    else
    {
        stream_type = STREAM_NUM;
    }

    if (rss->stm_type != stream_type)
    {
        rss->stm_type = stream_type;
    }
}

int _parse_transport(rtp_trans_t * trans, char * trans_str, rtpmd_type_e media_type, int cli_fd, int srv_port)
{
    char * p = NULL, * saved_ptr = NULL, * trans_tkn = NULL;
    struct sockaddr_in s;
    struct sockaddr rtsp_peer;
    socklen_t namelen = sizeof(rtsp_peer);

    if (trans == NULL || trans_str == NULL)
    {
        return -RTSP_STATUS_UNSUPPORTED_MTYPE;
    }
    /* ���������ļ���������ȡ���ӵ������ַ */
    if (getpeername(cli_fd, &rtsp_peer, &namelen) != 0)
    {
        return -RTSP_STATUS_UNSUPPORTED_MTYPE;
    }

    /* ����Transport�ֶ�, �ж��Ƿ���������Transport�ֶ� */
    trans_tkn = av_strtok(trans_str, ",", &saved_ptr);
    if (trans_tkn == NULL)
    {
        rsrv_log(AV_LOG_ERROR, "Malformed Transport string from client");
        return -RTSP_STATUS_BAD_REQUEST;
    }

    do
    {
        /* �ж��Ƿ����RTP/AVP�ֶ� */
        p = sstrstr(trans_tkn, RTSP_RTP_AVP);
        if (p != NULL)
        {
            /* Transport: RTP/AVP*/
            p += sstrlen(RTSP_RTP_AVP);
            /* �ж�RTP/AVP�ֶκ��Ƿ���/TCP�ֶ� */

            /* TCP����*/
            if (0 == sstrncmp(p, "/TCP", 4))
            {
                /* Transport: RTP/AVP/TCP;unicast;interleaved=x-y*/
                _get_rtp_port(trans_tkn, &trans->tcp_info.interleaved, &trans->tcp_tinfo.interleaved, media_type);

                /*trans.resend_time = 0;*/
                trans->tcp_fd = cli_fd;

                /* ����RTP���������� */
                trans->type = RTP_RTP_AVP_TCP;
                break;
            }
            else if (!*p || (*p == ';') || (*p == ' ') || 0 == sstrncmp(p, "/UDP", 4))
            {
                /* UDP����*/
                /*Transport: RTP/AVP;unicast;client_port=x-x����Transport: RTP/AVP/UDP;unicast;client_port=x-x*/
                /*av_log(NULL, AV_LOG_WARNING, "RTSP setup for UDP");*/
                /*�ж��Ƿ����unicast�ֶ�*/
                if (sstrstr(trans_tkn, "unicast") != NULL)
                {
                    /*�ж��Ƿ����client_port�ֶ�*/
                    p = sstrstr(trans_tkn, "client_port");
                    if (p != NULL)
                    {
                        if (media_type == AUDIO)
                        {
                            /*ͨ������client_port�ֶλ�ȡ�ͻ��˵�RTP��RTCP�˿ڶ�*/
                            p = sstrstr(p, "=");
                            sscanf(p + 1, "%2hd", &(trans->udp_ainfo.cli_ports.rtp));
                            p = sstrstr(p, "-");
                            sscanf(p + 1, "%2hd", &(trans->udp_ainfo.cli_ports.rtcp));

                            /*��ȡ�������˵�RTP��RTCP�˿ڶ�*/
                            trans->udp_ainfo.srv_ports.rtp = (uint16)srv_port;
                            trans->udp_ainfo.srv_ports.rtcp = trans->udp_ainfo.srv_ports.rtp + 1;
                        }
                        else
                        {
                            /*//ͨ������client_port�ֶλ�ȡ�ͻ��˵�RTP��RTCP�˿ڶ�*/
                            p = sstrstr(p, "=");
                            sscanf(p + 1, "%2hd", &(trans->udp_vinfo.cli_ports.rtp));
                            p = sstrstr(p, "-");
                            sscanf(p + 1, "%2hd", &(trans->udp_vinfo.cli_ports.rtcp));

                            /*//��ȡ�������˵�RTP��RTCP�˿ڶ�*/
                            trans->udp_vinfo.srv_ports.rtp = (uint16)srv_port;
                            trans->udp_vinfo.srv_ports.rtcp = trans->udp_vinfo.srv_ports.rtp + 1;
                        }
                    }

                    /* ΪRTP��������Ƶ������Ƶ��UDP���ӵ������ַ */
                    if (media_type == M_AUDIO)
                    {
                        trans->rtp_udp_afd = rtspsrv_setup_udp((uint16)(trans->udp_ainfo.srv_ports.rtp), 1);

                        memset(&s, 0x00, sizeof(struct sockaddr_in));
                        s.sin_family = AF_INET;
                        s.sin_addr.s_addr = (*((struct sockaddr_in *)(&rtsp_peer))).sin_addr.s_addr;
                        s.sin_port = htons((uint16)(trans->udp_ainfo.cli_ports.rtp));
                        memcpy(&trans->udp_ainfo.rem_addr, (struct sockaddr *)&s, sizeof(struct sockaddr));
                    }
                    else
                    {
                        trans->rtp_udp_vfd = rtspsrv_setup_udp((uint16)(trans->udp_vinfo.srv_ports.rtp), 1);

                        memset(&s, 0x00, sizeof(struct sockaddr_in));
                        s.sin_family = AF_INET;
                        s.sin_addr.s_addr = (*((struct sockaddr_in *)(&rtsp_peer))).sin_addr.s_addr;
                        s.sin_port = htons((uint16)(trans->udp_vinfo.cli_ports.rtp));

                        memcpy(&trans->udp_vinfo.rem_addr, (struct sockaddr *)&s, sizeof(struct sockaddr));
                    }
                }
                else
                {
                    continue;
                }

                trans->type = RTP_RTP_AVP; /*����RTP����������*/

                break; /* found a valid transport */
            }
        }
        trans_tkn = av_strtok(NULL, ",", &saved_ptr);
    }
    while (trans_tkn != NULL);

    return 0;
}

int rsrv_do_setup(rtsp_srv_t * rs_srv, rtsp_sessn_t * rss)
{
    char object[MAX_STR_LENGTH] = { 0 }, server[MAX_STR_LENGTH] = { 0 };
    char url[MAX_STR_LENGTH] = { 0 }, trans_str[MAX_STR_LENGTH] = { 0 };
    char * p = NULL;
    rtp_sessn_t * rtp_sessn = NULL;
    rtp_trans_t trans = {0};
    rtpmd_type_e media_type;
    uint32 sessn_id = 0;
    uint16 port = 0;
    int ret = 0, err_code = 0;

    if (rs_srv == NULL || rss == NULL)
    {
        return -1;
    }

    rtp_sessn = &rss->rtp_sessn;

    /* ��ȡURL */
    if (!sscanf(rss->in_buf, " %*5s %255s ", url))
    {
        err_code = RTSP_STATUS_BAD_REQUEST;
        ret = ERR_NOERROR;
        goto REPLY_ERROR;
    }

    ret = _parse_url(url, server, sizeof(server), &port, object, sizeof(object));
    if (1 == ret || -1 == ret)
    {
        err_code = RTSP_STATUS_BAD_REQUEST;
        ret = ERR_NOERROR;
        goto REPLY_ERROR;
    }

    /*av_log(NULL, AV_LOG_WARNING, "server[%s] port[%d] object[%s] \n", server, port, object);*/

    _set_streamtype(rss, object);
    if (rss->stm_type == STREAM_NUM)
    {
        err_code = RTSP_STATUS_NOT_FOUND;
        ret = ERR_NOERROR;
        goto REPLY_ERROR;
    }

    /* �ж�����Ƶ����Ƶ���ı� */
    if (NULL != sstrstr(object, "track1"))
    {
        media_type = M_VIDEO;
        rss->rtp_sessn.media_type = rtp_sessn->media_type | 0x01;
    }
    else if (NULL != sstrstr(object, "track2"))
    {
        media_type = M_AUDIO;
        rtp_sessn->media_type = rtp_sessn->media_type | 0x02;
    }
    else
    {
        media_type = M_TEXT;
        rtp_sessn->media_type = rtp_sessn->media_type | 0x04;
    }

    /* ��ȡCSeq */
    p = sstrstr(rss->in_buf, HDR_CSEQ);
    if (p == NULL)
    {
        err_code = RTSP_STATUS_BAD_REQUEST;
        ret = ERR_NOERROR;
        goto REPLY_ERROR;
    }

    if (sscanf(p, "%*5s %4d", &(rss->cseq)) != 1)
    {
        err_code = RTSP_STATUS_BAD_REQUEST;
        ret = ERR_NOERROR;
        goto REPLY_ERROR;
    }

    /* ����Transport�ֶ� */
    /* Transport: RTP/AVP;unicast;client_port=x-x */
    /* Transport: RTP/AVP/TCP;unicast;interleaved=x-x */
    p = sstrstr(rss->in_buf, HDR_TRANSPORT);
    if (NULL == p)
    {
        err_code = RTSP_STATUS_NOT_ACCEPTABLE;
        ret = ERR_NOERROR;
        goto REPLY_ERROR;
    }

    if (sscanf(p, "%*10s%255s", trans_str) != 1)
    {
        rsrv_log(AV_LOG_ERROR, "SETUP request malformed: Transport string is empty");
        err_code = RTSP_STATUS_BAD_REQUEST;
        ret = ERR_NOERROR;
        goto REPLY_ERROR;
    }

    /* ����Transport�ֶ�, �ж��Ƿ���������Transport�ֶ� */
    trans.type = RTP_NO_TRANSPORT;

    ret = _parse_transport(&trans, trans_str, media_type, rss->fd, rs_srv->srv_port);
    if (ret < 0)
    {
        err_code = -ret;
        ret = ERR_GENERIC;
        goto REPLY_ERROR;
    }

    /*�Ϸ����ж�*/
    if (trans.type == RTP_NO_TRANSPORT || trans.type == RTP_RTP_AVP)
    {
        rsrv_log(AV_LOG_ERROR, "Unsupported Transport\n");
        err_code = RTSP_STATUS_TRANSPORT;
        ret = ERR_NOERROR;
        goto REPLY_ERROR;
    }

    /*�ж��Ƿ����Session�ֶΣ����û�У�������һ��Session ID*/
    p = sstrstr(rss->in_buf, HDR_SESSION);
    if (p != NULL)
    {
        if ((sessn_id = _find_sessnid(p)) <= 0)
        {
            /* Session Not Found*/
            err_code = RTSP_STATUS_SESSION;
            ret = ERR_NOERROR;
            goto REPLY_ERROR;
        }
        /*av_log(NULL, AV_LOG_WARNING, "Get session id: %ld\n", session_id);*/
    }
    else
    {
        sessn_id = av_get_random_seed();
        if (0 == sessn_id)
        {
            sessn_id++;
        }
        /*av_log(NULL, AV_LOG_WARNING, "Create session id: 0x%x\n", sessn_id);*/
    }

    /* ��RTP������Ϣ��ֵ */
    rtp_sessn->trans.type = trans.type;
    if (rtp_sessn->trans.type == RTP_RTP_AVP_TCP)
    {
        rtp_sessn->trans.tcp_fd = trans.tcp_fd;
        memcpy(&rtp_sessn->trans.tcp_info, &trans.tcp_info, sizeof(tcp_info_t));
        /*av_log(NULL, AV_LOG_WARNING, "sessn: %p trans.type: %d\n", rtp_sessn, trans.type);*/
    }
    else /*RTP_RTP_AVP*/
    {
        if (media_type == M_AUDIO)
        {
            rsrv_log(AV_LOG_WARNING, "Add audio RTP(UDP) fd: %d", trans.rtp_udp_afd);
            rtp_sessn->trans.rtp_udp_afd = trans.rtp_udp_afd;
            memcpy(&rtp_sessn->trans.udp_ainfo, &trans.udp_ainfo, sizeof(udp_info_t));
        }
        else
        {
            rsrv_log(AV_LOG_WARNING, "Add video RTP(UDP) fd: %d", trans.rtp_udp_vfd);
            rtp_sessn->trans.rtp_udp_vfd = trans.rtp_udp_vfd;
            memcpy(&rtp_sessn->trans.udp_vinfo, &trans.udp_vinfo, sizeof(udp_info_t));
        }
    }

    /* ��ȡ��Ƶ����Ƶ��SSRC */
    rtp_sessn->ssrc = rss->ssrc;

    /* ����RTSP��Session ID*/
    rss->sessn_id = sessn_id;

    /* ����SETUPӦ��*/
    _reply_setup(rss, rtp_sessn, media_type);
    rss->state = RTSP_STATE_SETUP;

    /*av_log(NULL, AV_LOG_WARNING, "rss->sessn_id: 0x%x\n", rss->sessn_id);*/

    return ERR_NOERROR;

REPLY_ERROR:

    /* bad request */
    rsrv_do_reply(rss, err_code);
    return ret;
}

int rsrv_do_play(rtsp_srv_t * rs_srv, rtsp_sessn_t * rss, int ch_idx)
{
    char object[MAX_STR_LENGTH] = { 0 }, server[MAX_STR_LENGTH] = { 0 }, trash[MAX_STR_LENGTH] = { 0 }, url[MAX_STR_LENGTH] = {0};
    char * p = NULL;
    uint16 port;
    sint32 sessn_id, ret = 0, err_code = 0;

    if (rs_srv == NULL || rss == NULL)
    {
        return -1;
    }

    /* ��ȡCSeq */
    p = sstrstr(rss->in_buf, HDR_CSEQ);
    if (NULL == p)
    {
        err_code = RTSP_STATUS_BAD_REQUEST;
        ret = ERR_NOERROR;
        goto REPLY_ERROR;
    }
    else
    {
        if (sscanf(p, "%254s %4d", trash, &(rss->cseq)) != 2)
        {
            err_code = RTSP_STATUS_BAD_REQUEST;
            ret = ERR_NOERROR;
            goto REPLY_ERROR;
        }
    }

    /* ��ȡCSeq */
    p = sstrstr(rss->in_buf, HDR_CSEQ);
    if (NULL == p)
    {
        err_code = RTSP_STATUS_BAD_REQUEST;
        ret = ERR_NOERROR;
        goto REPLY_ERROR;
    }

    /* ��ȡSession ID */
    p = sstrstr(rss->in_buf, HDR_SESSION);
    if (p != NULL)
    {
        if (sscanf(p, "%254s %4u", trash, &sessn_id) != 2)
        {
            err_code = RTSP_STATUS_SESSION; /*Session Not Found*/
            ret = ERR_NOERROR;
            goto REPLY_ERROR;
        }
    }
    else
    {
        err_code = RTSP_STATUS_BAD_REQUEST; /*Bad Request*/
        ret = ERR_NOERROR;
        goto REPLY_ERROR;
    }

    /* ��ȡURL */
    if (!sscanf(rss->in_buf, " %*4s %255s ", url))
    {
        err_code = RTSP_STATUS_BAD_REQUEST; /*Bad Request*/
        ret = ERR_NOERROR;
        goto REPLY_ERROR;
    }

    /* ����URL rtsp://server:port/object */
    ret = _parse_url(url, server, sizeof(server), &port, object, sizeof(object));
    switch (ret)
    {
        case 1:
            err_code = RTSP_STATUS_BAD_REQUEST; /*Bad Request*/
            ret = ERR_NOERROR;
            goto REPLY_ERROR;

        case -1:
            err_code = RTSP_STATUS_INTERNAL; /*internal server error*/
            ret = ERR_NOERROR;
            goto REPLY_ERROR;

        default:
            break;
    }

    _sessn_set_strmtype(rss, object);

    if (_sessn_init_rtpinfo(rs_srv, rss, ch_idx) < 0)
    {
        rss->state = RTSP_STATE_SERVER_ERROR;
        err_code = RTSP_STATUS_INTERNAL; /*internal server error*/
        ret = ERR_NOERROR;
        goto REPLY_ERROR;
    }

    /*����PLAYӦ��*/
    _reply_play(rs_srv, rss, ch_idx, url);

    rss->state = RTSP_STATE_PLAY;

    if (rss->rtp_sessn.trans.type == RTP_RTP_AVP_TCP)
    {
        /*��ӵ���Ӧsrv������*/
        ret = rsrv_ep_add_for_rw(rss->ep_fd, rss->fd, (void *)rss);
        if (ret < 0)
        {
            rsrv_log(AV_LOG_ERROR, "rsrv_ep_add err\n");
            return -1;
        }
    }
    else if (rss->rtp_sessn.trans.type == RTP_RTP_AVP)
    {
        if (rss->rtp_sessn.trans.rtp_udp_afd > 0)
        {
            ret = rsrv_ep_add_for_read(rss->ep_fd, rss->rtp_sessn.trans.rtp_udp_afd, (void *)rss);
            if (ret < 0)
            {
                rsrv_log(AV_LOG_ERROR, "ep_add rtp_udp_afd err\n");
            }
        }

        if (rss->rtp_sessn.trans.rtcp_udp_afd > 0)
        {
            ret = rsrv_ep_add_for_read(rss->ep_fd, rss->rtp_sessn.trans.rtcp_udp_afd, (void *)rss);
            if (ret < 0)
            {
                rsrv_log(AV_LOG_ERROR, "ep_add rtcp_udp_audio err\n");
            }
        }

        if (rss->rtp_sessn.trans.rtp_udp_vfd > 0)
        {
            ret = rsrv_ep_add_for_read(rss->ep_fd, rss->rtp_sessn.trans.rtp_udp_vfd, (void *)rss);
            if (ret < 0)
            {
                rsrv_log(AV_LOG_ERROR, "ep_add rtp_udp_vfd err\n");
            }
        }

        if (rss->rtp_sessn.trans.rtcp_udp_vfd > 0)
        {
            ret = rsrv_ep_add_for_read(rss->ep_fd, rss->rtp_sessn.trans.rtcp_udp_vfd, (void *)rss);
            if (ret < 0)
            {
                rsrv_log(AV_LOG_ERROR, "ep_add rtcp_udp_video err\n");
            }
        }
    }

    return 0;

REPLY_ERROR:
    /* bad request */
    rsrv_do_reply(rss, err_code);
    return ret;
}

int rsrv_do_pause(rtsp_sessn_t * rss)
{
    char url[MAX_STR_LENGTH], method[MAX_STR_LENGTH];
    char ver[MAX_STR_LENGTH], trash[MAX_STR_LENGTH];
    char * p = NULL;

    p = sstrstr(rss->in_buf, HDR_CSEQ);
    if (NULL == p)
    {
        rsrv_do_reply(rss, RTSP_STATUS_BAD_REQUEST);	/* Bad Request */
        return ERR_NOERROR;
    }
    else
    {
        if (sscanf(p, "%254s %4d", trash, &(rss->cseq)) != 2)
        {
            rsrv_do_reply(rss, RTSP_STATUS_BAD_REQUEST);	/* Bad Request */
            return ERR_NOERROR;
        }
    }

    sscanf(rss->in_buf, " %31s %255s %31s ", method, url, ver);

    rss->state = RTSP_STATE_PAUSE;
    _reply_pause(rss);
    return ERR_NOERROR;
}

int rsrv_do_teardown(rtsp_srv_t * rs_srv, rtsp_sessn_t * rss, int ch_idx)
{
    char * ptr = NULL;
    char object[MAX_STR_LENGTH], server[MAX_STR_LENGTH];
    char url[MAX_STR_LENGTH], trash[MAX_STR_LENGTH];
    unsigned short port;
    int ret = 0, err_code = 0;

    /*��ȡCSeq*/
    ptr = sstrstr(rss->in_buf, HDR_CSEQ);
    if (NULL == ptr)
    {
        err_code = RTSP_STATUS_BAD_REQUEST;
        ret = ERR_PARSE;
        goto REPLY_ERROR;
    }
    else
    {
        if (sscanf(ptr, "%254s %4d", trash, &(rss->cseq)) != 2)
        {
            err_code = RTSP_STATUS_BAD_REQUEST;
            ret = ERR_PARSE;
            goto REPLY_ERROR;
        }
    }

    /*��ȡURL*/
    if (!sscanf(rss->in_buf, " %*8s %255s ", url))
    {
        err_code = RTSP_STATUS_BAD_REQUEST;
        ret = ERR_PARSE;
        goto REPLY_ERROR;
    }

    /*av_log(NULL, AV_LOG_WARNING, "url: %s\n", url);*/

    ret = _parse_url(url, server, sizeof(server), &port, object, sizeof(object));
    if (1 == ret || -1 == ret)
    {
        err_code = RTSP_STATUS_BAD_REQUEST;
        ret = ERR_NOERROR;
        goto REPLY_ERROR;
    }

    /* ��ȡSession ID */
    ptr = sstrstr(rss->in_buf, HDR_SESSION);
    if (ptr != NULL)
    {
        if ((rss->sessn_id = _find_sessnid(ptr)) <= 0)
        {
            err_code = RTSP_STATUS_SESSION;/* Session Not Found */
            ret = ERR_PARSE;
            goto REPLY_ERROR;
        }
    }
    else
    {
        rss->sessn_id = 0;
    }

    /* ����Session ID��RTSP�Ự�����в��Ҷ�Ӧ�Ľڵ�*/
    ret = _sessn_search_teardown(rs_srv, rss, ch_idx);
    if (ret < 0)
    {
        err_code = RTSP_STATUS_SESSION;/* Session Not Found */
        ret = ERR_PARSE;
        goto REPLY_ERROR;
    }

    /* ����TEARDOWNӦ��*/
    _reply_teardown(rss);
    return ERR_NOERROR;

REPLY_ERROR:

    /* bad request */
    rsrv_do_reply(rss, err_code);
    return ret;
}

/******************************************************************************
* ��������: RTSP��������
* �������: rs_srv: RTSP server ������
*				  ch_idx: ͨ����
* �������: ��
* ����ֵ  : �ɹ�����ERR_NOERROR��������ERR_GENERIC
* ˵  ��  : ����inputbuffer�н��յ������ݣ��ж���ʲô����
*           ȫ��������:OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE, GET_PARAMETER, SET_PARAMETER
*           ֧�ֵ�����:OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY
*****************************************************************************/
int rsrv_do_dispatch(rtsp_srv_t * rs_srv, rtsp_sessn_t * rss, int ch_idx)
{
    int ret = -1;

    if (0 == sstrncmp(rss->in_buf, "OPTIONS", sstrlen("OPTIONS")))
    {
        /* OPTIONS����*/
        ret = rsrv_do_options(rss);
    }
    else if (0 == sstrncmp(rss->in_buf, "DESCRIBE", sstrlen("DESCRIBE")))
    {
        /* DESCRIBE����*/
        ret = rsrv_do_describe(rs_srv, rss, ch_idx);
    }
    else if (0 == sstrncmp(rss->in_buf, "SETUP", sstrlen("SETUP")))
    {
        /* SETUP����*/
        ret = rsrv_do_setup(rs_srv, rss);
    }
    else if (0 == sstrncmp(rss->in_buf, "PLAY", sstrlen("PLAY")))
    {
        /* PLAY����*/
        ret = rsrv_do_play(rs_srv, rss, ch_idx);
    }
    else if (0 == sstrncmp(rss->in_buf, "TEARDOWN", sstrlen("TEARDOWN")))
    {
        /* TEARDOWN����*/
        ret = rsrv_do_teardown(rs_srv, rss, ch_idx);
    }
    else if (0 == sstrncmp(rss->in_buf, "PAUSE", sstrlen("PAUSE")))
    {
        ret = rsrv_do_pause(rss);
    }
    else
    {
        if (0 == sstrncmp(rss->in_buf, "ANNOUNCE", sstrlen("ANNOUNCE"))
                || 0 == sstrncmp(rss->in_buf, "PAUSE", sstrlen("PAUSE"))
                || 0 == sstrncmp(rss->in_buf, "RECORD", sstrlen("RECORD"))
                || 0 == sstrncmp(rss->in_buf, "REDIRECT", sstrlen("REDIRECT"))
                || 0 == sstrncmp(rss->in_buf, "GET_PARAMETER", sstrlen("GET_PARAMETER"))
                || 0 == sstrncmp(rss->in_buf, "SET_PARAMETER", sstrlen("SET_PARAMETER")))
        {
            ret = rsrv_do_reply(rss, RTSP_STATUS_METHOD); /*Method Not Allowed*/
            return ret;
        }
        else
        {
            return ERR_GENERIC;
        }
    }

    return ret;
}

/******************************************************************************
* ��������: RTSP ��������
* �������: rs_srv: RTSP server ������
*				  rss: RTSP session ʵ��
*				  ch_idx: ͨ����
* �������: ��
* ����ֵ  : >0-д�����ݴ�С, <0-�������
*****************************************************************************/
int rsrv_do_request(rtsp_srv_t * rs_srv, rtsp_sessn_t * rss, int ch_idx)
{
    int ret;

    if (rss == NULL || rss->in_buf == NULL)
    {
        return -1;
    }

    /* ������������ */
    memset(rss->in_buf, 0x00, RTSP_BUFSIZE);

    /* �������ļ��������ж�ȡ���� */
    rss->in_size = rsrv_tcp_read(rss->fd, (void *)rss->in_buf, RTSP_BUFSIZE);
    if (rss->in_size <= 0)
    {
        /*av_log(NULL, AV_LOG_ERROR, "rsrv_tcp_read error, rtsp fd = %d, errno = %d\n", rss->fd, errno);*/
        rss->state = RTSP_STATE_TEARDOWN;
        return -1;
    }

    /* �ų�RTCP�Ľ��� */
    if ((rss->in_buf[0] >= 'a' && rss->in_buf[0] <= 'z')
            || (rss->in_buf[0] >= 'A' && rss->in_buf[0] <= 'Z'))
    {
        ret = rsrv_do_dispatch(rs_srv, rss, ch_idx);
        /* RTSP������ */
        if (ret < 0)
        {
            rsrv_log(AV_LOG_WARNING, "rtsp dispatch err = %d\n", ret);
            return ret;
        }
    }

    return 0;
}

/******************************************************************************
* ��������: �����½�socket��������
* �������: rs_srv: RTSP server ������
*				  ch_idx: ͨ����
* �������: ��
* ����ֵ  : >0-д�����ݴ�С, <0-�������
*****************************************************************************/
int rsrv_do_accept(rtsp_srv_t * rs_srv, int ch_idx)
{
    rtsp_sessn_t * rss = NULL;
    sint32 client_socket;
    uint16 remote_port = 0;
    char remote_ip_str[16] = {0};
    rsrv_cli_info_t * rss_cli = NULL;

    client_socket = rsrv_tcp_accept(rs_srv->srv_fd, remote_ip_str, &remote_port);
    if (client_socket > 0)
    {
        /*av_log(NULL, AV_LOG_WARNING, "a new client connet, socket = %d\n", client_socket);*/

        /* ���������ļ�������������Ϊ������ģʽ */
        if (rtspsrv_set_non_blocking(client_socket) < 0)
        {
            rsrv_log(AV_LOG_ERROR, "rtspsrv_set_non_blocking err\n");
        }

        /* ��TCPЭ��������KEEPALIVE��⣬��������UDP����RTP������¼������Ͽ� */
        if (rtspsrv_set_keep_alive(client_socket) < 0)
        {
            rsrv_log(AV_LOG_ERROR, "make_socket_keep_alive()");
        }

        /* TCP����RTP������£������ر�socket */
        /*if (rtspsrv_set_linger(client_socket) < 0)
        {
            av_log(NULL, AV_LOG_ERROR, "set_socket_linger() err");
        }*/

#ifdef _RTSPSRV_TEST
        /* ΪRTSP�������ռ� */
        rss = (rtsp_sessn_t *)av_mallocz(sizeof(rtsp_sessn_t));
#else
        rss = (rtsp_sessn_t *)mem_manage_malloc(COMP_RTSPSRV, sizeof(rtsp_sessn_t));
#endif

        if (rss == NULL)
        {
            rsrv_log(AV_LOG_ERROR, "Could not alloc memory\n");
            return -1;
        }

        memset(rss, 0x00, sizeof(rtsp_sessn_t));

        /* RTSP�Ự��ʼ�� */
        if (rsrv_sessn_init(rss, client_socket, rs_srv->chi[ch_idx].ep_fd, ch_idx) < 0)
        {
            rsrv_log(AV_LOG_ERROR, "rtsp_session_init()\n");
            return -1;
        }

        memcpy(rss->remote_ip_str, remote_ip_str, 16);
        rss->remote_port = remote_port;
        inet_pton(AF_INET, remote_ip_str, (void *)&rss->remote_ip);
        rss->remote_ip = ntohl(rss->remote_ip);

        /* ��Ӵ�rtsp�Ự�������� */

        rs_srv->chi[ch_idx].sessn_lst = n_slist_append(rs_srv->chi[ch_idx].sessn_lst, rss);

        if (rs_srv->chi[ch_idx].sessn_lst == NULL)
        {
            rsrv_log(AV_LOG_ERROR, "add_rtsp_session_tail()\n");
            /*rsrv_sessn_free(rss);*/
            return -1;
        }

        /*av_log(NULL, AV_LOG_WARNING, "remote ip %s:%d fd:%d\n", rss->remote_ip_str, rss->remote_port, rss->fd);*/

        /* ��������ļ���������epoll�ļ��������� */
        if (rsrv_ep_add_for_read(rss->ep_fd, client_socket, (void *)rss) < 0)
        {
            rsrv_log(AV_LOG_ERROR, "rtsp_ep_add() error!\n");
            return -1;
        }

        rss_cli = rss_get_cli(rss->cli_id);
        if (rss_cli)
        {
            sstrncpy((char *)rss_cli->rem_ip_str, (char *)rss->remote_ip_str, 16);
            rss_cli->remote_port = rss->remote_port;
            rss_cli->state = rss->state;
        }
    }
    else
    {
        rsrv_log(AV_LOG_WARNING, "tcp_accept()\n");
        return -1;
    }

    return 0;
}

