#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>


#include <libavformat/avformat.h>
#include <libavformat/avformat_rtsp.h>
#include <libavutil/avutil_dict.h>
#include <libavutil/avutil_time.h>
#include <libavutil/avutil_string.h>
#include <libavcodec/avcodec_golomb.h>
#include <libavcodec/avcodec_hevc.h>


#if defined(_WIN32)
#include <windows.h>
#define HAVE_COMMON_H 1
#include <sys/epoll.h>
#endif

#if defined(_X86)
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/epoll.h>

#define HAVE_COMMON_H 1
#endif

#if defined(_ARM)
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/epoll.h>
#include "cli_stat.h"
#define HAVE_COMMON_H 1
#include "mem_manage.h"
#endif

#include "avformat_rtp_h264_decoder.h"
#include "rtsp.h"
#include "session.h"


#define RTSP_FIFO_LOWWATER  1500
#define RTSP_FIFO_HIGHWATER 128000

#if (defined(PLATFORM_HI3520D) || defined(PLATFORM_HI3520D_V300) || defined(PLATFORM_HI3521A))
#define WORKER_NUM 1
#define MAX_CH_NUM 24
#define DATA_MASK_BITS  24
#elif (defined(PLATFORM_HI3531) || defined(PLATFORM_HI3531A) || defined(PLATFORM_HI3798M))
#define WORKER_NUM 4
#define MAX_CH_NUM 64
#define DATA_MASK_BITS  16
#elif (defined(PLATFORM_HI3535))
#define WORKER_NUM 4
#define MAX_CH_NUM 64
#define DATA_MASK_BITS  16
#define SO_TOE_ENABLE 44
#elif (defined(PLATFORM_HI3536))
#define WORKER_NUM 5
#define MAX_CH_NUM 80
#define DATA_MASK_BITS  16
#define SO_TOE_ENABLE 76
#else
#define WORKER_NUM 1
#define MAX_CH_NUM 8
#define DATA_MASK_BITS  8
#endif

#define STIMEOUT "10000000"
#define TIMEOUT_TIMES 2
#define ONE_MINUTES  60000000
#define ONE_SECONDS  1000000
#define TWO_SECONDS  2000000
#define TEN_SECONDS  10000000
#define HALF_MINUTES 30000000

#define THREEHUNDREDS_MSECS 300000
#define TWOHUNDREDS_MSECS 200000
#define FIVEHUNDREDS_MSECS 500000
#define ONEHUNDREDS_MSECS 100000
#define EIGHTY_MSECS 80000
#define FIFTY_MSECS 50000
#define FOURTY_MSECS 40000
#define THIRTY_MSECS 30000
#define TWENTY_MSECS 20000
#define TEN_MSECS 10000


#define NAL_TYPE_SLICE  (1)
#define NAL_TYPE_IDR    (5)
#define NAL_TYPE_SEI    (6)
#define NAL_TYPE_SPS    (7)
#define NAL_TYPE_PPS    (8)

static FUNC_RTSP_CALLBACK  __rtsp_callback = NULL;
pthread_t dispatcher_thread, stream_thread, rtsp_worker[WORKER_NUM], sendcmd_thread;
extern AVInputFormat ff_rtsp_demuxer;

#define RTSP_RCV_FIFO_SIZE    (512 * 1024)
static unsigned int data_ready[WORKER_NUM] = {0};
static unsigned int session_ready[WORKER_NUM] = {0};

static uint64_t worker_sleep[WORKER_NUM] = {0};

static int epoll_register(RTSPSessionInfo * rs_info);
static int epoll_unregister(RTSPSessionInfo * rs_info);


int __rtsp_open_input(AVFormatContext ** ps, const char * filename, AVInputFormat * fmt, AVDictionary ** options);
int epoll_del(int e_fd, RTSPSessionInfo * rs_info);
int epoll_add(int e_fd, int events, RTSPSessionInfo * rs_info);
int rtsp_read_play(AVFormatContext * s);
void get_word(char * buf, int buf_size, const char ** pp);
int rtp_valid_packet_in_sequence(RTPStatistics * s, uint16_t seq);


enum statistical_method
{
    TS_REMOTE_METHOD = 0,
    TS_LOCAL_METHOD = 1
};

static int fps_ts[] =
{
    /*    0,       1,      2,      3,      4,      5,      6,      7,      8,      9,     10*/
    1000000, 1000000, 500000, 333333, 250000, 200000, 166666, 142850, 125000, 111111, 100000,
    /* 11,     12,        13,     14,     15,     16,     17,     18,     19,     20*/
    90909,     83333,  76920,  71420,  66666,  62500,  58820,  55555,  52630,   50000,
    /* 21,     22,        23,     24,     25,     26,     27,     28,     29,    30*/
    47610,     45450,  43470,  41660,  40000,  38460,  37030,  35710,  34480,  33333,
    /* 31,     32,        33,     34,     35,     36,     37,     38,     39,     40*/
    32258,     31250,  30303,  29411,  28571,  27777,  27027,  26315,  25641,  25000,
    /* 41,     42,        43,     44,     45,     46,     47,     48,     49,     50*/
    24390,     23809,  23255,  22727,  22222,  21739,  21276,  20833,  20408,  20000,
    /* 51,     52,        53,     54,     55,     56,     57,     58,     59,     60*/
    19607,     19230,  18867,  18518,  18181,  17857,  17543,  17241,  16949,  16666,
    /* 61,     62,       63,      64,     65,     66,     67,     68,     69,     70*/
    16393,     16129,  15873,  15625,  15384,  15151,  14925,  14705,  14492,  14285,
    /* 71,     72,        73,     74,     75,     76,     77,     78,     79,     80*/
    14084,     13888,  13698,  13513,  13333,  13157,  12987,  12820,  12658,  12500,
    /* 81,     82,       83,      84,     85,     86,     87,     88,     89,     90*/
    12345,     12195,  12048,  11904,  11764,  11627,  11494,  11363,  11235,  11111,
    /* 91,     92,     93,     94,     95,     96,     97,     98,     99,     100*/
    10989,     10869,  10752,  10638,  10526,  10416,  10309,  10204,  10101,  10000
};

#if !HAVE_COMMON_H

typedef unsigned int uint32;

#define prctl(a,b)
#define TIME_STR                    "[%04d-%02d-%02d %02d:%02d:%02d]"
#define SYSTIME4TM_FMT(systime)     (systime).year + 2000, (systime).month, (systime).day, \
	(systime).hour, (systime).min, (systime).sec

typedef struct
{
    uint32 sec : 6;
    uint32 min : 6;
    uint32 hour : 5;
    uint32 day : 5;
    uint32 month : 4;
    uint32 year : 6;
} SYSTIME, *PSYSTIME;
#define SYSTIME_LEN     sizeof(SYSTIME)

uint32 time_sec2time(uint32 sec, PSYSTIME psystm)
{
#if 0
    struct tm tm_tmp;

    gmtime_s((void *)&sec, &tm_tmp);
    psystm->year = tm_tmp.tm_year - 100;
    psystm->month = tm_tmp.tm_mon + 1;
    psystm->day = tm_tmp.tm_mday;
    psystm->hour = tm_tmp.tm_hour;
    psystm->min = tm_tmp.tm_min;
    psystm->sec = tm_tmp.tm_sec;
#endif

    return 0;
}
#else
#include "common.h"
#include "fifo.h"

#endif

#define _PATH_PROCNET_TCP		"/proc/net/tcp"

enum
{
    TCP_ESTABLISHED = 1,
    TCP_SYN_SENT,
    TCP_SYN_RECV,
    TCP_FIN_WAIT1,
    TCP_FIN_WAIT2,
    TCP_TIME_WAIT,
    TCP_CLOSE,
    TCP_CLOSE_WAIT,
    TCP_LAST_ACK,
    TCP_LISTEN,
    TCP_CLOSING			/* now a valid state */
};

static const char * tcp_state[] =
{
    "",
    "ESTABLISHED",
    "SYN_SENT",
    "SYN_RECV",
    "FIN_WAIT1",
    "FIN_WAIT2",
    "TIME_WAIT",
    "CLOSE",
    "CLOSE_WAIT",
    "LAST_ACK",
    "LISTEN",
    "CLOSING"
};


int rtsp_dispatcher_running = 1;
int worker_running = 1;
void  __rtsp_stream_body(void * arg);
void  __rtsp_process_worker(void * arg);
void  __rtsp_sendcmd_worker(void * arg);

int tcp_do_one(const char * line, struct sockaddr_in * local, struct sockaddr_in * rem, int * rbuf_num, int * sbuf_num)
{
    unsigned long rxq, txq, time_len, retr, inode;
    int num, local_port, rem_port, d, state, uid, timer_run, timeout;
    char rem_addr[128], local_addr[128], buffer[1024], more[512];
    struct sockaddr_in localaddr, remaddr;

    av_log(NULL, AV_LOG_DEBUG, "line = [%s]\n", line);

    num = sscanf(line, "%d: %64[0-9A-Fa-f]:%X %64[0-9A-Fa-f]:%X %X %lX:%lX %X:%lX %lX %d %d %ld %512s\n",
                 &d, local_addr, &local_port, rem_addr, &rem_port, &state,
                 &txq, &rxq, &timer_run, &time_len, &retr, &uid, &timeout, &inode, more);


    sscanf(local_addr, "%X", &((struct sockaddr_in *) &localaddr)->sin_addr.s_addr);
    sscanf(rem_addr, "%X", &((struct sockaddr_in *) &remaddr)->sin_addr.s_addr);
    ((struct sockaddr *) &localaddr)->sa_family = AF_INET;
    ((struct sockaddr *) &remaddr)->sa_family = AF_INET;

    if (!remaddr.sin_addr.s_addr || !local_port || !rem_port)
    {
        return -1;
    }

    if (num < 11)
    {
        av_log(NULL, AV_LOG_WARNING, "got bogus tcp line.\n");
        return -1;
    }

    if (state == TCP_LISTEN)
    {
        time_len = 0;
        retr = 0L;
        rxq = 0L;
        txq = 0L;
    }

    if (rem->sin_addr.s_addr == remaddr.sin_addr.s_addr && ntohs(local->sin_port) == local_port)
    {
        *rbuf_num = rxq;
        *sbuf_num = txq;
        av_log(NULL, AV_LOG_DEBUG, "socket(0)--rbuf(%lu)--sbuf(%lu)\n", rxq, txq);
        av_log(NULL, AV_LOG_DEBUG, "rem->sin_addr.s_addr(%x)--remaddr.sin_addr.s_addr(%x)--sin_port(%d)--local_port(%d)\n",
                 rem->sin_addr.s_addr, remaddr.sin_addr.s_addr, ntohs(local->sin_port), local_port);
        return 0;
    }
    else
    {
        av_log(NULL, AV_LOG_DEBUG, "rem->sin_addr.s_addr(%x)--remaddr.sin_addr.s_addr(%x)--sin_port(%d)--local_port(%d)\n",
                 rem->sin_addr.s_addr, remaddr.sin_addr.s_addr, ntohs(local->sin_port), local_port);
    }

    return -1;

#if 0
    strncpy(local_addr, inet_ntoa(localaddr.sin_addr), sizeof(local_addr));
    strncpy(rem_addr, inet_ntoa(remaddr.sin_addr), sizeof(rem_addr));

    snprintf(buffer, sizeof(buffer), "%d", local_port);

    if ((strlen(local_addr) + strlen(buffer)) > 22)
        local_addr[22 - strlen(buffer)] = '\0';

    strcat(local_addr, ":");
    strcat(local_addr, buffer);
    snprintf(buffer, sizeof(buffer), "%d", rem_port);

    if ((strlen(rem_addr) + strlen(buffer)) > 22)
        rem_addr[22 - strlen(buffer)] = '\0';

    strcat(rem_addr, ":");
    strcat(rem_addr, buffer);

    av_log(NULL, AV_LOG_DEBUG, "tcp   %6ld %6ld %-23s %-23s %-12s", rxq, txq, local_addr, rem_addr, tcp_state[state]);
    putchar('\n');
    return -1;
#endif

}

int tcp_info(int sockfd, int * rbuf_num, int * sbuf_num)
{
    FILE * procinfo;
    char buffer[8192];
    struct sockaddr_in local, rem;
    int ret;
    socklen_t local_len, rem_len;

    local_len = sizeof(local);
    rem_len = sizeof(rem);

    ret = getsockname(sockfd, (struct sockaddr *)&local, &local_len);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "getsockname fail[%d].\n", errno);
        return -1;
    }
    else
    {
        av_log(NULL, AV_LOG_DEBUG, "local IP:%s ", inet_ntoa(local.sin_addr));
        av_log(NULL, AV_LOG_DEBUG, "local PORT:%d \n", ntohs(local.sin_port));
    }

    ret = getpeername(sockfd, (struct sockaddr *)&rem, &rem_len);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "getpeername fail[%d].\n", errno);
        return -1;
    }
    else
    {
        av_log(NULL, AV_LOG_INFO, "rem IP:%s ", inet_ntoa(rem.sin_addr));
        av_log(NULL, AV_LOG_INFO, "rem PORT:%d \n", ntohs(rem.sin_port));
    }


    procinfo = fopen(_PATH_PROCNET_TCP, "r");
    if (procinfo == NULL)
    {
        av_log(NULL, AV_LOG_ERROR, "%s open fail[%d].\n", _PATH_PROCNET_TCP, errno);
        return -1;
    }

    do
    {
        if (fgets(buffer, sizeof(buffer), procinfo))
        {
            ret = tcp_do_one(buffer, &local, &rem, rbuf_num, sbuf_num);
        }
    }
    while (!feof(procinfo));

    fclose(procinfo);

    return 0;
}


void __rtsp_dispatcher_body(void * arg)
{
    int ret, sidx, dispatched, i, nonzero_count;
    /* int last_sec_count1, last_sec_count2, last_sec_count3; */
    /* int last_sec_index1, last_sec_index2, last_sec_index3; */
    int count = 0, fifo_size;
    int64_t start = 0, end = 0, now = 0;;
    unsigned long long selfid_64 = 0;
    RTSPSessionInfo * rs_info;
    FIFO_BUFF_INFO_S buff_info;
    float fps;
    char name[16] = {0};
    /*prctl(PR_SET_NAME, "rtsp_calc");*/

    /*int cpu_num = 1;

    unsigned long mask = 1 << cpu_num;

    av_log(NULL, AV_LOG_INFO, "Force Run On Proccessor :%d mask:%x\n", cpu_num, mask);

    if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
    {
        DBG_PRINTFA;
        exit(0);
    }*/

    while (rtsp_dispatcher_running)
    {
        dispatched = 0;

        if (rss_get_active_num() < 1)
        {
            av_usleep(100000);
        }

        /*av_log(NULL, AV_LOG_VERBOSE, "++++++++ active_num = %d\n", rss_get_active_num());*/

        for (sidx = 0;
                ((rs_info = rss_get_session(sidx)) != NULL) &&
                dispatched < rss_get_active_num();
                sidx++)
        {
            if (rs_info->fmt_ctx != NULL)
            {
                if (rs_info->state == RSESSION_STATE_IDLE)
                {
                    /* TODO handle race condition */
                    rs_info->state = RSESSION_STATE_DISPATCHED;
                    rs_info->start = av_gettime();
                }
                else
                {
                    rs_info->end = av_gettime() - rs_info->start;

                    /*一秒钟统计平均值刷新一次*/
                    if (rs_info->end > ONE_SECONDS)
                    {
                        int32_t total_10ms_cnt = 0, total_20ms_cnt = 0, total_40ms_cnt = 0, total_ms_cnt = 0;

                        /*根据每秒接收次数统计平均帧率*/
                        rs_info->total_recv_gap = 0;
                        rs_info->total_recv_count = 0;
                        nonzero_count = 0;
                        for (i = 0; i < CALC_PERIOD; i++)
                        {
                            rs_info->total_recv_gap += rs_info->one_sec_rev_total[i];
                            rs_info->total_recv_count += rs_info->one_sec_rev_count[i];
                            if (rs_info->one_sec_rev_count[i]) nonzero_count++;
                        }
                        rs_info->average_recv_gap = rs_info->total_recv_count ? rs_info->total_recv_gap / rs_info->total_recv_count : 0;
                        rs_info->fps = nonzero_count ? (rs_info->total_recv_count * 10 + 5) / 10 / nonzero_count : 0;
                        /* rs_info->fps = fps ? fps + 0.7 / 10 : 0; //四舍五入 */
                        rs_info->fps = rs_info->fps > 100 ? 100 : rs_info->fps;

                        /*av_log(NULL, AV_LOG_VERBOSE, "fps = %d\n", rs_info->fps);*/

                        /*计算平均时间戳差*/

                        rs_info->total_ts_gap = 0;
                        rs_info->total_ts_count = 0;
                        for (i = 0; i < CALC_PERIOD; i++)
                        {
                            rs_info->total_ts_gap += rs_info->one_sec_ts_total[i];
                            rs_info->total_ts_count += rs_info->one_sec_ts_count[i];
                        }
                        rs_info->average_ts_gap = rs_info->total_ts_count ? rs_info->total_ts_gap / rs_info->total_ts_count : 0;

                        /*计算平均补偿次数，由此计算网络抖动率*/

                        for (i = 0; i < CALC_JITTER_PERIOD; i++)
                        {
                            total_10ms_cnt += rs_info->one_sec_ts_10ms_cnt[i];
                            total_20ms_cnt += rs_info->one_sec_ts_20ms_cnt[i];
                            total_40ms_cnt += rs_info->one_sec_ts_40ms_cnt[i];
                            total_ms_cnt += rs_info->one_sec_ts_cnt[i];
                        }

                        rs_info->average_ts_ms_cnt = total_40ms_cnt > 0 ? 500 : (total_10ms_cnt * 10 + total_20ms_cnt * 90);

                        rs_info->average_ts_ms_cnt = rs_info->average_ts_ms_cnt > 500 ? 500 : rs_info->average_ts_ms_cnt;

                        /*av_log(NULL, AV_LOG_VERBOSE, "[fd:%d %d] 10ms_cnt = %d 20ms_cnt = %d 40ms_cnt = %d total_ms_cnt = %d\n",
                                 rs_info->tcp_fd, rs_info->average_ts_ms_cnt, total_10ms_cnt, total_20ms_cnt, total_40ms_cnt, total_ms_cnt);*/

                        if ((rs_info->calc_index + 1) == CALC_PERIOD)
                        {
                            rs_info->calc_index = 0;
                        }
                        else
                        {
                            rs_info->calc_index++;
                        }

                        if ((rs_info->calc_jitter_idx + 1) == CALC_JITTER_PERIOD)
                        {
                            rs_info->calc_jitter_idx = 0;
                        }
                        else
                        {
                            rs_info->calc_jitter_idx++;
                        }

                        rs_info->one_sec_rev_total[rs_info->calc_index] = 0;
                        rs_info->one_sec_rev_count[rs_info->calc_index] = 0;
                        rs_info->one_sec_ts_total[rs_info->calc_index] = 0;
                        rs_info->one_sec_ts_count[rs_info->calc_index] = 0;

                        rs_info->one_sec_ts_10ms_cnt[rs_info->calc_jitter_idx] = 0;
                        rs_info->one_sec_ts_20ms_cnt[rs_info->calc_jitter_idx] = 0;
                        rs_info->one_sec_ts_40ms_cnt[rs_info->calc_jitter_idx] = 0;
                        rs_info->one_sec_ts_cnt[rs_info->calc_jitter_idx] = 0;

                        rs_info->start = av_gettime();

                        /*处理系统自身校时*/
                        now = rs_info->start;
                        if (now - rs_info->systm_last < -TEN_SECONDS || now - rs_info->systm_last > TEN_SECONDS)
                        {
                            SYSTIME last_tm, sys_tm;
                            time_sec2time(rs_info->systm_last / 1000000, &last_tm);
                            time_sec2time(now / 1000000, &sys_tm);

                            av_log(NULL, AV_LOG_INFO, "++++++++[fd:%d %lld] systime ", rs_info->tcp_fd, now - rs_info->systm_last);
                            av_log(NULL, AV_LOG_INFO, TIME_STR, SYSTIME4TM_FMT(last_tm));
                            av_log(NULL, AV_LOG_INFO, "to ");
                            av_log(NULL, AV_LOG_INFO, TIME_STR, SYSTIME4TM_FMT(sys_tm));
                            av_log(NULL, AV_LOG_INFO, "\n");

                            rs_info->ts_base = now;
                            rs_info->ts = 0;
                        }
                        rs_info->systm_last = now;
                    }

                }
                dispatched ++;
            }
            else
            {
                /*free session slot, not used or closed already*/
                /*av_log(NULL, AV_LOG_VERBOSE, "free session slot\n");*/
            }
        }
#if HAVE_USLEEP
        usleep(100);
#else
        Sleep(10);
#endif
    }
    av_log(NULL, AV_LOG_WARNING, "dispatcher exited\n");
}

int __rtsp_start_dispatcher(void)
{
    if ((pthread_create(&dispatcher_thread, NULL, (void *) &__rtsp_dispatcher_body, NULL)) != 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Create dispatcher failed:%s\n", strerror(errno));
    }
    return 0;
}

int rtsp_init(int max_connection)
{
    int nr_thread = 1;
    int queue_depth = 64;
    int idx;

    /* __rtsp_callback = rtsp_callback; */

    /*init session manager*/
#if 0
    if (max_connection > 0)
    {
        rss_init(max_connection);
        av_log(NULL, AV_LOG_INFO,  "rtsp max_connection = %d\n", max_connection);
    }
    else
#endif
    {
#define DEFAULT_MAX_SESSION MAX_CH_NUM
        rss_init(DEFAULT_MAX_SESSION);
        av_log(NULL, AV_LOG_WARNING,  "rtsp default_connection = %d\n", DEFAULT_MAX_SESSION);
    }

    /*
     *Init ffmpeg components
     */
    av_register_all();
    avformat_network_init();

    if ((pthread_create(&dispatcher_thread, NULL, (void *)&__rtsp_dispatcher_body, NULL)) != 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Create dispatcher failed:%s\n", strerror(errno));
    }

#if 1
    if ((pthread_create(&stream_thread, NULL, (void *)&__rtsp_stream_body, NULL)) != 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Create stream_thread failed:%s\n", strerror(errno));
    }
#endif

    for (idx = 1; idx <= WORKER_NUM; idx++)
    {
        av_log(NULL, AV_LOG_WARNING, "idx:%d\n", idx);

        if ((pthread_create(&rtsp_worker[idx], NULL, (void *)&__rtsp_process_worker, (void *)idx)) != 0)
        {
            av_log(NULL, AV_LOG_ERROR, "Create rtsp_worker failed:%s\n", strerror(errno));
        }
    }

#if 1
    if ((pthread_create(&sendcmd_thread, NULL, (void *)&__rtsp_sendcmd_worker, NULL)) != 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Create sendcmd_thread failed:%s\n", strerror(errno));
    }
#endif

#if defined(_ARM)
    cli_stat_regist(COMP_RTSP, "rtspc", rss_stat_info);
#endif
    return 0;
}

void rtsp_deinit(void)
{
    /*stop the dispatcher thread*/
    /* pthread_cancel(dispatcher_thread); */
    rtsp_dispatcher_running = 0;
    pthread_join(dispatcher_thread, NULL);

    /*destroy thread poll*/
    rss_deinit();
}

static int rtsp_interrupt_cb(void * ctx)
{
    RTSPSessionInfo * info = ctx;
    /* int64_t now = 0; */
    /* char name[16]; */
    int ret;

    if (info == NULL)
    {
        return 0;
    }

#if 0
    now = av_gettime();

    prctl(PR_GET_NAME, (unsigned long)name);

    if (now - info->systm_last < 0 || now - info->systm_last > FIVEHUNDREDS_MSECS)
    {
        SYSTIME last_tm, sys_tm;
        time_sec2time(info->systm_last / 1000000, &last_tm);
        time_sec2time(now / 1000000, &sys_tm);

        av_log(NULL, AV_LOG_INFO, "++++++++[%s] systime changed from ", name);
        av_log(NULL, AV_LOG_INFO, TIME_STR, SYSTIME4TM_FMT(last_tm));
        av_log(NULL, AV_LOG_INFO, NULL, AV_LOG_ERROR, "to ");
        av_log(NULL, AV_LOG_INFO, TIME_STR, SYSTIME4TM_FMT(sys_tm));
        av_log(NULL, AV_LOG_INFO, "\n");

        info->ts_base = now;
        info->ts = 0;
    }

    info->systm_last = now;
#endif

    ret = (info->state == RSESSION_STATE_CLOSED);
    if (ret)
    {
        av_log(NULL, AV_LOG_WARNING, "rtsp_interrupt_cb exit \n");
    }
    return ret;
}


static void assemble_authed_url(char * auth_url, int size, char * url, char * username, char * password)
{
    char * host = NULL;
    if (!username && !password)
    {
        strncpy(auth_url, url, size);
        return;
    }

    host = strstr(url, "rtsp://") + strlen("rtsp://");
    if (host != NULL)
    {
        snprintf(auth_url, size, "%s%s:%s@%s", "rtsp://", username, password, host);
    }
}

int rtsp_open(const char * url, const char * username, const char * password, FUNC_RTSP_CALLBACK rtsp_callback, void * priv_data)
{
    AVFormatContext * fmt_ctx = NULL;
    AVDictionary * options = NULL;
    RTSPSessionInfo * rs_info = NULL;
    RTSP_CLINFO * rtsp_cli = NULL;
    RTSPState * rt = NULL;
    TCPContext * tcp_ctx = NULL;
    char error_buf[1024] = {0};
    char auth_url[1024] = {0};
    char host[1024], path[1024], auth[128];
    int ret = 0, handle, i, port;
    int64_t now = 0;
    SYSTIME now_tm;
    now = av_gettime();
    time_sec2time(now / 1000000, &now_tm);

    if (url == NULL || strlen(url) <= sizeof("rtsp://1.1.1.1"))
    {
        return RTSP_ERR_PARAM;
    }

    /* av_log_set_flags(AV_LOG_SKIP_REPEATED); */

    /*使用tcp连接*/
    av_dict_set(&options, "rtsp_transport", "tcp", 0);

    /*默认10秒超时，影响rtsp connect*/
    av_dict_set(&options, "stimeout", STIMEOUT, 0);

    /*组合帐号密码地址为rtsp://usr:pwd@1.1.1.1*/
    assemble_authed_url(auth_url, sizeof(auth_url), (char *)url, (char *)username, (char *)password);

    ret = __rtsp_open_input(&fmt_ctx, auth_url, &ff_rtsp_demuxer, &options);

    /*返回自定义错误代码*/
    if (ret < 0)
    {
        if (ret == AVERROR_AUTH_FAIL)
        {
            ret = RTSP_ERR_AUTH_FAIL;
            av_log(NULL, AV_LOG_ERROR, "RTSP_ERR_AUTH_FAIL\n");
        }
        else if (ret == AVERROR_INVALIDDATA)
        {
            ret = RTSP_ERR_NETWORK;
            av_log(NULL, AV_LOG_ERROR, "RTSP_ERR_NETWORK\n");
        }
        else if (ret == -EIO)
        {
            ret = RTSP_ERR_NETWORK;
            av_log(NULL, AV_LOG_ERROR, "RTSP_ERR_NETWORK\n");
        }
        else if (ret == -EPROTONOSUPPORT)
        {
            ret = RTSP_ERR_UNSUPPORTED_PROTOCOL;
            av_log(NULL, AV_LOG_ERROR, "RTSP_ERR_UNSUPPORTED_PROTOCOL\n");
        }
        else
        {
            av_strerror(ret, error_buf, sizeof(error_buf));
            av_log(NULL, AV_LOG_ERROR, "Open stream unknown err:%s(0x%x|%d)\n", error_buf, ret, ret);
        }
        /*pass ret to invoker*/
        goto ERR_OPEN_CTX;
    }

    /*获取空闲的管理句柄*/
    handle = rss_take_unused_fd();
    if (handle < 0)
    {
        ret = RTSP_ERR_TOO_MANY_SESSION;
        goto ERR_TAKE_UNUSED_FD;
    }

    /*管理句柄加锁*/
    if (rss_lock_session(handle) < 0)
    {
        ret = RTSP_ERR_LOCK_SESSION;
        goto ERR_LOCK_SESSION;
    }
    rs_info = rss_get_session(handle);

    av_log(NULL, AV_LOG_WARNING, "[rss:%d]", rs_info->self_idx);

    /*创建接收fifo，默认512K*/
    if ((rs_info->fifo_fd = fifo_openext(RTSP_RCV_FIFO_SIZE, 80, 20, 0)) < 0)
    {
        PERROR(rs_info->fifo_fd);
    }

    av_log(NULL, AV_LOG_WARNING, TIME_STR, SYSTIME4TM_FMT(now_tm));
    av_log(NULL, AV_LOG_WARNING, "rtsp_open[id:%d]--->%s\n", rs_info->self_idx, auth_url);

    av_dict_free(&options);

    rs_info->cb_func = rtsp_callback;
    rs_info->priv_data = priv_data;
    rs_info->fmt_ctx = fmt_ctx;
    rs_info->ts_method = TS_REMOTE_METHOD;
    rs_info->ts_error_count = 0;
    rs_info->ts_base = 0;
    rs_info->ts = 0;
    rs_info->start = rs_info->systm_last = av_gettime();
    rs_info->cb_time = rs_info->start;
    rs_info->cb_time_last = 0;
    rs_info->open_time = rs_info->start;
    rs_info->send_start = rs_info->start;
    rs_info->first_keyframe = 0;

    rs_info->average_recv_gap = 0;
    rs_info->total_recv_gap = 0;
    rs_info->total_recv_count = 0;
    rs_info->last_recv_ts = 0;
    rs_info->jitter = 0;
    rs_info->cur_transport_priv = NULL;


    rs_info->average_ts_gap = 0;
    rs_info->total_ts_gap = 0;
    rs_info->total_ts_count = 0;
    rs_info->iframe_count = 0;
    rs_info->vframe_count = 0;
    rs_info->aframe_count = 0;
    rs_info->meta_count = 0;
    rs_info->frame_count = 0;
    rs_info->ts_40ms_cnt = 0;
    rs_info->ts_20ms_cnt = 0;
    rs_info->ts_10ms_cnt = 0;
    rs_info->ts_exceed_cnt = 0;
    rs_info->ts_new_cnt = 0;
    rs_info->recv_bytes = 0;
    rs_info->max_vframe = 0;

    rs_info->calc_index = 0;
    rs_info->calc_jitter_idx = 0;

    rs_info->eagain1 = 0;
    rs_info->eagain2 = 0;

    rs_info->pkt_lost_current = 0;
    rs_info->pkt_lost = 0;
    rs_info->vdrop_count = 0;

    rs_info->have_sps = 0;
    rs_info->dealed_sps = 0;

    rs_info->so_big_count = 0;
    rs_info->max_com_frame = 0;
    rs_info->max_video_cb = 0;
    rs_info->max_audio_cb = 0;
    rs_info->max_meta_cb = 0;

    /* av_log(NULL, AV_LOG_WARNING, "[sps:%p]\n", rs_info->sps); */

    for (i = 0; i < CALC_PERIOD; i++)
    {
        rs_info->one_sec_rev_total[i] = 0;
        rs_info->one_sec_rev_count[i] = 0;
        rs_info->one_sec_rev_average[i] = 0;

        rs_info->one_sec_ts_total[i] = 0;
        rs_info->one_sec_ts_count[i] = 0;
        rs_info->one_sec_ts_average[i] = 0;
    }
    rs_info->fps = 25;

    for (i = 0; i < CALC_JITTER_PERIOD; i++)
    {
        rs_info->one_sec_ts_10ms_cnt[i] = 0;
        rs_info->one_sec_ts_20ms_cnt[i] = 0;
        rs_info->one_sec_ts_40ms_cnt[i] = 0;
        rs_info->one_sec_ts_cnt[i] = 0;
    }

    if (!fmt_ctx->frame_buf)
    {
        fmt_ctx->frame_buf = av_mallocz(MAX_FRAME_SIZE);

        if (!fmt_ctx->frame_buf)
        {
            ret = AVERROR(ENOMEM);
            goto ERR_MALLOC;
        }
        fmt_ctx->frame_buf_len = MAX_FRAME_SIZE;
        fmt_ctx->frame_num = 0;
    }

    if (!fmt_ctx->audio_buf)
    {
        fmt_ctx->audio_buf = av_mallocz(MAX_AUDIO_SIZE);

        if (!fmt_ctx->audio_buf)
        {
            ret = AVERROR(ENOMEM);
            goto ERR_MALLOC;
        }
        fmt_ctx->audio_buf_len = MAX_AUDIO_SIZE;
    }

    if (!fmt_ctx->h264_ctx.avctx)
    {
        fmt_ctx->h264_ctx.avctx = av_mallocz(sizeof(AVCodecContext));

        if (!fmt_ctx->h264_ctx.avctx)
        {
            ret = AVERROR(ENOMEM);
            goto ERR_MALLOC;
        }
    }

    if (!fmt_ctx->hevc_ctx.avctx)
    {
        fmt_ctx->hevc_ctx.avctx = av_mallocz(sizeof(AVCodecContext));

        if (!fmt_ctx->hevc_ctx.avctx)
        {
            ret = AVERROR(ENOMEM);
            goto ERR_MALLOC;
        }
    }

    /*注册中断回调，目前只在添加超时之前删除会有作用*/
    rt = (RTSPState *)fmt_ctx->priv_data;
    if (rt->rtsp_hd)
    {
        rt->rtsp_hd->interrupt_callback.callback = rtsp_interrupt_cb;
        rt->rtsp_hd->interrupt_callback.opaque = rs_info;
        tcp_ctx = (TCPContext *)rt->rtsp_hd->priv_data;
    }

    if (rt->rtsp_streams[0])
    {
        rt->rtsp_streams[0]->feedback = 1;
    }

    rs_info->tcp_fd = -1;

    /*注册epoll管理句柄*/
    if (tcp_ctx && tcp_ctx->fd > 0)
    {
        int val = 1; /* DISABLE TOE : val = 0 ENABLE TOE: val = 1*/
        int len = sizeof(int);
        rs_info->tcp_fd = tcp_ctx->fd;
        av_log(NULL, AV_LOG_INFO, "rs_info->tcp_fd = %d\n", rs_info->tcp_fd);

#ifdef PLATFORM_HI3535
        ret = setsockopt(rs_info->tcp_fd, SOL_SOCKET, SO_TOE_ENABLE, &val, len);
        if (ret < 0)
        {
            av_log(NULL, AV_LOG_ERROR, "SO_TOE_ENABLE");
        }
#endif

#ifdef PLATFORM_HI3536
        ret = setsockopt(rs_info->tcp_fd, SOL_SOCKET, SO_TOE_ENABLE, &val, len);
        if (ret < 0)
        {
            av_log(NULL, AV_LOG_ERROR, "SO_TOE_ENABLE");
        }
#endif
        data_ready[rs_info->self_idx / DATA_MASK_BITS] &= ~(1 << (rs_info->self_idx % DATA_MASK_BITS));
    }

    /*注册cli调试信息*/
    av_url_split(NULL, 0, auth, sizeof(auth), host, sizeof(host), &port, path, sizeof(path), auth_url);
    strncpy(rs_info->ipaddr, host, 32);
    rs_info->cli_id = rss_take_unused_cli();
    av_log(NULL, AV_LOG_WARNING, "rs_info->ipc_id = %d\n", rs_info->cli_id);
    rtsp_cli = rss_get_cli(rs_info->cli_id);
    if (rtsp_cli)
    {
        memset(rtsp_cli, 0x00, sizeof(RTSP_CLINFO));
        rtsp_cli->used = 1;
        rtsp_cli->min_fps = 25;
        strncpy(rtsp_cli->ipadrr, host, 32);
        strncpy(rtsp_cli->rtsp, url, 128);
        rtsp_cli->open_tm = rs_info->start;
        rtsp_cli->fd = rs_info->tcp_fd;
        rtsp_cli->rcvq_buf = -1;
        rtsp_cli->sndq_buf = -1;
    }
    /* usleep(500000); */
    rss_unlock_session(handle);
    /*管理进程准备就绪，1位对应一个管理进程*/
    session_ready[rs_info->self_idx / DATA_MASK_BITS] |= 1 << (rs_info->self_idx % DATA_MASK_BITS);
    rs_info->state = RSESSION_STATE_DISPATCHED;
    return handle;

ERR_MALLOC:
    fifo_close(rs_info->fifo_fd);

    if (fmt_ctx && fmt_ctx->frame_buf)
    {
        av_free(fmt_ctx->frame_buf);
        fmt_ctx->frame_buf = NULL;
    }
    fmt_ctx->frame_buf_len = 0;

    if (fmt_ctx && fmt_ctx->audio_buf)
    {
        av_free(fmt_ctx->audio_buf);
        fmt_ctx->audio_buf = NULL;
    }
    fmt_ctx->audio_buf_len = 0;

    if (fmt_ctx->h264_ctx.avctx)
    {
        av_free(fmt_ctx->h264_ctx.avctx);
        fmt_ctx->h264_ctx.avctx = NULL;
    }

    if (fmt_ctx->hevc_ctx.avctx)
    {
        av_free(fmt_ctx->hevc_ctx.avctx);
        fmt_ctx->hevc_ctx.avctx = NULL;
    }

    rss_unlock_session(handle);

ERR_LOCK_SESSION:
    rss_untake_fd(handle);

ERR_TAKE_UNUSED_FD:
    avformat_close_input(&fmt_ctx);
    avformat_free_context(fmt_ctx);

ERR_OPEN_CTX:
    av_dict_free(&options);
    return ret;
}

int rtsp_close(int fd)
{
    int ret = 0, fifo_fd = -1;
    int64_t start = 0;
    RTSPSessionInfo * rs_info = NULL;
    AVFormatContext * fmt_ctx = NULL;
    RTSPState * rt = NULL;
    TCPContext * tcp_ctx = NULL;
    RTSP_CLINFO * rtsp_cli = NULL;
    int64_t now = 0;
    SYSTIME now_tm;
    now = av_gettime();
    time_sec2time(now / 1000000, &now_tm);

    /*1. See if this session is OK to be closed*/

    /* av_log(NULL, AV_LOG_INFO, TIME_STR, SYSTIME4TM_FMT(now_tm)); */
    /* start = av_gettime_relative(); */
    rs_info = rss_get_session(fd);
    if (!rs_info || rs_info->in_use == 0)
    {
        av_log(NULL, AV_LOG_ERROR, "rtsp_close: empty session fd=%d rs_info=0x%08x in_use=%d\n",
                 fd, (unsigned int)rs_info, rs_info->in_use);
        ret = RTSP_ERR_PARAM;
        goto ERR_RSS;
    }
    av_log(NULL, AV_LOG_WARNING, "[fd:%d id:%d]rss_get_session: %lld ms\n", rs_info->tcp_fd, rs_info->self_idx, (av_gettime_relative() - start) / 1000);

    rs_info->state = RSESSION_STATE_CLOSED;
    rs_info->close_time = now;
    rtsp_cli = rss_get_cli(rs_info->cli_id);
    if (rtsp_cli)
    {
        rtsp_cli->close_tm = now;
    }

    start = av_gettime_relative();
    /*2 .Lock the session*/
    if (rss_lock_session(fd) < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "rtsp_close:Err locking session#%d\n", fd);
        ret = RTSP_ERR_LOCK_SESSION;
        goto ERR_LOCK_SESSION;
    }
    av_log(NULL, AV_LOG_WARNING, TIME_STR, SYSTIME4TM_FMT(now_tm));
    av_log(NULL, AV_LOG_WARNING, "[fd:%d id:%d]rss_lock_session: %lld ms\n", rs_info->tcp_fd, rs_info->self_idx, (av_gettime_relative() - start) / 1000);

    /* start = av_gettime_relative(); */
    if (rss_lock_session_cmd(fd) < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "rtsp_close:Err locking cmd_session#%d\n", fd);
        ret = RTSP_ERR_LOCK_SESSION;
        goto ERR_LOCK_SESSION;
    }
    av_log(NULL, AV_LOG_VERBOSE, TIME_STR, SYSTIME4TM_FMT(now_tm));
    av_log(NULL, AV_LOG_VERBOSE, "[fd:%d id:%d]rss_lock_session_cmd: %lld ms\n", rs_info->tcp_fd, rs_info->self_idx, (av_gettime_relative() - start) / 1000);

    fmt_ctx = rs_info->fmt_ctx;
    if (!fmt_ctx)
    {
        av_log(NULL, AV_LOG_ERROR, "rtsp_close: empty context fd=%d fmt_ctx=0x%x\n",
                 fd, (unsigned int) rs_info);
        ret = RTSP_ERR_PARAM;
        goto ERR_FMT_CTX;
    }

    /* start = av_gettime_relative(); */
    ret = epoll_unregister(rs_info);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_WARNING, "epoll_register %d\n", ret);
    }
    rs_info->is_epoll_on = 0;
    av_log(NULL, AV_LOG_VERBOSE, "epoll_unregister: %lld ms\n", (av_gettime_relative() - start) / 1000);

    if (fmt_ctx && fmt_ctx->frame_buf)
    {
        av_free(fmt_ctx->frame_buf);
        fmt_ctx->frame_buf = NULL;
    }
    fmt_ctx->frame_buf_len = 0;

    if (fmt_ctx && fmt_ctx->audio_buf)
    {
        av_free(fmt_ctx->audio_buf);
        fmt_ctx->audio_buf = NULL;
    }
    fmt_ctx->audio_buf_len = 0;

    /*3. Free ffmpeg related resource*/
    start = av_gettime_relative();
    if (fmt_ctx)
    {
        if (fmt_ctx->priv_data)
        {
            rt = (RTSPState *)fmt_ctx->priv_data;
            if (rt->rtsp_hd && rt->rtsp_hd->interrupt_callback.callback)
            {
                rt->rtsp_hd->interrupt_callback.callback = NULL;
                /* av_log(NULL, AV_LOG_ERROR, TIME_STR, SYSTIME4TM_FMT(now_tm)); */
                /* av_log(NULL, AV_LOG_ERROR, "[fd:%d id:%d]url: %s\n", rs_info->tcp_fd, rs_info->self_idx, rt->rtsp_hd->filename); */
            }
        }
        ff_h264_free_context(&fmt_ctx->h264_ctx);
        av_log(NULL, AV_LOG_WARNING, "[fd:%d id:%d]h264_free: %lld ms\n", rs_info->tcp_fd, rs_info->self_idx, (av_gettime_relative() - start) / 1000);
        ff_hevc_free_context(&fmt_ctx->hevc_ctx);
        av_log(NULL, AV_LOG_WARNING, "[fd:%d id:%d]hevc_free: %lld ms\n", rs_info->tcp_fd, rs_info->self_idx, (av_gettime_relative() - start) / 1000);
        avformat_close_input(&fmt_ctx);
        av_log(NULL, AV_LOG_WARNING, "[fd:%d id:%d]avf_close: %lld ms\n", rs_info->tcp_fd, rs_info->self_idx, (av_gettime_relative() - start) / 1000);
        avformat_free_context(fmt_ctx);
        av_log(NULL, AV_LOG_WARNING, "[fd:%d id:%d]avf_free: %lld ms\n", rs_info->tcp_fd, rs_info->self_idx, (av_gettime_relative() - start) / 1000);
        rs_info->fmt_ctx = NULL;
    }
    data_ready[rs_info->self_idx / DATA_MASK_BITS] &= ~(1 << (rs_info->self_idx % DATA_MASK_BITS));
    session_ready[rs_info->self_idx / DATA_MASK_BITS] &= ~(1 << (rs_info->self_idx % DATA_MASK_BITS));

    fifo_fd = rs_info->fifo_fd;
    rs_info->fifo_fd = -1;

    ret = fifo_close(fifo_fd);
    if (ret < 0)
    {
        PERROR(ret);
    }

    /*4. Deinitialize tstamper and rtsp session handle*/
    /* start = av_gettime_relative(); */
    rss_untake_fd(fd);
    rss_unlock_session(fd);
    rss_unlock_session_cmd(fd);
    av_log(NULL, AV_LOG_VERBOSE, TIME_STR, SYSTIME4TM_FMT(now_tm));
    av_log(NULL, AV_LOG_VERBOSE, "rss_unlock_session: %lld ms\n", (av_gettime_relative() - start) / 1000);
    return 0;

ERR_FMT_CTX:
    av_log(NULL, AV_LOG_ERROR, "[rss:%d]ERR_FMT_CTX\n", fd);
ERR_LOCK_SESSION:
    rss_unlock_session(fd);
    rss_unlock_session_cmd(fd);
    rss_untake_fd(fd);
ERR_RSS:
    return ret;
}

E_RTSP_STREAM_STATE rtsp_get_state(int fd)
{
    RTSPSessionInfo * rs_info = rss_get_session(fd);
    if (!rs_info)
    {
        av_log(NULL, AV_LOG_ERROR, "rtsp_get_state: empty session = %d\n", fd);
        return RTSP_ERR_PARAM;
    }
    if (rs_info->state == RSESSION_STATE_CLOSED)
    {
        return RTSP_ST_EOF;
    }
    else
    {
        return RTSP_ST_OK;
    }
}

int rtsp_get_ipaddr(int fd, char * ipaddr)
{
    RTSPSessionInfo * rs_info = rss_get_session(fd);
    if (!rs_info || rs_info->state == RSESSION_STATE_IDLE || NULL == ipaddr)
    {
        av_log(NULL, AV_LOG_ERROR, "rtsp_get_ipaddr RTSP_ERR_PARAM!\n");
        return RTSP_ERR_PARAM;
    }

    strncpy(ipaddr, rs_info->ipaddr, 32);

    return 0;
}

static int  epollfd = 0;
#define MAX_EVENTS      64
static pthread_mutex_t _gmutex = PTHREAD_MUTEX_INITIALIZER;
#define     TMP_BUF_SIZE (512<<10)

#define _DIRTY_BUF_DBG  0

void __rtsp_stream_body(void * arg)
{
    struct epoll_event events[MAX_EVENTS];
    FIFO_BUFF_INFO_S buff_info = {0};
    unsigned long long selfid_64 = 0;
    int i, ret = 0 , rtval = 0 , recv_size = 0;
    int _fd = 0;
    RTSPSessionInfo * rs_info = NULL;
    struct timeval start, end;
    int force_out = 0;
    int total_bytes = 0, total_big_count = 0;

    uint8 * dirty_buf = NULL;   /* 如果接收缓存满了, 把所有的接收缓存数据收下，清空接收缓存 */

    /*prctl(PR_SET_NAME, "rtsp_stream");*/

restart:
#if _DIRTY_BUF_DBG
    dirty_buf = mem_manage_malloc(COMP_RTSP, TMP_BUF_SIZE);
    if (NULL == dirty_buf)
    {
        av_log(NULL, AV_LOG_ERROR, "dirty_buf malloc fail!\n");
        usleep(1000000);
        goto restart;
    }
#endif

    epollfd = epoll_create(MAX_EVENTS);

    gettimeofday(&start, NULL);

    while (1)
    {
        int fds = epoll_wait(epollfd, events, MAX_EVENTS, 1);
        if (fds < 0)
        {
            if ((errno == EAGAIN) || (errno == EINTR))
            {
                /* DBG_PRINTFA; */
                av_log(NULL, AV_LOG_VERBOSE, "epoll_wait EAGAIN!\n");
                continue;
            }
            /* DBG_PRINTFA; */
            av_log(NULL, AV_LOG_VERBOSE, "epoll_waite fail!\n");
            break;
        }
        if (fds == 0)
        {
            /*av_log(NULL, AV_LOG_VERBOSE, "epoll_wait timeout!\n");*/
            continue;
        }

        total_bytes = 0;

        total_big_count = 0;

        for (i = 0; i < fds; i++)
        {
            rs_info = (RTSPSessionInfo *)events[i].data.ptr;
            _fd = rs_info->tcp_fd;
            force_out = 0;

            if (rs_info->state == RSESSION_STATE_CLOSED)
            {
                av_log(NULL, AV_LOG_INFO, "[fd:%d]rtsp normal closed\n", rs_info->tcp_fd);
                force_out = 1;
                recv_size = -1;
                goto done;
            }

#if 0
            if (rs_info->so_big_count > 10000)
            {
                av_log(NULL, AV_LOG_ERROR, "[fd:%d]rtsp so_big closed\n", rs_info->tcp_fd);
                force_out = 1;
                recv_size = -1;
                goto done;
            }
#else
            if (rs_info->so_big_count > 10000)
            {
                rs_info->so_big_count = 0;
                av_usleep(10);
            }
#endif

            if (events[i].events & EPOLLIN)
            {
                ret = fifo_write_buffer_get(rs_info->fifo_fd, &buff_info, 0);
                if (ret == 0)
                {
                    /*通常为2，buf掉头，需要接收两次*/
                    if (buff_info.buff_count == 2)
                    {
                        if (buff_info.buff[0].size > 0)
                        {
                            /*按照free_size接收数据，一般情况free_size>>recv_size*/
                            recv_size = recv(_fd, buff_info.buff[0].pbuff, buff_info.buff[0].size, 0);
                            if (recv_size <= 0)
                            {
                                if ((errno == EAGAIN))
                                {
                                    /* av_log(NULL, AV_LOG_ERROR,  "fd = %d free = %d \n", _fd, buff_info.buff[0].size); */
                                }
                                /* perror("[2-0]"); */
                                goto done;
                            }

                            total_bytes += recv_size;

                            /*更新fifo*/
                            ret = fifo_write_buffer_update(rs_info->fifo_fd, &buff_info, recv_size);
                            if (ret < 0)
                            {
                                av_log(NULL, AV_LOG_WARNING, "fifo_update_buffer 1 = %x\n", ret);
                            }

                            /*数据已经写入标记，1位对应一路*/
                            data_ready[rs_info->self_idx / DATA_MASK_BITS] |= 1 << (rs_info->self_idx % DATA_MASK_BITS);
                            //av_log(NULL, AV_LOG_DEBUG,  "[sfd:%d][idx:%d] data_ready=[%x]\n", rs_info->tcp_fd, rs_info->self_idx / DATA_MASK_BITS, data_ready[rs_info->self_idx / DATA_MASK_BITS]);
                            /*如果recv_size和free_size相等，说明可能还有数据没有收完，使用buf[1]继续接收*/
                            if (recv_size == buff_info.buff[0].size)
                            {
                                if (buff_info.buff[1].size > 0)
                                {
                                    recv_size = recv(_fd, buff_info.buff[1].pbuff, buff_info.buff[1].size, 0);
                                    if (recv_size <= 0)
                                    {
                                        if ((errno == EAGAIN))
                                        {
                                            /* av_log(NULL, AV_LOG_ERROR, "[%d]fd = %d free = %d \n", rs_info->self_idx, _fd, buff_info.buff[1].size); */
                                        }
                                        /* perror("[2-1]"); */
                                        goto done;
                                    }

                                    total_bytes += recv_size;

                                    /*更新fifo*/
                                    ret = fifo_write_buffer_update(rs_info->fifo_fd, &buff_info, recv_size);
                                    if (ret < 0)
                                    {
                                        av_log(NULL, AV_LOG_WARNING,  "fifo_update_buffer 2 = %x\n", ret);
                                    }

                                    /*如果recv_size和free_size相等，说明可能还有数据没有收完，fifo已经没有空间，停止接收，防止数据错乱*/
                                    if (recv_size == buff_info.buff[1].size)
                                    {
                                        int fifo_size;
                                        fifo_size = fifo_read_buffer_get(rs_info->fifo_fd, &buff_info);
#if _DIRTY_BUF_DBG
                                        /* 接收到临时缓存并丢弃 */
                                        recv_size = recv(_fd, dirty_buf, TMP_BUF_SIZE, 0);
                                        if (recv_size <= 0)
                                        {
                                            if ((errno == EAGAIN))
                                            {
                                                /* av_log(NULL, AV_LOG_ERROR,  "[%d]fd = %d free = %d \n", rs_info->self_idx, _fd, buff_info.buff[1].size); */
                                            }
                                            goto done;
                                        }
#endif
                                        if (rs_info->so_big_count ++ == INT_MAX)
                                            rs_info->so_big_count = 0;
                                        av_log(NULL, AV_LOG_INFO,  "[fd:%d] [fifo:%d] so big 2 = %d \n", _fd, fifo_size, recv_size);
                                    }
                                }
                                else
                                {
                                    av_log(NULL, AV_LOG_VERBOSE,  "[fd:%d] buff[2-0].size is zero\n", _fd);
                                }
                            }

                        }
                        else
                        {
                            av_log(NULL, AV_LOG_VERBOSE, "[fd:%d] buff[2-0].size is zero\n", _fd);
                        }
                    }
                    else if (buff_info.buff_count == 1)
                    {
                        if (buff_info.buff[0].size > 0)
                        {
                            /*按照free_size接收数据，一般情况free_size>>recv_size*/
                            recv_size = recv(_fd, buff_info.buff[0].pbuff, buff_info.buff[0].size, 0);
                            if (recv_size <= 0)
                            {
                                if ((errno == EAGAIN))
                                {
                                    /* av_log(NULL, AV_LOG_ERROR, "[%d]fd = %d free = %d ", rs_info->self_idx, _fd, buff_info.buff[0].size); */
                                }
                                /* perror("[1-0]"); */
                                goto done;
                            }

                            total_bytes += recv_size;

                            /*更新fifo*/
                            ret = fifo_write_buffer_update(rs_info->fifo_fd, &buff_info, recv_size);
                            if (ret < 0)
                            {
                                av_log(NULL, AV_LOG_WARNING,  "fifo_update_buffer 3 = %x\n", ret);
                            }

                            /*数据已经写入标记，1位对应一路*/
                            data_ready[rs_info->self_idx / DATA_MASK_BITS] |= 1 << (rs_info->self_idx % DATA_MASK_BITS);
                            //av_log(NULL, AV_LOG_DEBUG, "[sfd:%d][idx:%d] data_ready=[%x]\n", rs_info->tcp_fd, rs_info->self_idx / DATA_MASK_BITS, data_ready[rs_info->self_idx / DATA_MASK_BITS]);

                            /*如果recv_size和free_size相等，说明可能还有数据没有收完，fifo已经没有空间，停止接收，防止数据错乱*/
                            if (recv_size == buff_info.buff[0].size)
                            {
                                int fifo_size;
                                fifo_size = fifo_read_buffer_get(rs_info->fifo_fd, &buff_info);
#if _DIRTY_BUF_DBG
                                /* 接收到临时缓存并丢弃 */
                                recv_size = recv(_fd, dirty_buf, TMP_BUF_SIZE, 0);
                                if (recv_size <= 0)
                                {
                                    if ((errno == EAGAIN))
                                    {
                                        /* av_log(NULL, AV_LOG_ERROR,  "[%d]fd = %d free = %d \n", rs_info->self_idx, _fd, buff_info.buff[1].size); */
                                    }
                                    goto done;
                                }
#endif
                                if (rs_info->so_big_count ++ == INT_MAX)
                                    rs_info->so_big_count = 0;
                                av_log(NULL, AV_LOG_INFO,  "[fd:%d] [fifo:%d] so big 1 = %d \n", _fd, fifo_size, recv_size);
                            }
                        }
                        else
                        {
#if _DIRTY_BUF_DBG
                            /* 接收到临时缓存并丢弃 */
                            recv_size = recv(_fd, dirty_buf, TMP_BUF_SIZE, 0);
                            if (recv_size <= 0)
                            {
                                if ((errno == EAGAIN))
                                {
                                    /* av_log(NULL, AV_LOG_ERROR,  "[%d]fd = %d free = %d \n", rs_info->self_idx, _fd, buff_info.buff[1].size); */
                                }
                                goto done;
                            }
#endif
                            if (rs_info->so_big_count ++ == INT_MAX)
                                rs_info->so_big_count = 0;
                            av_log(NULL, AV_LOG_VERBOSE, "[fd:%d] buff[1-0].size is zero\n", _fd);
                        }
                    }
                }
                else
                {
                    av_log(NULL, AV_LOG_ERROR, "fifo_get_buffer error = %x\n", ret);
                    recv_size = -1;
                    force_out = 1;
                }

                rs_info->recv_bytes += recv_size;
                continue;
done:
                if (recv_size < 0)
                {
                    if (force_out)
                    {
                        rs_info->state = RSESSION_STATE_CLOSED;
                    }
                    else if ((errno == EAGAIN) || (errno == EINTR))
                    {
                        continue;
                    }
                    av_log(NULL, AV_LOG_INFO, "fd:%d ret:%d\n", _fd, recv_size);
                    epoll_del(epollfd, rs_info);
                }
                else if (recv_size == 0)
                {
                    av_log(NULL, AV_LOG_INFO, "remote closed !\n");
                    epoll_del(epollfd, rs_info);
                }
            }
            else if ((EPOLLHUP | EPOLLERR) & events[i].events)
            {
                av_log(NULL, AV_LOG_INFO, "fd:%d recv_size:%d\n", _fd, recv_size);
                epoll_del(epollfd, rs_info);
            }
            else
            {
                av_log(NULL, AV_LOG_INFO, "fd:%d events else:%x\n", _fd, events[i].events);
            }
        }
    }
    closesocket(epollfd);
    if (NULL != dirty_buf)
    {
        av_free(dirty_buf);
    }
    av_usleep(1000);
    av_log(NULL, AV_LOG_WARNING, "epoll failed restart\n");
    goto restart;
}

int epoll_del(int e_fd, RTSPSessionInfo * rs_info)
{
    struct epoll_event ev = {0};
    int ret = 0;
    int64_t now = 0;
    SYSTIME now_tm;

    pthread_mutex_lock(&_gmutex);

    now = av_gettime();
    time_sec2time(now / 1000000, &now_tm);

    rs_info->length = -1;

    if (rs_info->tcp_fd > 0)
    {
        ret = epoll_ctl(e_fd, EPOLL_CTL_DEL, rs_info->tcp_fd, NULL);
        if (ret < 0)
        {
            av_log(NULL, AV_LOG_ERROR, "epoll_del fail\n");
        }
        else
        {
            /* close(rs_info->tcp_fd); */
            av_log(NULL, AV_LOG_WARNING, TIME_STR, SYSTIME4TM_FMT(now_tm));
            av_log(NULL, AV_LOG_WARNING, "[fd:%d id:%d]epoll del fd\n", rs_info->tcp_fd, rs_info->self_idx);
        }
        rs_info->tcp_fd = -1;
    }
    pthread_mutex_unlock(&_gmutex);
    return 0;
}

int epoll_add(int e_fd, int events, RTSPSessionInfo * rs_info)
{
    struct epoll_event ev = {0};
    int op;
    char ipaddr[32] = {0};
    int64_t now = 0;
    SYSTIME now_tm;

    pthread_mutex_lock(&_gmutex);

    now = av_gettime();
    time_sec2time(now / 1000000, &now_tm);

    ev.events = events;
    ev.data.ptr = rs_info;

    rs_info->length = 1;

    rtsp_get_ipaddr(rs_info->self_idx, ipaddr);

    if (rs_info->tcp_fd > 0)
    {
        if (epoll_ctl(e_fd, EPOLL_CTL_ADD, rs_info->tcp_fd, &ev) < 0)
        {
            perror("epoll_add");
            pthread_mutex_unlock(&_gmutex);
            return -1;
        }
        else
        {
            av_log(NULL, AV_LOG_WARNING, TIME_STR, SYSTIME4TM_FMT(now_tm));
            av_log(NULL, AV_LOG_WARNING, "[fd:%d id:%d]epoll add %s\n", rs_info->tcp_fd, rs_info->self_idx, ipaddr);
        }
    }
    pthread_mutex_unlock(&_gmutex);
    return 0;
}

static int epoll_register(RTSPSessionInfo * rs_info)
{
    return epoll_add(epollfd, EPOLLIN, rs_info);
}

static int epoll_unregister(RTSPSessionInfo * rs_info)
{
    return epoll_del(epollfd, rs_info);
}


#define START_CODE_LEN  4
#define RTP_INTERLEAVED_LEN 4
#define RTP_HEADER_LEN  12
#define RTP_FIX_EXT_LEN 4

#define RTP_PT_H264     96
#define RTP_PT_MULAW    0
#define RTP_PT_ALAW     8

#define RTSP_MAX_FRAME_SIZE 2048 << 10

static const uint8_t start_code[] = { 0, 0, 0, 1 };

int __rtsp_open_input(AVFormatContext ** ps, const char * filename, AVInputFormat * fmt, AVDictionary ** options)
{
    AVFormatContext * s = *ps;
    int ret = 0;
    AVDictionary * tmp = NULL;
    HEVCNAL * tmp_nal = NULL;

    if (!s && !(s = avformat_alloc_context()))
        return AVERROR(ENOMEM);
    if (!s->av_class)
    {
        return AVERROR(EINVAL);
    }
    if (fmt)
        s->iformat = fmt;

    if (options)
        av_dict_copy(&tmp, *options, 0);

    if ((ret = av_opt_set_dict(s, &tmp)) < 0)
        goto fail;

    s->duration = s->start_time = AV_NOPTS_VALUE;
    av_strlcpy(s->filename, filename ? filename : "", sizeof(s->filename));

    /* Allocate private data. */
    if (s->iformat->priv_data_size > 0)
    {
        if (!(s->priv_data = av_mallocz(s->iformat->priv_data_size)))
        {
            ret = AVERROR(ENOMEM);
            goto fail;
        }
        if (s->iformat->priv_class)
        {
            *(const AVClass **) s->priv_data = s->iformat->priv_class;
            av_opt_set_defaults(s->priv_data);
            if ((ret = av_opt_set_dict(s->priv_data, &tmp)) < 0)
                goto fail;
        }
    }

    /*if (s->iformat->read_header)
        if ((ret = rtsp_read_header(s)) < 0)
            goto fail;*/

    ret = ff_rtsp_connect(s);
    if (ret < 0)
        goto fail;

    ret = rtsp_read_play(s);
    if (ret < 0)
    {
        ff_rtsp_close_streams(s);
        ff_rtsp_close_connections(s);
        goto fail;
    }

    if (options)
    {
        av_dict_free(options);
        *options = tmp;
    }
    *ps = s;
    return 0;

fail:
    av_dict_free(&tmp);
    if (s->pb && !(s->flags & AVFMT_FLAG_CUSTOM_IO))
        avio_close(s->pb);

    avformat_free_context(s);
    *ps = NULL;
    return ret;
}


void __rtsp_print_frameinfo(RTSPSessionInfo * rs_info)
{
    SYSTIME rec_tm, now_tm;
    int64_t now = 0;

#if 1
    now = av_gettime();
    /* if (rs_info && rs_info->key_frame && rs_info->frame_width >= 1280) */
    /* if (rs_info->frame_width >= 1280) */
    if (rs_info && rs_info->key_frame)
    {
        time_sec2time(now / 1000000, &now_tm);
        time_sec2time(rs_info->cb_info.time_sec, &rec_tm);
        av_log(NULL, AV_LOG_VERBOSE, "delay = %d", rs_info->recv_gap / 1000);
        av_log(NULL, AV_LOG_VERBOSE, ".%d ms ", rs_info->cb_info.time_msec);
        av_log(NULL, AV_LOG_VERBOSE, "key = %d fps = %d width = %d height = %d len = %d bytes\n", rs_info->key_frame, rs_info->fps, rs_info->frame_width, rs_info->frame_height, rs_info->frame_size);
    }
#endif
}

void __rtsp_cli_stat(RTSPSessionInfo * rs_info, RTSP_CLINFO * rtsp_cli)
{
    if (rs_info && rtsp_cli)
    {
        if (rs_info->key_frame)
        {
            rtsp_cli->iframe_count++;
            rs_info->iframe_count++;
        }
        rtsp_cli->vframe_count++;
        rtsp_cli->aframe_count = rs_info->aframe_count;
        rtsp_cli->width = rs_info->frame_width;
        rtsp_cli->height = rs_info->frame_height;
        rtsp_cli->enc_type = rs_info->frame_type;
        rtsp_cli->fps = rs_info->fps;
        rtsp_cli->vdrop_count = rs_info->vdrop_count;
        if (rtsp_cli->min_fps > rtsp_cli->fps)
        {
            rtsp_cli->min_fps = rtsp_cli->fps;
        }
        if (rs_info->frame_size > rs_info->max_vframe)
        {
            rs_info->max_vframe = rs_info->frame_size;
        }
        rtsp_cli->max_vframe = rs_info->max_vframe;
        rtsp_cli->max_aframe = rs_info->max_aframe;
        rtsp_cli->ts_new_cnt = rs_info->ts_new_cnt;
        rtsp_cli->ts_exceed_cnt = rs_info->ts_exceed_cnt;
        rtsp_cli->ts_40ms_cnt = rs_info->ts_40ms_cnt;
        rtsp_cli->ts_20ms_cnt = rs_info->ts_20ms_cnt;
        rtsp_cli->ts_10ms_cnt = rs_info->ts_10ms_cnt;
        rtsp_cli->recv_bytes = rs_info->recv_bytes;
    }
}

void __rtsp_cli_err(RTSP_CLINFO * rtsp_cli, int error_code)
{
    if (rtsp_cli)
    {
        rtsp_cli->error_code = error_code;
    }
}
void __rtsp_cli_netstat(RTSP_CLINFO * rtsp_cli, int recvq, int sndq)
{
    if (rtsp_cli)
    {
        rtsp_cli->rcvq_buf = recvq;
        rtsp_cli->sndq_buf = sndq;
    }
}

int __rtsp_calc_timestamp(RTSPSessionInfo * rs_info)
{
    int64_t now = 0, sys_delta, abs_sys_delta, tmp_ts, ts_increment;
    SYSTIME ts_tm, sys_tm;

    now = av_gettime();
    ts_increment = fps_ts[rs_info->fps];
    /*当前系统时间与时间戳基准和偏移量的差值*/
    sys_delta = now - rs_info->ts - rs_info->ts_base;
    abs_sys_delta = abs(sys_delta);

    if (abs_sys_delta > TEN_SECONDS)
    {
        time_sec2time((rs_info->ts + rs_info->ts_base) / 1000000, &ts_tm);
        time_sec2time(now / 1000000, &sys_tm);
        rs_info->ts_base = now;
        rs_info->ts = 0;
        /* av_log(NULL, AV_LOG_VERBOSE, "[fd:%d] rev_gap is [%lld ms]\n", rs_info->tcp_fd, (now - rs_info->systm_last) / 1000); */
        av_log(NULL, AV_LOG_VERBOSE, "[fd:%d] BTS changed from ", rs_info->tcp_fd);
        av_log(NULL, AV_LOG_VERBOSE, TIME_STR, SYSTIME4TM_FMT(ts_tm));
        av_log(NULL, AV_LOG_VERBOSE, "to ");
        av_log(NULL, AV_LOG_VERBOSE, TIME_STR, SYSTIME4TM_FMT(sys_tm));
        av_log(NULL, AV_LOG_VERBOSE, "\n");
        rs_info->ts_new_cnt++;
    }
    else if (abs_sys_delta > FIVEHUNDREDS_MSECS)
    {
        ts_increment = sys_delta > 0 ? (ts_increment + FOURTY_MSECS) : (ts_increment - FOURTY_MSECS);/* 超过500ms误差时，时间增量补偿40ms */
        rs_info->ts_40ms_cnt++;
        rs_info->one_sec_ts_40ms_cnt[rs_info->calc_jitter_idx]++;

#if 0
        time_sec2time((rs_info->ts + rs_info->ts_base + ts_increment) / 1000000, &ts_tm);
        time_sec2time(now / 1000000, &sys_tm);

        av_log(NULL, AV_LOG_VERBOSE, "sys_tm = ");
        av_log(NULL, AV_LOG_VERBOSE, TIME_STR, SYSTIME4TM_FMT(sys_tm));
        av_log(NULL, AV_LOG_VERBOSE, " ts_tm = ");
        av_log(NULL, AV_LOG_VERBOSE, TIME_STR, SYSTIME4TM_FMT(ts_tm));
        av_log(NULL, AV_LOG_VERBOSE, "\n");
#endif
    }
    else if (abs_sys_delta > THREEHUNDREDS_MSECS)
    {
        ts_increment = sys_delta > 0 ? (ts_increment + TWENTY_MSECS) : (ts_increment - TWENTY_MSECS);/* 超过300ms误差时，时间增量补偿20ms */
        rs_info->ts_20ms_cnt++;
        rs_info->one_sec_ts_20ms_cnt[rs_info->calc_jitter_idx]++;
#if 0
        time_sec2time((rs_info->ts + rs_info->ts_base + ts_increment) / 1000000, &ts_tm);
        time_sec2time(now / 1000000, &sys_tm);
        av_log(NULL, AV_LOG_WARNING, "[fd:%d] TS changed [20 ms] sys_delta = %lld fps = %d ", rs_info->tcp_fd, sys_delta, rs_info->fps);
        /*av_log(NULL, AV_LOG_VERBOSE, "sys_tm = ");
        av_log(NULL, AV_LOG_VERBOSE, TIME_STR, SYSTIME4TM_FMT(sys_tm));
        av_log(NULL, AV_LOG_VERBOSE, " ts_tm = ");
        av_log(NULL, AV_LOG_VERBOSE, TIME_STR, SYSTIME4TM_FMT(ts_tm));*/
        av_log(NULL, AV_LOG_VERBOSE, "\n");
#endif
    }
    else if (abs_sys_delta > TWOHUNDREDS_MSECS)
    {
        ts_increment = sys_delta > 0 ? (ts_increment + TEN_MSECS) : (ts_increment - TEN_MSECS);/* 超过200ms误差时，时间增量补偿10ms */
        rs_info->ts_10ms_cnt++;
        rs_info->one_sec_ts_10ms_cnt[rs_info->calc_jitter_idx]++;
#if 0
        time_sec2time((rs_info->ts + rs_info->ts_base + ts_increment) / 1000000, &ts_tm);
        time_sec2time(now / 1000000, &sys_tm);
        av_log(NULL, AV_LOG_WARNING, "[fd:%d] TS changed [10 ms] sys_delta = %lld fps = %d ", rs_info->tcp_fd, sys_delta, rs_info->fps);
		/*av_log(NULL, AV_LOG_VERBOSE, "sys_tm = ");
		av_log(NULL, AV_LOG_VERBOSE, TIME_STR, SYSTIME4TM_FMT(sys_tm));
		av_log(NULL, AV_LOG_VERBOSE, " ts_tm = ");
		av_log(NULL, AV_LOG_VERBOSE, TIME_STR, SYSTIME4TM_FMT(ts_tm));*/
        av_log(NULL, AV_LOG_VERBOSE, "\n");
#endif
    }

    /*确保时间戳不会超过系统时间1.5秒*/

    tmp_ts = rs_info->ts + rs_info->ts_base + ts_increment - now;

    if (tmp_ts > 1500000)
    {
        /* av_log(NULL, AV_LOG_ERROR, "[%s][%lld][%d] TS too big ", name, tmp_ts, rs_info->fps); */

#if 0
        time_sec2time((rs_info->ts + rs_info->ts_base + ts_increment) / 1000000, &ts_tm);
        av_log(NULL, AV_LOG_VERBOSE, "ts_increment = %lld ts_tm_before = ", ts_increment);
        av_log(NULL, AV_LOG_VERBOSE, TIME_STR, SYSTIME4TM_FMT(ts_tm));
#endif

        rs_info->ts = rs_info->last_ts;
        ts_increment = THIRTY_MSECS;

#if 0
        time_sec2time((rs_info->ts + rs_info->ts_base + ts_increment) / 1000000, &ts_tm);
        av_log(NULL, AV_LOG_VERBOSE, " ts_increment = %lld ts_tm_after = ", ts_increment);
        av_log(NULL, AV_LOG_VERBOSE, TIME_STR, SYSTIME4TM_FMT(ts_tm));

        time_sec2time((rs_info->last_ts + rs_info->ts_base) / 1000000, &ts_tm);
        av_log(NULL, AV_LOG_VERBOSE, " last = ");
        av_log(NULL, AV_LOG_VERBOSE, TIME_STR, SYSTIME4TM_FMT(ts_tm));
        av_log(NULL, AV_LOG_VERBOSE, "\n");
#endif
        rs_info->ts_exceed_cnt++;
    }

    if (ts_increment < THIRTY_MSECS)
    {
        ts_increment = THIRTY_MSECS;
        /* av_log(NULL, AV_LOG_ERROR, "TS too small  ts_increment = %lld  fps = %d\n", ts_increment, rs_info->fps); */
    }
    rs_info->last_ts = rs_info->ts;
    rs_info->ts += ts_increment;
    rs_info->systm_last = now;
    rs_info->one_sec_ts_cnt[rs_info->calc_jitter_idx]++;
    return ts_increment;
}

int __rtsp_read_reply(RTSPSessionInfo * rs_info, RTSPMessageHeader * reply,
                      unsigned char ** content_ptr, const char * method)
{
    RTSPState * rt = rs_info->fmt_ctx->priv_data;
    char buf[4096], buf1[1024], buf2[128], *q;
    const char * p;
    unsigned char ch;
    int ret, content_length, line_count = 0, request = 0, count = 0;

    memset(reply, 0, sizeof(*reply));

    /* parse reply (XXX: use buffers) */
    rt->last_reply[0] = '\0';
    for (;;)
    {
        q = buf;
        for (;;)
        {
            ret = fifo_read(rs_info->fifo_fd, (char *)&ch, 1, FIFO_PEEK_YES);
            if (ret < 0)
            {
                av_log(NULL, AV_LOG_ERROR, "[fd:%d] rtsp read 1 bytes error = %x\n ", rs_info->tcp_fd, ret);
                return AVERROR(EAGAIN);
            }
            else if (ret != 1)
            {
                return AVERROR(EAGAIN);
            }

            if (ch == '\n')
            {
                count = 0;
                break;
            }

            count++;

#if 0
            if (count > 1)
            {
                av_log(NULL, AV_LOG_INFO, "0x%x ", ch);
            }
#endif

            if (ch == '$')
            {
                if (count > 1)
                {
                    rs_info->pkt_lost_current = 1;
                    if (count > 1)
                    {
                        av_log(NULL, AV_LOG_WARNING, "\n[fd:%d] skip %d\n", rs_info->tcp_fd, count);

						fifo_read(rs_info->fifo_fd, (char *)&buf2, 32, FIFO_PEEK_NO);

						av_hex_dump(NULL, buf2, 32);
                    }
                }
                return 1;
            }
            else if (ch != '\r')
            {
                if ((q - buf) < sizeof(buf) - 1)
                    *q++ = ch;
            }

            if (count >= 512)
            {
                av_log(NULL, AV_LOG_ERROR, "[fd:%d] skip %d\n", rs_info->tcp_fd, count);
                return AVERROR(EAGAIN);
            }
        }
        *q = '\0';

        /* av_log(NULL, AV_LOG_DEBUG, "line='%s'\n", buf); */

        /* test if last line */
        if (buf[0] == '\0')
            break;
        p = buf;
        if (line_count == 0)
        {
            /* get reply code */
            get_word(buf1, sizeof(buf1), &p);
            if (!strncmp(buf1, "RTSP/", 5))
            {
                get_word(buf1, sizeof(buf1), &p);
                reply->status_code = atoi(buf1);
                av_strlcpy(reply->reason, p, sizeof(reply->reason));
            }
        }
        else
        {
            ff_rtsp_parse_line(reply, p, rt, NULL);
            av_strlcat(rt->last_reply, p,    sizeof(rt->last_reply));
            av_strlcat(rt->last_reply, "\n", sizeof(rt->last_reply));
        }
        line_count++;
    }
    return AVERROR(EAGAIN);
}

int __rtp_parse_header(RTSPSessionInfo * rs_info, FIFO_BUFF_INFO_S * buff_info)
{
    int i, ret, retry, id, interleaved_len;

    RTPDemuxContext * rtp_demux;
    RTSPState * rt = rs_info->fmt_ctx->priv_data;
    RTSPStream * rtsp_st = NULL;
    uint8_t interleaved_buf[8];
    uint8_t rtp_buf[128] = {0};

    unsigned int ssrc;
    int payload_type, seq, flags = 0;
    int ext, csrc;
    AVStream * st;
    uint32_t timestamp;
    int rv = 0, rtp_len = 0, nal_len = 0, stream_index;
    static int seq_count = 0;
    int16_t diff;


    if (rs_info->fmt_ctx && rs_info->fmt_ctx->priv_data)
    {
        rt = (RTSPState *)rs_info->fmt_ctx->priv_data;
    }
    else
    {
        av_log(NULL, AV_LOG_ERROR, "RTSPState error\n ");
        goto again;
    }

    for (;;)
    {
        RTSPMessageHeader reply;

        ret = __rtsp_read_reply(rs_info, &reply, NULL, NULL);
        if (ret < 0)
            return ret;
        if (ret == 1) /* received '$' */
        {
            break;
        }
        /* XXX: parse message */
        if (rt->state != RTSP_STATE_STREAMING)
            return 0;
    }


    /*获取fifo信息*/
    ret = fifo_read(rs_info->fifo_fd, (char *)interleaved_buf, RTP_INTERLEAVED_LEN - 1, FIFO_PEEK_YES);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, " rtsp read 3 bytes error = %d\n ", ret);
        goto again;
    }
    else if (ret != RTP_INTERLEAVED_LEN - 1)
    {
        goto again;
    }

    id  = interleaved_buf[0];
    rtp_len = interleaved_len = AV_RB16(interleaved_buf + 1);

    /*av_log(NULL, AV_LOG_WARNING, "id = %d rtp_len = %d\n", id, rtp_len);*/

    /* av_log(NULL, AV_LOG_DEBUG, "id = %d rtp_len = %d\n", id, rtp_len); */

    if (interleaved_len > RTSP_RCV_FIFO_SIZE || interleaved_len < 8)
    {
        av_log(NULL, AV_LOG_ERROR, "[fd:%d] interleaved_len error = %d\n", rs_info->tcp_fd, interleaved_len);
        goto again;
    }

    /* find the matching stream */
    for (i = 0; i < rt->nb_rtsp_streams; i++)
    {
        rtsp_st = rt->rtsp_streams[i];
        if (id >= rtsp_st->interleaved_min &&
                id <= rtsp_st->interleaved_max)
        {
            stream_index = i;
            goto found;
        }
    }
    goto again;

found:
    rtp_demux = (RTPDemuxContext *)rtsp_st->transport_priv;
    rs_info->rtp_len = rtp_len;
    rs_info->id = id;
    rs_info->cur_transport_priv = rtp_demux;

    if (rs_info->fmt_ctx->streams[stream_index] == NULL ||
            rs_info->fmt_ctx->streams[stream_index]->codec == NULL)
    {
        av_log(NULL, AV_LOG_ERROR, "Invalid context: stream context\n");
        goto fail;
    }

    rs_info->payload_type = rs_info->fmt_ctx->streams[stream_index]->codec->codec_id;

    if (rs_info->have_sps == 0)
    {
        if (rs_info->fmt_ctx->streams[stream_index]->codec->sps_data)
        {
            rs_info->sps_len = rs_info->fmt_ctx->streams[stream_index]->codec->sps_data_size;
            memcpy(rs_info->sps, rs_info->fmt_ctx->streams[stream_index]->codec->sps_data, rs_info->sps_len);
            av_free(rs_info->fmt_ctx->streams[stream_index]->codec->sps_data);
            rs_info->fmt_ctx->streams[stream_index]->codec->sps_data = NULL;
        }

        if (rs_info->fmt_ctx->streams[stream_index]->codec->pps_data)
        {
            rs_info->pps_len = rs_info->fmt_ctx->streams[stream_index]->codec->pps_data_size;
            memcpy(rs_info->pps, rs_info->fmt_ctx->streams[stream_index]->codec->pps_data, rs_info->pps_len);
            av_free(rs_info->fmt_ctx->streams[stream_index]->codec->pps_data);
            rs_info->fmt_ctx->streams[stream_index]->codec->pps_data = NULL;
        }

#if 0
        if (rs_info->sps_len > 0 && rs_info->sps != NULL)
        {
            rs_info->have_sps = 1;
        }
#endif
    }

    /* av_log(NULL, AV_LOG_DEBUG, "codec_id = %d\n", rs_info->payload_type); */

    /*av_log(NULL, AV_LOG_WARNING, "codec_id = %d\n", rs_info->payload_type);*/

    ret = fifo_read(rs_info->fifo_fd, (char *)rtp_buf, RTP_HEADER_LEN, FIFO_PEEK_YES);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, " rtsp read 12 bytes error = %d\n ", ret);
        goto again;
    }
    else if (ret != RTP_HEADER_LEN)
    {
        av_log(NULL, AV_LOG_ERROR, "[fd:%d] rtsp read RTP_HEADER_LEN bytes again!\n ", rs_info->tcp_fd);
        goto again;
    }

    if ((rtp_buf[0] & 0xc0) != (RTP_VERSION << 6))
    {
        av_log(NULL, AV_LOG_ERROR, "[fd:%d] RTP_VERSION error = %d\n ", rs_info->tcp_fd, rtp_buf[0] & 0xc0);
        goto again;
    }

    if (RTP_PT_IS_RTCP(rtp_buf[1]))
    {
        rs_info->is_rtcp = 1;
        memcpy(rs_info->rtcp_buf, rtp_buf, RTP_HEADER_LEN);
        rtp_len -= RTP_HEADER_LEN;
        rs_info->rtp_len = rtp_len;
        return rtp_len;
    }

    rtp_h264_set_marker(rs_info->fmt_ctx, (rtp_buf[1] & 0x80) != 0);

    csrc         = rtp_buf[0] & 0x0f;
    ext          = rtp_buf[0] & 0x10;
    payload_type = rtp_buf[1] & 0x7f;

	/*av_log(NULL, AV_LOG_DEBUG, "payload = %d %d\n", payload_type, rtp_demux->payload_type);*/

    if (rtp_buf[1] & 0x80)
    {
        flags |= RTP_FLAG_MARKER;
    }

    seq       = AV_RB16(rtp_buf + 2);
    timestamp = AV_RB32(rtp_buf + 4);
    ssrc      = AV_RB32(rtp_buf + 8);
    /* store the ssrc in the RTPDemuxContext */
    rtp_demux->ssrc = ssrc;

#if 0
    /* if (payload_type == RTP_PT_H264) */
    {
        /* av_hex_dump(NULL, interleaved_buf, 3); */
        av_log(NULL, AV_LOG_DEBUG, "[fd:%d 0x%x 0x%x 0x%x]: seq = %d len = %d\n", rs_info->tcp_fd, interleaved_buf[0], interleaved_buf[1], interleaved_buf[2], seq, rtp_len);
    }
#endif

    /* NOTE: we can handle only one payload type */
    if (rtp_demux->payload_type != payload_type)
    {
        /* av_log(NULL, AV_LOG_ERROR,"payload_type error = %d\n ", payload_type); */
        /* av_hex_dump(NULL, rtp_buf, rtp_len); */
        goto again;
    }

    /* rs_info->payload_type = payload_type; */

    st = rtp_demux->st;
    /*  only do something with this if all the rtp checks pass... */
    if (!rtp_valid_packet_in_sequence(&rtp_demux->statistics, seq))
    {
        av_log(NULL, AV_LOG_ERROR,
                 "[fd:%d]: PT=%02x: bad cseq %04x expected=%04x\n",
                 rs_info->tcp_fd, payload_type, seq, ((rtp_demux->seq + 1) & 0xffff));
        goto again;
    }

    if (rtp_demux->seq == 0)
    {
        rtp_demux->seq = seq;
    }

    diff = seq - rtp_demux->seq;
    if (diff < 0 || diff > 1)
    {
        /* Packet older than the previously emitted one, drop */
        av_log(NULL, AV_LOG_WARNING, "[fd:%d]: lost packet seq = %d last_seq = %d\n", rs_info->tcp_fd, seq, rtp_demux->seq);
        rs_info->pkt_lost_current++;
    }

#if 0
    if (seq_count++ % 500 == 0)
    {
        av_log(NULL, AV_LOG_WARNING,
                 "RTP: PT=%02x: cseq %04x expected=%04x\n",
                 payload_type, seq, ((rtp_demux->seq + 1) & 0xffff));
    }
#endif

    /*若填充位被设置，则此包包含一到多个附加在末端的填充比特，填充比特不算作
      负载的一部分。填充的最后一个字节指明可以忽略多少个填充比特*/
    if (rtp_buf[0] & 0x20)
    {
        /*int padding = rtp_buf[len - 1];
        if (len >= 12 + padding)
            len -= padding;*/
        rs_info->padding = 1;
    }
    else
    {
        rs_info->padding = 0;
    }

    rtp_demux->seq = seq;
    rtp_len -= RTP_HEADER_LEN;
    rtp_len -= 4 * csrc;

    if (rtp_len < 0)
    {
        goto again;
    }

    /* RFC 3550 Section 5.3.1 RTP Header Extension handling */
    if (ext)
    {
        if (rtp_len < 4)
        {
            av_log(NULL, AV_LOG_WARNING, "ext rtp_len too short = %d\n ", rtp_len);
            return AVERROR(EAGAIN);
        }
        ret = fifo_read(rs_info->fifo_fd, (char *)rtp_buf, RTP_FIX_EXT_LEN, FIFO_PEEK_YES);
        if (ret < 0)
        {
            av_log(NULL, AV_LOG_ERROR, "rtsp read ext 4bytes error = %x\n ", ret);
            goto fail;
        }
        else if (ret != 4)
        {
            goto again;
        }
        /* calculate the header extension length (stored as number
         * of 32-bit words) */
        ext = (AV_RB16(rtp_buf + 2) + 1) << 2;

        /* av_hex_dump(NULL, rtp_buf, 4); */

        if (rtp_len < ext || ext > 128)
        {
            av_log(NULL, AV_LOG_WARNING, "ext too long = %d %d\n ", rtp_len, ext);
            goto again;
        }

        /*  skip past RTP header extension */
        rtp_len -= ext;
        /* av_log(NULL, AV_LOG_DEBUG,  "ext = %d rtp_len = %d\n", ext, rtp_len); */

        ret = fifo_read(rs_info->fifo_fd, (char *)rtp_buf, ext - RTP_FIX_EXT_LEN, FIFO_PEEK_YES);
        if (ret < 0)
        {
            av_log(NULL, AV_LOG_WARNING, "rtsp read ext %d bytes error = %x\n ", ext - RTP_FIX_EXT_LEN, ret);
            goto again;
        }
        else if (ret != ext - RTP_FIX_EXT_LEN)
        {
            goto again;
        }
        /* av_hex_dump(NULL, rtp_buf, ext - RTP_FIX_EXT_LEN); */
    }

    rs_info->rtp_len = rtp_len;
    return rtp_len;

again:
    rs_info->rtp_len = -1;
    return AVERROR(EAGAIN);

fail:
    rs_info->rtp_len = -1;
    return -10;
}

int __rtp_parse_rtcp(RTSPSessionInfo * rs_info, FIFO_BUFF_INFO_S * buff_info)
{
    int ret, payload_len, rtp_len;
    unsigned char rtcp_buf[128];
    unsigned char * buf;
    /* 读取RTCP剩余部分*/

    rtp_len = rs_info->rtp_len < 128 ? rs_info->rtp_len : 128;

    /*获取fifo信息*/
    ret = fifo_read(rs_info->fifo_fd, (char *)(rs_info->rtcp_buf + RTP_HEADER_LEN), rtp_len, FIFO_PEEK_YES);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, " rtsp read rtcp error = %x\n ", ret);
        goto fail;
    }
    else if (ret != rtp_len)
    {
        return AVERROR(EAGAIN);
    }

    buf = rs_info->rtcp_buf;

    while (rtp_len >= 4)
    {
        payload_len = FFMIN(rtp_len + RTP_HEADER_LEN, (AV_RB16(buf + 2) + 1) * 4);

        switch (buf[1])
        {
            case RTCP_SR:
                if (payload_len < 20 - RTP_HEADER_LEN)
                {
                    av_log(NULL, AV_LOG_WARNING, "Invalid length for RTCP SR packet %d %d\n", payload_len, rtp_len);
                    rs_info->rtp_len = -1;
                    return AVERROR(EAGAIN);
                }
                /* av_hex_dump(NULL, buf, payload_len); */
                break;
            case RTCP_BYE:
                rs_info->rtp_len = -1;
                return -RTCP_BYE;
        }

        buf += payload_len;
        rtp_len -= payload_len;
    }
    rs_info->rtp_len = -1;
    return AVERROR(EAGAIN);

fail:
    rs_info->rtp_len = -1;
    return -10;
}

int __rtp_parse_g711(RTSPSessionInfo * rs_info, FIFO_BUFF_INFO_S * buff_info, uint8_t * buf)
{
    int ret, rtp_len = 0;

    if (rs_info == NULL || buff_info == NULL || buf == NULL)
    {
        return -1;
    }

    rtp_len = rs_info->rtp_len;

    ret = fifo_read(rs_info->fifo_fd, (char *)buf, rtp_len, FIFO_PEEK_YES);
    if (ret != rtp_len)
    {
        av_log(NULL, AV_LOG_WARNING, " rtsp read RTP error = %d\n ", ret);
        return AVERROR(EAGAIN);
    }
    return ret;
}

int __rtp_parse_metadata(RTSPSessionInfo * rs_info, FIFO_BUFF_INFO_S * buff_info, uint8_t * buf)
{
    int ret, rtp_len = 0;

    if (rs_info == NULL || buff_info == NULL || buf == NULL)
    {
        return -1;
    }

    rtp_len = rs_info->rtp_len;

    ret = fifo_read(rs_info->fifo_fd, (char *)buf, rtp_len, FIFO_PEEK_YES);
    if (ret != rtp_len)
    {
        av_log(NULL, AV_LOG_WARNING, " rtsp read metadata error = %d\n ", ret);
        return AVERROR(EAGAIN);
    }
    return ret;
}


int __rtp_parse_unknown(RTSPSessionInfo * rs_info, FIFO_BUFF_INFO_S * buff_info, uint8_t * buf)
{
    int ret, rtp_len = 0;

    if (rs_info == NULL || buff_info == NULL || buf == NULL)
    {
        return -1;
    }

    rtp_len = rs_info->rtp_len;

    ret = fifo_read(rs_info->fifo_fd, (char *)buf, rtp_len, FIFO_PEEK_YES);
    if (ret != rtp_len)
    {
        av_log(NULL, AV_LOG_WARNING, " rtsp read RTP error = %d\n ", ret);
        return AVERROR(EAGAIN);
    }
    return ret;
}

int __rtsp_find_nal_unit(uint8_t * buf, int size, int * nal_start, int * nal_end)
{
    int i;
    /*  find start */
    *nal_start = 0;
    *nal_end = 0;

    /* fprintf( h264_dbgfile, "size = %d\n",size); */

    i = 0;
    while (   /* ( next_bits( 24 ) != 0x000001 && next_bits( 32 ) != 0x00000001 ) */
        (buf[i] != 0 || buf[i + 1] != 0 || buf[i + 2] != 0x01) &&
        (buf[i] != 0 || buf[i + 1] != 0 || buf[i + 2] != 0 || buf[i + 3] != 0x01)
    )
    {
        i++; /*  skip leading zero */
        /* fprintf( h264_dbgfile, "i1 = %d\n",i); */
        if (i + 4 >= size)
        {
            return 0;    /*  did not find nal start */
        }
    }

    if (buf[i] != 0 || buf[i + 1] != 0 || buf[i + 2] != 0x01) /*  ( next_bits( 24 ) != 0x000001 ) */
    {
        i++;
        /* fprintf( h264_dbgfile, "i2 = %d\n",i); */
    }

    if (buf[i] != 0 || buf[i + 1] != 0 || buf[i + 2] != 0x01)
    {
        /* error, should never happen */ return 0;
    }
    i += 3;
    /* fprintf( h264_dbgfile, "i3 = %d\n",i); */
    *nal_start = i;

    while (   /* ( next_bits( 24 ) != 0x000000 && next_bits( 24 ) != 0x000001 ) */
        (buf[i] != 0 || buf[i + 1] != 0 || buf[i + 2] != 0x01) &&
        (buf[i] != 0 || buf[i + 1] != 0 || buf[i + 2] != 0 || buf[i + 3] != 0x01)
    )
    {
        i++;
        /* fprintf( h264_dbgfile, "i4 = %d\n",i); */
        /*  FIXME the next line fails when reading a nal that ends exactly at the end of the data */
        if (i + 3 >= size)
        {
            *nal_end = size;    /*  did not find nal end, stream ended first */
            return -1;
        }
    }

    *nal_end = i;
    return (*nal_end - *nal_start);
}

int __rtp_parse_h264(RTSPSessionInfo * rs_info, FIFO_BUFF_INFO_S * buff_info, uint8_t * buf)
{
    int i, ret, frame_start_found = 0;
    int rv = 0, rtp_len = 0, nal_len = 0;
    uint8_t nal, type, fu_indicator, fu_header;
    int result = 0, new_frame = 0, padding = 0;
    AVFormatContext * s = rs_info->fmt_ctx;

    rtp_len = rs_info->rtp_len;

    /* 读取1字节NAL类型 */
    ret = fifo_read(rs_info->fifo_fd, (char *)&nal, 1, FIFO_PEEK_YES);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, " rtsp read nal error = %x\n ", ret);
        return ret;
    }
    else if (ret != 1)
    {
        return AVERROR(EAGAIN);
    }
    rtp_len--;

    type = nal & 0x1f;
    s->nalu_type = type;

    /* Simplify the case (these are all the nal types used internally by
     * the h264 codec). */
    if (type >= 1 && type <= 23 && !(type == NAL_SPS || type == NAL_SLICE || type == NAL_SEI))
        type = 0;

    switch (type)
    {
        case NAL_SPS:
        {
            if (rtp_len > 256)
            {
                return AVERROR(EAGAIN);
            }
            s->frame_num = 0;
            new_frame = 1;
            memcpy(buf, start_code, START_CODE_LEN);
            buf[START_CODE_LEN] = nal;
            /* 读取SPS */
            ret = fifo_read(rs_info->fifo_fd, (char *)(buf + START_CODE_LEN + 1), rtp_len, FIFO_PEEK_YES);
            if (ret < 0)
            {
                av_log(NULL, AV_LOG_ERROR, " rtsp read NAL_SPS error = %d\n ", ret);
                goto fail;
            }
            else if (ret != rtp_len)
            {
                return AVERROR(EAGAIN);
            }
#if 0
            av_log(NULL, AV_LOG_DEBUG,  "sps_len = %d rtp_len = %d\n", rs_info->sps_len, rtp_len + START_CODE_LEN + 1);

            av_hex_dump(NULL, buf, rtp_len + START_CODE_LEN + 1);

            if (rs_info->have_sps)
            {
                memcpy(buf, rs_info->sps, rs_info->sps_len);
            }

            av_hex_dump(NULL, buf, rs_info->sps_len);
#endif

            if (padding)
            {
                /* av_hex_dump(NULL, buf, rtp_len+START_CODE_LEN); */
                padding = buf[rtp_len + START_CODE_LEN];
                if (rtp_len >= 12 + padding)
                    rtp_len -= padding;
                /* av_log(NULL, AV_LOG_WARNING,  "sps_padding = %d\n", padding); */

            }
            nal_len = rtp_len + START_CODE_LEN + 1;

            /* av_log(NULL, AV_LOG_DEBUG, "rtp_len = %d\n", nal_len); */
            /* av_hex_dump(NULL, buf, nal_len); */

            if (!rs_info->have_sps || (rs_info->have_sps && memcmp(buf + START_CODE_LEN, rs_info->sps, rs_info->sps_len)))
            {
                memcpy(rs_info->sps, buf + START_CODE_LEN, rtp_len + 1);
                rs_info->sps_len = rtp_len + 1;
                rs_info->have_sps = 1;
                /* av_hex_dump(NULL, buf + START_CODE_LEN, nal_len - START_CODE_LEN); */

                s->h264_ctx.nal_ref_idc   = nal >> 5;/*  00 00 00 01 67 */
                s->h264_ctx.nal_unit_type = nal & 0x1f;
                init_get_bits(&s->h264_ctx.gb, buf + START_CODE_LEN + 1, 8 * rtp_len);
                ff_h264_decode_seq_parameter_set(&s->h264_ctx);
            }

            rtp_h264_set_start_end_bit(s, 1, 0);
            /* av_log(NULL, AV_LOG_DEBUG, "sps_pos = %d\n", s->frame_buf_pos); */
            rs_info->dealed_sps = 1;
            break;
        }
        case NAL_SLICE:
        {
            new_frame = 1;
            s->frame_num++;

            memcpy(buf, start_code, START_CODE_LEN);
            buf[START_CODE_LEN] = nal;

            /* 读取单P帧 */
            ret = fifo_read(rs_info->fifo_fd, (char *)(buf + START_CODE_LEN + 1), rtp_len, FIFO_PEEK_YES);
            if (ret < 0)
            {
                av_log(NULL, AV_LOG_ERROR, " rtsp read NAL_SLICE error = %x\n ", ret);
                goto fail;
            }
            else if (ret != rtp_len)
            {
                return AVERROR(EAGAIN);
            }

            if (padding)
            {
                padding = buf[rtp_len + START_CODE_LEN];
                if (rtp_len >= 12 + padding)
                    rtp_len -= padding;
                /* av_log(NULL, AV_LOG_DEBUG,  "slice_padding = %d\n", padding); */
            }
            nal_len = rtp_len + START_CODE_LEN + 1;

            rtp_h264_set_start_end_bit(s, 1, 0);
            /*av_hex_dump(NULL, buf, 8);*/
            /* av_log(NULL, AV_LOG_DEBUG,  "slice_pos = %d\n", s->frame_buf_pos); */
            break;
        }

        case NAL_PPS:
        case NAL_SEI:
        {
            if (rtp_len > 512)
            {
                return AVERROR(EAGAIN);
            }
        }
        /*未定义的载体类型直接输出，JXJ存在I帧为单RTP包2K*/
        case 0:
        {
            int extradata_len = 0;

            /* 单包I帧, 并且RTP封包没有NAL_SPS */
            if ((!rs_info->dealed_sps) && (NAL_IDR_SLICE == s->nalu_type))
            {
                if (rs_info->sps_len > 0 && rs_info->sps != NULL)
                {
                    memcpy(buf, start_code, START_CODE_LEN);
                    memcpy(buf + START_CODE_LEN, rs_info->sps, rs_info->sps_len);
                    extradata_len = rs_info->sps_len + START_CODE_LEN;
                    s->h264_ctx.nal_ref_idc   = rs_info->sps[0] >> 5; /*  00 00 00 01 67 */
                    s->h264_ctx.nal_unit_type = rs_info->sps[0] & 0x1f;
                    init_get_bits(&s->h264_ctx.gb, rs_info->sps + 1, 8 * (rs_info->sps_len - 1));
                    ff_h264_decode_seq_parameter_set(&s->h264_ctx);
                }
                if (rs_info->pps_len > 0 && rs_info->pps != NULL)
                {
                    memcpy(buf + extradata_len, start_code, START_CODE_LEN);
                    memcpy(buf + extradata_len + START_CODE_LEN, rs_info->pps, rs_info->pps_len);
                    extradata_len += rs_info->pps_len + START_CODE_LEN;
                }
            }

            memcpy(buf + extradata_len, start_code, START_CODE_LEN);
            buf[extradata_len + START_CODE_LEN] = nal;
            /* 读取其他帧 */
            ret = fifo_read(rs_info->fifo_fd, (char *)(buf + extradata_len + START_CODE_LEN + 1), rtp_len, FIFO_PEEK_YES);
            if (ret < 0)
            {
                av_log(NULL, AV_LOG_ERROR, " rtsp read NAL_PPS error = %x\n ", ret);
                goto fail;
            }
            else if (ret != rtp_len)
            {
                return AVERROR(EAGAIN);
            }

            if (padding)
            {
                padding = buf[rtp_len + extradata_len + START_CODE_LEN];
                if (rtp_len >= 12 + padding)
                    rtp_len -= padding;
                /* av_log(NULL, AV_LOG_DEBUG,  "pps_padding = %d\n", padding); */
            }
            nal_len = rtp_len + extradata_len + START_CODE_LEN + 1;
#if 0
            if (type == NAL_SEI)
            {
                /* av_hex_dump(NULL, buf + START_CODE_LEN + 1, rtp_len); */
                init_get_bits(&s->h264_ctx.gb, buf + START_CODE_LEN + 1, 8 * rtp_len);
                ff_h264_decode_sei(&s->h264_ctx);
                /* av_log(NULL, AV_LOG_DEBUG,  "sei_type = %x sei_size = %d\n", s->h264_ctx.sei_type, s->h264_ctx.sei_size); */
            }
#endif
            /* av_log(NULL, AV_LOG_WARNING,  "0_pos = %d\n", s->frame_buf_pos); */
            break;
        }

        case 24:                   /*  STAP-A (one packet, multiple nals) */
        {
            /*  consume the STAP-A NAL */
            char sta_buf[2048] = {0};

            if (rtp_len > sizeof(sta_buf))
            {
                av_log(NULL, AV_LOG_ERROR,
                         "STAP-A size exceeds length: %d %d\n", rtp_len, sizeof(sta_buf));
                goto fail;
            }

            ret = fifo_read(rs_info->fifo_fd, sta_buf, rtp_len, FIFO_PEEK_YES);
            if (ret < 0)
            {
                av_log(NULL, AV_LOG_ERROR,
                         " rtsp read STAP-A NAL error = %x\n ", ret);
                goto fail;
            }
            else if (ret != rtp_len)
            {
                return AVERROR(EAGAIN);
            }

            /* av_log(NULL, AV_LOG_DEBUG, "STAP-A len=%d, start=%02x%02x%02x\n", rtp_len, sta_buf[1], sta_buf[2], sta_buf[3]); */
            /*  first we are going to figure out the total size */
            {
                int pass         = 0;
                int total_length = 0;
                uint8_t * dst     = buf;

                for (pass = 0; pass < 2; pass++)
                {
                    const uint8_t * src = (uint8_t *)sta_buf;
                    int src_len        = rtp_len;

                    while (src_len > 2)
                    {
                        uint16_t nal_size = AV_RB16(src);

                        /*  consume the length of the aggregate */
                        src     += 2;
                        src_len -= 2;

                        if (nal_size <= src_len)
                        {
                            if (pass == 0)
                            {
                                /*  counting */
                                total_length += sizeof(start_code) + nal_size;
                            }
                            else
                            {
                                /*  copying */
                                memcpy(dst, start_code, sizeof(start_code));
                                dst += sizeof(start_code);
                                memcpy(dst, src, nal_size);
                                dst += nal_size;
                                /* av_log(NULL, AV_LOG_DEBUG, " rtp read STAP-A %d %02x\n", nal_size, *src); */
                                nal_len += nal_size + START_CODE_LEN;

                                if (((*src) & 0x1f) == NAL_SPS)
                                {
                                    if (!rs_info->have_sps || (rs_info->have_sps && memcmp(src, rs_info->sps, rs_info->sps_len)))
                                    {
                                        memcpy(rs_info->sps, src, nal_size);
                                        rs_info->sps_len = nal_size;
                                        rs_info->have_sps = 1;
                                        /* av_hex_dump(NULL, src, nal_size); */

                                        s->h264_ctx.nal_ref_idc   = (*src) >> 5;/*  00 00 00 01 67 */
                                        s->h264_ctx.nal_unit_type = (*src) & 0x1f;
                                        init_get_bits(&s->h264_ctx.gb, src + 1, 8 * nal_size);
                                        ff_h264_decode_seq_parameter_set(&s->h264_ctx);
                                    }
                                    rtp_h264_set_start_end_bit(s, 1, 0);
                                    /* av_log(NULL, AV_LOG_DEBUG,  "sps_pos = %d\n", s->frame_buf_pos); */
                                    rs_info->dealed_sps = 1;
                                }
                            }
                        }
                        else
                        {
                            av_log(NULL, AV_LOG_ERROR,
                                     "nal size exceeds length: %d %d\n", nal_size, src_len);
                        }

                        /*  eat what we handled */
                        src     += nal_size;
                        src_len -= nal_size;

                        if (src_len < 0)
                        {
                            av_log(NULL, AV_LOG_ERROR,
                                     "Consumed more bytes than we got! (%d)\n", src_len);
                        }
                    }
                }
            }
            break;
        }

        case 25:                   /*  STAP-B */
        case 26:                   /*  MTAP-16 */
        case 27:                   /*  MTAP-24 */
        case 29:                   /*  FU-B */
        {
            /* av_log(NULL, AV_LOG_DEBUG, "Unhandled type (%d) \n", type); */
            return AVERROR(EAGAIN);
        }
        case 28:                        /*  FU-A (NAL分片) */
        {
            ret = fifo_read(rs_info->fifo_fd, (char *)&fu_header, 1, FIFO_PEEK_YES);
            if (ret < 0)
            {
                av_log(NULL, AV_LOG_ERROR, " rtsp read fu_header error = %x\n ", ret);
                goto fail;
            }
            else if (ret != 1)
            {
                return AVERROR(EAGAIN);
            }

            rtp_len--;                  /* 跳过 fu_indicator */

            if (rtp_len > 0)
            {
                /*  these are the same as above, we just redo them here for clarity */
                uint8_t fu_indicator      = nal;
                uint8_t start_bit         = fu_header >> 7;
                uint8_t end_bit           = (fu_header & 0x40) >> 6;
                uint8_t nal_type          = fu_header & 0x1f;
                uint8_t reconstructed_nal;
                int     extradata_len = 0;

                /*  Reconstruct this packet's true nal; only the data follows. */
                /* The original nal forbidden bit and NRI are stored in this
                 * packet's nal. */
                reconstructed_nal  = fu_indicator & 0xe0;
                reconstructed_nal |= nal_type;
                s->nalu_type = reconstructed_nal & 0x1f;

                if (start_bit)
                {
                    /* 如果RTP封包没有NAL_SPS, 就从RTSP Describe中拷贝过来 */
                    if (!rs_info->dealed_sps && (s->nalu_type == NAL_IDR_SLICE))
                    {
                        if (rs_info->sps_len > 0 && rs_info->sps != NULL)
                        {
                            memcpy(buf, start_code, START_CODE_LEN);
                            memcpy(buf + START_CODE_LEN, rs_info->sps, rs_info->sps_len);
                            extradata_len = rs_info->sps_len + START_CODE_LEN;
                            s->h264_ctx.nal_ref_idc   = rs_info->sps[0] >> 5;/*  00 00 00 01 67 */
                            s->h264_ctx.nal_unit_type = rs_info->sps[0] & 0x1f;
                            init_get_bits(&s->h264_ctx.gb, rs_info->sps + 1, 8 * (rs_info->sps_len - 1));
                            ff_h264_decode_seq_parameter_set(&s->h264_ctx);
                        }
                        if (rs_info->pps_len > 0 && rs_info->pps != NULL)
                        {
                            memcpy(buf + extradata_len, start_code, START_CODE_LEN);
                            memcpy(buf + extradata_len + START_CODE_LEN, rs_info->pps, rs_info->pps_len);
                            extradata_len += rs_info->pps_len + START_CODE_LEN;
                        }
                    }

                    /* copy in the start sequence, and the reconstructed nal */
                    memcpy(buf + extradata_len, start_code, START_CODE_LEN);
                    buf[START_CODE_LEN + extradata_len] = reconstructed_nal;

                    /* 读取FU-A起始包 */
                    ret = fifo_read(rs_info->fifo_fd, (char *)(buf + extradata_len + START_CODE_LEN + 1), rtp_len, FIFO_PEEK_YES);
                    if (ret < 0)
                    {
                        av_log(NULL, AV_LOG_ERROR, " rtsp read FU-A error = %d\n ", ret);
                        goto fail;
                    }
                    else if (ret != rtp_len)
                    {
                        return AVERROR(EAGAIN);
                    }

                    if (s->nalu_type == NAL_SPS)
                    {
                        int nal_start, nal_end, sps_len;
                        uint8_t * p;

                        if ((sps_len = __rtsp_find_nal_unit(buf, rtp_len, &nal_start, &nal_end)) > 0)
                        {
                            /*av_log(NULL, AV_LOG_DEBUG, "!! Found NAL at offset %lld, size %lld %d %d\n",
                                   (long long int)((p - buf) + nal_start),
                                   (long long int)(nal_end - nal_start), (p-buf),nal_start);*/

                            p = buf + nal_start + 1;
                            /* av_hex_dump(NULL, p, sps_len - 1); */

                            s->h264_ctx.nal_ref_idc   = reconstructed_nal >> 5;/*  00 00 00 01 67 */
                            s->h264_ctx.nal_unit_type = reconstructed_nal & 0x1f;
                            init_get_bits(&s->h264_ctx.gb, p, 8 * (sps_len - 1));
                            ff_h264_decode_seq_parameter_set(&s->h264_ctx);

                        }

                        s->nalu_type = NAL_IDR_SLICE;
                    }

                    if (padding)
                    {
                        padding = buf[rtp_len + START_CODE_LEN];
                        if (rtp_len >= 12 + padding)
                            rtp_len -= padding;
                        /* av_log(NULL, AV_LOG_DEBUG,  "fu1_padding = %d\n", padding); */
                    }
                    nal_len = extradata_len + rtp_len + START_CODE_LEN + 1;
                    /* av_log(NULL, AV_LOG_DEBUG, "fu1_pos = %d len = %d\n", s->frame_buf_pos, nal_len); */
                    /* av_hex_dump(NULL, buf, 8); */
                }
                else
                {
                    /* 读取FU-A分片包 */
                    ret = fifo_read(rs_info->fifo_fd, (char *)buf, rtp_len, FIFO_PEEK_YES);
                    if (ret < 0)
                    {
                        av_log(NULL, AV_LOG_ERROR, " rtsp read FU-A error = %x\n ", ret);
                        goto fail;
                    }
                    else if (ret != rtp_len)
                    {
                        return AVERROR(EAGAIN);
                    }

                    if (padding)
                    {
                        /* av_hex_dump(NULL, buf, rtp_len); */
                        padding = buf[rtp_len - 1];
                        if (rtp_len >= 12 + padding)
                            rtp_len -= padding;
                        /* av_log(NULL, AV_LOG_DEBUG,  "fu2_padding = %d\n", padding); */
                    }
                    nal_len = rtp_len;
                    /* av_log(NULL, AV_LOG_DEBUG, "fu2_pos = %d len = %d\n", s->frame_buf_pos, nal_len); */
                    /* av_hex_dump(NULL, buf, 8); */
                }
                /* end_bit = frame_start_found ? end_bit : 0; */
                if (s->nalu_type == NAL_SPS)
                {
                    s->nalu_type = NAL_IDR_SLICE;
                }
                rtp_h264_set_start_end_bit(s, start_bit != 0, end_bit != 0);
            }
            else
            {
                char tmp_buf[8];
                ret = fifo_read(rs_info->fifo_fd, tmp_buf, rtp_len, FIFO_PEEK_YES);
                if (ret < 0)
                {
                    av_log(NULL, AV_LOG_ERROR, "rtsp read short data error = %x\n ", ret);
                    goto fail;
                }
                av_log(NULL, AV_LOG_WARNING, "Too short data for FU-A %d 0x%x\n", rtp_len, tmp_buf[0]);

                /* av_hex_dump(NULL, rtp_buf, 32); */
                return AVERROR(EAGAIN);
            }
            break;
        }
        case 30:                   /*  undefined */
        case 31:                   /*  undefined */
        default:
        {
            av_log(NULL, AV_LOG_ERROR, "Undefined type (%d)\n", type);
            return AVERROR(EAGAIN);
        }
    }

    return nal_len > 0 ? nal_len : result;

fail:
    rs_info->rtp_len = -1;
    return -10;
}

int __rtp_parse_hevc(RTSPSessionInfo * rs_info, FIFO_BUFF_INFO_S * buff_info, uint8_t * buf)
{
    int i, ret;
    int rv = 0, rtp_len = 0, nal_len = 0;
    uint8_t nal[2], nal_type, lid, tid, fu_indicator, fu_header;
    int result = 0, new_frame = 0, padding = 0;
    AVFormatContext * s = rs_info->fmt_ctx;

    rtp_len = rs_info->rtp_len;

    /* 读取2字节NAL类型 */
    ret = fifo_read(rs_info->fifo_fd, (char *)nal, 2, FIFO_PEEK_YES);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, " rtsp read nal error = %x\n ", ret);
        return ret;
    }
    else if (ret != 2)
    {
        av_log(NULL, AV_LOG_WARNING, " rtsp read nal EAGAIN = %x\n ", ret);
        return AVERROR(EAGAIN);
    }
    rtp_len = rtp_len - 2;

    /*
    * decode the HEVC payload header according to section 4 of draft version 9:
    *
    *    0                   1
    *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
    *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    *   |F|   Type    |  LayerId  | TID |
    *   +-------------+-----------------+
    *
    *      Forbidden zero (F): 1 bit
    *      NAL unit type (Type): 6 bits
    *      NUH layer ID (LayerId): 6 bits
    *      NUH temporal ID plus 1 (TID): 3 bits
    */

    nal_type = (nal[0] >> 1) & 0x3f;
    lid = ((nal[0] << 5) & 0x20) | ((nal[1] >> 3) & 0x1f);
    tid = nal[1] & 0x07;

    /* s->nalu_type = nal_type; */

    //av_log(NULL, AV_LOG_DEBUG, "nal_type = %d lid = %d tid = %d\n", nal_type, lid, tid);

    //av_log(NULL, AV_LOG_WARNING, "nal_type = %d lid = %d tid = %d\n", nal_type, lid, tid);

    /* sanity check for correct layer ID */
    if (lid)
    {
        /* future scalable or 3D video coding extensions */
        /* avpriv_report_missing_feature(NULL, "Multi-layer HEVC coding\n"); */
        return AVERROR(EAGAIN);
    }

    /* sanity check for correct temporal ID */
    if (!tid)
    {
        /* av_log(NULL, AV_LOG_ERROR, "Illegal temporal ID in RTP/HEVC packet\n"); */
        return AVERROR(EAGAIN);
    }

    /* sanity check for correct NAL unit type */
    if (nal_type > 50)
    {
        /* av_log(NULL, AV_LOG_ERROR, "Unsupported (HEVC) NAL type (%d)\n", nal_type); */
        return AVERROR(EAGAIN);
    }

    s->hevc_ctx.nal_unit_type = (nal[0] >> 1) & 0x3f;
    s->hevc_ctx.temporal_id   = (nal[1] & 0x07) - 1;

    switch (nal_type)
    {
        case HEVC_NAL_VPS:
        {
            if (rtp_len > 256)
            {
                return AVERROR(EAGAIN);
            }
            s->frame_num = 0;
            new_frame = 1;
            memcpy(buf, start_code, START_CODE_LEN);
            buf[START_CODE_LEN] = nal[0];
            buf[START_CODE_LEN + 1] = nal[1];
            /* 读取VPS */
            ret = fifo_read(rs_info->fifo_fd, (char *)(buf + START_CODE_LEN + 2), rtp_len, FIFO_PEEK_YES);
            if (ret < 0)
            {
                av_log(NULL, AV_LOG_ERROR, " rtsp read HEVC_NAL_VPS error = %d\n ", ret);
                goto fail;
            }
            else if (ret != rtp_len)
            {
                return AVERROR(EAGAIN);
            }
            
			av_log(NULL, AV_LOG_DEBUG, "vps_len = %d \n", rtp_len);
			/*av_hex_dump(NULL, buf + START_CODE_LEN + 2, rtp_len);*/
#if 0
            
            if (rs_info->have_sps)
            {
                memcpy(buf, rs_info->sps, rs_info->sps_len);
            }

            /* av_hex_dump(NULL, buf, rs_info->sps_len); */
#endif
            if (padding)
            {
                /* av_hex_dump(NULL, buf, rtp_len+START_CODE_LEN); */
                padding = buf[rtp_len + START_CODE_LEN];
                if (rtp_len >= 12 + padding)
                    rtp_len -= padding;
                /* av_log(NULL, AV_LOG_DEBUG,  "sps_padding = %d\n", padding); */

            }
            nal_len = rtp_len + START_CODE_LEN + 2;

            /* av_hex_dump(NULL, buf, nal_len); */

            /* init_get_bits8(&s->hevc_ctx.HEVClc->gb, hevc_nal->data + 2, hevc_nal->size); */

            /* ff_hevc_decode_nal_vps(&s->hevc_ctx); */

            rtp_h264_set_start_end_bit(s, 1, 0);
            s->nalu_type = nal_type;
            break;
        }
        case HEVC_NAL_SPS:
        {
            if (rtp_len > 256)
            {
                return AVERROR(EAGAIN);
            }

            memcpy(buf, start_code, START_CODE_LEN);
            buf[START_CODE_LEN] = nal[0];
            buf[START_CODE_LEN + 1] = nal[1];
            /* 读取SPS */
            ret = fifo_read(rs_info->fifo_fd, (char *)(buf + START_CODE_LEN + 2), rtp_len, FIFO_PEEK_YES);
            if (ret < 0)
            {
                av_log(NULL, AV_LOG_ERROR, " rtsp read NAL_SPS error = %d\n ", ret);
                goto fail;
            }
            else if (ret != rtp_len)
            {
                return AVERROR(EAGAIN);
            }

            if (padding)
            {
                /* av_hex_dump(NULL, buf, rtp_len+START_CODE_LEN); */
                padding = buf[rtp_len + START_CODE_LEN];
                if (rtp_len >= 12 + padding)
                    rtp_len -= padding;
                /* av_log(NULL, AV_LOG_DEBUG,  "sps_padding = %d\n", padding); */

            }
            nal_len = rtp_len + START_CODE_LEN + 2;

            if (!rs_info->have_sps || (rs_info->have_sps && memcmp(buf + START_CODE_LEN, rs_info->sps, rs_info->sps_len)))
            {
                int nal_size = rtp_len;
                int rbsp_size = rtp_len;
                char rbsp_buf[1024] = {0};
                GetBitContext gb = {0};

                memcpy(rs_info->sps, buf + START_CODE_LEN, rtp_len + 2);
                rs_info->sps_len = rtp_len + 2;
                rs_info->have_sps = 1;

                /* av_log(NULL, AV_LOG_DEBUG, "nal_len = %d\n", nal_len); */
                /* av_hex_dump(NULL, buf, 32); */
                /* ff_hevc_extract_rbsp(&s->hevc_ctx, buf + START_CODE_LEN, rtp_len, hevc_nal); */

                ret = nal_to_rbsp(buf + START_CODE_LEN + 1, &nal_size, (uint8_t *)rbsp_buf, &rbsp_size);
                if (ret > 0 && rbsp_size <= 256)
                {
					/*av_log(NULL, AV_LOG_DEBUG, "rbsp_size = %d\n", rbsp_size);
					av_log(NULL, AV_LOG_WARNING, "rbsp_size = %d\n", rbsp_size);*/
                    /* av_hex_dump(NULL, rbsp_buf, rbsp_size); */
                    /* init_get_bits8(&s->hevc_ctx.HEVClc->gb, rbsp_buf, rbsp_size); */
                    init_get_bits8(&gb, (uint8_t *)rbsp_buf, rbsp_size);
                    ff_hevc_decode_nal_sps(&s->hevc_ctx, &gb);
                }

            }

            rtp_h264_set_start_end_bit(s, 1, 0);
            rs_info->dealed_sps = 1;
            s->nalu_type = nal_type;
            break;
        }
        case HEVC_NAL_TRAIL_N:
        case HEVC_NAL_TRAIL_R:
        case HEVC_NAL_TSA_N:
        case HEVC_NAL_TSA_R:
        case HEVC_NAL_STSA_N:
        case HEVC_NAL_STSA_R:
        case HEVC_NAL_RADL_N:
        case HEVC_NAL_RADL_R:
        case HEVC_NAL_RASL_N:
        case HEVC_NAL_RASL_R:
        case HEVC_NAL_BLA_W_LP:
        case HEVC_NAL_BLA_W_RADL:
        case HEVC_NAL_BLA_N_LP:
        case HEVC_NAL_IDR_W_RADL:
        case HEVC_NAL_IDR_N_LP:
        case HEVC_NAL_CRA_NUT:
        {
            new_frame = 1;
            s->frame_num++;

            memcpy(buf, start_code, START_CODE_LEN);
            buf[START_CODE_LEN] = nal[0];
            buf[START_CODE_LEN + 1] = nal[1];

            /* 读取单P帧 */
            ret = fifo_read(rs_info->fifo_fd, (char *)(buf + START_CODE_LEN + 2), rtp_len, FIFO_PEEK_YES);
            if (ret < 0)
            {
                av_log(NULL, AV_LOG_ERROR, " rtsp read NAL_SLICE error = %x\n ", ret);
                goto fail;
            }
            else if (ret != rtp_len)
            {
                return AVERROR(EAGAIN);
            }

            if (padding)
            {
                padding = buf[rtp_len + START_CODE_LEN];
                if (rtp_len >= 12 + padding)
                    rtp_len -= padding;
                /* av_log(NULL, AV_LOG_DEBUG, "slice_padding = %d\n", padding); */
            }
            nal_len = rtp_len + START_CODE_LEN + 2;

            rtp_h264_set_start_end_bit(s, 1, 0);
            /* av_hex_dump(NULL, buf, 8); */
            /* av_log(NULL, AV_LOG_DEBUG, "slice_pos = %d\n", s->frame_buf_pos); */
            /*av_log(NULL, AV_LOG_WARNING, "slice_pos = %d\n", s->frame_buf_pos);*/
            s->nalu_type = nal_type;
            break;
        }
        case HEVC_NAL_PPS:
        case HEVC_NAL_SEI_PREFIX:
        case HEVC_NAL_SEI_SUFFIX:
        {
            if (rtp_len > 512)
            {
                return AVERROR(EAGAIN);
            }
            memcpy(buf, start_code, START_CODE_LEN);
            buf[START_CODE_LEN] = nal[0];
            buf[START_CODE_LEN + 1] = nal[1];
            /* 读取其他帧 */
            ret = fifo_read(rs_info->fifo_fd, (char *)(buf + START_CODE_LEN + 2), rtp_len, FIFO_PEEK_YES);
            if (ret < 0)
            {
                av_log(NULL, AV_LOG_ERROR, " rtsp read NAL_PPS error = %x\n ", ret);
                goto fail;
            }
            else if (ret != rtp_len)
            {
                return AVERROR(EAGAIN);
            }

            if (padding)
            {
                padding = buf[rtp_len + START_CODE_LEN];
                if (rtp_len >= 12 + padding)
                    rtp_len -= padding;
                /* av_log(NULL, AV_LOG_DEBUG, "pps_padding = %d\n", padding); */
            }
            nal_len = rtp_len + START_CODE_LEN + 2;
#if 0
            if (nal_type == HEVC_NAL_SEI_PREFIX)
            {
                int nal_size = rtp_len;
                int rbsp_size = rtp_len;
                char rbsp_buf[1024] = {0};
                GetBitContext gb = {0};

                /* av_log(NULL, AV_LOG_DEBUG, "HEVC_NAL_SEI_PREFIX = %d\n", rtp_len); */
                /* av_hex_dump(NULL, buf + START_CODE_LEN + 2, rtp_len); */

                ret = nal_to_rbsp(buf + START_CODE_LEN + 1, &nal_size, rbsp_buf, &rbsp_size);
                if (ret > 0 && rbsp_size < 128)
                {
                    /* av_log(NULL, AV_LOG_DEBUG, "rbsp_size = %d\n", rbsp_size); */
                    /* av_hex_dump(NULL, rbsp_buf, rbsp_size); */
                    init_get_bits8(&gb, rbsp_buf, rbsp_size);
                    decode_nal_sei_message(&s->hevc_ctx, &gb);
                    /* av_log(NULL, AV_LOG_DEBUG, "sei_type = %x sei_size = %d\n", s->hevc_ctx.sei_type, s->hevc_ctx.sei_size); */
                }
            }
#endif
            /* av_log(NULL, AV_LOG_DEBUG,  "pps_pos = %d\n", s->frame_buf_pos); */
            s->nalu_type = nal_type;
            break;
        }

        case 49:                        /*  FU (NAL分片) */
        {
            int first_fragment, last_fragment, fu_type;
            int extradata_len = 0;
            uint8_t reconstructed_nal[2];

            ret = fifo_read(rs_info->fifo_fd, (char *)&fu_header, 1, FIFO_PEEK_YES);
            if (ret < 0)
            {
                av_log(NULL, AV_LOG_ERROR, " rtsp read fu_header error = %x\n ", ret);
                goto fail;
            }
            else if (ret != 1)
            {
                return AVERROR(EAGAIN);
            }

            /*
            *    decode the FU header
            *
            *     0 1 2 3 4 5 6 7
            *    +-+-+-+-+-+-+-+-+
            *    |S|E|  FuType   |
            *    +---------------+
            *
            *       Start fragment (S): 1 bit
            *       End fragment (E): 1 bit
            *       FuType: 6 bits
            */
            first_fragment = fu_header & 0x80;
            last_fragment  = fu_header & 0x40;
            fu_type        = fu_header & 0x3f;

            rtp_len--;                  /* 跳过 fu_indicator */

            if (rtp_len > 0)
            {
                /*av_log(NULL, AV_LOG_DEBUG, " FU type %d with %d bytes\n", fu_type, rtp_len);
                av_log(NULL, AV_LOG_WARNING, " FU type %d with %d bytes\n", fu_type, rtp_len);*/

                if (first_fragment && last_fragment)
                {
                    /* av_log(NULL, AV_LOG_ERROR, "Illegal combination of S and E bit in RTP/HEVC packet\n"); */
                    return AVERROR(EAGAIN);
                }

                reconstructed_nal[0] = (nal[0] & 0x81) | (fu_type << 1);
                reconstructed_nal[1] = nal[1];

                if (first_fragment)
                {
#if 0
                    if (!rs_info->dealed_sps && rs_info->have_sps && (s->nalu_type == NAL_IDR_SLICE))
                    {
                        if (rs_info->sps_len > 0 && rs_info->sps != NULL)
                        {
                            memcpy(buf, start_code, START_CODE_LEN);
                            memcpy(buf + START_CODE_LEN, rs_info->sps, rs_info->sps_len);
                            extradata_len = rs_info->sps_len + START_CODE_LEN;
                            s->h.nal_ref_idc   = rs_info->sps[0] >> 5;/*  00 00 00 01 67 */
                            s->h.nal_unit_type = rs_info->sps[0] & 0x1f;
                            init_get_bits(&s->h.gb, rs_info->sps + 1, 8 * (rs_info->sps_len - 1));
                            ff_h264_decode_seq_parameter_set(&s->h);
                        }
                        if (rs_info->pps_len > 0 && rs_info->pps != NULL)
                        {
                            memcpy(buf + extradata_len, start_code, START_CODE_LEN);
                            memcpy(buf + extradata_len + START_CODE_LEN, rs_info->pps, rs_info->pps_len);
                            extradata_len += rs_info->pps_len + START_CODE_LEN;
                        }
                    }


#endif
                    s->nalu_type = (reconstructed_nal[0] >> 1) & 0x3f;

                    /* av_log(NULL, AV_LOG_DEBUG, "fu_header = 0x%x fu_type =0x%x nalu_type = %d\n", fu_header, fu_type, s->nalu_type); */

                    /* av_log(NULL, AV_LOG_DEBUG, "re_nal = 0x%x re_nal = 0x%x\n", reconstructed_nal[0], reconstructed_nal[1]); */

                    /* copy in the start sequence, and the reconstructed nal */

                    memcpy(buf + extradata_len, start_code, START_CODE_LEN);
                    buf[START_CODE_LEN + extradata_len] = reconstructed_nal[0];
                    buf[START_CODE_LEN + extradata_len + 1] = reconstructed_nal[1];

                    /* 读取FU-A起始包 */
                    ret = fifo_read(rs_info->fifo_fd, (char *)(buf + extradata_len + START_CODE_LEN + 2), rtp_len, FIFO_PEEK_YES);
                    if (ret < 0)
                    {
                        av_log(NULL, AV_LOG_ERROR, " rtsp read FU-A error = %d\n ", ret);
                        goto fail;
                    }
                    else if (ret != rtp_len)
                    {
                        return AVERROR(EAGAIN);
                    }

                    if (padding)
                    {
                        padding = buf[rtp_len + START_CODE_LEN];
                        if (rtp_len >= 12 + padding)
                            rtp_len -= padding;
                        /* av_log(NULL, AV_LOG_DEBUG,  "fu1_padding = %d\n", padding); */
                    }
                    nal_len = extradata_len + rtp_len + START_CODE_LEN + 2;
                    /* av_log(NULL, AV_LOG_DEBUG, "fu1_pos = %d len = %d\n", s->frame_buf_pos, nal_len); */
                    /* av_hex_dump(NULL, buf, 8); */
                }
                else
                {
                    /* 读取FU-A分片包 */
                    ret = fifo_read(rs_info->fifo_fd, (char *)buf, rtp_len, FIFO_PEEK_YES);
                    if (ret < 0)
                    {
                        av_log(NULL, AV_LOG_ERROR, " rtsp read FU-A error = %x\n ", ret);
                        goto fail;
                    }
                    else if (ret != rtp_len)
                    {
                        return AVERROR(EAGAIN);
                    }

                    if (padding)
                    {
                        /* av_hex_dump(NULL, buf, rtp_len); */
                        padding = buf[rtp_len - 1];
                        if (rtp_len >= 12 + padding)
                            rtp_len -= padding;
                        /* av_log(NULL, AV_LOG_DEBUG, "fu2_padding = %d\n", padding); */
                    }
                    nal_len = rtp_len;
                    /* av_log(NULL, AV_LOG_DEBUG, "fu2_pos = %d len = %d\n", s->frame_buf_pos, nal_len); */
                    /* av_hex_dump(NULL, buf, 8); */
                    /*av_log(NULL, AV_LOG_WARNING, "fu2_pos = %d len = %d\n", s->frame_buf_pos, nal_len);*/
                }
				rtp_hevc_set_start_end_bit(s, first_fragment != 0, last_fragment != 0);
            }
            else
            {
                char tmp_buf[8];
                ret = fifo_read(rs_info->fifo_fd, tmp_buf, rtp_len, FIFO_PEEK_YES);
                if (ret < 0)
                {
                    av_log(NULL, AV_LOG_ERROR, "rtsp read short data error = %x\n ", ret);
                    goto fail;
                }
                av_log(NULL, AV_LOG_WARNING, "Too short data for FU-A %d 0x%x\n", rtp_len, tmp_buf[0]);

                /* av_hex_dump(NULL, rtp_buf, 32); */
                return AVERROR(EAGAIN);
            }
            break;
        }
        case 48:
        default:
        {
            av_log(NULL, AV_LOG_ERROR, "Undefined type (%d)\n", nal_type);
            return AVERROR(EAGAIN);
        }
    }

    return nal_len > 0 ? nal_len : result;

fail:
    rs_info->rtp_len = -1;
    return -10;
}

int __rtsp_combine_frame(RTSPSessionInfo * rs_info)
{
    int ret, rtp_len = -1;
    FIFO_BUFF_INFO_S buff_info = {0};
    AVFormatContext * s = NULL;
    static int print_count = 0;

    if (rs_info != NULL && rs_info->fmt_ctx != NULL)
    {
        s = rs_info->fmt_ctx;
    }
    else
    {
        return -1;
    }

retry:

    if (rs_info->tcp_fd < 0)
    {
        av_log(NULL, AV_LOG_ERROR,  "tcp_fd [-1]\n");
        return AVERROR_EOF;
    }

    if (rs_info->state == RSESSION_STATE_CLOSED)
    {
        av_log(NULL, AV_LOG_ERROR,  "RSESSION_STATE_CLOSED\n");
        return AVERROR_EXIT;
    }

    /*如果上一次分析rtp包头，直接解析payload*/
    if (rs_info->rtp_len > 0)
    {
        rtp_len = rs_info->rtp_len;
    }
    else
    {
        /*获取可读取的fifo大小*/
        ret = fifo_read_buffer_get(rs_info->fifo_fd, &buff_info);
        if (ret < 0)
        {
            av_log(NULL, AV_LOG_ERROR,  "fifo_read_buffer_get error [%x]\n", ret);
            return -9;
        }
        /*大于12+4字节时，再进行解析rtp头*/
        else if (ret >= RTP_HEADER_LEN + RTP_INTERLEAVED_LEN)
        {
            rtp_len = __rtp_parse_header(rs_info, &buff_info);
            if (rtp_len < 0)
            {
                if (rtp_len != AVERROR(EAGAIN))
                {
                    av_log(NULL, AV_LOG_ERROR,  "rtp_parse_header error [%d]\n", rtp_len);
                }
                /* av_log(NULL, AV_LOG_DEBUG, "fifo_read_buffer_get = %d\n", ret); */
                return rtp_len;
            }
            rs_info->rtp_len = rtp_len;
            /* rs_info->recv_ts = av_gettime(); */
            /*av_log(NULL, AV_LOG_WARNING, "rtp_len = %d\n", rtp_len);*/
        }
        else
        {
            /*int rbuf = -1, sbuf = -1;*/
            rs_info->eagain1++;
            /*tcp_info(rs_info->tcp_fd, &rbuf, &sbuf);
            av_log(NULL, AV_LOG_DEBUG,  "[%d][%d][%d] EAGAIN 1[%d] ",rs_info->self_idx, rs_info->tcp_fd, rs_info->eagain1,ret);
            av_log(NULL, AV_LOG_DEBUG, "rbuf(%d)--sbuf(%d)\n", rbuf, sbuf);*/
            return AVERROR(EAGAIN);
        }
    }

    if (rtp_len > 0)
    {
        if (rs_info->pkt_lost_current > 0)
        {
            /* av_log(NULL, AV_LOG_WARNING, "[fd:%d]pkt_lost = %d pkt_lost_current = %d\n", rs_info->tcp_fd, rs_info->pkt_lost, rs_info->pkt_lost_current); */
        }

        /*获取可读取的fifo大小*/
        ret = fifo_read_buffer_get(rs_info->fifo_fd, &buff_info);
        if (ret > 512000)
        {
            /* av_log(NULL, AV_LOG_WARNING, "fifo=[%d] rtp=[%d] [%d]\n", ret, rtp_len, rs_info->payload_type); */
        }
        if (ret < 0)
        {
            return -10;
        }
        /*大于等于rtp_len字节时(完整rtp包)，再进行解析payload*/
        else if (ret >= rtp_len)
        {
            if (rs_info->is_rtcp)
            {
                /* av_log(NULL, AV_LOG_DEBUG, "rtcp_len = %d\n", rtp_len); */

                ret = __rtp_parse_rtcp(rs_info, &buff_info);
                if (ret > 0)
                {
                    rs_info->rtp_len = -1;
                    av_log(NULL, AV_LOG_VERBOSE,  "rtcp.size = %d\n", ret);
                }
                else if (ret == -RTCP_BYE)
                {
                    RTSPState * rt = s->priv_data;
                    rt->nb_byes++;
                    /* av_log(NULL, AV_LOG_DEBUG,  "[fd:%d]RTCP_BYE = (%d - %d)\n", rs_info->tcp_fd, rt->nb_byes, rt->nb_rtsp_streams); */
                    /*if (rt->nb_byes == rt->nb_rtsp_streams)
                        return AVERROR_EOF;*/
                }
                rs_info->is_rtcp = 0;
                goto retry;
            }
            else if (rs_info->payload_type == AV_CODEC_ID_H264)
            {
                if (s->frame_buf_pos + rs_info->rtp_len > s->frame_buf_len)
                {
                    if ((s->frame_buf_len + (32 << 10) < RTSP_MAX_FRAME_SIZE) && !rs_info->pkt_lost_current)
                    {
                        int mem_len = s->frame_buf_len + (32 << 10);
                        void * new_buffer = av_fast_realloc(s->frame_buf, (unsigned int *)&s->frame_buf_len, mem_len);

                        if (!new_buffer)
                        {
                            s->frame_buf_pos = 0;
                            return AVERROR(ENOMEM);
                        }

                        s->frame_buf = new_buffer;
                    }
                    else
                    {
                        s->frame_buf_pos = 0;
                        rs_info->pkt_lost_current = 1;
                        rs_info->recv_bytes = 0;
                        goto retry;
                    }
                }

                ret = __rtp_parse_h264(rs_info, &buff_info, s->frame_buf + s->frame_buf_pos);
                if (ret > 0)
                {
                    /* av_log(NULL, AV_LOG_DEBUG,  "[fd:%d %d]frame_buf = %p pos = %d\n", rs_info->tcp_fd, s->frame_buf_len, s->frame_buf, s->frame_buf_pos); */
                    rs_info->rtp_len = -1;
                    s->frame_buf_pos += ret;
                    if ((s->marker) && (s->nalu_type != NAL_SPS && s->nalu_type != NAL_PPS && s->nalu_type != NAL_SEI))
                    {
                        /*av_log(NULL, AV_LOG_DEBUG, "out_pkt.size = %d\n", s->frame_buf_pos);*/
                        rs_info->frame_type = RTSP_CODEC_ID_H264;
                        rs_info->frame_data = s->frame_buf;
                        rs_info->frame_size = s->frame_buf_pos;
                        rs_info->key_frame = (s->nalu_type == NAL_IDR_SLICE) ? 1 : 0;
                        rs_info->frame_width = s->h264_ctx.width;
                        rs_info->frame_height = s->h264_ctx.height;
                        s->frame_buf_pos = 0;
                        rs_info->recv_ts = av_gettime();
						rs_info->dealed_sps = 0;

                        /*if (rs_info->key_frame)
                            av_hex_dump(NULL, s->frame_buf, 128);*/

                        /*if(rs_info->frame_width>=1280)
                        {
                            av_log(NULL, AV_LOG_DEBUG, "[fd:%d key:%d]pkt_lost = %d pkt_lost_current = %d\n",rs_info->tcp_fd, rs_info->key_frame, rs_info->pkt_lost, rs_info->pkt_lost_current);
                        }*/

                        if (rs_info->pkt_lost_current > 0)
                        {
                            rs_info->pkt_lost += rs_info->pkt_lost_current;
                            /* av_log(NULL, AV_LOG_DEBUG, "[1fd:%d key:%d]pkt_lost = %d pkt_lost_current = %d\n", rs_info->tcp_fd, rs_info->key_frame, rs_info->pkt_lost, rs_info->pkt_lost_current); */
                            rs_info->pkt_lost_current = 0;
                            rs_info->vdrop_count++;
                            rs_info->recv_bytes = 0;
                        }
                        else
                        {
                            /*如果当前帧没有丢包,且是关键帧*/
                            if (rs_info->key_frame)
                            {
                                /*帧间丢包记录清零*/
                                rs_info->pkt_lost = 0;
                                return rs_info->frame_size;
                            }
                            /*非关键帧*/
                            else
                            {

                                if (rs_info->pkt_lost > 0)
                                {
                                    /* av_log(NULL, AV_LOG_DEBUG, "[2fd:%d key:%d]pkt_lost = %d pkt_lost_current = %d\n", rs_info->tcp_fd, rs_info->key_frame, rs_info->pkt_lost, rs_info->pkt_lost_current); */
                                }
                                else
                                {
                                    /*帧间没有丢包*/
                                    return rs_info->frame_size;
                                }
                            }
                        }
                    }
                    goto retry;
                }
                else
                {
                    if (ret != AVERROR(EAGAIN))
                    {
                        av_log(NULL, AV_LOG_ERROR,  "rtp_parse_h264 error [%x]\n", ret);
                    }
                    return ret;
                }
            }
            else if (rs_info->payload_type == AV_CODEC_ID_HEVC)
            {
                if (s->frame_buf_pos + rs_info->rtp_len > s->frame_buf_len)
                {
                    if ((s->frame_buf_len + (32 << 10) < RTSP_MAX_FRAME_SIZE) && !rs_info->pkt_lost_current)
                    {
                        int mem_len = s->frame_buf_len + (32 << 10);
                        void * new_buffer = av_fast_realloc(s->frame_buf, (unsigned int *)&s->frame_buf_len, mem_len);

                        if (!new_buffer)
                        {
                            s->frame_buf_pos = 0;
                            return AVERROR(ENOMEM);
                        }

                        s->frame_buf = new_buffer;
                    }
                    else
                    {
                        s->frame_buf_pos = 0;
                        rs_info->pkt_lost_current = 1;
                        rs_info->recv_bytes = 0;
                        goto retry;
                    }
                }

                ret = __rtp_parse_hevc(rs_info, &buff_info, s->frame_buf + s->frame_buf_pos);
                if (ret > 0)
                {
                    /*av_log(NULL, AV_LOG_DEBUG, "[marker: %d  type: %d  len = %d]\n", s->marker, s->nalu_type, s->frame_buf_pos);*/
                    rs_info->rtp_len = -1;
                    s->frame_buf_pos += ret;
                    if (s->marker &&
                            (/*s->nalu_type != HEVC_NAL_VPS &&*/
							 s->nalu_type != HEVC_NAL_SPS &&
                             s->nalu_type != HEVC_NAL_PPS &&
                             s->nalu_type != HEVC_NAL_SEI_PREFIX))
                    {
						//av_log(NULL, AV_LOG_DEBUG, "out_pkt.size = %d nalu_type = %d\n", s->frame_buf_pos, s->nalu_type);
                        /*av_hex_dump(NULL, s->frame_buf, 64);*/
                        rs_info->frame_type = RTSP_CODEC_ID_H265;
                        rs_info->frame_data = s->frame_buf;
                        rs_info->frame_size = s->frame_buf_pos;
                        rs_info->key_frame = (s->nalu_type >= 16 && s->nalu_type <= 23) ? 1 : 0;
                        rs_info->frame_width = s->hevc_ctx.width;
                        rs_info->frame_height = s->hevc_ctx.height;
                        s->frame_buf_pos = 0;
                        rs_info->recv_ts = av_gettime();

                        /*if (rs_info->key_frame)
                            av_hex_dump(NULL, s->frame_buf, 128);*/

                        /*if(rs_info->frame_width>=1280)
                        {
                            av_log(NULL, AV_LOG_DEBUG, "[fd:%d key:%d]pkt_lost = %d pkt_lost_current = %d\n",rs_info->tcp_fd, rs_info->key_frame, rs_info->pkt_lost, rs_info->pkt_lost_current);
                        }*/

                        if (rs_info->pkt_lost_current > 0)
                        {
                            rs_info->pkt_lost += rs_info->pkt_lost_current;
                            /* av_log(NULL, AV_LOG_DEBUG, "[1fd:%d key:%d]pkt_lost = %d pkt_lost_current = %d\n", rs_info->tcp_fd, rs_info->key_frame, rs_info->pkt_lost, rs_info->pkt_lost_current); */
                            rs_info->pkt_lost_current = 0;
                            rs_info->vdrop_count++;
                            rs_info->recv_bytes = 0;
                        }
                        else
                        {
                            /*如果当前帧没有丢包,且是关键帧*/
                            if (rs_info->key_frame)
                            {
                                /*帧间丢包记录清零*/
                                rs_info->pkt_lost = 0;
                                return rs_info->frame_size;
                            }
                            /*非关键帧*/
                            else
                            {

                                if (rs_info->pkt_lost > 0)
                                {
                                    /* av_log(NULL, AV_LOG_DEBUG, "[2fd:%d key:%d]pkt_lost = %d pkt_lost_current = %d\n", rs_info->tcp_fd, rs_info->key_frame, rs_info->pkt_lost, rs_info->pkt_lost_current); */
                                }
                                else
                                {
                                    /*帧间没有丢包*/
                                    return rs_info->frame_size;
                                }
                            }
                        }
                    }
                    goto retry;
                }
                else
                {
                    if (ret != AVERROR(EAGAIN))
                    {
                        av_log(NULL, AV_LOG_ERROR,  "rtp_parse_hevc error [%x]\n", ret);
                    }
                    return ret;
                }

            }
            else if (rs_info->payload_type == AV_CODEC_ID_PCM_ALAW)
            {
                if (rs_info->rtp_len > s->audio_buf_len)
                {
                    av_log(NULL, AV_LOG_VERBOSE, "audio.size too big = %d\n", rs_info->rtp_len);
                    rs_info->rtp_len = s->audio_buf_len;
                }

                ret = __rtp_parse_g711(rs_info, &buff_info, s->audio_buf);
                if (ret > 0)
                {
                    rs_info->rtp_len = -1;
                    /* av_hex_dump(NULL, s->audio_buf, 8); */
                    /* av_log(NULL, AV_LOG_DEBUG, "G711A out_pkt.size = %d\n", ret); */
                    rs_info->frame_type = RTSP_CODEC_ID_G711A;
                    rs_info->frame_data = s->audio_buf;
                    rs_info->frame_size = ret;
                    rs_info->key_frame = 0;
                    rs_info->frame_width = 0;
                    rs_info->frame_height = 0;
                    return rs_info->frame_size;
                }
            }
            else if (rs_info->payload_type == AV_CODEC_ID_PCM_MULAW)
            {
                if (rs_info->rtp_len > s->audio_buf_len)
                {
                    av_log(NULL, AV_LOG_VERBOSE, "audio.size too big = %d\n", rs_info->rtp_len);
                    rs_info->rtp_len = s->audio_buf_len;
                }

                ret = __rtp_parse_g711(rs_info, &buff_info, s->audio_buf);
                if (ret > 0)
                {
                    rs_info->rtp_len = -1;
                    /* av_hex_dump(NULL, s->audio_buf, 8); */
                    /* av_log(NULL, AV_LOG_DEBUG, "G711U out_pkt.size = %d\n", ret); */
                    rs_info->frame_type = RTSP_CODEC_ID_G711U;
                    rs_info->frame_data = s->audio_buf;
                    rs_info->frame_size = ret;
                    rs_info->key_frame = 0;
                    rs_info->frame_width = 0;
                    rs_info->frame_height = 0;
                    return rs_info->frame_size;
                }
            }
            else if (rs_info->payload_type == AV_CODEC_ID_TEXT_XML)
            {
                if (rs_info->rtp_len > s->audio_buf_len)
                {
                    av_log(NULL, AV_LOG_VERBOSE, "meta.size too big = %d\n", rs_info->rtp_len);
                    rs_info->rtp_len = s->audio_buf_len;
                }

                memset(s->audio_buf, 0x00, s->audio_buf_len);

                ret = __rtp_parse_metadata(rs_info, &buff_info, s->audio_buf);
                if (ret > 0)
                {
                    rs_info->rtp_len = -1;
                    /* av_hex_dump(NULL, s->audio_buf, 8); */
                    /* av_log(NULL, AV_LOG_DEBUG, "%s\n", s->audio_buf); */
                    rs_info->frame_type = RTSP_CODEC_ID_META;
                    rs_info->frame_data = s->audio_buf;
                    rs_info->frame_size = ret;
                    rs_info->key_frame = 0;
                    rs_info->frame_width = 0;
                    rs_info->frame_height = 0;
                    return rs_info->frame_size;
                }
            }
            else if (rs_info->payload_type == AV_CODEC_ID_APP_XML)
            {
                if ((rs_info->rtp_len > s->audio_buf_len) || ((s->audio_buf_pos + rs_info->rtp_len) > s->audio_buf_len))
                {
                    av_log(NULL, AV_LOG_VERBOSE, "meta.size too big = %d\n", rs_info->rtp_len);
                    rs_info->rtp_len = -1;
					return AVERROR(EAGAIN);
                }

                ret = __rtp_parse_metadata(rs_info, &buff_info, s->audio_buf + s->audio_buf_pos);
                if (ret > 0)
                {
                    /* av_hex_dump(NULL, s->audio_buf, 8); */                    
					rs_info->rtp_len = -1;
					s->audio_buf_pos += ret;
					if (s->marker)
					{
						av_log(NULL, AV_LOG_VERBOSE, "\n%s\n", s->audio_buf);
						rs_info->frame_type = RTSP_CODEC_ID_META_SPECO;
						rs_info->frame_data = s->audio_buf;
						rs_info->frame_size = s->audio_buf_pos;
						rs_info->key_frame = 0;
						rs_info->frame_width = 0;
						rs_info->frame_height = 0;
						s->audio_buf_pos = 0;
						return rs_info->frame_size;
					}
                }
            }
            else
            {
                /*不明数据丢掉，用audio_buf接收*/
                if (rs_info->rtp_len > s->audio_buf_len)
                {
                    av_log(NULL, AV_LOG_VERBOSE, "unknown.size too big = %d\n", rs_info->rtp_len);
                    rs_info->rtp_len = s->audio_buf_len;
                }

                ret = __rtp_parse_unknown(rs_info, &buff_info, s->audio_buf);
                if (ret > 0)
                {
                    rs_info->rtp_len = -1;
                    /* av_hex_dump(NULL, s->audio_buf, ret); */
                    av_log(NULL, AV_LOG_WARNING,  "unknown %s\n", s->audio_buf);
#if 0
                    if (print_count++ % 10 == 0)
                    {
                        av_log(NULL, AV_LOG_INFO, "[fd:%d]unknown = %d size = %d\n", rs_info->tcp_fd, rs_info->payload_type, ret);
                    }
#endif
                }

            }
        }
        else
        {
            rs_info->eagain2++;
            /* av_log(NULL, AV_LOG_ERROR,  "[%d][%d] EAGAIN 2[%d %d]\n",rs_info->self_idx, rs_info->eagain2, ret,rtp_len); */
            return AVERROR(EAGAIN);
        }
    }

    return 0;
}

void  __rtsp_deal_err(RTSPSessionInfo * rs_info, RTSP_CLINFO * rtsp_cli, int err_code)
{
    char error_buf[1024] = {0};
    SYSTIME rec_tm, now_tm;
    int64_t now = 0;

    if (rs_info == NULL)
    {
        return;
    }

    now = av_gettime();
    time_sec2time(now / 1000000, &now_tm);

    rs_info->last_err = err_code;
    if (err_code == AVERROR_EOF)
    {
        av_log(NULL, AV_LOG_ERROR, TIME_STR, SYSTIME4TM_FMT(now_tm));
        av_log(NULL, AV_LOG_ERROR, "[fd:%d]rtsp remote closed\n", rs_info->tcp_fd);
        if (rs_info->cb_func)
        {
            rs_info->cb_func(RTSP_EVT_EOF, rs_info->self_idx, rs_info->priv_data, &rs_info->cb_info);
        }

        __rtsp_cli_err(rtsp_cli, RTSP_EVT_EOF);
        rs_info->state = RSESSION_STATE_CLOSED;
        /*rss_unlock_session(rs_info->self_idx);
        rs_info->is_locked = 0;*/
    }
    else if (err_code == AVERROR(ETIMEDOUT))
    {
        int rbuf = -1, sbuf = -1;
        /*tcp_info(rs_info->tcp_fd, &rbuf, &sbuf);*/
        av_log(NULL, AV_LOG_WARNING, TIME_STR, SYSTIME4TM_FMT(now_tm));
        av_log(NULL, AV_LOG_WARNING, "[fd:%d]rtsp timeout closed  ", rs_info->tcp_fd);
        av_log(NULL, AV_LOG_WARNING, "rbuf(%d)--sbuf(%d)\n", rbuf, sbuf);
        if (rs_info->cb_func)
        {
            rs_info->cb_func(RTSP_EVT_EOF, rs_info->self_idx, rs_info->priv_data, &rs_info->cb_info);
        }

        __rtsp_cli_err(rtsp_cli, RTSP_EVT_READ_FRAME_TIMEOUT);
        __rtsp_cli_netstat(rtsp_cli, rbuf, sbuf);
        rs_info->state = RSESSION_STATE_CLOSED;
        /*rss_unlock_session(rs_info->self_idx);
        rs_info->is_locked = 0;*/
    }
    else if (err_code == AVERROR_EXIT)
    {
        av_log(NULL, AV_LOG_INFO, TIME_STR, SYSTIME4TM_FMT(now_tm));
        av_log(NULL, AV_LOG_INFO, "[fd:%d]rtsp normal closed\n", rs_info->tcp_fd);
        if (rs_info->cb_func)
        {
            rs_info->cb_func(RTSP_EVT_CLOSED, rs_info->self_idx, rs_info->priv_data, &rs_info->cb_info);
        }

        __rtsp_cli_err(rtsp_cli, RTSP_EVT_CLOSED);
        rs_info->state = RSESSION_STATE_CLOSED;
        /*rss_unlock_session(rs_info->self_idx);
        rs_info->is_locked = 0;*/
    }
    else if (err_code == AVERROR(ENOMEM))
    {
        av_log(NULL, AV_LOG_ERROR, TIME_STR, SYSTIME4TM_FMT(now_tm));
        av_log(NULL, AV_LOG_ERROR, "[fd:%d]rtsp ENOMEM closed\n", rs_info->tcp_fd);
        if (rs_info->cb_func)
        {
            rs_info->cb_func(RTSP_EVT_EOF, rs_info->self_idx, rs_info->priv_data, &rs_info->cb_info);
        }

        __rtsp_cli_err(rtsp_cli, 6);
        rs_info->state = RSESSION_STATE_CLOSED;
        /*rss_unlock_session(rs_info->self_idx);
        rs_info->is_locked = 0;*/
    }
    else
    {
        av_strerror(err_code, error_buf, sizeof(error_buf));
        av_log(NULL, AV_LOG_ERROR, "[fd:%d] Read frame unknown err:%s(0x%x|%d)\n", rs_info->tcp_fd, error_buf, err_code, err_code);
        if (rs_info->cb_func)
        {
            /* __rtsp_callback(RTSP_EVT_READ_FRAME_ERROR, rs_info->self_idx, rs_info->priv_data, &rs_info->cb_info); */
        }
        __rtsp_cli_err(rtsp_cli, RTSP_EVT_READ_FRAME_ERROR);
    }
}

/*定时发送RTSP心跳包和RTCP包*/
void  __rtsp_sendcmd_worker(void * arg)
{
    int ret, sidx, i;
    RTSPSessionInfo * rs_info;
    /*prctl(PR_SET_NAME, "rtsp_sendcmd");*/

    while (1)
    {
        for (sidx = 0; sidx < rss_get_max_num(); sidx++)
        {
            rs_info = rss_get_session(sidx);

            if (rs_info->fmt_ctx != NULL && !rss_trylock_session_cmd(sidx))
            {
                rs_info->send_interval = av_gettime() - rs_info->send_start;

                /*一秒钟统计平均值刷新一次*/
                if (rs_info->send_interval > TEN_SECONDS / 2 && rs_info->tcp_fd > 0 && rs_info->state != RSESSION_STATE_CLOSED)
                {
                    RTSPState * rt = rs_info->fmt_ctx->priv_data;
                    /* av_log(NULL, AV_LOG_DEBUG, "[fd:%d] send = %s\n", rs_info->tcp_fd, rt->control_uri); */
                    /*ff_rtsp_send_cmd_async(rs_info->fmt_ctx, "OPTIONS", rt->control_uri, NULL);*/
                    rs_info->send_start = av_gettime();
                    if (rs_info->cur_transport_priv)
                    {
                        RTSPStream * rtsp_st = NULL;

                        for (i = 0; i < rt->nb_rtsp_streams; i++)
                        {
                            rtsp_st = rt->rtsp_streams[i];
                            /*ff_rtp_check_and_send_back_rr_1(rs_info->cur_transport_priv, rt->rtsp_hd, rtsp_st->interleaved_max, 1500);*/
                        }
                    }
                    else
                    {
                        /* av_log(NULL, AV_LOG_DEBUG, "[fd:%d] cur_transport_priv is NULL\n", rs_info->tcp_fd); */
                    }
                }

                rss_unlock_session_cmd(sidx);
            }

        }
        av_usleep(1000000);
    }
}

void __rtsp_process_worker(void * arg)
{
    int i, ret, session_fd, delta_ret = 0, audio_pkt_cnt = 0;
    int rss_idx, rss_max_idx, rss_min_idx, rss_active_idx, rss_max_num;
    int rss_max_idx_static;
    uint64_t time_usec, time_sec;
    /* int last_1, last_2, last_3; */
    /* struct timeval cur_time = {0}; */
    int count = 0, fifo_size, dispatched;
    int64_t start = 0, end = 0;
    unsigned long long selfid_64 = 0;
    RTSPSessionInfo * rs_info;
    FIFO_BUFF_INFO_S buff_info;
    RTSP_CLINFO * rtsp_cli = NULL;
    AVFormatContext * fmt_ctx = NULL;
    char tname[16] = {0};
    char error_buf[1024] = {0};
    SYSTIME rec_tm, now_tm;
    int64_t now = 0, interval = 0;
    int idx = (int)arg;
    int frame_count_out = 0;
    unsigned int readable = 0;

    snprintf(tname, sizeof(tname) , "rtsp_worker#%d", idx);
    /*prctl(PR_SET_NAME, tname);*/

    rss_max_num = rss_get_max_num();

    /*max=48/4x1-1=11*/
    /*min=48/4x(1-1)=0*/
    rss_max_idx = rss_max_num / WORKER_NUM * idx - 1;
    rss_min_idx = rss_max_num / WORKER_NUM * (idx - 1);

    av_log(NULL, AV_LOG_ERROR, "[%s:%d] min = %d max = %d\n", tname, idx, rss_min_idx, rss_max_idx);

    while (worker_running)
    {
        rss_active_idx = rss_get_active_num();

        /*当前管理线程没有活动的进程，则休眠等待*/
        if (!session_ready[rss_min_idx / DATA_MASK_BITS])
        {
            av_usleep(100000);
            continue;
        }

        /*session最大值为活动数或当前worker管理最大值较小的一个*/
        /* rss_max_idx = rss_max_idx_static < rss_active_idx - 1 ? rss_max_idx : rss_active_idx - 1; */

        /* av_log(NULL, AV_LOG_ERROR, "[%s:%d] min = %d max = %d\n", tname, rss_active_idx - 1, rss_min_idx, rss_max_idx); */

        for (rss_idx = rss_min_idx; rss_idx <= rss_max_idx; rss_idx++)
        {
            if ((!session_ready[rss_idx / DATA_MASK_BITS]) & (1 << (rss_idx % DATA_MASK_BITS)))
            {
                continue;
            }

            if (rss_idx >= rss_max_num)
            {
                continue;
            }

            rs_info = rss_get_session(rss_idx);

            if (rs_info == NULL || rs_info->fmt_ctx == NULL)
            {
                /* av_log(NULL, AV_LOG_ERROR,  "Empty context session=0x%x\n", (unsigned int)rs_info); */
                continue;
            }

            if (rs_info->state == RSESSION_STATE_CLOSED)
            {
                /* av_log(NULL, AV_LOG_ERROR, "[fd:%d]session#%d closed %d\n", rs_info->tcp_fd, rss_idx, rs_info->is_locked); */
                if (rs_info->is_epoll_on)
                {
                    ret = epoll_unregister(rs_info);
                    if (ret < 0)
                    {
                        av_log(NULL, AV_LOG_ERROR, "epoll_unregister %d\n", ret);
                    }
                    rs_info->is_epoll_on = 0;
                }
                if (rs_info->is_locked)
                {
                    if (!rss_unlock_session(rss_idx))
                    {
                        data_ready[rss_idx / DATA_MASK_BITS] &= ~(1 << (rss_idx % DATA_MASK_BITS));
                        session_ready[rss_idx / DATA_MASK_BITS] &= ~(1 << (rss_idx % DATA_MASK_BITS));
                        rs_info->is_locked = 0;
                    }
                    else
                    {
                        av_log(NULL, AV_LOG_ERROR, "[fd:%d]Err unlocking session#%d\n", rs_info->tcp_fd, rss_idx);
                    }

                }
                continue;
            }
            else if (!rs_info->is_locked)
            {
                if (rss_trylock_session(rss_idx))
                {
                    av_log(NULL, AV_LOG_ERROR, "[fd:%d]trylock session#%d err!\n", rs_info->tcp_fd, rss_idx);
                    continue;
                }
                else
                {
                    av_log(NULL, AV_LOG_WARNING, "[fd:%d]trylock session#%d success!\n", rs_info->tcp_fd, rss_idx);
                    rs_info->is_locked = 1;
                }
            }

            /*添加到管理队里里面后，再接收数据，防止处理不及时，fifo溢出*/
            if (!rs_info->is_epoll_on)
            {
                ret = epoll_register(rs_info);
                if (ret < 0)
                {
                    av_log(NULL, AV_LOG_WARNING, "epoll_register error");
                }
                rs_info->is_epoll_on = 1;
                /* av_log(NULL, AV_LOG_WARNING, "epoll_register ok\n", rs_info->tcp_fd); */
            }

            rtsp_cli = rss_get_cli(rs_info->cli_id);

            if (rtsp_cli)
            {
                rtsp_cli->recv_bytes = rs_info->recv_bytes;
            }
            /* rs_info->start = av_gettime_relative(); */

            /* av_log(NULL, AV_LOG_WARNING, "[fd:%d][idx:%d] data_ready=[%x]\n", rs_info->tcp_fd, rs_info->self_idx / DATA_MASK_BITS, data_ready[rs_info->self_idx / DATA_MASK_BITS]); */

            /*当所有通道没有数据时才进行休眠操作*/
            if (data_ready[rss_idx / DATA_MASK_BITS])
            {
                /* av_log(NULL, AV_LOG_WARNING, "[pfd:%d][idx:%d] data_ready=[%x]\n", rs_info->tcp_fd, rs_info->self_idx / DATA_MASK_BITS, data_ready[rs_info->self_idx / DATA_MASK_BITS]); */

                /*av_log(NULL, AV_LOG_WARNING, "[pfd:%d][idx:%d] data_ready=[%x]\n", rs_info->tcp_fd, rs_info->self_idx / DATA_MASK_BITS, data_ready[rs_info->self_idx / DATA_MASK_BITS]);*/
#if 0
                if (count % 1000 == 0)
                {
                    fifo_size = fifo_read_buffer_get(rs_info->fifo_fd, &buff_info);
                    /* if (fifo_size > 0) */
                    {
                        /* av_log(NULL, AV_LOG_DEBUG, "[%s][fd:%d] fifo=[%d]\n",tname, rs_info->tcp_fd, fifo_size); */
                        av_log(NULL, AV_LOG_DEBUG, "[%s][id:%d][idx:%d] [ac:%d] [min:%d] [max:%d] data_ready=[%x]\n", tname, rss_idx,
                                 rss_idx / DATA_MASK_BITS, rss_active_idx, rss_min_idx, rss_max_idx, data_ready[rss_idx / DATA_MASK_BITS]);
                    }
                }
#endif
                readable = data_ready[rss_idx / DATA_MASK_BITS] & 1 << (rss_idx % DATA_MASK_BITS);
                rs_info->readable = readable > 0 ? 1 : 0;

                /*有数据可读*/
                if (readable)
                {
                    fifo_size = fifo_read_buffer_get(rs_info->fifo_fd, &buff_info);
                    rs_info->fifo_size = fifo_size;
                    if (fifo_size > RTSP_FIFO_LOWWATER)
                    {
                        frame_count_out = 0;

                        do
                        {
                            start = av_gettime_relative();
                            ret = __rtsp_combine_frame(rs_info);
                            interval = av_gettime_relative() - start;
                            rs_info->max_com_frame = (interval > rs_info->max_com_frame) ? interval : rs_info->max_com_frame;

                            if (interval > FIFTY_MSECS)
                            {
                                av_log(NULL, AV_LOG_ERROR, "[fd:%d] combine_frame slow %lld ms\n", rs_info->tcp_fd, interval / 1000);
                            }

                            /*ret>0为完整一帧长度,如果fifo里面数据大于高水位，继续读下一帧数据*/
                            if (ret > 0)
                            {
                                if ((fifo_size - ret) > 512000)
                                {
                                    av_log(NULL, AV_LOG_VERBOSE, "[fd:%d][%d] fifo=[%d]\n", rs_info->tcp_fd, rs_info->self_idx, ret);
                                }

                                rs_info->timeout_times = 0;
                                rs_info->frame_count ++;
                                frame_count_out ++;
                                if (frame_count_out > 512)
                                {
                                    /* 连续处理512帧后就退出循环 */
                                    break;
                                }

                                /*Check packet info*/
                                if (rs_info->frame_data == NULL || rs_info->frame_size == 0)
                                {
                                    av_log(NULL, AV_LOG_ERROR,  "Invalid packet data=0x%08x size=%d\n", (unsigned int)rs_info->frame_data, rs_info->frame_size);
                                    /* continue; */
                                    break;
                                }

                                rs_info->cb_info.data = (char *)rs_info->frame_data;
                                rs_info->cb_info.size = rs_info->frame_size;

                                rs_info->cb_info.frame_type = rs_info->frame_type;
                                if (rs_info->cb_info.frame_type == RTSP_CODEC_NONE)
                                {
                                    /*Since no codec matched, just ignore it*/
                                    av_log(NULL, AV_LOG_ERROR,  "Since no codec matched, just ignore it\n");
                                    break;
                                }

                                if (rs_info->cb_info.frame_type == RTSP_CODEC_ID_H264 || rs_info->cb_info.frame_type == RTSP_CODEC_ID_H265)
                                {
                                    rs_info->vframe_count++;

                                    rs_info->recv_gap = rs_info->recv_ts - rs_info->last_recv_ts;

                                    /*int32_t d = rs_info->recv_ts - rs_info->last_recv_ts;

                                    d = FFABS(d);

                                    rs_info->jitter += d - (int32_t)((rs_info->jitter + 8) >> 4);

                                    av_log(NULL, AV_LOG_DEBUG, "[fd:%d] jitter = %d\n", rs_info->tcp_fd, rs_info->jitter);*/

                                    /*统计1秒内的接收时间和接收次数,以计算平均间隔*/
                                    i = rs_info->calc_index;
                                    rs_info->one_sec_rev_total[i] += rs_info->recv_gap;
                                    rs_info->one_sec_rev_count[i] ++;

                                    rs_info->last_recv_ts = rs_info->recv_ts;

                                    if (rs_info->ts_base)
                                    {
                                        __rtsp_calc_timestamp(rs_info);
                                    }
                                    else
                                    {
                                        rs_info->ts_base = av_gettime();
                                        rs_info->systm_last = rs_info->ts_base;
                                    }

                                    /*补偿网络校时1秒误差，录像时间减小1秒*/
                                    time_sec = (rs_info->ts_base + rs_info->ts) / 1000000;
                                    time_usec = rs_info->ts_base + rs_info->ts - time_sec * 1000000;
                                    rs_info->cb_info.time_sec = time_sec - 1;
                                    rs_info->cb_info.time_msec = time_usec / 1000;

                                    rs_info->ts_base_last = rs_info->ts_base;

                                    /*回调数据信息*/
                                    rs_info->cb_info.key_frame = rs_info->key_frame;
                                    rs_info->cb_info.fps = rs_info->fps;
                                    rs_info->cb_info.width = rs_info->frame_width;
                                    rs_info->cb_info.height = rs_info->frame_height;
                                    rs_info->cb_info.jitter = rs_info->average_ts_ms_cnt;

                                    if (rs_info->key_frame)
                                    {
                                        rs_info->first_keyframe = 1;
                                    }

                                    __rtsp_cli_stat(rs_info, rtsp_cli);

                                    __rtsp_print_frameinfo(rs_info);

                                    if (rs_info->first_keyframe && rs_info->cb_func)
                                    {
                                        rs_info->cb_func(RTSP_EVT_NEW_FRAME, rss_idx, rs_info->priv_data, &rs_info->cb_info);                                        
                                        rs_info->cb_time = av_gettime();
                                        rs_info->eagain1 = 0;
                                        rs_info->eagain2 = 0;
                                        rs_info->recv_bytes = 0;
                                    }
                                }
                                else if (rs_info->cb_info.frame_type == RTSP_CODEC_ID_G711A)
                                {
                                    /* av_log(NULL, AV_LOG_DEBUG, "[fd:%d]G711A size = %d\n",rs_info->tcp_fd, rs_info->cb_info.size); */
                                    if (rs_info->frame_size > rs_info->max_aframe)
                                    {
                                        rs_info->max_aframe = rs_info->frame_size;
                                    }
                                    rs_info->aframe_count++;
                                    rs_info->cb_func(RTSP_EVT_NEW_FRAME, rss_idx, rs_info->priv_data, &rs_info->cb_info);                                    
                                }
                                else if (rs_info->cb_info.frame_type == RTSP_CODEC_ID_G711U)
                                {
                                    if (rs_info->frame_size > rs_info->max_aframe)
                                    {
                                        rs_info->max_aframe = rs_info->frame_size;
                                    }
                                    /* av_log(NULL, AV_LOG_DEBUG, "[fd:%d]G711U size = %d\n",rs_info->tcp_fd, rs_info->cb_info.size); */
                                    rs_info->aframe_count++;
                                    rs_info->cb_func(RTSP_EVT_NEW_FRAME, rss_idx, rs_info->priv_data, &rs_info->cb_info);
                                }
                                else if ((rs_info->cb_info.frame_type == RTSP_CODEC_ID_META) || 
                                        (rs_info->cb_info.frame_type == RTSP_CODEC_ID_META_SPECO))
                                {
                                    /*if (rs_info->frame_size > rs_info->max_aframe)
                                    {
                                        rs_info->max_aframe = rs_info->frame_size;
                                    }*/
                                    av_log(NULL, AV_LOG_VERBOSE, "[fd:%d]meta size = %d\n",rs_info->tcp_fd, rs_info->cb_info.size);
                                    rs_info->meta_count++;
                                    rs_info->cb_func(RTSP_EVT_NEW_FRAME, rss_idx, rs_info->priv_data, &rs_info->cb_info);
                                }
                            }
                            else if (ret == AVERROR(EAGAIN) || ret == 0)
                            {
                                if (rs_info->eagain2 > 1024 && (av_gettime() - rs_info->cb_time > TEN_SECONDS))
                                {
                                    __rtsp_deal_err(rs_info, rtsp_cli, AVERROR(ETIMEDOUT));
                                    break;
                                }
                                if (rs_info->tcp_fd < 0)
                                {
                                    __rtsp_deal_err(rs_info, rtsp_cli, AVERROR_EOF);
                                    break;
                                }
                                continue;
                            }
                            /*Read frame error*/
                            else
                            {
                                av_log(NULL, AV_LOG_ERROR, "[fd:%d] deal_err %d\n", rs_info->tcp_fd, ret);
                                __rtsp_deal_err(rs_info, rtsp_cli, ret);
                                break;
                            }
                        }
                        while (ret > 0 && ((fifo_size - ret) > RTSP_FIFO_HIGHWATER));

                        /* 记录最大值 */
                        rs_info->frame_count_out = (frame_count_out > rs_info->frame_count_out) ? frame_count_out : rs_info->frame_count_out;
                    }
                    else
                    {
                        /*av_log(NULL, AV_LOG_DEBUG, "[fd:%d] fifo clear ready!\n", rs_info->tcp_fd);*/
                        /*fifo小于低水位时,数据准备位清零*/
                        data_ready[rss_idx / DATA_MASK_BITS] &= ~(1 << (rss_idx % DATA_MASK_BITS));
                    }
                }
                else
                {
                    /*当短时间没有数据可读上层又关闭了链接，关闭原因需要单独记录一下*/
                    if (rs_info->state == RSESSION_STATE_CLOSED)
                    {
                        __rtsp_deal_err(rs_info, rtsp_cli, AVERROR_EXIT);
                    }
                }

                // interval = av_gettime_relative() - start;
                // rs_info->cb_time_last = (interval > rs_info->cb_time_last) ? interval : rs_info->cb_time_last;
            }
            else
            {
                /*读视频超时处理*/
                if (av_gettime() - rs_info->cb_time > HALF_MINUTES)
                {
                    __rtsp_deal_err(rs_info, rtsp_cli, AVERROR(ETIMEDOUT));
                }
                /*异常断线处理*/
                if (rs_info->tcp_fd < 0)
                {
                    __rtsp_deal_err(rs_info, rtsp_cli, AVERROR_EOF);
                }

                /*记录休眠次数*/
                if (worker_sleep[rss_idx / DATA_MASK_BITS]++ == ULONG_MAX)
                {
                    worker_sleep[rss_idx / DATA_MASK_BITS] = 0;
                }
#if 0
                if (count++ % 1000 == 0)
                {
                    fifo_size = fifo_read_buffer_get(rs_info->fifo_fd, &buff_info);
                    /* if (fifo_size > 0) */
                    {
                        av_log(NULL, AV_LOG_WARNING, "[%s][fd:%d] fifo=[%d] usleep=%d\n", tname, rs_info->tcp_fd, fifo_size, count);
                        av_log(NULL, AV_LOG_WARNING, "[%s][id:%d][idx:%d] [ac:%d] [min:%d] [max:%d] data_ready=[%x]\n", tname, rs_info->self_idx,
                                 rs_info->self_idx / DATA_MASK_BITS, rss_active_idx, rss_min_idx, rss_max_idx, data_ready[rs_info->self_idx / DATA_MASK_BITS]);
                    }
                }
#endif
                av_usleep(1000);
            }
        }
    }
}
