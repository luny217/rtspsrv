/*
 * FileName:       rtspsrv_utils.c
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
#ifndef _WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#endif
#include <libavcodec/avcodec_hevc.h>
#include <libavutil/avutil_random_seed.h>
#include <libavutil/avutil_log.h>
#include <libavformat/avformat_network.h>
#include <libavutil/avutil_time.h>
#include <sstring.h>
#include "rtspsrv_defines.h"
#include "rtspsrv_utils.h"
#include "rtspsrv_adapter.h"

#ifndef _RTSPSRV_TEST
#include "mem_manage.h"
#include "cli_stat.h"
#endif


#define LISTEN_BACKLOG_SIZE (100)
#define _PATH_PROCNET_TCP "/proc/net/tcp"
static int rsrv_info_idx = 0;

static rsrv_cli_info_t * _rtrv_cli_info = NULL;

/*#define HEXDUMP_PRINT(...)                      \
    do {                                        \
            printf(__VA_ARGS__); \
    } while (0)

void rsrv_hex_dump(const uint8_t * buf, int size)
{
    int len, i, j;

    for (i = 0; i < size; i += 16)
    {
        len = size - i;
        if (len > 16)
            len = 16;
        HEXDUMP_PRINT("%08x ", i);
        for (j = 0; j < 16; j++)
        {
            if (j < len)
                HEXDUMP_PRINT(" %02x", buf[i + j]);
            else
                HEXDUMP_PRINT("   ");
        }
        HEXDUMP_PRINT(" ");
        HEXDUMP_PRINT("\n");
    }
}*/


/******************************************************************************
* 函数介绍: packbits算法编码函数
* 输入参数: src: 需要编码的字符串;
*				  dst: 编码后的字符串;
*				  n:	需要编码字符串的长度；
* 输出参数: 无
* 返回值  :  成功返回dst的长度
*****************************************************************************/
uint32 packbits(uint8 * src, uint8 * dst, uint32 n)
{
    uint8 * p, *q, *run, *dataend;
    int count, maxrun;

    dataend = src + n;
    for (run = src, q = dst; n > 0; run = p, n -= count)
    {
        /* A run cannot be longer than 128 bytes.*/
        maxrun = n < 128 ? n : 128;
        if (run <= (dataend - 3) && run[1] == run[0] && run[2] == run[0])
        {
            /* 'run' points to at least three duplicated values.*/
            /* Step forward until run length limit, end of input,*/
            /* or a non matching byte:*/
            for (p = run + 3; p < (run + maxrun) && *p == run[0];)
            {
                ++p;
            }

            count = p - run;

            /* replace this run in output with two bytes:*/
            *q++ = 1 + 256 - count; /* flag byte, which encodes count (129..254) */

            *q++ = run[0];      /* byte value that is duplicated */

        }
        else
        {
            /* If the input doesn't begin with at least 3 duplicated values,*/
            /* then copy the input block, up to the run length limit,*/
            /* end of input, or until we see three duplicated values:*/
            for (p = run; p < (run + maxrun);)
            {
                if (p <= (dataend - 3) && p[1] == p[0] && p[2] == p[0])
                {
                    break; /* 3 bytes repeated end verbatim run*/
                }
                else
                {
                    ++p;
                }
            }

            count = p - run;
            *q++ = count - 1;      /* flag byte, which encodes count (0..127) */
            memcpy(q, run, count); /* followed by the bytes in the run */
            q += count;
        }
    }

    return q - dst;
}

/******************************************************************************
* 函数介绍: 进行base64编码操作
* 输入参数: lpString - 需要进行编码的字符串指针;
*				  lpBuffer - 编码后的数据存放的指针;
*				  sLen - 需要进行编码的数据长度;
* 输出参数: 无
* 返回值  :  成功返回编码后的数据长度, 编码后数据的大小与编码数据长度的比为4：3
*****************************************************************************/
sint32 fn_base64_encode(uint8 * lpString, uint8 * lpBuffer, sint32 sLen)
{
    register int vLen = 0;	/* 寄存器局部变量，提速*/
    uint8 base_code[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    while (sLen > 0)		/* 处理整个字符串*/
    {
        *lpBuffer++ = base_code[(lpString[0] >> 2) & 0x3F];	/*右移两位，与00111111是防止溢出，自加*/

        if (sLen > 2)	/*够3个字符*/
        {
            *lpBuffer++ = base_code[((lpString[0] & 3) << 4) | (lpString[1] >> 4)];
            *lpBuffer++ = base_code[((lpString[1] & 0xF) << 2) | (lpString[2] >> 6)];
            *lpBuffer++ = base_code[lpString[2] & 0x3F];
        }
        else
        {
            switch (sLen)	/*追加“=”*/
            {
                case 1:
                    *lpBuffer ++ = base_code[(lpString[0] & 3) << 4 ];
                    *lpBuffer ++ = '=';
                    *lpBuffer ++ = '=';
                    break;
                case 2:
                    *lpBuffer ++ = base_code[((lpString[0] & 3) << 4) | (lpString[1] >> 4)];
                    *lpBuffer ++ = base_code[((lpString[1] & 0x0F) << 2) | (lpString[2] >> 6)];
                    *lpBuffer ++ = '=';
                    break;
            }
        }

        lpString += 3;
        sLen -= 3;
        vLen += 4;
    }

    *lpBuffer = 0;

    return vLen;
}

#if 0
/* 红块区域加密(packbits + base64)*/
sint32 AreaEncrypt(RTSP_TEXT_DATA_OBJ * _ptTextData, HB_CHAR * _pcBuff, HB_S32 * _piLen)
{
    RTSP_TEXT_DATA_OBJ * ptText = _ptTextData;
    HB_S32 packbits_len = 0;
    unsigned char rever[50] = {0};
    unsigned char tmp[50] = {0};
    HB_S32 i = 0;
    HB_S32 j = 0;

    i = 0;
    j = 0;

    while ((i < 50) && (j < 18))
    {
        tmp[i]   = ptText->motion_block[j] & 0xff;
        tmp[++i] = (ptText->motion_block[j] >> 8) & 0xff;
        tmp[++i] = ((ptText->motion_block[j] >> 16) & 0x3f) | ((ptText->motion_block[j + 1] << 6) & 0xc0);
        j += 1;
        tmp[++i] = (ptText->motion_block[j] >> 2) & 0xff;
        tmp[++i] = (ptText->motion_block[j] >> 10) & 0xff;

        if ((i == 48) && (j == 17))
        {
            tmp[++i] = ((ptText->motion_block[j] >> 18) & 0x0f);
            break;
        }

        tmp[++i] = ((ptText->motion_block[j] >> 18) & 0x0f) | ((ptText->motion_block[j + 1] << 4) & 0xf0);
        j += 1;
        tmp[++i] = (ptText->motion_block[j] >> 4) & 0xff;
        tmp[++i] = (ptText->motion_block[j] >> 12) & 0xff;
        tmp[++i] = ((ptText->motion_block[j] >> 20) & 0x03) | ((ptText->motion_block[j + 1] << 2) & 0xfc);
        j += 1;
        tmp[++i] = (ptText->motion_block[j] >> 6) & 0xff;
        tmp[++i] = (ptText->motion_block[j] >> 14) & 0xff;

        ++i;
        ++j;
    }

    /* rever存放字符比特位反转后的数据*/
    /* 第50个字符只用到了高4位，低四位始终为0，反转完后只用低4位，高4位无用*/
    for (i = 0; i < 50 ; i++)
    {
        rever[i] = strrev(tmp[i]);
    }

    memset(tmp, 0, 50);
    packbits_len = packbits(rever, tmp, 50);

    *_piLen = fn_base64_encode(tmp, _pcBuff, packbits_len);

    return HB_SUCCESS;
}
#endif

int com_rand(void)
{
#if 0
    struct timeval tv;

    gettimeofday(&tv, 0);
    srand((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
#endif
    return rand();
}

/******************************************************************************
* 函数介绍: 获取无符号整形的随机数
* 输入参数: 无
* 输出参数: 无
* 返回值  : 无符号整形的随机数, 出错返回0
*****************************************************************************/
uint32 com_random32(void)
{
    /* Return a 32-bit random number.
       Because "rtsp_random()" returns a 31-bit random number, we call it a second time,
       to generate the high bit. (Actually, to increase the likelhood of randomness,
       we take the middle 16 bits of two successive calls to "rtsp_random()")
    */
    uint32 random_1 = av_get_random_seed();
    uint32 random16_1 = (uint32)(random_1 & 0x00FFFF00);

    uint32 random_2 = av_get_random_seed();
    uint32 random16_2 = (unsigned int)(random_2 & 0x00FFFF00);

    return (random16_1 << 8) | (random16_2 >> 8);
}

/******************************************************************************
* 函数介绍: 获取UTC时间
* 输入参数: t  获取的UTC时间的指针
*           b  时间字符串
* 输出参数: 无
* 返回值  : 成功返回0，出错返回-1
*****************************************************************************/
int rtspsrv_get_UTC_time(struct tm * t, char * b)
{
    char tmp[5];
    if (sstrnlen(b, 256) < 16)
    {
        return -1;
    }

    /*日期*/
    sstrncpy(tmp, b, 4);
    tmp[4] = '\0';
    sscanf(tmp, "%4d", &(t->tm_year));
    sstrncpy(tmp, b + 4, 2);
    tmp[2] = '\0';
    sscanf(tmp, "%4d", &(t->tm_mon));
    sstrncpy(tmp, b + 6, 2);
    tmp[2] = '\0';
    sscanf(tmp, "%4d", &(t->tm_mday));

    /*时间*/
    sstrncpy(tmp, b + 9, 2);
    tmp[2] = '\0';
    sscanf(tmp, "%4d", &(t->tm_hour));
    sstrncpy(tmp, b + 11, 2);
    tmp[2] = '\0';
    sscanf(tmp, "%4d", &(t->tm_min));
    sstrncpy(tmp, b + 13, 2);
    tmp[2] = '\0';
    sscanf(tmp, "%4d", &(t->tm_sec));

    return 0;
}

#if 0
/******************************************************************************
* 函数介绍:	获取编码类型
* 输入参数:	ch_idx: 通道号;
*           stm_type: 流类型;
* 输出参数:	无
* 返回值:   成功返回编码类型
*****************************************************************************/
int rtspsrv_get_enc_type(int ch_idx, int stm_type)
{
    int enc_type;
    enc_type = rtspsrv_adpt_get_enc_type(ch_idx, stm_type);
    if (enc_type < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "get_enc_type err!\n");
    }
    return enc_type;
}
#endif

/******************************************************************************
* 函数介绍:	获取vps的base64编码
* 输入参数:	vps_buf: buf缓存地址;
* 输出参数:	vps_base64_buf: 编码base64后的buf
*           vps_base64_len: 编码base64后的buf长度
* 返回值:   成功返回vps base64编码的长度
*****************************************************************************/
int rsrv_get_vps_base64(uint8 * vps_buf, sint32 vps_len, uint8 * vps_base64_buf)
{
    uint8 * p = NULL;
    sint32 out_len;

	out_len = fn_base64_encode(vps_buf, vps_base64_buf, vps_len);/* 对vps进行base64编码 */
	if (out_len < 0)
	{
		rsrv_log(AV_LOG_ERROR, "fn_base64_encode vps err!\n");
		return -1;
	}

    /* ssl PEM格式,会以\n换行符结尾, 需去掉该\n */
    p = &vps_base64_buf[out_len - 1];
    if (*p == '\n')
    {
        *p = '\0';
        out_len--;
    }

    return out_len;
}

/******************************************************************************
* 函数介绍:	获取sps的base64编码
* 输入参数:	sps_buf: buf缓存地址;
* 输出参数:	sps_base64_buf: 编码base64后的buf
*           sps_base64_len: 编码base64后的buf长度
* 返回值:   成功返回sps base64编码的长度
*****************************************************************************/
int rsrv_get_sps_base64(uint8 * sps_buf, sint32 sps_len, uint8 * sps_base64_buf)
{
    uint8 * p = NULL;
    sint32 out_len;

	out_len = fn_base64_encode(sps_buf, sps_base64_buf, sps_len);/* 对sps进行base64编码 */
    if (out_len < 0)
    {
        rsrv_log(AV_LOG_ERROR, "fn_base64_encode sps err!\n");
        return -1;
    }

	/* ssl PEM格式,会以\n换行符结尾, 需去掉该\n */
	p = &sps_base64_buf[out_len - 1];
	if (*p == '\n')
	{
		*p = '\0';
		out_len--;
	}

    return out_len;
}

/******************************************************************************
* 函数介绍:	获取pps的base64编码
* 输入参数: pps_buf: buf缓存地址;
* 输出参数:	pps_base64_buf: 编码base64后的buf
*           pps_base64_len: 编码base64后的buf长度
* 返回值:   成功返回vps base64编码的长度
*****************************************************************************/
int rsrv_get_pps_base64(uint8 * pps_buf, sint32 pps_len, uint8 * pps_base64_buf)
{
    uint8 * p = NULL;
    sint32 out_len;

	out_len = fn_base64_encode(pps_buf, pps_base64_buf, pps_len);/* 对pps进行base64编码 */
    if (out_len < 0)
    {
        rsrv_log(AV_LOG_ERROR, "fn_base64_encode pps err!\n");
        return -1;
    }

    /* ssl PEM格式,会以\n换行符结尾, 需去掉该\n */
    p = &pps_base64_buf[out_len - 1];
    if (*p == '\n')
    {
        *p = '\0';
        out_len--;
    }	

    return out_len;
}

int _socket_nonblock(int socket, int enable)
{
#ifdef _WIN32
    u_long param = enable;
    return ioctlsocket(socket, FIONBIO, &param);
#else
    if (enable)
        return fcntl(socket, F_SETFL, fcntl(socket, F_GETFL) | O_NONBLOCK);
    else
        return fcntl(socket, F_SETFL, fcntl(socket, F_GETFL) & ~O_NONBLOCK);
#endif /* HAVE_WINSOCK2_H */
}

/******************************************************************************
* 函数介绍:	设置网络文件描述符为非阻塞模式
* 输入参数:	sock: 网络文件描述符;
* 输出参数:	无
* 返回值:   成功返回>0
*****************************************************************************/
int rtspsrv_set_non_blocking(sint32 sockfd)
{
    return _socket_nonblock(sockfd, 1);
}

/******************************************************************************
* 函数介绍:	设置网络文件描述符为阻塞模式
* 输入参数:	sock: 网络文件描述符;
* 输出参数:	无
* 返回值:   成功返回>0
*****************************************************************************/
int rtspsrv_set_blocking(sint32 sockfd)
{
    return _socket_nonblock(sockfd, 0);
}

/******************************************************************************
* 函数介绍:	在TCP协议中设置KEEPALIVE检测，适用于在UDP传输RTP的情况下检测网络断开
* 输入参数:	sockfd: 网络文件描述符;
* 输出参数:	无
* 返回值:   成功返回0，出错返回-1
*****************************************************************************/
int rtspsrv_set_keep_alive(sint32 sockfd)
{
	return 0;
}

/******************************************************************************
* 函数介绍:	设置网络文件描述符是否立即关闭属性
* 输入参数:	sockfd: 网络文件描述符;
* 输出参数:	无
* 返回值:   成功返回0，出错返回-1
*****************************************************************************/
int rtspsrv_set_linger(sint32 sockfd)
{
    struct linger so_linger; /* 设置是否立即关闭 */
    so_linger.l_onoff = 1; /*立即关闭，不发送缓冲数据*/
    so_linger.l_linger = 0;

    if (setsockopt(sockfd, SOL_SOCKET, SO_LINGER, (void *)&so_linger, sizeof(struct linger)) < 0)
    {
        rsrv_log(AV_LOG_ERROR, "setsockopt() while set SO_LINGER");
        return -1;
    }

    const char chOpt = 0;

    if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &chOpt, sizeof(char)) < 0)
    {
        rsrv_log(AV_LOG_ERROR, "setsockopt() while set TCP_NODELAY");
        return -1;
    }

    return 0;
}

/******************************************************************************
* 函数介绍:	设置报式套接字
* 输入参数:	port: 端口号;
*					make_non_blocking: 是否是非阻塞模式的标识;
* 输出参数:	无
* 返回值:   成功返回>0
*****************************************************************************/
int rtspsrv_setup_udp(uint16 port, sint32 make_non_blocking)
{
    int new_socket = -1;
    int reuse_flag = 1;
    struct sockaddr_in s;

    /* 创建报式网络文件描述符 */
    if ((new_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket() error");
        return -1;
    }

    /* 防止端口被占用 */
    if (setsockopt(new_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse_flag, sizeof(reuse_flag)) < 0)
    {
        closesocket(new_socket);
        perror("setsockopt() error");
        return -1;
    }

    /* 设置网络地址 */
    memset(&s, 0x00, sizeof(struct sockaddr_in));
    s.sin_family = AF_INET;
    s.sin_addr.s_addr = htonl(INADDR_ANY);
    s.sin_port = htons(port);

    /* 绑定网络地址和网络文件描述符 */
    if (bind(new_socket, (struct sockaddr *)&s, sizeof(struct sockaddr_in)) != 0)
    {
        closesocket(new_socket);
        perror("bind() error");
        return -1;
    }

    /* 根据非阻塞模式标识判断是否要设置成非阻塞模式 */
    if (make_non_blocking)
    {
        if (rtspsrv_set_non_blocking(new_socket) < 0)
        {
            closesocket(new_socket);
            perror("bind() error");
            return -1;
        }
    }
    return new_socket;
}

/******************************************************************************
* 函数介绍:	设置流式套接字
* 输入参数:	port: 端口号;
*			make_non_blocking: 是否是非阻塞模式的标识;
* 输出参数:	无
* 返回值:   成功返回网络文件描述符，出错返回ERR_GENERIC
*****************************************************************************/
int rtspsrv_setup_tcp(sint16 port, int make_non_blocking)
{
    int new_socket = -1;
    int reuse_flag = 1;
    struct sockaddr_in s;

    /* 创建流式网络文件描述符*/
    if ((new_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket() error");
        return -1;
    }

    /* 防止端口被占用*/
    if (setsockopt(new_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse_flag, sizeof(reuse_flag)) < 0)
    {
        closesocket(new_socket);
        perror("setsockopt() error");
        return -1;
    }

    /* 设置网络地址*/
    memset(&s, 0x00, sizeof(struct sockaddr_in));
    s.sin_family = AF_INET;
    s.sin_addr.s_addr = htonl(INADDR_ANY);
    s.sin_port = htons(port);

    /* 绑定网络地址和网络文件描述符*/
    if (bind(new_socket, (struct sockaddr *)&s, sizeof(struct sockaddr_in)) != 0)
    {
        closesocket(new_socket);
        perror("bind() error in setup_stream_socket.");
        return -1;
    }

    /* 根据非阻塞模式标识判断是否要设置成非阻塞模式*/
    if (make_non_blocking)
    {
        if (rtspsrv_set_non_blocking(new_socket) < 0)
        {
            closesocket(new_socket);
            perror("make_socket_non_blocking() error\n");
            return -1;
        }
    }

    /* 监听网络文件描述符*/
    if (listen(new_socket, LISTEN_BACKLOG_SIZE) < 0)
    {
        perror("listen() error in setup_rtsp_socket !");
        return -1;
    }

    return new_socket;
}

/******************************************************************************
* 函数介绍:	判断是否为结束的nal单元，是I帧或者P帧即可
* 输入参数:	enc_type: 编码类型;
*           nal_ptr: nal字节缓存;
* 输出参数:	无
* 返回值:   结束为1，否则为0，<0为错误
*****************************************************************************/
int rsrv_check_end_of_frame(int enc_type, char nal_byte, int * _nal_type)
{
    int nal_type, end_of_frame = 0;

    if (enc_type == VIDEO_H265)
    {
        nal_type = (nal_byte >> 1) & 0x3f;
        if (nal_type != HEVC_NAL_VPS && nal_type != HEVC_NAL_SPS
                && nal_type != HEVC_NAL_PPS && nal_type != HEVC_NAL_SEI_PREFIX)
        {
            end_of_frame = 1;
        }
    }
    else
    {
        nal_type = (nal_byte & 0x1f);
        if (nal_type == H264_NAL_SLICE || nal_type == H264_NAL_IDR_SLICE)
        {
            end_of_frame = 1;
        }
    }

    *_nal_type = nal_type;

    return end_of_frame;
}

int rsrv_find_nal_unit(uint8 * buf, int size, int * nal_start, int * nal_end, int enc_type)
{
    int i = 0, nal_type, end_of_frame = 0;
    *nal_start = 0;
    *nal_end = 0;

    /* ( next_bits( 24 ) != 0x000001 && next_bits( 32 ) != 0x00000001 ) */
    while
    (
        (buf[i] != 0 || buf[i + 1] != 0 || buf[i + 2] != 0x01) &&
        (buf[i] != 0 || buf[i + 1] != 0 || buf[i + 2] != 0 || buf[i + 3] != 0x01)
    )
    {
        i++; /* skip leading zero */
        if (i + 4 > 1024)
        {
            return -1;    /* did not find nal start*/
        }
    }

    if (buf[i] != 0 || buf[i + 1] != 0 || buf[i + 2] != 0x01) /* ( next_bits( 24 ) != 0x000001 )*/
    {
        i++;
    }

    if (buf[i] != 0 || buf[i + 1] != 0 || buf[i + 2] != 0x01)
    {
        /* error, should never happen */ return 0;
    }
    i += 3;
    *nal_start = i;

    end_of_frame = rsrv_check_end_of_frame(enc_type, buf[i], &nal_type);
    if (end_of_frame == HB_TRUE)
    {
        *nal_end = size;
    }

    if (*nal_end == 0)
    {
        /* next_bits( 24 ) != 0x000000 && next_bits( 24 ) != 0x000001 */
        while
        (
            (buf[i] != 0 || buf[i + 1] != 0 || buf[i + 2] != 0x01) &&
            (buf[i] != 0 || buf[i + 1] != 0 || buf[i + 2] != 0 || buf[i + 3] != 0x01)
        )
        {
            i++;
            if (i + 3 >= size)
            {
                i = size;
                break;
            }
        }
        /*av_hex_dump(NULL, buf, 80);*/
        *nal_end = i;
    }
    return (*nal_end - *nal_start);
}

/******************************************************************************
* 函数介绍:	获取ntp时间
* 输入参数:	无
* 输出参数:	无
* 返回值:   ntp时间
*****************************************************************************/
uint64_t rsrv_ntp_time(void)
{
    return (av_gettime() / 1000) * 1000 + NTP_OFFSET_US;
}

int _tcp_do_one(const char * line, struct sockaddr_in * local, struct sockaddr_in * rem, int * rbuf_num, int * sbuf_num)
{
    unsigned long rxq, txq, time_len, retr, inode;
    int num, local_port, rem_port, d, state, uid, timer_run, timeout;
    char rem_addr[128], local_addr[128], more[512];
    struct sockaddr_in localaddr, remaddr;

    num = sscanf(line, "%d: %64[0-9A-Fa-f]:%X %64[0-9A-Fa-f]:%X %X %lX:%lX %X:%lX %lX %d %d %ld %512s\n",
                 &d, local_addr, &local_port, rem_addr, &rem_port, &state,
                 &txq, &rxq, &timer_run, &time_len, &retr, &uid, &timeout, &inode, more);


    sscanf(local_addr, "%X", &((struct sockaddr_in *) &localaddr)->sin_addr.s_addr);
    sscanf(rem_addr, "%X", &((struct sockaddr_in *) &remaddr)->sin_addr.s_addr);
    ((struct sockaddr *) &localaddr)->sa_family = AF_INET;
    ((struct sockaddr *) &remaddr)->sa_family = AF_INET;

    if (!remaddr.sin_addr.s_addr || !local_port || !rem_port)
    {
        return -1;
    }

    if (num < 11)
    {
        /*av_log(NULL, AV_LOG_ERROR, "got bogus tcp line.\n");*/
        return -1;
    }

    if (state == 10) /* TCP_LISTEN */
    {
        time_len = 0;
        retr = 0L;
        rxq = 0L;
        txq = 0L;
    }

    if (rem->sin_addr.s_addr == remaddr.sin_addr.s_addr && ntohs(local->sin_port) == local_port)
    {
        *rbuf_num = rxq;
        *sbuf_num = txq;        
        return 0;
    }    

    return -1;
}


int rsrv_tcp_info(int sockfd, int * rbuf_num, int * sbuf_num)
{
    FILE * procinfo;
	char buffer[8192] = {0};
    struct sockaddr_in local, rem;
    int ret;
    socklen_t local_len, rem_len;

    local_len = sizeof(local);
    rem_len = sizeof(rem);

    ret = getsockname(sockfd, (struct sockaddr *)&local, &local_len);
    if (ret < 0)
    {
        rsrv_log(AV_LOG_ERROR, "getsockname fail[%d].\n", errno);
        return -1;
    }    

    ret = getpeername(sockfd, (struct sockaddr *)&rem, &rem_len);
    if (ret < 0)
    {
        rsrv_log(AV_LOG_ERROR, "getpeername fail[%d].\n", errno);
        return -1;
    }
    
#ifndef _WIN32
	procinfo = fopen(_PATH_PROCNET_TCP, "r");
    if (procinfo == NULL)
    {
        rsrv_log(AV_LOG_ERROR, "%s open fail[%d].\n", _PATH_PROCNET_TCP, errno);
        return -1;
    }

    do
    {
        if (fgets(buffer, sizeof(buffer), procinfo))
        {
            ret = _tcp_do_one(buffer, &local, &rem, rbuf_num, sbuf_num);
        }
    }
    while (!feof(procinfo));

    fclose(procinfo);
#else
	*rbuf_num = 0;
	*sbuf_num = 0;
	procinfo = NULL;
	memset(buffer, 0, sizeof(buffer));
#endif

    return 0;
}

int rss_init_cli(void)
{
    int i;
#ifdef _RTSPSRV_TEST
	_rtrv_cli_info = av_mallocz(sizeof(rsrv_cli_info_t) * RSRV_CLI_NUM);
#else
	_rtrv_cli_info = (rsrv_cli_info_t *)mem_manage_malloc(COMP_RTSPSRV, sizeof(rsrv_cli_info_t) * RSRV_CLI_NUM);
#endif
    
    if (_rtrv_cli_info == NULL)
    {
        rsrv_log(AV_LOG_ERROR,  "_rtrv_cli_info malloc error\n");
        return -1;
    }
    
    memset(_rtrv_cli_info, 0, sizeof(rsrv_cli_info_t) * RSRV_CLI_NUM);
    for (i = 0; i < RSRV_CLI_NUM; i++)
    {
        
    }
    return 0;
}

int rsrv_take_unused_cli(void)
{
    if (++rsrv_info_idx >= RSRV_CLI_NUM)
    {
        rsrv_info_idx = 0;
    }
    return rsrv_info_idx;
}

rsrv_cli_info_t * rsrv_get_cli(int idx)
{
    if (idx >= RSRV_CLI_NUM || idx < 0 || _rtrv_cli_info == NULL)
    {
        return NULL;
    }
    return &_rtrv_cli_info[idx];
}

int rsrv_clear_cli(void)
{
    if (_rtrv_cli_info == NULL)
    {
        return -1;
    }
    
    memset(_rtrv_cli_info, 0, sizeof(rsrv_cli_info_t) * RSRV_CLI_NUM);
    return 0;
}

#ifndef _RTSPSRV_TEST

int rss_stat_device(char * line)
{
    int i, idx = 0;
    char tm_str[32] = {0};
    SYSTIME tm;

    info_seg_init("RTSPSRV HISTORY INFO");

    for (i = 0; i < RSRV_CLI_NUM ; i++)
    {
        rsrv_cli_info_t * ipc = &_rtrv_cli_info[i];
        if ('\0' != ipc->rem_ip_str[0] && sstrstr(line, (char *)ipc->rem_ip_str))
        {
            info_seg_add("Idx",                 INFO_SINTIGER,  idx++);
            info_seg_add("Fd ",                 INFO_SINTIGER,  ipc->fd);
            info_seg_add("St",               INFO_SINTIGER,  ipc->state);
            info_seg_add("Err",               INFO_SINTIGER,  ipc->error_code);
            info_seg_add("Fps",                 INFO_SINTIGER,  ipc->fps);
            info_seg_add("Ec",               INFO_SINTIGER,  ipc->enc_type);
            info_seg_add("Sm",               INFO_SINTIGER,  ipc->stm_type);
            info_seg_add("Bits",               INFO_SINTIGER, ipc->bitrate);
            info_seg_add("    Video",               INFO_SINTIGER,  ipc->vframe_count);
            info_seg_add("    Audio",               INFO_SINTIGER,  ipc->aframe_count);
            info_seg_add(" Vdrop",              INFO_SINTIGER,  ipc->vdrop_count);
            info_seg_add("    RQ",             INFO_SINTIGER,  ipc->rcvq_buf);
            info_seg_add("    SQ",             INFO_SINTIGER,  ipc->sndq_buf);
            info_seg_add(" Hcnt",               INFO_SINTIGER, ipc->high_water_cnt);
            info_seg_add("  Smax",               INFO_SINTIGER, (sint32)ipc->max_snd_time);
            info_seg_add("  Tcnt",               INFO_SINTIGER, ipc->timeout_cnt);
            info_seg_add("Trysnd",               INFO_SINTIGER, ipc->try_snd_len);

            if (ipc->open_tm)
            {
                time_sec2time(ipc->open_tm / 1000000, &tm);
				snprintf(tm_str, 32, TIME_STR, SYSTIME4TM_FMT(tm));
            }
            info_seg_add("       Opentime      ",     INFO_STRING, tm_str);

            memset(tm_str, 0, 32);
            if (ipc->close_tm)
            {
                time_sec2time(ipc->close_tm / 1000000, &tm);
                snprintf(tm_str, 32, TIME_STR, SYSTIME4TM_FMT(tm));
            }
            info_seg_add("      Closetime      ",     INFO_STRING, tm_str);
            
            info_seg_end();
        }
        else if ('\0' != ipc->rem_ip_str[0] && sstrstr(line, "all"))
        {
            info_seg_add("Idx",                 INFO_SINTIGER,  idx++);
            info_seg_addl("         Ipaddr",      INFO_STRING, ipc->rem_ip_str);
            info_seg_add("St",               INFO_SINTIGER,  ipc->state);
            info_seg_add("Err",               INFO_SINTIGER,  ipc->error_code);
            info_seg_add("Fps",                 INFO_SINTIGER,  ipc->fps);
            info_seg_add("Ec",               INFO_SINTIGER,  ipc->enc_type);
            info_seg_add("Sm",               INFO_SINTIGER,  ipc->stm_type);
            info_seg_add("Bits",               INFO_SINTIGER, ipc->bitrate);
            info_seg_add("    Video",               INFO_SINTIGER,  ipc->vframe_count);
            info_seg_add("    Audio",               INFO_SINTIGER,  ipc->aframe_count);
            info_seg_add(" Vdrop",              INFO_SINTIGER,  ipc->vdrop_count);
            info_seg_add("    RQ",             INFO_SINTIGER,  ipc->rcvq_buf);
            info_seg_add("    SQ",             INFO_SINTIGER,  ipc->sndq_buf);
            info_seg_add(" Hcnt",               INFO_SINTIGER, ipc->high_water_cnt);
            info_seg_add("  Smax",               INFO_SINTIGER, (sint32)ipc->max_snd_time);
            info_seg_add("  Tcnt",               INFO_SINTIGER, ipc->timeout_cnt);
            info_seg_add("Trysnd",               INFO_SINTIGER, ipc->try_snd_len);
            if (ipc->open_tm)
            {
                time_sec2time(ipc->open_tm / 1000000, &tm);
				snprintf(tm_str, 32, TIME_STR, SYSTIME4TM_FMT(tm));
            }
            info_seg_add("       Opentime      ",     INFO_STRING, tm_str);
            memset(tm_str, 0, 32);
            if (ipc->close_tm)
            {
                time_sec2time(ipc->close_tm / 1000000, &tm);
				snprintf(tm_str, 32, TIME_STR, SYSTIME4TM_FMT(tm));
            }
            info_seg_add("      Closetime      ",     INFO_STRING, tm_str);
            info_seg_end();
        }
    }
    return 0;
}

#endif

