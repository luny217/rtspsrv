/*
 * FileName:      rtsp_epoll.c
 * Author:         luny  Version: 1.0  Date: 2017-5-17
 * Description:    
 * Version:        
 * Function List:  
 *                 1.
 * History:        
 *     <author>   <time>    <version >   <desc>
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>
#include <sys/epoll.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include "ffconfig.h"
#include "rtspsrv_defines.h"
#include "rtspsrv_epoll.h"

#ifdef _RTSPSRV_TEST
#include "avutil_log.h"
#else
#include "common.h"
#endif

int rsrv_ep_creat(int ev_max)
{
	int ep_fd = 0;

    ep_fd = epoll_create(ev_max);
    if (ep_fd < 0)
    {
        perror("epoll_create");
        return -1;
    }
    return ep_fd;
}

void rsrv_ep_close(int ep_fd)
{
    closesocket(ep_fd);
}

/*
** 函数名：rtspsrv_ep_add
** 描述：  添加网络文件描述符到epoll文件描述符中
** 参数：
**    [in]  fd 要添加的网络文件描述符
**	  [out] 无
** 返回值：成功返回0，失败返回-1
** 说明：
*/
int rsrv_ep_add_for_read(int ep_fd, int socket_fd, void * data_ptr)
{
    struct epoll_event ev;

    memset(&ev, 0x00, sizeof(struct epoll_event));
    ev.events = EPOLLIN;
    ev.data.ptr = data_ptr;

    /*av_log(NULL, AV_LOG_WARNING, "epoll_add read fd: %d socket: %d data_ptr: %p\n", ep_fd, socket_fd, ev.data.ptr);*/

    if (epoll_ctl(ep_fd, EPOLL_CTL_ADD, socket_fd, &ev) == -1)
    {
        rsrv_log(AV_LOG_ERROR, "EPOLL_CTL_ADD: %d\n", errno);
        return -1;
    }

    return 0;
}

/*
** 函数名：rtspsrv_ep_add
** 描述：  添加网络文件描述符到epoll文件描述符中
** 参数：
**    [in]  fd 要添加的网络文件描述符
**	  [out] 无
** 返回值：成功返回0，失败返回-1
** 说明：
*/
int rsrv_ep_add_for_rw(int ep_fd, int socket_fd, void * data_ptr)
{
    struct epoll_event ev;

    memset(&ev, 0x00, sizeof(struct epoll_event));
    ev.events = EPOLLIN | EPOLLOUT;
    ev.data.ptr = data_ptr;

    /*av_log(NULL, AV_LOG_WARNING, "epoll_add rw fd = 0x%x, socket = %d\n", ep_fd, socket_fd);*/

    if (epoll_ctl(ep_fd, EPOLL_CTL_MOD, socket_fd, &ev) == -1)
    {
        rsrv_log(AV_LOG_ERROR, "EPOLL_CTL_ADD: %d\n", errno);
        return -1;
    }

    return 0;
}


/*
** 函数名：rtspsrv_ep_del
** 描述：  删除网络文件描述符到epoll文件描述符中
** 参数：
**    [in]  fd 要添加的网络文件描述符
**	  [out] 无
** 返回值：成功返回0，失败返回-1
** 说明：
*/

int rsrv_ep_del(int ep_fd, int socket_fd, void * data_ptr)
{
    struct epoll_event ev;

    memset(&ev, 0x00, sizeof(struct epoll_event));
    ev.events = EPOLLIN | EPOLLOUT;
    ev.data.ptr = (void *)data_ptr;

    if (epoll_ctl(ep_fd, EPOLL_CTL_DEL, socket_fd, &ev) == -1)
    {
        rsrv_log(AV_LOG_ERROR, "EPOLL_CTL_DEL\n");
        return -1;
    }

    return 0;
}

int rsrv_ep_mod(int ep_fd, int socket_fd, void * data_ptr)
{
    struct epoll_event ev;

    memset(&ev, 0x00, sizeof(struct epoll_event));
    ev.events = EPOLLIN | EPOLLOUT;
    ev.data.ptr = (void *)data_ptr;

    rsrv_log(AV_LOG_WARNING, "epoll_mod fd = %d, socket = %d\n", ep_fd, socket_fd);

    if (epoll_ctl(ep_fd, EPOLL_CTL_MOD, socket_fd, &ev) == -1)
    {
        rsrv_log(AV_LOG_ERROR, "EPOLL_CTL_MOD failed:%d\n", errno);
        return -1;
    }

    return 0;
}
