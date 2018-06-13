#ifndef _MSHEAD_H_
#define _MSHEAD_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "common.h"

/******************************************************************************
 * MSHEAD�ڴ�洢�ṹʾ��ͼ
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
 * һ�����͵���Ƶ֡����: MSHEAD��Audio Data;
 * һ�����͵���ƵI֡����: MSHEAD,MSHEAD_SEG_VIDEO_S,����MSHEAD_SEG��Video Data;
 * һ�����͵���ƵP֡����: MSHEAD,MSHEAD_SEG_VIDEO_S��Video Data;
 *****************************************************************************/
#define MSHEAD_FLAG 0xFF620068  /* ֡ͷ��־ */

#define MSHEAD_HEAD_ALIGN_SIZE  4   /* MSHEADý��ͷ���볤�� */
#define MSHEAD_DATA_ALIGN_SIZE  4   /* MSHEADý�����ݶ��볤�� */

#define MSHEAD_MAX_MSHSIZE_ALIGN_BITS   7
#define MSHEAD_MAX_MSHSIZE_ALIGN_SIZE   128 /* MSHEADý��ͷ���֧�ֶ��볤�� */

#define MSHEAD_MAX_MSHSIZE  (0x10000 - MSHEAD_MAX_MSHSIZE_ALIGN_SIZE)   /* MSHEADý��ͷ���֧�ֶ��볤�� */
#define MSHEAD_MAXHEADSIZE  (0x10000 - MSHEAD_HEAD_ALIGN_SIZE)          /* ý��ͷ��󳤶�(MSHEAD_HEAD_ALIGN_SIZE�ֽڶ���) */
#define MSHEAD_MAXDATASIZE  (0x1000000 - MSHEAD_DATA_ALIGN_SIZE)        /* MSHEAD V1.0ý��������󳤶�(MSHEAD_DATA_ALIGN_SIZE�ֽڶ���) */

#define MSHEAD_VIDEO_WIDTH_ALIGN_SIZE   4   /* MSHEADý��ͷ��Ƶ����볤�� */
#define MSHEAD_VIDEO_HEIGHT_ALIGN_SIZE  4   /* MSHEADý��ͷ��Ƶ�߶��볤�� */

#define MSHEAD_MAX_VIDEO_FRAME_RATE 127 /* �����Ƶ֡�� */

typedef enum
{
    MSHEAD_DATA = 0,    /* ����Ƶ�����԰�ģʽ */
    MSHEAD_HEAD = 1,    /* ����Ƶ������֡ģʽ */
} MSHEAD_DATA_MODE_E;

/* Audio ALGORITHM */
typedef enum
{
    MSHEAD_ALGORITHM_AUDIO_PCM8_16  = 0,    /* 16-bit PCMֻ������8λ(�����Ϊ,1ͨ��,8000Hz������,16-bit��PCM����) */
    MSHEAD_ALGORITHM_AUDIO_G711A    = 1,    /* G.711 Alaw(�����Ϊ,1ͨ��,8000Hz������,16-bit��PCM����) */
    MSHEAD_ALGORITHM_AUDIO_G722     = 2,    /* G.722(�����Ϊ,1ͨ��,8000Hz������,16-bit��PCM����) */
    MSHEAD_ALGORITHM_AUDIO_G726     = 3,    /* G.726(�����Ϊ,1ͨ��,8000Hz������,16-bit��PCM����) */
    MSHEAD_ALGORITHM_AUDIO_PCM16    = 4,    /* 16-bit PCM(����Ҫ����,1ͨ��,8000Hz������,16-bit��PCM����)*/
    MSHEAD_ALGORITHM_AUDIO_G711U    = 5,    /* G.711 Ulaw(�����Ϊ,1ͨ��,8000Hz������,16-bit��PCM����) */
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
    MSHEAD_ALGORITHM_VIDEO_MAX              = 32,   /* MSHEAD����ռ��5λ,���ֵֻ����31 */
} MSHEAD_ALGORITHM_VIDEO_TYPE_E;

/* ֡���� */
typedef enum
{
    MSHEAD_STREAM_FRAME_AUDIO   = 0,    /* ��Ƶ֡ */
    MSHEAD_STREAM_FRAME_VIDEO_I = 1,    /* ��ƵI֡ */
    MSHEAD_STREAM_FRAME_VIDEO_P = 2,    /* ��ƵP֡ */
    MSHEAD_STREAM_FRAME_VIDEO_B = 3,    /* ��ƵB֡ */
    MSHEAD_STREAM_FRAME_VIDEO_E = 4,    /* ��ƵE֡ */
} MSHEAD_STREAM_FRAME_TYPE_E;

#define ISMSHEAD(fd)        (MSHEAD_FLAG == ((PMSHEAD)(fd))->flag)
#define ISAUDIOFRAME(fd)    (MSHEAD_STREAM_FRAME_AUDIO == ((PMSHEAD)(fd))->type)
#define ISVIDEOFRAME(fd)    ((MSHEAD_STREAM_FRAME_VIDEO_P == ((PMSHEAD)(fd))->type) || \
    (MSHEAD_STREAM_FRAME_VIDEO_I == ((PMSHEAD)(fd))->type) || (MSHEAD_STREAM_FRAME_VIDEO_B == ((PMSHEAD)(fd))->type))
#define ISKEYFRAME(fd)      (MSHEAD_STREAM_FRAME_VIDEO_I == ((PMSHEAD)(fd))->type)

#define MSHEAD_GETMSHSIZE(fd)   (((PMSHEAD)(fd))->mshsize)  /* ��ȡý��ͷ��С */
#define MSHEAD_GETMSDSIZE(fd)   (((PMSHEAD)(fd))->msdsize)  /* ��ȡý�����ݴ�С(������ý��ͷ��С) */
#define MSHEAD_GETFRAMESIZE(fd) (MSHEAD_GETMSHSIZE(fd) + MSHEAD_GETMSDSIZE(fd)) /* ��ȡý������֡��С(����ý��ͷ��С) */

#define MSHEAD_MSDSIZEINVAL(fd, s)  ((s) > MSHEAD_MAXDATASIZE)  /* У������MSHEAD��msdsize��С�Ϸ��� */

#define MSHEAD_GET_MAX_MSHSIZE(fd)      (((PMSHEAD)(fd))->max_mshsize << MSHEAD_MAX_MSHSIZE_ALIGN_BITS) /* ��ȡý��ͷ���֧�ִ�С */
#define MSHEAD_SET_MAX_MSHSIZE(fd, s)  (((PMSHEAD)(fd))->max_mshsize = \
    MUX((s) > MSHEAD_MAX_MSHSIZE, MSHEAD_MAX_MSHSIZE >> MSHEAD_MAX_MSHSIZE_ALIGN_BITS, \
    ALIGN_N((s), MSHEAD_MAX_MSHSIZE_ALIGN_SIZE) >> MSHEAD_MAX_MSHSIZE_ALIGN_BITS))   /* ����ý��ͷ���֧�ִ�С */

#define MSHEAD_SETMSHSIZE_MAX(fd, s)    /* ����MSHEAD��mshsize��С,����������ֵ�����ֵ���� */ \
    MUX(((s) > MSHEAD_MAXHEADSIZE), (fd)->mshsize = MSHEAD_MAXHEADSIZE, (fd)->mshsize = ALIGN_N(s, MSHEAD_HEAD_ALIGN_SIZE))
#define MSHEAD_SETHSDSIZE_ERR(fd, s)    /* ����MSHEAD��mshsize��С,����������ֵ�������� */ \
    MUX(((s) > MSHEAD_MAXHEADSIZE), ERRNO(COMMON_ERROR_PARAM, COMP_MSHEAD), (fd)->mshsize = ALIGN_N(s, MSHEAD_HEAD_ALIGN_SIZE))

#define MSHEAD_SETMSDSIZE_MAX(fd, s)    /* ����MSHEAD��msdsize��С,����������ֵ�����ֵ����,H265������4�ֽڶ��� */ \
    MUX(((s) > MSHEAD_MAXDATASIZE), (fd)->msdsize = MSHEAD_MAXDATASIZE, \
    MUX((MSHEAD_ALGORITHM_VIDEO_H265_HISILICON == (fd)->algorithm) || \
    (MSHEAD_ALGORITHM_VIDEO_H265_GENERAL == (fd)->algorithm), \
    (fd)->msdsize = s, (fd)->msdsize = ALIGN_N(s, MSHEAD_DATA_ALIGN_SIZE)))
#define MSHEAD_SETMSDSIZE_ERR(fd, s)    /* ����MSHEAD��msdsize��С,����������ֵ��������,H265������4�ֽڶ��� */ \
    MUX(((s) > MSHEAD_MAXDATASIZE), ERRNO(COMMON_ERROR_PARAM, COMP_MSHEAD), \
    MUX((MSHEAD_ALGORITHM_VIDEO_H265_HISILICON == (fd)->algorithm) || \
    (MSHEAD_ALGORITHM_VIDEO_H265_GENERAL == (fd)->algorithm), \
    (fd)->msdsize = s, (fd)->msdsize = ALIGN_N(s, MSHEAD_DATA_ALIGN_SIZE)))

#define MSHEAD_SEGP_FIR(fd)     (((sint32)fd) + MSHEAD_LEN) /* ��ȡý����׵�ַ */
#define MSHEAD_SEGP_VIDEO(fd)   MSHEAD_SEGP_FIR(fd)         /* ��ȡý����Ƶ�ε�ַ */

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
    MSHEAD_SEGP_VIDEO(fd) + ((PMSHEAD_SEG_VIDEO_S)MSHEAD_SEGP_VIDEO(fd))->video_header_size) /* ��ȡý��OSD��ָ�� */
#define MSHEAD_DATAP(fd)    (((sint32)fd) + MSHEAD_GETMSHSIZE(fd))      /* ��ȡý������ָ�� */
#define MSHEAD_NEXTP(fd)    (((sint32)fd) + MSHEAD_GETFRAMESIZE(fd))    /* ��ȡ��һ��ý��ͷָ�� */

#define MSHEAD_GENERATE_CHECKSUM(fd, pdata, dsize)  mshead_generate_checksum((fd), (sint8 *)(pdata), (dsize))

#define ISVALIDFRAME(fd, s) /* �ж��Ƿ���������֡ */ \
    (ISMSHEAD(fd) && (MSHEAD_GETMSHSIZE(fd) >= MSHEAD_LEN) && (MSHEAD_GETFRAMESIZE(fd) <= (s)) && \
    (ISVIDEOFRAME(fd) || ISAUDIOFRAME(fd)) && (MSHEAD_GETMSDSIZE(fd) > 0) && (0 != ((PMSHEAD)(fd))->checksum) && \
    MSHEAD_GENERATE_CHECKSUM(fd, MSHEAD_DATAP(fd), MSHEAD_GETMSDSIZE(fd)) == ((PMSHEAD)(fd))->checksum)

/* ������ */
typedef enum
{
    MSHEAD_SEG_OSD_STRING   = 0,    /* �Զ����ַ��� */
    MSHEAD_SEG_OSD_TIME     = 1,    /* ʱ����Ϣ */
    MSHEAD_SEG_MOSAIC       = 2,    /* ������ */
    MSHEAD_SEG_MVBLK_TBL    = 3,    /* �ƶ����ֿ�λ����Ϣ */
    MSHEAD_SEG_ENCYPT_INFO  = 4,    /* ������Ϣ(MSHEAD_ENCYPT_DATA_S) */
} MSHEAD_SEG_TYPE_E;

/* OSD���� */
typedef enum
{
    MSHEAD_FONT_DEFAULT     = 0,    /* ȱʡ���� */
    MSHEAD_FONT_POINT_16    = 1,    /* 16�������� */
    MSHEAD_FONT_POINT_24    = 2,    /* 24�������� */
    MSHEAD_FONT_POINT_32    = 3,    /* 32�������� */
    MSHEAD_FONT_SYSTEM_12   = 4,    /* 12��System���� */
    MSHEAD_FONT_SYSTEM      = 5,    /* System����(����Ӧ��С) */
    MSHEAD_FONT_SONGTI      = 6,    /* ����(����Ӧ��С) */
} MSHEAD_FONT_E;

/* OSD����ģʽ */
typedef enum
{
    MSHEAD_OSD_BACKGROUND_OPAQUE        = 0,    /* ��͸��,����ɫΪ��ɫ */
    MSHEAD_OSD_BACKGROUND_TRANSPARENT   = 1     /* ͸�� */
} MSHEAD_OSD_BACKGROUND_MODE_E;

/* ʱ���ʽ,�����Զ�����ָ�ʽ */
typedef enum
{
    MSHEAD_TIME_FORMAT_WHIPPTREE_YMD_HMS    = 0,    /* yyyy-MM-dd HH:mm:ss ==> %04d-%02d-%02d %02d:%02d:%02d */
    MSHEAD_TIME_FORMAT_WHIPPTREE_YMD_HMS_M  = 1,    /* yyyy-MM-dd HH:mm:ss.mmm ==> %04d-%02d-%02d %02d:%02d:%02d.%03d */
    MSHEAD_TIME_FORMAT_BACKSLASH_MDY_HMS    = 2,    /* MM/dd/yyyy HH:mm:ss ==> %02d/%02d/%04d %02d:%02d:%02d */
    MSHEAD_TIME_FORMAT_SEMICOLON_D_MDY_HMS  = 3,    /* dddd, MMMM d, yyyy, HH:mm:ss ==> %s, %s %d, %04d, %02d:%02d:%02d */
    MSHEAD_TIME_FORMAT_BACKSLASH_MDY_Z_HMS  = 4,    /* M/d/yy h:mm:ss tt ==> %d/%d/%02d %d:%02d:%02d %s */
    MSHEAD_TIME_FORMAT_CHARACTER_YMD_HMS    = 5,    /* yyyy��M��d�� HHʱmm��ss�� ==> %04d��%d��%d�� %02dʱ%02d��%02d�� */
    MSHEAD_TIME_FORMAT_CHARACTER_YMD_W_HMS  = 6,    /* yyyy��M��d�� ���� HHʱmm��ss�� ==> %04d��%d��%d�� ����%s %02dʱ%02d��%02d�� */
    MSHEAD_TIME_FORMAT_CHARACTER_YMD_Z_HMS  = 7,    /* yyyy��M��d�� tt h:mm:ss ==> 04d��%d��%d�� %s %d:%02d:%02d */
    MSHEAD_TIME_FORMAT_CUSTOM               = 8,    /* �����Զ����ʽ */
} MSHEAD_TIME_FORMAT_E;

/* ���������ģʽ */
typedef enum
{
    MSHEAD_MOSAIC_FILL_BLACK        = 0,    /* ����ɫ */
    MSHEAD_MOSAIC_FILL_BACKGROUND   = 1,    /* ���ʵʱ����ɫ */
} MSHEAD_MOSAIC_FILL_E;

/* �ƶ����ֿ�λ����Ϣ */
typedef struct
{
    uint8 row;          /* �ƶ���������� */
    uint8 col;          /* �ƶ���������� */
    uint16 size;        /* move_area��С(����ƽ̨�ͻ��˶������֧��2047�ֽ�) */
    char move_area[];   /* �ƶ���������ƶ�״̬(��λ��ʾ) */
} MSHEAD_MVBLK_S, *PMSHEAD_MVBLK_S;
#define MSHEAD_MVBLK_S_LEN  sizeof(MSHEAD_MVBLK_S)

/* �������� */
typedef enum
{
    ENCRYPT_TYPE_NONE = 0,  /* δ���� */
    ENCRYPT_TYPE_ZF,        /* ���ڼ���(SMS4�ӽ���) */
    ENCRYPT_TYPE_BLOWFISH,  /* BLOWFISH�ӽ��� */
    ENCRYPT_TYPE_RC4,       /* RC4�ӽ��� */
    ENCRYPT_TYPE_MD5,       /* MD5���� */
    ENCRYPT_TYPE_BASE64,    /* BASE64�ӽ��� */
    ENCRYPT_TYPE_SMS4,      /* SMS4�ӽ���(����) */
} ENCRYPT_TYPE_E;

/* ���ݼ������� */
typedef enum
{
    DATA_ENCRYPT_NONE = 0,  /* δ���� */
    DATA_ENCRYPT_VIDEO,     /* ��Ƶ���� */
    DATA_ENCRYPT_AUDIO,     /* ��Ƶ���� */
    DATA_ENCRYPT_ALL,       /* ����Ƶ������ */
} DATA_ENCRYPT_TYPE_E;

/* MSHEAD_SEG_ENCYPT_INFO���ܶ���������չ��Ϣ */
typedef struct
{
    uint32 encrypt_type;            /* ��������(ENCRYPT_TYPE_E) */
    uint32 data_encrypt_type: 8;    /* ���ݼ�������(DATA_ENCRYPT_TYPE_E) */
    uint32 data_encrypt_size: 24;   /* ���ݼ��ܴ�С */
    char data[];                    /* ����,���ں�����չ */
} MSHEAD_ENCYPT_DATA_S, *PMSHEAD_ENCYPT_DATA_S;
#define MSHEAD_ENCYPT_DATA_S_LEN    sizeof(MSHEAD_ENCYPT_DATA_S)

/* OSD�Զ�����Ϣ */
typedef struct
{
    uint16 x;           /* x����,������Ϊ��λ */
    uint16 y;           /* y����,������Ϊ��λ */
    uint8 reserved;     /* ���� */
    uint8 blue;         /* ��ɫ���� */
    uint8 green;        /* ��ɫ���� */
    uint8 red;          /* ��ɫ���� */
    uint8 font_type;    /* ����(MSHEAD_FONT_E) */
    uint8 bkmode;       /* ����ģʽ(MSHEAD_OSD_BACKGROUND_MODE_E) */
    char str[0];        /* �Զ����ַ���,������'\0'���� */
} MSHEAD_OSD_STRING_S, *PMSHEAD_OSD_STRING_S;
#define MSHEAD_OSD_STRING_S_LEN sizeof(MSHEAD_OSD_STRING_S)

typedef struct 
{
    uint16 x;           /* x����,������Ϊ��λ */
    uint16 y;           /* y����,������Ϊ��λ */
    uint8 reserved;     /* ���� */
    uint8 blue;         /* ��ɫ���� */
    uint8 green;        /* ��ɫ���� */
    uint8 red;          /* ��ɫ���� */
    uint8 font_type;    /* ����(MSHEAD_FONT_E) */
    uint8 bkmode;       /* ����ģʽ(MSHEAD_OSD_BACKGROUND_MODE_E) */
    uint8 format;       /* ʱ���ʽ(MSHEAD_TIME_FORMAT_E) */
} MSHEAD_OSD_TIME_S, *PMSHEAD_OSD_TIME_S;
#define MSHEAD_OSD_TIME_S_LEN   sizeof(MSHEAD_OSD_TIME_S)

/* ������ */
typedef struct 
{
    uint16 x;           /* x����,������Ϊ��λ */
    uint16 y;           /* y����,������Ϊ��λ */
    uint16 width;       /* �����˵Ŀ�� */
    uint16 height;      /* �����˵ĸ߶� */
    uint8 fill_mode;    /* ���ģʽ,ʹ��STREAM_MOSAIC_FILL_* */
} MSHEAD_MOSAIC_S, *PMSHEAD_MOSAIC_S;
#define MSHEAD_MOSAIC_S_LEN sizeof(MSHEAD_MOSAIC_S)

typedef union
{
    MSHEAD_OSD_STRING_S osd;
    MSHEAD_OSD_TIME_S time; /* ʱ����Ϣ,ʹ��MSHEAD_SEG_VIDEO_S�ṹ���е�timestamp_low,
                             * timestamp_hight��timestamp_millisecond��Ϊʱ����Ϣ */
    MSHEAD_MOSAIC_S mosaic;
} MSHEAD_SEG_DATA_U, *PMSHEAD_SEG_DATA_U;

/* MSHEAD����Ϣ */
typedef struct
{
	uint32 size: 11;        /* �δ�С,���ֽ�Ϊ��λ,ȡֵ��Χ[sizeof(MSHEAD_SEG_S), 2047],
                             * ����������u����չ���ݵĳ���,����չ����Ϊ�ַ���ʱ,����������'\0' */
	uint32 type: 5;         /* ������,ָ��data���ݵ�����,
	                         * ����������MSHEAD_SEG_OSD_STRINGʱ,MSHEAD_OSD_STRING_S��Ч;
                             * ����������MSHEAD_SEG_OSD_TIMEʱ,MSHEAD_OSD_TIME_S��Ч;
                             * ����������MSHEAD_SEG_MOSAICʱ,MSHEAD_MOSAIC_S��Ч;
                             * ����������MSHEAD_SEG_MVBLK_TBLʱ,MSHEAD_MVBLK_S��Ч */
	uint32 reverved: 16;    /* ���� */
    char data[];
} MSHEAD_SEG, *PMSHEAD_SEG;
#define MSHEAD_SEG_LEN  sizeof(MSHEAD_SEG)
HB_ASSERT_SIZEOF_TYPEDEF_STRUCT(MSHEAD_SEG, 4);

/* ��Ƶ֡ͷ��Ϣ */
typedef struct
{
    uint32 video_header_size: 8;        /* ��Ƶ֡ͷ�Ĵ�С,���ֽ�Ϊ��λ,ȡֵ��Χ[sizeof(MSHEAD_SEG_VIDEO_S), 255] */
    uint32 timestamp_millisecond: 10;   /* ��Ƶ֡����ʱ��ĺ���ֵ,ȡֵ��Χ[0, 999];
                                         * ����ͨ��_ftime64��_ftime64_s��������ȡtimestamp_low,timestamp_hight��timestamp_millisecond;
                                         * ����ʹ��timestamp_hight��timestamp_millisecond,����ͨ��time��_time32��������ȡtimestamp_low */
    uint32 width: 14;                   /* ͼ����,��4����Ϊ��λ,ȡֵ��Χ[0, 16383];ʵ��ͼ���ȵ���width * 4 */
    uint32 height: 13;                  /* ͼ��߶�,��4����Ϊ��λ,ȡֵ��Χ[0, 8191];ʵ��ͼ���ȵ���height * 4 */
    uint32 frate_rate: 7;               /* ��Ƶ֡��,�Ժ���(Hz)Ϊ��λ,ȡֵ��Χ[1, 127] */
    uint32 reserved: 12;                /* ���� */
    uint32 fcount;                      /* ֡���;֡������𽥵�����,�����ж���Ƶ֡�������� */
    sint32 tick_count;                  /* ��Ƶ֡���ʱ��,�Ժ���(millisecond)Ϊ��λ;����ʱ,���ڿ���2֮֡���ʱ���� */
    /* ��Ƶ֡����ʱ��,����(second)Ϊ��λ;timestamp_lowΪ64λ�����ĵ�32λ,timestamp_hightΪ64λ�����ĸ�32λ;
     * ������ʱ��(UTC)ʱ�����,�ӹ�Ԫ1970-01-01 00:00:00�𾭹�������;timestamp_low����ʾ����Ԫ2038-01-19 03:14:07,UTC;
     * timestamp_low��timestamp_hight��ͬ����ʾ����Ԫ3000-12-31 23:59:59,UTC */
    uint32 timestamp_low;
    sint32 timestamp_hight;
} MSHEAD_SEG_VIDEO_S, *PMSHEAD_SEG_VIDEO_S;
#define MSHEAD_SEG_VIDEO_S_LEN   sizeof(MSHEAD_SEG_VIDEO_S)
HB_ASSERT_SIZEOF_TYPEDEF_STRUCT(MSHEAD_SEG_VIDEO_S, 24);

/* ��ý���׼ͷ�ṹ����Ϣ(��λ:�ֽ�,��׼��С16�ֽ�) */
typedef struct
{
    uint32 flag;                        /* ֡ͷ��־,������MSHEAD_FLAG */
    uint32 mshsize: 16;                 /* ý��ͷ��Ϣ��С(MAX size MSHEAD_MAXHEADSIZE) */
    uint32 type: 3;                     /* ý��֡����(MSHEAD_STREAM_FRAME_TYPE_E) */
    uint32 algorithm: 5;                /* ��Ƶ����Ƶ�����㷨(MSHEAD_ALGORITHM_VIDEO_TYPE_E��MSHEAD_ALGORITHM_AUDIO_TYPE_E) */
    uint32 segment: 8;                  /* MSHEAD_SEG�ṹ�������,ȡֵ��Χ[0, 255] */
    uint32 checked_encode_data_size: 8; /* ��ҪУ��ı������ݵĴ�С,ȡֵ��Χ[0, 255];һ��ֻУ��������ݵ�ǰ,�󲿷��ֽ�;
                                         * ��checked_encode_data_size==0ʱ,��У���������;����:����У��ǰ���16���ֽڵı������� */
    uint32 checksum: 24;                /* У���,���ܵ���0;����MSHEAD_SEG_VIDEO_S,MSHEAD_SEGMENT[0 : n-1]�ͱ������ݵ�ǰ,�󲿷��ֽ� */
    uint32 msdsize: 24;                 /* �������ݵĴ�С,���ֽ�Ϊ��λ,ȡֵ��Χ[0, MSHEAD_MAXDATASIZE] */
    uint32 max_mshsize: 8;              /* MSHEAD��󳤶�֧��(Ҫ��MSHEAD_MAX_MSHSIZE_ALIGN_SIZE�ֽڶ���) */
} MSHEAD, *PMSHEAD;
#define MSHEAD_LEN  sizeof(MSHEAD)
HB_ASSERT_SIZEOF_TYPEDEF_STRUCT(MSHEAD, 16);

/******************************************************************************
 * ��������: ������ͷ,�˴���Ϊ�������max_mshsize�ֽ��ڴ�,
 *           ���а���MSHEAD�ṹ�峤��,���Ⱥ����ڲ����Զ�MSHEAD_HEAD_ALIGN_SIZE�ֽڶ���
 * �������: algorithm: �����׼
 *           max_mshsize: ý��ͷ��Ϣ��󳤶�,��С����ܳ���MSHEAD_MAXHEADSIZE
 *           checked_encode_data_size: ��ҪУ��ı������ݵĴ�С,ȡֵ��Χ[0, 255];һ��ֻУ��������ݵ�ǰ,�󲿷��ֽ�;
 *                                     ��checked_encode_data_size==0ʱ,��У���������;����:����У��ǰ���16���ֽڵı�������
 * �������: ��
 * ����ֵ  : <0-����,>0-�ɹ�,��ʾ�����ľ��
 *****************************************************************************/
sint32 mshead_open(sint32 algorithm, sint32 max_mshsize, uint8 checked_encode_data_size);

/******************************************************************************
 * ��������: �����Ѵ��ڵ��ڴ��ַ�����ϴ���ý��ͷ��Ϣ,
 * �������: algorithm: �����׼
 *           pmshead: ý��ͷ��Ϣ�ṹ��ָ��
 *           max_mshsize: ý��ͷ��Ϣ��󳤶�,����Ҫ��MSHEAD_HEAD_ALIGN_SIZE�ֽڶ���,
 *                        ��С��С����С��MSHEAD_LEN,���ȳ���MSHEAD_MAXHEADSIZE���ֽ����ض�
 *           checked_encode_data_size: ��ҪУ��ı������ݵĴ�С,ȡֵ��Χ[0, 255];һ��ֻУ��������ݵ�ǰ,�󲿷��ֽ�;
 *                                     ��checked_encode_data_size==0ʱ,��У���������;����:����У��ǰ���16���ֽڵı�������
 * �������: ��
 * ����ֵ  : <0-����,>0-�ɹ�,��ʾ�����ľ��
 *****************************************************************************/
sint32 mshead_openext(sint32 algorithm, PMSHEAD pmshead, sint32 max_mshsize, uint8 checked_encode_data_size);

/******************************************************************************
 * ��������: д��֡��Ϣ����,�������ݲ����п���
 * �������: handle: MSHEAD���
 *           frame_type: ֡����(MSHEAD_STREAM_FRAME_TYPE_E)
 *           en_checksum: �Ƿ���Ҫ����У���(ע��:��û����������Ϣʱ,���鿪��ʹ��;�����ʹ��,
 *                        ��Ҫ�������ýӿ�mshead_update_checksum����,����ǰ��������е�SEG����Ϣд��)
 *           data: ����Ƶ���ݻ�����(ʵ��δʹ��)
 *           size: ����Ƶ���ݳ���
 *           sec: ֻ����Ƶ��Ч,��ǰ֡ʱ�������,��ʾ�ӹ�Ԫ1970��1��1��0ʱ0��0������,
 *                �����UTCʱ��������������(ע��: ����ֵΪ0ʱ,ʱ������ڲ�����)
 *           msec: ֻ����Ƶ��Ч,��ǰ֡ʱ��(��λ: ����)
 *           width: ֻ����Ƶ��Ч,���ʾ���(��λ1����)
 *           height: ֻ����Ƶ��Ч,���ʾ�߶�(��λ1����)
 *           frame_rate: ��Ƶ֡��,ֻ����Ƶ֡����Ч,���ֵΪMSHEAD_MAX_VIDEO_FRAME_RATE
 * �������: ��
 * ����ֵ  : <0-����,>0-�ɹ�,��ʾд������ݳ���
 *  (ע��: ���Ȳ���MSHEAD_HEAD_ALIGN_SIZE�ֽڶ��뽫�Զ��ӳ������,H265������4�ֽڶ���)
 *****************************************************************************/
sint32 mshead_writeext(sint32 handle, MSHEAD_STREAM_FRAME_TYPE_E frame_type, HB_BOOL en_checksum, 
    sint8 *data, uint32 size, uint32 sec, uint32 msec, uint32 width, uint32 height, uint32 frame_rate);

/******************************************************************************
 * ��������: д��֡��Ϣ����,�������ݲ����п���
 * �������: handle: MSHEAD���
 *           frame_type: ֡���Ͷ�����Ƶ,������Ƶ��Ч
 *           en_checksum: �Ƿ���Ҫ����У���(ע��:��û����������Ϣʱ,���鿪��ʹ��;�����ʹ��,
 *                        ��Ҫ�������ýӿ�mshead_update_checksum����,����ǰ��������е�SEG����Ϣд��)
 *           data: ����Ƶ���ݻ�����,ʵ��δʹ��
 *           size: ����Ƶ���ݳ���
 *           width: ֻ����Ƶ��Ч,���ʾ���(��λ1����)
 *           height: ֻ����Ƶ��Ч,���ʾ�߶�(��λ1����)
 *           frame_rate: ��Ƶ֡��,ֻ����Ƶ֡����Ч,���ֵΪMSHEAD_MAX_VIDEO_FRAME_RATE
 * �������: ��
 * ����ֵ  : <0-����,>0-�ɹ�,��ʾд������ݳ���
 *  (ע��: ���Ȳ���MSHEAD_HEAD_ALIGN_SIZE�ֽڶ��뽫�Զ��ӳ������,H265������4�ֽڶ���)
 *****************************************************************************/
sint32 mshead_write(sint32 handle, MSHEAD_STREAM_FRAME_TYPE_E frame_type, HB_BOOL en_checksum, 
    sint8 *data, uint32 size, uint32 width, uint32 height, uint32 frame_rate);

/******************************************************************************
 * ��������: ��ȡ����Ƶ����,��ȡ����ָ��,�Լ�����
 * �������: handle: MSHEAD���
 * �������: data: ����Ƶ���ݻ�������ַָ��
 *           size: ����Ƶ���ݳ���ָ��
 * ����ֵ  : <0-����,>0-�ɹ�,��������Ƶ���ݻ�����ָ��
 *****************************************************************************/
sint32 mshead_read(sint32 handle, sint8 **data, uint32 *size);

/******************************************************************************
 * ��������: д�����Ϣ����MSHEAD_SEG
 * �������: handle: MSHEAD���
 *           seg: ��ͷ��Ϣ,��size,data�����������û�����
 *           data: �û�����
 *           size: �û����ݳ���,��Ϊ�ַ���Ӧ����β��'\0'�ĳ���
 * �������: ��
 * ����ֵ  : <0-����,>0-�ɹ�,��ʾ��ǰ��ͷ����(ע��: ���Ȳ���MSHEAD_HEAD_ALIGN_SIZE�ֽڶ��뽫�Զ��ӳ������)
 *****************************************************************************/
sint32 mshead_write_seg(sint32 handle, MSHEAD_SEG seg, sint8 *data, uint32 size);

/******************************************************************************
 * ��������: ����֡��Ϣ������У��ֵ(��������е�SEG����Ϣд�����ܸ���)
 * �������: handle: MSHEAD���
 *           data: ����Ƶ���ݻ�����(ʵ��δʹ��)
 *           size: ����Ƶ���ݳ���
 * �������: ��
 * ����ֵ  : 0-�ɹ�,<0-�������
 *****************************************************************************/
sint32 mshead_update_checksum(sint32 handle, sint8 *data, uint32 size);

/******************************************************************************
 * ��������: ��ȡ��ͷ��Ϣ�е�һ������Ϣ
 * �������: handle: MSHEAD���
 *           index: MSHEAD_SEG���������
 * ����ֵ: <0-ʧ��,0-������޿�����Ϣ,>0-��ָ���ȡ�Ķ���Ϣָ��PMSHEAD_SEG
 *****************************************************************************/
PMSHEAD_SEG mshead_read_seg(sint32 handle, uint32 index);

/******************************************************************************
 * ��������: ����һ֡���ݵ�У���
 * �������: handle: MSHEAD���
 * �������: data: ����Ƶ���ݻ�������ַָ��
 *           size: ����Ƶ���ݳ���ָ��
 * ����ֵ: <=0-ʧ��,>0-���ݵ�У���
 *****************************************************************************/
sint32 mshead_generate_checksum(sint32 handle, sint8 *data, uint32 size);

/******************************************************************************
 * ��������: MSHEAD����
 * �������: handle: MSHEAD���
 *           cmd: ����
 *           channel: ͨ����,�˴���Ч
 *           param: �������
 *           param_len: param����,�ر����GET����ʱ,�������Ӧ���жϻ������Ƿ��㹻
 * �������: param: �������
 * ����ֵ  : 0-�ɹ�,<0-�������
 *****************************************************************************/
sint32 mshead_ioctrl(sint32 handle, sint32 cmd, sint32 channel, sint32 *param, sint32 param_len);

/******************************************************************************
 * ��������: �ر�MSHEAD,�ú�������������������,���ͷ������Դ
 * �������: handle: MSHEAD���;
 * �������: ��
 * ����ֵ  : 0-�ɹ�,<0-�������
 *****************************************************************************/
sint32 mshead_close(sint32 handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _MSHEAD_H_ */
