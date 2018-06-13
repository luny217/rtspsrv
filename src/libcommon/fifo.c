/******************************************************************************
 * 模块名称: FIFO-实现管道缓冲机制,提供各模块之间数据通讯使用
 * 修改记录:
 *  1. 2008-12-31 V1.0.1
 *  1). 统一系统错误号和CMD值的定义规范
 *  2). 统一除xxx_open()之外所有函数接口的定义规范
 *  2. 2009-02-24 V1.1.1
 *  1). 增加FIFO的锁定和解锁操作
 *  3. 2009-04-03 V1.1.2
 *  1). 修正fifo_readext()函数陷入死循环问题
 *  4. 2009-07-15 V1.1.3
 *  1). 增加fifo_frameprocess(),fifo_datprocess()两个函数,减少内存拷贝,优化CPU
 *  5. 2009-07-29 V1.1.4
 *  1). 修正fifo_frameprocess(),fifo_datprocess()两个函数,内存越界问题引起崩溃
 *  2). 增加FIFO_CMD_GETVFRAMECOUNT接口,用于获取FIFO中视频帧数
 *  6. 2009-07-30 V1.1.5
 *  1). 修正fifo_frameprocess(),fifo_datprocess()两个函数,内存越界问题引起崩溃
 *  7. 2009-09-03 V1.1.6
 *  1). 修正fifo_frameprocess(),fifo_datprocess()两个函数,内存拼接引起花屏问题
 *  8. 2009-09-03 V1.1.7
 *  1). 修正预留BUF空间为128K,解决内存拼接BUF不够引起花屏问题
 *  9. 2010-04-23 V1.1.8
 *  1). 重新整理,去警告信息(理学伟)
 * 10. 2010-05-17 V1.1.9
 *  1). 修改fifo_ioctrl返回值有误的问题
 * 11. 2011-12-30 V2.0.1
 *  1). 修改相关函数参数定义类型
 *  2). 添加fifo_openext接口
 *  3). 修改函数fifo_readext,fifo_frameprocess和fifo_datprocess内部处理机制
 * 12. 2012-01-07 V2.0.2
 *  1). 添加函数fifo_frameprocess和fifo_datprocess内部数据丢弃处理机制
 * 13. 2012-01-13 V2.0.3
 *  1). 添加函数fifo_ioctrl命令FIFO_CMD_SETHIWATER和FIFO_CMD_SETLOWATER参数合法性校验
 *  2). 添加函数fifo_ioctrl命令FIFO_CMD_GETLINKSTAT支持
 *  3). 添加函数fifo_frameprocess和fifo_datprocess对数据拼接由外部实现,
 *      通过函数fifo_ioctrl命令FIFO_CMD_GETLINKSTAT和fifo_readframe组合来完成
 *  4). 增强函数fifo_readext容错机制
 * 14. 2012-08-13 V2.0.4
 *  1). 添加MEM_MANAGE内存管理模块对内存管理
 * 15. 2012-08-22 V2.0.5
 *  1). 优化函数fifo_read和fifo_write容错,添加TRACE_ASSERT机制
 * 16. 2012-08-29 V2.0.6
 *  1). 修改TRACE_ASSERT机制为记录FLASH日志(/mnt/mtd/log/fifo_err.log)
 * 17. 2012-09-06 V2.0.7
 *  1). 优化模块内部函数_fifo_dataprocess,加强容错处理
 * 18. 2012-10-30 V2.0.8
 *  1). 优化模块函数fifo_readext,加强容错处理
 * 19. 2012-11-22 V2.1.0
 *  1). FLASH日志信息添加时间戳信息
 *  2). 添加接口fifo_read_newfull_keyframe支持
 * 20. 2012-11-25 V2.1.1
 *  1). 修改模块内部函数_fifo_dataprocess,当时FIFO的resv_size为0且数据要掉头时,回调处理的数据长度过大的问题
 * 21. 2012-12-10 V2.1.2
 *  1). 去掉无用打印信息
 * 22. 2012-12-21 V2.1.3
 *  1). 修改模块内部函数_fifo_dataprocess,当时FIFO的resv_size为0且数据要掉头时,关键帧计数的处理问题
 * 23. 2013-03-21 V2.2.0
 *  1). 修改模块接口fifo_read_newfull_keyframe,fifo_readframe和fifo_readext,加强容错处理
 *  2). 修改模块接口fifo_write和fifo_writeext,对音视频数据进行校验,加强容错处理
 * 24. 2013-05-30 V3.0.0
 *  1). 添加多消费者机制,添加接口fifo_openmulti,fifo_readmulti,fifo_readframemulti,
 *      fifo_readextmulti,fifo_frameprocessmulti,fifo_datprocessmulti和fifo_closemulti支持
 *  2). 添加接口fifo_ioctrl命令FIFO_CMD_GETFREESIZESTAT,FIFO_CMD_GETHTWATER,FIFO_CMD_SETHTWATER,
 *      FIFO_CMD_CONSUMERTRYLOCK,FIFO_CMD_CONSUMERLOCK和FIFO_CMD_CONSUMERUNLOCK支持
 * 25. 2013-06-18 V3.1.0
 *  1). 修改枚举体FIFO_CUSTOMER_E为FIFO_CONSUMER_E
 *  2). 修改fifo_ioctrl命令FIFO_CMD_CUSTOMERTRYLOCK,FIFO_CMD_CUSTOMERLOCK和FIFO_CMD_CUSTOMERUNLOCK为
 *      FIFO_CMD_CONSUMERTRYLOCK,FIFO_CMD_CONSUMERLOCK和FIFO_CMD_CONSUMERUNLOCK支持
 * 26. 2013-06-19 V3.1.1
 *  1). 修改接口fifo_closemulti关闭FIFO其中一个消费者而误关闭整个FIFO的问题
 *  2). 修改接口fifo_write写入音视频数据时,对音视频MSHEAD信息的处理错误的问题
 *  3). 修改接口fifo_readmulti,fifo_readframemulti,fifo_readextmulti,
 *      fifo_frameprocessmulti和fifo_datprocessmulti多消费者数据处理错误的问题
 * 27. 2013-06-20 V3.1.2
 *  1). 修改模块内部函数_fifo_dataprocess,当MSHEAD头信息不完整时处理错误的问题
 * 28. 2013-06-28 V3.2.0
 *  1). 修改接口fifo_ioctrl命令FIFO_CMD_GETFREESIZESTAT支持
 *  2). 添加接口fifo_ioctrl命令FIFO_CMD_GETHTWATERSTAT,FIFO_CMD_GETHIWATERSTAT和FIFO_CMD_GETLOWATERSTAT支持
 * 29. 2013-07-04 V3.2.1
 *  1). 添加接口fifo_ioctrl命令FIFO_CMD_CONSUMERSTAT支持
 * 30. 2013-08-27 V3.3.0
 *  1). 去掉fifo_ioctrl命令FIFO_CMD_CONSUMERTRYLOCK,FIFO_CMD_CONSUMERLOCK和FIFO_CMD_CONSUMERUNLOCK支持
 *  2). 内部修改单生产者和多消费者情况下,由于消费者过慢,生产者清除数据时,会引起数据错位的问题
 * 31. 2013-09-27 V3.3.1
 *  1). 修改函数fifo_open,fifo_openext和fifo_openmulti的初始化问题造成打开失败的问题
 * 32. 2013-10-11 V3.4.0
 *  1). 修改函数_fifo_readext处理数据出错的问题
 * 33. 2013-10-12 V3.4.1
 *  1). 修改函数_fifo_dataprocess处理数据视频帧计数出错的问题
 * 34. 2013-11-06 V3.5.0
 *  1). 添加函数fifo_dataprocessext和fifo_datprocessmultiext支持多关键帧处理机制
 * 35. 2013-12-31 V3.5.1
 *  1). 修改函数fifo_closemulti内部机制,不会清楚fifo数据和释放相关资源
 * 36. 2014-09-23 V4.0.0
 *  1). 增强函数_fifo_dataprocess处理数据容错性
 *  2). 添加函数fifo_dump_info,fifo_dump_data和fifo_relocate_mshead支持
 * 37. 2014-11-25 V4.1.0
 *  1). 修改FIFO内部机制,对关键帧定位进行优化和修改,如果有关键帧将定位到关键帧位置
 *****************************************************************************/
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef _WIN32
#include <WinSock2.h>
#include <windows.h>
#define pthread_mutex_t CRITICAL_SECTION
#else
#include <pthread.h>
//#include "mem_manage.h"
#endif /* _WIN32 */
#include <common.h>
#include <mshead.h>
#include <fifo.h>

#define COMP_NENT           COMP_FIFO
#define COMP_FLAG           0x4F464946 /* "FIFO" */
#define FLAG_VERIFY(fd)     (COMP_FLAG == (fd)->flag)

#define FIFO_DATA_SIZE_MORE     (128 << 10)
#define FIFO_RESV_SIZE_DEF      (128 << 10)
#define FIFO_RESV_SIZE_INVAL    0xFFFFFFFF

#define FIFO_CONSUMER_DNUM      1
#define FIFO_HTWATER_DEF        100
#define FIFO_DEAL_IFRM_NUMS     0

#define FIFO_DISABLE_RESET_FILE_NAME    "/mnt/mtd/dis_fifo_reset"

#define FIFO_PARAM_VERIFY(p, l, s)          (NULL == (p)) || ((l) < (s))
#define FIFO_CONSUMER_VERIFY(mi, cm, mx)    (((cm) < (mi)) || ((cm) >= (mx)))

#define FIFO_CALC_SIZE(oft, add, tsz)   (((oft) + (add)) & ((tsz) - 1))

#define FIFO_CONSUMER_STAT(fd, cm)  ((fd)->consumer_stat & BIT64(cm))
#define FIFO_IS_FULL(fd, cm)        ((fd)->pcm[(cm)].ci == (((fd)->pi + 1) & ((fd)->size - 1)))
#define FIFO_IS_EMPTY(fd, cm)       ((fd)->pcm[(cm)].ci == (fd)->pi)
#define FIFO_DATA_SIZE(fd, cm)      (((fd)->pi - (fd)->pcm[(cm)].ci) & ((fd)->size - 1))
#define FIFO_FREE_SIZE(fd, cm)      (((fd)->pcm[(cm)].ci - (fd)->pi - 1) & ((fd)->size - 1))
#define FIFO_WRITE_UPDATE(fd, sz)   ((fd)->pi = ((fd)->pi + sz) & ((fd)->size - 1))
#define FIFO_WRITE(fd, data, sz)    \
    sint32 len = 0; \
    if ((len = (fd)->size - (fd)->pi) < sz) \
        memcpy((fd)->space + (fd)->pi, data, len), memcpy((fd)->space, data + len, sz - len); \
    else \
        memcpy((fd)->space + (fd)->pi, data, sz); \
    FIFO_WRITE_UPDATE(fd, sz)
#define FIFO_READ_UPDATE(fd, cm, sz)    ((fd)->pcm[(cm)].ci = (fd)->pcm[(cm)].cci = ((fd)->pcm[(cm)].cci + sz) & ((fd)->size - 1))
#define FIFO_READ(fd, cm, data, sz, peek)   \
    if (data) { \
        sint32 len = (fd)->size - (fd)->pcm[(cm)].cci; \
        if (len < sz) \
            memcpy(data, (fd)->space + (fd)->pcm[(cm)].cci, len), memcpy(data + len, (fd)->space, sz - len); \
        else \
            memcpy(data, (fd)->space + (fd)->pcm[(cm)].cci, sz); \
    } \
    if (FIFO_PEEK_YES == peek) \
        FIFO_READ_UPDATE(fd, cm, sz)

//#define FIFO_DUMP_DEBUG
#ifdef FIFO_DUMP_DEBUG
#define FIFO_DUMP_CHECK_TIMECOUNT(fd, cm)   \
    (((fd)->pcm[(cm)].timecount_pi - (fd)->pcm[(cm)].timecount_ci) != (fd)->pcm[(cm)].timecount)
#define FIFO_DUMP_CHECK_FRAMECOUNT(fd, cm)  \
    (((fd)->pcm[(cm)].vframe_count_pi - (fd)->pcm[(cm)].vframe_count_ci) != (fd)->pcm[(cm)].vframe_count)
#endif /* FIFO_DUMP_DEBUG */

#define FIFO_KEYFRM_OVERLAY_SAFETY_PERCENT  10
#define FIFO_CHECK_KEYFRM_OVERLAY(fd, posi) \
    ((((posi) - (fd)->pi - 1) & ((fd)->size - 1)) < ((fd)->size * FIFO_KEYFRM_OVERLAY_SAFETY_PERCENT / 100))

typedef enum
{
    FIFO_DATAPROCESS_ONE_VIDEO_OR_AUDIO_FRAME,      /* 处理单帧视频或音频数据 */
    FIFO_DATAPROCESS_ONE_VIDEO_MULTY_AUDIO_FRAME,   /* 处理单帧视频和多帧音频数据 */
    FIFO_DATAPROCESS_MULTY_VIDEO_MULTY_AUDIO_FRAME, /* 处理多帧音视频数据 */
} FIFO_DATAPROCESS_E;

typedef enum
{
    FIFO_FRAME_FULL,    /* 帧已完整 */
    FIFO_FRAME_PART,    /* 帧不完整 */
} FIFO_FRAME_STAT_E;

/* 关键帧状态(按位表示,共8位) */
typedef enum
{
    FIFO_KEYFRM_NOT_EXIST   = 0x00, /* 关键帧不存在存在 */
    FIFO_KEYFRM_EXIST       = 0x01, /* 关键帧已存在 */
    FIFO_KEYFRM_OVERLAY     = 0x02, /* 关键帧已覆盖 */
} FIFO_KEYFRM_STAT_E;

typedef struct
{
    uint8 opened;       /* 当前进程是否已打开过此模块 */
    uint8 reserved[3];  /* 保留位 */
} FIFO_PARAM_S, *PFIFO_PARAM_S;
#define FIFO_PARAM_S_LEN    sizeof(FIFO_PARAM_S)

typedef struct
{
    uint32 htwater;         /* 最高水位值,hiwater<=htwater<=size */
    uint32 hiwater;         /* 高水位值,lowwater<=hiwater<=htwater */
    uint32 lowater;         /* 低水位值,0<=lowwater<=hiwater */
    uint32 ci, cci;         /* 生产者和消费者共同拥有的消费者内存偏移位置(默认值为0),消费者所拥有的消费者内存偏移位置(默认值为0) */
    uint8 link_stat;        /* 是否需要数据拼接的状态(FIFO_LINKSTAT_E) */
    uint8 reserved[3];      /* 保留位 */
    sint32 timecount;       /* 缓冲数据的时间,特为音视频数据设计,编译计算缓冲时间,以I帧个数计算 */
    sint32 vframe_count;    /* 视频帧数,记录当前FIFO中视频帧总数 */
#ifdef FIFO_DUMP_DEBUG
    sint32 timecount_pi;    /* 缓冲数据的时间,特为音视频数据设计,编译计算缓冲时间,以I帧个数计算 */
    sint32 timecount_ci;    /* 缓冲数据的时间,特为音视频数据设计,编译计算缓冲时间,以I帧个数计算 */
    sint32 vframe_count_pi; /* 视频帧数,记录当前FIFO中视频帧总数 */
    sint32 vframe_count_ci; /* 视频帧数,记录当前FIFO中视频帧总数 */
#endif /* FIFO_DUMP_DEBUG */
    pthread_mutex_t mtx_ci; /* 用于访问保护 */
} FIFO_CONSUMER_INFO_S, *PFIFO_CONSUMER_INFO_S;
#define FIFO_CONSUMER_INFO_S_LEN    sizeof(FIFO_CONSUMER_INFO_S)

#define FIFO_DBG_ERR_COUNT 8
/* FIFO句柄结构体信息 */
typedef struct
{
    uint32 flag, keyflag;           /* 用于合法性检测,关键帧标志 */
    sint32 maxch;                   /* 最大通道数 */
    uint32 size, resv_size, pi;     /* 缓冲区大小;缓冲尾部预留空间,用于不完整数据拼接;生产者内存偏移位置(默认值为0) */
    uint32 full_keyfrm_posi;        /* 最新完整关键帧的位置 */
    uint32 full_vframe_posi;        /* 最新完整视频帧的位置 */
    uint32 part_allfrm_posi;        /* 最新不完整帧的位置,大小还不完整 */
    uint32 part_allfrm_tlsz;        /* 最新不完整帧的大小,大小还不完整,帧总大小 */
    uint32 part_allfrm_wtsz;        /* 最新不完整帧的大小,大小还不完整,当前大小 */
    uint8 dbg_err_count;            /* 用于写头和数据错误时,控制打印次数 */
    uint8 reserved[3];              /* 保留位 */
    uint16 full_vframe_count;       /* 最新完整关键帧的位置开始视频帧数 */
    uint8 full_keyfrm_stat;         /* 最新完整关帧是否完整(FIFO_KEYFRM_STAT_E) */
    uint8 part_allfrm_stat;         /* 最新不完整帧是否完整(FIFO_FRAME_STAT_E) */
    uint64 consumer_stat;           /* 是否打开了相应的消费者按位表示(0-未打开,1-已打开) */
    pthread_mutex_t mutex;          /* 用于访问保护 */
    PFIFO_CONSUMER_INFO_S pcm;      /* FIFO消费信息结构体指针 */
    char *space;                    /* FIFO缓冲区指针 */
} FIFO_FD_S, *PFIFO_FD_S;
#define FIFO_FD_S_LEN   sizeof(FIFO_FD_S)

//#define ENABLE_FIFO_FILELOG
#ifdef ENABLE_FIFO_FILELOG
#include "flash_log.h"

#define FIFO_LOG_FILE_SIZE      (4 << 10)   /* 4KB */
#define FIFO_LOG_FILE_NAME      "fifo_err.log"
#define FIFO_LOG_FILE_FD_INVAL  (-1)

static sint32 fifo_log_file_hdl = FIFO_LOG_FILE_FD_INVAL;

#define TRACE_LOG(str, args...) flash_log_fwrite(fifo_log_file_hdl, \
    FLOG_LVL_ERROR, "%s#%d>"str, __FILE__, __LINE__, ## args)
#endif /* ENABLE_FIFO_FILELOG */

static FIFO_PARAM_S fifo_param = {0};
static SWVERSION swver = {0, 1, 4, 25, 11, 14}; /* 模块版本定义 */

#if 0 //ndef _WIN32
static sint32 _fifo_dump_cli(char *args);
static CLI_S gpa_fifo_cli_tbl[] = 
{
    {"dfifo", _fifo_dump_cli, "dump fifo stats info, -f [fifo_hdl] -p [dump file] -s [dump size]", 0},
    {NULL, NULL, NULL, 0} /* 必须加,END FLAG */
};
static MODULE_LIST_S pfifo_cli[] = {
    {COMP_FIFO, "FIFO", gpa_fifo_cli_tbl, PRN_LVL_ERR, PRN_SHOW_LVL_LEQ},
    {0, NULL, NULL, 0, 0}};
#endif /* _WIN32 */

#if 0 //def _WIN32
static int pthread_mutex_init(pthread_mutex_t *__mutex, const void *__mutexattr)
{
    InitializeCriticalSection(__mutex);

    return 0;
}

static int pthread_mutex_lock(pthread_mutex_t *__mutex)
{
    EnterCriticalSection(__mutex);

    return 0;
}

static int pthread_mutex_unlock(pthread_mutex_t *__mutex)
{
    LeaveCriticalSection(__mutex);

    return 0;
}

static int pthread_mutex_destroy(pthread_mutex_t *__mutex)
{
    DeleteCriticalSection(__mutex);

    return 0;
}
#endif /* _WIN32 */

static sint32 _fifo_minfreesize(PFIFO_FD_S fd)
{
    sint32 free_size = fd->size, i = 0;

    for (i = 0; i < fd->maxch; i++)
    {
        if (FIFO_CONSUMER_STAT(fd, i))
        {
            free_size = IMIN(free_size, FIFO_FREE_SIZE(fd, i));
        }
    }

    return free_size;
}

static sint32 _fifo_maxdatasize(PFIFO_FD_S fd)
{
    sint32 data_size = 0, i = 0;

    for (i = 0; i < fd->maxch; i++)
    {
        if (FIFO_CONSUMER_STAT(fd, i))
        {
            data_size = IMAX(data_size, FIFO_DATA_SIZE(fd, i));
        }
    }

    return data_size;
}

static sint32 _fifo_calc_posi(PFIFO_FD_S fd, uint32 posi, uint32 size)
{
    return ((posi + size) & (fd->size - 1));
}

static sint32 _fifo_check_posi(PFIFO_FD_S fd, uint32 posi, uint32 size)
{
    return ((fd->size - posi) < size);
}

static sint32 _fifo_read_posi(PFIFO_FD_S fd, uint32 posi, char *data, uint32 size)
{
    sint32 len = fd->size - posi;
    
    if (len < size)
    {
        memcpy(data, fd->space + posi, len), memcpy(data + len, fd->space, size - len);
    }
    else
    {
        memcpy(data, fd->space + posi, size);
    }

    return size;
}

static sint32 _fifo_read_buffer(PFIFO_BUFF_INFO_S pbuff_info, char *data, uint32 size)
{
    if (size > pbuff_info->buff[0].size)
    {
        memcpy(data, pbuff_info->buff[0].pbuff, pbuff_info->buff[0].size), \
            memcpy(data + pbuff_info->buff[0].size, pbuff_info->buff[1].pbuff, size - pbuff_info->buff[0].size);
    }
    else
    {
        memcpy(data, pbuff_info->buff[0].pbuff, size);
    }

    return size;
}

#if 1// ndef _WIN32
static sint32 _fifo_find_mshead(char *pdate, uint32 offset, uint32 limit)
{
    uint32 *pu32_data = (uint32 *)pdate;
    
    offset = offset >> 2, limit = limit >> 2;
    TRACE_DBG("offset(0x%x) limit(0x%x)\n", offset, limit);
    for (; offset < limit; offset++)
    {
        if (ISMSHEAD(&pu32_data[offset]))
        {
            TRACE_DBG("Found MSHEAD(0x%x)\n", offset << 2);
            return offset << 2;
        }
    }
    TRACE_ERR("Not find MSHEAD\n");
    
    return ERRNO(FIFO_ERROR_MSHEAD, COMP_NENT);
}

static void _fifo_framedeal(PFIFO_FD_S fd, PMSHEAD pmshead, uint32 frame_posi)
{
    if (ISVIDEOFRAME(pmshead))
    {            
        sint32 i = 0;
        
        for (i = 0; i < fd->maxch; i++)
        {
            if (FIFO_CONSUMER_STAT(fd, i))
            {
                INCREMENT_ATOMIC(fd->pcm[i].vframe_count, 1); /* 视频帧,主要用于视频帧计数 */
            #ifdef FIFO_DUMP_DEBUG
                INCREMENT_ATOMIC(fd->pcm[i].vframe_count_pi, 1); /* 视频帧,主要用于视频帧计数 */
            #endif /* FIFO_DUMP_DEBUG */
                if (ISKEYFRAME(pmshead))
                {
                    INCREMENT_ATOMIC(fd->pcm[i].timecount, 1); /* 视频关键帧,主要用于关键帧计数 */
                #ifdef FIFO_DUMP_DEBUG
                    INCREMENT_ATOMIC(fd->pcm[i].timecount_pi, 1); /* 视频关键帧,主要用于关键帧计数 */
                #endif /* FIFO_DUMP_DEBUG */
                    TRACE_REL("handle(0x%x)[%d] fcount(%d) time_sec(%d) timecount(%d)\n", 
                        (sint32)fd, i, pmshead->fcount, pmshead->time_sec, fd->pcm[i].timecount);
                }
            #ifdef FIFO_DUMP_DEBUG
                TRACE_ASSERT(FIFO_DUMP_CHECK_TIMECOUNT(fd, i), "timecount_pi(%d) timecount_ci(%d) timecount(%d)", 
                    fd->pcm[i].timecount_pi, fd->pcm[i].timecount_ci, fd->pcm[i].timecount);
                TRACE_ASSERT(FIFO_DUMP_CHECK_FRAMECOUNT(fd, i), "vframe_count_pi(%d) timecount_ci(%d) timecount(%d)", 
                    fd->pcm[i].vframe_count_pi, fd->pcm[i].vframe_count_ci, fd->pcm[i].vframe_count);
            #endif /* FIFO_DUMP_DEBUG */
            }
        }
        fd->full_vframe_posi = frame_posi;
        if (ISKEYFRAME(pmshead))
        {
            fd->full_vframe_count = 1;
            fd->full_keyfrm_posi = frame_posi; /* 视频关键帧,主要用于记录最近完整关键帧位置 */
            fd->full_keyfrm_stat = FIFO_KEYFRM_EXIST;
        }
        else
        {
            INCREMENT_ATOMIC(fd->full_vframe_count, 1);
        }
        if (FIFO_CHECK_KEYFRM_OVERLAY(fd, fd->full_keyfrm_posi) > 0)
        {
            fd->full_keyfrm_stat |= FIFO_KEYFRM_OVERLAY;
        }
    }
}
#endif /* _WIN32 */

/******************************************************************************
 * 函数介绍: 根据不同的消费者创建FIFO(每次只能创建一个消费者)
 * 输入参数: handle: FIFO句柄,首次创建时,句柄需要<=0,其他-已创建的FIFO句柄
 *           consumer: 消费者序号,每个消费者只能创建一次;
 *                     首次创建时,表示需要创建的最大消费者个数;
 *                     非首次创建时,FIFO_CONSUMER_NUL表示将返回一个空闲的消费者
 *           size: FIFO大小,单位bytes,为优化运算,size必须等于1<<N(首次创建时有效,其他时无效)
 *           htwater: 最高警戒位大小(单位为%,hiwater~100)
 *           hiwater: 高警戒位大小(单位为%,lowater~htwater)
 *           lowater: 低警戒位大小(单位为%,0~hiwater)
 *           resv_size: 保留FIFO大小,单位bytes(首次创建时有效,其他时无效)
 *                      (为优化运算,resv_size必须等于1<<N,主要用于音视频数据不连续数据拼接,
 *                      请根据音视频最大帧长度设置;如果大小为0时,拼接情况下,将由外部提前申请
 *                      内存好内存,通过fifo_ioctrl的fifo_ioctrl命令FIFO_CMD_GETLINKSTAT查询后,
 *                      通过fifo_readframe读出)
 * 输出参数: 无
 * 返回值  : <0-失败,>0-FIFO句柄
 *****************************************************************************/
sint32 _fifo_open(sint32 handle, FIFO_CONSUMER_E consumer, uint32 size, 
    uint32 htwater, uint32 hiwater, uint32 lowater, uint32 resv_size)
{
    PFIFO_PARAM_S pfifo_param = &fifo_param;
    PFIFO_FD_S fd = (PFIFO_FD_S)handle;
    sint32 i = 0;

    if ((lowater > hiwater) || (hiwater > htwater) || (htwater > 100))
    {
        TRACE_ERR("unsupported lowater(%d) or hiwater(%d) or htwater(%d)\n", lowater, hiwater, htwater);
        return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
    }
    
    if ((handle > 0) && FLAG_VERIFY(fd))
    {
        pthread_mutex_lock(&fd->mutex);
        if (FIFO_CONSUMER_NUL == consumer)
        {
            for (i = 0; i < fd->maxch; i++)
            {
                if (0 == FIFO_CONSUMER_STAT(fd, i))
                {
                    consumer = i;
                    TRACE_DBG("handle(0x%x)[%d] unopened\n", handle, consumer);
                    goto initd;
                }
            }
            pthread_mutex_unlock(&fd->mutex);
            TRACE_ERR("handle(0x%x) reach maxch(%d) consumer_stat(0x%llx)\n", handle, fd->maxch, fd->consumer_stat);
            return ERRNO(FIFO_ERROR_REACHMAXCH, COMP_NENT);
        }
        else
        {
            if (FIFO_CONSUMER_VERIFY(0, consumer, fd->maxch))
            {
                pthread_mutex_unlock(&fd->mutex);
                TRACE_ERR("unsupported handle(0x%x) consumer(%d) maxch(%d)\n", handle, consumer, fd->maxch);  
                return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
            }
            if (FIFO_CONSUMER_STAT(fd, consumer))
            {
                pthread_mutex_unlock(&fd->mutex);
                TRACE_ERR("handle(0x%x) consumer_stat(0x%llx) consumer(%d) opened\n", handle, fd->consumer_stat, consumer);
                return ERRNO(COMMON_ERROR_HANDLEOPEND, COMP_NENT);
            }
            goto initd;
        }
    }
    
    for (i = 0; i < (sizeof(size) << 3); i++)
    {
        if (BIT32(i) == size)
        {
            goto resv;
        }
    }
    TRACE_ERR("unsupported size, 0x%x\n", size);
    return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);

resv:
    if (resv_size)
    {
        for (i = 0; i < (sizeof(resv_size) << 3); i++)
        {
            if (BIT32(i) == resv_size)
            {
                goto created;
            }
        }
    }
    else
    {
        goto created;
    }
    TRACE_ERR("unsupported resv_size, 0x%x\n", resv_size);
    return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
    
created:
    if ((consumer <= 0) || (consumer > FIFO_CONSUMER_MAX))
    {
        TRACE_ERR("unsupported consumer(%d)\n", consumer);
        return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
    }
#ifdef _RTSPSRV_TEST
    if (NULL == (fd = calloc(1, FIFO_FD_S_LEN + FIFO_CONSUMER_INFO_S_LEN * consumer + size + resv_size)))
#else
    if (NULL == (fd = mem_manage_calloc(COMP_NENT, 1, FIFO_FD_S_LEN + FIFO_CONSUMER_INFO_S_LEN * consumer + size + resv_size)))
#endif /* _WIN32 */
    {
        TRACE_ERR("NO MEMERY(%d)\n", FIFO_FD_S_LEN + FIFO_CONSUMER_INFO_S_LEN * consumer + size + resv_size);
        return ERRNO(COMMON_ERROR_NOMEM, COMP_NENT);
    }
    pthread_mutex_init(&fd->mutex, NULL);
    fd->maxch = consumer;
    fd->size = size;
    fd->resv_size = resv_size;
    fd->pcm = (PFIFO_CONSUMER_INFO_S)(fd + 1);
    fd->space = (char *)(fd->pcm + consumer);
    fd->dbg_err_count = 0;
    for (i = 0; i < fd->maxch; i++)
    {
        pthread_mutex_init(&fd->pcm[i].mtx_ci, NULL);
    }
    fd->flag = COMP_FLAG;
    if (HB_FALSE == pfifo_param->opened)
    {
    #ifdef ENABLE_FIFO_FILELOG
        FLASH_LOG_CONFIG_S flashwritecnfg = {1, {0}, 1};
    #endif /* ENABLE_FIFO_FILELOG */
        pfifo_param->opened = HB_TRUE;
#if 0 //ndef _WIN32
        TRACE_SWVER(COMP_NENT, swver);
        GET_SRC_REVISION(FIFO);
        cli_reg(pfifo_cli);
#endif /* _WIN32 */
    #ifdef ENABLE_FIFO_FILELOG
        if ((fifo_log_file_hdl = flash_log_fopen(COMP_FIFO, flashwritecnfg, FIFO_LOG_FILE_NAME, FIFO_LOG_FILE_SIZE)) < 0)
        {
            TRACE_DBG_LVL(COMP_FIFO, PRN_LVL_ERR, "FIFO LOG FILE OPEN FAIL! 0x%x\n", fifo_log_file_hdl);
        }
    #endif /* ENABLE_FIFO_FILELOG */
    }
    TRACE_DBG("fd(0x%x) consumer_stat(0x%llx) pcm(0x%x) space(0x%x) size(%d) resv_size(%d) consumer(%d)\n", 
        (sint32)fd, fd->consumer_stat, (sint32)fd->pcm, (sint32)fd->space, size, resv_size, consumer);

    return (sint32)fd;

initd:
    pthread_mutex_init(&fd->pcm[consumer].mtx_ci, NULL);
    fd->pcm[consumer].lowater = lowater * fd->size / 100;
    fd->pcm[consumer].hiwater = hiwater * fd->size / 100;
    fd->pcm[consumer].htwater = htwater * fd->size / 100;
    fd->pcm[consumer].link_stat = FIFO_LINKSTAT_NO;
    if (fd->full_keyfrm_stat)
    {
        if (0 == (fd->full_keyfrm_stat & FIFO_KEYFRM_OVERLAY))
        {
            fd->pcm[consumer].timecount = 1;
            fd->pcm[consumer].vframe_count = fd->full_vframe_count;
            fd->pcm[consumer].cci = fd->pcm[consumer].ci = fd->full_keyfrm_posi;
        }
        else
        {
            fd->pcm[consumer].timecount = 0;
            fd->pcm[consumer].vframe_count = 1;
            fd->pcm[consumer].cci = fd->pcm[consumer].ci = fd->full_vframe_posi;
        }
    }
    else
    {
        fd->pcm[consumer].timecount = 0;
        fd->pcm[consumer].vframe_count = 0;
        fd->pcm[consumer].cci = fd->pcm[consumer].ci = fd->pi;
    }
    fd->consumer_stat |= BIT64(consumer);
    pthread_mutex_unlock(&fd->mutex);
    TRACE_DBG("fd(0x%x) size(%d) resv_size(%d) consumer_stat(0x%llx) consumer(%d) lowater(%d) hiwater(%d)\n", 
        (sint32)fd, size, resv_size, fd->consumer_stat, consumer, lowater, hiwater);

    return consumer;
}

/******************************************************************************
 * 函数介绍: 关闭FIFO,该函数将清楚fifo所有数据,并释放相关资源
 * 输入参数: fd: FIFO句柄结构体信息指针
 * 输出参数: 无
 * 返回值  : 0-成功,<0-错误代码
 *****************************************************************************/
static sint32 _fifo_close(PFIFO_FD_S fd)
{
    fd->flag = 0;
#ifdef ENABLE_FIFO_FILELOG
    if (fifo_log_file_hdl >= 0)
    {
        flash_log_fclose(fifo_log_file_hdl);
        fifo_log_file_hdl = FIFO_LOG_FILE_FD_INVAL;
    }
#endif /* ENABLE_FIFO_FILELOG */
    pthread_mutex_destroy(&fd->mutex);

#ifdef _RTSPSRV_TEST
    free(fd);

    return 0;
#else
    return mem_manage_free(COMP_NENT, fd);
#endif /* _WIN32 */
}

/******************************************************************************
 * 函数介绍: 根据不同的消费者从FIFO中读取数据,该函数一次读取长度size数据,当FIFO中数据不足时将返回错误信息
 * 输入参数: fd: FIFO句柄结构体信息指针
 *           consumer: 消费者序号(FIFO_CONSUMER_E)
 *           data: 缓冲区指针(此值可以为空(NULL),为空时,数据将不会被拷贝)
 *           size: 读取大小
 *           peek: 是否清除FIFO中已读数据(FIFO_PEEK_E)
 * 输出参数: 无
 * 返回值  : >0-读出数据大小,<0-错误代码
 *****************************************************************************/
static sint32 _fifo_read(PFIFO_FD_S fd, FIFO_CONSUMER_E consumer, char *data, uint32 size, FIFO_PEEK_E peek)
{
    sint32 data_size = 0;

    if (0 == size)
    {
    #ifdef ENABLE_FIFO_FILELOG
        TRACE_LOG("fifo_read handle(0x%x)[%d], unsupported size(%d)\n", (sint32)fd, consumer, size);
    #else
    #ifndef _WIN32
        TRACE_ASSERT(0 != size, "fifo_read handle(0x%x)[%d], unsupported size(%d)\n", (sint32)fd, consumer, size);
    #endif /* _WIN32 */
    #endif /* ENABLE_FIFO_FILELOG */
        return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
    }
    
    pthread_mutex_lock(&fd->pcm[consumer].mtx_ci);
    if (fd->pcm[consumer].cci == fd->pcm[consumer].ci)
    {
        if ((data_size = FIFO_DATA_SIZE(fd, consumer)) < size) /* 缓存数据长度不足 */
        {
            pthread_mutex_unlock(&fd->pcm[consumer].mtx_ci);
            TRACE_REL("handle(0x%x) size(%d) data_size(%d) ci[%d](%d) pi(%d) is not enough\n",
                (sint32)fd, size, data_size, consumer, fd->pcm[consumer].ci, fd->pi);
            return ERRNO(FIFO_ERROR_NODAT, COMP_NENT);
        }
        FIFO_READ(fd, consumer, data, size, peek);
        pthread_mutex_unlock(&fd->pcm[consumer].mtx_ci);
    }
    else
    {
        TRACE_ERR("cci(%d) and ci(%d) unmatched\n", fd->pcm[consumer].cci, fd->pcm[consumer].ci);
        fd->pcm[consumer].cci = fd->pcm[consumer].ci;
        pthread_mutex_unlock(&fd->pcm[consumer].mtx_ci);
        return ERRNO(FIFO_ERROR_CCICIUNMATCHED, COMP_NENT);
    }

    return size;
}

#if 1 //ndef _WIN32
/******************************************************************************
 * 函数介绍: 根据不同的消费者从FIFO中读取数据,该函数一次读取1帧数据,当FIFO中数据不足时将返回错误信息
 * 输入参数: handle: FIFO句柄
 *           consumer: 消费者序号(FIFO_CONSUMER_E)
 *           data: 缓冲区指针(此值可以为空(NULL),为空时,数据将不会被拷贝)
 *           limit: 缓冲区大小(data为空(NULL)时,此值可填无限大)
 *           peek: 是否清除FIFO中已读数据(FIFO_PEEK_E)
 * 输出参数: 无
 * 返回值  : >0-读出数据大小,0-缓冲区不足,<0-错误代码
 *****************************************************************************/
static sint32 _fifo_readframe(sint32 handle, FIFO_CONSUMER_E consumer, char *data, uint32 limit, FIFO_PEEK_E peek)
{
    PFIFO_FD_S fd = (PFIFO_FD_S)handle;
    
    if ((handle <= 0) || (0 == FLAG_VERIFY(fd)) || (0 == FIFO_CONSUMER_STAT(fd, consumer)))
    {
        TRACE_ERR("handle(0x%x)[%d] invalid\n", handle, consumer);
        return ERRNO(COMMON_ERROR_HANDLEINVAL, COMP_NENT);
    }

    pthread_mutex_lock(&fd->pcm[consumer].mtx_ci);
    if (fd->pcm[consumer].cci == fd->pcm[consumer].ci)
    {
        sint32 frame_size = 0;
        MSHEAD mshead, *pmshead = (PMSHEAD)&fd->space[fd->pcm[consumer].cci];

        if (FIFO_DATA_SIZE(fd, consumer) < MSHEAD_LEN) /* 缓存数据长度不足 */
        {
            pthread_mutex_unlock(&fd->pcm[consumer].mtx_ci);
            TRACE_REL("handle(0x%x) MSHEAD_LEN(%d) ci[%d](%d) pi(%d) is not enough\n",
                (sint32)fd, MSHEAD_LEN, consumer, fd->pcm[consumer].ci, fd->pi);
            return ERRNO(FIFO_ERROR_NODAT, COMP_NENT);
        }
        if (_fifo_check_posi(fd, fd->pcm[consumer].cci, MSHEAD_LEN))
        { /* 如果MSHEAD头信息不连续,需要提前取出才能处理 */
            pmshead = &mshead;
            _fifo_read_posi(fd, fd->pcm[consumer].cci, (char *)pmshead, MSHEAD_LEN);
            TRACE_DBG("cci position frame head is incomplete\n");
        }
        if (ISMSHEAD(pmshead))
        {
            if ((frame_size = MSHEAD_GETFRAMESIZE(pmshead)) > limit)
            {
                pthread_mutex_unlock(&fd->pcm[consumer].mtx_ci);
                TRACE_ERR("unsupported handle(0x%x)[%d] limit(%d) < frame_size(%d)\n", 
                    handle, consumer, limit, frame_size);
                return ERRNO(FIFO_ERROR_EXTBUFTOSMALL, COMP_NENT);
            }
            if (FIFO_DATA_SIZE(fd, consumer) < frame_size) /* 缓存数据长度不足 */
            {
                pthread_mutex_unlock(&fd->pcm[consumer].mtx_ci);
                TRACE_REL("handle(0x%x) frame_size(%d) ci[%d](%d) pi(%d) is not enough\n",
                    (sint32)fd, frame_size, consumer, fd->pcm[consumer].ci, fd->pi);
                return ERRNO(FIFO_ERROR_NODAT, COMP_NENT);
            }
            FIFO_READ(fd, consumer, data, frame_size, peek);
            if (FIFO_PEEK_YES == peek)
            {
                fd->pcm[consumer].link_stat = FIFO_LINKSTAT_NO;
                if (ISVIDEOFRAME(pmshead))
                {
                    DECREMENT_ATOMIC(fd->pcm[consumer].vframe_count, 1);
                #ifdef FIFO_DUMP_DEBUG
                    INCREMENT_ATOMIC(fd->pcm[consumer].vframe_count_ci, 1);
                #endif /* FIFO_DUMP_DEBUG */
                    if (ISKEYFRAME(pmshead))
                    {
                        DECREMENT_ATOMIC(fd->pcm[consumer].timecount, 1); /* 主要用于预录时间计数 */
                    #ifdef FIFO_DUMP_DEBUG
                        INCREMENT_ATOMIC(fd->pcm[consumer].timecount_ci, 1);
                    #endif /* FIFO_DUMP_DEBUG */
                        TRACE_REL("handle(0x%x)[%d] fcount(%d) time_sec(%d) timecount(%d)\n", 
                            handle, consumer, pmshead->fcount, pmshead->time_sec, fd->pcm[consumer].timecount);
                    }
                #ifdef FIFO_DUMP_DEBUG
                    TRACE_ASSERT(FIFO_DUMP_CHECK_TIMECOUNT(fd, consumer), "timecount_pi(%d) timecount_ci(%d) timecount(%d)", 
                        fd->pcm[consumer].timecount_pi, fd->pcm[consumer].timecount_ci, fd->pcm[consumer].timecount);
                    TRACE_ASSERT(FIFO_DUMP_CHECK_FRAMECOUNT(fd, consumer), "vframe_count_pi(%d) timecount_ci(%d) timecount(%d)", 
                        fd->pcm[consumer].vframe_count_pi, fd->pcm[consumer].vframe_count_ci, fd->pcm[consumer].vframe_count);
                #endif /* FIFO_DUMP_DEBUG */
                }
            }
            pthread_mutex_unlock(&fd->pcm[consumer].mtx_ci);
            return frame_size;
        }
        pthread_mutex_unlock(&fd->pcm[consumer].mtx_ci);
        TRACE_ERR("handle(0x%x)[%d] flag(0x%x)\n", handle, consumer, pmshead->flag);
        return ERRNO(FIFO_ERROR_MSHEAD, COMP_NENT);
    }
    TRACE_ERR("cci(%d) and ci(%d) unmatched\n", fd->pcm[consumer].cci, fd->pcm[consumer].ci);
    fd->pcm[consumer].cci = fd->pcm[consumer].ci;
    pthread_mutex_unlock(&fd->pcm[consumer].mtx_ci);
    
    return ERRNO(FIFO_ERROR_CCICIUNMATCHED, COMP_NENT);
}

/******************************************************************************
 * 函数介绍: 根据不同的消费者从FIFO中读取数据,该函数一次读取多帧音视频数据,直至遇到下一个关键帧
 * 输入参数: handle: FIFO句柄
 *           consumer: 消费者序号(FIFO_CONSUMER_E)
 *           data: 缓冲区指针(此值可以为空(NULL),为空时,数据将不会被拷贝)
 *           limit: 缓冲区大小(data为空(NULL)时,此值可填无限大)
 *           nexttype: 下一帧数据类型
 * 输出参数: 无
 * 返回值  : >0-读出数据大小,<0-错误代码
 *****************************************************************************/
static sint32 _fifo_readext(sint32 handle, FIFO_CONSUMER_E consumer, char *data, uint32 limit, sint32 *nexttype)
{
    PFIFO_FD_S fd = (PFIFO_FD_S)handle;
    
    if ((handle <= 0) || (0 == FLAG_VERIFY(fd)) || (0 == FIFO_CONSUMER_STAT(fd, consumer)))
    {
        TRACE_ERR("handle(0x%x)[%d] invalid\n", handle, consumer);
        return ERRNO(COMMON_ERROR_HANDLEINVAL, COMP_NENT);
    }
    if (NULL == nexttype)
    {
        TRACE_ERR("unsupported handle(0x%x)[%d] nexttype is NULL\n", handle, consumer);
        return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
    }

    pthread_mutex_lock(&fd->pcm[consumer].mtx_ci);
    if (fd->pcm[consumer].cci == fd->pcm[consumer].ci)
    {
        sint32 frame_size = 0, total_size = 0;
        MSHEAD mshead, *pmshead = (PMSHEAD)&fd->space[fd->pcm[consumer].cci];
        
        if (FIFO_DATA_SIZE(fd, consumer) < MSHEAD_LEN) /* 缓存数据长度不足 */
        {
            pthread_mutex_unlock(&fd->pcm[consumer].mtx_ci);
            TRACE_REL("handle(0x%x) MSHEAD_LEN(%d) ci[%d](%d) pi(%d) is not enough\n",
                (sint32)fd, MSHEAD_LEN, consumer, fd->pcm[consumer].ci, fd->pi);
            return ERRNO(FIFO_ERROR_NODAT, COMP_NENT);
        }
        if (_fifo_check_posi(fd, fd->pcm[consumer].cci, MSHEAD_LEN))
        { /* 如果MSHEAD头信息不连续,需要提前取出才能处理 */
            pmshead = &mshead;
            _fifo_read_posi(fd, fd->pcm[consumer].cci, (char *)pmshead, MSHEAD_LEN);
            TRACE_DBG("cci position frame head is incomplete\n");
        }
        if (ISMSHEAD(pmshead))
        {
            frame_size = MSHEAD_GETFRAMESIZE(pmshead);
            while ((NULL == data) || (frame_size <= (limit - total_size))) /* 如果data指针为空或缓冲区长度足够 */
            {
                if (FIFO_DATA_SIZE(fd, consumer) < frame_size) /* 缓存数据长度不足 */
                {
                    pthread_mutex_unlock(&fd->pcm[consumer].mtx_ci);
                    TRACE_REL("handle(0x%x) frame_size(%d) ci[%d](%d) pi(%d) is not enough\n",
                        (sint32)fd, frame_size, consumer, fd->pcm[consumer].ci, fd->pi);
                    return ERRNO(FIFO_ERROR_NODAT, COMP_NENT);
                }
                FIFO_READ(fd, consumer, data, frame_size, FIFO_PEEK_YES);
                total_size += frame_size; /* 用来记录读出数据的总大小 */
                if (data)
                {
                    data += frame_size;
                }
                if (ISVIDEOFRAME(pmshead))
                {
                    DECREMENT_ATOMIC(fd->pcm[consumer].vframe_count, 1);
                #ifdef FIFO_DUMP_DEBUG
                    INCREMENT_ATOMIC(fd->pcm[consumer].vframe_count_ci, 1);
                #endif /* FIFO_DUMP_DEBUG */
                    if (ISKEYFRAME(pmshead))
                    {
                        DECREMENT_ATOMIC(fd->pcm[consumer].timecount, 1); /* 主要用于预录时间计数 */
                    #ifdef FIFO_DUMP_DEBUG
                        INCREMENT_ATOMIC(fd->pcm[consumer].timecount_ci, 1);
                    #endif /* FIFO_DUMP_DEBUG */
                        TRACE_REL("handle(0x%x)[%d] fcount(%d) time_sec(%d) timecount(%d)\n", 
                           handle, consumer, pmshead->fcount, pmshead->time_sec, fd->pcm[consumer].timecount);
                    }
                #ifdef FIFO_DUMP_DEBUG
                    TRACE_ASSERT(FIFO_DUMP_CHECK_TIMECOUNT(fd, consumer), "timecount_pi(%d) timecount_ci(%d) timecount(%d)", 
                        fd->pcm[consumer].timecount_pi, fd->pcm[consumer].timecount_ci, fd->pcm[consumer].timecount);
                    TRACE_ASSERT(FIFO_DUMP_CHECK_FRAMECOUNT(fd, consumer), "vframe_count_pi(%d) timecount_ci(%d) timecount(%d)", 
                        fd->pcm[consumer].vframe_count_pi, fd->pcm[consumer].vframe_count_ci, fd->pcm[consumer].vframe_count);
                #endif /* FIFO_DUMP_DEBUG */
                }
                if (FIFO_DATA_SIZE(fd, consumer) < MSHEAD_LEN) /* 缓存数据长度不足 */
                {
                    TRACE_REL("handle(0x%x) MSHEAD_LEN(%d) ci[%d](%d) pi(%d) is not enough\n",
                        (sint32)fd, MSHEAD_LEN, consumer, fd->pcm[consumer].ci, fd->pi);
                    break;
                }
                if (_fifo_check_posi(fd, fd->pcm[consumer].cci, MSHEAD_LEN))
                { /* 如果MSHEAD头信息不连续,需要提前取出才能处理 */
                    pmshead = &mshead;
                    _fifo_read_posi(fd, fd->pcm[consumer].cci, (char *)pmshead, MSHEAD_LEN);
                    TRACE_DBG("cci position frame head is incomplete\n");
                }
                else
                {
                    pmshead = (PMSHEAD)&fd->space[fd->pcm[consumer].cci];
                }
                if (ISMSHEAD(pmshead))
                {
                    if (ISVIDEOFRAME(pmshead) && ISKEYFRAME(pmshead))
                    {
                        TRACE_DBG("consumer(%d) cci(%d) ci(%d) type(%d)\n", 
                            consumer, fd->pcm[consumer].cci, fd->pcm[consumer].ci, pmshead->type);
                        break;
                    }
                }
                else
                {
                    pthread_mutex_unlock(&fd->pcm[consumer].mtx_ci);
                    TRACE_ERR("handle(0x%x)[%d] flag(0x%x) cci(%d) ci(%d)\n", 
                        handle, consumer, pmshead->flag, fd->pcm[consumer].cci, fd->pcm[consumer].ci);
                    return ERRNO(FIFO_ERROR_MSHEAD, COMP_NENT);
                }
                frame_size = MSHEAD_GETFRAMESIZE(pmshead);
                TRACE_DBG("consumer(%d) cci(%d) ci(%d) type(%d)\n", 
                    consumer, fd->pcm[consumer].cci, fd->pcm[consumer].ci, pmshead->type);
            }
            *nexttype = pmshead->type;
            pthread_mutex_unlock(&fd->pcm[consumer].mtx_ci);

            return total_size;
        }
        pthread_mutex_unlock(&fd->pcm[consumer].mtx_ci);
        TRACE_ERR("handle(0x%x)[%d] flag(0x%x) cci(%d) ci(%d)\n", 
            handle, consumer, pmshead->flag, fd->pcm[consumer].cci, fd->pcm[consumer].ci);
        return ERRNO(FIFO_ERROR_MSHEAD, COMP_NENT);
    }
    TRACE_ERR("cci(%d) and ci(%d) unmatched\n", fd->pcm[consumer].cci, fd->pcm[consumer].ci);
    fd->pcm[consumer].cci = fd->pcm[consumer].ci;
    pthread_mutex_unlock(&fd->pcm[consumer].mtx_ci);
    
    return ERRNO(FIFO_ERROR_CCICIUNMATCHED, COMP_NENT);
}
#endif /* _WIN32 */

/******************************************************************************
 * 函数介绍: 根据不同的消费者获取FIFO中可用的缓冲区信息
 * 输入参数: handle: FIFO句柄
 *           consumer: 消费者序号(FIFO_CONSUMER_E)
 *           pbuff_info: 缓冲区信息结构体指针
 *           need_size: 需要的缓冲区大小
 * 输出参数: 无
 * 返回值  : >=0-成功,缓冲区总长度,<0-错误代码
 *****************************************************************************/
static sint32 _fifo_read_buffer_get(sint32 handle, FIFO_CONSUMER_E consumer, PFIFO_BUFF_INFO_S pbuff_info)
{
    PFIFO_FD_S fd = (PFIFO_FD_S)handle;
    sint32 data_size = 0;

    if ((handle <= 0) || (0 == FLAG_VERIFY(fd)))
    {
        TRACE_ERR("handle(0x%x) invalid\n", handle);
        return ERRNO(COMMON_ERROR_HANDLEINVAL, COMP_NENT);
    }

    if (pbuff_info)
    {
        pthread_mutex_lock(&fd->pcm[consumer].mtx_ci);
        if (fd->pcm[consumer].cci == fd->pcm[consumer].ci)
        {
            if ((data_size = FIFO_DATA_SIZE(fd, consumer)) > 0)
            {
                sint32 fifo_limit = IMIN(fd->size - fd->pcm[consumer].cci, data_size); /* 计算当前FIFO连续一段数据长度 */
                
                pbuff_info->buff[0].pbuff = fd->space + fd->pcm[consumer].cci;
                if (data_size > fifo_limit)
                {
                    pbuff_info->buff[0].size = fifo_limit;
                    pbuff_info->buff[1].pbuff = fd->space;
                    pbuff_info->buff[1].size = data_size - fifo_limit;
                    pbuff_info->buff_count = 2;
                }
                else
                {
                    pbuff_info->buff[0].size = data_size;
                    pbuff_info->buff[1].pbuff = NULL;
                    pbuff_info->buff[1].size = 0;
                    pbuff_info->buff_count = 1;
                }
            }
            pthread_mutex_unlock(&fd->pcm[consumer].mtx_ci);
        }
        else
        {
            TRACE_ERR("cci(%d) and ci(%d) unmatched\n", fd->pcm[consumer].cci, fd->pcm[consumer].ci);
            fd->pcm[consumer].cci = fd->pcm[consumer].ci;
            pthread_mutex_unlock(&fd->pcm[consumer].mtx_ci);
            return ERRNO(FIFO_ERROR_CCICIUNMATCHED, COMP_NENT);
        }
    }
    else
    {
        TRACE_ERR("handle(0x%x) unsupported pbuff_info is NULL\n", handle);
        return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
    }
    
    return data_size;
}

/******************************************************************************
 * 函数介绍: 根据不同的消费者更新FIFO中可用的缓冲区信息
 * 输入参数: handle: FIFO句柄
 *           consumer: 消费者序号(FIFO_CONSUMER_E)
 *           pbuff_info: 缓冲区信息结构体指针
 *           update_size: 更新的缓冲区大小
 * 输出参数: 无
 * 返回值  : 0-成功,<0-错误代码
 *****************************************************************************/
static sint32 _fifo_read_buffer_update(sint32 handle, FIFO_CONSUMER_E consumer, PFIFO_BUFF_INFO_S pbuff_info, uint32 update_size)
{
    PFIFO_FD_S fd = (PFIFO_FD_S)handle;

    if ((handle <= 0) || (0 == FLAG_VERIFY(fd)))
    {
        TRACE_ERR("handle(0x%x) invalid\n", handle);
        return ERRNO(COMMON_ERROR_HANDLEINVAL, COMP_NENT);
    }

    pthread_mutex_lock(&fd->pcm[consumer].mtx_ci);
    if (fd->pcm[consumer].cci == fd->pcm[consumer].ci)
    {
        FIFO_READ_UPDATE(fd, consumer, update_size);
        pthread_mutex_unlock(&fd->pcm[consumer].mtx_ci);
    }
    else
    {
        TRACE_ERR("cci(%d) and ci(%d) unmatched\n", fd->pcm[consumer].cci, fd->pcm[consumer].ci);
        fd->pcm[consumer].cci = fd->pcm[consumer].ci;
        pthread_mutex_unlock(&fd->pcm[consumer].mtx_ci);
        return ERRNO(FIFO_ERROR_CCICIUNMATCHED, COMP_NENT);
    }
    
    return 0;
}


/******************************************************************************
 * 函数介绍: 根据不同的消费者从FIFO中读取数据并处理
 * 输入参数: handle: FIFO句柄
 *           consumer: 消费者序号(FIFO_CONSUMER_E)
 *           dataprocess: 数据处理要求
 *           func: 数据处理回调函数
 *           func_idx: datacallback()函数中的idx,
 *           func_handle: datacallback()函数中的fifo,此处用于回调函数的句柄,例如MFS句柄等等
 *           deal_ifrm_nums: 需要一次性处理关键帧个数,最小值为1
 * 输出参数: 无
 * 返回值  : >0-已处理数据大小,0-数据不足一帧,或FIFO的resv_size为0时,FIFO帧处理要做掉头处理,<0-错误代码
 *****************************************************************************/
static sint32 _fifo_dataprocess(sint32 handle, FIFO_CONSUMER_E consumer, FIFO_DATAPROCESS_E dataprocess, 
    datacallback func, sint32 func_idx, sint32 func_handle, sint32 deal_ifrm_nums)
{
    PFIFO_FD_S fd = (PFIFO_FD_S)handle;

    if ((handle <= 0) || (0 == FLAG_VERIFY(fd)) || (0 == FIFO_CONSUMER_STAT(fd, consumer)))
    {
        TRACE_ERR("handle(0x%x)[%d] invalid\n", handle, consumer);
        return ERRNO(COMMON_ERROR_HANDLEINVAL, COMP_NENT);
    }

    pthread_mutex_lock(&fd->pcm[consumer].mtx_ci);
    if (fd->pcm[consumer].cci == fd->pcm[consumer].ci)
    {
        PMSHEAD pmshead_curt = (PMSHEAD)&fd->space[fd->pcm[consumer].cci];
        
        if (ISMSHEAD(pmshead_curt))
        {
            sint32 rtval = 0, timecount = 0, frame_count = 0, vframe_count = 0;
            sint32 frame_size_back = 0, frame_size = 0, deal_size = 0, lose_size = 0, write_size = 0;
            sint32 data_size = FIFO_DATA_SIZE(fd, consumer), fifo_limit = IMIN(fd->size - fd->pcm[consumer].cci, data_size); /* 计算当前FIFO连续一段数据长度 */

            TRACE_REL("handle(0x%x) data_size(%d) fifo_limit(%d)\n", handle, data_size, fifo_limit);
            while (deal_size < fifo_limit) /* 逐帧检测数据,直到遇到一个I帧,或者到达该段数据末尾,并得出实际数据长度 */
            {
                if (data_size < (deal_size + MSHEAD_LEN)) /* 数据帧头不完整,数据长度不够 */
                {
                    TRACE_DBG("handle(0x%x) last frame mshead is incomplete\n", handle);
                    break;
                }
                if (fifo_limit < (deal_size + MSHEAD_LEN)) /* 数据帧头为内存不连续数据,即FIFO发生掉头现象 */
                {
                    if (fd->resv_size)
                    {
                        memcpy(fd->space + fd->size, fd->space, deal_size + MSHEAD_LEN - fifo_limit);
                        TRACE_DBG("handle(0x%x) frame mshead may be incomplete\n", handle);
                    }
                    else
                    {
                        fd->pcm[consumer].link_stat = FIFO_LINKSTAT_YES;
                        TRACE_DBG("handle(0x%x) link_stat[%d](%d)\n", handle, consumer, fd->pcm[consumer].link_stat);
                        break;
                    }
                }
                if (data_size < (deal_size + (frame_size = MSHEAD_GETFRAMESIZE(pmshead_curt))))
                { /* 帧数据不完整,数据长度不够 */
                    TRACE_DBG("handle(0x%x) fifo_limit(%d) last frame data is incomplete\n", handle, fifo_limit);
                    break;
                }
                if (fifo_limit < (deal_size + frame_size)) /* 帧数据为内存不连续数据,即FIFO发生掉头现象 */
                {
                    if (fd->resv_size) /* 将数据拼接为一个整段连续数据 */
                    {
                        sint32 move_size = deal_size + frame_size - fifo_limit;

                        if (move_size <= fd->resv_size)
                        {
                            memcpy(fd->space + fd->size, fd->space, move_size);
                            TRACE_REL("handle(0x%x)[%d] move_size(%d)\n", handle, move_size);
                        }
                        else
                        {
                            sint32 mdsize = MSHEAD_GETMSDSIZE(pmshead_curt);
                            
                            lose_size = ALIGN_N(move_size - fd->resv_size, 4);      /* 算出丢弃数据大小,并按4字节对齐 */
                            memcpy(fd->space + fd->size, fd->space, fd->resv_size); /* 截取丢弃数据 */
                            MSHEAD_SETMSDSIZE_MAX(pmshead_curt, mdsize - lose_size);/* 重新设置MSHEAD中msdsize大小 */
                            TRACE_ERR("handle(0x%x)[%d] lose data type(%d) deal_size(%d) frame_size(%d) lose_size(%d)\n", 
                                handle, consumer, pmshead_curt->type, deal_size, frame_size, lose_size);
                        }
                    }
                    else /* 将数据暂时不向外回调,通过其他方式取出,例如: fifo_readframe方式 */
                    {
                        fd->pcm[consumer].link_stat = FIFO_LINKSTAT_YES;
                        TRACE_DBG("handle(0x%x) link_stat[%d](%d)\n", handle, consumer, fd->pcm[consumer].link_stat);
                        break;
                    }
                }
                if (ISMSHEAD(pmshead_curt))
                {
                    if (ISVIDEOFRAME(pmshead_curt))
                    {
                        if ((FIFO_DATAPROCESS_ONE_VIDEO_MULTY_AUDIO_FRAME == dataprocess) && vframe_count)
                        { /* 处理数据直到遇到视频帧(除了头部第一帧为视频帧的情况) */
                            break;
                        }
                        if (ISKEYFRAME(pmshead_curt))
                        {
                            if ((FIFO_DATAPROCESS_MULTY_VIDEO_MULTY_AUDIO_FRAME == dataprocess) && 
                                frame_count && (timecount >= deal_ifrm_nums))
                            { /* 处理数据直到遇到I帧(除了头部第一帧为视频I帧的情况) */
                                break;
                            }
                            timecount++; /* 视频关键帧计数,主要用于预录时间计数 */
                            TRACE_REL("handle(0x%x)[%d] fcount(%d) time_sec(%d) timecount(%d) deal_ifrm_nums(%d)\n", 
                                handle, consumer, pmshead_curt->fcount, pmshead_curt->time_sec, timecount, deal_ifrm_nums);
                        }
                        vframe_count++; /* 视频帧计数 */
                    }
                }
                else
                {
                    lose_size = frame_size_back;
                    TRACE_ERR("handle(0x%x)[%d] flag(0x%x) ci(0x%x) cci(0x%x)\n"
                        "\tlinkstat(%d) tmcount(%d) vfcount(%d) deal_size(%d) lose_size(%d)\n",
                        handle, consumer, pmshead_curt->flag, fd->pcm[consumer].ci, 
                        fd->pcm[consumer].cci, fd->pcm[consumer].link_stat, fd->pcm[consumer].timecount, 
                        fd->pcm[consumer].vframe_count, deal_size, lose_size);
                    if (frame_size_back)
                    {
                        data_hex_dump((char *)((sint32)pmshead_curt - lose_size), lose_size);
                    }
                    break;
                }
                deal_size += frame_size;
                if (FIFO_DATAPROCESS_ONE_VIDEO_OR_AUDIO_FRAME == dataprocess)
                { /* 只处理一帧视频或音频数据 */
                    break;
                }
                frame_count++;
                pmshead_curt = (PMSHEAD)(((sint32)pmshead_curt) + frame_size);
                frame_size_back = frame_size;
            }
            if (deal_size > 0)
            {
                pthread_mutex_unlock(&fd->pcm[consumer].mtx_ci);
                write_size = deal_size - lose_size;
                if (func && (write_size > 0)) /* 调用回调函数处理该段数据(不包含丢弃数据大小) */
                {
                    rtval = func(func_idx, func_handle, fd->space + fd->pcm[consumer].cci, write_size);
                }
                pthread_mutex_lock(&fd->pcm[consumer].mtx_ci);
                if (fd->pcm[consumer].cci == fd->pcm[consumer].ci)
                {
					if (rtval >= 0)
					{
						/* 数据读指针后移已处理的数据长度(包含丢弃数据大小) */
						FIFO_READ_UPDATE(fd, consumer, deal_size);
						DECREMENT_ATOMIC(fd->pcm[consumer].timecount, timecount);
						DECREMENT_ATOMIC(fd->pcm[consumer].vframe_count, vframe_count);
					
                #ifdef FIFO_DUMP_DEBUG
                    INCREMENT_ATOMIC(fd->pcm[consumer].timecount_ci, timecount);
                    INCREMENT_ATOMIC(fd->pcm[consumer].vframe_count_ci, vframe_count);
                    TRACE_ASSERT(FIFO_DUMP_CHECK_TIMECOUNT(fd, consumer), "timecount_pi(%d) timecount_ci(%d) timecount(%d-%d)", 
                        fd->pcm[consumer].timecount_pi, fd->pcm[consumer].timecount_ci, fd->pcm[consumer].timecount, timecount);
                    TRACE_ASSERT(FIFO_DUMP_CHECK_FRAMECOUNT(fd, consumer), "vframe_count_pi(%d) timecount_ci(%d) timecount(%d-%d)", 
                        fd->pcm[consumer].vframe_count_pi, fd->pcm[consumer].vframe_count_ci, 
                        fd->pcm[consumer].vframe_count, vframe_count);
                #endif /* FIFO_DUMP_DEBUG */
					}
                    pthread_mutex_unlock(&fd->pcm[consumer].mtx_ci);
                    TRACE_REL("handle(0x%x)[%d] cci(%d) rtval(0x%x) deal_size(%d) write_size(%d) lose_size(%d)\n", 
                        handle, consumer, fd->pcm[consumer].cci, rtval, deal_size, write_size, lose_size);
                    
                    return MUX(rtval < 0, rtval, write_size); /* 返回数据处理结果(不包含丢弃数据大小) */
                }
                TRACE_ERR("handle(0x%x)[%d] cci(%d) and ci(%d) unmatched, may have been written wrong data\n", 
                    handle, consumer, fd->pcm[consumer].cci, fd->pcm[consumer].ci);
                fd->pcm[consumer].cci = fd->pcm[consumer].ci;
                pthread_mutex_unlock(&fd->pcm[consumer].mtx_ci);
                return ERRNO(FIFO_ERROR_CCICIUNMATCHED, COMP_NENT);
            }
            pthread_mutex_unlock(&fd->pcm[consumer].mtx_ci);
            TRACE_DBG("handle(0x%x)[%d] cci(%d) deal_size(%d) write_size(%d) lose_size(%d)\n", 
                handle, consumer, fd->pcm[consumer].cci, deal_size, write_size, lose_size);

            return deal_size;
        }
        else
        {
            pthread_mutex_unlock(&fd->pcm[consumer].mtx_ci);
            TRACE_ERR("handle(0x%x)[%d] flag(0x%x) ci(0x%x) cci(0x%x) linkstat(%d) tmcount(%d) vfcount(%d)\n",
                handle, consumer, pmshead_curt->flag, fd->pcm[consumer].ci, 
                fd->pcm[consumer].cci, fd->pcm[consumer].link_stat, 
                fd->pcm[consumer].timecount, fd->pcm[consumer].vframe_count);
            return ERRNO(FIFO_ERROR_MSHEAD, COMP_NENT);
        }
    }
    TRACE_ERR("handle(0x%x)[%d] cci(%d) and ci(%d) unmatched\n", handle, consumer, fd->pcm[consumer].cci, fd->pcm[consumer].ci);
    fd->pcm[consumer].cci = fd->pcm[consumer].ci;
    pthread_mutex_unlock(&fd->pcm[consumer].mtx_ci);
    
    return ERRNO(FIFO_ERROR_CCICIUNMATCHED, COMP_NENT);
}

#ifndef _RTSPSRV_TEST
static sint32 _fifo_dump_cli(char *args)
{
    PFIFO_FD_S fd = NULL;
    sint32 handle = 0, dfd = 0, size = 32;
    char dump_fpath[128] = {0}, opt = 0x00, *param = NULL;
    
    if ((NULL == args) || (0 == args[0]))
    {
        cli_print("No args!\n");
        return -1;
    }
    CLI_CMD_ARGS_PARSE_BGN(args, "hfps");
    opt = CLI_CMD_GET_OPTION();
    param = CLI_CMD_GET_OPT_PARAM();
    switch (opt)
    {
    case 'f':
        if (('0' == param[0]) && (('x' == param[1]) || ('X' == param[1])))
        {
            char fmt[] = "0x%x";
            
            fmt[1] = param[1];
            sscanf(param, fmt, &handle);
            cli_print("handle = 0x%x\n", handle);
            fd = (PFIFO_FD_S)handle;
            if ((handle <= 0) || !FLAG_VERIFY(fd))
            {
                fd = NULL;
            }
        }
        break;
    case 's':
        if (0 > (size = atoi(param)))
        {
            size = 32;
        }
        break;        
    case 'p':
        if (param[0])
        {
            strncpy(dump_fpath, param, sizeof(dump_fpath));
        }
        break;
    case 'h':
        cli_print("-f handle_value\n");
        break;
    }
    CLI_CMD_ARGS_PARSE_END();

    if (!fd)
    {
        cli_print("Invalid fd 0x%x!\n", handle);
        return -1;
    }
    if (!pthread_mutex_trylock(&fd->mutex))
    {
        CLI_PRN_TBL_BGN("fifo info dump", 1);        
        CLI_PRN_TBL_COLUMN("key_flag",      1, "%d", fd->keyflag);
        CLI_PRN_TBL_COLUMN("   size   ",    1, "%d", fd->size);
        CLI_PRN_TBL_COLUMN(" rsv_size ",    1, "%d", fd->resv_size);
        CLI_PRN_TBL_COLUMN("    pi    ",    1, "%d", fd->pi);
        CLI_PRN_TBL_COLUMN("full_kf_pos",   1, "%d", fd->full_keyfrm_posi);
        CLI_PRN_TBL_COLUMN(" ffrag_pos",    1, "%d", fd->part_allfrm_posi);
        CLI_PRN_TBL_COLUMN("ffrag_real_sz", 1, "%d", fd->part_allfrm_tlsz);
        CLI_PRN_TBL_COLUMN("ffrag_cur_sz",  1, "%d", fd->part_allfrm_wtsz);
        CLI_PRN_TBL_COLUMN("full_vf_count", 1, "%d", fd->full_vframe_count);
        CLI_PRN_TBL_COLUMN("full_keyfrm_stat",   1, "%d", fd->full_keyfrm_stat);
        CLI_PRN_TBL_COLUMN("part_allfrm_stat",   1, "%d", fd->part_allfrm_stat);
        CLI_PRN_TBL_COLUMN("consumer_stat", 1,      "0x%llx", fd->consumer_stat);
        CLI_PRN_TBL_COLUMN("pcm",           fd->pcm, "%p", fd->pcm);
        CLI_PRN_TBL_END();
        
        CLI_PRN_TBL_BGN("fifo pcm dump", 1);
        CLI_PRN_TBL_COLUMN("topwater",      fd->pcm, "%d", fd->pcm->htwater);
        CLI_PRN_TBL_COLUMN("hiwater",       fd->pcm, "%d", fd->pcm->hiwater);        
        CLI_PRN_TBL_COLUMN("lowater",       fd->pcm, "%d", fd->pcm->lowater);
        CLI_PRN_TBL_COLUMN("    ci    ",    fd->pcm, "%d", fd->pcm->ci);
        CLI_PRN_TBL_COLUMN("   cci    ",    fd->pcm, "%d", fd->pcm->cci);
        CLI_PRN_TBL_COLUMN("link_stat",     fd->pcm, "%d", fd->pcm->link_stat);
        CLI_PRN_TBL_COLUMN("timecount",     fd->pcm, "%d", fd->pcm->timecount);
        CLI_PRN_TBL_COLUMN("vframe_count",  fd->pcm, "%d", fd->pcm->vframe_count);
        CLI_PRN_TBL_END();
        
        if (dump_fpath[0])
        {
            if ((dfd = open(dump_fpath, O_RDWR | O_CREAT | O_NONBLOCK, 
                S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) < 0)
            {
                cli_print("open %s error : %s\n", dump_fpath, strerror(errno));
            }
        }

        if (!dump_fpath[0] || (dfd < 0))
        {
            sint32 data_size = 0, len = 0;

            if (fd->pcm)
            {
                pthread_mutex_lock(&fd->pcm[0].mtx_ci);            
                if (fd->pcm[0].cci == fd->pcm[0].ci)
                {
                    if ((data_size = FIFO_DATA_SIZE(fd, 0)) < size) /* 缓存数据长度不足 */
                    {
                        cli_print("handle(%p) dump size(%d) data_size(%d) ci[%d](%d) pi(%d) is not enough\n",
                            fd, size, data_size, 0, fd->pcm[0].ci, fd->pi);
                    }
                    else
                    {
                        len = fd->size - fd->pcm[0].cci;
                        if (len < size)
                        {
                            cli_hexdump((uint8 *)(fd->space + fd->pcm[0].cci), len);
                            cli_hexdump((uint8 *)fd->space, size - len);
                        }
                        else
                        {
                            cli_hexdump((uint8 *)(fd->space + fd->pcm[0].cci), size);
                        }
                    }
                }
                else
                {
                    cli_print("cci(%d) and ci(%d) unmatched\n", fd->pcm[0].cci, fd->pcm[0].ci);
                }
                pthread_mutex_unlock(&fd->pcm[0].mtx_ci);
            }
        }
        else
        {
            char *pdata = fd->space;
            sint32 total = fd->size + fd->resv_size, once = 0;
            
            while (total > 0)
            {
                if (0 >= (once = write(dfd, pdata, total)))
                {
                    cli_print("write %s error : %s\n", dump_fpath, strerror(errno));
                }
                else
                {
                    total -= once;
                    pdata += once;                        
                }
            }
            close(dfd);
        }
        pthread_mutex_unlock(&fd->mutex);
    }
    
    return 0;
}
#endif /* _WIN32 */

static sint32 _fifo_dump_info(PFIFO_FD_S fd, FIFO_CONSUMER_E consumer)
{    
    fd->pcm = (PFIFO_CONSUMER_INFO_S)(fd + 1);
    fd->space = (char *)(fd->pcm + fd->maxch);
    TRACE_ALWAYS("flag:   %s \n", (char *)&fd->flag);
    TRACE_ALWAYS("keyflag:%#x \n", fd->keyflag);
    TRACE_ALWAYS("maxch:  %d \n", fd->maxch);
    TRACE_ALWAYS("size:   %d \n", fd->size);
    TRACE_ALWAYS("pi:     %d \n", fd->pi);
    TRACE_ALWAYS("full_keyfrm_posi:   %d \n", fd->full_keyfrm_posi);
    TRACE_ALWAYS("full_vframe_posi:   %d \n", fd->full_vframe_posi);
    TRACE_ALWAYS("part_allfrm_posi:   %d \n", fd->part_allfrm_posi);
    TRACE_ALWAYS("part_allfrm_tlsz:   %d \n", fd->part_allfrm_tlsz);
    TRACE_ALWAYS("part_allfrm_wtsz:   %d \n", fd->part_allfrm_wtsz);
    TRACE_ALWAYS("full_vframe_count:  %d \n", fd->full_vframe_count);
    TRACE_ALWAYS("full_keyfrm_stat:   %d \n", fd->full_keyfrm_stat);
    TRACE_ALWAYS("part_allfrm_stat:   %d \n", fd->part_allfrm_stat);
    TRACE_ALWAYS("consumer_stat:      %#llx \n\n", fd->consumer_stat);
    if (FIFO_CONSUMER_ALL == consumer)
    {
        sint32 i = FIFO_CONSUMER_FIR;
        
        for (; i < fd->maxch; i++)
        {
            if (FIFO_CONSUMER_STAT(fd, i))
            {
                TRACE_ALWAYS("\n-------FIFO_CONSUMER_INFO_S[%d]--------\n", i);
                TRACE_ALWAYS("htwater:       %d \n", fd->pcm[i].htwater);
                TRACE_ALWAYS("hiwater:       %d \n", fd->pcm[i].hiwater);
                TRACE_ALWAYS("lowater:       %d \n", fd->pcm[i].lowater);
                TRACE_ALWAYS("ci:            %d \n", fd->pcm[i].ci);
                TRACE_ALWAYS("cci:           %d \n", fd->pcm[i].cci);
                TRACE_ALWAYS("data size:     %d \n", FIFO_DATA_SIZE(fd, i));
                TRACE_ALWAYS("free size:     %d \n", FIFO_FREE_SIZE(fd, i));
                TRACE_ALWAYS("link_stat:     %d \n", fd->pcm[i].link_stat);
                TRACE_ALWAYS("timecount:     %d \n", fd->pcm[i].timecount);
                TRACE_ALWAYS("vframe_count:  %d \n", fd->pcm[i].vframe_count);
            }
        }
    }
    else if ((consumer >= FIFO_CONSUMER_FIR) && (consumer < FIFO_CONSUMER_MAX))
    {
        if (FIFO_CONSUMER_STAT(fd, consumer))
        {
            TRACE_ALWAYS("\n-------FIFO_CONSUMER_INFO_S[%d]--------\n", consumer);
            TRACE_ALWAYS("htwater:       %d \n", fd->pcm[consumer].htwater);
            TRACE_ALWAYS("hiwater:       %d \n", fd->pcm[consumer].hiwater);
            TRACE_ALWAYS("lowater:       %d \n", fd->pcm[consumer].lowater);
            TRACE_ALWAYS("ci:            %d \n", fd->pcm[consumer].ci);
            TRACE_ALWAYS("cci:           %d \n", fd->pcm[consumer].cci);
            TRACE_ALWAYS("data size:     %d \n", FIFO_DATA_SIZE(fd, consumer));
            TRACE_ALWAYS("free size:     %d \n", FIFO_FREE_SIZE(fd, consumer));
            TRACE_ALWAYS("link_stat:     %d \n", fd->pcm[consumer].link_stat);
            TRACE_ALWAYS("timecount:     %d \n", fd->pcm[consumer].timecount);
            TRACE_ALWAYS("vframe_count:  %d \n", fd->pcm[consumer].vframe_count);
        }
        else
        {
            TRACE_ERR("consumer(%d) not enable\n", consumer);
            return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
        }
    }
    else
    {
        TRACE_ERR("unsupported consumer(%d)\n", consumer);
        return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
    }
    
    return 0;
}

static sint32 _fifo_dump_data_find_mshead(PFIFO_FD_S fd, FIFO_CONSUMER_E consumer, uint32 ci)
{
    SYSTIME systm;
    MSHEAD mshead, *pmshead = NULL;
    HB_BOOL en_find_mshead = HB_FALSE;
    uint32 offset = MUX(ci >= fd->size, fd->pcm[consumer].ci, ci);
    
    TRACE_DBG("consumer(%d) ci(%d) size(%d)\n", consumer, fd->pcm[consumer].ci, fd->size);
    for (;;)
    {
        pmshead = (PMSHEAD)&fd->space[offset];
        if (ISMSHEAD(pmshead))
        {
            if (_fifo_check_posi(fd, offset, MSHEAD_LEN))
            {
                pmshead = &mshead;
                _fifo_read_posi(fd, offset, (char *)pmshead, MSHEAD_LEN);
                TRACE_DBG("offset position frame head is incomplete\n");
            }
            TRACE_DBG("offset(0x%x) size(0x%x) frame size(0x%x) data size(0x%x) MSHEAD OK\n", 
                offset, fd->size, MSHEAD_GETFRAMESIZE(pmshead), MSHEAD_GETMSDSIZE(pmshead));
            if (ISVIDEOFRAME(pmshead))
            {
                uint32 sec = MSHEAD_GET_TIMESTAMP_SEC(pmshead);
                
                TRACE_DBG("VIDEO: width(%d) height(%d) sec(%d) msec(%d)\n", MSHEAD_GET_VIDEO_WIDTH(pmshead), 
                    MSHEAD_GET_VIDEO_HEIGHT(pmshead), sec, MSHEAD_GET_TIMESTAMP_MSEC(pmshead));
                if (sec)
                {
                    time_sec2time(sec, &systm);
                    TRACE_DBG(TIME_STR"\n", systm.year + 2000, systm.month, systm.day, systm.hour, systm.min, systm.min);
                }
            }
        }
        else
        {
            TRACE_ERR("offset(0x%x-0x%x) size(0x%x) MSHEAD ERROR\n", offset, pmshead->flag, fd->size);
            if (HB_FALSE == en_find_mshead)
            {
                en_find_mshead = HB_TRUE;
                if ((offset = _fifo_find_mshead(fd->space, offset, fd->size)) > 0)
                {
                    continue;
                }
            }
            break;
        }
        offset = FIFO_CALC_SIZE(offset, MSHEAD_GETFRAMESIZE(pmshead), fd->size);
    }

    return 0;
}

static sint32 _fifo_dump_data(PFIFO_FD_S fd, FIFO_CONSUMER_E consumer, uint32 ci)
{
    _fifo_dump_info(fd, FIFO_CONSUMER_ALL);
    
    if (FIFO_CONSUMER_ALL == consumer)
    {
        sint32 i = FIFO_CONSUMER_FIR;
        
        for (; i < fd->maxch; i++)
        {
            if (FIFO_CONSUMER_STAT(fd, i))
            {
                _fifo_dump_data_find_mshead(fd, i, ci);
            }
        }
    }
    else if ((consumer >= FIFO_CONSUMER_FIR) && (consumer < FIFO_CONSUMER_MAX))
    {
        if (0 == FIFO_CONSUMER_STAT(fd, consumer))
        {
            TRACE_ERR("consumer(%d) not enable\n", consumer);
            return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
        }
        _fifo_dump_data_find_mshead(fd, consumer, ci);
    }
    else
    {
        TRACE_ERR("unsupported consumer(%d)\n", consumer);
        return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
    }    
    
    return 0;
}


/******************************************************************************
 * 函数介绍: 创建FIFO(当FIFO大小大于等于128KB时,FIFO将增加拼接缓冲区128KB)
 * 输入参数: size: FIFO大小,单位bytes,为优化运算,size必须等于1<<N
 *           hiwater: 高警戒位大小,单位%,0~100
 *           lowater: 低警戒位大小,单位%,0~100
 * 输出参数: 无
 * 返回值  : <0-失败,>0-FIFO句柄
 *****************************************************************************/
sint32 fifo_open(uint32 size, uint32 hiwater, uint32 lowater)
{
    sint32 handle = 0, rtval = 0;
    /* size大于等于128K时,增加缓冲128K,用于不连续数据拼接 */
    uint32 resv_size = MUX(size >= FIFO_DATA_SIZE_MORE, FIFO_RESV_SIZE_DEF, 0);

    if ((handle = _fifo_open(0, FIFO_CONSUMER_DNUM, size, FIFO_HTWATER_DEF, hiwater, lowater, resv_size)) > 0)
    {
        if ((rtval = _fifo_open(handle, FIFO_CONSUMER_FIR, size, FIFO_HTWATER_DEF, hiwater, lowater, resv_size)) < 0)
        {
            _fifo_close((PFIFO_FD_S)handle);
            return rtval;
        }
    }

    return handle;
}

/******************************************************************************
 * 函数介绍: 创建FIFO扩展接口
 * 输入参数: size: FIFO大小,单位bytes,为优化运算,size必须等于1<<N
 *           hiwater: 高警戒位大小,单位%,0~100
 *           lowater: 低警戒位大小,单位%,0~100
 *           resv_size: 保留FIFO大小,单位bytes,为优化运算,resv_size必须等于1<<N,
 *                      主要用于音视频数据不连续数据拼接,请根据音视频最大帧长度设置;
 *                      如果大小为0时,拼接情况下,将由外部提前申请内存好内存,通过fifo_ioctrl
 *                      的fifo_ioctrl命令FIFO_CMD_GETLINKSTAT查询后,通过fifo_readframe读出
 * 输出参数: 无
 * 返回值  : <0-失败,>0-FIFO句柄
 *****************************************************************************/
sint32 fifo_openext(uint32 size, uint32 hiwater, uint32 lowater, uint32 resv_size)
{
    sint32 handle = 0, rtval = 0;

    if ((handle = _fifo_open(0, FIFO_CONSUMER_DNUM, size, FIFO_HTWATER_DEF, hiwater, lowater, resv_size)) > 0)
    {
        if ((rtval = _fifo_open(handle, FIFO_CONSUMER_FIR, size, FIFO_HTWATER_DEF, hiwater, lowater, resv_size)) < 0)
        {
            _fifo_close((PFIFO_FD_S)handle);
            return rtval;
        }
    }

    return handle;
}

/******************************************************************************
 * 函数介绍: 根据不同的消费者创建FIFO(每次只能创建一个消费者,第一创建时,只是创建FIFO,不生成任何消费者)
 * 输入参数: handle: FIFO句柄,首次创建时,句柄需要<=0,其他-已创建的FIFO句柄
 *           consumer: 消费者序号,每个消费者只能创建一次,创建消费者时(FIFO_CONSUMER_E):
 *                     FIFO_CONSUMER_NUL-创建一个未使用的消费者,其他-创建相应的消费者
 *           size: FIFO大小,单位bytes,为优化运算,size必须等于1<<N(首次创建时有效,其他时无效)
 *           htwater: 最高警戒位大小(单位为%,hiwater~100)
 *           hiwater: 高警戒位大小(单位为%,lowater~htwater)
 *           lowater: 低警戒位大小(单位为%,0~hiwater)
 *           resv_size: 保留FIFO大小,单位bytes(首次创建时有效,其他时无效)
 *                      (为优化运算,resv_size必须等于1<<N,主要用于音视频数据不连续数据拼接,
 *                      请根据音视频最大帧长度设置;如果大小为0时,拼接情况下,将由外部提前申请
 *                      内存好内存,通过fifo_ioctrl的fifo_ioctrl命令FIFO_CMD_GETLINKSTAT查询后,
 *                      通过fifo_readframe读出)
 * 输出参数: 无
 * 返回值  : <0-失败,>=0-第一创建时,返回FIFO句柄,其他返回消费者序号(FIFO_CONSUMER_E)
 *****************************************************************************/
sint32 fifo_openmulti(sint32 handle, FIFO_CONSUMER_E consumer, uint32 size, 
    uint32 htwater, uint32 hiwater, uint32 lowater, uint32 resv_size)
{
    return _fifo_open(handle, consumer, size, htwater, hiwater, lowater, resv_size);
}

/******************************************************************************
 * 函数介绍: 写入数据到FIFO,该函数一次写入长度size数据,当FIFO中缓冲不足时将返回错误信息
 * 输入参数: handle: FIFO句柄
 *           data: 缓冲区指针
 *           size: 待写数据长度
 * 输出参数: 无
 * 返回值  : >0-写入数据大小,<0-错误代码
 *****************************************************************************/
sint32 fifo_write(sint32 handle, char *data, uint32 size)
{
    PFIFO_FD_S fd = (PFIFO_FD_S)handle;
    sint32 free_size = 0;

    if ((handle <= 0) || (0 == FLAG_VERIFY(fd)))
    {
        TRACE_ERR("handle(0x%x) invalid\n", handle);
        return ERRNO(COMMON_ERROR_HANDLEINVAL, COMP_NENT);
    }

    if ((NULL == data) || (0 == size))
    {
    #ifdef ENABLE_FIFO_FILELOG
        TRACE_LOG("fifo_write handle(0x%x), unsupported data is NULL, or size(%d)\n", handle, size);
    #else
    #ifndef _WIN32
        TRACE_ASSERT((NULL != data) && (0 != size), 
            "fifo_write handle(0x%x), unsupported data is NULL, or size(%d)\n", handle, size);
    #endif /* _WIN32 */
    #endif /* ENABLE_FIFO_FILELOG */
        return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
    }

    if ((free_size = _fifo_minfreesize(fd)) < size) /* BUF空间不够 */
    {
        TRACE_ERR("handle(0x%x) size(%d) free_size(%d) is full\n", handle, size, free_size);
        return ERRNO(FIFO_ERROR_NOBUF, COMP_NENT);
    }
#if 1 //ndef _WIN32
    if (fd->part_allfrm_stat)
    {
        uint32 part_allfrm_wtsz = fd->part_allfrm_wtsz + size;
        
        if (ISMSHEAD(data))
        {
            if (fd->dbg_err_count < FIFO_DBG_ERR_COUNT)
            {
            #ifdef ENABLE_FIFO_FILELOG
                TRACE_LOG("handle(0x%x) dbg_err_count(%d) ISMSHEAD(%d) part_allfrm_wtsz(%d) part_allfrm_tlsz(%d)\n", 
                    handle, fd->dbg_err_count, ISMSHEAD(data), part_allfrm_wtsz, fd->part_allfrm_tlsz);
            #endif /* ENABLE_FIFO_FILELOG */
                TRACE_ERR("handle(0x%x) dbg_err_count(%d) ISMSHEAD(%d) part_allfrm_wtsz(%d) part_allfrm_tlsz(%d)\n", 
                    handle, fd->dbg_err_count, ISMSHEAD(data), part_allfrm_wtsz, fd->part_allfrm_tlsz);
            }
        }

        if (part_allfrm_wtsz > fd->part_allfrm_tlsz)
        {
            if (fd->dbg_err_count++ < FIFO_DBG_ERR_COUNT)
            {
            #ifdef ENABLE_FIFO_FILELOG
                TRACE_LOG("handle(0x%x) dbg_err_count(%d) ISMSHEAD(%d) part_allfrm_wtsz(%d) part_allfrm_tlsz(%d)\n", 
                    handle, fd->dbg_err_count, ISMSHEAD(data), part_allfrm_wtsz, fd->part_allfrm_tlsz);
            #endif /* ENABLE_FIFO_FILELOG */
                TRACE_ERR("handle(0x%x) dbg_err_count(%d) ISMSHEAD(%d) part_allfrm_wtsz(%d) part_allfrm_tlsz(%d)\n", 
                    handle, fd->dbg_err_count, ISMSHEAD(data), part_allfrm_wtsz, fd->part_allfrm_tlsz);
            }
            return ERRNO(FIFO_ERROR_UNMATCHMSHEAD, COMP_NENT);
        }
        FIFO_WRITE(fd, data, size);
        if (part_allfrm_wtsz == fd->part_allfrm_tlsz)
        {
            uint32 part_allfrm_posi = fd->part_allfrm_posi;
            MSHEAD mshead, *pmshead = (PMSHEAD)&fd->space[part_allfrm_posi];

            if (_fifo_check_posi(fd, part_allfrm_posi, MSHEAD_LEN))
            { /* 如果MSHEAD头信息不连续,需要提前取出才能处理 */
                pmshead = &mshead;
                _fifo_read_posi(fd, part_allfrm_posi, (char *)pmshead, MSHEAD_LEN);
                TRACE_DBG("part all frame head is incomplete\n");
            }
            _fifo_framedeal(fd, pmshead, part_allfrm_posi);
            fd->part_allfrm_stat = FIFO_FRAME_FULL;
        }
        else
        {
            fd->part_allfrm_wtsz = part_allfrm_wtsz;
        }
        TRACE_REL("handle(0x%x) size(%d)\n", handle, size);
    }
    else
#endif /* _WIN32 */
    {
        FIFO_WRITE(fd, data, size);
        TRACE_REL("handle(0x%x) size(%d)\n", handle, size);
    }

    return size;
}

#if 1 //ndef _WIN32
/******************************************************************************
 * 函数介绍: 写入数据到FIFO,该函数一次写入长度size数据,当FIFO中缓冲不足时将返回错误信息
 *           此函数是fifo_write的扩展,为音视频流写入而设计,会对MSHEAD头信息做判断,
 *           计算预录时间和帧计数做相应处理
 * 输入参数: handle: FIFO句柄
 *           data: 缓冲区指针
 *           size: 待写数据长度
 * 输出参数: 无
 * 返回值  : >0-写入数据大小,<0-错误代码
 *****************************************************************************/
sint32 fifo_writeext(sint32 handle, char *data, uint32 size)
{
    PFIFO_FD_S fd = (PFIFO_FD_S)handle;
    PMSHEAD pmshead = (PMSHEAD)data;

    if ((handle <= 0) || (0 == FLAG_VERIFY(fd)))
    {
        TRACE_ERR("handle(0x%x) invalid\n", handle);
        return ERRNO(COMMON_ERROR_HANDLEINVAL, COMP_NENT);
    }

    if ((NULL == data) || (size < MSHEAD_LEN))
    {
    #ifdef ENABLE_FIFO_FILELOG
        TRACE_LOG("fifo_writeext handle(0x%x), unsupported data is NULL, or size(%d) < MSHEAD_LEN(%d)\n", 
            handle, size, MSHEAD_LEN);
    #else
#ifndef _WIN32
        TRACE_ASSERT((NULL != data) && (size >= MSHEAD_LEN), 
            "fifo_writeext handle(0x%x), unsupported data is NULL, or size(%d) < MSHEAD_LEN(%d)\n", handle, size, MSHEAD_LEN);
#endif
    #endif /* ENABLE_FIFO_FILELOG */
        return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
    }

    if (ISMSHEAD(pmshead))
    {
        uint32 pi_posi_back = fd->pi, free_size = 0;
        uint32 frame_size = MSHEAD_GETFRAMESIZE(pmshead);

        if ((size > frame_size) || (fd->part_allfrm_stat)) /* 帧长度不对或前一帧数据不完整,不允许再次写入帧头信息 */
        {
            if (fd->dbg_err_count++ < FIFO_DBG_ERR_COUNT)
            {
            #ifdef ENABLE_FIFO_FILELOG
                TRACE_LOG("unsupported dbg_err_count(%d) handle(0x%x) size(%d) > frame_size(%d) or part_allfrm_stat(%d)\n", 
                    handle, fd->dbg_err_count, size, frame_size, fd->part_allfrm_stat);
            #endif /* ENABLE_FIFO_FILELOG */
                TRACE_ERR("unsupported dbg_err_count(%d) handle(0x%x) size(%d) > frame_size(%d) or part_allfrm_stat(%d)\n", 
                    handle, fd->dbg_err_count, size, frame_size, fd->part_allfrm_stat);
            }
            return ERRNO(FIFO_ERROR_UNMATCHMSHEAD, COMP_NENT);
        }
        if ((free_size = _fifo_minfreesize(fd)) < size) /* BUF空间不够 */
        {
            TRACE_ERR("handle(0x%x) no free_size(%d) buffer to write(%d)\n", handle, free_size, size);
            return ERRNO(FIFO_ERROR_NOBUF, COMP_NENT);
        }

        FIFO_WRITE(fd, data, size);
        if (size == frame_size) /* 数据长度与帧长度相等,完整帧一次性写入 */
        {
            _fifo_framedeal(fd, pmshead, pi_posi_back);
        }
        else
        {
            fd->part_allfrm_wtsz = size;
            fd->part_allfrm_tlsz = frame_size;
            fd->part_allfrm_posi = pi_posi_back;
            fd->part_allfrm_stat = FIFO_FRAME_PART;
        }
        TRACE_REL("handle(0x%x) size(%d)\n", handle, size);
    }
    else
    {
        TRACE_ERR("handle(0x%x) flag(0x%x)\n", handle, pmshead->flag);
        return ERRNO(FIFO_ERROR_MSHEAD, COMP_NENT);
    }

    return size;
}
#endif /* _WIN32 */

/******************************************************************************
 * 函数介绍: 获取FIFO中可写的缓冲区信息
 * 输入参数: handle: FIFO句柄
 *           pbuff_info: 缓冲区信息结构体指针
 *           need_size: 需要的缓冲区大小
 * 输出参数: 无
 * 返回值  : 0-成功,<0-错误代码
 *****************************************************************************/
sint32 fifo_write_buffer_get(sint32 handle, PFIFO_BUFF_INFO_S pbuff_info, uint32 need_size)
{
    PFIFO_FD_S fd = (PFIFO_FD_S)handle;

    if ((handle <= 0) || (0 == FLAG_VERIFY(fd)))
    {
        TRACE_ERR("handle(0x%x) invalid\n", handle);
        return ERRNO(COMMON_ERROR_HANDLEINVAL, COMP_NENT);
    }

    if (pbuff_info)
    {
        uint32 free_size = _fifo_minfreesize(fd), limit_size = fd->size - fd->pi;
        
        pbuff_info->buff[0].pbuff = fd->space + fd->pi;
        if (free_size > limit_size)
        {
            pbuff_info->buff[0].size = limit_size;
            pbuff_info->buff[1].pbuff = fd->space;
            pbuff_info->buff[1].size = free_size - limit_size;
            pbuff_info->buff_count = 2;
        }
        else
        {
            pbuff_info->buff[0].size = free_size;
            pbuff_info->buff[1].pbuff = NULL;
            pbuff_info->buff[1].size = 0;
            pbuff_info->buff_count = 1;
        }
    }
    else
    {
        TRACE_ERR("handle(0x%x) unsupported pbuff_info is NULL\n", handle);
        return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
    }
    
    return 0;
}

/******************************************************************************
 * 函数介绍: 更新FIFO中可写的缓冲区信息
 * 输入参数: handle: FIFO句柄
 *           pbuff_info: 缓冲区信息结构体指针
 *           update_size: 更新的缓冲区大小
 * 输出参数: 无
 * 返回值  : 0-成功,<0-错误代码
 *****************************************************************************/
sint32 fifo_write_buffer_update(sint32 handle, PFIFO_BUFF_INFO_S pbuff_info, uint32 update_size)
{
    PFIFO_FD_S fd = (PFIFO_FD_S)handle;

    if ((handle <= 0) || (0 == FLAG_VERIFY(fd)))
    {
        TRACE_ERR("handle(0x%x) invalid\n", handle);
        return ERRNO(COMMON_ERROR_HANDLEINVAL, COMP_NENT);
    }

    switch (pbuff_info->buff_head)
    {
#if 1 //ndef _WIN32
    case FIFO_BUFF_HEAD_MSHEAD:
    {
        PMSHEAD pmshead = (PMSHEAD)pbuff_info->buff[0].pbuff;

        if (ISMSHEAD(pmshead))
        {            
            MSHEAD mshead;

            if (pbuff_info->buff[0].size < MSHEAD_LEN)
            { /* 如果MSHEAD头信息不连续,需要提前取出才能处理 */
                pmshead = &mshead;
                _fifo_read_buffer(pbuff_info, (char *)pmshead, MSHEAD_LEN);
                TRACE_DBG("part all frame head is incomplete\n");
            }
            if (MSHEAD_GETFRAMESIZE(pmshead) == update_size) /* 数据长度与帧长度相等,完整帧一次性写入 */
            {
                _fifo_framedeal(fd, pmshead, fd->pi);
            }
            else
            {
                TRACE_ERR("unsupported handle(0x%x) frame size(%d) update_size(%d)\n", 
                    handle, MSHEAD_GETFRAMESIZE(pmshead), update_size);
                return ERRNO(FIFO_ERROR_UNMATCHMSHEAD, COMP_NENT);
            }
            TRACE_REL("handle(0x%x) update_size(%d)\n", handle, update_size);
        }
        else
        {
            TRACE_ERR("handle(0x%x) flag(0x%x)\n", handle, pmshead->flag);
            return ERRNO(FIFO_ERROR_MSHEAD, COMP_NENT);
        }
        break;
    }
#endif /* _WIN32 */
    case FIFO_BUFF_HEAD_DATA:
    default:
        break;
    }
    FIFO_WRITE_UPDATE(fd, update_size);
    
    return 0;
}

/******************************************************************************
 * 函数介绍: 从FIFO中读取数据,该函数一次读取长度size数据,当FIFO中数据不足时将返回错误信息
 * 输入参数: handle: FIFO句柄
 *           data: 缓冲区指针(此值可以为空(NULL),为空时,数据将不会被拷贝)
 *           size: 读取大小
 *           peek: 是否清除FIFO中已读数据(FIFO_PEEK_E)
 * 输出参数: 无
 * 返回值  : >0-读出数据大小,<0-错误代码
 *****************************************************************************/
sint32 fifo_read(sint32 handle, char *data, uint32 size, FIFO_PEEK_E peek)
{
    PFIFO_FD_S fd = (PFIFO_FD_S)handle;
    FIFO_CONSUMER_E consumer = FIFO_CONSUMER_FIR;

    if ((handle <= 0) || (0 == FLAG_VERIFY(fd)) || (0 == FIFO_CONSUMER_STAT(fd, consumer)))
    {
        TRACE_ERR("handle(0x%x)[%d] invalid\n", handle, consumer);
        return ERRNO(COMMON_ERROR_HANDLEINVAL, COMP_NENT);
    }
    
    return _fifo_read(fd, consumer, data, size, peek);
}

/******************************************************************************
 * 函数介绍: 根据不同的消费者从FIFO中读取数据,该函数一次读取长度size数据,当FIFO中数据不足时将返回错误信息
 * 输入参数: handle: FIFO句柄
 *           consumer: 消费者序号(FIFO_CONSUMER_E)
 *           data: 缓冲区指针(此值可以为空(NULL),为空时,数据将不会被拷贝)
 *           size: 读取大小
 *           peek: 是否清除FIFO中已读数据(FIFO_PEEK_E)
 * 输出参数: 无
 * 返回值  : >0-读出数据大小,<0-错误代码
 *****************************************************************************/
sint32 fifo_readmulti(sint32 handle, FIFO_CONSUMER_E consumer, char *data, uint32 size, FIFO_PEEK_E peek)
{
    PFIFO_FD_S fd = (PFIFO_FD_S)handle;

    if ((handle <= 0) || (0 == FLAG_VERIFY(fd)) || (0 == FIFO_CONSUMER_STAT(fd, consumer)))
    {
        TRACE_ERR("handle(0x%x)[%d] invalid\n", handle, consumer);
        return ERRNO(COMMON_ERROR_HANDLEINVAL, COMP_NENT);
    }
    
    return _fifo_read(fd, consumer, data, size, peek);
}

#if 1 //ndef _WIN32
/******************************************************************************
 * 函数介绍: 从FIFO中读取数据,该函数一次读取1帧数据,当FIFO中数据不足时将返回错误信息
 * 输入参数: handle: FIFO句柄
 *           data: 缓冲区指针(此值可以为空(NULL),为空时,数据将不会被拷贝)
 *           limit: 缓冲区大小(data为空(NULL)时,此值可填无限大)
 *           peek: 是否清除FIFO中已读数据(FIFO_PEEK_E)
 * 输出参数: 无
 * 返回值  : >0-读出数据大小,0-缓冲区不足,<0-错误代码
 *****************************************************************************/
sint32 fifo_readframe(sint32 handle, char *data, uint32 limit, FIFO_PEEK_E peek)
{
    return _fifo_readframe(handle, FIFO_CONSUMER_FIR, data, limit, peek);
}

/******************************************************************************
 * 函数介绍: 根据不同的消费者从FIFO中读取数据,该函数一次读取1帧数据,当FIFO中数据不足时将返回错误信息
 * 输入参数: handle: FIFO句柄
 *           consumer: 消费者序号(FIFO_CONSUMER_E)
 *           data: 缓冲区指针(此值可以为空(NULL),为空时,数据将不会被拷贝)
 *           limit: 缓冲区大小(data为空(NULL)时,此值可填无限大)
 *           peek: 是否清除FIFO中已读数据(FIFO_PEEK_E)
 * 输出参数: 无
 * 返回值  : >0-读出数据大小,0-缓冲区不足,<0-错误代码
 *****************************************************************************/
sint32 fifo_readframemulti(sint32 handle, FIFO_CONSUMER_E consumer, char *data, uint32 limit, FIFO_PEEK_E peek)
{
    return _fifo_readframe(handle, consumer, data, limit, peek);
}

/******************************************************************************
 * 函数介绍: 从FIFO中读取数据,该函数一次读取多帧音视频数据,直至遇到下一个关键帧
 * 输入参数: handle: FIFO句柄
 *           data: 缓冲区指针(此值可以为空(NULL),为空时,数据将不会被拷贝)
 *           limit: 缓冲区大小(data为空(NULL)时,此值可填无限大)
 *           nexttype: 下一帧数据类型
 * 输出参数: 无
 * 返回值  : >0-读出数据大小,<0-错误代码
 *****************************************************************************/
sint32 fifo_readext(sint32 handle, char *data, uint32 limit, sint32 *nexttype)
{
    return _fifo_readext(handle, FIFO_CONSUMER_FIR, data, limit, nexttype);
}

/******************************************************************************
 * 函数介绍: 根据不同的消费者从FIFO中读取数据,该函数一次读取多帧音视频数据,直至遇到下一个关键帧
 * 输入参数: handle: FIFO句柄
 *           consumer: 消费者序号(FIFO_CONSUMER_E)
 *           data: 缓冲区指针(此值可以为空(NULL),为空时,数据将不会被拷贝)
 *           limit: 缓冲区大小(data为空(NULL)时,此值可填无限大)
 *           nexttype: 下一帧数据类型
 * 输出参数: 无
 * 返回值  : >0-读出数据大小,<0-错误代码
 *****************************************************************************/
sint32 fifo_readextmulti(sint32 handle, FIFO_CONSUMER_E consumer, char *data, uint32 limit, sint32 *nexttype)
{
    return _fifo_readext(handle, consumer, data, limit, nexttype);
}

/******************************************************************************
 * 函数介绍: 从FIFO中读取数据,该函数一次读取1帧最新的完整的I帧数据
 * 输入参数: handle: FIFO句柄
 *           data: 缓冲区指针
 *           limit: 缓冲区大小
 *           have_mshead: 是否带有MSHEAD(HB_TRUE-有,HB_FALSE-无)
 * 输出参数: 无
 * 返回值  : >0-读出数据大小,<0-错误代码
 *****************************************************************************/
sint32 fifo_read_newfull_keyframe(sint32 handle, char *data, uint32 limit, HB_BOOL have_mshead)
{
    PFIFO_FD_S fd = (PFIFO_FD_S)handle;

    if ((handle <= 0) || (0 == FLAG_VERIFY(fd)))
    {
        TRACE_ERR("handle(0x%x) is invalid\n", handle);
        return ERRNO(COMMON_ERROR_HANDLEINVAL, COMP_NENT);
    }

    if ((NULL == data) || (limit < MSHEAD_LEN))
    {
        TRACE_ERR("unsupported handle(0x%x) data is NULL, or limit(%d) < MSHEAD_LEN(%d)\n", handle, limit, MSHEAD_LEN);
        return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
    }
    if (fd->full_keyfrm_stat && (0 == (fd->full_keyfrm_stat & FIFO_KEYFRM_OVERLAY)))
    {
        uint32 full_keyfrm_posi = fd->full_keyfrm_posi;
        MSHEAD mshead, *pmshead = (PMSHEAD)&fd->space[full_keyfrm_posi];
        
        if (_fifo_check_posi(fd, full_keyfrm_posi, MSHEAD_LEN))
        { /* 如果MSHEAD头信息不连续,需要提前取出才能处理 */
            pmshead = &mshead;
            _fifo_read_posi(fd, full_keyfrm_posi, (char *)pmshead, MSHEAD_LEN);
            TRACE_DBG("full key frame head is incomplete\n");
        }
        if (ISMSHEAD(pmshead) & ISVIDEOFRAME(pmshead) & ISKEYFRAME(pmshead))
        {
            sint32 frame_size = MSHEAD_GETFRAMESIZE(pmshead);

            if (limit < frame_size)
            {
                TRACE_ERR("unsupported handle(0x%x) limit(%d) < frame_size(%d)\n", handle, limit, frame_size);
                return ERRNO(FIFO_ERROR_EXTBUFTOSMALL, COMP_NENT);
            }
            if (have_mshead)
            {
                _fifo_read_posi(fd, full_keyfrm_posi, data, frame_size);
            }
            else
            {
                _fifo_read_posi(fd, _fifo_calc_posi(fd, full_keyfrm_posi, 
                    MSHEAD_GETMSHSIZE(pmshead)), data, MSHEAD_GETMSDSIZE(pmshead));
            }
            TRACE_DBG("handle(0x%x) frame_size(%d)\n", handle, frame_size);

            return frame_size;
        }
        else
        {
            TRACE_ERR("unsupported handle(0x%x) ISMSHEAD(%d) ISVIDEOFRAME(%d) ISKEYFRAME(%d)\n",
                handle, ISMSHEAD(pmshead), ISVIDEOFRAME(pmshead), ISKEYFRAME(pmshead));
            return ERRNO(FIFO_ERROR_UNMATCHMSHEAD, COMP_NENT);
        }
    }
    TRACE_ERR("handle(0x%x) FIFO_ERROR_KEYFRMNOTEXIST\n", handle);

    return ERRNO(FIFO_ERROR_KEYFRMNOTEXIST, COMP_NENT);
}
#endif /* _WIN32 */

/******************************************************************************
 * 函数介绍: 获取FIFO中可读的缓冲区信息
 * 输入参数: handle: FIFO句柄
 *           pbuff_info: 缓冲区信息结构体指针
 *           need_size: 需要的缓冲区大小
 * 输出参数: 无
 * 返回值  : >=0-成功,缓冲区总长度,<0-错误代码
 *****************************************************************************/
sint32 fifo_read_buffer_get(sint32 handle, PFIFO_BUFF_INFO_S pbuff_info)
{
    return _fifo_read_buffer_get(handle, FIFO_CONSUMER_FIR, pbuff_info);
}

/******************************************************************************
 * 函数介绍: 根据不同的消费者获取FIFO中可读的缓冲区信息
 * 输入参数: handle: FIFO句柄
 *           consumer: 消费者序号(FIFO_CONSUMER_E)
 *           pbuff_info: 缓冲区信息结构体指针
 *           need_size: 需要的缓冲区大小
 * 输出参数: 无
 * 返回值  : >=0-成功,缓冲区总长度,<0-错误代码
 *****************************************************************************/
sint32 fifo_read_buffer_getmulti(sint32 handle, FIFO_CONSUMER_E consumer, PFIFO_BUFF_INFO_S pbuff_info)
{
    return _fifo_read_buffer_get(handle, consumer, pbuff_info);
}

/******************************************************************************
 * 函数介绍: 更新FIFO中可读的缓冲区信息
 * 输入参数: handle: FIFO句柄
 *           pbuff_info: 缓冲区信息结构体指针
 *           update_size: 更新的缓冲区大小
 * 输出参数: 无
 * 返回值  : 0-成功,<0-错误代码
 *****************************************************************************/
sint32 fifo_read_buffer_update(sint32 handle, PFIFO_BUFF_INFO_S pbuff_info, uint32 update_size)
{
    return _fifo_read_buffer_update(handle, FIFO_CONSUMER_FIR, pbuff_info, update_size);
}

/******************************************************************************
 * 函数介绍: 根据不同的消费者更新FIFO中可读的缓冲区信息
 * 输入参数: handle: FIFO句柄
 *           consumer: 消费者序号(FIFO_CONSUMER_E)
 *           pbuff_info: 缓冲区信息结构体指针
 *           update_size: 更新的缓冲区大小
 * 输出参数: 无
 * 返回值  : 0-成功,<0-错误代码
 *****************************************************************************/
sint32 fifo_read_buffer_updatemulti(sint32 handle, FIFO_CONSUMER_E consumer, PFIFO_BUFF_INFO_S pbuff_info, uint32 update_size)
{
    return _fifo_read_buffer_update(handle, consumer, pbuff_info, update_size);
}

/******************************************************************************
 * 函数介绍: 从FIFO中读取数据并处理,该函数一次读取一帧视频数据和N个音频帧
 * 输入参数: handle: FIFO句柄
 *           func: 数据处理回调函数
 *           func_idx: datacallback()函数中的idx
 *           func_handle: datacallback()函数中的fifo,此处用于回调函数的句柄,例如MFS句柄等等
 * 输出参数: 无
 * 返回值  : >0-已处理数据大小,0-数据不足一帧,或FIFO的resv_size为0时,FIFO帧处理要做掉头处理,<0-错误代码
 *****************************************************************************/
sint32 fifo_frameprocess(sint32 handle, datacallback func, sint32 func_idx, sint32 func_handle)
{
    return _fifo_dataprocess(handle, FIFO_CONSUMER_FIR, FIFO_DATAPROCESS_ONE_VIDEO_MULTY_AUDIO_FRAME, 
        func, func_idx, func_handle, FIFO_DEAL_IFRM_NUMS);
}

/******************************************************************************
 * 函数介绍: 根据不同的消费者从FIFO中读取数据并处理,该函数一次读取一帧视频数据和N个音频帧
 * 输入参数: handle: FIFO句柄
 *           consumer: 消费者序号(FIFO_CONSUMER_E)
 *           func: 数据处理回调函数
 *           func_idx: datacallback()函数中的idx
 *           func_handle: datacallback()函数中的fifo,此处用于回调函数的句柄,例如MFS句柄等等
 * 输出参数: 无
 * 返回值  : >0-已处理数据大小,0-数据不足一帧,或FIFO的resv_size为0时,FIFO帧处理要做掉头处理,<0-错误代码
 *****************************************************************************/
sint32 fifo_frameprocessmulti(sint32 handle, FIFO_CONSUMER_E consumer, 
    datacallback func, sint32 func_idx, sint32 func_handle)
{
    return _fifo_dataprocess(handle, consumer, FIFO_DATAPROCESS_ONE_VIDEO_MULTY_AUDIO_FRAME, 
        func, func_idx, func_handle, FIFO_DEAL_IFRM_NUMS);
}

/******************************************************************************
 * 函数介绍: 从FIFO中读取一帧视频或音频数据并处理,该函数一次只读取一帧音视频或音频数据
 * 输入参数: handle: FIFO句柄
 *           func: 数据处理回调函数
 *           func_idx: datacallback()函数中的idx
 *           func_handle: datacallback()函数中的fifo,此处用于回调函数的句柄,例如MFS句柄等等
 *           resvered: 保留参数
 * 输出参数: 无
 * 返回值  : >0-已处理数据大小,0-数据不足一帧,或FIFO的resv_size为0时,FIFO帧处理要做掉头处理,<0-错误代码
 *****************************************************************************/
sint32 fifo_frameprocessext(sint32 handle, datacallback func, sint32 func_idx, sint32 func_handle, sint32 resvered)
{
    return _fifo_dataprocess(handle, FIFO_CONSUMER_FIR, FIFO_DATAPROCESS_ONE_VIDEO_OR_AUDIO_FRAME, 
        func, func_idx, func_handle, FIFO_DEAL_IFRM_NUMS);
}

/******************************************************************************
 * 函数介绍: 根据不同的消费者从FIFO中读取一帧视频或音频数据并处理,该函数一次只读取一帧音视频或音频数据
 * 输入参数: handle: FIFO句柄
 *           consumer: 消费者序号(FIFO_CONSUMER_E)
 *           func: 数据处理回调函数
 *           func_idx: datacallback()函数中的idx
 *           func_handle: datacallback()函数中的fifo,此处用于回调函数的句柄,例如MFS句柄等等
 *           resvered: 保留参数
 * 输出参数: 无
 * 返回值  : >0-已处理数据大小,0-数据不足一帧,或FIFO的resv_size为0时,FIFO帧处理要做掉头处理,<0-错误代码
 *****************************************************************************/
sint32 fifo_frameprocessextmulti(sint32 handle, FIFO_CONSUMER_E consumer, 
    datacallback func, sint32 func_idx, sint32 func_handle, sint32 resvered)
{
    return _fifo_dataprocess(handle, consumer, FIFO_DATAPROCESS_ONE_VIDEO_OR_AUDIO_FRAME, 
        func, func_idx, func_handle, FIFO_DEAL_IFRM_NUMS);
}

/******************************************************************************
 * 函数介绍: 从FIFO中读取数据并处理,该函数一次读取多帧音视频数据,直至遇到下一个视频关键帧
 * 输入参数: handle: FIFO句柄
 *           func: 数据处理回调函数
 *           func_idx: datacallback()函数中的idx
 *           func_handle: datacallback()函数中的fifo,此处用于回调函数的句柄,例如MFS句柄等等
 * 输出参数: 无
 * 返回值  : >0-已处理数据大小,0-数据不足一帧,或FIFO的resv_size为0时,FIFO帧处理要做掉头处理,<0-错误代码
 *****************************************************************************/
sint32 fifo_datprocess(sint32 handle, datacallback func, sint32 func_idx, sint32 func_handle)
{
    return _fifo_dataprocess(handle, FIFO_CONSUMER_FIR, FIFO_DATAPROCESS_MULTY_VIDEO_MULTY_AUDIO_FRAME, 
        func, func_idx, func_handle, FIFO_DEAL_IFRM_NUMS);
}

/******************************************************************************
 * 函数介绍: 根据不同的消费者从FIFO中读取数据并处理,该函数一次读取多帧音视频数据,直至遇到下一个视频关键帧
 * 输入参数: handle: FIFO句柄
 *           consumer: 消费者序号(FIFO_CONSUMER_E)
 *           func: 数据处理回调函数
 *           func_idx: datacallback()函数中的idx
 *           func_handle: datacallback()函数中的fifo,此处用于回调函数的句柄,例如MFS句柄等等
 * 输出参数: 无
 * 返回值  : >0-已处理数据大小,0-数据不足一帧,或FIFO的resv_size为0时,FIFO帧处理要做掉头处理,<0-错误代码
 *****************************************************************************/
sint32 fifo_datprocessmulti(sint32 handle, FIFO_CONSUMER_E consumer, 
    datacallback func, sint32 func_idx, sint32 func_handle)
{
    return _fifo_dataprocess(handle, consumer, FIFO_DATAPROCESS_MULTY_VIDEO_MULTY_AUDIO_FRAME, 
        func, func_idx, func_handle, FIFO_DEAL_IFRM_NUMS);
}

/******************************************************************************
 * 函数介绍: 从FIFO中读取数据并处理,该函数一次读取多帧音视频数据,直至处理完成所需要处理视频关键帧数
 * 输入参数: handle: FIFO句柄
 *           func: 数据处理回调函数
 *           func_idx: datacallback()函数中的idx,
 *           func_handle: datacallback()函数中的fifo,此处用于回调函数的句柄,例如MFS句柄等等
 *           deal_ifrm_nums: 需要一次性处理关键帧个数,最小值为1
 * 输出参数: 无
 * 返回值  : >0-已处理数据大小,0-数据不足一帧,或FIFO的resv_size为0时,FIFO帧处理要做掉头处理,<0-错误代码
 *****************************************************************************/
sint32 fifo_datprocessext(sint32 handle, datacallback func, 
    sint32 func_idx, sint32 func_handle, sint32 deal_ifrm_nums)
{
    deal_ifrm_nums = MUX(deal_ifrm_nums < FIFO_DEAL_IFRM_NUMS, FIFO_DEAL_IFRM_NUMS, deal_ifrm_nums);

    return _fifo_dataprocess(handle, FIFO_CONSUMER_FIR, FIFO_DATAPROCESS_MULTY_VIDEO_MULTY_AUDIO_FRAME, 
        func, func_idx, func_handle, deal_ifrm_nums);
}

/******************************************************************************
 * 函数介绍: 根据不同的消费者从FIFO中读取数据并处理,该函数一次读取多帧音视频数据,
 *           直至处理完成所需要处理视频关键帧数
 * 输入参数: handle: FIFO句柄
 *           consumer: 消费者序号(FIFO_CONSUMER_E)
 *           func: 数据处理回调函数
 *           func_idx: datacallback()函数中的idx
 *           func_handle: datacallback()函数中的fifo,此处用于回调函数的句柄,例如MFS句柄等等
 *           deal_ifrm_nums: 需要一次性处理关键帧个数,最小值为1
 * 输出参数: 无
 * 返回值  : >0-已处理数据大小,0-数据不足一帧,或FIFO的resv_size为0时,FIFO帧处理要做掉头处理,<0-错误代码
 *****************************************************************************/
sint32 fifo_datprocessextmulti(sint32 handle, FIFO_CONSUMER_E consumer, 
    datacallback func, sint32 func_idx, sint32 func_handle, sint32 deal_ifrm_nums)
{
    deal_ifrm_nums = MUX(deal_ifrm_nums < FIFO_DEAL_IFRM_NUMS, FIFO_DEAL_IFRM_NUMS, deal_ifrm_nums);

    return _fifo_dataprocess(handle, consumer, FIFO_DATAPROCESS_MULTY_VIDEO_MULTY_AUDIO_FRAME, 
        func, func_idx, func_handle, deal_ifrm_nums);
}

/******************************************************************************
 * 函数介绍: FIFO重定位MSHEAH
 * 输入参数: handle: FIFO句柄
 *           consumer: 消费者序号(FIFO_CONSUMER_E)
 * 输出参数: 无
 * 返回值  : 0-成功,<0-错误代码
 *****************************************************************************/
sint32 fifo_relocate_mshead(sint32 handle, FIFO_CONSUMER_E consumer)
{
    PFIFO_FD_S fd = (PFIFO_FD_S)handle;
    sint32 mshead_offset = 0, data_size = 0, fifo_limit = 0;
    
    if ((handle <= 0) || (0 == FLAG_VERIFY(fd)) || (0 == FIFO_CONSUMER_STAT(fd, consumer)))
    {
        TRACE_ERR("handle(0x%x)[%d] invalid\n", handle, consumer);
        return ERRNO(COMMON_ERROR_HANDLEINVAL, COMP_NENT);
    }

    pthread_mutex_lock(&fd->pcm[consumer].mtx_ci);
    data_size = FIFO_DATA_SIZE(fd, consumer);
    fifo_limit = fd->size - fd->pcm[consumer].ci;
    TRACE_ERR("handle(0x%x)[%d] ci(%d) cci(%d) data_size(%d) fifo_limit(%d) dump error data:\n", 
        handle, consumer, fd->pcm[consumer].ci, fd->pcm[consumer].cci, data_size, fifo_limit);
    if (data_size <= fifo_limit)
    {
        if ((mshead_offset = _fifo_find_mshead(fd->space, fd->pcm[consumer].ci, fd->pi)) < 0)
        {
            data_hex_dump(&fd->space[fd->pcm[consumer].ci], fd->pi - fd->pcm[consumer].ci);
            pthread_mutex_unlock(&fd->pcm[consumer].mtx_ci);
            return mshead_offset;
        }
    }
    else
    {
        if ((mshead_offset = _fifo_find_mshead(fd->space, fd->pcm[consumer].ci, fd->size)) < 0)
        {
            data_hex_dump(&fd->space[fd->pcm[consumer].ci], fd->size - fd->pcm[consumer].ci);
            if ((mshead_offset = _fifo_find_mshead(fd->space, 0, fd->pi)) < 0)
            {
                data_hex_dump(fd->space, fd->pi);
                pthread_mutex_unlock(&fd->pcm[consumer].mtx_ci);
                return mshead_offset;
            }
            else
            {
                data_hex_dump(fd->space, mshead_offset);
            }
        }
    }
    data_hex_dump(&fd->space[fd->pcm[consumer].ci], mshead_offset - fd->pcm[consumer].ci);
    fd->pcm[consumer].cci = fd->pcm[consumer].ci = mshead_offset;
    TRACE_ERR("handle(0x%x)[%d] ci(%d) cci(%d) data_size(%d) fifo_limit(%d) lose_size(%d)\n", 
        handle, consumer, fd->pcm[consumer].ci, fd->pcm[consumer].cci, 
        data_size, fifo_limit, data_size - FIFO_DATA_SIZE(fd, consumer));
    pthread_mutex_unlock(&fd->pcm[consumer].mtx_ci);

    return 0;
}

/******************************************************************************
 * 函数介绍: FIFO信息DUMP打印
 * 输入参数: fifo_buff_addr: FIFO内存缓冲区地址
 *           consumer: 消费者序号(FIFO_CONSUMER_E)
 * 输出参数: 无
 * 返回值  : 0-成功,<0-错误代码
 *****************************************************************************/
sint32 fifo_dump_info(char *fifo_buff_addr, FIFO_CONSUMER_E consumer)
{
    return _fifo_dump_info((PFIFO_FD_S)fifo_buff_addr, consumer);
}

/******************************************************************************
 * 函数介绍: FIFO数据DUMP打印
 * 输入参数: fifo_buff_addr: FIFO内存缓冲区地址
 *           consumer: 消费者序号(FIFO_CONSUMER_E)
 *           fifo_buff_offset: FIFO内存缓冲区偏移
 * 输出参数: 无
 * 返回值  : 0-成功,<0-错误代码
 *****************************************************************************/
sint32 fifo_dump_data(char *fifo_buff_addr, FIFO_CONSUMER_E consumer, uint32 fifo_buff_offset)
{
    return _fifo_dump_data((PFIFO_FD_S)fifo_buff_addr, consumer, fifo_buff_offset);
}

/******************************************************************************
 * 函数介绍: FIFO配置
 * 输入参数: handle: FIFO句柄
 *           cmd: 命令
 *           channel: 通道号,视命令而定
 *           param: 输入参数
 *           param_len: param长度,特别对于GET命令时,输出参数应先判断缓冲区是否足够
 * 输出参数: param: 输出参数
 * 返回值  : 0-成功,<0-错误代码
 *****************************************************************************/
sint32 fifo_ioctrl(sint32 handle, sint32 cmd, sint32 channel, sint32 *param, sint32 param_len)
{
    PFIFO_FD_S fd = (PFIFO_FD_S)handle;
    sint32 i = 0;

    if (COMMON_CMD_GETVERSION == cmd)
    {
        if (FIFO_PARAM_VERIFY(param, param_len, sizeof(SWVERSION)))
        {
            return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
        }

        *(PSWVERSION)param = swver;
        return 0;
    }

    if ((handle <= 0) || (0 == FLAG_VERIFY(fd)))
    {
        TRACE_ERR("handle(0x%x) is invalid\n", handle);
        return ERRNO(COMMON_ERROR_HANDLEINVAL, COMP_NENT);
    }

    switch (cmd)
    {
    case COMMON_CMD_RESET:
        if (FIFO_CONSUMER_VERIFY(ALL_CHANNEL, channel, fd->maxch))
        {
            return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
        }
    #ifndef _WIN32
        if (0 == access(FIFO_DISABLE_RESET_FILE_NAME, R_OK))
        {
            TRACE_ERR("Flag file disable reset fifo(%p)\n", fd);
            return ERRNO(COMMON_ERROR_NOCMD, COMP_NENT);
        }
    #endif /* _WIN32 */
        if (ALL_CHANNEL == channel)
        {
            for (i = 0; i < fd->maxch; i++)
            {
                if (FIFO_CONSUMER_STAT(fd, i))
                {
                    fd->pcm[i].cci = fd->pcm[i].ci = fd->pi;
                    fd->pcm[i].link_stat = FIFO_LINKSTAT_NO;
                    fd->pcm[i].timecount = fd->pcm[i].vframe_count = 0;
                }
            }
        }
        else
        {
            if (FIFO_CONSUMER_STAT(fd, channel))
            {
                fd->pcm[channel].cci = fd->pcm[channel].ci = fd->pi;
                fd->pcm[channel].link_stat = FIFO_LINKSTAT_NO;
                fd->pcm[channel].timecount = fd->pcm[channel].vframe_count = 0;
            }
            if ((~(BIT64(channel))) & fd->consumer_stat)
            {
                goto not_reset;
            }
        }
        fd->full_keyfrm_posi = fd->part_allfrm_posi = 0;
        fd->part_allfrm_tlsz = fd->part_allfrm_wtsz = 0;
        fd->full_keyfrm_stat = FIFO_KEYFRM_NOT_EXIST;
        fd->part_allfrm_stat = FIFO_FRAME_FULL;
        fd->dbg_err_count = 0;
    not_reset:
        TRACE_ERR("channel(%d) COMMON_CMD_RESET\n", channel);
        break;
    case FIFO_CMD_GETFIFOSIZE:
        if (FIFO_PARAM_VERIFY(param, param_len, sizeof(sint32)))
        {
            return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
        }

        *param = fd->size;
        break;
    case FIFO_CMD_GETDATASIZE:
        if (FIFO_CONSUMER_VERIFY(ALL_CHANNEL, channel, fd->maxch) || 
            FIFO_PARAM_VERIFY(param, param_len, sizeof(sint32)))
        {
            return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
        }

        if (ALL_CHANNEL == channel)
        {
            *param = _fifo_maxdatasize(fd);
        }
        else
        {
            *param = FIFO_DATA_SIZE(fd, channel);
        }
        break;
    case FIFO_CMD_GETFREESIZE:
        if (FIFO_CONSUMER_VERIFY(ALL_CHANNEL, channel, fd->maxch) || 
            FIFO_PARAM_VERIFY(param, param_len, sizeof(sint32)))
        {
            return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
        }

        if (ALL_CHANNEL == channel)
        {
            *param = _fifo_minfreesize(fd);
        }
        else
        {
            *param = FIFO_FREE_SIZE(fd, channel);
        }
        break;
    case FIFO_CMD_GETFREESIZESTAT:
    {
        sint32 free_size = 0, free_stat = 0;
        
        if (FIFO_CONSUMER_VERIFY(ALL_CHANNEL, channel, fd->maxch) || 
            FIFO_PARAM_VERIFY(param, param_len, sizeof(sint32)) || ((free_size = *param) < 0))
        {
            return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
        }

        if (ALL_CHANNEL == channel)
        {
            for (i = 0; i < fd->maxch; i++)
            {
                if (FIFO_CONSUMER_STAT(fd, i) && (FIFO_FREE_SIZE(fd, i) < free_size))
                {
                    free_stat |= BIT32(i);
                }
            }
        }
        else
        {
            if (FIFO_CONSUMER_STAT(fd, channel) && (FIFO_FREE_SIZE(fd, channel) < free_size))
            {
                free_stat = BIT32(channel);
            }
        }
        
        return free_stat;
    }
    case FIFO_CMD_GETCURWATER:
    {
        sint32 data_size = 0;

        if (FIFO_CONSUMER_VERIFY(0, channel, fd->maxch) || FIFO_PARAM_VERIFY(param, param_len, sizeof(sint32)))
        {
            return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
        }
        
        data_size = FIFO_DATA_SIZE(fd, channel);
        *param = MUX(data_size <= fd->pcm[channel].lowater, FIFO_WATER_LOW, MUX(data_size < fd->pcm[channel].hiwater, 
            FIFO_WATER_MIDDLE, MUX(data_size < fd->pcm[channel].htwater, FIFO_WATER_HIGH, FIFO_WATER_HIGHEST)));
        break;
    }
    case FIFO_CMD_GETHTWATER:
        if (FIFO_CONSUMER_VERIFY(0, channel, fd->maxch) || FIFO_PARAM_VERIFY(param, param_len, sizeof(sint32)))
        {
            return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
        }

        *param = fd->pcm[channel].htwater * 100 / fd->size;
        break;
    case FIFO_CMD_SETHTWATER:
    {
        sint32 htwater = (sint32)param * fd->size / 100;

        if (FIFO_CONSUMER_VERIFY(0, channel, fd->maxch))
        {
            return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
        }
        if ((fd->pcm[channel].hiwater > htwater) || (htwater > 100))
        {
            TRACE_ERR("unsupported handle(0x%x) channel(%d) hiwater(%d) > htwater(%d) > 100\n", 
                handle, channel, fd->pcm[channel].hiwater, htwater);
            return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
        }
        fd->pcm[channel].htwater = htwater;
        break;
    }
    case FIFO_CMD_GETHTWATERSTAT:        
        if (FIFO_CONSUMER_VERIFY(ALL_CHANNEL, channel, fd->maxch) || 
            FIFO_PARAM_VERIFY(param, param_len, sizeof(sint32)))
        {
            return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
        }
        
        *param = 0;
        if (ALL_CHANNEL == channel)
        {
            for (i = 0; i < fd->maxch; i++)
            {
                if (FIFO_CONSUMER_STAT(fd, i) && (FIFO_DATA_SIZE(fd, i) > (fd->size * 100 / fd->pcm[i].htwater)))
                {
                    *param |= BIT32(i);
                }
            }
        }
        else
        {
            if (FIFO_CONSUMER_STAT(fd, channel) && 
                (FIFO_DATA_SIZE(fd, channel) > (fd->size * 100 / fd->pcm[channel].htwater)))
            {
                *param |= BIT32(channel);
            }
        }
        break;
    case FIFO_CMD_GETHIWATER:
        if (FIFO_CONSUMER_VERIFY(0, channel, fd->maxch) || FIFO_PARAM_VERIFY(param, param_len, sizeof(sint32)))
        {
            return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
        }

        *param = fd->pcm[channel].hiwater * 100 / fd->size;
        break;
    case FIFO_CMD_SETHIWATER:
    {
        sint32 hiwater = (sint32)param * fd->size / 100;

        if (FIFO_CONSUMER_VERIFY(0, channel, fd->maxch))
        {
            return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
        }
        if ((fd->pcm[channel].lowater > hiwater) || (hiwater > fd->pcm[channel].htwater))
        {
            TRACE_ERR("unsupported handle(0x%x) channel(%d) lowater(%d) > hiwater(%d) > htwater(%d)\n", 
                handle, channel, fd->pcm[channel].lowater, hiwater, fd->pcm[channel].htwater);
            return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
        }
        fd->pcm[channel].hiwater = hiwater;
        break;
    }
    case FIFO_CMD_GETHIWATERSTAT:        
        if (FIFO_CONSUMER_VERIFY(ALL_CHANNEL, channel, fd->maxch) || 
            FIFO_PARAM_VERIFY(param, param_len, sizeof(sint32)))
        {
            return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
        }
        
        *param = 0;
        if (ALL_CHANNEL == channel)
        {
            for (i = 0; i < fd->maxch; i++)
            {
                if (FIFO_CONSUMER_STAT(fd, i) && (FIFO_DATA_SIZE(fd, i) > (fd->size * 100 / fd->pcm[i].hiwater)))
                {
                    *param |= BIT32(i);
                }
            }
        }
        else
        {
            if (FIFO_CONSUMER_STAT(fd, channel) && 
                (FIFO_DATA_SIZE(fd, channel) > (fd->size * 100 / fd->pcm[channel].hiwater)))
            {
                *param |= BIT32(channel);
            }
        }
        break;
    case FIFO_CMD_GETLOWATER:
        if (FIFO_CONSUMER_VERIFY(0, channel, fd->maxch) || FIFO_PARAM_VERIFY(param, param_len, sizeof(sint32)))
        {
            return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
        }

        *param = fd->pcm[channel].lowater * 100 / fd->size;
        break;
    case FIFO_CMD_SETLOWATER:
    {
        sint32 lowater = (sint32)param * fd->size / 100;
        
        if (FIFO_CONSUMER_VERIFY(0, channel, fd->maxch))
        {
            return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
        }
        if ((lowater < 0) || (lowater > fd->pcm[channel].hiwater))
        {
            TRACE_ERR("unsupported handle(0x%x) channel(%d) 0 > lowater(%d) > hiwater(%d)\n", 
                handle, channel, lowater, fd->pcm[channel].hiwater);
            return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
        }
        fd->pcm[channel].lowater = lowater;
        break;
    }
    case FIFO_CMD_GETLOWATERSTAT:        
        if (FIFO_CONSUMER_VERIFY(ALL_CHANNEL, channel, fd->maxch) || 
            FIFO_PARAM_VERIFY(param, param_len, sizeof(sint32)))
        {
            return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
        }
        
        *param = 0;
        if (ALL_CHANNEL == channel)
        {
            for (i = 0; i < fd->maxch; i++)
            {
                if (FIFO_CONSUMER_STAT(fd, i) && (FIFO_DATA_SIZE(fd, i) > (fd->size * 100 / fd->pcm[i].lowater)))
                {
                    *param |= BIT32(i);
                }
            }
        }
        else
        {
            if (FIFO_CONSUMER_STAT(fd, channel) && 
                (FIFO_DATA_SIZE(fd, channel) > (fd->size * 100 / fd->pcm[channel].lowater)))
            {
                *param |= BIT32(channel);
            }
        }
        break;
    case FIFO_CMD_SETKEYFLAG:
        fd->keyflag = (sint32)param;
        break;
    case FIFO_CMD_GETTIMECOUNT:
        if (FIFO_CONSUMER_VERIFY(0, channel, fd->maxch) || FIFO_PARAM_VERIFY(param, param_len, sizeof(sint32)))
        {
            return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
        }

        *param = fd->pcm[channel].timecount;
        TRACE_REL("handle(0x%x)[%d] FIFO_CMD_GETTIMECOUNT(%d-%d)\n", 
            handle, channel, fd->pcm[channel].timecount, *param);
        break;
    case FIFO_CMD_SETTIMECOUNT:
        if (FIFO_CONSUMER_VERIFY(0, channel, fd->maxch))
        {
            return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
        }
        if ((sint32)param < 0)
        {
            DECREMENT_ATOMIC(fd->pcm[channel].timecount, 1);
            TRACE_ERR("handle(0x%x)[%d] FIFO_CMD_SETTIMECOUNT--\n", handle, channel);
        }
        else
        {
            INCREMENT_ATOMIC(fd->pcm[channel].timecount, 1);
            TRACE_ERR("handle(0x%x)[%d] FIFO_CMD_SETTIMECOUNT++\n", handle, channel);
        }
        break;
    case FIFO_CMD_LOCK:
        pthread_mutex_lock(&fd->mutex);
        break;
    case FIFO_CMD_UNLOCK:
        pthread_mutex_unlock(&fd->mutex);
        break;
    case FIFO_CMD_CONSUMERSTAT:
    {
        sint32 consumer_nums = 0;
        
        if (FIFO_PARAM_VERIFY(param, param_len, sizeof(sint32)))
        {
            return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
        }
        
        *param = 0;
        for (i = 0; i < fd->maxch; i++)
        {
            if (FIFO_CONSUMER_STAT(fd, i))
            {
                *param |= BIT32(i);
                consumer_nums++;
            }
        }
        
        return consumer_nums;
    }
    case FIFO_CMD_GETVFRAMECOUNT:
        if (FIFO_CONSUMER_VERIFY(0, channel, fd->maxch) || FIFO_PARAM_VERIFY(param, param_len, sizeof(sint32)))
        {
            return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
        }

        *param = fd->pcm[channel].vframe_count;
        break;
    case FIFO_CMD_GETLINKSTAT:
        if (FIFO_CONSUMER_VERIFY(0, channel, fd->maxch) || FIFO_PARAM_VERIFY(param, param_len, sizeof(sint32)))
        {
            return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
        }

        *param = fd->pcm[channel].link_stat;
        break;
    case FIFO_CMD_SETNEWFULKEYFRM:
        if (FIFO_CONSUMER_VERIFY(ALL_CHANNEL, channel, fd->maxch))
        {
            return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
        }

        TRACE_DBG("maxch(%d) consumer_stat(0x%llx) full_keyfrm_stat(%d) channel(%d)\n", 
            fd->maxch, fd->consumer_stat, fd->full_keyfrm_stat, channel);
        if (fd->full_keyfrm_stat && (0 == (fd->full_keyfrm_stat & FIFO_KEYFRM_OVERLAY)))
        {
            if (ALL_CHANNEL == channel)
            {
                for (i = 0; i < fd->maxch; i++)
                {
                    if (FIFO_CONSUMER_STAT(fd, i))
                    {
                        pthread_mutex_lock(&fd->pcm[i].mtx_ci);
                        fd->pcm[i].timecount = 1;
                        fd->pcm[i].vframe_count = fd->full_vframe_count;
                        fd->pcm[i].ci = fd->full_keyfrm_posi;
                        pthread_mutex_unlock(&fd->pcm[i].mtx_ci);
                        TRACE_DBG("FIFO_CMD_SETCITONEWFULKEYFRM i(%d) OK\n", i);
                    }
                }
            }
            else
            {
                if (FIFO_CONSUMER_STAT(fd, channel))
                {
                    pthread_mutex_lock(&fd->pcm[channel].mtx_ci);
                    fd->pcm[channel].timecount = 1;
                    fd->pcm[channel].vframe_count = fd->full_vframe_count;
                    fd->pcm[channel].ci = fd->full_keyfrm_posi;
                    pthread_mutex_unlock(&fd->pcm[channel].mtx_ci);
                    TRACE_DBG("FIFO_CMD_SETCITONEWFULKEYFRM channel(%d) OK\n", channel);
                }
            }
        }
        break;
    default:
        TRACE_ERR("unsupported handle(0x%x)[%d] cmd, 0x%x\n", handle, channel, cmd);
        return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
    }

    return 0;
}

/******************************************************************************
 * 函数介绍: 关闭FIFO,该函数将清楚fifo所有数据,并释放相关资源
 * 输入参数: handle: FIFO句柄
 * 输出参数: 无
 * 返回值  : 0-成功,<0-错误代码
 *****************************************************************************/
sint32 fifo_close(sint32 handle)
{
    PFIFO_FD_S fd = (PFIFO_FD_S)handle;

    if ((handle <= 0) || (0 == FLAG_VERIFY(fd)))
    {
        TRACE_ERR("handle(0x%x) is invalid\n", handle);
        return ERRNO(COMMON_ERROR_HANDLEINVAL, COMP_NENT);
    }
    
    return _fifo_close(fd);
}

/******************************************************************************
 * 函数介绍: 根据不同的消费者关闭FIFO,该函数不会清楚fifo数据和释放相关资源,
 *           如需要清楚fifo数据和释放相关资源,请调用函数fifo_close
 * 输入参数: handle: FIFO句柄
 *           consumer: 消费者序号(FIFO_CONSUMER_E)
 * 输出参数: 无
 * 返回值  : 0-成功,<0-错误代码
 *****************************************************************************/
sint32 fifo_closemulti(sint32 handle, FIFO_CONSUMER_E consumer)
{
    PFIFO_FD_S fd = (PFIFO_FD_S)handle;

    if ((handle <= 0) || (0 == FLAG_VERIFY(fd)) || (0 == FIFO_CONSUMER_STAT(fd, consumer)))
    {
        TRACE_ERR("handle(0x%x)[%d] invalid\n", handle, consumer);
        return ERRNO(COMMON_ERROR_HANDLEINVAL, COMP_NENT);
    }
    if (FIFO_CONSUMER_VERIFY(0, consumer, fd->maxch))
    {
        return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
    }

    if (FIFO_CONSUMER_STAT(fd, consumer))
    {
        fd->consumer_stat &= ~(BIT64(consumer));
        fd->pcm[consumer].link_stat = FIFO_LINKSTAT_NO;
        fd->pcm[consumer].timecount = fd->pcm[consumer].vframe_count = 0;
        pthread_mutex_destroy(&fd->pcm[consumer].mtx_ci);
    }
    
    return 0;
}
