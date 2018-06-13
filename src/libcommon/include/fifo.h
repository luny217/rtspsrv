#ifndef _FIFO_H_
#define _FIFO_H_

#include "common.h"

#define FIFO_MAX_BUFF_COUNT 2
#define FIFO_MAX_BUFF_SIZE  0xFFFFFFFF

/* 该模块只限制于同一进程中的不同线程调用,不适用于进程间通讯;不支持阻塞模式 */
typedef enum
{
    FIFO_WATER_LOW      = -1,   /* FIFO低水位 */
    FIFO_WATER_MIDDLE   = 0,    /* FIFO中水位 */
    FIFO_WATER_HIGH     = 1,    /* FIFO高水位 */
    FIFO_WATER_HIGHEST  = 2,    /* FIFO最高水位 */
} FIFO_WATER_E;

typedef enum
{
    FIFO_LINKSTAT_NO    = 0,    /* 无需拼接 */
    FIFO_LINKSTAT_YES   = 1,    /* 需要拼接 */
} FIFO_LINKSTAT_E;

typedef enum
{
    FIFO_PEEK_YES   = 0,    /* 数据清空 */
    FIFO_PEEK_NO    = 1,    /* 数据不清空 */
} FIFO_PEEK_E;

/* 消费者序号,现设计最多支持4个 */
typedef enum
{
    FIFO_CONSUMER_NUL = -1,     /* 无效的消费者 */
    FIFO_CONSUMER_FIR = 0,      /* 第一个消费者 */
    FIFO_CONSUMER_SEC,          /* 第二个消费者 */
    FIFO_CONSUMER_THR,          /* 第三个消费者 */
    FIFO_CONSUMER_FOR,          /* 第四个消费者 */
    FIFO_CONSUMER_MAX = 64,     /* 最大支持的消费者个数 */
    FIFO_CONSUMER_ALL = 0xFF,   /* 所有消费者 */
} FIFO_CONSUMER_E;

/* 以下宏定义为出错信息 */
typedef enum
{
    FIFO_ERROR_FULL = COMP_ERROR_START, /* FIFO已满 */
    FIFO_ERROR_EMPTY,                   /* FIFO已空 */
    FIFO_ERROR_NOBUF,                   /* FIFO没有足够缓冲 */
    FIFO_ERROR_NODAT,                   /* FIFO没有足够数据 */
    FIFO_ERROR_MSHEAD,                  /* MSHEAD出错 */
    FIFO_ERROR_FOPEN,                   /* 打开文件出错 */
    FIFO_ERROR_FWRITE,                  /* 写入文件出错 */
    FIFO_ERROR_FSEEK,                   /* 移动文件句柄指针出错 */
    FIFO_ERROR_FTELL,                   /* 移动文件句柄指针到文件尾出错(40) */
    FIFO_ERROR_EXTBUFTOSMALL,           /* 外部BUFTTER太小 */
    FIFO_ERROR_UNMATCHMSHEAD,           /* 匹配MSHEAD错误 */
    FIFO_ERROR_KEYFRMNOTEXIST,          /* 关键帧不存在 */
    FIFO_ERROR_CCICIUNMATCHED,          /* CCI和CI值不匹配,丢弃数据 */
    FIFO_ERROR_REACHMAXCH,              /* 达到最大通道数 */
} FIFO_ERROR_E;

/* 以下宏定义为fifo_ioctrl()的cmd类型 */
typedef enum
{
    FIFO_CMD_GETFIFOSIZE = CMDNO(0, COMP_FIFO), /* 获取FIFO缓冲区大小,channel(此处无效),param参数为(sint32 *) */
    FIFO_CMD_GETDATASIZE,       /* 获取FIFO中数据大小,channel(ALL_CHANNEL-获取消费者中最大数据长度,FIFO_CONSUMER_E-获取相应消费者序号数据长度),param参数为(sint32 *) */
    FIFO_CMD_GETFREESIZE,       /* 获取FIFO中空闲大小,channel(ALL_CHANNEL-获取消费者中最小空闲长度,FIFO_CONSUMER_E-获取相应消费者序号空闲长度),param参数为(sint32 *) */
    FIFO_CMD_GETCURWATER,       /* 获取FIFO中当前WATER(FIFO_WATER_E),channel(FIFO_CONSUMER_E-消费者序号),param参数为(sint32 *) */
    FIFO_CMD_GETHIWATER,        /* 获取FIFO中HIWATER大小(单位为%,LOWATER~HTWATER),channel(FIFO_CONSUMER_E-消费者序号),param参数为(sint32 *) */
    FIFO_CMD_SETHIWATER,        /* 设置FIFO中HIWATER大小(单位为%,LOWATER~HTWATER),channel(FIFO_CONSUMER_E-消费者序号),param参数为(sint32) */
    FIFO_CMD_GETLOWATER,        /* 获取FIFO中LOWATER大小(单位为%,0~LOWATER),channel(FIFO_CONSUMER_E-消费者序号),param参数为(sint32 *) */
    FIFO_CMD_SETLOWATER,        /* 设置FIFO中LOWATER大小(单位为%,0~LOWATER),channel(FIFO_CONSUMER_E-消费者序号),param参数为(sint32) */
    FIFO_CMD_SETKEYFLAG,        /* 设置关键帧标志,param参数为(sint32) */
    FIFO_CMD_GETTIMECOUNT,      /* 获取timecount,channel(FIFO_CONSUMER_E-消费者序号),param参数为(sint32 *) */
    FIFO_CMD_SETTIMECOUNT,      /* 设置timecount,channel(FIFO_CONSUMER_E-消费者序号),param参数为(sint32,<0-timecount-1,>0-timecount+1) */
    FIFO_CMD_LOCK,              /* 锁定FIFO句柄 */
    FIFO_CMD_UNLOCK,            /* 解锁FIFO句柄 */
    FIFO_CMD_GETVFRAMECOUNT,    /* 获取FIFO中视频帧数,channel(FIFO_CONSUMER_E-消费者序号),param参数为(sint32 *) */
    FIFO_CMD_GETLINKSTAT,       /* 获取FIFO中音视频帧是否出现拼接,channel(FIFO_CONSUMER_E-消费者序号),param参数为(sint32 *(FIFO_LINKSTAT_E)) */
    FIFO_CMD_GETFREESIZESTAT,   /* 根据实际需要的空闲大小,获取FIFO中空闲大小是否实际需要的空闲大小状态,返回值<0-错误号,>=0表示消费者空闲大小状态(按位表示,0-满足,1-不满足),
                                 * channel(ALL_CHANNEL-所有消费者,FIFO_CONSUMER_E-消费者序号),param参数为(sint32 *,表示实际需要的空闲大小) */
    FIFO_CMD_GETHTWATER,        /* 获取FIFO中HTWATER大小(单位为%,HIWATER~100),channel(FIFO_CONSUMER_E-消费者序号),param参数为(sint32 *) */
    FIFO_CMD_SETHTWATER,        /* 设置FIFO中HTWATER大小(单位为%,HIWATER~100),channel(FIFO_CONSUMER_E-消费者序号),param参数为(sint32) */
    FIFO_CMD_GETHTWATERSTAT,    /* 获取FIFO中HTWATER状态,channel(ALL_CHANNEL-所有消费者,FIFO_CONSUMER_E-消费者序号),
                                 * param参数为(表示水位状态(按位表示相应消费者水位状态,0-未到达,1-已到达)) */
    FIFO_CMD_GETHIWATERSTAT,    /* 获取FIFO中HIWATER状态,channel(ALL_CHANNEL-所有消费者,FIFO_CONSUMER_E-消费者序号),
                                 * param参数为(表示水位状态(按位表示相应消费者水位状态,0-未到达,1-已到达)) */
    FIFO_CMD_GETLOWATERSTAT,    /* 获取FIFO中LOWATER状态,channel(ALL_CHANNEL-所有消费者,FIFO_CONSUMER_E-消费者序号),
                                 * param参数为(表示水位状态(按位表示相应消费者水位状态,0-未到达,1-已到达)) */
    FIFO_CMD_CONSUMERSTAT,      /* 获取FIFO消费者状态,返回值<0-错误号,>=0表示消费者个数,channel(此处无效),param参数为(sint32 *,消费者状态,按位表示,0-不存在,1-存在) */
    FIFO_CMD_SETNEWFULKEYFRM,   /* 设置到最新完整关键帧位置,channel(ALL_CHANNEL-所有消费者,FIFO_CONSUMER_E-消费者序号) */
} FIFO_CMD_E;

typedef struct
{
    uint32 size;    /* 内存缓冲区大小 */
    void *pbuff;    /* 内存缓冲区指针 */
} FIFO_BUFF_S, *PFIFO_BUFF_S;
#define FIFO_BUFF_S_LEN sizeof(FIFO_BUFF_S)

typedef enum
{
    FIFO_BUFF_HEAD_DATA,    /* 缓冲区首地址是数据 */
    FIFO_BUFF_HEAD_MSHEAD,  /* 缓冲区首地址是MSHEAD头信息 */
} FIFO_BUFF_HEAD_TYPE_E;

typedef struct
{
    uint8 buff_count;   /* 内存缓冲区个数 */
    uint8 buff_head;    /* 内存缓冲区缓冲区首地址数据类型(FIFO_BUFF_HEAD_TYPE_E) */
    uint8 reserved[3];  /* 保留位 */
    FIFO_BUFF_S buff[FIFO_MAX_BUFF_COUNT];  /* 内存缓冲区信息结构体 */
} FIFO_BUFF_INFO_S, *PFIFO_BUFF_INFO_S;
#define FIFO_BUFF_INFO_S_LEN    sizeof(FIFO_BUFF_INFO_S)

/******************************************************************************
 * 函数介绍: 创建FIFO(当FIFO大小大于等于128KB时,FIFO将增加拼接缓冲区128KB)
 * 输入参数: size: FIFO大小,单位bytes,为优化运算,size必须等于1<<N
 *           hiwater: 高警戒位大小,单位%,0~100
 *           lowater: 低警戒位大小,单位%,0~100
 * 输出参数: 无
 * 返回值  : <0-失败,>0-FIFO句柄
 *****************************************************************************/
sint32 fifo_open(uint32 size, uint32 hiwater, uint32 lowater);

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
sint32 fifo_openext(uint32 size, uint32 hiwater, uint32 lowater, uint32 resv_size);

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
    uint32 htwater, uint32 hiwater, uint32 lowater, uint32 resv_size);

/******************************************************************************
 * 函数介绍: 写入数据到FIFO,该函数一次写入长度size数据,当FIFO中缓冲不足时将返回错误信息
 * 输入参数: handle: FIFO句柄
 *           data: 缓冲区指针
 *           size: 待写数据长度
 * 输出参数: 无
 * 返回值  : >0-写入数据大小,<0-错误代码
 *****************************************************************************/
sint32 fifo_write(sint32 handle, char *data, uint32 size);

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
sint32 fifo_writeext(sint32 handle, char *data, uint32 size);

/******************************************************************************
 * 函数介绍: 获取FIFO中可写的缓冲区信息
 * 输入参数: handle: FIFO句柄
 *           pbuff_info: 缓冲区信息结构体指针
 *           need_size: 需要的缓冲区大小
 * 输出参数: 无
 * 返回值  : 0-成功,<0-错误代码
 *****************************************************************************/
sint32 fifo_write_buffer_get(sint32 handle, PFIFO_BUFF_INFO_S pbuff_info, uint32 need_size);

/******************************************************************************
 * 函数介绍: 更新FIFO中可写的缓冲区信息
 * 输入参数: handle: FIFO句柄
 *           pbuff_info: 缓冲区信息结构体指针
 *           update_size: 更新的缓冲区大小
 * 输出参数: 无
 * 返回值  : 0-成功,<0-错误代码
 *****************************************************************************/
sint32 fifo_write_buffer_update(sint32 handle, PFIFO_BUFF_INFO_S pbuff_info, uint32 update_size);

/******************************************************************************
 * 函数介绍: 从FIFO中读取数据,该函数一次读取长度size数据,当FIFO中数据不足时将返回错误信息
 * 输入参数: handle: FIFO句柄
 *           data: 缓冲区指针(此值可以为空(NULL),为空时,数据将不会被拷贝)
 *           size: 读取大小
 *           peek: 是否清除FIFO中已读数据(FIFO_PEEK_E)
 * 输出参数: 无
 * 返回值  : >0-读出数据大小,<0-错误代码
 *****************************************************************************/
sint32 fifo_read(sint32 handle, char *data, uint32 size, FIFO_PEEK_E peek);

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
sint32 fifo_readmulti(sint32 handle, FIFO_CONSUMER_E consumer, char *data, uint32 size, FIFO_PEEK_E peek);

/******************************************************************************
 * 函数介绍: 从FIFO中读取数据,该函数一次读取1帧数据,当FIFO中数据不足时将返回错误信息
 * 输入参数: handle: FIFO句柄
 *           data: 缓冲区指针(此值可以为空(NULL),为空时,数据将不会被拷贝)
 *           limit: 缓冲区大小(data为空(NULL)时,此值可填无限大)
 *           peek: 是否清除FIFO中已读数据(FIFO_PEEK_E)
 * 输出参数: 无
 * 返回值  : >0-读出数据大小,0-缓冲区不足,<0-错误代码
 *****************************************************************************/
sint32 fifo_readframe(sint32 handle, char *data, uint32 limit, FIFO_PEEK_E peek);

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
sint32 fifo_readframemulti(sint32 handle, FIFO_CONSUMER_E consumer, char *data, uint32 limit, FIFO_PEEK_E peek);

/******************************************************************************
 * 函数介绍: 从FIFO中读取数据,该函数一次读取多帧音视频数据,直至遇到下一个关键帧
 * 输入参数: handle: FIFO句柄
 *           data: 缓冲区指针(此值可以为空(NULL),为空时,数据将不会被拷贝)
 *           limit: 缓冲区大小(data为空(NULL)时,此值可填无限大)
 *           nexttype: 下一帧数据类型
 * 输出参数: 无
 * 返回值  : >0-读出数据大小,<0-错误代码
 *****************************************************************************/
sint32 fifo_readext(sint32 handle, char *data, uint32 limit, sint32 *nexttype);

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
sint32 fifo_readextmulti(sint32 handle, FIFO_CONSUMER_E consumer, char *data, uint32 limit, sint32 *nexttype);

/******************************************************************************
 * 函数介绍: 从FIFO中读取数据,该函数一次读取1帧最新的完整的I帧数据
 * 输入参数: handle: FIFO句柄
 *           data: 缓冲区指针
 *           limit: 缓冲区大小
 *           have_mshead: 是否带有MSHEAD(HB_TRUE-有,HB_FALSE-无)
 * 输出参数: 无
 * 返回值  : >0-读出数据大小,<0-错误代码
 *****************************************************************************/
sint32 fifo_read_newfull_keyframe(sint32 handle, char *data, uint32 limit, HB_BOOL have_mshead);

/******************************************************************************
 * 函数介绍: 获取FIFO中可读的缓冲区信息
 * 输入参数: handle: FIFO句柄
 *           pbuff_info: 缓冲区信息结构体指针
 *           need_size: 需要的缓冲区大小
 * 输出参数: 无
 * 返回值  : >=0-成功,缓冲区总长度,<0-错误代码
 *****************************************************************************/
sint32 fifo_read_buffer_get(sint32 handle, PFIFO_BUFF_INFO_S pbuff_info);

/******************************************************************************
 * 函数介绍: 根据不同的消费者获取FIFO中可读的缓冲区信息
 * 输入参数: handle: FIFO句柄
 *           consumer: 消费者序号(FIFO_CONSUMER_E)
 *           pbuff_info: 缓冲区信息结构体指针
 *           need_size: 需要的缓冲区大小
 * 输出参数: 无
 * 返回值  : >=0-成功,缓冲区总长度,<0-错误代码
 *****************************************************************************/
sint32 fifo_read_buffer_getmulti(sint32 handle, FIFO_CONSUMER_E consumer, PFIFO_BUFF_INFO_S pbuff_info);

/******************************************************************************
 * 函数介绍: 更新FIFO中可读的缓冲区信息
 * 输入参数: handle: FIFO句柄
 *           pbuff_info: 缓冲区信息结构体指针
 *           update_size: 更新的缓冲区大小
 * 输出参数: 无
 * 返回值  : 0-成功,<0-错误代码
 *****************************************************************************/
sint32 fifo_read_buffer_update(sint32 handle, PFIFO_BUFF_INFO_S pbuff_info, uint32 update_size);

/******************************************************************************
 * 函数介绍: 根据不同的消费者更新FIFO中可读的缓冲区信息
 * 输入参数: handle: FIFO句柄
 *           consumer: 消费者序号(FIFO_CONSUMER_E)
 *           pbuff_info: 缓冲区信息结构体指针
 *           update_size: 更新的缓冲区大小
 * 输出参数: 无
 * 返回值  : 0-成功,<0-错误代码
 *****************************************************************************/
sint32 fifo_read_buffer_updatemulti(sint32 handle, FIFO_CONSUMER_E consumer, PFIFO_BUFF_INFO_S pbuff_info, uint32 update_size);

/******************************************************************************
 * 函数介绍: 从FIFO中读取一帧视频数据并处理,该函数一次读取一帧音视频数据和N个音频帧
 * 输入参数: handle: FIFO句柄
 *           func: 数据处理回调函数
 *           func_idx: datacallback()函数中的idx
 *           func_handle: datacallback()函数中的fifo,此处用于回调函数的句柄,例如MFS句柄等等
 * 输出参数: 无
 * 返回值  : >0-已处理数据大小,0-数据不足一帧,或FIFO的resv_size为0时,FIFO帧处理要做掉头处理,<0-错误代码
 *****************************************************************************/
sint32 fifo_frameprocess(sint32 handle, datacallback func, sint32 func_idx, sint32 func_handle);

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
    datacallback func, sint32 func_idx, sint32 func_handle);

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
sint32 fifo_frameprocessext(sint32 handle, datacallback func, sint32 func_idx, sint32 func_handle, sint32 resvered);

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
    datacallback func, sint32 func_idx, sint32 func_handle, sint32 resvered);

/******************************************************************************
 * 函数介绍: 从FIFO中读取数据并处理,该函数一次读取多帧音视频数据,直至遇到下一个视频关键帧
 * 输入参数: handle: FIFO句柄
 *           func: 数据处理回调函数
 *           func_idx: datacallback()函数中的idx
 *           func_handle: datacallback()函数中的fifo,此处用于回调函数的句柄,例如MFS句柄等等
 * 输出参数: 无
 * 返回值  : >0-已处理数据大小,0-数据不足一帧,或FIFO的resv_size为0时,FIFO帧处理要做掉头处理,<0-错误代码
 *****************************************************************************/
sint32 fifo_datprocess(sint32 handle, datacallback func, sint32 func_idx, sint32 func_handle);

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
    datacallback func, sint32 func_idx, sint32 func_handle);

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
    sint32 func_idx, sint32 func_handle, sint32 deal_ifrm_nums);

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
    datacallback func, sint32 func_idx, sint32 func_handle, sint32 deal_ifrm_nums);

/******************************************************************************
 * 函数介绍: FIFO重定位MSHEAH
 * 输入参数: handle: FIFO句柄
 *           consumer: 消费者序号(FIFO_CONSUMER_E)
 * 输出参数: 无
 * 返回值  : 0-成功,<0-错误代码
 *****************************************************************************/
sint32 fifo_relocate_mshead(sint32 handle, FIFO_CONSUMER_E consumer);

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
sint32 fifo_ioctrl(sint32 handle, sint32 cmd, sint32 channel, sint32 *param, sint32 param_len);

/******************************************************************************
 * 函数介绍: FIFO信息DUMP打印
 * 输入参数: fifo_buff_addr: FIFO内存缓冲区地址
 *           consumer: 消费者序号(FIFO_CONSUMER_E)
 * 输出参数: 无
 * 返回值  : 0-成功,<0-错误代码
 *****************************************************************************/
sint32 fifo_dump_info(char *fifo_buff_addr, FIFO_CONSUMER_E consumer);

/******************************************************************************
 * 函数介绍: FIFO数据DUMP打印
 * 输入参数: fifo_buff_addr: FIFO内存缓冲区地址
 *           consumer: 消费者序号(FIFO_CONSUMER_E)
 *           fifo_buff_offset: FIFO内存缓冲区偏移
 * 输出参数: 无
 * 返回值  : 0-成功,<0-错误代码
 *****************************************************************************/
sint32 fifo_dump_data(char *fifo_buff_addr, FIFO_CONSUMER_E consumer, uint32 fifo_buff_offset);

/******************************************************************************
 * 函数介绍: 关闭FIFO,该函数将清楚fifo所有数据,并释放相关资源
 * 输入参数: handle: FIFO句柄
 * 输出参数: 无
 * 返回值  : 0-成功,<0-错误代码
 *****************************************************************************/
sint32 fifo_close(sint32 handle);

/******************************************************************************
 * 函数介绍: 根据不同的消费者关闭FIFO,该函数不会清楚fifo数据和释放相关资源,
 *           如需要清楚fifo数据和释放相关资源,请调用函数fifo_close
 * 输入参数: handle: FIFO句柄
 *           consumer: 消费者序号(FIFO_CONSUMER_E)
 * 输出参数: 无
 * 返回值  : 0-成功,<0-错误代码
 *****************************************************************************/
sint32 fifo_closemulti(sint32 handle, FIFO_CONSUMER_E consumer);

#endif /* _FIFO_H_ */
