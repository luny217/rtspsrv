/*
 * FileName:       rtsp_srv.c
 * Author:         luny  Version: 1.0  Date: 2017-5-15
 * Description:
 * Version:
 * Function List:
 *                 1.
 * History:
 *     <author>   <time>    <version >   <desc>
 */

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#ifndef _WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/prctl.h>
#endif
#include <errno.h>
#include <sys/epoll.h>

#include "avformat_network.h"
#include "avutil_time.h"
#include "avutil_avstring.h"
#include "avutil_random_seed.h"

#include "sstring.h"
#include "fifo.h"
#include "rtspsrv_session.h"
#include "rtspsrv_epoll.h"
#include "rtspsrv_stream.h"
#include "rtspsrv_handler.h"
#include "rtspsrv_rtp.h"
#include "rtspsrv_utils.h"
#include "rtspsrv.h"

#ifndef _RTSPSRV_TEST
#include "mem_manage.h"
#include "sys_netapi.h"
#include "cli_stat.h"
#else
#include "avutil_log.h"
#endif

#define DEFAULT_MAX_SRV 2

static int rsrv_process_request(rtsp_srv_t * rs_srv, int ch_idx);

int rsrv_dispatcher_running = 1;
int rsrv_worker_running = 1;

pthread_t rsrv_worker_pt[DEFAULT_MAX_SRV];
pthread_t rsrv_dispatcher_pt;

rtsp_srv_t * rtsp_srv_fd = NULL;

int rss_stat_info(char * line);

static int _rtp_info_init(rtsp_srv_t * rs_srv, int ch_idx)
{
    int ch_strm;
    rtp_hdr_t * video_hdr = NULL;
    rtp_hdr_t * audio_hdr = NULL;
    rtp_hdr_t * text_hdr = NULL;
    rtp_chinfo_t * ch_info = NULL;
    rtp_info_t * vinfo = NULL;
    rtp_info_t * ainfo = NULL;
    rtp_info_t * tinfo = NULL;

    if (rs_srv == NULL)
    {
        return -1;
    }

    for (ch_strm = 0; ch_strm < CH_STRM_MAX; ch_strm++)
    {
        ch_info = &rs_srv->chi[ch_idx].rtp_chinfo[ch_strm];

        video_hdr = &ch_info->mdstrms[M_VIDEO].rtp_hdr;
        audio_hdr = &ch_info->mdstrms[M_AUDIO].rtp_hdr;
        text_hdr = &ch_info->mdstrms[M_TEXT].rtp_hdr;

        vinfo = &ch_info->mdstrms[M_VIDEO];
        ainfo = &ch_info->mdstrms[M_AUDIO];
        tinfo = &ch_info->mdstrms[M_TEXT];

        /* 初始化视频RTP信息 */
        video_hdr->payload = PAYLOAD_TYPE_H264;
        vinfo->ssrc = video_hdr->ssrc = com_random32();
        vinfo->channel_id = VIDEO_RTP;
        video_hdr->version = RTP_VERSION;
        video_hdr->seq_no = (uint16)(com_rand() % 0xFFFF);
        video_hdr->timestamp = com_random32();

        /* 初始化音频RTP信息 */
        audio_hdr->payload = PAYLOAD_TYPE_PCMA;
        ainfo->ssrc = audio_hdr->ssrc = com_random32();
        vinfo->channel_id = AUDIO_RTP;
        audio_hdr->version = RTP_VERSION;
        audio_hdr->seq_no = (uint16)(com_rand() % 0xFFFF);
        audio_hdr->timestamp = com_random32();

        /* 初始化文本RTP信息, 主码流携带自定义文本信息 */
        text_hdr->payload = PAYLOAD_TYPE_TEXT;
        tinfo->ssrc = text_hdr->ssrc = com_random32();
        text_hdr->version = RTP_VERSION;
        text_hdr->seq_no = (uint16)(com_rand() % 0xFFFF);
        text_hdr->timestamp = com_random32();
    }
    return 0;
}

static int _process_timeout(rtsp_sessn_t * rss, sint64 time_gap_ms)
{
    if (rss == NULL)
    {
        rsrv_log(AV_LOG_ERROR, "_process_timeout rss is NULL\n");
        return -1;
    }

    if ((rss->rtp_sessn.trans.type == RTP_RTP_AVP) || (rss->state == RTSP_STATE_PAUSE))
    {
        return 0;
    }

    if ((time_gap_ms > MIN_NO_FRM_VAL) && (time_gap_ms < MAX_NO_FRM_VAL))
    {
        if (rss->state == RTSP_STATE_PLAY)
        {
            if (rss->timeout_cnt == 0)
            {
                char test_buf[8192] = { 0 };

                rss->try_snd_len = send(rss->fd, test_buf, sizeof(test_buf), 0);
                if (rss->try_snd_len < 0)
                {
                    return -1;
                }
                rsrv_log(AV_LOG_ERROR, "[fd: %d ip:%s:%d try_snd_len:%d need_send:%d mnow_s:%lld]\n",
                       rss->fd, rss->remote_ip_str, rss->remote_port, rss->try_snd_len, sizeof(test_buf), time_gap_ms);
            }
            rss->timeout_cnt++;
        }
        return 0;
    }

    if (time_gap_ms > MAX_NO_FRM_VAL / 2)
    {
        rsrv_log(AV_LOG_ERROR, "[fd: %d ip: %s:%d mnow_s:%lld timeout_cnt:%d]\n",
               rss->fd, rss->remote_ip_str, rss->remote_port, time_gap_ms, rss->timeout_cnt);
        return -1;
    }
    return 0;
}

/******************************************************************************
* 函数介绍: 从RTSP链表中删除一个RTSP会话
* 输入参数: rs_srv: RTSP server 管理句柄
*				  rss: RTSP session实例
*				  ch_idx: 通道号
* 输出参数: 无
* 返回值  : >0-写入数据大小, <0-错误代码
*****************************************************************************/
int rsrv_sessn_del(rtsp_srv_t * rs_srv, rtsp_sessn_t * rss, int ch_idx)
{
    rtp_trans_t * trans = NULL;
    n_slist_t * sessn_list = NULL;
    rsrv_cli_info_t * rss_cli = NULL;
    int ret = -1;

    if (rs_srv == NULL || rss == NULL)
    {
        rsrv_log(AV_LOG_ERROR, "_sessn_del rs_srv is NULL\n");
        return -1;
    }

    sessn_list = rs_srv->chi[ch_idx].sessn_lst;

    if (sessn_list == NULL)
    {
        rsrv_log(AV_LOG_ERROR, "_sessn_del sessn_list is NULL\n");
        return -1;
    }

    /*av_log(NULL, AV_LOG_ERROR, "[id:%d fd:%d] del RTSP session [%d]\n", rss->self_idx, rss->fd, cli_fd);*/

    trans = &rss->rtp_sessn.trans;

    rss_cli = rss_get_cli(rss->cli_id);
    if (rss_cli)
    {
        rss_cli->close_tm = av_gettime();
        rss_cli->state = rss->state;
        rss_cli->fps = rss->fps;
        rss_cli->bitrate = rss->cbitrate;
        rss_cli->vframe_count = rss->cvfr_cnt;
        rss_cli->iframe_count = rss->ciframe;
        rss_cli->aframe_count = rss->cafr_cnt;
        rss_cli->vdrop_count = rss->drop_frame;

        rss_cli->high_water_cnt = rss->high_water_cnt;
        rss_cli->max_snd_time = rss->max_snd_time;
        rss_cli->timeout_cnt = rss->timeout_cnt;
        rss_cli->try_snd_len = rss->try_snd_len;

        rss_cli->state = rss->state;
        tcp_info(rss->fd, &rss_cli->rcvq_buf, &rss_cli->sndq_buf);
        /*av_log(NULL, AV_LOG_WARNING, "[id:%d] rcvq_buf:%d sndq_buf:%d\n", rss->cli_id, rss_cli->rcvq_buf, rss_cli->sndq_buf);*/
    }

    /* 判断是TCP还是UDP，关闭对应的网络文件描述符 */
    if (trans->type == RTP_RTP_AVP)
    {
        if (trans->rtp_udp_vfd > 0)
        {
            rsrv_log(AV_LOG_WARNING, "close video UDP fd: %d\n", trans->rtp_udp_vfd);
            closesocket(trans->rtp_udp_vfd);
            trans->rtp_udp_vfd = -1;
        }

        if (trans->rtp_udp_afd > 0)
        {
            rsrv_log(AV_LOG_WARNING, "close audio UDP fd: %d\n", trans->rtp_udp_afd);
            closesocket(trans->rtp_udp_afd);
            trans->rtp_udp_afd = -1;
        }

        if (trans->rtp_udp_tfd > 0)
        {
            rsrv_log(AV_LOG_WARNING, "close text UDP fd: %d\n", trans->rtp_udp_tfd);
            closesocket(trans->rtp_udp_tfd);
            trans->rtp_udp_tfd = -1;
        }

        if (trans->rtcp_udp_vfd > 0)
        {
            rsrv_log(AV_LOG_WARNING, "close video UDP fd: %d\n", trans->rtcp_udp_vfd);
            closesocket(trans->rtcp_udp_vfd);
            trans->rtcp_udp_vfd = -1;
        }

        if (trans->rtcp_udp_afd > 0)
        {
            rsrv_log(AV_LOG_WARNING, "close audio UDP fd: %d\n", trans->rtcp_udp_afd);
            closesocket(trans->rtcp_udp_afd);
            trans->rtcp_udp_afd = -1;
        }
    }
    else /* RTP_RTP_AVP_TCP */
    {
        if (trans->tcp_fd > 0)
        {
            /*av_log(NULL, AV_LOG_WARNING, "Close RTP(TCP) fd: %d\n", trans->tcp_fd);*/
            closesocket(trans->tcp_fd);
            trans->tcp_fd = -1;
        }
    }

    if (trans->wv_info.buf != NULL)
    {
#ifdef _RTSPSRV_TEST
        av_free(trans->wv_info.buf);
#else
        mem_manage_free(COMP_RTSPSRV, trans->wv_info.buf);
#endif

        trans->wv_info.buf = NULL;
    }

    if (rss->fd > 0)
    {
        /*av_log(NULL, AV_LOG_WARNING, "[id:%d fd:%d]close RTSP socket\n", rss->ch_idx, rss->fd);*/
        closesocket(rss->fd);
        rss->fd = -1;
    }

    if (rss->fifo > 0 && rss->consumer >= 0)
    {
#ifdef _RTSPSRV_TEST
        ret = fifo_closemulti(rss->fifo, rss->consumer);
#else
        ret = sys_api_close_active_mode(rss->ac_handle, rss->stm_type, rss->fifo, rss->consumer);
#endif
        if (ret < 0)
        {
            rsrv_log(AV_LOG_ERROR, "fifo_close err %x\n", ret);
        }
        rss->consumer = -1;
    }

    /* 在链表中删除此rtsp会话 , 返回的链表可能为NULL*/
    rs_srv->chi[ch_idx].sessn_lst = n_slist_remove(rs_srv->chi[ch_idx].sessn_lst, rss);

    if (rss->in_buf)
    {
#ifdef _RTSPSRV_TEST
        av_free(rss->in_buf);
#else
        mem_manage_free(COMP_RTSPSRV, rss->in_buf);
#endif
        rss->in_buf = NULL;
    }

    if (rss->out_buf)
    {
#ifdef _RTSPSRV_TEST
        av_free(rss->out_buf);
#else
        mem_manage_free(COMP_RTSPSRV, rss->out_buf);
#endif
        rss->out_buf = NULL;
    }

    if (rss)
    {
#ifdef _RTSPSRV_TEST
        av_free(rss);
#else
        mem_manage_free(COMP_RTSPSRV, rss);
#endif
        rss = NULL;
    }

    return 0;
}

/******************************************************************************
* 函数介绍: RTSP session初始化
* 输入参数: rss: RTSP session实例
*				  fd: socket 句柄
*				  ch_idx: 通道号
* 输出参数: 无
* 返回值  : >0-写入数据大小, <0-错误代码
*****************************************************************************/
int rsrv_sessn_init(rtsp_sessn_t * rss, int fd, int ep_fd, int ch_idx)
{
    rsrv_cli_info_t * rss_cli = NULL;    
    
    if (rss == NULL || fd < 0)
    {
        rsrv_log(AV_LOG_ERROR, "sessn_init is NULL\n");
        return -1;
    }

    rss->in_size = RTSP_BUFSIZE;
    rss->out_size = RTSP_BUFSIZE;

#ifdef _RTSPSRV_TEST
    rss->in_buf = av_malloc(rss->in_size);
#else
    rss->in_buf = mem_manage_malloc(COMP_RTSPSRV, rss->in_size);
#endif

    if (rss->in_buf == NULL)
    {
        rsrv_log(AV_LOG_ERROR, "sessn_init alloc imem err\n");
        return -1;
    }

    memset(rss->in_buf, 0x00, rss->in_size);

#ifdef _RTSPSRV_TEST
    rss->out_buf = av_malloc(rss->out_size);
#else
    rss->out_buf = mem_manage_malloc(COMP_RTSPSRV, rss->out_size);
#endif

    if (rss->out_buf == NULL)
    {
        rsrv_log(AV_LOG_ERROR, "sessn_init alloc omem err\n");
        goto SESSN_INIT_ERR;
    }

    memset(rss->out_buf, 0x00, rss->out_size);

    rss->fd = fd;
    rss->ep_fd = ep_fd;
    rss->ch_idx = ch_idx;
    rss->err_code = 0;
    rss->left_size = 0;
    rss->frame_buf = NULL;
    rss->frame_len = 0;
    rss->video_base_timestamp = av_get_random_seed();
    rss->audio_base_timestamp = av_get_random_seed();
    rss->last_enc_type = -1;
    rss->last_frame_width = -1;
    rss->last_frame_height = -1;

    rss->rtp_sessn.trans.tcp_fd = fd;
    rss->rtp_sessn.trans.rtp_udp_vfd = -1;
    rss->rtp_sessn.trans.rtp_udp_afd = -1;
    rss->update_time = 0;
    rss->max_snd_time = 0;
    rss->last_snd_time = av_gettime_relative();
    rss->rtp_sessn.trans.wv_info.buf_size = RTSP_WV_BUFSIZE;

#ifdef _RTSPSRV_TEST
    rss->rtp_sessn.trans.wv_info.buf = av_malloc(RTSP_WV_BUFSIZE);
#else
    rss->rtp_sessn.trans.wv_info.buf = mem_manage_malloc(COMP_RTSPSRV, RTSP_WV_BUFSIZE);
#endif

    if (rss->rtp_sessn.trans.wv_info.buf == NULL)
    {
        rsrv_log(AV_LOG_ERROR, "sessn_init alloc mem err\n");
        goto SESSN_INIT_ERR;
    }

    memset(rss->rtp_sessn.trans.wv_info.buf, 0x00, RTSP_WV_BUFSIZE);

    rss->cli_id = rss_take_unused_cli();    
    rss_cli = rss_get_cli(rss->cli_id);
    if (rss_cli)
    {
        memset(rss_cli, 0x00, sizeof(rsrv_cli_info_t));
        rss_cli->used = 1;
        rss_cli->open_tm = av_gettime();        
        rss_cli->fd = rss->fd;
        rss_cli->rcvq_buf = -1;
        rss_cli->sndq_buf = -1;
    }

    return 0;

SESSN_INIT_ERR:

    if (rss->in_buf)
    {
#ifdef _RTSPSRV_TEST
        av_free(rss->in_buf);
#else
        mem_manage_free(COMP_RTSPSRV, rss->in_buf);
#endif
        rss->in_buf = NULL;
    }

    if (rss->out_buf)
    {
#ifdef _RTSPSRV_TEST
        av_free(rss->out_buf);
#else
        mem_manage_free(COMP_RTSPSRV, rss->out_buf);
#endif
        rss->out_buf = NULL;
    }

    if (rss->rtp_sessn.trans.wv_info.buf)
    {
#ifdef _RTSPSRV_TEST
        av_free(rss->rtp_sessn.trans.wv_info.buf);
#else
        mem_manage_free(COMP_RTSPSRV, rss->rtp_sessn.trans.wv_info.buf);
#endif
        rss->rtp_sessn.trans.wv_info.buf = NULL;
    }

    if (rss->rtp_sessn.trans.wv_info.frame_buf)
    {
#ifdef _RTSPSRV_TEST
        av_free(rss->rtp_sessn.trans.wv_info.frame_buf);
#else
        mem_manage_free(COMP_RTSPSRV, rss->rtp_sessn.trans.wv_info.frame_buf);
#endif
        rss->rtp_sessn.trans.wv_info.frame_buf = NULL;
    }

    return -1;
}

/******************************************************************************
* 函数介绍: 遍历RTSP链表, 查找失效节点
* 输入参数: rs_srv: RTSP server 管理句柄
*				  ch_idx: 通道号
* 输出参数: 无
* 返回值  : >0-写入数据大小, <0-错误代码
*****************************************************************************/
int rsrv_sessn_trip(rtsp_srv_t * rs_srv, int ch_idx)
{
    n_slist_t * item = NULL;
    n_slist_t * sessn_list = NULL;
    rtsp_sessn_t * rss = NULL;

    sint64 time_gap_ms = 0;
    int ret = -1, dispatched = 0;

    if (rs_srv == NULL || rs_srv->chi == NULL)
    {
        return -1;
    }

    sessn_list = rs_srv->chi[ch_idx].sessn_lst;

    for (item = sessn_list; item; item = item->next)
    {
        rss = (rtsp_sessn_t *)item->data;
        if (rss == NULL)
        {
            continue;
        }

        dispatched++;

        time_gap_ms = (av_gettime_relative() - rss->last_snd_time) / 1000;

        if ((_process_timeout(rss, time_gap_ms) < 0) || (rss->fd <= 0) ||
                (rss->state == RTSP_STATE_TEARDOWN) || (rss->state == RTSP_STATE_SERVER_ERROR))
        {
			if (rs_srv->sessn_total > 0 && rss->err_code == 0)
			{
				rs_srv->sessn_total--;
			}

			if (rs_srv->chi[ch_idx].sessn_active > 0 && rss->err_code == 0)
			{
				rs_srv->chi[ch_idx].sessn_active--;
			}

            rsrv_ep_del(rss->ep_fd, rss->fd, (void *)rss);

            rsrv_log(AV_LOG_WARNING, "remove sessn [fd:%d ip:%s consumer:%d state:%d err:%d]\n",
                   rss->fd,  rss->remote_ip_str, rss->consumer, rss->state, rss->err_code);

            rsrv_sessn_del(rs_srv, rss, ch_idx);            

            break; /* 一次只释放一个节点，否则，循环无法继续，造成段错误 */
        }

        if ((RTSP_STATE_PLAY == rss->state) && (rss->rtp_sessn.trans.type == RTP_RTP_AVP))
        {
            ret = rstrm_send_data(rs_srv, rss, ch_idx);
            if (ret < 0)
            {
                /* av_log(NULL, AV_LOG_WARNING, "[id:%d fd:%d]rsrv_send_data() err\n", rss->ch_idx, rss->fd); */
            }
        }
    }
    return 0;
}

/******************************************************************************
* 函数介绍: 等待RTSP的epoll文件描述符的I/O事件，并处理请求、发送数据
* 输入参数: rs_srv: RTSP server 管理句柄
*				  ch_idx: 通道号
* 输出参数: 无
* 返回值  : >0-写入数据大小, <0-错误代码
*****************************************************************************/
int rsrv_process_request(rtsp_srv_t * rs_srv, int ch_idx)
{
    int i, ret = -1;
    int nfds = -1, srv_fd = -1, ep_fd;
    uint32 revents = 0;
    rtsp_sessn_t * rss = NULL;
    struct epoll_event * events_list = NULL;

    if (rs_srv == NULL)
    {
        rsrv_log(AV_LOG_WARNING, "rs_srv is NULL\n");
        return -1;
    }

    events_list = rs_srv->chi[ch_idx].ev;
    ep_fd = rs_srv->chi[ch_idx].ep_fd;

    nfds = epoll_wait(ep_fd, events_list, EV_MAX, 0);
    if (nfds == -1)
    {
        rsrv_log(AV_LOG_ERROR, "epoll_wait() err");
        return -1;
    }

    for (i = 0; i < nfds; i++)
    {
        revents = events_list[i].events;
        srv_fd = (sint32)events_list[i].data.ptr;

        if (srv_fd == rs_srv->srv_fd) /* 新的connect */
        {
            if (revents & EPOLLIN)
            {
                ret = rsrv_do_accept(rs_srv, ch_idx);
                if (ret < 0)
                {
                    rsrv_log(AV_LOG_ERROR, "rtsp_do_accept() err\n");
                }
            }
        }
        else
        {
            rss = events_list[i].data.ptr;

            if (rss == NULL)
            {
                rsrv_log(AV_LOG_ERROR, "[i: %d data: %p]rss is NULL\n", i, events_list[i].data.ptr);
            }

            if (revents & EPOLLERR)
            {
                int status, err;
				socklen_t len = sizeof(err);
                status = getsockopt(rss->fd, SOL_SOCKET, SO_ERROR, (char*)&err, &len);
                rss->state = RTSP_STATE_SERVER_ERROR;

                rsrv_log(AV_LOG_ERROR, "[fd:%d ip:%s:%d stream:%d errno:%d SO_ERROR(%d:%d)]\n",
                       rss->fd, rss->remote_ip_str, rss->remote_port, rss->stm_type, ff_neterrno(), status, err);
                continue;
            }

            if (revents & EPOLLIN) /* RTSP请求 or RTCP */
            {
                ret = rsrv_do_request(rs_srv, rss, ch_idx);
                if (ret < 0)
                {
                    /*av_log(NULL, AV_LOG_ERROR, "rtsp_do_request() err\n");*/
                }
            }

            if (revents & EPOLLOUT) /* 可写，发送数据 */
            {
                if (rss->rtp_sessn.trans.type == RTP_RTP_AVP_TCP && rss->state == RTSP_STATE_PLAY)
                {
                    /* av_log(NULL, AV_LOG_WARNING, "[fd:%d id:0x%x con:%d]\n", rss->fd, rss->sessn_id, rss->consumer); */
                    ret = rstrm_send_data(rs_srv, rss, ch_idx);
                    if (ret < 0)
                    {
                        /* av_log(NULL, AV_LOG_WARNING, "[id:%d fd:%d]rsrv_send_data() err\n", rss->ch_idx, rss->fd); */

                        if (ret == ERR_EOF)
                        {
                            rss->state = RTSP_STATE_TEARDOWN;
                        }
                    }
                }
            }
        }
    }

    return 0;
}

/******************************************************************************
* 函数介绍: rtsp工作分发线程
* 输入参数: wkr_idx: 工作线程编号
* 输出参数: 无
* 返回值  : >0-写入数据大小, <0-错误代码
*****************************************************************************/
int rsrv_worker_body(sint32 wkr_idx)
{
    int err = -1;
    int ch_idx, ch_idx_max, ch_idx_min;
    rtsp_srv_t * rs_srv = rtsp_srv_fd;
    char tname[16] = {0};

    rsrv_log(AV_LOG_WARNING, "%s(%d): start \n", __FUNCTION__, __LINE__);

    if (rs_srv == NULL)
    {
        rsrv_log(AV_LOG_ERROR, "rtsp_srv_fd error");
        return -1;
    }
    
#ifndef _WIN32
    snprintf(tname, sizeof(tname) , "rsrv_worker#%d", wkr_idx);
    prctl(PR_SET_NAME, tname);
#endif

    rs_srv->state = RSESSION_STATE_DISPATCHED;

    /* min=16/4x(1-1)=0, max=16/4x1-1=3, 0为起始通道数 */
    ch_idx_max = rs_srv->chn_num_max / rs_srv->wkr_num * wkr_idx - 1;
    ch_idx_min = rs_srv->chn_num_max / rs_srv->wkr_num * (wkr_idx - 1);

    for (ch_idx = ch_idx_min; ch_idx <= ch_idx_max; ch_idx++)
    {
        /* 视频和音频rtp信息初始化 */
        _rtp_info_init(rs_srv, ch_idx);
    }

    /* av_log(NULL, AV_LOG_WARNING, "[rsrv_worker:%d] min = %d max = %d\n", wkr_idx, ch_idx_min, ch_idx_max); */

    while (rsrv_worker_running)
    {
        for (ch_idx = ch_idx_min; ch_idx <= ch_idx_max; ch_idx++)
        {
            /*当前管理线程没有活动的会话, 则休眠等待*/
            /*if (rs_srv->sessn_active[ch_idx] <= 0)*/
            {
                av_usleep(1000);
                /*continue;*/
            }

            /* 遍历rtsp会话链表 */
            err = rsrv_sessn_trip(rs_srv, ch_idx);
            if (err < 0)
            {
                rsrv_log(AV_LOG_ERROR, "rsrv_sessn_trip(%d)\n", err);
            }

            err = rsrv_process_request(rs_srv, ch_idx);
            if (err < 0)
            {
                rsrv_log(AV_LOG_ERROR, "rsrv_process_request(%d)\n", err);
                continue;
            }
        }
    }

    /*关闭RTSP的epoll文件描述符*/
    /*rsrv_ep_close(STREAM_MAIN);*/

    return -1;
}

/******************************************************************************
* 函数介绍: rtspsrv创建
* 输入参数: chn_num: 支持的最大通道数量
*           wrk_num: 创建的工作线程数量
*
* 输出参数: 无
* 返回值  : >0-写入数据大小,<0-错误代码
*****************************************************************************/
int rtspsrv_init(sint16 port)
{
    int server_socket;	  /*服务器网络文件描述符*/

    /* 设置rtsp服务器端的网络文件描述符 */
    server_socket = rtspsrv_setup_tcp(port, 1);
    if (server_socket < 0)
    {
        rsrv_log(AV_LOG_ERROR, "rtspsrv_setup_tcp err!\n");
    }

    return server_socket;
}

/******************************************************************************
 * 函数介绍: rtspsrv创建
 * 输入参数: chn_num: 支持的最大通道数量
 *           wrk_num: 创建的工作线程数量
 *
 * 输出参数: 无
 * 返回值  : >0-写入数据大小,<0-错误代码
 *****************************************************************************/
int rtspsrv_open(sint32 chn_num, sint32 wrk_num, sint16 port, rsrv_fifo_t * fifos)
{
    rtsp_srv_t * rs_srv = NULL;
    int ch_idx;

    if (chn_num < 1  || chn_num > CHN_MAX)
    {
        rsrv_log(AV_LOG_ERROR,  "chn_num err!\n");
        return -1;
    }

    if (wrk_num <= 0 || wrk_num > WORKER_MAX)
    {
        rsrv_log(AV_LOG_ERROR,  "wrk_num err!\n");
        return -1;
    }

#ifdef _RTSPSRV_TEST
    rs_srv = (rtsp_srv_t *)av_mallocz(sizeof(rtsp_srv_t));
#else
    rs_srv = (rtsp_srv_t *)mem_manage_malloc(COMP_RTSPSRV, sizeof(rtsp_srv_t));
#endif

    if (rs_srv == NULL)
    {
        rsrv_log(AV_LOG_ERROR,  "rtsp_srv_fd malloc error\n");
        return -1;
    }

    memset(rs_srv, 0x00, sizeof(rtsp_srv_t));

#ifdef _RTSPSRV_TEST
    rs_srv->chi = av_mallocz(sizeof(rsrv_chinfo_t) * chn_num);
#else
    rs_srv->chi = mem_manage_malloc(COMP_RTSPSRV, sizeof(rsrv_chinfo_t) * chn_num);
#endif
    if (rs_srv->chi == NULL)
    {
        rsrv_log(AV_LOG_ERROR, "chi malloc error\n");
        goto ERR_OPN;
    }
    memset(rs_srv->chi, 0, sizeof(rsrv_chinfo_t) * chn_num);

    rs_srv->srv_port = port;
    rs_srv->chn_num_max = chn_num;
    rs_srv->wkr_num = wrk_num;

    rs_srv->sessn_total = 0;
    rs_srv->sessn_max = MAX_SESSION_USER;

    for (ch_idx = 0; ch_idx < rs_srv->chn_num_max; ch_idx++, fifos++)
    {
        rs_srv->chi[ch_idx].ep_fd = rsrv_ep_creat(EV_MAX);
        if (rs_srv->chi[ch_idx].ep_fd < 0)
        {
            rsrv_log(AV_LOG_ERROR, "rsrv_ep_creat error\n");
            goto ERR_OPN;
        }

#ifdef _RTSPSRV_TEST
        rs_srv->chi[ch_idx].ev = av_mallocz(sizeof(struct epoll_event) * EV_MAX);
#else
        rs_srv->chi[ch_idx].ev = mem_manage_malloc(COMP_RTSPSRV, sizeof(struct epoll_event) * EV_MAX);
#endif
        if (rs_srv->chi[ch_idx].ev == NULL)
        {
            rsrv_log(AV_LOG_ERROR, "epoll_event malloc error\n");
            goto ERR_OPN;
        }
        memset(rs_srv->chi[ch_idx].ev, 0, sizeof(struct epoll_event) * EV_MAX);

        if (fifos == NULL)
        {
            rs_srv->chi[ch_idx].fifos[MAIN_STREAM] = 0;
            rs_srv->chi[ch_idx].fifos[SUB_STREAM1] = 0;
        }
        else
        {
            rs_srv->chi[ch_idx].fifos[MAIN_STREAM] = fifos->pri_handle;
            rs_srv->chi[ch_idx].fifos[SUB_STREAM1] = fifos->sec_handle;
        }

        rs_srv->chi[ch_idx].sessn_active = 0;
    }

    rtsp_srv_fd = rs_srv;

#ifndef _RTSPSRV_TEST
    cli_stat_regist(COMP_RTSPSRV, "rtspsrv", rss_stat_info);    
#endif
	rss_init_cli();
    return 0;

ERR_OPN:
    for (ch_idx = 0; ch_idx < rs_srv->chn_num_max; ch_idx++)
    {
        if (rs_srv->chi[ch_idx].ep_fd > 0)
        {
            closesocket(rs_srv->chi[ch_idx].ep_fd);
        }
    }

    if (rs_srv != NULL && rs_srv->chi != NULL)
    {
#ifdef _RTSPSRV_TEST
        av_free(rs_srv->chi);
#else
        mem_manage_free(COMP_RTSPSRV, rs_srv->chi);
#endif
        rs_srv->chi = NULL;
    }

    if (rs_srv != NULL)
    {
#ifdef _RTSPSRV_TEST
        av_free(rs_srv);
#else
        mem_manage_free(COMP_RTSPSRV, rs_srv);
#endif
        rs_srv = NULL;
    }
    return -1;
}

int rtspsrv_start(void)
{
    uint32 wkr_idx;
    int ret;
    rtsp_srv_t * rs_srv = rtsp_srv_fd;

    if (rs_srv == NULL || rs_srv->wkr_num <= 0 || rs_srv->wkr_num > CHN_MAX)
    {
        return -1;
    }

    rsrv_log(AV_LOG_WARNING, "RTSP Build: %s %s \n", __DATE__, __TIME__);

    for (wkr_idx = 1; wkr_idx <= rs_srv->wkr_num; wkr_idx++)
    {
        if ((pthread_create(&rsrv_worker_pt[wkr_idx], NULL, (void *)&rsrv_worker_body, (void *)wkr_idx)) != 0)
        {
            rsrv_log(AV_LOG_ERROR, "create worker-%d failed:%s\n", wkr_idx, strerror(errno));
        }
    }

    rs_srv->srv_fd = rtspsrv_init(rs_srv->srv_port);
    if (rs_srv->srv_fd < 0)
    {
        rsrv_log(AV_LOG_ERROR, "rtspsrv_init err\n");
        return -1;
    }

    /* 添加服务器监听socket到 0 epoll管理句柄 */
    ret = rsrv_ep_add_for_read(rs_srv->chi[0].ep_fd, rs_srv->srv_fd, (void *)rs_srv->srv_fd);
    if (ret < 0)
    {
        rsrv_log(AV_LOG_ERROR, "rsrv_ep_add err\n");
        return -1;
    }
    return 0;
}

int rtspsrv_stop(void)
{
    return 0;
}

int rtspsrv_close(void)
{
    return 0;
}

#ifndef _RTSPSRV_TEST

int rss_stat_brief()
{
    int idx = 0, ch_idx;
    /*SYSTIME tm;
    char tm_str[32] = {0};*/
    rtsp_srv_t * rs_srv = rtsp_srv_fd;
    rtsp_sessn_t * rss = NULL;
    n_slist_t * item = NULL;
    n_slist_t * sessn_list = NULL;
    /*sint64 snd_update_time;*/

    info_seg_init("RTSPSRV SESSION INFO");

    /* 增加段信息,可以使用循环,需要记住info_seg_end在循环末尾*/


    if (rs_srv == NULL || rs_srv->chi == NULL)
    {
        return -1;
    }

    for (ch_idx = 0; ch_idx < rs_srv->chn_num_max; ch_idx++)
    {
        sessn_list = rs_srv->chi[ch_idx].sessn_lst;

        for (item = sessn_list; item; item = item->next)
        {
            rss = (rtsp_sessn_t *)item->data;
            if (rss == NULL)
            {
                continue;
            }
            else
            {
                /*snd_update_time = (av_gettime_relative() - rss->last_snd_time) / 1000;*/

                info_seg_add("Idx",                 INFO_SINTIGER,  idx++);
                info_seg_add(" Fd",                INFO_SINTIGER,  rss->fd);                
                info_seg_add("St",                INFO_SINTIGER,  rss->state);
                info_seg_add("Sm",                INFO_SINTIGER,  rss->stm_type);
                info_seg_add("    Sessn_id",                INFO_HEXINTIGER,  rss->sessn_id);
                info_seg_add("         Ipaddr",    INFO_STRING,    rss->remote_ip_str);
                info_seg_add("Fps",                INFO_SINTIGER,  rss->fps);
                info_seg_add("Bits",               INFO_SINTIGER, rss->cbitrate);
                info_seg_add("    Video",               INFO_SINTIGER,  rss->cvfr_cnt);
                info_seg_add("   Iframe",              INFO_SINTIGER,  rss->cifr_cnt);
                info_seg_add("  Vdrop",              INFO_SINTIGER,  rss->drop_frame);
                info_seg_add("   Audio",               INFO_SINTIGER,  rss->cafr_cnt);
                info_seg_add("  Smax",               INFO_SINTIGER, (sint32)rss->max_snd_time);
                info_seg_add("Wbuf",               INFO_SINTIGER, rss->left_size);
                info_seg_add(" Again",               INFO_SINTIGER, rss->rtp_sessn.again_count);
                info_seg_add(" Hcnt",               INFO_SINTIGER, rss->high_water_cnt);
                info_seg_add("Tcnt",               INFO_SINTIGER, rss->timeout_cnt);
                info_seg_end();
            }
        }
    }
    return 0;
}

int rss_stat_info(char * line)
{
    rtsp_srv_t * rs_srv = rtsp_srv_fd;

    info_create("RTSP_SERVER", "max_sessn:%d active:%d", rs_srv->sessn_max, rs_srv->sessn_total);

    if (sstrnlen(++line, 128) > 0)
    {
        rss_stat_device(line);
    }
    else
    {
        rss_stat_brief();
    }

    info_release();
    return 0;
}
#endif
