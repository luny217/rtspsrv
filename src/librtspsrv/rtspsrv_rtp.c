/*
 * FileName:
 * Author:         luny  Version: 1.0  Date: 2017-5-20
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
#include <libavutil/avutil_log.h>
#include "rtspsrv_defines.h"
#include "rtspsrv_session.h"
#include "rtspsrv_utils.h"
#include "rtspsrv_rtp.h"

#define DEFAULT_UDP_SERVER_PORT 60000
#define MAX_UDP_SERVER_PORT     65535

static int g_start_port = DEFAULT_UDP_SERVER_PORT;
static int g_port_count = 0;

////////////////////////////////////////////////////////////////////////////////
// 函数名：get_udp_server_port
// 描  述：获取服务器端的UDP端口
// 参  数：无
// 返回值：成功返回获取的UDP端口
// 说  明：
////////////////////////////////////////////////////////////////////////////////
int get_udp_server_port(void)
{

    int port = g_start_port + g_port_count;

    //判断是否达到最大值
    if (port >= MAX_UDP_SERVER_PORT)
    {
        g_port_count = 0;
    }

    g_port_count += 2;

    return port;
}
