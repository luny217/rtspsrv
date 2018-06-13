/*
 * FileName:       
 * Author:         luny  Version: 1.0  Date: 2017-5-17
 * Description:    
 * Version:        
 * Function List:  
 *                 1.
 * History:        
 *     <author>   <time>    <version >   <desc>
 */

#ifndef _RTSP_HANDLER_H
#define _RTSP_HANDLER_H

#include "defines.h"
#include "rtspsrv_session.h"

///////////////////////////////////////////////////////////////////////////////////////////
// 播放参数结构体，暂时不使用其中的参数
///////////////////////////////////////////////////////////////////////////////////////////
typedef struct _play_args
{
	struct tm playback_time;   //播放时间
	sint32 playback_time_valid; //播放时间是否有效的标识	
	sint32 start_time_valid;    //开始时间是否有效的标识
	float start_time;          //开始时间
	float end_time;            //结束时间
} play_args_t;

/******************************************************************************
* 函数介绍: 处理新进socket连接请求
* 输入参数: rs_srv: RTSP server 管理句柄
*				  ch_idx: 通道号
* 输出参数: 无
* 返回值  : >0-写入数据大小, <0-错误代码
*****************************************************************************/
int rsrv_do_accept(rtsp_srv_t * rs_srv, int ch_idx);

/******************************************************************************
* 函数介绍: RTSP 处理请求
* 输入参数: rs_srv: RTSP server 管理句柄
*				  rss: RTSP session 实例
*				  ch_idx: 通道号
* 输出参数: 无
* 返回值  : >0-写入数据大小, <0-错误代码
*****************************************************************************/
int rsrv_do_request(rtsp_srv_t * rs_srv, rtsp_sessn_t * rss, int ch_idx);

/******************************************************************************
* 函数介绍: RTSP reply
* 输入参数: rss: RTSP session 实例
*				  ecode: RTSP错误码
* 输出参数: 无
* 返回值  : >0-写入数据大小, <0-错误代码
*****************************************************************************/
int rsrv_do_reply(rtsp_sessn_t * rss, int ecode);

/******************************************************************************
* 函数介绍: RTSP reply options
* 输入参数: rss: RTSP session 实例
* 输出参数: 无
* 返回值  : >0-写入数据大小, <0-错误代码
*****************************************************************************/
int rsrv_reply_options(rtsp_sessn_t * rss);

#endif


