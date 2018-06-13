#ifndef _FIFO_H_
#define _FIFO_H_

#include "common.h"

#define FIFO_MAX_BUFF_COUNT 2
#define FIFO_MAX_BUFF_SIZE  0xFFFFFFFF

/* ��ģ��ֻ������ͬһ�����еĲ�ͬ�̵߳���,�������ڽ��̼�ͨѶ;��֧������ģʽ */
typedef enum
{
    FIFO_WATER_LOW      = -1,   /* FIFO��ˮλ */
    FIFO_WATER_MIDDLE   = 0,    /* FIFO��ˮλ */
    FIFO_WATER_HIGH     = 1,    /* FIFO��ˮλ */
    FIFO_WATER_HIGHEST  = 2,    /* FIFO���ˮλ */
} FIFO_WATER_E;

typedef enum
{
    FIFO_LINKSTAT_NO    = 0,    /* ����ƴ�� */
    FIFO_LINKSTAT_YES   = 1,    /* ��Ҫƴ�� */
} FIFO_LINKSTAT_E;

typedef enum
{
    FIFO_PEEK_YES   = 0,    /* ������� */
    FIFO_PEEK_NO    = 1,    /* ���ݲ���� */
} FIFO_PEEK_E;

/* ���������,��������֧��4�� */
typedef enum
{
    FIFO_CONSUMER_NUL = -1,     /* ��Ч�������� */
    FIFO_CONSUMER_FIR = 0,      /* ��һ�������� */
    FIFO_CONSUMER_SEC,          /* �ڶ��������� */
    FIFO_CONSUMER_THR,          /* ������������ */
    FIFO_CONSUMER_FOR,          /* ���ĸ������� */
    FIFO_CONSUMER_MAX = 64,     /* ���֧�ֵ������߸��� */
    FIFO_CONSUMER_ALL = 0xFF,   /* ���������� */
} FIFO_CONSUMER_E;

/* ���º궨��Ϊ������Ϣ */
typedef enum
{
    FIFO_ERROR_FULL = COMP_ERROR_START, /* FIFO���� */
    FIFO_ERROR_EMPTY,                   /* FIFO�ѿ� */
    FIFO_ERROR_NOBUF,                   /* FIFOû���㹻���� */
    FIFO_ERROR_NODAT,                   /* FIFOû���㹻���� */
    FIFO_ERROR_MSHEAD,                  /* MSHEAD���� */
    FIFO_ERROR_FOPEN,                   /* ���ļ����� */
    FIFO_ERROR_FWRITE,                  /* д���ļ����� */
    FIFO_ERROR_FSEEK,                   /* �ƶ��ļ����ָ����� */
    FIFO_ERROR_FTELL,                   /* �ƶ��ļ����ָ�뵽�ļ�β����(40) */
    FIFO_ERROR_EXTBUFTOSMALL,           /* �ⲿBUFTTER̫С */
    FIFO_ERROR_UNMATCHMSHEAD,           /* ƥ��MSHEAD���� */
    FIFO_ERROR_KEYFRMNOTEXIST,          /* �ؼ�֡������ */
    FIFO_ERROR_CCICIUNMATCHED,          /* CCI��CIֵ��ƥ��,�������� */
    FIFO_ERROR_REACHMAXCH,              /* �ﵽ���ͨ���� */
} FIFO_ERROR_E;

/* ���º궨��Ϊfifo_ioctrl()��cmd���� */
typedef enum
{
    FIFO_CMD_GETFIFOSIZE = CMDNO(0, COMP_FIFO), /* ��ȡFIFO��������С,channel(�˴���Ч),param����Ϊ(sint32 *) */
    FIFO_CMD_GETDATASIZE,       /* ��ȡFIFO�����ݴ�С,channel(ALL_CHANNEL-��ȡ��������������ݳ���,FIFO_CONSUMER_E-��ȡ��Ӧ������������ݳ���),param����Ϊ(sint32 *) */
    FIFO_CMD_GETFREESIZE,       /* ��ȡFIFO�п��д�С,channel(ALL_CHANNEL-��ȡ����������С���г���,FIFO_CONSUMER_E-��ȡ��Ӧ��������ſ��г���),param����Ϊ(sint32 *) */
    FIFO_CMD_GETCURWATER,       /* ��ȡFIFO�е�ǰWATER(FIFO_WATER_E),channel(FIFO_CONSUMER_E-���������),param����Ϊ(sint32 *) */
    FIFO_CMD_GETHIWATER,        /* ��ȡFIFO��HIWATER��С(��λΪ%,LOWATER~HTWATER),channel(FIFO_CONSUMER_E-���������),param����Ϊ(sint32 *) */
    FIFO_CMD_SETHIWATER,        /* ����FIFO��HIWATER��С(��λΪ%,LOWATER~HTWATER),channel(FIFO_CONSUMER_E-���������),param����Ϊ(sint32) */
    FIFO_CMD_GETLOWATER,        /* ��ȡFIFO��LOWATER��С(��λΪ%,0~LOWATER),channel(FIFO_CONSUMER_E-���������),param����Ϊ(sint32 *) */
    FIFO_CMD_SETLOWATER,        /* ����FIFO��LOWATER��С(��λΪ%,0~LOWATER),channel(FIFO_CONSUMER_E-���������),param����Ϊ(sint32) */
    FIFO_CMD_SETKEYFLAG,        /* ���ùؼ�֡��־,param����Ϊ(sint32) */
    FIFO_CMD_GETTIMECOUNT,      /* ��ȡtimecount,channel(FIFO_CONSUMER_E-���������),param����Ϊ(sint32 *) */
    FIFO_CMD_SETTIMECOUNT,      /* ����timecount,channel(FIFO_CONSUMER_E-���������),param����Ϊ(sint32,<0-timecount-1,>0-timecount+1) */
    FIFO_CMD_LOCK,              /* ����FIFO��� */
    FIFO_CMD_UNLOCK,            /* ����FIFO��� */
    FIFO_CMD_GETVFRAMECOUNT,    /* ��ȡFIFO����Ƶ֡��,channel(FIFO_CONSUMER_E-���������),param����Ϊ(sint32 *) */
    FIFO_CMD_GETLINKSTAT,       /* ��ȡFIFO������Ƶ֡�Ƿ����ƴ��,channel(FIFO_CONSUMER_E-���������),param����Ϊ(sint32 *(FIFO_LINKSTAT_E)) */
    FIFO_CMD_GETFREESIZESTAT,   /* ����ʵ����Ҫ�Ŀ��д�С,��ȡFIFO�п��д�С�Ƿ�ʵ����Ҫ�Ŀ��д�С״̬,����ֵ<0-�����,>=0��ʾ�����߿��д�С״̬(��λ��ʾ,0-����,1-������),
                                 * channel(ALL_CHANNEL-����������,FIFO_CONSUMER_E-���������),param����Ϊ(sint32 *,��ʾʵ����Ҫ�Ŀ��д�С) */
    FIFO_CMD_GETHTWATER,        /* ��ȡFIFO��HTWATER��С(��λΪ%,HIWATER~100),channel(FIFO_CONSUMER_E-���������),param����Ϊ(sint32 *) */
    FIFO_CMD_SETHTWATER,        /* ����FIFO��HTWATER��С(��λΪ%,HIWATER~100),channel(FIFO_CONSUMER_E-���������),param����Ϊ(sint32) */
    FIFO_CMD_GETHTWATERSTAT,    /* ��ȡFIFO��HTWATER״̬,channel(ALL_CHANNEL-����������,FIFO_CONSUMER_E-���������),
                                 * param����Ϊ(��ʾˮλ״̬(��λ��ʾ��Ӧ������ˮλ״̬,0-δ����,1-�ѵ���)) */
    FIFO_CMD_GETHIWATERSTAT,    /* ��ȡFIFO��HIWATER״̬,channel(ALL_CHANNEL-����������,FIFO_CONSUMER_E-���������),
                                 * param����Ϊ(��ʾˮλ״̬(��λ��ʾ��Ӧ������ˮλ״̬,0-δ����,1-�ѵ���)) */
    FIFO_CMD_GETLOWATERSTAT,    /* ��ȡFIFO��LOWATER״̬,channel(ALL_CHANNEL-����������,FIFO_CONSUMER_E-���������),
                                 * param����Ϊ(��ʾˮλ״̬(��λ��ʾ��Ӧ������ˮλ״̬,0-δ����,1-�ѵ���)) */
    FIFO_CMD_CONSUMERSTAT,      /* ��ȡFIFO������״̬,����ֵ<0-�����,>=0��ʾ�����߸���,channel(�˴���Ч),param����Ϊ(sint32 *,������״̬,��λ��ʾ,0-������,1-����) */
    FIFO_CMD_SETNEWFULKEYFRM,   /* ���õ����������ؼ�֡λ��,channel(ALL_CHANNEL-����������,FIFO_CONSUMER_E-���������) */
} FIFO_CMD_E;

typedef struct
{
    uint32 size;    /* �ڴ滺������С */
    void *pbuff;    /* �ڴ滺����ָ�� */
} FIFO_BUFF_S, *PFIFO_BUFF_S;
#define FIFO_BUFF_S_LEN sizeof(FIFO_BUFF_S)

typedef enum
{
    FIFO_BUFF_HEAD_DATA,    /* �������׵�ַ������ */
    FIFO_BUFF_HEAD_MSHEAD,  /* �������׵�ַ��MSHEADͷ��Ϣ */
} FIFO_BUFF_HEAD_TYPE_E;

typedef struct
{
    uint8 buff_count;   /* �ڴ滺�������� */
    uint8 buff_head;    /* �ڴ滺�����������׵�ַ��������(FIFO_BUFF_HEAD_TYPE_E) */
    uint8 reserved[3];  /* ����λ */
    FIFO_BUFF_S buff[FIFO_MAX_BUFF_COUNT];  /* �ڴ滺������Ϣ�ṹ�� */
} FIFO_BUFF_INFO_S, *PFIFO_BUFF_INFO_S;
#define FIFO_BUFF_INFO_S_LEN    sizeof(FIFO_BUFF_INFO_S)

/******************************************************************************
 * ��������: ����FIFO(��FIFO��С���ڵ���128KBʱ,FIFO������ƴ�ӻ�����128KB)
 * �������: size: FIFO��С,��λbytes,Ϊ�Ż�����,size�������1<<N
 *           hiwater: �߾���λ��С,��λ%,0~100
 *           lowater: �;���λ��С,��λ%,0~100
 * �������: ��
 * ����ֵ  : <0-ʧ��,>0-FIFO���
 *****************************************************************************/
sint32 fifo_open(uint32 size, uint32 hiwater, uint32 lowater);

/******************************************************************************
 * ��������: ����FIFO��չ�ӿ�
 * �������: size: FIFO��С,��λbytes,Ϊ�Ż�����,size�������1<<N
 *           hiwater: �߾���λ��С,��λ%,0~100
 *           lowater: �;���λ��С,��λ%,0~100
 *           resv_size: ����FIFO��С,��λbytes,Ϊ�Ż�����,resv_size�������1<<N,
 *                      ��Ҫ��������Ƶ���ݲ���������ƴ��,���������Ƶ���֡��������;
 *                      �����СΪ0ʱ,ƴ�������,�����ⲿ��ǰ�����ڴ���ڴ�,ͨ��fifo_ioctrl
 *                      ��fifo_ioctrl����FIFO_CMD_GETLINKSTAT��ѯ��,ͨ��fifo_readframe����
 * �������: ��
 * ����ֵ  : <0-ʧ��,>0-FIFO���
 *****************************************************************************/
sint32 fifo_openext(uint32 size, uint32 hiwater, uint32 lowater, uint32 resv_size);

/******************************************************************************
 * ��������: ���ݲ�ͬ�������ߴ���FIFO(ÿ��ֻ�ܴ���һ��������,��һ����ʱ,ֻ�Ǵ���FIFO,�������κ�������)
 * �������: handle: FIFO���,�״δ���ʱ,�����Ҫ<=0,����-�Ѵ�����FIFO���
 *           consumer: ���������,ÿ��������ֻ�ܴ���һ��,����������ʱ(FIFO_CONSUMER_E):
 *                     FIFO_CONSUMER_NUL-����һ��δʹ�õ�������,����-������Ӧ��������
 *           size: FIFO��С,��λbytes,Ϊ�Ż�����,size�������1<<N(�״δ���ʱ��Ч,����ʱ��Ч)
 *           htwater: ��߾���λ��С(��λΪ%,hiwater~100)
 *           hiwater: �߾���λ��С(��λΪ%,lowater~htwater)
 *           lowater: �;���λ��С(��λΪ%,0~hiwater)
 *           resv_size: ����FIFO��С,��λbytes(�״δ���ʱ��Ч,����ʱ��Ч)
 *                      (Ϊ�Ż�����,resv_size�������1<<N,��Ҫ��������Ƶ���ݲ���������ƴ��,
 *                      ���������Ƶ���֡��������;�����СΪ0ʱ,ƴ�������,�����ⲿ��ǰ����
 *                      �ڴ���ڴ�,ͨ��fifo_ioctrl��fifo_ioctrl����FIFO_CMD_GETLINKSTAT��ѯ��,
 *                      ͨ��fifo_readframe����)
 * �������: ��
 * ����ֵ  : <0-ʧ��,>=0-��һ����ʱ,����FIFO���,�����������������(FIFO_CONSUMER_E)
 *****************************************************************************/
sint32 fifo_openmulti(sint32 handle, FIFO_CONSUMER_E consumer, uint32 size, 
    uint32 htwater, uint32 hiwater, uint32 lowater, uint32 resv_size);

/******************************************************************************
 * ��������: д�����ݵ�FIFO,�ú���һ��д�볤��size����,��FIFO�л��岻��ʱ�����ش�����Ϣ
 * �������: handle: FIFO���
 *           data: ������ָ��
 *           size: ��д���ݳ���
 * �������: ��
 * ����ֵ  : >0-д�����ݴ�С,<0-�������
 *****************************************************************************/
sint32 fifo_write(sint32 handle, char *data, uint32 size);

/******************************************************************************
 * ��������: д�����ݵ�FIFO,�ú���һ��д�볤��size����,��FIFO�л��岻��ʱ�����ش�����Ϣ
 *           �˺�����fifo_write����չ,Ϊ����Ƶ��д������,���MSHEADͷ��Ϣ���ж�,
 *           ����Ԥ¼ʱ���֡��������Ӧ����
 * �������: handle: FIFO���
 *           data: ������ָ��
 *           size: ��д���ݳ���
 * �������: ��
 * ����ֵ  : >0-д�����ݴ�С,<0-�������
 *****************************************************************************/
sint32 fifo_writeext(sint32 handle, char *data, uint32 size);

/******************************************************************************
 * ��������: ��ȡFIFO�п�д�Ļ�������Ϣ
 * �������: handle: FIFO���
 *           pbuff_info: ��������Ϣ�ṹ��ָ��
 *           need_size: ��Ҫ�Ļ�������С
 * �������: ��
 * ����ֵ  : 0-�ɹ�,<0-�������
 *****************************************************************************/
sint32 fifo_write_buffer_get(sint32 handle, PFIFO_BUFF_INFO_S pbuff_info, uint32 need_size);

/******************************************************************************
 * ��������: ����FIFO�п�д�Ļ�������Ϣ
 * �������: handle: FIFO���
 *           pbuff_info: ��������Ϣ�ṹ��ָ��
 *           update_size: ���µĻ�������С
 * �������: ��
 * ����ֵ  : 0-�ɹ�,<0-�������
 *****************************************************************************/
sint32 fifo_write_buffer_update(sint32 handle, PFIFO_BUFF_INFO_S pbuff_info, uint32 update_size);

/******************************************************************************
 * ��������: ��FIFO�ж�ȡ����,�ú���һ�ζ�ȡ����size����,��FIFO�����ݲ���ʱ�����ش�����Ϣ
 * �������: handle: FIFO���
 *           data: ������ָ��(��ֵ����Ϊ��(NULL),Ϊ��ʱ,���ݽ����ᱻ����)
 *           size: ��ȡ��С
 *           peek: �Ƿ����FIFO���Ѷ�����(FIFO_PEEK_E)
 * �������: ��
 * ����ֵ  : >0-�������ݴ�С,<0-�������
 *****************************************************************************/
sint32 fifo_read(sint32 handle, char *data, uint32 size, FIFO_PEEK_E peek);

/******************************************************************************
 * ��������: ���ݲ�ͬ�������ߴ�FIFO�ж�ȡ����,�ú���һ�ζ�ȡ����size����,��FIFO�����ݲ���ʱ�����ش�����Ϣ
 * �������: handle: FIFO���
 *           consumer: ���������(FIFO_CONSUMER_E)
 *           data: ������ָ��(��ֵ����Ϊ��(NULL),Ϊ��ʱ,���ݽ����ᱻ����)
 *           size: ��ȡ��С
 *           peek: �Ƿ����FIFO���Ѷ�����(FIFO_PEEK_E)
 * �������: ��
 * ����ֵ  : >0-�������ݴ�С,<0-�������
 *****************************************************************************/
sint32 fifo_readmulti(sint32 handle, FIFO_CONSUMER_E consumer, char *data, uint32 size, FIFO_PEEK_E peek);

/******************************************************************************
 * ��������: ��FIFO�ж�ȡ����,�ú���һ�ζ�ȡ1֡����,��FIFO�����ݲ���ʱ�����ش�����Ϣ
 * �������: handle: FIFO���
 *           data: ������ָ��(��ֵ����Ϊ��(NULL),Ϊ��ʱ,���ݽ����ᱻ����)
 *           limit: ��������С(dataΪ��(NULL)ʱ,��ֵ�������޴�)
 *           peek: �Ƿ����FIFO���Ѷ�����(FIFO_PEEK_E)
 * �������: ��
 * ����ֵ  : >0-�������ݴ�С,0-����������,<0-�������
 *****************************************************************************/
sint32 fifo_readframe(sint32 handle, char *data, uint32 limit, FIFO_PEEK_E peek);

/******************************************************************************
 * ��������: ���ݲ�ͬ�������ߴ�FIFO�ж�ȡ����,�ú���һ�ζ�ȡ1֡����,��FIFO�����ݲ���ʱ�����ش�����Ϣ
 * �������: handle: FIFO���
 *           consumer: ���������(FIFO_CONSUMER_E)
 *           data: ������ָ��(��ֵ����Ϊ��(NULL),Ϊ��ʱ,���ݽ����ᱻ����)
 *           limit: ��������С(dataΪ��(NULL)ʱ,��ֵ�������޴�)
 *           peek: �Ƿ����FIFO���Ѷ�����(FIFO_PEEK_E)
 * �������: ��
 * ����ֵ  : >0-�������ݴ�С,0-����������,<0-�������
 *****************************************************************************/
sint32 fifo_readframemulti(sint32 handle, FIFO_CONSUMER_E consumer, char *data, uint32 limit, FIFO_PEEK_E peek);

/******************************************************************************
 * ��������: ��FIFO�ж�ȡ����,�ú���һ�ζ�ȡ��֡����Ƶ����,ֱ��������һ���ؼ�֡
 * �������: handle: FIFO���
 *           data: ������ָ��(��ֵ����Ϊ��(NULL),Ϊ��ʱ,���ݽ����ᱻ����)
 *           limit: ��������С(dataΪ��(NULL)ʱ,��ֵ�������޴�)
 *           nexttype: ��һ֡��������
 * �������: ��
 * ����ֵ  : >0-�������ݴ�С,<0-�������
 *****************************************************************************/
sint32 fifo_readext(sint32 handle, char *data, uint32 limit, sint32 *nexttype);

/******************************************************************************
 * ��������: ���ݲ�ͬ�������ߴ�FIFO�ж�ȡ����,�ú���һ�ζ�ȡ��֡����Ƶ����,ֱ��������һ���ؼ�֡
 * �������: handle: FIFO���
 *           consumer: ���������(FIFO_CONSUMER_E)
 *           data: ������ָ��(��ֵ����Ϊ��(NULL),Ϊ��ʱ,���ݽ����ᱻ����)
 *           limit: ��������С(dataΪ��(NULL)ʱ,��ֵ�������޴�)
 *           nexttype: ��һ֡��������
 * �������: ��
 * ����ֵ  : >0-�������ݴ�С,<0-�������
 *****************************************************************************/
sint32 fifo_readextmulti(sint32 handle, FIFO_CONSUMER_E consumer, char *data, uint32 limit, sint32 *nexttype);

/******************************************************************************
 * ��������: ��FIFO�ж�ȡ����,�ú���һ�ζ�ȡ1֡���µ�������I֡����
 * �������: handle: FIFO���
 *           data: ������ָ��
 *           limit: ��������С
 *           have_mshead: �Ƿ����MSHEAD(HB_TRUE-��,HB_FALSE-��)
 * �������: ��
 * ����ֵ  : >0-�������ݴ�С,<0-�������
 *****************************************************************************/
sint32 fifo_read_newfull_keyframe(sint32 handle, char *data, uint32 limit, HB_BOOL have_mshead);

/******************************************************************************
 * ��������: ��ȡFIFO�пɶ��Ļ�������Ϣ
 * �������: handle: FIFO���
 *           pbuff_info: ��������Ϣ�ṹ��ָ��
 *           need_size: ��Ҫ�Ļ�������С
 * �������: ��
 * ����ֵ  : >=0-�ɹ�,�������ܳ���,<0-�������
 *****************************************************************************/
sint32 fifo_read_buffer_get(sint32 handle, PFIFO_BUFF_INFO_S pbuff_info);

/******************************************************************************
 * ��������: ���ݲ�ͬ�������߻�ȡFIFO�пɶ��Ļ�������Ϣ
 * �������: handle: FIFO���
 *           consumer: ���������(FIFO_CONSUMER_E)
 *           pbuff_info: ��������Ϣ�ṹ��ָ��
 *           need_size: ��Ҫ�Ļ�������С
 * �������: ��
 * ����ֵ  : >=0-�ɹ�,�������ܳ���,<0-�������
 *****************************************************************************/
sint32 fifo_read_buffer_getmulti(sint32 handle, FIFO_CONSUMER_E consumer, PFIFO_BUFF_INFO_S pbuff_info);

/******************************************************************************
 * ��������: ����FIFO�пɶ��Ļ�������Ϣ
 * �������: handle: FIFO���
 *           pbuff_info: ��������Ϣ�ṹ��ָ��
 *           update_size: ���µĻ�������С
 * �������: ��
 * ����ֵ  : 0-�ɹ�,<0-�������
 *****************************************************************************/
sint32 fifo_read_buffer_update(sint32 handle, PFIFO_BUFF_INFO_S pbuff_info, uint32 update_size);

/******************************************************************************
 * ��������: ���ݲ�ͬ�������߸���FIFO�пɶ��Ļ�������Ϣ
 * �������: handle: FIFO���
 *           consumer: ���������(FIFO_CONSUMER_E)
 *           pbuff_info: ��������Ϣ�ṹ��ָ��
 *           update_size: ���µĻ�������С
 * �������: ��
 * ����ֵ  : 0-�ɹ�,<0-�������
 *****************************************************************************/
sint32 fifo_read_buffer_updatemulti(sint32 handle, FIFO_CONSUMER_E consumer, PFIFO_BUFF_INFO_S pbuff_info, uint32 update_size);

/******************************************************************************
 * ��������: ��FIFO�ж�ȡһ֡��Ƶ���ݲ�����,�ú���һ�ζ�ȡһ֡����Ƶ���ݺ�N����Ƶ֡
 * �������: handle: FIFO���
 *           func: ���ݴ���ص�����
 *           func_idx: datacallback()�����е�idx
 *           func_handle: datacallback()�����е�fifo,�˴����ڻص������ľ��,����MFS����ȵ�
 * �������: ��
 * ����ֵ  : >0-�Ѵ������ݴ�С,0-���ݲ���һ֡,��FIFO��resv_sizeΪ0ʱ,FIFO֡����Ҫ����ͷ����,<0-�������
 *****************************************************************************/
sint32 fifo_frameprocess(sint32 handle, datacallback func, sint32 func_idx, sint32 func_handle);

/******************************************************************************
 * ��������: ���ݲ�ͬ�������ߴ�FIFO�ж�ȡ���ݲ�����,�ú���һ�ζ�ȡһ֡��Ƶ���ݺ�N����Ƶ֡
 * �������: handle: FIFO���
 *           consumer: ���������(FIFO_CONSUMER_E)
 *           func: ���ݴ���ص�����
 *           func_idx: datacallback()�����е�idx
 *           func_handle: datacallback()�����е�fifo,�˴����ڻص������ľ��,����MFS����ȵ�
 * �������: ��
 * ����ֵ  : >0-�Ѵ������ݴ�С,0-���ݲ���һ֡,��FIFO��resv_sizeΪ0ʱ,FIFO֡����Ҫ����ͷ����,<0-�������
 *****************************************************************************/
sint32 fifo_frameprocessmulti(sint32 handle, FIFO_CONSUMER_E consumer, 
    datacallback func, sint32 func_idx, sint32 func_handle);

/******************************************************************************
 * ��������: ��FIFO�ж�ȡһ֡��Ƶ����Ƶ���ݲ�����,�ú���һ��ֻ��ȡһ֡����Ƶ����Ƶ����
 * �������: handle: FIFO���
 *           func: ���ݴ���ص�����
 *           func_idx: datacallback()�����е�idx
 *           func_handle: datacallback()�����е�fifo,�˴����ڻص������ľ��,����MFS����ȵ�
 *           resvered: ��������
 * �������: ��
 * ����ֵ  : >0-�Ѵ������ݴ�С,0-���ݲ���һ֡,��FIFO��resv_sizeΪ0ʱ,FIFO֡����Ҫ����ͷ����,<0-�������
 *****************************************************************************/
sint32 fifo_frameprocessext(sint32 handle, datacallback func, sint32 func_idx, sint32 func_handle, sint32 resvered);

/******************************************************************************
 * ��������: ���ݲ�ͬ�������ߴ�FIFO�ж�ȡһ֡��Ƶ����Ƶ���ݲ�����,�ú���һ��ֻ��ȡһ֡����Ƶ����Ƶ����
 * �������: handle: FIFO���
 *           consumer: ���������(FIFO_CONSUMER_E)
 *           func: ���ݴ���ص�����
 *           func_idx: datacallback()�����е�idx
 *           func_handle: datacallback()�����е�fifo,�˴����ڻص������ľ��,����MFS����ȵ�
 *           resvered: ��������
 * �������: ��
 * ����ֵ  : >0-�Ѵ������ݴ�С,0-���ݲ���һ֡,��FIFO��resv_sizeΪ0ʱ,FIFO֡����Ҫ����ͷ����,<0-�������
 *****************************************************************************/
sint32 fifo_frameprocessextmulti(sint32 handle, FIFO_CONSUMER_E consumer, 
    datacallback func, sint32 func_idx, sint32 func_handle, sint32 resvered);

/******************************************************************************
 * ��������: ��FIFO�ж�ȡ���ݲ�����,�ú���һ�ζ�ȡ��֡����Ƶ����,ֱ��������һ����Ƶ�ؼ�֡
 * �������: handle: FIFO���
 *           func: ���ݴ���ص�����
 *           func_idx: datacallback()�����е�idx
 *           func_handle: datacallback()�����е�fifo,�˴����ڻص������ľ��,����MFS����ȵ�
 * �������: ��
 * ����ֵ  : >0-�Ѵ������ݴ�С,0-���ݲ���һ֡,��FIFO��resv_sizeΪ0ʱ,FIFO֡����Ҫ����ͷ����,<0-�������
 *****************************************************************************/
sint32 fifo_datprocess(sint32 handle, datacallback func, sint32 func_idx, sint32 func_handle);

/******************************************************************************
 * ��������: ���ݲ�ͬ�������ߴ�FIFO�ж�ȡ���ݲ�����,�ú���һ�ζ�ȡ��֡����Ƶ����,ֱ��������һ����Ƶ�ؼ�֡
 * �������: handle: FIFO���
 *           consumer: ���������(FIFO_CONSUMER_E)
 *           func: ���ݴ���ص�����
 *           func_idx: datacallback()�����е�idx
 *           func_handle: datacallback()�����е�fifo,�˴����ڻص������ľ��,����MFS����ȵ�
 * �������: ��
 * ����ֵ  : >0-�Ѵ������ݴ�С,0-���ݲ���һ֡,��FIFO��resv_sizeΪ0ʱ,FIFO֡����Ҫ����ͷ����,<0-�������
 *****************************************************************************/
sint32 fifo_datprocessmulti(sint32 handle, FIFO_CONSUMER_E consumer, 
    datacallback func, sint32 func_idx, sint32 func_handle);

/******************************************************************************
 * ��������: ��FIFO�ж�ȡ���ݲ�����,�ú���һ�ζ�ȡ��֡����Ƶ����,ֱ�������������Ҫ������Ƶ�ؼ�֡��
 * �������: handle: FIFO���
 *           func: ���ݴ���ص�����
 *           func_idx: datacallback()�����е�idx,
 *           func_handle: datacallback()�����е�fifo,�˴����ڻص������ľ��,����MFS����ȵ�
 *           deal_ifrm_nums: ��Ҫһ���Դ���ؼ�֡����,��СֵΪ1
 * �������: ��
 * ����ֵ  : >0-�Ѵ������ݴ�С,0-���ݲ���һ֡,��FIFO��resv_sizeΪ0ʱ,FIFO֡����Ҫ����ͷ����,<0-�������
 *****************************************************************************/
sint32 fifo_datprocessext(sint32 handle, datacallback func, 
    sint32 func_idx, sint32 func_handle, sint32 deal_ifrm_nums);

/******************************************************************************
 * ��������: ���ݲ�ͬ�������ߴ�FIFO�ж�ȡ���ݲ�����,�ú���һ�ζ�ȡ��֡����Ƶ����,
 *           ֱ�������������Ҫ������Ƶ�ؼ�֡��
 * �������: handle: FIFO���
 *           consumer: ���������(FIFO_CONSUMER_E)
 *           func: ���ݴ���ص�����
 *           func_idx: datacallback()�����е�idx
 *           func_handle: datacallback()�����е�fifo,�˴����ڻص������ľ��,����MFS����ȵ�
 *           deal_ifrm_nums: ��Ҫһ���Դ���ؼ�֡����,��СֵΪ1
 * �������: ��
 * ����ֵ  : >0-�Ѵ������ݴ�С,0-���ݲ���һ֡,��FIFO��resv_sizeΪ0ʱ,FIFO֡����Ҫ����ͷ����,<0-�������
 *****************************************************************************/
sint32 fifo_datprocessextmulti(sint32 handle, FIFO_CONSUMER_E consumer, 
    datacallback func, sint32 func_idx, sint32 func_handle, sint32 deal_ifrm_nums);

/******************************************************************************
 * ��������: FIFO�ض�λMSHEAH
 * �������: handle: FIFO���
 *           consumer: ���������(FIFO_CONSUMER_E)
 * �������: ��
 * ����ֵ  : 0-�ɹ�,<0-�������
 *****************************************************************************/
sint32 fifo_relocate_mshead(sint32 handle, FIFO_CONSUMER_E consumer);

/******************************************************************************
 * ��������: FIFO����
 * �������: handle: FIFO���
 *           cmd: ����
 *           channel: ͨ����,���������
 *           param: �������
 *           param_len: param����,�ر����GET����ʱ,�������Ӧ���жϻ������Ƿ��㹻
 * �������: param: �������
 * ����ֵ  : 0-�ɹ�,<0-�������
 *****************************************************************************/
sint32 fifo_ioctrl(sint32 handle, sint32 cmd, sint32 channel, sint32 *param, sint32 param_len);

/******************************************************************************
 * ��������: FIFO��ϢDUMP��ӡ
 * �������: fifo_buff_addr: FIFO�ڴ滺������ַ
 *           consumer: ���������(FIFO_CONSUMER_E)
 * �������: ��
 * ����ֵ  : 0-�ɹ�,<0-�������
 *****************************************************************************/
sint32 fifo_dump_info(char *fifo_buff_addr, FIFO_CONSUMER_E consumer);

/******************************************************************************
 * ��������: FIFO����DUMP��ӡ
 * �������: fifo_buff_addr: FIFO�ڴ滺������ַ
 *           consumer: ���������(FIFO_CONSUMER_E)
 *           fifo_buff_offset: FIFO�ڴ滺����ƫ��
 * �������: ��
 * ����ֵ  : 0-�ɹ�,<0-�������
 *****************************************************************************/
sint32 fifo_dump_data(char *fifo_buff_addr, FIFO_CONSUMER_E consumer, uint32 fifo_buff_offset);

/******************************************************************************
 * ��������: �ر�FIFO,�ú��������fifo��������,���ͷ������Դ
 * �������: handle: FIFO���
 * �������: ��
 * ����ֵ  : 0-�ɹ�,<0-�������
 *****************************************************************************/
sint32 fifo_close(sint32 handle);

/******************************************************************************
 * ��������: ���ݲ�ͬ�������߹ر�FIFO,�ú����������fifo���ݺ��ͷ������Դ,
 *           ����Ҫ���fifo���ݺ��ͷ������Դ,����ú���fifo_close
 * �������: handle: FIFO���
 *           consumer: ���������(FIFO_CONSUMER_E)
 * �������: ��
 * ����ֵ  : 0-�ɹ�,<0-�������
 *****************************************************************************/
sint32 fifo_closemulti(sint32 handle, FIFO_CONSUMER_E consumer);

#endif /* _FIFO_H_ */
