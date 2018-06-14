/*
 * TCP protocol
 * Copyright (c) 2002 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include "avformat.h"
#include "libavutil/avutil_parseutils.h"
#include "libavutil/avutil_opt.h"
#include "libavutil/avutil_time.h"
#include "avformat_internal.h"
#include "avformat_network.h"
#include "avformat_os_support.h"
#include "avformat_url.h"
/* #include "fifo.h" */
#if HAVE_POLL_H
#include <poll.h>
#endif

#define OFFSET(x) offsetof(TCPContext, x)
#define D AV_OPT_FLAG_DECODING_PARAM
#define E AV_OPT_FLAG_ENCODING_PARAM
static const AVOption options[] =
{
    {"timeout", "set timeout of socket I/O operations", OFFSET(rw_timeout), AV_OPT_TYPE_INT, {.i64 = -1}, -1, INT_MAX, D | E },
    {NULL}
};

const AVClass tcp_context_class =
{
    .class_name = "tcp",
    .item_name  = av_default_item_name,
    .option     = options,
    .version    = LIBAVUTIL_VERSION_INT,
};

int tcp_open(URLContext * h, const char * uri, int flags);

/* return non zero if error */
int tcp_open(URLContext * h, const char * uri, int flags)
{
    struct addrinfo hints = { 0 }, *ai, *cur_ai;
    int port, fd = -1;
    TCPContext * s = h->priv_data;
    const char * p;
    char buf[256];
    int ret;
    char hostname[1024], proto[1024], path[1024];
    char portstr[10];
    s->open_timeout = 60 * 1000 * 100;

    av_url_split(proto, sizeof(proto), NULL, 0, hostname, sizeof(hostname),
                 &port, path, sizeof(path), uri);
    if (strcmp(proto, "tcp"))
        return AVERROR(EINVAL);
    if (port <= 0 || port >= 65536)
    {
        av_log(h, AV_LOG_ERROR, "Port missing in uri\n");
        return AVERROR(EINVAL);
    }
    p = strchr(uri, '?');
    if (p)
    {
        if (av_find_info_tag(buf, sizeof(buf), "timeout", p))
        {
            s->rw_timeout = strtol(buf, NULL, 10);
        }
    }
    if (s->rw_timeout > 0)
    {
        h->rw_timeout   = s->rw_timeout;
    }
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    snprintf(portstr, sizeof(portstr), "%d", port);
    if (s->listen)
        hints.ai_flags |= AI_PASSIVE;
    if (!hostname[0])
        ret = getaddrinfo(NULL, portstr, &hints, &ai);
    else
        ret = getaddrinfo(hostname, portstr, &hints, &ai);
    if (ret)
    {
        av_log(h, AV_LOG_ERROR,
               "Failed to resolve hostname %s: %s\n",
               hostname, gai_strerror(ret));
        return AVERROR(EIO);
    }

    cur_ai = ai;

restart:
    fd = ff_socket(cur_ai->ai_family,
                   cur_ai->ai_socktype,
                   cur_ai->ai_protocol);
    if (fd < 0)
    {
        ret = ff_neterrno();
        goto fail;
    }


    if ((ret = ff_listen_connect(fd, cur_ai->ai_addr, cur_ai->ai_addrlen,
                                 s->open_timeout / 1000, h, !!cur_ai->ai_next)) < 0)
    {

        if (ret == AVERROR_EXIT)
            goto fail1;
        else
            goto fail;
    }

    h->is_streamed = 1;
    s->fd = fd;
    freeaddrinfo(ai);
    return 0;

fail:
    if (cur_ai->ai_next)
    {
        /* Retry with the next sockaddr */
        cur_ai = cur_ai->ai_next;
        if (fd >= 0)
            closesocket(fd);
        ret = 0;
        goto restart;
    }
fail1:
    if (fd >= 0)
        closesocket(fd);
    freeaddrinfo(ai);
    return ret;
}


int tcp_read(URLContext * h, uint8_t * buf, int size);
int tcp_read(URLContext * h, uint8_t * buf, int size)
{
    TCPContext * s = h->priv_data;
    int ret;

    if (!(h->flags & AVIO_FLAG_NONBLOCK))
    {
        ret = ff_network_wait_fd_timeout(s->fd, 0, h->rw_timeout, &h->interrupt_callback);
        if (ret)
        {
            return ret;
        }
    }
    ret = recv(s->fd, buf, size, 0);
    if (ret <= 0)
    {
        /* printf("tcp_recv error = %d\n",ret); */
        /* abort(); */
    }
    /* else printf("url = %s tcp_read = %d\n",s-> ret); */

    return ret < 0 ? ff_neterrno() : ret;
}

int tcp_write(URLContext * h, const uint8_t * buf, int size);
int tcp_write(URLContext * h, const uint8_t * buf, int size)
{
    TCPContext * s = NULL;
    int ret;

    if (!h)
    {
        return AVERROR_EOF;
    }

    s = (TCPContext *)h->priv_data;

    if (s->fd < 0)
    {
        return AVERROR_EOF;
    }

    if (!(h->flags & AVIO_FLAG_NONBLOCK))
    {
        ret = ff_network_wait_fd_timeout(s->fd, 1, h->rw_timeout, &h->interrupt_callback);
        if (ret)
        {
            return ret;
        }
    }
    ret = send(s->fd, buf, size, 0);
    if (ret < 0)
    {
        /* printf("tcp_send error = %d\n", ret); */
    }
    return ret < 0 ? ff_neterrno() : ret;
}

/* int tcp_shutdown(URLContext * h, int flags); */
static int tcp_shutdown(URLContext * h, int flags)
{
    TCPContext * s = h->priv_data;
    int how;

    if (flags & AVIO_FLAG_WRITE && flags & AVIO_FLAG_READ)
    {
        how = SHUT_RDWR;
    }
    else if (flags & AVIO_FLAG_WRITE)
    {
        how = SHUT_WR;
    }
    else
    {
        how = SHUT_RD;
    }

    return shutdown(s->fd, how);
}

int tcp_close(URLContext * h);
int tcp_close(URLContext * h)
{
    TCPContext * s = h->priv_data;
    closesocket(s->fd);
    return 0;
}

int tcp_get_file_handle(URLContext * h);
int tcp_get_file_handle(URLContext * h)
{
    TCPContext * s = h->priv_data;
    return s->fd;
}

URLProtocol ff_tcp_protocol =
{
    .name                = "tcp",
    .url_open            = tcp_open,
    .url_read            = tcp_read,
    .url_write           = tcp_write,
    .url_close           = tcp_close,
    .url_get_file_handle = tcp_get_file_handle,
    .url_shutdown        = tcp_shutdown,
    .priv_data_size      = sizeof(TCPContext),
    .priv_data_class     = &tcp_context_class,
    .flags               = URL_PROTOCOL_FLAG_NETWORK,
};
