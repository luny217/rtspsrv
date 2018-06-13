#ifndef _RTSPSRV_H	
#define _RTSPSRV_H

#include <defines.h>

/* RTSP fifo句柄 */
typedef struct _rsrv_fifo
{
    sint32 pri_handle; /* 主码流 fifo 句柄 */
    sint32 sec_handle; /* 子码流 fifo 句柄 */
    sint32 thr_handle; /* 三码流 fifo 句柄 */
} rsrv_fifo_t;

/******************************************************************************
 * 函数介绍: rtspsrv创建
 * 输入参数: chn_num: 支持的最大通道数量
 *           wrk_num: 创建的工作线程数量
 *           port: 监听端口
 *           fifos: 主子次码流的fifo句柄, 主动模式下可以为NULL
 *           
 * 输出参数: 无
 * 返回值  : >0-写入数据大小, <0-错误代码
 *****************************************************************************/
int rtspsrv_open(sint32 chn_num, sint32 wrk_num, sint16 port, rsrv_fifo_t * fifos);

/******************************************************************************
* 函数介绍: rtspsrv启动工作线程
* 输入参数: 无
* 输出参数: 无
* 返回值  : >0-写入数据大小, <0-错误代码
*****************************************************************************/
int rtspsrv_start(void);

/******************************************************************************
* 函数介绍: rtspsrv停止工作线程
* 输入参数: 无
* 输出参数: 无
* 返回值  : >0-写入数据大小, <0-错误代码
*****************************************************************************/
int rtspsrv_stop(void);

/******************************************************************************
* 函数介绍: rtspsrv关闭服务器
* 输入参数: 无
* 输出参数: 无
* 返回值  : >0-写入数据大小,<0-错误代码
*****************************************************************************/
int rtspsrv_close(void);

#endif
	
