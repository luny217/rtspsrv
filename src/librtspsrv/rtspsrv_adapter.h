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
#ifndef _RTSPSRV_ADPATER_H
#define _RTSPSRV_ADPATER_H

#include <defines.h>

 /******************************************************************************
 * ��������:	��ȡVPS
 * �������:	ch_idx: ͨ����;
 *					stm_type: ������;
 *					vps_buf: buf�����ַ;
 * �������:	��
 * ����ֵ:		�ɹ�����sps�ĳ���
 *****************************************************************************/
int rtspsrv_adpt_get_vps(int ch_idx, int stm_type, char * vps_buf, int * vps_len);

/******************************************************************************
* ��������:	��ȡSPS
* �������:	ch_idx: ͨ����;
*					stm_type: ������;
*					sps_buf: buf�����ַ;
* �������:	��
* ����ֵ:		�ɹ�����sps�ĳ���
*****************************************************************************/
int rtspsrv_adpt_get_sps(int ch_idx, int stm_type, char * sps_buf, int * sps_len);

/******************************************************************************
* ��������:	��ȡPPS
* �������:	ch_idx: ͨ����;
*					stm_type: ������;
*					pps_buf: buf�����ַ;
* �������:	��
* ����ֵ:		�ɹ�����sps�ĳ���
*****************************************************************************/
int rtspsrv_adpt_get_pps(int ch_idx, int stm_type, char * pps_buf, int * pps_len);

/******************************************************************************
* ��������:	��ȡ����IP��ַ
* �������:	ch_idx: ͨ����;
*					stm_type: ������;
*					pps_buf: buf�����ַ;
* �������:	��
* ����ֵ:		�ɹ�����sps�ĳ���
*****************************************************************************/
char * rtspsrv_adpt_get_ip(void);

#endif


