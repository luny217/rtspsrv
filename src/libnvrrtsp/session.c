#include <stdio.h>

#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>

#include "avutil_time.h"
#include "session.h"


#if defined(_WIN32)
#include <windows.h>
#define HAVE_COMMON_H 0
#endif

#if defined(_X86)
#include <unistd.h>
#include <sys/prctl.h>
#define HAVE_COMMON_H 1
#endif

#if defined(_ARM)
#include <unistd.h>
#include <sys/prctl.h>
#define HAVE_COMMON_H 1
#endif

#if !HAVE_COMMON_H

#else
#include "common.h"
#include "cli_stat.h"
#endif


static int max_session = 0;
static int session_active = 0;
static int info_index = 0;
static RTSPSessionInfo * __rtsp_sessions = NULL;
#define RTSP_CLI_NUM 512
static RTSP_CLINFO * _rtsp_clinfo = NULL;
static pthread_mutex_t fd_lock;


int rss_deinit(void)
{
    free(__rtsp_sessions);
    __rtsp_sessions = NULL;
    return 0;
}
int rss_init(int nr_session)
{
    int i;
    /*
     *Init rtsp session info
     */
    max_session = nr_session;
    session_active = 0;
    __rtsp_sessions = malloc(sizeof(RTSPSessionInfo) * max_session);
    memset(__rtsp_sessions, 0, sizeof(RTSPSessionInfo) * max_session);
    for (i = 0; i < max_session; i++)
    {
        if ((pthread_mutex_init(&__rtsp_sessions[i].lock, NULL) != 0))
        {
            free(__rtsp_sessions);
            return -1;
        }
        if ((pthread_mutex_init(&__rtsp_sessions[i].cmd_lock, NULL) != 0))
        {
            free(__rtsp_sessions);
            return -1;
        }
        __rtsp_sessions[i].self_idx = i;
    }
    _rtsp_clinfo = malloc(sizeof(RTSP_CLINFO) * RTSP_CLI_NUM);
    memset(_rtsp_clinfo, 0, sizeof(RTSP_CLINFO) * RTSP_CLI_NUM);
    for (i = 0; i < RTSP_CLI_NUM; i++)
    {
        _rtsp_clinfo[i].min_fps = 25;
    }

    if ((pthread_mutex_init(&fd_lock, NULL) != 0))
    {
        free(__rtsp_sessions);
        return -1;
    }

    return 0;
}

int rss_get_max_num(void)
{
    return max_session;
}

int rss_get_active_num(void)
{
    return session_active;
}

RTSPSessionInfo * rss_get_session(int fd)
{
    if (fd >= max_session || fd < 0)
    {
        //av_log(NULL, AV_LOG_ERROR,  "rss_get_session = %d error!\n", fd);
        return NULL;
    }
    return &__rtsp_sessions[fd];
}

int rss_take_unused_fd(void)
{
    int i;

    if (session_active >= max_session)
    {
        return -1;
    }

    pthread_mutex_lock(&fd_lock);

    for (i = 1; i < max_session; i++)
    {
        RTSPSessionInfo * candidate = &__rtsp_sessions[i];
        if (!candidate->in_use)
        {
            candidate->in_use = 1;
            session_active ++;
            pthread_mutex_unlock(&fd_lock);
            return i;
        }
    }
    pthread_mutex_unlock(&fd_lock);
    return -1;
}

int rss_lock_session(int fd)
{
    if (fd >= max_session)
    {
        return -1;
    }
    return pthread_mutex_lock(&__rtsp_sessions[fd].lock);
}

int rss_lock_session_cmd(int fd)
{
    if (fd >= max_session)
    {
        return -1;
    }
    return pthread_mutex_lock(&__rtsp_sessions[fd].cmd_lock);
}


int rss_trylock_session(int fd)
{
    if (fd >= max_session)
    {
        return -1;
    }
    return pthread_mutex_trylock(&__rtsp_sessions[fd].lock);
}

int rss_trylock_session_cmd(int fd)
{
    if (fd >= max_session)
    {
        return -1;
    }
    return pthread_mutex_trylock(&__rtsp_sessions[fd].cmd_lock);
}


int rss_unlock_session(int fd)
{
    if (fd >= max_session)
    {
        return -1;
    }
    return pthread_mutex_unlock(&__rtsp_sessions[fd].lock);
}

int rss_unlock_session_cmd(int fd)
{
    if (fd >= max_session)
    {
        return -1;
    }
    return pthread_mutex_unlock(&__rtsp_sessions[fd].cmd_lock);
}


int rss_untake_fd(int fd)
{
    RTSPSessionInfo * rs_info = &__rtsp_sessions[fd];

    pthread_mutex_lock(&fd_lock);
    if (rs_info->in_use)
    {
        rs_info->in_use = 0;
        rs_info->state = RSESSION_STATE_IDLE;
        rs_info->fmt_ctx = NULL;
        rs_info->frame_count = 0;
        rs_info->timeout_times = 0;
        session_active --;
    }
    else
    {
        pthread_mutex_unlock(&fd_lock);
        return -1;
    }
    pthread_mutex_unlock(&fd_lock);
    return 0;
}

int rss_take_unused_cli(void)
{
    if (++info_index >= RTSP_CLI_NUM)
    {
        info_index = 0;
    }
    return info_index;
}

RTSP_CLINFO * rss_get_cli(int fd)
{
    if (fd >= RTSP_CLI_NUM || fd < 0)
    {
        return NULL;
    }
    return &_rtsp_clinfo[fd];
}
int rss_clear_cli()
{
    memset(_rtsp_clinfo, 0, sizeof(RTSP_CLINFO) * RTSP_CLI_NUM);
    return 0;
}
int rss_stat_device(char * line)
{

#if defined(_ARM)

    int i, idx = 0;
    char tm_str[32] = {0};
    SYSTIME tm;

    info_seg_init("RTSP HISTORY INFO");

    for (i = 0; i < RTSP_CLI_NUM ; i++)
    {
        RTSP_CLINFO * ipc = &_rtsp_clinfo[i];
        if ('\0' != ipc->ipadrr[0] && strstr(line, ipc->ipadrr))
        {
            //info_seg_init("RTSP HISTORY INDEX");
            info_seg_add("Idx",                 INFO_SINTIGER,  idx++);
            //info_seg_end();

            //info_seg_init("RTSP HISTORY IPaddr");
            //info_seg_addl("    IP addr    ",      INFO_STRING, ipc->ipadrr);
            //info_seg_end();

            //info_seg_init("RTSP HISTORY RTSPaddr");
            //info_seg_addl("rtsp addr    ",      INFO_STRING, ipc->rtsp);
            //info_seg_end();

            //info_seg_init("RTSP HISTORY INFO");
            //info_seg_add("Minfps",                 INFO_SINTIGER,  ipc->min_fps);
            info_seg_add("Fd ",                 INFO_SINTIGER,  ipc->fd);
            info_seg_add("Fps",                 INFO_SINTIGER,  ipc->fps);
            info_seg_add("Enc",               INFO_SINTIGER,  ipc->enc_type);
            info_seg_add("Width",               INFO_SINTIGER,  ipc->width);
            info_seg_add("Height",              INFO_SINTIGER,  ipc->height);
            info_seg_add("  Video  ",               INFO_SINTIGER,  ipc->vframe_count);
            info_seg_add(" MaxV",               INFO_SINTIGER,  ipc->max_vframe / 1024);
            //info_seg_add("  Iframe ",              INFO_SINTIGER,  ipc->iframe_count);
            info_seg_add("  Audio  ",               INFO_SINTIGER,  ipc->aframe_count);
            info_seg_add("MaxA",               INFO_SINTIGER,  ipc->max_aframe);
            info_seg_add(" Vdrop",              INFO_SINTIGER,  ipc->vdrop_count);
            info_seg_add(" Recv",              INFO_SINTIGER,  ipc->recv_bytes / 1024);
            info_seg_add(" RQ",             INFO_SINTIGER,  ipc->rcvq_buf);
            info_seg_add(" SQ",             INFO_SINTIGER,  ipc->sndq_buf);

            if (ipc->open_tm)
            {
                time_sec2time(ipc->open_tm / 1000000, &tm);
                sprintf(tm_str, TIME_STR, SYSTIME4TM_FMT(tm));
            }
            info_seg_add("       Opentime      ",     INFO_STRING, tm_str);

            memset(tm_str, 0, 32);
            if (ipc->close_tm)
            {
                time_sec2time(ipc->close_tm / 1000000, &tm);
                sprintf(tm_str, TIME_STR, SYSTIME4TM_FMT(tm));
            }
            info_seg_add("      Closetime      ",     INFO_STRING, tm_str);

            info_seg_add("err",                 INFO_SINTIGER,  ipc->error_code);

            info_seg_add("  T-40",                 INFO_SINTIGER,  ipc->ts_40ms_cnt);
            //info_seg_add("  T-20  ",                 INFO_SINTIGER,  ipc->ts_20ms_cnt);
            info_seg_add("  T-10K",                 INFO_SINTIGER,  ipc->ts_10ms_cnt / 1024);
            info_seg_end();
        }
        else if ('\0' != ipc->ipadrr[0] && strstr(line, "all"))
        {
            info_seg_add("Idx",                 INFO_SINTIGER,  idx++);
            info_seg_addl("      Ipaddr   ",      INFO_STRING, ipc->ipadrr);
            info_seg_add("Fps",                 INFO_SINTIGER,  ipc->fps);
            info_seg_add("Enc",               INFO_SINTIGER,  ipc->enc_type);
            info_seg_add("  Video  ",               INFO_SINTIGER,  ipc->vframe_count);
            info_seg_add(" MaxV",               INFO_SINTIGER,  ipc->max_vframe / 1024);
            info_seg_add("  Audio  ",               INFO_SINTIGER,  ipc->aframe_count);
            info_seg_add("MaxA",               INFO_SINTIGER,  ipc->max_aframe);
            info_seg_add(" Vdrop",              INFO_SINTIGER,  ipc->vdrop_count);
            info_seg_add(" Recv",              INFO_SINTIGER,  ipc->recv_bytes / 1024);
            info_seg_add(" RQ",             INFO_SINTIGER,  ipc->rcvq_buf);
            info_seg_add(" SQ",             INFO_SINTIGER,  ipc->sndq_buf);
            if (ipc->open_tm)
            {
                time_sec2time(ipc->open_tm / 1000000, &tm);
                sprintf(tm_str, TIME_STR, SYSTIME4TM_FMT(tm));
            }
            info_seg_add("       Opentime      ",     INFO_STRING, tm_str);
            memset(tm_str, 0, 32);
            if (ipc->close_tm)
            {
                time_sec2time(ipc->close_tm / 1000000, &tm);
                sprintf(tm_str, TIME_STR, SYSTIME4TM_FMT(tm));
            }
            info_seg_add("      Closetime      ",     INFO_STRING, tm_str);
            info_seg_add("err",                 INFO_SINTIGER,  ipc->error_code);
            info_seg_add("  T-40",                 INFO_SINTIGER,  ipc->ts_40ms_cnt);
            //info_seg_add("  T-20  ",                 INFO_SINTIGER,  ipc->ts_20ms_cnt);
            info_seg_add("   T-10K",                 INFO_SINTIGER,  ipc->ts_10ms_cnt / 1024);

            info_seg_end();
        }
    }

    if (strstr(line, "cpu"))
    {
        rtsp_prn_sleep();
    }
#endif
    return 0;
}

int rss_stat_brief()
{
#if defined(_ARM)

    int i;
    SYSTIME tm;
    char tm_str[32] = {0};

    info_seg_init("RTSP SESSION INFO");

    /* 增加段信息,可以使用循环,需要记住info_seg_end在循环末尾*/
    for (i = 0; i < max_session; i++)
    {
        RTSPSessionInfo * rs = &__rtsp_sessions[i];

        if (rs->in_use)
        {

            info_seg_add("Idx",                 INFO_SINTIGER,  i);
            info_seg_add(" Fd",                INFO_SINTIGER,  rs->tcp_fd);
            info_seg_add(" St",                INFO_SINTIGER,  rs->state);
            info_seg_add("      Ipaddr      ",    INFO_STRING,    rs->ipaddr);
            info_seg_add("Jitter",               INFO_SINTIGER,  rs->average_ts_ms_cnt);
            info_seg_add("Fps",                 INFO_SINTIGER,  rs->fps);
            info_seg_add("Width",               INFO_SINTIGER,  rs->cb_info.width);
            info_seg_add("Height",              INFO_SINTIGER,  rs->cb_info.height);
            info_seg_add("  Video  ",               INFO_SINTIGER,  rs->vframe_count);
            info_seg_add("  MaxV",               INFO_SINTIGER,  rs->max_vframe / 1024);
            info_seg_add("  Iframe ",              INFO_SINTIGER,  rs->iframe_count);
            info_seg_add("  Vdrop ",              INFO_SINTIGER,  rs->vdrop_count);
            info_seg_add("  Audio  ",               INFO_SINTIGER,  rs->aframe_count);
            info_seg_add("  MaxA",               INFO_SINTIGER,  rs->max_aframe);
            info_seg_add("  Meta  ",               INFO_SINTIGER,  rs->meta_count);
            /*info_seg_add("  CBIt ",       INFO_SINTIGER,  (av_gettime() - rs->cb_time)/1000);
            info_seg_add(" EAgain1 ",          INFO_SINTIGER,  rs->eagain1);
            info_seg_add(" EAgain2 ",          INFO_SINTIGER,  rs->eagain2);
            info_seg_add(" MaxVCB ",       INFO_SINTIGER,  rs->max_video_cb / 1000);
            info_seg_add(" MaxACB ",       INFO_SINTIGER,  rs->max_audio_cb / 1000);
            info_seg_add(" MaxMCB ",       INFO_SINTIGER,  rs->max_meta_cb / 1000);
            info_seg_add(" MaxCom ",       INFO_SINTIGER,  rs->max_com_frame / 1000);

            info_seg_add("FrameOut",          INFO_SINTIGER,  rs->frame_count_out);
            info_seg_add("Read",          INFO_SINTIGER,  rs->readable);
            info_seg_add("FiFoSize",          INFO_SINTIGER,  rs->fifo_size / 1024);
            info_seg_add("SoBig",             INFO_SINTIGER,  rs->so_big_count);*/

            info_seg_end();
        }
    }
#endif
    return 0;
}


int rss_stat_info(char * line)
{
#if defined(_ARM)

    info_create("RTSP", "max_session:%d active:%d info_index:%d", max_session, session_active, info_index);

    if (strlen(++line) > 0)
    {
        rss_stat_device(line);
    }
    else
    {
        rss_stat_brief();
    }

    info_release();
#endif
    return 0;
}

