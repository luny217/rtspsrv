#ifndef __NVRRTSP_SESSION_H__
#define __NVRRTSP_SESSION_H__

//#include <sys/time.h>
#include "libavformat/avformat.h"
#include "rtsp.h"
#define RSESSION_STATE_IDLE             (0)
#define RSESSION_STATE_DISPATCHED       (1)
#define RSESSION_STATE_CLOSED           (2)
#define RSESSION_STATE_CLOSING          (3)
#define RSESSION_STATE_INTERUPT         (4)


#define CALC_PERIOD  10
#define CALC_JITTER_PERIOD  60

/*
 * This structure save nessary info of a rtsp session
 * Accesser thead:
 *      woker       -- set tv_last_frame/last_err
 *      dispatcher  -- select/poll socket when state == RSESSION_STATE_DISPATCHED
 */
typedef struct _RTSPSessionInfo
{
    int                 self_idx;       // Argument for rtsp woker routine
    int                 pid;
    int                 cli_id;
    int                 tcp_fd;
    int                 length;
    int                 fifo_fd;
    int                 is_epoll_on;
    int                 is_locked;
    int                 flag;
    int                 id;
    int                 rtp_len;
    uint8_t             rtp_buf[12];
    uint8_t             rtcp_buf[128];
    int                 padding;
    int                 payload_type;
    int                 is_rtcp;
    int                 eagain1;
    int                 eagain2;
    int64_t             cb_time;
    int64_t             cb_time_last;
    int                 pkt_lost;
    int                 pkt_lost_current;
    void                * cur_transport_priv;

    uint8_t             *frame_data;
    int                 frame_size;
    int                 frame_type;
    int                 stream_index;
    int                 key_frame;

    int                 nalu_type;
    int                 frame_width;
    int                 frame_height;
    int                 dealed_sps;
    int                 have_sps;
    int                 sps_len;
    uint8_t             sps[1024];
    int                 pps_len;
    uint8_t             pps[1024];

    pthread_mutex_t     lock;           //the lock to protect the session
    pthread_mutex_t     cmd_lock;

    char                state;          //RSESSION_STATE_xxxx

    int                 in_use;         //If this slot in use

    char                name[16];
    char                ipaddr[32];
    int                 first_keyframe;

    int                 timeout_times;

    int                 ts_method;
    int                 ts_error_count;

    int                 fps;
    int                 last_fps;

    int64_t             ts_base;        //
    int64_t             ts;
    int64_t             last_ts;
    //int64_t           ts_audio;


    int64_t             ts_base_last;        //
    int64_t             ts_video_last;
    int64_t             ts_audio_last;
    int64_t             systm_last;
    
    int64_t             recv_ts;
    int64_t             last_recv_ts;
    int                 recv_gap;
    int                 last_recv_gap;
    int                 average_recv_gap;
    uint64_t            total_recv_gap;
    uint64_t            total_recv_count;
    uint64_t            jitter;

    int                 ts_gap;
    int                 average_ts_gap;
    uint64_t            total_ts_gap;
    uint64_t            total_ts_count;

    int one_sec_rev_total[CALC_PERIOD];
    int one_sec_rev_count[CALC_PERIOD];
    int one_sec_rev_average[CALC_PERIOD];

    int one_sec_ts_total[CALC_PERIOD];
    int one_sec_ts_count[CALC_PERIOD];
    int one_sec_ts_average[CALC_PERIOD];

    int calc_index;

    
    int one_sec_ts_40ms_cnt[CALC_JITTER_PERIOD];
    int one_sec_ts_20ms_cnt[CALC_JITTER_PERIOD];
    int one_sec_ts_10ms_cnt[CALC_JITTER_PERIOD];
    int one_sec_ts_cnt[CALC_JITTER_PERIOD];
    int average_ts_ms_cnt;

    int calc_jitter_idx;

    int64_t             start;
    int64_t             end;
    int64_t             open_time;
    int64_t             close_time;

    int64_t             send_start;
    int64_t             send_interval;

    AVFormatContext   *  fmt_ctx;       //the rtsp input context of ffmpeg

    int                 last_err;       //save the last error returned by ffmpeg

    int64_t                 frame_count;    //how many frame have we read from this context
    int64_t                 iframe_count;
    int64_t                 vframe_count;
    int64_t                 aframe_count;
    int64_t                 meta_count;
    int64_t                 vdrop_count;
    int64_t                 recv_bytes; //最近网络接收数据之和,回调帧数据后清零
    int error_code;
    int min_fps;
    int ts_new_cnt;
    int ts_40ms_cnt;
    int ts_20ms_cnt;
    int ts_10ms_cnt;
    int ts_exceed_cnt;
    int max_vframe;
    int max_aframe;
    /* 新增调试消息 */
    int frame_count_out;    /* 解析rtp循环计数器 */
    unsigned int readable;           /* 可读写标记 */
    int fifo_size;          /* FiFo水位 */
    int so_big_count;       /* FiFo不足计数器 */

    int64_t max_com_frame;
    int64_t max_video_cb;
    int64_t max_audio_cb;
    int64_t max_meta_cb;

    FUNC_RTSP_CALLBACK      cb_func;
    void        *           priv_data;     //private data passed in by caller through
    //rtsp_open interface
    RTSPCallbackInfo    cb_info;  // callback handler
} RTSPSessionInfo;
typedef struct rtsp_clinfo
{
    int used;
    int fd;
    char ipadrr[16];
    char rtsp[128];
    int64_t open_tm;
    int64_t close_tm;
    int error_code;
    int fps;
    int width;
    int height;
    int enc_type;
    int64_t frame_count;    //how many frame have we read from this context
    int64_t iframe_count;
    int64_t vframe_count;
    int64_t aframe_count;
    int64_t vdrop_count;
    int64_t recv_bytes; //最近网络接收数据之和,回调帧数据后清零
    int max_vframe;//最大视频帧
    int max_aframe;//最大音频帧
    int rcvq_buf;//netstat revq大小
    int sndq_buf;//netstat sendq大小
    int min_fps;
    int ts_new_cnt;
    int ts_40ms_cnt;
    int ts_20ms_cnt;
    int ts_10ms_cnt;
    int ts_exceed_cnt;
} RTSP_CLINFO;

int rss_init(int nr_session);
int rss_deinit(void);
int rss_get_active_num(void);
int rss_get_max_num(void);
RTSPSessionInfo * rss_get_session(int fd);
int rss_take_unused_fd(void);
int rss_lock_session(int fd);
int rss_unlock_session(int fd);
int rss_untake_fd(int fd);
int rss_clear_cli();
int rss_take_unused_cli(void);
RTSP_CLINFO * rss_get_cli(int fd);
int rss_stat_device(char * line);
int rss_stat_brief();
int rss_stat_info(char * line);
int rss_lock_session_cmd(int fd);
int rss_unlock_session_cmd(int fd);
int rss_trylock_session_cmd(int fd);
int rss_trylock_session(int fd);

void rtsp_prn_sleep(void);

#endif //__NVRRTSP_SESSION_H__
