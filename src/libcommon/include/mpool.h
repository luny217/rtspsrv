#ifndef __MPOOL_H__
#define __MPOOL_H__

#define MPOOL_MAX_BLOCK 4

typedef enum
{
    MPOOL_SIZE_INVALID = 0,
    MPOOL_SIZE_32K,
    MPOOL_SIZE_64K,
    MPOOL_SIZE_96K,
    MPOOL_SIZE_128K,
}MPOOL_SIZE_E;

typedef struct
{
    int num;    /* ָ��MPOOL_SIZE_E size��С�Ŀ��� */
    MPOOL_SIZE_E size;   /* ÿһ��MPOOL_SIZE_E��С */
}MPOOL_INIT_S;

/**
*   onvif mpool��ʼ��,�������ò�ͬ������ʹ�С
* @block :��ſ���Ϣ������׵�ַ
* @count :�����С,���ܴ���MPOOL_MAX_BLOCK.
* @return: >=0,�����ɹ� <0 ʧ��
*/
int mpool_init(MPOOL_INIT_S * init, int count);

/**
*   onvif mpool�����ڴ�,���ص��ڴ�鲻С�ڸ����Ĵ�С
* @size : MPOOL_SIZE_Eָ��
* @return: >0,�ɹ�,�������뵽���ڴ��׵�ַ  ==NULL,����ʧ��
*/
void * mpool_malloc(MPOOL_SIZE_E size);
void * mpool_calloc(MPOOL_SIZE_E size);

/**
*   onvif mpool�ͷ��ڴ�,
* @ p   :mpool_malloc/mpool_calloc���صĵ�ַ
*/
void mpool_free(void * p);

/**
*   onvif mpool��ȡö�ٶ�Ӧ���ڴ��С
*   @size: MPOOL_SIZE_Eָ��
*   @return: ���ص�ʵ�ʴ�С(�ֽ�)
*/
int mpool_get_size(MPOOL_SIZE_E size);

#endif