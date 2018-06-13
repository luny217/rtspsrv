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
* ��������:	��ȡ��������
* �������:	ch_idx: ͨ����;
*           stm_type: ������;
* �������:	��
* ����ֵ:   �ɹ�����sps�ĳ���
*****************************************************************************/
int rtspsrv_adpt_get_enc_type(int ch_idx, int stm_type)
{
	int enc_type = VIDEO_H265;

	return enc_type;
}

/******************************************************************************
* ��������:	��ȡVPS
* �������:	ch_idx: ͨ����;
*           stm_type: ������;
*           vps_buf: buf�����ַ;
* �������:	��
* ����ֵ:   �ɹ�����vps�ĳ���
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
* ��������:	��ȡSPS
* �������:	ch_idx: ͨ����;
*					stm_type: ������;
*					sps_buf: buf�����ַ;
* �������:	��
* ����ֵ:		�ɹ�����sps�ĳ���
*****************************************************************************/
int rtspsrv_adpt_get_sps(int ch_idx, int stm_type, char * sps_buf, int * sps_len)
{
    char sps[] = {0x4D, 0x00, 0x1F, 0x95, 0xA8, 0x14, 0x01, 0x6E, 0x9B, 0x80, 0x80, 0x80, 0x81};
    memcpy(sps_buf, sps, sizeof(sps));
    *sps_len = sizeof(sps);
    return 0;
}

/******************************************************************************
* ��������:	��ȡPPS
* �������:	ch_idx: ͨ����;
*					stm_type: ������;
*					pps_buf: buf�����ַ;
* �������:	��
* ����ֵ:		�ɹ�����sps�ĳ���
*****************************************************************************/
int rtspsrv_adpt_get_pps(int ch_idx, int stm_type, char * pps_buf, int * pps_len)
{
    char pps[] = {0xEE, 0x3C, 0x80};
    memcpy(pps_buf, pps, sizeof(pps));
    *pps_len = sizeof(pps);
    return 0;
}

/******************************************************************************
* ��������:	��ȡ����IP��ַ
* �������:	ch_idx: ͨ����;
*					stm_type: ������;
*					pps_buf: buf�����ַ;
* �������:	��
* ����ֵ:		�ɹ�����sps�ĳ���
*****************************************************************************/
char * rtspsrv_adpt_get_ip(void)
{
#if 1 /*def _WIN32*/
    static char ipaddr[] = { "10.0.0.50" };
    return ipaddr;
#else
    int  i;                 /* ���ڼ��� */
    int  num;               /* ����ӿڵ���Ŀ */
    int  sock_fd = -1;      /* �����ļ������� */
    char buff[BUFSIZ];      /* ����������нӿ���Ϣ�Ļ����� */
    struct ifconf  conf;    /* ����ӿ����ýṹ�� */
    struct ifreq  * ifr;    /* ��������ip��ַ������ӿڣ�����MTU�Ƚӿ���Ϣ�Ľṹ��ָ�� */
    static char ipc_ip[16]; /* �洢ip��ַ�ľ�̬���� */

    /* �������� */
    memset(ipc_ip, 0x00, 16);

    /* ��ȡ�����ļ������� */
    sock_fd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0)
    {
        perror("socket err\n");
        return NULL;
    }
    conf.ifc_len = BUFSIZ; /* ��ʾ����������нӿ���Ϣ�Ļ��������� */
    conf.ifc_buf = buff;   /* ��ʾ��Žӿ���Ϣ�Ļ����� */

    /* ��ȡ��������ӿ���Ϣ */
    ioctl(sock_fd, SIOCGIFCONF, &conf);

    num = conf.ifc_len / sizeof(struct ifreq); /* ����ӿڵ���Ŀ */
    ifr = conf.ifc_req;                        /* ��������ĳ���ӿڵ���Ϣ */

    for (i = 0; i < num; i++)
    {
        struct sockaddr_in * sin = (struct sockaddr_in *)(&ifr->ifr_addr);

        ioctl(sock_fd, SIOCGIFFLAGS, ifr); /* ��ȡ�ӿڱ�ʶ */

        /* �ж��Ƿ��Ǳ��صķ�loopback��IP��ַ */
        if (((ifr->ifr_flags & IFF_LOOPBACK) == 0) && (ifr->ifr_flags & IFF_UP))
        {
            /* ����ip��ַ */
            strcpy(ipc_ip, inet_ntoa(sin->sin_addr));
            close(sock_fd);
            return ipc_ip;
        }
        ifr++;
    }

    return NULL;
#endif
}


