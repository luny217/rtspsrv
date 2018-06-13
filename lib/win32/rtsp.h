#ifndef __NVRRTSP_RTSP_H__
#define __NVRRTSP_RTSP_H__

//#include <sys/time.h>

typedef enum _RTSP_EVENT
{
    //New h264 frame
    RTSP_EVT_NEW_FRAME,

    //Video resolution changed
    RTSP_EVT_RESOLUTION_CHANGED,

    //Remote server closed/shutdown this connection
    RTSP_EVT_EOF,

    //Read rtsp frame timed out
    RTSP_EVT_READ_FRAME_TIMEOUT,

    //Other unknown read frame error
    RTSP_EVT_READ_FRAME_ERROR,

    //close normal
    RTSP_EVT_CLOSED,

    RTSP_EVT_MAX
}E_RTSP_EVT;
 
/*
 * Refered from inc/base/mshead.h
 */
#define RTSP_CODEC_NONE       -2
#define RTSP_CODEC_UNKNOWN    -1
#define RTSP_CODEC_ID_G722    0
#define RTSP_CODEC_ID_G728    2
#define RTSP_CODEC_ID_G729    4
#define RTSP_CODEC_ID_PCM     6   
#define RTSP_CODEC_ID_G711A   8
#define RTSP_CODEC_ID_G711U   12 //RTP payload type: ITU-T G.711 PCMU (0)



#define RTSP_CODEC_ID_MPEG4   1
#define RTSP_CODEC_ID_H264    3   //RTP payload type: DynamicRTP-Type-96 (96)
#define RTSP_CODEC_ID_H265    5
#define RTSP_CODEC_ID_META   26
#define RTSP_CODEC_ID_META_SPECO   27    /* For Speco */

#define RTSP_IS_VIDEO_FRAME(codec_id)    ((codec_id) & 1)

typedef struct _RTSPCallbackInfo
{    
    int frame_type;    
    int size;    
    int key_frame;
    int time_msec;
    int time_sec;
    int fps;
    int jitter;
    /*
     *Video resolution when event is RTSP_EVT_RESOLUTION_CHANGED
     */
    int     width;
    int     height;

    /*
     *Audio 
     */
    int     sample_rate;
    int     sample_bit;
    /*
     *Point to frame data when event is RTSP_EVT_NEW_FRAME
     */
    char *  data;
}RTSPCallbackInfo;

/*
 *  evt             E_RTSP_EVT      indicate which event happend
 *  session_idx     int             indicate which session trigged the event
 */
typedef void (*FUNC_RTSP_CALLBACK)(E_RTSP_EVT evt, int session_idx,
                void* priv_data, RTSPCallbackInfo *rtsp_cb_info);

int rtsp_init(int max_connection);
void rtsp_deinit(void);

/*
 *Wrong username@password combination
 */
#define RTSP_ERR_AUTH_FAIL              (-1)
/*
 *1. Remote device is not online
 *2. Remote device refused the connection
 */
#define RTSP_ERR_NETWORK                (-2)
/*
 *Remote device did support RTSP over UDP only
 */
#define RTSP_ERR_UNSUPPORTED_PROTOCOL   (-3)

/*
 *We meet the session limit (default 64)
 */
#define RTSP_ERR_TOO_MANY_SESSION       (-4)
/*
 *Mutex lock session failed
 */
#define RTSP_ERR_LOCK_SESSION           (-5)
/*
 *Invalid parameter passed
 */
#define RTSP_ERR_PARAM                  (-6)
int rtsp_open(const char *url, const char *username, const char *password, FUNC_RTSP_CALLBACK rtsp_callback, void * priv_data);

int rtsp_close(int fd);

int rtsp_get_ipaddr(int fd, char * ipaddr);

/*
 * The polling version of RTSP_EVT_EOF
 */
typedef enum _RTSP_STREAM_STATE
{
    RTSP_ST_OK,
    RTSP_ST_EOF,
}E_RTSP_STREAM_STATE;
E_RTSP_STREAM_STATE rtsp_get_state(int fd);

#endif //__NVRRTSP_RTSP_H__

