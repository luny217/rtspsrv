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
    int num;    /* 指定MPOOL_SIZE_E size大小的块数 */
    MPOOL_SIZE_E size;   /* 每一块MPOOL_SIZE_E大小 */
}MPOOL_INIT_S;

/**
*   onvif mpool初始化,可以配置不同块个数和大小
* @block :存放块信息数组的首地址
* @count :数组大小,不能大于MPOOL_MAX_BLOCK.
* @return: >=0,创建成功 <0 失败
*/
int mpool_init(MPOOL_INIT_S * init, int count);

/**
*   onvif mpool申请内存,返回的内存块不小于给定的大小
* @size : MPOOL_SIZE_E指定
* @return: >0,成功,返回申请到的内存首地址  ==NULL,创建失败
*/
void * mpool_malloc(MPOOL_SIZE_E size);
void * mpool_calloc(MPOOL_SIZE_E size);

/**
*   onvif mpool释放内存,
* @ p   :mpool_malloc/mpool_calloc返回的地址
*/
void mpool_free(void * p);

/**
*   onvif mpool获取枚举对应的内存大小
*   @size: MPOOL_SIZE_E指定
*   @return: 返回的实际大小(字节)
*/
int mpool_get_size(MPOOL_SIZE_E size);

#endif