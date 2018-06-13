/*
* FileName:       rtspsrv_utils.h
* Author:         luny  Version: 1.0  Date: 2017-5-20
* Description:
* Version:
* Function List:
*                 1.
* History:
*     <author>   <time>    <version >   <desc>
*/

#ifndef _RTSPSRV_UTILS_H
#define _RTSPSRV_UTILS_H

#include <time.h>
#include "defines.h"

#define SPS_PPS_SIZE 128

typedef struct _rsrv_cli_info
{
    sint32 fd;
    sint32 used;
    
    sint32 error_code;
    sint32 state;
    sint32 fps;
    sint32 enc_type;
    sint32 bitrate;
    sint32 stm_type;
    sint32 fifo;
    sint32 consumer;

    sint32 max_vframe;  /* 最大视频帧 */
    sint32 max_aframe;  /* 最大音频帧 */
    sint32 rcvq_buf;    /* netstat revq大小 */
    sint32 sndq_buf;    /* netstat sendq大小 */

    sint32 timeout_cnt;
    sint32 try_snd_len;
    sint32 low_water_cnt;   /* 没有数据计数 */
    sint32 high_water_cnt;  /* 数据溢出计数 */

    sint8 rem_ip_str[16];
    uint32 remote_port;

    sint64 max_snd_time;
    sint64 open_tm;
    sint64 close_tm;
    sint64 frame_count;
    sint64 iframe_count;
    sint64 vframe_count;
    sint64 aframe_count;
    sint64 vdrop_count;
    sint64 recv_bytes; /* 最近网络发送数据之和,回调帧数据后清零 */

} rsrv_cli_info_t;

void rsrv_hex_dump(const uint8_t * buf, int size);


////////////////////////////////////////////////////////////////////////////////
// 函数名：com_rand
// 描  述：获取整形的随机数
// 参  数：无
// 返回值：成功返回获取的随机数，出错返回0
// 说  明：  
////////////////////////////////////////////////////////////////////////////////
int com_rand(void);

////////////////////////////////////////////////////////////////////////////////
// 函数名：com_random
// 描  述：获取长整形的随机数
// 参  数：无
// 返回值：成功返回获取的随机数，出错返回0
// 说  明：  
////////////////////////////////////////////////////////////////////////////////
long int com_random(void);

////////////////////////////////////////////////////////////////////////////////
// 函数名：com_random32
// 描  述：获取无符号整形的随机数
// 参  数：无
// 返回值：成功返回获取的随机数，出错返回0
// 说  明：  
////////////////////////////////////////////////////////////////////////////////
unsigned int com_random32(void);

/******************************************************************************
* 函数介绍: 获取UTC时间
* 输入参数: t  获取的UTC时间的指针
*           b  时间字符串
* 输出参数: 无
* 返回值  : 成功返回0，出错返回-1
*****************************************************************************/
int rtspsrv_get_UTC_time(struct tm * t, char * b);

/******************************************************************************
* 函数介绍:	设置网络文件描述符为非阻塞模式
* 输入参数:	sock: 网络文件描述符;
* 输出参数:	无
* 返回值:   成功返回>0
*****************************************************************************/
int rtspsrv_set_non_blocking(sint32 sockfd);

/******************************************************************************
* 函数介绍:	在TCP协议中设置KEEPALIVE检测，适用于在UDP传输RTP的情况下检测网络断开
* 输入参数:	sockfd: 网络文件描述符;
* 输出参数:	无
* 返回值:   成功返回0，出错返回-1
*****************************************************************************/
int rtspsrv_set_keep_alive(sint32 sockfd);

/******************************************************************************
* 函数介绍:	设置网络文件描述符是否立即关闭属性
* 输入参数:	sockfd: 网络文件描述符;
* 输出参数:	无
* 返回值:   成功返回0，出错返回-1
*****************************************************************************/
int rtspsrv_set_linger(sint32 sockfd);

/******************************************************************************
* 函数介绍:	获取vps的base64编码
* 输入参数:	vps_buf: buf缓存地址;
* 输出参数:	vps_base64_buf: 编码base64后的buf
* 返回值:   成功返回vps base64编码的长度
*****************************************************************************/
int rsrv_get_vps_base64(uint8 * vps_buf, sint32 vps_len, uint8 * vps_base64_buf);

/******************************************************************************
* 函数介绍:	获取sps的base64编码
* 输入参数:	sps_buf: buf缓存地址;
* 输出参数:	sps_base64_buf: 编码base64后的buf
* 返回值:   成功返回sps base64编码的长度
*****************************************************************************/
int rsrv_get_sps_base64(uint8 * sps_buf, sint32 sps_len, uint8 * sps_base64_buf);

/******************************************************************************
* 函数介绍:	获取pps的base64编码
* 输入参数: pps_buf: buf缓存地址;
* 输出参数:	pps_base64_buf: 编码base64后的buf
* 返回值:   成功返回vps base64编码的长度
*****************************************************************************/
int rsrv_get_pps_base64(uint8 * pps_buf, sint32 pps_len, uint8 * pps_base64_buf);

/******************************************************************************
* 函数介绍:	设置报式套接字
* 输入参数:	port: 端口号;
*					make_non_blocking: 是否是非阻塞模式的标识;
* 输出参数:	无
* 返回值:   成功返回>0
*****************************************************************************/
int rtspsrv_setup_udp(uint16 port, sint32 make_non_blocking);

////////////////////////////////////////////////////////////////////////////////
// 函数名：setup_stream_socket
// 描  述：设置流式套接字
// 参  数：[in] port              端口号
//         [in] make_non_blocking 是否是非阻塞模式的标识
// 返回值：成功返回网络文件描述符，出错返回ERR_GENERIC
// 说  明：  
////////////////////////////////////////////////////////////////////////////////
int rtspsrv_setup_tcp(sint16 port, int make_non_blocking);

/******************************************************************************
* 函数介绍:	查找nal单元
* 输入参数:	buf: 帧缓存;
*           size: 帧大小;
*           enc_type: 编码类型;
* 输出参数:	nal_start: 找到的nal起始地址;
*           nal_end: 找到的nal结束地址;
* 返回值:   成功返回>0
*****************************************************************************/
int rsrv_find_nal_unit(uint8 * buf, int size, int * nal_start, int * nal_end, int enc_type);

/******************************************************************************
* 函数介绍:	判断是否为结束的nal单元，是I帧或者P帧即可
* 输入参数:	enc_type: 编码类型;
*           nal_ptr: nal字节缓存;
* 输出参数:	无
* 返回值:   结束为1，否则为0，<0为错误
*****************************************************************************/
int rsrv_check_end_of_frame(int enc_type, char nal_byte, int * _nal_type);

/******************************************************************************
* 函数介绍:	获取ntp时间
* 输入参数:	无
* 输出参数:	无
* 返回值:   ntp时间
*****************************************************************************/
uint64 rsrv_ntp_time(void);

int tcp_info(int sockfd, int * rbuf_num, int * sbuf_num);

int rss_init_cli(void);

int rss_take_unused_cli(void);

rsrv_cli_info_t * rss_get_cli(int fd);

int rss_clear_cli(void);

int rss_stat_device(char * line);

#endif

