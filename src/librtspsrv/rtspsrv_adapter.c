/*
 * FileName:     rtspsrv_adpater.c
 * Author:         luny  Version: 1.0  Date: 2017-6-13
 * Description:
 * Version:
 * Function List:
 *                 1.
 * History:
 *     <author>   <time>    <version >   <desc>
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "avutil_random_seed.h"
#include "sstring.h"
#include "rtspsrv_defines.h"
#include "rtspsrv_utils.h"

#ifdef _RTSPSRV_TEST
#include "avutil_log.h"
#endif


/******************************************************************************
* 函数介绍:	获取编码类型
* 输入参数:	ch_idx: 通道号;
*           stm_type: 流类型;
* 输出参数:	无
* 返回值:   成功返回sps的长度
*****************************************************************************/
int rtspsrv_adpt_get_enc_type(int ch_idx, int stm_type)
{
	int enc_type = VIDEO_H265;

	return enc_type;
}

/******************************************************************************
* 函数介绍:	获取VPS
* 输入参数:	ch_idx: 通道号;
*           stm_type: 流类型;
*           vps_buf: buf缓存地址;
* 输出参数:	无
* 返回值:   成功返回vps的长度
*****************************************************************************/
int rtspsrv_adpt_get_vps(int ch_idx, int stm_type, char * vps_buf, int * vps_len)
{
	char vps[] = { 0x01, 0x0C, 0x01, 0xFF, 0xFF, 0x01, 0x60, 0x00, 0x00, 0x03, 
		0x00, 0x90, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x78, 0xBA, 0x02, 0x40 };
    memcpy(vps_buf, vps, sizeof(vps));
    *vps_len = sizeof(vps);
    return 0;
}

/******************************************************************************
* 函数介绍:	获取SPS
* 输入参数:	ch_idx: 通道号;
*					stm_type: 流类型;
*					sps_buf: buf缓存地址;
* 输出参数:	无
* 返回值:		成功返回sps的长度
*****************************************************************************/
int rtspsrv_adpt_get_sps(int ch_idx, int stm_type, char * sps_buf, int * sps_len)
{
    char sps[] = {0x4D, 0x00, 0x1F, 0x95, 0xA8, 0x14, 0x01, 0x6E, 0x9B, 0x80, 0x80, 0x80, 0x81};
    memcpy(sps_buf, sps, sizeof(sps));
    *sps_len = sizeof(sps);
    return 0;
}

/******************************************************************************
* 函数介绍:	获取PPS
* 输入参数:	ch_idx: 通道号;
*					stm_type: 流类型;
*					pps_buf: buf缓存地址;
* 输出参数:	无
* 返回值:		成功返回sps的长度
*****************************************************************************/
int rtspsrv_adpt_get_pps(int ch_idx, int stm_type, char * pps_buf, int * pps_len)
{
    char pps[] = {0xEE, 0x3C, 0x80};
    memcpy(pps_buf, pps, sizeof(pps));
    *pps_len = sizeof(pps);
    return 0;
}

/******************************************************************************
* 函数介绍:	获取本机IP地址
* 输入参数:	ch_idx: 通道号;
*					stm_type: 流类型;
*					pps_buf: buf缓存地址;
* 输出参数:	无
* 返回值:		成功返回sps的长度
*****************************************************************************/
char * rtspsrv_adpt_get_ip(void)
{
#if 1 /*def _WIN32*/
    static char ipaddr[] = { "10.0.0.50" };
    return ipaddr;
#else
    int  i;                 /* 用于计数 */
    int  num;               /* 网络接口的数目 */
    int  sock_fd = -1;      /* 网络文件描述符 */
    char buff[BUFSIZ];      /* 用来存放所有接口信息的缓冲区 */
    struct ifconf  conf;    /* 网络接口配置结构体 */
    struct ifreq  * ifr;    /* 用来配置ip地址，激活接口，配置MTU等接口信息的结构体指针 */
    static char ipc_ip[16]; /* 存储ip地址的静态数组 */

    /* 数组清零 */
    memset(ipc_ip, 0x00, 16);

    /* 获取网络文件描述符 */
    sock_fd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0)
    {
        perror("socket err\n");
        return NULL;
    }
    conf.ifc_len = BUFSIZ; /* 表示用来存放所有接口信息的缓冲区长度 */
    conf.ifc_buf = buff;   /* 表示存放接口信息的缓冲区 */

    /* 获取所有网络接口信息 */
    ioctl(sock_fd, SIOCGIFCONF, &conf);

    num = conf.ifc_len / sizeof(struct ifreq); /* 网络接口的数目 */
    ifr = conf.ifc_req;                        /* 用来保存某个接口的信息 */

    for (i = 0; i < num; i++)
    {
        struct sockaddr_in * sin = (struct sockaddr_in *)(&ifr->ifr_addr);

        ioctl(sock_fd, SIOCGIFFLAGS, ifr); /* 获取接口标识 */

        /* 判断是否是本地的非loopback的IP地址 */
        if (((ifr->ifr_flags & IFF_LOOPBACK) == 0) && (ifr->ifr_flags & IFF_UP))
        {
            /* 保存ip地址 */
            strcpy(ipc_ip, inet_ntoa(sin->sin_addr));
            close(sock_fd);
            return ipc_ip;
        }
        ifr++;
    }

    return NULL;
#endif
}


