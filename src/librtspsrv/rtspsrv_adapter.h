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
 * 函数介绍:	获取VPS
 * 输入参数:	ch_idx: 通道号;
 *					stm_type: 流类型;
 *					vps_buf: buf缓存地址;
 * 输出参数:	无
 * 返回值:		成功返回sps的长度
 *****************************************************************************/
int rtspsrv_adpt_get_vps(int ch_idx, int stm_type, char * vps_buf, int * vps_len);

/******************************************************************************
* 函数介绍:	获取SPS
* 输入参数:	ch_idx: 通道号;
*					stm_type: 流类型;
*					sps_buf: buf缓存地址;
* 输出参数:	无
* 返回值:		成功返回sps的长度
*****************************************************************************/
int rtspsrv_adpt_get_sps(int ch_idx, int stm_type, char * sps_buf, int * sps_len);

/******************************************************************************
* 函数介绍:	获取PPS
* 输入参数:	ch_idx: 通道号;
*					stm_type: 流类型;
*					pps_buf: buf缓存地址;
* 输出参数:	无
* 返回值:		成功返回sps的长度
*****************************************************************************/
int rtspsrv_adpt_get_pps(int ch_idx, int stm_type, char * pps_buf, int * pps_len);

/******************************************************************************
* 函数介绍:	获取本机IP地址
* 输入参数:	ch_idx: 通道号;
*					stm_type: 流类型;
*					pps_buf: buf缓存地址;
* 输出参数:	无
* 返回值:		成功返回sps的长度
*****************************************************************************/
char * rtspsrv_adpt_get_ip(void);

#endif


