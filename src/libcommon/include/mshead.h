#ifndef _MSHEAD_H_
#define _MSHEAD_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "common.h"

/******************************************************************************
 * MSHEAD内存存储结构示意图
 * +----------------------------+
 * |    MSHEAD INFO             |
 * +----------------------------+
 * |    MSHEAD_SEG_VIDEO_S      |
 * +----------------------------+
 * |    MSHEAD_SEG[0]           |
 * +----------------------------+
 * |          ......            |
 * +----------------------------+
 * |    MSHEAD_SEG[n-1]         |
 * +----------------------------+
 * |                            |
 * |  Audio Data or Video Data  |
 * |                            |
 * +----------------------------+
 * 一个典型的音频帧包括: MSHEAD和Audio Data;
 * 一个典型的视频I帧包括: MSHEAD,MSHEAD_SEG_VIDEO_S,若干MSHEAD_SEG和Video Data;
 * 一个典型的视频P帧包括: MSHEAD,MSHEAD_SEG_VIDEO_S和Video Data;
 *****************************************************************************/
#define MSHEAD_FLAG 0xFF620068  /* 帧头标志 */

#define MSHEAD_HEAD_ALIGN_SIZE  4   /* MSHEAD媒体头对齐长度 */
#define MSHEAD_DATA_ALIGN_SIZE  4   /* MSHEAD媒体数据对齐长度 */

#define MSHEAD_MAX_MSHSIZE_ALIGN_BITS   7
#define MSHEAD_MAX_MSHSIZE_ALIGN_SIZE   128 /* MSHEAD媒体头最大支持对齐长度 */

#define MSHEAD_MAX_MSHSIZE  (0x10000 - MSHEAD_MAX_MSHSIZE_ALIGN_SIZE)   /* MSHEAD媒体头最大支持对齐长度 */
#define MSHEAD_MAXHEADSIZE  (0x10000 - MSHEAD_HEAD_ALIGN_SIZE)          /* 媒体头最大长度(MSHEAD_HEAD_ALIGN_SIZE字节对齐) */
#define MSHEAD_MAXDATASIZE  (0x1000000 - MSHEAD_DATA_ALIGN_SIZE)        /* MSHEAD V1.0媒体数据最大长度(MSHEAD_DATA_ALIGN_SIZE字节对齐) */

#define MSHEAD_VIDEO_WIDTH_ALIGN_SIZE   4   /* MSHEAD媒体头视频宽对齐长度 */
#define MSHEAD_VIDEO_HEIGHT_ALIGN_SIZE  4   /* MSHEAD媒体头视频高对齐长度 */

#define MSHEAD_MAX_VIDEO_FRAME_RATE 127 /* 最大视频帧率 */

typedef enum
{
    MSHEAD_DATA = 0,    /* 音视频数据以包模式 */
    MSHEAD_HEAD = 1,    /* 音视频数据以帧模式 */
} MSHEAD_DATA_MODE_E;

/* Audio ALGORITHM */
typedef enum
{
    MSHEAD_ALGORITHM_AUDIO_PCM8_16  = 0,    /* 16-bit PCM只保留高8位(解码后为,1通道,8000Hz采样率,16-bit的PCM数据) */
    MSHEAD_ALGORITHM_AUDIO_G711A    = 1,    /* G.711 Alaw(解码后为,1通道,8000Hz采样率,16-bit的PCM数据) */
    MSHEAD_ALGORITHM_AUDIO_G722     = 2,    /* G.722(解码后为,1通道,8000Hz采样率,16-bit的PCM数据) */
    MSHEAD_ALGORITHM_AUDIO_G726     = 3,    /* G.726(解码后为,1通道,8000Hz采样率,16-bit的PCM数据) */
    MSHEAD_ALGORITHM_AUDIO_PCM16    = 4,    /* 16-bit PCM(不需要解码,1通道,8000Hz采样率,16-bit的PCM数据)*/
    MSHEAD_ALGORITHM_AUDIO_G711U    = 5,    /* G.711 Ulaw(解码后为,1通道,8000Hz采样率,16-bit的PCM数据) */
    MSHEAD_ALGORITHM_AUDIO_MAX
} MSHEAD_ALGORITHM_AUDIO_TYPE_E;

/* Video ALGORITHM */
typedef enum
{
    MSHEAD_ALGORITHM_VIDEO_H264_HISILICON   = 0,    /* Hisilicon H.264 */
    MSHEAD_ALGORITHM_VIDEO_H264_AMBARELLA   = 1,    /* Ambarella H.264 */
    MSHEAD_ALGORITHM_VIDEO_H264_TECHWELL    = 2,    /* Techwell  H.264 */
    MSHEAD_ALGORITHM_VIDEO_H264_STANDARD    = 3,    /* Standard  H.264 */
    MSHEAD_ALGORITHM_VIDEO_H265_HISILICON   = 4,    /* Hisilicon H.265 */
    MSHEAD_ALGORITHM_VIDEO_H265_GENERAL     = 5,    /* General   H.265 */
    MSHEAD_ALGORITHM_VIDEO_JPEG_STANDARD    = 29,   /* Standard  JPEG */
    MSHEAD_ALGORITHM_VIDEO_MPEG4            = 30,   /* MPEG4 */
    MSHEAD_ALGORITHM_VIDEO_MPEG4_ISO        = 31,   /* MPEG4 ISO */
    MSHEAD_ALGORITHM_VIDEO_MAX              = 32,   /* MSHEAD定义占用5位,最大值只能是31 */
} MSHEAD_ALGORITHM_VIDEO_TYPE_E;

/* 帧类型 */
typedef enum
{
    MSHEAD_STREAM_FRAME_AUDIO   = 0,    /* 音频帧 */
    MSHEAD_STREAM_FRAME_VIDEO_I = 1,    /* 视频I帧 */
    MSHEAD_STREAM_FRAME_VIDEO_P = 2,    /* 视频P帧 */
    MSHEAD_STREAM_FRAME_VIDEO_B = 3,    /* 视频B帧 */
    MSHEAD_STREAM_FRAME_VIDEO_E = 4,    /* 视频E帧 */
} MSHEAD_STREAM_FRAME_TYPE_E;

#define ISMSHEAD(fd)        (MSHEAD_FLAG == ((PMSHEAD)(fd))->flag)
#define ISAUDIOFRAME(fd)    (MSHEAD_STREAM_FRAME_AUDIO == ((PMSHEAD)(fd))->type)
#define ISVIDEOFRAME(fd)    ((MSHEAD_STREAM_FRAME_VIDEO_P == ((PMSHEAD)(fd))->type) || \
    (MSHEAD_STREAM_FRAME_VIDEO_I == ((PMSHEAD)(fd))->type) || (MSHEAD_STREAM_FRAME_VIDEO_B == ((PMSHEAD)(fd))->type))
#define ISKEYFRAME(fd)      (MSHEAD_STREAM_FRAME_VIDEO_I == ((PMSHEAD)(fd))->type)

#define MSHEAD_GETMSHSIZE(fd)   (((PMSHEAD)(fd))->mshsize)  /* 获取媒体头大小 */
#define MSHEAD_GETMSDSIZE(fd)   (((PMSHEAD)(fd))->msdsize)  /* 获取媒体数据大小(不包含媒体头大小) */
#define MSHEAD_GETFRAMESIZE(fd) (MSHEAD_GETMSHSIZE(fd) + MSHEAD_GETMSDSIZE(fd)) /* 获取媒体数据帧大小(包含媒体头大小) */

#define MSHEAD_MSDSIZEINVAL(fd, s)  ((s) > MSHEAD_MAXDATASIZE)  /* 校验设置MSHEAD中msdsize大小合法性 */

#define MSHEAD_GET_MAX_MSHSIZE(fd)      (((PMSHEAD)(fd))->max_mshsize << MSHEAD_MAX_MSHSIZE_ALIGN_BITS) /* 获取媒体头最大支持大小 */
#define MSHEAD_SET_MAX_MSHSIZE(fd, s)  (((PMSHEAD)(fd))->max_mshsize = \
    MUX((s) > MSHEAD_MAX_MSHSIZE, MSHEAD_MAX_MSHSIZE >> MSHEAD_MAX_MSHSIZE_ALIGN_BITS, \
    ALIGN_N((s), MSHEAD_MAX_MSHSIZE_ALIGN_SIZE) >> MSHEAD_MAX_MSHSIZE_ALIGN_BITS))   /* 设置媒体头最大支持大小 */

#define MSHEAD_SETMSHSIZE_MAX(fd, s)    /* 设置MSHEAD中mshsize大小,如果超过最大值按最大值处理 */ \
    MUX(((s) > MSHEAD_MAXHEADSIZE), (fd)->mshsize = MSHEAD_MAXHEADSIZE, (fd)->mshsize = ALIGN_N(s, MSHEAD_HEAD_ALIGN_SIZE))
#define MSHEAD_SETHSDSIZE_ERR(fd, s)    /* 设置MSHEAD中mshsize大小,如果超过最大值按错误处理 */ \
    MUX(((s) > MSHEAD_MAXHEADSIZE), ERRNO(COMMON_ERROR_PARAM, COMP_MSHEAD), (fd)->mshsize = ALIGN_N(s, MSHEAD_HEAD_ALIGN_SIZE))

#define MSHEAD_SETMSDSIZE_MAX(fd, s)    /* 设置MSHEAD中msdsize大小,如果超过最大值按最大值处理,H265不按照4字节对其 */ \
    MUX(((s) > MSHEAD_MAXDATASIZE), (fd)->msdsize = MSHEAD_MAXDATASIZE, \
    MUX((MSHEAD_ALGORITHM_VIDEO_H265_HISILICON == (fd)->algorithm) || \
    (MSHEAD_ALGORITHM_VIDEO_H265_GENERAL == (fd)->algorithm), \
    (fd)->msdsize = s, (fd)->msdsize = ALIGN_N(s, MSHEAD_DATA_ALIGN_SIZE)))
#define MSHEAD_SETMSDSIZE_ERR(fd, s)    /* 设置MSHEAD中msdsize大小,如果超过最大值按错误处理,H265不按照4字节对其 */ \
    MUX(((s) > MSHEAD_MAXDATASIZE), ERRNO(COMMON_ERROR_PARAM, COMP_MSHEAD), \
    MUX((MSHEAD_ALGORITHM_VIDEO_H265_HISILICON == (fd)->algorithm) || \
    (MSHEAD_ALGORITHM_VIDEO_H265_GENERAL == (fd)->algorithm), \
    (fd)->msdsize = s, (fd)->msdsize = ALIGN_N(s, MSHEAD_DATA_ALIGN_SIZE)))

#define MSHEAD_SEGP_FIR(fd)     (((sint32)fd) + MSHEAD_LEN) /* 获取媒体段首地址 */
#define MSHEAD_SEGP_VIDEO(fd)   MSHEAD_SEGP_FIR(fd)         /* 获取媒体视频段地址 */

#define MSHEAD_GET_VIDEO_WIDTH(fd)      MUX(ISAUDIOFRAME(fd), 0, ((PMSHEAD_SEG_VIDEO_S)MSHEAD_SEGP_VIDEO(fd))->width << 2)
#define MSHEAD_SET_VIDEO_WIDTH(fd, w)   MUX(ISAUDIOFRAME(fd), 0, ((PMSHEAD_SEG_VIDEO_S)MSHEAD_SEGP_VIDEO(fd))->width = (w) >> 2)
#define MSHEAD_GET_VIDEO_HEIGHT(fd)     MUX(ISAUDIOFRAME(fd), 0, ((PMSHEAD_SEG_VIDEO_S)MSHEAD_SEGP_VIDEO(fd))->height << 2)
#define MSHEAD_SET_VIDEO_HEIGHT(fd, h)  MUX(ISAUDIOFRAME(fd), 0, ((PMSHEAD_SEG_VIDEO_S)MSHEAD_SEGP_VIDEO(fd))->height = (h) >> 2)

#define MSHEAD_GET_VIDEO_FCOUNT(fd)     MUX(ISAUDIOFRAME(fd), 0, ((PMSHEAD_SEG_VIDEO_S)MSHEAD_SEGP_VIDEO(fd))->fcount)

#define MSHEAD_GET_TIMESTAMP_SEC(fd)    MUX(ISAUDIOFRAME(fd), 0, ((PMSHEAD_SEG_VIDEO_S)MSHEAD_SEGP_VIDEO(fd))->timestamp_low)
#define MSHEAD_GET_TIMESTAMP_MSEC(fd)   MUX(ISAUDIOFRAME(fd), 0, ((PMSHEAD_SEG_VIDEO_S)MSHEAD_SEGP_VIDEO(fd))->timestamp_millisecond)

#define MSHEAD_GET_TIMESTAMP(fd, sec, msec)   MUX(ISAUDIOFRAME(fd), ((sec) = 0) && ((msec) = 0), \
    (sec = ((PMSHEAD_SEG_VIDEO_S)MSHEAD_SEGP_VIDEO(fd))->timestamp_low) && \
    (msec = ((PMSHEAD_SEG_VIDEO_S)MSHEAD_SEGP_VIDEO(fd))->timestamp_millisecond))

#define MSHEAD_SEGP(fd)     MUX(ISAUDIOFRAME(fd), MSHEAD_SEGP_FIR(fd), \
    MSHEAD_SEGP_VIDEO(fd) + ((PMSHEAD_SEG_VIDEO_S)MSHEAD_SEGP_VIDEO(fd))->video_header_size) /* 获取媒体OSD段指针 */
#define MSHEAD_DATAP(fd)    (((sint32)fd) + MSHEAD_GETMSHSIZE(fd))      /* 获取媒体数据指针 */
#define MSHEAD_NEXTP(fd)    (((sint32)fd) + MSHEAD_GETFRAMESIZE(fd))    /* 获取下一个媒体头指针 */

#define MSHEAD_GENERATE_CHECKSUM(fd, pdata, dsize)  mshead_generate_checksum((fd), (sint8 *)(pdata), (dsize))

#define ISVALIDFRAME(fd, s) /* 判断是否是完整的帧 */ \
    (ISMSHEAD(fd) && (MSHEAD_GETMSHSIZE(fd) >= MSHEAD_LEN) && (MSHEAD_GETFRAMESIZE(fd) <= (s)) && \
    (ISVIDEOFRAME(fd) || ISAUDIOFRAME(fd)) && (MSHEAD_GETMSDSIZE(fd) > 0) && (0 != ((PMSHEAD)(fd))->checksum) && \
    MSHEAD_GENERATE_CHECKSUM(fd, MSHEAD_DATAP(fd), MSHEAD_GETMSDSIZE(fd)) == ((PMSHEAD)(fd))->checksum)

/* 段类型 */
typedef enum
{
    MSHEAD_SEG_OSD_STRING   = 0,    /* 自定义字符串 */
    MSHEAD_SEG_OSD_TIME     = 1,    /* 时间信息 */
    MSHEAD_SEG_MOSAIC       = 2,    /* 马赛克 */
    MSHEAD_SEG_MVBLK_TBL    = 3,    /* 移动侦测分块位表信息 */
    MSHEAD_SEG_ENCYPT_INFO  = 4,    /* 加密信息(MSHEAD_ENCYPT_DATA_S) */
} MSHEAD_SEG_TYPE_E;

/* OSD字体 */
typedef enum
{
    MSHEAD_FONT_DEFAULT     = 0,    /* 缺省字体 */
    MSHEAD_FONT_POINT_16    = 1,    /* 16点阵字体 */
    MSHEAD_FONT_POINT_24    = 2,    /* 24点阵字体 */
    MSHEAD_FONT_POINT_32    = 3,    /* 32点阵字体 */
    MSHEAD_FONT_SYSTEM_12   = 4,    /* 12号System字体 */
    MSHEAD_FONT_SYSTEM      = 5,    /* System字体(自适应大小) */
    MSHEAD_FONT_SONGTI      = 6,    /* 宋体(自适应大小) */
} MSHEAD_FONT_E;

/* OSD背景模式 */
typedef enum
{
    MSHEAD_OSD_BACKGROUND_OPAQUE        = 0,    /* 不透明,背景色为黑色 */
    MSHEAD_OSD_BACKGROUND_TRANSPARENT   = 1     /* 透明 */
} MSHEAD_OSD_BACKGROUND_MODE_E;

/* 时间格式,可以自定义多种格式 */
typedef enum
{
    MSHEAD_TIME_FORMAT_WHIPPTREE_YMD_HMS    = 0,    /* yyyy-MM-dd HH:mm:ss ==> %04d-%02d-%02d %02d:%02d:%02d */
    MSHEAD_TIME_FORMAT_WHIPPTREE_YMD_HMS_M  = 1,    /* yyyy-MM-dd HH:mm:ss.mmm ==> %04d-%02d-%02d %02d:%02d:%02d.%03d */
    MSHEAD_TIME_FORMAT_BACKSLASH_MDY_HMS    = 2,    /* MM/dd/yyyy HH:mm:ss ==> %02d/%02d/%04d %02d:%02d:%02d */
    MSHEAD_TIME_FORMAT_SEMICOLON_D_MDY_HMS  = 3,    /* dddd, MMMM d, yyyy, HH:mm:ss ==> %s, %s %d, %04d, %02d:%02d:%02d */
    MSHEAD_TIME_FORMAT_BACKSLASH_MDY_Z_HMS  = 4,    /* M/d/yy h:mm:ss tt ==> %d/%d/%02d %d:%02d:%02d %s */
    MSHEAD_TIME_FORMAT_CHARACTER_YMD_HMS    = 5,    /* yyyy年M月d日 HH时mm分ss秒 ==> %04d年%d月%d日 %02d时%02d分%02d秒 */
    MSHEAD_TIME_FORMAT_CHARACTER_YMD_W_HMS  = 6,    /* yyyy年M月d日 星期 HH时mm分ss秒 ==> %04d年%d月%d日 星期%s %02d时%02d分%02d秒 */
    MSHEAD_TIME_FORMAT_CHARACTER_YMD_Z_HMS  = 7,    /* yyyy年M月d日 tt h:mm:ss ==> 04d年%d月%d日 %s %d:%02d:%02d */
    MSHEAD_TIME_FORMAT_CUSTOM               = 8,    /* 其他自定义格式 */
} MSHEAD_TIME_FORMAT_E;

/* 马赛克填充模式 */
typedef enum
{
    MSHEAD_MOSAIC_FILL_BLACK        = 0,    /* 填充黑色 */
    MSHEAD_MOSAIC_FILL_BACKGROUND   = 1,    /* 填充实时背景色 */
} MSHEAD_MOSAIC_FILL_E;

/* 移动侦测分块位表信息 */
typedef struct
{
    uint8 row;          /* 移动侦测区域行 */
    uint8 col;          /* 移动侦测区域列 */
    uint16 size;        /* move_area大小(根据平台客户端定义最大支持2047字节) */
    char move_area[];   /* 移动侦测区域移动状态(按位表示) */
} MSHEAD_MVBLK_S, *PMSHEAD_MVBLK_S;
#define MSHEAD_MVBLK_S_LEN  sizeof(MSHEAD_MVBLK_S)

/* 加密类型 */
typedef enum
{
    ENCRYPT_TYPE_NONE = 0,  /* 未加密 */
    ENCRYPT_TYPE_ZF,        /* 中孚加密(SMS4加解密) */
    ENCRYPT_TYPE_BLOWFISH,  /* BLOWFISH加解密 */
    ENCRYPT_TYPE_RC4,       /* RC4加解密 */
    ENCRYPT_TYPE_MD5,       /* MD5加密 */
    ENCRYPT_TYPE_BASE64,    /* BASE64加解密 */
    ENCRYPT_TYPE_SMS4,      /* SMS4加解密(中孚) */
} ENCRYPT_TYPE_E;

/* 数据加密类型 */
typedef enum
{
    DATA_ENCRYPT_NONE = 0,  /* 未加密 */
    DATA_ENCRYPT_VIDEO,     /* 视频加密 */
    DATA_ENCRYPT_AUDIO,     /* 音频加密 */
    DATA_ENCRYPT_ALL,       /* 音视频都加密 */
} DATA_ENCRYPT_TYPE_E;

/* MSHEAD_SEG_ENCYPT_INFO加密段数据区扩展信息 */
typedef struct
{
    uint32 encrypt_type;            /* 加密类型(ENCRYPT_TYPE_E) */
    uint32 data_encrypt_type: 8;    /* 数据加密类型(DATA_ENCRYPT_TYPE_E) */
    uint32 data_encrypt_size: 24;   /* 数据加密大小 */
    char data[];                    /* 保留,便于后续扩展 */
} MSHEAD_ENCYPT_DATA_S, *PMSHEAD_ENCYPT_DATA_S;
#define MSHEAD_ENCYPT_DATA_S_LEN    sizeof(MSHEAD_ENCYPT_DATA_S)

/* OSD自定义信息 */
typedef struct
{
    uint16 x;           /* x坐标,以像素为单位 */
    uint16 y;           /* y坐标,以像素为单位 */
    uint8 reserved;     /* 保留 */
    uint8 blue;         /* 蓝色分量 */
    uint8 green;        /* 绿色分量 */
    uint8 red;          /* 红色分量 */
    uint8 font_type;    /* 字体(MSHEAD_FONT_E) */
    uint8 bkmode;       /* 背景模式(MSHEAD_OSD_BACKGROUND_MODE_E) */
    char str[0];        /* 自定义字符串,必须以'\0'结束 */
} MSHEAD_OSD_STRING_S, *PMSHEAD_OSD_STRING_S;
#define MSHEAD_OSD_STRING_S_LEN sizeof(MSHEAD_OSD_STRING_S)

typedef struct 
{
    uint16 x;           /* x坐标,以像素为单位 */
    uint16 y;           /* y坐标,以像素为单位 */
    uint8 reserved;     /* 保留 */
    uint8 blue;         /* 蓝色分量 */
    uint8 green;        /* 绿色分量 */
    uint8 red;          /* 红色分量 */
    uint8 font_type;    /* 字体(MSHEAD_FONT_E) */
    uint8 bkmode;       /* 背景模式(MSHEAD_OSD_BACKGROUND_MODE_E) */
    uint8 format;       /* 时间格式(MSHEAD_TIME_FORMAT_E) */
} MSHEAD_OSD_TIME_S, *PMSHEAD_OSD_TIME_S;
#define MSHEAD_OSD_TIME_S_LEN   sizeof(MSHEAD_OSD_TIME_S)

/* 马赛克 */
typedef struct 
{
    uint16 x;           /* x坐标,以像素为单位 */
    uint16 y;           /* y坐标,以像素为单位 */
    uint16 width;       /* 马赛克的宽度 */
    uint16 height;      /* 马赛克的高度 */
    uint8 fill_mode;    /* 填充模式,使用STREAM_MOSAIC_FILL_* */
} MSHEAD_MOSAIC_S, *PMSHEAD_MOSAIC_S;
#define MSHEAD_MOSAIC_S_LEN sizeof(MSHEAD_MOSAIC_S)

typedef union
{
    MSHEAD_OSD_STRING_S osd;
    MSHEAD_OSD_TIME_S time; /* 时间信息,使用MSHEAD_SEG_VIDEO_S结构体中的timestamp_low,
                             * timestamp_hight和timestamp_millisecond作为时间信息 */
    MSHEAD_MOSAIC_S mosaic;
} MSHEAD_SEG_DATA_U, *PMSHEAD_SEG_DATA_U;

/* MSHEAD段信息 */
typedef struct
{
	uint32 size: 11;        /* 段大小,以字节为单位,取值范围[sizeof(MSHEAD_SEG_S), 2047],
                             * 包括联合体u中扩展内容的长度,当扩展数据为字符串时,包括结束符'\0' */
	uint32 type: 5;         /* 段类型,指定data数据的类型,
	                         * 当段类型是MSHEAD_SEG_OSD_STRING时,MSHEAD_OSD_STRING_S有效;
                             * 当段类型是MSHEAD_SEG_OSD_TIME时,MSHEAD_OSD_TIME_S有效;
                             * 当段类型是MSHEAD_SEG_MOSAIC时,MSHEAD_MOSAIC_S有效;
                             * 当段类型是MSHEAD_SEG_MVBLK_TBL时,MSHEAD_MVBLK_S有效 */
	uint32 reverved: 16;    /* 保留 */
    char data[];
} MSHEAD_SEG, *PMSHEAD_SEG;
#define MSHEAD_SEG_LEN  sizeof(MSHEAD_SEG)
HB_ASSERT_SIZEOF_TYPEDEF_STRUCT(MSHEAD_SEG, 4);

/* 视频帧头信息 */
typedef struct
{
    uint32 video_header_size: 8;        /* 视频帧头的大小,以字节为单位,取值范围[sizeof(MSHEAD_SEG_VIDEO_S), 255] */
    uint32 timestamp_millisecond: 10;   /* 视频帧绝对时间的毫秒值,取值范围[0, 999];
                                         * 可以通过_ftime64或_ftime64_s函数来获取timestamp_low,timestamp_hight和timestamp_millisecond;
                                         * 若不使用timestamp_hight和timestamp_millisecond,可以通过time或_time32函数来获取timestamp_low */
    uint32 width: 14;                   /* 图像宽度,以4像素为单位,取值范围[0, 16383];实际图像宽度等于width * 4 */
    uint32 height: 13;                  /* 图像高度,以4像素为单位,取值范围[0, 8191];实际图像宽度等于height * 4 */
    uint32 frate_rate: 7;               /* 视频帧率,以赫兹(Hz)为单位,取值范围[1, 127] */
    uint32 reserved: 12;                /* 保留 */
    uint32 fcount;                      /* 帧序号;帧序号是逐渐递增的,用于判断视频帧的连续性 */
    sint32 tick_count;                  /* 视频帧相对时间,以毫秒(millisecond)为单位;播放时,用于控制2帧之间的时间间隔 */
    /* 视频帧绝对时间,以秒(second)为单位;timestamp_low为64位整数的低32位,timestamp_hight为64位整数的高32位;
     * 以世界时间(UTC)时间计算,从公元1970-01-01 00:00:00起经过的秒数;timestamp_low最大表示到公元2038-01-19 03:14:07,UTC;
     * timestamp_low和timestamp_hight共同最大表示到公元3000-12-31 23:59:59,UTC */
    uint32 timestamp_low;
    sint32 timestamp_hight;
} MSHEAD_SEG_VIDEO_S, *PMSHEAD_SEG_VIDEO_S;
#define MSHEAD_SEG_VIDEO_S_LEN   sizeof(MSHEAD_SEG_VIDEO_S)
HB_ASSERT_SIZEOF_TYPEDEF_STRUCT(MSHEAD_SEG_VIDEO_S, 24);

/* 流媒体标准头结构体信息(单位:字节,标准大小16字节) */
typedef struct
{
    uint32 flag;                        /* 帧头标志,必须是MSHEAD_FLAG */
    uint32 mshsize: 16;                 /* 媒体头信息大小(MAX size MSHEAD_MAXHEADSIZE) */
    uint32 type: 3;                     /* 媒体帧类型(MSHEAD_STREAM_FRAME_TYPE_E) */
    uint32 algorithm: 5;                /* 音频或视频编码算法(MSHEAD_ALGORITHM_VIDEO_TYPE_E或MSHEAD_ALGORITHM_AUDIO_TYPE_E) */
    uint32 segment: 8;                  /* MSHEAD_SEG结构体的数量,取值范围[0, 255] */
    uint32 checked_encode_data_size: 8; /* 需要校验的编码数据的大小,取值范围[0, 255];一般只校验编码数据的前,后部分字节;
                                         * 当checked_encode_data_size==0时,不校验编码数据;建议:至少校验前后各16个字节的编码数据 */
    uint32 checksum: 24;                /* 校验和,不能等于0;包括MSHEAD_SEG_VIDEO_S,MSHEAD_SEGMENT[0 : n-1]和编码数据的前,后部分字节 */
    uint32 msdsize: 24;                 /* 编码数据的大小,以字节为单位,取值范围[0, MSHEAD_MAXDATASIZE] */
    uint32 max_mshsize: 8;              /* MSHEAD最大长度支持(要求MSHEAD_MAX_MSHSIZE_ALIGN_SIZE字节对齐) */
} MSHEAD, *PMSHEAD;
#define MSHEAD_LEN  sizeof(MSHEAD)
HB_ASSERT_SIZEOF_TYPEDEF_STRUCT(MSHEAD, 16);

/******************************************************************************
 * 函数介绍: 创建包头,此处将为句柄分配max_mshsize字节内存,
 *           其中包含MSHEAD结构体长度,长度函数内部会自动MSHEAD_HEAD_ALIGN_SIZE字节对齐
 * 输入参数: algorithm: 编码标准
 *           max_mshsize: 媒体头信息最大长度,大小最大不能超过MSHEAD_MAXHEADSIZE
 *           checked_encode_data_size: 需要校验的编码数据的大小,取值范围[0, 255];一般只校验编码数据的前,后部分字节;
 *                                     当checked_encode_data_size==0时,不校验编码数据;建议:至少校验前后各16个字节的编码数据
 * 输出参数: 无
 * 返回值  : <0-错误,>0-成功,表示创建的句柄
 *****************************************************************************/
sint32 mshead_open(sint32 algorithm, sint32 max_mshsize, uint8 checked_encode_data_size);

/******************************************************************************
 * 函数介绍: 根据已存在的内存地址基础上创建媒体头信息,
 * 输入参数: algorithm: 编码标准
 *           pmshead: 媒体头信息结构体指针
 *           max_mshsize: 媒体头信息最大长度,长度要求MSHEAD_HEAD_ALIGN_SIZE字节对齐,
 *                        大小最小不能小于MSHEAD_LEN,长度超过MSHEAD_MAXHEADSIZE部分将被截断
 *           checked_encode_data_size: 需要校验的编码数据的大小,取值范围[0, 255];一般只校验编码数据的前,后部分字节;
 *                                     当checked_encode_data_size==0时,不校验编码数据;建议:至少校验前后各16个字节的编码数据
 * 输出参数: 无
 * 返回值  : <0-错误,>0-成功,表示创建的句柄
 *****************************************************************************/
sint32 mshead_openext(sint32 algorithm, PMSHEAD pmshead, sint32 max_mshsize, uint8 checked_encode_data_size);

/******************************************************************************
 * 函数介绍: 写入帧信息数据,编码数据不进行拷贝
 * 输入参数: handle: MSHEAD句柄
 *           frame_type: 帧类型(MSHEAD_STREAM_FRAME_TYPE_E)
 *           en_checksum: 是否需要生成校验和(注意:在没有其他段信息时,建议开启使能;如果不使能,
 *                        需要主动调用接口mshead_update_checksum生成,调用前务必在所有的SEG段信息写入)
 *           data: 音视频数据缓冲区(实际未使用)
 *           size: 音视频数据长度
 *           sec: 只对视频有效,当前帧时间的秒数,表示从公元1970年1月1日0时0分0秒算起,
 *                至今的UTC时间所经过的秒数(注意: 当该值为0时,时间戳由内部产生)
 *           msec: 只对视频有效,当前帧时间(单位: 毫秒)
 *           width: 只对视频有效,则表示宽度(单位1像素)
 *           height: 只对视频有效,则表示高度(单位1像素)
 *           frame_rate: 视频帧率,只有视频帧是有效,最大值为MSHEAD_MAX_VIDEO_FRAME_RATE
 * 输出参数: 无
 * 返回值  : <0-错误,>0-成功,表示写入的数据长度
 *  (注意: 长度不够MSHEAD_HEAD_ALIGN_SIZE字节对齐将自动加长后对齐,H265不按照4字节对其)
 *****************************************************************************/
sint32 mshead_writeext(sint32 handle, MSHEAD_STREAM_FRAME_TYPE_E frame_type, HB_BOOL en_checksum, 
    sint8 *data, uint32 size, uint32 sec, uint32 msec, uint32 width, uint32 height, uint32 frame_rate);

/******************************************************************************
 * 函数介绍: 写入帧信息数据,编码数据不进行拷贝
 * 输入参数: handle: MSHEAD句柄
 *           frame_type: 帧类型对于视频,对于音频无效
 *           en_checksum: 是否需要生成校验和(注意:在没有其他段信息时,建议开启使能;如果不使能,
 *                        需要主动调用接口mshead_update_checksum生成,调用前务必在所有的SEG段信息写入)
 *           data: 音视频数据缓冲区,实际未使用
 *           size: 音视频数据长度
 *           width: 只对视频有效,则表示宽度(单位1像素)
 *           height: 只对视频有效,则表示高度(单位1像素)
 *           frame_rate: 视频帧率,只有视频帧是有效,最大值为MSHEAD_MAX_VIDEO_FRAME_RATE
 * 输出参数: 无
 * 返回值  : <0-错误,>0-成功,表示写入的数据长度
 *  (注意: 长度不够MSHEAD_HEAD_ALIGN_SIZE字节对齐将自动加长后对齐,H265不按照4字节对其)
 *****************************************************************************/
sint32 mshead_write(sint32 handle, MSHEAD_STREAM_FRAME_TYPE_E frame_type, HB_BOOL en_checksum, 
    sint8 *data, uint32 size, uint32 width, uint32 height, uint32 frame_rate);

/******************************************************************************
 * 函数介绍: 读取音视频数据,获取数据指针,以及长度
 * 输入参数: handle: MSHEAD句柄
 * 输出参数: data: 音视频数据缓冲区地址指针
 *           size: 音视频数据长度指针
 * 返回值  : <0-错误,>0-成功,返回音视频数据缓冲区指针
 *****************************************************************************/
sint32 mshead_read(sint32 handle, sint8 **data, uint32 *size);

/******************************************************************************
 * 函数介绍: 写入短信息数据MSHEAD_SEG
 * 输入参数: handle: MSHEAD句柄
 *           seg: 段头信息,除size,data其余数据由用户输入
 *           data: 用户数据
 *           size: 用户数据长度,如为字符串应包含尾部'\0'的长度
 * 输出参数: 无
 * 返回值  : <0-错误,>0-成功,表示当前包头长度(注意: 长度不够MSHEAD_HEAD_ALIGN_SIZE字节对齐将自动加长后对齐)
 *****************************************************************************/
sint32 mshead_write_seg(sint32 handle, MSHEAD_SEG seg, sint8 *data, uint32 size);

/******************************************************************************
 * 函数介绍: 更新帧信息及数据校验值(务必在所有的SEG段信息写入后才能更新)
 * 输入参数: handle: MSHEAD句柄
 *           data: 音视频数据缓冲区(实际未使用)
 *           size: 音视频数据长度
 * 输出参数: 无
 * 返回值  : 0-成功,<0-错误代码
 *****************************************************************************/
sint32 mshead_update_checksum(sint32 handle, sint8 *data, uint32 size);

/******************************************************************************
 * 函数介绍: 获取包头信息中的一个段信息
 * 输入参数: handle: MSHEAD句柄
 *           index: MSHEAD_SEG段索引序号
 * 返回值: <0-失败,0-句柄中无可用信息,>0-则指向获取的段信息指针PMSHEAD_SEG
 *****************************************************************************/
PMSHEAD_SEG mshead_read_seg(sint32 handle, uint32 index);

/******************************************************************************
 * 函数介绍: 生成一帧数据的校验和
 * 输入参数: handle: MSHEAD句柄
 * 输出参数: data: 音视频数据缓冲区地址指针
 *           size: 音视频数据长度指针
 * 返回值: <=0-失败,>0-数据的校验和
 *****************************************************************************/
sint32 mshead_generate_checksum(sint32 handle, sint8 *data, uint32 size);

/******************************************************************************
 * 函数介绍: MSHEAD配置
 * 输入参数: handle: MSHEAD句柄
 *           cmd: 命令
 *           channel: 通道号,此处无效
 *           param: 输入参数
 *           param_len: param长度,特别对于GET命令时,输出参数应先判断缓冲区是否足够
 * 输出参数: param: 输出参数
 * 返回值  : 0-成功,<0-错误代码
 *****************************************************************************/
sint32 mshead_ioctrl(sint32 handle, sint32 cmd, sint32 channel, sint32 *param, sint32 param_len);

/******************************************************************************
 * 函数介绍: 关闭MSHEAD,该函数将清除句柄所有数据,并释放相关资源
 * 输入参数: handle: MSHEAD句柄;
 * 输出参数: 无
 * 返回值  : 0-成功,<0-错误代码
 *****************************************************************************/
sint32 mshead_close(sint32 handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _MSHEAD_H_ */
