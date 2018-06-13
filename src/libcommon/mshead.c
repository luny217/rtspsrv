/******************************************************************************
 * ģ������: MSHEAD-��ý��ͷ��Ϣ�淶
 * �޸ļ�¼: 
 *  1. 2016-08-25 V3.1.0
 *  1). �޸Ķ���Ϣ����δ�ۼӵ�����
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

static SWVERSION swver = {0, 1, 3, 25, 8, 16}; /* ģ��汾���� */

/******************************************************************************
 * ��������: ����һ֡���ݵ�У���
 * �������: handle: MSHEAD���
 * �������: data: ����Ƶ���ݻ�������ַָ��
 *           size: ����Ƶ���ݳ���ָ��
 * ����ֵ: <=0-ʧ��,>0-���ݵ�У���
 *****************************************************************************/
static sint32 _mshead_generate_checksum(PMSHEAD fd, sint8 *data, uint32 size)
{
    uint32 checksum = 0; /* ��0��ʼ�ۼ�У���,��У��͵��㷨�����Ժܵ�,ֻ�ܽϵ�ǿ����֤���ݵ���ȷ�� */
    const uint8 *ptmp = NULL;
    uint32 mshsize = MSHEAD_GETMSHSIZE(fd), checked_size = 0, i = 0;
    
    if (mshsize > MSHEAD_LEN) /* ����MSHEAD_SEG_VIDEO_S��MSHEAD_SEGMENT[0 : n-1]��У��� */
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
    if ((checked_size > 0) && ((checked_size << 1) <= size)) /* ����������ݵ�У��� */
    {
        if (NULL == data)
        {
            MSHEAD_TRACE_ERR("unsupported data is NULL\n");
            return 0;
        }
        ptmp = (const uint8 *)data; /* ǰһ���� */
        for (i = 0; i < checked_size; i++)
        {
            checksum += ptmp[i];
        }
        ptmp = (const uint8 *)(data) + (size - checked_size); /* ��һ���� */
        for (i = 0; i < checked_size; i++)
        {
            checksum += ptmp[i];
        }
        TRACE_DBG("checked_size(%d) checksum(0x%x)\n", checked_size, checksum);
    }
    checksum &= 0x00FFFFFF; /* У��͵�24λ��Ч */
    checksum |= 0x00800000; /* ��֤У��Ͳ�Ϊ0 */
 	TRACE_DBG("size(%d) checksum(0x%x)\n", size, checksum);
	
	return checksum;
}

/******************************************************************************
 * ��������: д��֡��Ϣ����,�������ݲ����п���
 * �������: handle: MSHEAD���
 *           frame_type: ֡����(MSHEAD_STREAM_FRAME_TYPE_E)
 *           en_checksum: �Ƿ���Ҫ����У���
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
 *    (ע��: ���Ȳ���MSHEAD_HEAD_ALIGN_SIZE�ֽڶ��뽫�Զ��ӳ������,H265������4�ֽڶ���)
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
 * ��������: ������ͷ,�˴���Ϊ�������max_mshsize�ֽ��ڴ�,
 *           ���а���MSHEAD�ṹ�峤��,���Ⱥ����ڲ����Զ�MSHEAD_HEAD_ALIGN_SIZE�ֽڶ���
 * �������: fd: MSHEAD���
 *           algorithm: �����׼
 *           max_mshsize: ý��ͷ��Ϣ��󳤶�,��С����ܳ���MSHEAD_MAXHEADSIZE
 *           checked_encode_data_size: ��ҪУ��ı������ݵĴ�С,ȡֵ��Χ[0, 255];һ��ֻУ��������ݵ�ǰ,�󲿷��ֽ�;
 *                                     ��checked_encode_data_size==0ʱ,��У���������;����:����У��ǰ���16���ֽڵı�������
 * �������: ��
 * ����ֵ  : <0-����,>0-�ɹ�,��ʾ�����ľ��
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
 * ��������: ������ͷ,�˴���Ϊ�������max_mshsize�ֽ��ڴ�,
 *           ���а���MSHEAD�ṹ�峤��,���Ⱥ����ڲ����Զ�MSHEAD_HEAD_ALIGN_SIZE�ֽڶ���
 * �������: algorithm: �����׼
 *           max_mshsize: ý��ͷ��Ϣ��󳤶�,��С����ܳ���MSHEAD_MAXHEADSIZE
 *           checked_encode_data_size: ��ҪУ��ı������ݵĴ�С,ȡֵ��Χ[0, 255];һ��ֻУ��������ݵ�ǰ,�󲿷��ֽ�;
 *                                     ��checked_encode_data_size==0ʱ,��У���������;����:����У��ǰ���16���ֽڵı�������
 * �������: ��
 * ����ֵ  : <0-����,>0-�ɹ�,��ʾ�����ľ��
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
 * ��������: д�����Ϣ����MSHEAD_SEG
 * �������: handle: MSHEAD���
 *           seg: ��ͷ��Ϣ,��size,data�����������û�����
 *           data: �û�����
 *           size: �û����ݳ���,��Ϊ�ַ���Ӧ����β��'\0'�ĳ���
 * �������: ��
 * ����ֵ  : <0-����,>0-�ɹ�,��ʾ��ǰ��ͷ����(ע��: ���Ȳ���MSHEAD_HEAD_ALIGN_SIZE�ֽڶ��뽫�Զ��ӳ������)
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
 * ��������: ����֡��Ϣ������У��ֵ(��������е�SEG����Ϣд�����ܸ���)
 * �������: handle: MSHEAD���
 *           data: ����Ƶ���ݻ�����(ʵ��δʹ��)
 *           size: ����Ƶ���ݳ���
 * �������: ��
 * ����ֵ  : 0-�ɹ�,<0-�������
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
 * ��������: ��ȡ����Ƶ����,��ȡ����ָ��,�Լ�����
 * �������: handle: MSHEAD���
 * �������: data: ����Ƶ���ݻ�������ַָ��
 *           size: ����Ƶ���ݳ���ָ��
 * ����ֵ  : <0-����,>0-�ɹ�,��������Ƶ���ݻ�����ָ��
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
 * ��������: ��ȡ��ͷ��Ϣ�е�һ������Ϣ�ĵ�ַ
 * �������: handle: MSHEAD���
 *           index: MSHEAD_SEG���������
 * ����ֵ: <0-ʧ��,0-������޿�����Ϣ,>0-��ָ���ȡ�Ķ���Ϣָ��PMSHEAD_SEG
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
 * ��������: ����һ֡���ݵ�У���
 * �������: handle: MSHEAD���
 * �������: data: ����Ƶ���ݻ�������ַָ��
 *           size: ����Ƶ���ݳ���ָ��
 * ����ֵ: <=0-ʧ��,>0-���ݵ�У���
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
 * ��������: MSHEAD����
 * �������: handle: MSHEAD���
 *           cmd: ����
 *           channel: ͨ����,�˴���Ч
 *           param: �������
 *           param_len: param����,�ر����GET����ʱ,�������Ӧ���жϻ������Ƿ��㹻
 * �������: param: �������
 * ����ֵ  : 0-�ɹ�,<0-�������
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
 * ��������: �ر�MSHEAD,�ú�������������������,���ͷ������Դ
 * �������: handle: MSHEAD���;
 * �������: ��
 * ����ֵ  : 0-�ɹ�,<0-�������
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
