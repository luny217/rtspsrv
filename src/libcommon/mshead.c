/******************************************************************************
 * 模块名称: MSHEAD-流媒体头信息规范
 * 修改记录: 
 *  1. 2016-08-25 V3.1.0
 *  1). 修改段信息计数未累加的问题
 *****************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
//#include <windows.h>
#else
#include <sys/time.h>
#include <pthread.h>
//#include "mem_manage.h"
#endif /* _WIN32 */
#include "mshead.h"

#define COMP_NENT       COMP_MSHEAD
#define MIN_USEC        1000
#define FLAG_VERIFY(fd) ISMSHEAD(fd)

#ifdef _RTSPSRV_TEST
#define MSHEAD_TRACE_ERR(str, ...)  //TRACE_ERR(COMP_MSHEAD, str, ##args)
#else
#define MSHEAD_TRACE_ERR(str, args...)  TRACE_ERR(COMP_MSHEAD, str, ##args)
#endif

#define MSHEAD_ISVIDEOFRAME(frame_type) ((MSHEAD_STREAM_FRAME_VIDEO_I == (frame_type)) || \
    (MSHEAD_STREAM_FRAME_VIDEO_P == (frame_type)) || (MSHEAD_STREAM_FRAME_VIDEO_B == (frame_type)))

static SWVERSION swver = {0, 1, 3, 25, 8, 16}; /* 模块版本定义 */

/******************************************************************************
 * 函数介绍: 生成一帧数据的校验和
 * 输入参数: handle: MSHEAD句柄
 * 输出参数: data: 音视频数据缓冲区地址指针
 *           size: 音视频数据长度指针
 * 返回值: <=0-失败,>0-数据的校验和
 *****************************************************************************/
static sint32 _mshead_generate_checksum(PMSHEAD fd, sint8 *data, uint32 size)
{
    uint32 checksum = 0; /* 从0开始累加校验和,该校验和的算法复杂性很低,只能较低强度验证数据的正确性 */
    const uint8 *ptmp = NULL;
    uint32 mshsize = MSHEAD_GETMSHSIZE(fd), checked_size = 0, i = 0;
    
    if (mshsize > MSHEAD_LEN) /* 计算MSHEAD_SEG_VIDEO_S和MSHEAD_SEGMENT[0 : n-1]的校验和 */
    {
        ptmp = (const uint8 *)MSHEAD_SEGP_FIR(fd);
        checked_size = mshsize - MSHEAD_LEN;
        for (i = 0; i < checked_size; i++)
        {
            checksum += ptmp[i];
        }
        TRACE_DBG("mshsize(%d) checksum(0x%x)\n", mshsize, checksum);
    }
    checked_size = fd->checked_encode_data_size;
    if ((checked_size > 0) && ((checked_size << 1) <= size)) /* 计算编码数据的校验和 */
    {
        if (NULL == data)
        {
            MSHEAD_TRACE_ERR("unsupported data is NULL\n");
            return 0;
        }
        ptmp = (const uint8 *)data; /* 前一部分 */
        for (i = 0; i < checked_size; i++)
        {
            checksum += ptmp[i];
        }
        ptmp = (const uint8 *)(data) + (size - checked_size); /* 后一部分 */
        for (i = 0; i < checked_size; i++)
        {
            checksum += ptmp[i];
        }
        TRACE_DBG("checked_size(%d) checksum(0x%x)\n", checked_size, checksum);
    }
    checksum &= 0x00FFFFFF; /* 校验和低24位有效 */
    checksum |= 0x00800000; /* 保证校验和不为0 */
 	TRACE_DBG("size(%d) checksum(0x%x)\n", size, checksum);
	
	return checksum;
}

/******************************************************************************
 * 函数介绍: 写入帧信息数据,编码数据不进行拷贝
 * 输入参数: handle: MSHEAD句柄
 *           frame_type: 帧类型(MSHEAD_STREAM_FRAME_TYPE_E)
 *           en_checksum: 是否需要生成校验和
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
 *    (注意: 长度不够MSHEAD_HEAD_ALIGN_SIZE字节对齐将自动加长后对齐,H265不按照4字节对其)
 *****************************************************************************/
static sint32 _mshead_writeext(PMSHEAD fd, MSHEAD_STREAM_FRAME_TYPE_E frame_type, HB_BOOL en_checksum, 
    sint8 *data, uint32 size, uint32 sec, uint32 msec, uint32 width, uint32 height, uint32 frame_rate)
{    
    MSHEAD_SETMSDSIZE_MAX(fd, size);
    if (MSHEAD_ISVIDEOFRAME(frame_type))
    {
        PMSHEAD_SEG_VIDEO_S pvhead = (PMSHEAD_SEG_VIDEO_S)MSHEAD_SEGP_VIDEO(fd);
        
        if (0 == sec)
        {
            struct timeval tv;

            gettimeofday(&tv, NULL);
            sec = tv.tv_sec;
            msec = tv.tv_usec / MIN_USEC;
        }
        pvhead->fcount++;
        pvhead->width = width >> 2;
        pvhead->height = height >> 2;
        pvhead->frate_rate = MUX(frame_rate > MSHEAD_MAX_VIDEO_FRAME_RATE, MSHEAD_MAX_VIDEO_FRAME_RATE, frame_rate);
        pvhead->video_header_size = MSHEAD_SEG_VIDEO_S_LEN;
        pvhead->tick_count = (sec * ONE_SEC_PER_MSEC) + msec;
        pvhead->timestamp_low = sec;
        pvhead->timestamp_millisecond = msec;
        fd->type = frame_type;
        fd->segment = 0;
        fd->mshsize = MSHEAD_LEN + MSHEAD_SEG_VIDEO_S_LEN;
        TRACE_DBG("fcount(%d) frame_type(%d) width(%d) height(%d) tick_count(%d) csec(%d) msec(%d)\n", 
            pvhead->fcount, frame_type, width, height, pvhead->tick_count, sec, msec);
    }
    if (en_checksum)
    {
        if (0 == (fd->checksum = _mshead_generate_checksum(fd, data, MSHEAD_GETMSDSIZE(fd))))
        {
            MSHEAD_TRACE_ERR("unsupported checksum is zero\n");
            return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
        }
    }
#ifdef USER_DEBUG
    data_hex_dump((char *)fd, MSHEAD_GET_MAX_MSHSIZE(fd));
#endif /* USER_DEBUG */

    return MSHEAD_GETMSDSIZE(fd);
}

/******************************************************************************
 * 函数介绍: 创建包头,此处将为句柄分配max_mshsize字节内存,
 *           其中包含MSHEAD结构体长度,长度函数内部会自动MSHEAD_HEAD_ALIGN_SIZE字节对齐
 * 输入参数: fd: MSHEAD句柄
 *           algorithm: 编码标准
 *           max_mshsize: 媒体头信息最大长度,大小最大不能超过MSHEAD_MAXHEADSIZE
 *           checked_encode_data_size: 需要校验的编码数据的大小,取值范围[0, 255];一般只校验编码数据的前,后部分字节;
 *                                     当checked_encode_data_size==0时,不校验编码数据;建议:至少校验前后各16个字节的编码数据
 * 输出参数: 无
 * 返回值  : <0-错误,>0-成功,表示创建的句柄
 *****************************************************************************/
static sint32 _mshead_open(PMSHEAD fd, sint32 algorithm, sint32 max_mshsize, uint8 checked_encode_data_size)
{
    fd->mshsize = MSHEAD_LEN;
    fd->algorithm = algorithm;
    MSHEAD_SET_MAX_MSHSIZE(fd, max_mshsize);
    fd->checked_encode_data_size = checked_encode_data_size;
    fd->flag = MSHEAD_FLAG;
    TRACE_DBG("fd(0x%x) flag(0x%x) max_mshsize(%d)\n", (sint32)fd, fd->flag, MSHEAD_GET_MAX_MSHSIZE(fd));
#ifdef USER_DEBUG
    data_hex_dump((char *)fd, max_mshsize);
#endif /* USER_DEBUG */

    return (sint32)fd;
}

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
sint32 mshead_open(sint32 algorithm, sint32 max_mshsize, uint8 checked_encode_data_size)
{
    PMSHEAD fd = NULL;

    if (max_mshsize < MSHEAD_MAX_MSHSIZE_ALIGN_SIZE)
    {
        TRACE_DBG("warning max_mshsize(%d) < MSHEAD_MAX_MSHSIZE_ALIGN_SIZE(%d), default MSHEAD_MAX_MSHSIZE_ALIGN_SIZE\n", 
            max_mshsize, MSHEAD_MAX_MSHSIZE_ALIGN_SIZE);
        max_mshsize = MSHEAD_MAX_MSHSIZE_ALIGN_SIZE;
    }
    if (max_mshsize > MSHEAD_MAX_MSHSIZE)
    {
        TRACE_DBG("warning max_mshsize(%d) > MSHEAD_MAX_MSHSIZE(%d), default MSHEAD_MAX_MSHSIZE\n", 
            max_mshsize, MSHEAD_MAX_MSHSIZE);
        max_mshsize = MSHEAD_MAX_MSHSIZE;
    }
    max_mshsize = ALIGN_N(max_mshsize, MSHEAD_MAX_MSHSIZE_ALIGN_SIZE);
#ifdef _WIN32
	if (NULL == (fd = (PMSHEAD)calloc(1, max_mshsize)))
#else
	if (NULL == (fd = (PMSHEAD)calloc(1, max_mshsize)))
#endif /* _WIN32 */   
    {
        MSHEAD_TRACE_ERR("NO MEMERY(%d)\n", max_mshsize);
        return ERRNO(COMMON_ERROR_NOMEM, COMP_NENT);
    }

    return _mshead_open(fd, algorithm, max_mshsize, checked_encode_data_size);
}

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
sint32 mshead_openext(sint32 algorithm, PMSHEAD pmshead, sint32 max_mshsize, uint8 checked_encode_data_size)
{
    if ((max_mshsize < (sint32)MSHEAD_LEN) || (max_mshsize != ALIGN_N(max_mshsize, MSHEAD_HEAD_ALIGN_SIZE)))
    {
        MSHEAD_TRACE_ERR("max_mshsize(%d) < MSHEAD_LEN(%d) or Not Align MSHEAD_HEAD_ALIGN_SIZE(%d)\n", 
            max_mshsize, MSHEAD_LEN, MSHEAD_HEAD_ALIGN_SIZE);
        return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
    }
    if (max_mshsize > MSHEAD_MAX_MSHSIZE)
    {
        TRACE_DBG("warning max_mshsize(%d) > MSHEAD_MAX_MSHSIZE(%d), default MSHEAD_MAX_MSHSIZE\n", 
            max_mshsize, MSHEAD_MAX_MSHSIZE);
        max_mshsize = MSHEAD_MAX_MSHSIZE;
    }

    return _mshead_open(pmshead, algorithm, max_mshsize, checked_encode_data_size);
}

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
    sint8 *data, uint32 size, uint32 sec, uint32 msec, uint32 width, uint32 height, uint32 frame_rate)
{
    PMSHEAD fd = (PMSHEAD)handle;

    if ((handle <= 0) || (0 == FLAG_VERIFY(fd)))
    {
        MSHEAD_TRACE_ERR("handle(0x%x) is invalid\n", handle);
        return ERRNO(COMMON_ERROR_HANDLEINVAL, COMP_NENT);
    }
    
    return _mshead_writeext(fd, frame_type, en_checksum, data, size, sec, msec, width, height, frame_rate);
}

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
    sint8 *data, uint32 size, uint32 width, uint32 height, uint32 frame_rate)
{
    PMSHEAD fd = (PMSHEAD)handle;

    if ((handle <= 0) || (0 == FLAG_VERIFY(fd)))
    {
        MSHEAD_TRACE_ERR("handle(0x%x) is invalid\n", handle);
        return ERRNO(COMMON_ERROR_HANDLEINVAL, COMP_NENT);
    }
    
    return _mshead_writeext(fd, frame_type, en_checksum, data, size, 0, 0, width, height, frame_rate);
}

/******************************************************************************
 * 函数介绍: 写入短信息数据MSHEAD_SEG
 * 输入参数: handle: MSHEAD句柄
 *           seg: 段头信息,除size,data其余数据由用户输入
 *           data: 用户数据
 *           size: 用户数据长度,如为字符串应包含尾部'\0'的长度
 * 输出参数: 无
 * 返回值  : <0-错误,>0-成功,表示当前包头长度(注意: 长度不够MSHEAD_HEAD_ALIGN_SIZE字节对齐将自动加长后对齐)
 *****************************************************************************/
sint32 mshead_write_seg(sint32 handle, MSHEAD_SEG seg, sint8 *data, uint32 size)
{
    PMSHEAD fd = (PMSHEAD)handle;
    PMSHEAD_SEG pseg = NULL;

    if ((handle <= 0) || (0 == FLAG_VERIFY(fd)))
    {
        MSHEAD_TRACE_ERR("handle(0x%x) is invalid\n", handle);
        return ERRNO(COMMON_ERROR_HANDLEINVAL, COMP_NENT);
    }

    seg.size = ALIGN_N(size + MSHEAD_SEG_LEN, MSHEAD_HEAD_ALIGN_SIZE);
    if ((fd->mshsize + seg.size) > MSHEAD_GET_MAX_MSHSIZE(fd))
    {
        MSHEAD_TRACE_ERR("mshsize(%d) + size(%d) > max_mshsize(%d), align size(%d)\n", 
            fd->mshsize, size, MSHEAD_GET_MAX_MSHSIZE(fd), seg.size);
        return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
    }
    pseg = (PMSHEAD_SEG)MSHEAD_DATAP(fd);
    *pseg = seg;
    if (size)
    {
        sint32 wsize = seg.size - MSHEAD_SEG_LEN;

        memcpy(pseg->data, data, size);
        if ((wsize > 0) && (wsize > (sint32)size))
        {
            memset(pseg->data + size, 0x00, wsize - size);
        }
    }
    fd->segment++;
    fd->mshsize += seg.size;
    TRACE_REL("segment(%d) mshsize(%d) algorithm(%d) frame_type(%d) type(%d-%d) size(%d)\n", 
        fd->segment, fd->mshsize, fd->algorithm, fd->type, pseg->type, seg.type, seg.size);

    return MSHEAD_GETMSHSIZE(fd);
}

/******************************************************************************
 * 函数介绍: 更新帧信息及数据校验值(务必在所有的SEG段信息写入后才能更新)
 * 输入参数: handle: MSHEAD句柄
 *           data: 音视频数据缓冲区(实际未使用)
 *           size: 音视频数据长度
 * 输出参数: 无
 * 返回值  : 0-成功,<0-错误代码
 *****************************************************************************/
sint32 mshead_update_checksum(sint32 handle, sint8 *data, uint32 size)
{
    PMSHEAD fd = (PMSHEAD)handle;

    if ((handle <= 0) || (0 == FLAG_VERIFY(fd)))
    {
        MSHEAD_TRACE_ERR("handle(0x%x) is invalid\n", handle);
        return ERRNO(COMMON_ERROR_HANDLEINVAL, COMP_NENT);
    }

    if (0 == (fd->checksum = _mshead_generate_checksum(fd, data, MSHEAD_GETMSDSIZE(fd))))
    {
        MSHEAD_TRACE_ERR("unsupported checksum is zero\n");
        return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);    
    }

    return 0;
}

/******************************************************************************
 * 函数介绍: 读取音视频数据,获取数据指针,以及长度
 * 输入参数: handle: MSHEAD句柄
 * 输出参数: data: 音视频数据缓冲区地址指针
 *           size: 音视频数据长度指针
 * 返回值  : <0-错误,>0-成功,返回音视频数据缓冲区指针
 *****************************************************************************/
sint32 mshead_read(sint32 handle, sint8 **data, uint32 *size)
{
    PMSHEAD fd = (PMSHEAD)handle;

    if ((handle <= 0) || (0 == FLAG_VERIFY(fd)))
    {
        MSHEAD_TRACE_ERR("handle(0x%x) is invalid\n", handle);
        return ERRNO(COMMON_ERROR_HANDLEINVAL, COMP_NENT);
    }

    *data = (sint8 *)MSHEAD_DATAP(fd);
    *size = MSHEAD_GETMSDSIZE(fd);

    return (sint32)(*data);
}

/******************************************************************************
 * 函数介绍: 获取包头信息中的一个段信息的地址
 * 输入参数: handle: MSHEAD句柄
 *           index: MSHEAD_SEG段索引序号
 * 返回值: <0-失败,0-句柄中无可用信息,>0-则指向获取的段信息指针PMSHEAD_SEG
 *****************************************************************************/
PMSHEAD_SEG mshead_read_seg(sint32 handle, uint32 index)
{
    PMSHEAD fd = (PMSHEAD)handle;
    PMSHEAD_SEG seg = (PMSHEAD_SEG)MSHEAD_SEGP(fd);
    sint32 limit = MSHEAD_DATAP(fd);

    if ((handle <= 0) || (0 == FLAG_VERIFY(fd)))
    {
        MSHEAD_TRACE_ERR("handle(0x%x) is invalid\n", handle);
        return (PMSHEAD_SEG)ERRNO(COMMON_ERROR_HANDLEINVAL, COMP_NENT);
    }
    
    TRACE_REL("index(%d) size(%d) algorithm(%d) type(%d)\n", index, limit - handle, fd->algorithm, fd->type);
    while ((sint32)seg < limit)
    {
        TRACE_REL("index(%d) type(%d) size(%d)\n", index, seg->type, seg->size);
        if (0 == index--)
        {
            TRACE_DBG("index(%d) type(%d) size(%d)\n", index + 1, seg->type, seg->size);
            return seg;
        }
        seg = (PMSHEAD_SEG)((sint32)seg + seg->size);
    }
    MSHEAD_TRACE_ERR("NULL\n");

    return NULL;
}

/******************************************************************************
 * 函数介绍: 生成一帧数据的校验和
 * 输入参数: handle: MSHEAD句柄
 * 输出参数: data: 音视频数据缓冲区地址指针
 *           size: 音视频数据长度指针
 * 返回值: <=0-失败,>0-数据的校验和
 *****************************************************************************/
sint32 mshead_generate_checksum(sint32 handle, sint8 *data, uint32 size)
{
    PMSHEAD fd = (PMSHEAD)(handle);
    
    if ((handle <= 0) || (0 == FLAG_VERIFY(fd)))
    {
        MSHEAD_TRACE_ERR("handle(0x%x) is invalid\n", handle);
        return ERRNO(COMMON_ERROR_HANDLEINVAL, COMP_NENT);
    }
    
	return _mshead_generate_checksum(fd, data, size);
}

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
sint32 mshead_ioctrl(sint32 handle, sint32 cmd, sint32 channel, sint32 *param, sint32 param_len)
{
    PMSHEAD fd = (PMSHEAD)handle;

    if (COMMON_CMD_GETVERSION == cmd)
    {
        if ((NULL == param) || (param_len < (sint32)sizeof(SWVERSION)))
        {
            return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
        }

        *(PSWVERSION)param = swver;
        return 0;
    }

    if ((handle <= 0) || (0 == FLAG_VERIFY(fd)))
    {
        MSHEAD_TRACE_ERR("handle(0x%x) is invalid\n", handle);
        return ERRNO(COMMON_ERROR_HANDLEINVAL, COMP_NENT);
    }

    switch (cmd)
    {
    case COMMON_CMD_RESET:
        fd->mshsize = MSHEAD_LEN;
        break;
    case COMMON_CMD_GETHEALTHY:
        if ((NULL == param) || (param_len < (sint32)sizeof(SWVERSION)))
        {
            return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
        }
        *param = 0;
        break;
    case COMMON_CMD_GETPID:
        if ((NULL == param) || (param_len < (sint32)sizeof(SWVERSION)))
        {
            return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
        }
        *param = 0;
        break;
    default:
        MSHEAD_TRACE_ERR("unsupported cmd, 0x%x\n", cmd);
        return ERRNO(COMMON_ERROR_NOCMD, COMP_NENT);
    }

    return 0;
}

/******************************************************************************
 * 函数介绍: 关闭MSHEAD,该函数将清除句柄所有数据,并释放相关资源
 * 输入参数: handle: MSHEAD句柄;
 * 输出参数: 无
 * 返回值  : 0-成功,<0-错误代码
 *****************************************************************************/
sint32 mshead_close(sint32 handle)
{
    PMSHEAD fd = (PMSHEAD)handle;

    if ((handle <= 0) || (0 == FLAG_VERIFY(fd)))
    {
        MSHEAD_TRACE_ERR("handle(0x%x) is invalid\n", handle);
        return ERRNO(COMMON_ERROR_HANDLEINVAL, COMP_NENT);
    }

    fd->flag = 0;    
#ifdef _RTSPSRV_TEST
	free(fd);
#else
	mem_manage_free(COMP_NENT, fd);
#endif /* _WIN32 */
	fd = NULL;
	return 0;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
