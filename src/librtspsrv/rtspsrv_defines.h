#ifndef __FENICE_TYPES_H
#define __FENICE_TYPES_H

#ifndef _WIN32
#define closesocket close
#endif

#define CHN_MAX 1
#define EV_MAX 128
#define CH_STRM_MAX 3   /* 通道支持的最大流数量, 目前是三个流 */
#define MD_STRM_MAX 3   /* RTP会话支持的最大媒体流数量, 目前是音频、视频、文本 */

#define MAX_IFRM_NUM 1
#define WORKER_MAX 4

#define RTSP_BUFSIZE  2048
#define RTSP_WV_BUFSIZE  (16 << 10)
#define MAX_DESCR_LENGTH 2048
#define MAX_STR_LENGTH 256
#define MAX_SDP_LENGTH 512
#define MAX_SPS_LENGTH 256

#define SOCKET_SLICE_SIZE (24 << 10) //24576
#define SOCKET_SLICE_SIZE_MIN  (4 << 10) //4096

#define GET_MAIN_STREAM  "MainStream"
#define GET_SUB_STREAM1  "SubStream"
#define GET_SUB_STREAM2  "ThirdStream"

#define FIFO_CONSUMER_NUM    64  /* 录像+实时解码+16个网络消费者 */
#define FIFO_MAX_USER  64        /* 最大netms用户数*/

#define MAIN_REC_FIFO_SIZE    (2 * 1024 * 1024)
#define SUB_REC_FIFO_SIZE    (1 * 1024 * 1024)

#define FIFO_HTWATER_SIZE    85
#define FIFO_HIWATER_SIZE    75
#define FIFO_LOWATER_SIZE    20

#define MAX_NO_FRM_VAL (20 * 1000)
#define MIN_NO_FRM_VAL (10 * 1000)

#define MAX_TRY_TIME 10

#define MAX_SESSION_USER  99        /* 最大会话用户数*/
#define RSRV_CLI_NUM 512

#define NTP_OFFSET 2208988800ULL
#define NTP_OFFSET_US (NTP_OFFSET * 1000000ULL)

#define ONE_SECONDS  1000000
#define TEN_SECONDS  10000000
#define THIRTY_SECONDS  30000000


//Error Number
#define ERR_NOERROR 		 0
#define ERR_GENERIC 		 -1
#define ERR_NOT_FOUND		 -2
#define ERR_PARSE			 -3
#define ERR_ALLOC			 -4
#define ERR_INPUT_PARAM 	 -5
#define ERR_UNSUPPORTED_PT	 -6
#define ERR_EOF 			 -7
#define ERR_FATAL			 -8
#define ERR_CONNECTION_CLOSE -9

//Message Header Keywords
#define HDR_CONTENTLENGTH    "Content-Length"
#define HDR_ACCEPT           "Accept"
#define HDR_ALLOW            "Allow"
#define HDR_BLOCKSIZE        "Blocksize"
#define HDR_CONTENTTYPE      "Content-Type"
#define HDR_DATE             "Date"
#define HDR_REQUIRE          "Require"
#define HDR_TRANSPORTREQUIRE "Transport-Require"
#define HDR_SEQUENCENO       "SequenceNo"
#define HDR_CSEQ             "CSeq"
#define HDR_STREAM           "Stream"
#define HDR_SESSION          "Session"
#define HDR_TRANSPORT        "Transport"
#define HDR_RANGE            "Range"
#define HDR_USER_AGENT       "User-Agent"

#define VIDEO 0
#define AUDIO 1
#define META  2

#ifndef _RTSPSRV_TEST
#define AV_LOG_ERROR 1
#define AV_LOG_WARNING 2
/*#define rsrv_log(lvl, fmt, args...) TRACE_DBG_LVL(COMP_RTSPSRV, lvl, fmt, ## args)*/
/*#define rsrv_log(lvl, fmt, args...) TRACE_ERR(COMP_RTSPSRV, fmt, ## args)*/
#define rsrv_log(lvl, fmt, args...) TRACE_ALWAYS(fmt, ## args)
#else
#define rsrv_log(lvl, fmt, ...) av_log(NULL, lvl, fmt, __VA_ARGS__)
#endif

/** RTSP handling */
enum RTSPStatusCode
{
    RTSP_STATUS_CONTINUE             = 100,
    RTSP_STATUS_OK                   = 200,
    RTSP_STATUS_CREATED              = 201,
    RTSP_STATUS_LOW_ON_STORAGE_SPACE = 250,
    RTSP_STATUS_MULTIPLE_CHOICES     = 300,
    RTSP_STATUS_MOVED_PERMANENTLY    = 301,
    RTSP_STATUS_MOVED_TEMPORARILY    = 302,
    RTSP_STATUS_SEE_OTHER            = 303,
    RTSP_STATUS_NOT_MODIFIED         = 304,
    RTSP_STATUS_USE_PROXY            = 305,
    RTSP_STATUS_BAD_REQUEST          = 400,
    RTSP_STATUS_UNAUTHORIZED         = 401,
    RTSP_STATUS_PAYMENT_REQUIRED     = 402,
    RTSP_STATUS_FORBIDDEN            = 403,
    RTSP_STATUS_NOT_FOUND            = 404,
    RTSP_STATUS_METHOD               = 405,
    RTSP_STATUS_NOT_ACCEPTABLE       = 406,
    RTSP_STATUS_PROXY_AUTH_REQUIRED  = 407,
    RTSP_STATUS_REQ_TIME_OUT         = 408,
    RTSP_STATUS_GONE                 = 410,
    RTSP_STATUS_LENGTH_REQUIRED      = 411,
    RTSP_STATUS_PRECONDITION_FAILED  = 412,
    RTSP_STATUS_REQ_ENTITY_2LARGE    = 413,
    RTSP_STATUS_REQ_URI_2LARGE       = 414,
    RTSP_STATUS_UNSUPPORTED_MTYPE    = 415,
    RTSP_STATUS_PARAM_NOT_UNDERSTOOD = 451,
    RTSP_STATUS_CONFERENCE_NOT_FOUND = 452,
    RTSP_STATUS_BANDWIDTH            = 453,
    RTSP_STATUS_SESSION              = 454,
    RTSP_STATUS_STATE                = 455,
    RTSP_STATUS_INVALID_HEADER_FIELD = 456,
    RTSP_STATUS_INVALID_RANGE        = 457,
    RTSP_STATUS_RONLY_PARAMETER      = 458,
    RTSP_STATUS_AGGREGATE            = 459,
    RTSP_STATUS_ONLY_AGGREGATE       = 460,
    RTSP_STATUS_TRANSPORT            = 461,
    RTSP_STATUS_UNREACHABLE          = 462,
    RTSP_STATUS_INTERNAL             = 500,
    RTSP_STATUS_NOT_IMPLEMENTED      = 501,
    RTSP_STATUS_BAD_GATEWAY          = 502,
    RTSP_STATUS_SERVICE              = 503,
    RTSP_STATUS_GATEWAY_TIME_OUT     = 504,
    RTSP_STATUS_VERSION              = 505,
    RTSP_STATUS_UNSUPPORTED_OPTION   = 551,
};


typedef enum _chstrm_type
{
    MAIN_STREAM,
    SUB_STREAM1,
    SUB_STREAM2,
    STREAM_NUM
} chstrm_type_e;

/*one stream buffer content madia type info*/
typedef enum _rtpmd_type
{
    M_VIDEO,    //video type
    M_AUDIO, 	//audio type
    M_TEXT,		//text type
    MEDIA_NUM
} rtpmd_type_e;

typedef enum _video_type
{
    VIDEO_H264,
    VIDEO_H265,
    VIDEO_NUM
} video_type_e;

typedef enum _audio_type
{
    AUDIO_G711,
    AUDIO_NUM
} audio_type_e;

/* NAL unit types */
enum H264NALUnitType
{
    H264_NAL_SLICE           = 1,
    H264_NAL_DPA             = 2,
    H264_NAL_DPB             = 3,
    H264_NAL_DPC             = 4,
    H264_NAL_IDR_SLICE       = 5,
    H264_NAL_SEI             = 6,
    H264_NAL_SPS             = 7,
    H264_NAL_PPS             = 8,
    H264_NAL_AUD             = 9,
    H264_NAL_END_SEQUENCE    = 10,
    H264_NAL_END_STREAM      = 11,
    H264_NAL_FILLER_DATA     = 12,
    H264_NAL_SPS_EXT         = 13,
    H264_NAL_AUXILIARY_SLICE = 19,
    H264_NAL_FF_IGNORE       = 0xff0f001,
};


enum HevcNALUnitType
{
    HEVC_NAL_TRAIL_N = 0,
    HEVC_NAL_TRAIL_R = 1,
    HEVC_NAL_TSA_N = 2,
    HEVC_NAL_TSA_R = 3,
    HEVC_NAL_STSA_N = 4,
    HEVC_NAL_STSA_R = 5,
    HEVC_NAL_RADL_N = 6,
    HEVC_NAL_RADL_R = 7,
    HEVC_NAL_RASL_N = 8,
    HEVC_NAL_RASL_R = 9,
    HEVC_NAL_BLA_W_LP = 16,
    HEVC_NAL_BLA_W_RADL = 17,
    HEVC_NAL_BLA_N_LP = 18,
    HEVC_NAL_IDR_W_RADL = 19,
    HEVC_NAL_IDR_N_LP = 20,
    HEVC_NAL_CRA_NUT = 21,
    HEVC_NAL_VPS = 32,
    HEVC_NAL_SPS = 33,
    HEVC_NAL_PPS = 34,
    HEVC_NAL_AUD = 35,
    HEVC_NAL_EOS_NUT = 36,
    HEVC_NAL_EOB_NUT = 37,
    HEVC_NAL_FD_NUT = 38,
    HEVC_NAL_SEI_PREFIX = 39,
    HEVC_NAL_SEI_SUFFIX = 40
};

#endif


