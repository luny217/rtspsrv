#ifndef _COMMON_H_
#define _COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define CLI_DISABLE

#ifdef _WIN32
//#define CLI_DISABLE
#include <Winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <linux/types.h>
#include <syslog.h>
#include <pthread.h>
#endif /* _WIN32 */

#include <assert.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>

#include "defines.h"
#ifndef CLI_DISABLE
#include "cli.h"
#endif /* CLI_DISABLE */

#define SET_CURRENT_THREAD_PID  (-1)    /* 设置当前线程PID */
#define MAX_THREAD_NAME_LEN     32

typedef enum
{
    CPUID_MASK_1ST = BIT32(0), 
    CPUID_MASK_2ND = BIT32(1), 
    CPUID_MASK_3RD = BIT32(2), 
    CPUID_MASK_4TH = BIT32(3), 
} CPUID_MASK_E;

/* 模块ID定义 */
typedef enum
{
    COMP_COMMON         = 0, 
    COMP_VIDEOIN        = 1, 
    COMP_VIDEOOUT       = 2, 
    COMP_VIDEOENC       = 3, 
    COMP_VIDEODEC       = 4, 
    COMP_AUDIOIN        = 5, 
    COMP_AUDIOOUT       = 6, 
    COMP_AUDIOENC       = 7, 
    COMP_AUDIODEC       = 8, 
    COMP_RECORD         = 9, 
    COMP_PLAYBACK       = 10, 
    COMP_STATMACH       = 11, 
    COMP_NETMS          = 12, 
    COMP_NETVOD         = 13, 
    COMP_NETUPGRADE     = 14, 
    COMP_NETCMD         = 15, 
    COMP_WEBSERV        = 16, 
    COMP_SERIAL         = 17, 
    COMP_PTZ            = 18, 
    COMP_GUI            = 19, 
    COMP_FIFO           = 20, 
    COMP_EVENT          = 21, 
    COMP_MFS            = 22, 
    COMP_MSHEAD         = 23, 
    COMP_OSD            = 24, 
    COMP_VIDEOVPP       = 25, 
    COMP_LINK           = 26, 
    COMP_TIMER          = 27, 
    COMP_RTC            = 28, 
    COMP_ALARM          = 29,   /* 报警模块 */
    COMP_MSG            = 30,   /* 消息队列 */
    COMP_SEM            = 31,   /* 进程间信号灯 */
    COMP_SHM            = 32,   /* 进程间共享内存 */
    COMP_NTP            = 33, 
    COMP_DDNS           = 34, 
    COMP_PPPOE          = 35, 
    COMP_SYS            = 36, 
    COMP_NETWORK        = 37, 
    COMP_TRANSUART      = 38, 
    COMP_DAEMON         = 39, 
    COMP_PROCFIFO       = 40, 
    COMP_EXTCMD         = 41, 
    COMP_SERIALKEYPAD   = 42, 
    COMP_RAID           = 43, 
    COMP_CMS            = 44,   /* 集中管理平台 */
    COMP_HVO            = 45,   /* 高清视频输出 */
    COMP_NETAPI         = 46,   /* 网络接口模块 */
    COMP_SSMTP          = 47,   /* SSMTP模块 */
    COMP_VIDEOAD        = 48,   /* 视频AD模块 */
    COMP_PCIV           = 49,   /* 主从片间通讯PCIV模块 */
    COMP_CDRAPP         = 50,   /* 刻录模块 */
    COMP_PCIVMSG        = 51,   /* PCIV模块 */
    COMP_VIDEOCFG       = 52,   /* 视频基础模块的配置模块 */
    COMP_ATMI           = 53,   /* ATM智能模块 */
    COMP_SOFTRAID       = 54, 
    COMP_VIDEOVOSD      = 55,   /* 视频基础模块的OSD模块 */
    COMP_FTPUPGRADE     = 56,   /* FTP升级模块 */
    COMP_FONT           = 57, 
    COMP_LIST           = 58, 
    COMP_DEBUGS         = 59, 
    COMP_SMART          = 60, 
    COMP_MFSCOMM        = 61, 
    COMP_MFSFAT32       = 62, 
    COMP_MFSDEV         = 63, 
    COMP_MFSFS          = 64, 
    COMP_MFSLOG         = 65,
    COMP_MFSSTORE       = 66, 
    COMP_MFSCTRL        = 67, 
    COMP_MFSQUERY       = 68, 
    COMP_MFSPLAYBACK    = 69, 
    COMP_MFSBACKUP      = 70, 
    COMP_VIDEOVPSS      = 71,
    COMP_VIDEOVDA       = 72,
    COMP_HBSDK          = 73,
    COMP_CONFIG         = 74,
    COMP_DEVLIB         = 75,   /* 外围设备抽象层模块 */
    COMP_TOAVI          = 76, 
    COMP_MEMMANAGE      = 77,   /* 内存管理模块 */
    COMP_GUITRANS       = 78,   /* GUI转换层 */
    COMP_NETCOMM        = 79,   /* 网络基础模块 */
    COMP_CYASSL         = 80,   /* 邮件上传加密模块 */
    COMP_HUAKE          = 81,   /* 华科平台 */
    COMP_TOMP4          = 82,   /* MP4转换模块 */
    COMP_BLOWFISH       = 83,   /* BLOWFISH加解密模块 */
    COMP_RC4            = 84,   /* RC4加解密模块 */
    COMP_MD5            = 85,   /* MD5加密模块 */
    COMP_BASE64         = 86,   /* BASE64加解密模块 */
    COMP_MFSBTREE       = 87,   /* MFS_BTREE模块 */
    COMP_PROCCOMM       = 88,   /* PROC_COMM模块 */
    COMP_MFSLINK        = 89,   /* MFS_LINK模块 */
    COMP_MFSFCLK        = 90,   /* MFS_FCLK模块 */
    COMP_MFSFILE        = 91,   /* MFS_FILE模块 */
    COMP_MFSNFS         = 92,   /* MFS_NFS模块 */
    COMP_UPNP           = 93,   /* UPNP模块 */
    COMP_TEST           = 94,   /* 测试模块 */
    COMP_IPDEV          = 95,   /* IPDEV模块 */
    COMP_RTSP           = 96,   /* RTSP模块 */
    COMP_PNP            = 97,   /* IPC即插即用 */
    COMP_FTPAPI         = 98,   /* FTP应用接口模块 */
    COMP_DHCP           = 99,   /* DHCP服务器 */
    COMP_POE            = 100,  /* POE模块 */
    COMP_MFSMKFS        = 101,  /* MFS_MKFS模块 */
    COMP_FLASHLOG       = 102,  /* 写FLASHLOG模块 */
    COMP_VPN            = 103,  /* VPN模块 */
    COMP_SSTRING        = 104,  /* SSTRING模块 */
    COMP_THREADPOOL     = 105,  /* THREADPOOL模块 */
    COMP_MEMPOOL        = 106,  /* MEMPOOL模块 */
    COMP_DIAGNOSIS      = 107,  /* 网络诊断模块 */
    COMP_RTMP           = 108,  /* RTMP模块 */
    COMP_ISAPI          = 109,  /* ISAPI模块 */
    COMP_AUDIOCFG       = 110,  /* 音频基础模块的配置模块 */
    COMP_RTSPSRV        = 111,  /* RTSP SERVER模块*/
    COMP_SDKAUDIOPRO    = 112,  /* 音频处理模块,主要处理报警,对讲音频 */
    COMP_POECTRL        = 113,  /* POE功率控制模块 */
    COMP_NDCMP          = 114,  /* 设备组网管理模块 */
    COMP_SSOCKET        = 115,  /* 网络套接字基础模块 */
    COMP_NDCMPSRV       = 116,  /* 设备组网管理服务器模块 */
    COMP_DANALE         = 117,  /* 大拿穿透服务 */
    COMP_MAX
} COMPID, *PCOMPID;

/* 模块名称定义 */
static const char *const COMPSTR[] MASK_NO_USED_WARNING =
{
    "COMMON",       /* 0 */
    "VIDEOIN", 
    "VIDEOOUT", 
    "VIDEOENC", 
    "VIDEODEC", 
    "AUDIOIN", 
    "AUDIOOUT", 
    "AUDIOENC", 
    "AUDIODEC", 
    "RECORD", 
    "PLAYBACK",     /* 10 */
    "STATMACH", 
    "NETMS", 
    "NETVOD", 
    "NETUPGRADE", 
    "NETCMD", 
    "WEBSERV", 
    "SERIAL", 
    "PTZ", 
    "GUI", 
    "FIFO",         /* 20 */
    "EVENT", 
    "MFS", 
    "MSHEAD", 
    "OSD", 
    "VIDEOVPP", 
    "LINK", 
    "TIMER", 
    "RTC", 
    "ALARM",        /* 报警模块 */
    "MSG",          /* 30 消息队列 */
    "SEM",          /* 进程间信号灯 */
    "SHM",          /* 进程间共享内存 */
    "NTP", 
    "DDNS", 
    "PPPOE", 
    "SYS", 
    "NETWORK", 
    "TRANSUART", 
    "DAEMON", 
    "PROCFIFO",     /* 40 */
    "EXTCMD", 
    "SKEYPAD", 
    "RAID", 
    "CMS", 
    "HVO", 
    "NETAPI", 
    "SSMTP", 
    "VIDEOAD", 
    "PCIV", 
    "CDRAPP",       /* 50 刻录模块 */
    "PCIVMSG", 
    "VIDEOCFG", 
    "ATMI",         /* ATM智能模块 */
    "SOFTRAID", 
    "VIDEOOSD", 
    "FTPUPGRADE", 
    "FONT", 
    "LIST", 
    "DEBUGS", 
    "SMART",        /* 60 */
    "MFSCOMM", 
    "MFSFAT32", 
    "MFSDEV", 
    "MFSFS", 
    "MFSLOG", 
    "MFSSTORE", 
    "MFSCTRL", 
    "MFSQUERY", 
    "MFSPLAYBACK", 
    "MFSBACKUP",    /* 70 */
    "VIDEOVPSS", 
    "VIDEOVDA", 
    "HBSDK", 
    "CONFIG", 
    "DEVLIB", 
    "TOAVI", 
   	"MEMMANAGE", 
    "GUITRANS", 
    "NETCOMM", 
    "CYASSL", 
    "HUAKE", 
    "TOMP4", 
    "BLOWFISH",     /* BLOWFISH加解密模块 */
    "RC4",          /* RC4加解密模块 */
    "MD5",          /* MD5加密模块 */
    "BASE64",       /* BASE64加解密模块 */
    "MFSBTREE",     /* MFS_BTREE模块 */
    "PROCCOMM",     /* PROC_COMM模块 */
    "MFSLINK",      /* MFS_LINK模块 */
    "MFSFCLK",      /* MFS_FCLK模块 */
    "MFSFILE",      /* MFS_FILE模块 */
    "MFSNFS",       /* MFS_NFS模块 */
    "UPNP",         /* UPNP模块 */
    "TEST",         /* 测试模块 */
    "IPDEV",        /* IPDEV模块 */
    "RTSP",         /* RTSP模块 */
    "PNP",          /* IPC即插即用 */
    "FTPAPI",       /* FTP应用接口模块 */
    "DHCP",         /* DHCP服务器 */
    "POE",          /* POE模块 */
    "MFSMKFS",      /* MFS_MKFS模块 */
    "FLASHLOG",     /* 写FLASHLOG模块 */
    "VPN",          /* VPN模块 */
    "SSTRING",      /* SSTRING模块 */
    "THREADPOOL",   /* THREADPOOL模块 */
    "MEMPOOL",      /* MEMPOOL模块 */
    "DIAGNOSIS",    /* 网络诊断模块 */
    "RTMP",         /* RTMP模块 */
    "ISAPI",        /* ISAPI模块 */
    "AUDIOCFG",     /* 音频基础模块的配置模块 */
    "RTSPSRV",      /* RTSP SERVER模块 */
    "SDKAUDIOPRO",  /* 音频处理模块,主要处理报警,对讲音频 */
    "POECTRL",      /* POE功率控制 */
    "NDCMP",        /* 设备组网管理模块 */
    "SSOCKET",      /* 网络套接字基础模块 */
    "NDCMPSRV",     /* 设备组网管理服务器模块 */
    "DANALE",       /* 大拿穿透服务 */
    NULL            /* 字符串结束 */
};

#ifdef NOUSE_COMPFD
#else
typedef struct
{
    sint32 common;
    sint32 vi;
    sint32 vo;
    sint32 venc;
    sint32 vdec;
    sint32 ai;
    sint32 ao;
    sint32 aenc;
    sint32 adec;
    sint32 rec;
    sint32 pb, pb_net;  /* 回放模块句柄(本地,网络)*/
    sint32 statm;
    sint32 netms;
    sint32 netvod;
    sint32 upgrade;
    sint32 netcmd;
    sint32 webserv;
    sint32 uart;
    sint32 ptz;
    sint32 gui;
    sint32 fifo;
    sint32 event;
    sint32 mfs;
    sint32 mshead;
    sint32 osd;
    sint32 vpp;
    sint32 link;
    sint32 timer;
    sint32 rtc;
    sint32 alm;
    sint32 msg;
    sint32 sem;
    sint32 shmem;
    sint32 ntp;
    sint32 ddns;
    sint32 pppoe;
    sint32 sys;
    sint32 network;
    sint32 trans_uart;
    sint32 daemon;
    sint32 procfifo;
    sint32 extcmd;
    sint32 skeypad;
    sint32 raid;
    sint32 cms;
    sint32 hvo;
    sint32 netapi;
    sint32 ssmtp;
    sint32 vad;
    sint32 pciv;
    sint32 cdrapp;
    sint32 pciv_msg;
    sint32 vcfg;
    sint32 atmi;
    sint32 soft_raid;
    sint32 vosd;
    sint32 ftp_upgrade;
    sint32 font;
    sint32 list;
    sint32 debugs;
    sint32 smart;
    sint32 mfscomm;
    sint32 mfsfat32;
    sint32 mfsdev;
    sint32 mfsfs;
    sint32 mfslog;
    sint32 mfsstore;
    sint32 mfsctrl;
    sint32 mfsquery;
    sint32 mfsplayback;
    sint32 mfsbackup;
    sint32 vpss;
    sint32 vda;
    sint32 hbsdk;
    sint32 config;
    sint32 devlib;
    sint32 toavi;
	sint32 memmanage;
    sint32 guitrans;
	sint32 netcomm;
	sint32 cyassl;
	sint32 huake;
	sint32 tomp4;
    sint32 blowfish;    /* BLOWFISH加解密模块 */
    sint32 rc4;         /* RC4加解密模块 */
    sint32 md5;         /* MD5加密模块 */
    sint32 base64;      /* BASE64加解密模块 */
    sint32 mfsbtree;    /* MFS_BTREE模块 */
    sint32 proccomm;    /* PROC_COMM模块 */
    sint32 mfslink;     /* MFS_LINK模块 */
    sint32 mfsfclk;     /* MFS_FCLK模块 */
    sint32 mfsfile;     /* MFS_FILE模块 */
    sint32 mfsnfs;      /* MFS_NFS模块 */
    sint32 upnp;        /* UPNP模块 */
    sint32 test;        /* 测试模块 */
    sint32 ipdev;       /* IPDEV模块 */
    sint32 rtsp;        /* RTSP模块 */
    sint32 pnp;         /* IPC即插即用 */
    sint32 ftpapi;      /* FTP应用接口模块 */
    sint32 dhcp;        /* DHCP服务器 */
    sint32 poe;         /* POE模块 */
    sint32 mfsmkfs;     /* MFS_MKFS模块 */
    sint32 flashlog;    /* 写FLASHLOG模块 */
    sint32 vpn;         /* VPN模块 */
    sint32 sstring;     /* SSTRING模块 */
    sint32 threadpool;  /* THREADPOOL模块 */
    sint32 mempool;     /* MEMPOOL模块 */
	sint32 diagnosis;   /* 网络诊断模块 */
    sint32 rtmp;        /* RTMP模块 */
    sint32 isapi;       /* ISAPI模块 */
    sint32 audiocfg;    /* 音频基础模块的配置模块 */
    sint32 rtspsrv;     /* rtsp server模块 */
    sint32 sdkaudiopro; /* 音频处理模块,主要处理报警,对讲音频 */
    sint32 poectrl;     /* POE功率控制 */
    sint32 ndcmp;       /* 设备组网管理模块 */
    sint32 ssocket;     /* 网络套接字基础模块 */
    sint32 ndcmpsrv;    /* 设备组网管理服务器模块 */
    sint32 danale;      /* 大拿穿透服务 */
} COMPFD, *PCOMPFD;

COMPFD sys_handle;
#endif /* NOUSE_COMPFD */

/******************************************************************************
 * 错误代码定义: 类型为sint32,错误代码必须是负数,所以最高位置1,32位定义如下: 
 *  +-+-----+-------+---------------+---------------+---------------+
 *  |1|0 9 8|7 6 5 4|3 2 1 0 9 8 7 6|5 4 3 2 1 0 9 8|7 6 5 4 3 2 1 0|
 *  +-+-----+-------+---------------+---------------+---------------+
 *  |1|LEVEL|   R   |    COMPID     |            ERRORNO            |
 *  +-+-----+-------+---------------+---------------+---------------+
 * 这里: LEVEL:错误级别,R:保留位,COMPID:模块号,ERRORNO:错误号
 * 用户模块错误号从32开始定义,当返回错误时,采用ERRNO(errno, compid)形式
 * 错误代码采用枚举定义,格式为COMP_ERROR_XXX,全部大写,各模块按此标准定义
 *****************************************************************************/
#define ERRNO_L(level, error, id)   (0x80000000 | ((level) << 28) | ((id) << 16) | (error))

/*
 * 错误级别
 * HB_ERROR_LEVEL_MODULE: 单独某一模块重启后就可以解决该问题
 * HB_ERROR_LEVEL_SYSTEM: 整个应用软件系统重启才可以解决该问题
 * HB_ERROR_LEVEL_OS: 机器重启才可以解决该问题
 */
#define HB_ERROR_LEVEL_MODULE   0   /* 模块级别错误 */
#define HB_ERROR_LEVEL_SYSTEM   1   /* 系统级别错误 */
#define HB_ERROR_LEVEL_OS       2   /* 操作系统级别错误 */

#define GET_ERRNO(errno)    ((errno) & 0x0000FFFF)
#define GET_COMPID(errno)   (((errno) >> 16) & 0xFF)
#define GET_COMPSTR(compid) MUX(compid < COMP_MAX, COMPSTR[compid], "UNKNOWN")
#define ERRNO(error, id)    ERRNO_L(HB_ERROR_LEVEL_MODULE, error, id)

#ifdef CLI_DISABLE
#define PERROR(errno)   do { printf("COMP_%s: errno=%u, [%s:%s #%d]\n", \
    GET_COMPSTR(GET_COMPID(errno)), GET_ERRNO(errno), __FILE__, __FUNCTION__, __LINE__); } while (0)
#else
#define PERROR(errno)   do { \
    sint32 cli_flg = cli_get_prn_manner_flg(); \
    switch (cli_flg) \
    { \
    case PRN_MANNER_TERM: \
        printf("COMP_%s: errno=%u, [%s:%s #%d]\n", \
            GET_COMPSTR(GET_COMPID(errno)), GET_ERRNO(errno), __FILE__, __FUNCTION__, __LINE__); \
        break; \
    case PRN_MANNER_SYSLOG: \
        syslog(LOG_USER | LOG_ERR, "COMP_%s: errno=%u, [%s:%s #%d]\n", \
            GET_COMPSTR(GET_COMPID(errno)), GET_ERRNO(errno), __FILE__, __FUNCTION__, __LINE__); \
        break; \
    default: \
        break; \
    } } while (0)
#endif /* CLI_DISABLE */

#define TRACE_SWVER(compid, swver)  printf("COMP_%s: V%d.%d.%d build%04d%02d%02d[Build Time: %s %s]\n", \
    GET_COMPSTR(compid), ((SWVERSION)swver).majver, ((SWVERSION)swver).secver, \
    ((SWVERSION)swver).minver, ((SWVERSION)swver).year + 2000, \
    ((SWVERSION)swver).month, ((SWVERSION)swver).day, __DATE__, __TIME__)

#ifdef ERR_DEBUG
#ifdef CLI_DISABLE
#define TRACE_DBG(str, args...) do { printf(COLOR_STR_GREEN" %s>%s:#%d, "COLOR_STR_NONE" "str, \
    __FILE__, __FUNCTION__, __LINE__, ## args); } while(0)
#else
#define TRACE_DBG(str, args...) do { \
    sint32 cli_flg = cli_get_prn_manner_flg(); \
    switch (cli_flg) \
    { \
    case PRN_MANNER_TERM: \
        printf(COLOR_STR_GREEN" %s>%s:#%d, "COLOR_STR_NONE" "str, __FILE__, __FUNCTION__, __LINE__, ## args); \
        break; \
    case PRN_MANNER_SYSLOG: \
        syslog(LOG_USER | LOG_DEBUG, " %s>%s:#%d, "str, __FILE__, __FUNCTION__, __LINE__, ## args); \
        break; \
    default: \
        break; \
    } } while(0)
#endif /* CLI_DISABLE */

#ifdef CLI_DISABLE
#define TRACE_PRI(str, args...) do { printf(str, ## args); } while(0)
#else
#define TRACE_PRI(str, args...) do { \
    sint32 cli_flg = cli_get_prn_manner_flg(); \
    switch (cli_flg) \
    { \
    case PRN_MANNER_TERM: \
        printf(str, ## args); \
        break; \
    case PRN_MANNER_SYSLOG: \
        syslog(LOG_USER | LOG_DEBUG, str, ## args); \
        break; \
    default: \
        break; \
    } } while(0)
#endif /* CLI_DISABLE */
#else
#ifdef _WIN32
#define TRACE_DBG(str, ...)
#define TRACE_PRI(str, ...)
#else
#define TRACE_DBG(str, args...)
#define TRACE_PRI(str, args...)
#endif /* _WIN32 */
#endif /* ERR_DEBUG */

#ifdef S_SPLINT_S
#define TRACE_ERR(str, args...) 
#else /* S_SPLINT_S */
#ifdef CLI_DISABLE
#ifdef _WIN32
//#define TRACE_ERR(str, ...)
#else
#define TRACE_ERR(str, args...) do { \
    time_t tm_tmp; struct tm *ptm = NULL; time(&tm_tmp); ptm = gmtime(&tm_tmp); \
    printf(COLOR_STR_RED" "TIME_STR" %s>%s:#%d, "COLOR_STR_NONE" "str, \
        ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, \
        ptm->tm_sec, __FILE__, __FUNCTION__, __LINE__, ## args); } while(0)
#endif /* _WIN32 */
#else
#define TRACE_ERR(str, args...) do { \
    sint32 cli_flg = cli_get_prn_manner_flg(); \
    switch (cli_flg) \
    { \
    case PRN_MANNER_TERM: \
    { \
        time_t tm_tmp; struct tm *ptm = NULL; time(&tm_tmp); ptm = gmtime(&tm_tmp); \
        printf(COLOR_STR_RED" "TIME_STR" %s>%s:#%d, "COLOR_STR_NONE" "str, \
            ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, \
            ptm->tm_sec, __FILE__, __FUNCTION__, __LINE__, ## args); \
        break; \
    } \
    case PRN_MANNER_SYSLOG: \
        syslog(LOG_USER | LOG_ERR, " %s>%s:#%d, "str, __FILE__, __FUNCTION__, __LINE__, ## args); \
        break; \
    default: \
        break; \
    } } while (0)
#endif /* CLI_DISABLE */
#endif /* S_SPLINT_S */

#ifdef ERR_DEBUG
#define TRACE_ASSERT(exp, str, args...) if (!(exp)) { \
    do { \
        time_t tm_tmp; struct tm *ptm = NULL; time(&tm_tmp); ptm = gmtime(&tm_tmp); \
        printf(COLOR_STR_RED" "TIME_STR"assert @ %s>%s:#%d, "COLOR_STR_NONE" "str, \
            ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec, \
            __FILE__, __FUNCTION__, __LINE__, ## args); usleep(3000000);\
        } while (!(exp)); \
    }
#else
#ifdef _WIN32
#define TRACE_ERR(str, ...) do { printf(str, __VA_ARGS__); } while(0)
#else
#define TRACE_ASSERT(exp, str, args...) if (!(exp)) { \
    do { \
        time_t tm_tmp; struct tm *ptm = NULL; time(&tm_tmp); ptm = gmtime(&tm_tmp); \
        TRACE_ERR(COLOR_STR_RED" "TIME_STR"assert @ %s>%s:#%d, "COLOR_STR_NONE" "str, \
            ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec, \
            __FILE__, __FUNCTION__, __LINE__, ## args);\
        } while (0); \
    }
#endif /* _WIN32 */
#endif /* ERR_DEBUG */

//#define USER_DEBUG
#ifdef USER_DEBUG
#ifdef CLI_DISABLE
#define TRACE_REL(str, args...) do { printf(" %s>%s:#%d, "str, __FILE__, __FUNCTION__, __LINE__, ## args); } while(0)
#else
#define TRACE_REL(str, args...) do { \
    sint32 cli_flg = cli_get_prn_manner_flg(); \
    switch (cli_flg) \
    { \
    case PRN_MANNER_TERM: \
        printf(" %s>%s:#%d, "str, __FILE__, __FUNCTION__, __LINE__, ## args); \
        break; \
    case PRN_MANNER_SYSLOG: \
        syslog(LOG_USER | LOG_INFO, " %s>%s:#%d, "str, __FILE__, __FUNCTION__, __LINE__, ## args); \
        break; \
    default: \
        break; \
    } } while(0)
#endif /* CLI_DISABLE */
#else
#ifdef _WIN32
#define TRACE_REL(str, ...)
#else
#define TRACE_REL(str, args...)
#endif /* _WIN32 */
#endif

#ifdef S_SPLINT_S
#define TRACE_ALWAYS(str, args...)
#define TRACE_DBG_LVL(module_id, level, str, args...)
#else /* S_SPLINT_S */
#ifdef _WIN32
#define TRACE_ALWAYS(str, ...)      do { printf(str, __VA_ARGS__); } while(0)
#else
#define TRACE_ALWAYS(str, args...)  do { printf(str, ## args); } while(0)
#endif /* _WIN32 */
#ifdef CLI_DISABLE
#ifdef _WIN32
#define TRACE_DBG_LVL(module_id, level, fmt, ...)
#else
#define TRACE_DBG_LVL(module_id, level, fmt, args...)
#endif /* _WIN32 */
#else
#define TRACE_DBG_LVL(module_id, level, fmt, args...)   \
    cli_mod_lvl_prn(module_id, level, "%s#%d, "fmt, __FUNCTION__, __LINE__, ## args)
#endif /* CLI_DISABLE */
#endif /* S_SPLINT_S */

typedef enum
{
    COMMON_ERROR_HANDLEINVAL = 1,       /* 句柄描述字非法 */
    COMMON_ERROR_HANDLEOPEND,           /* 句柄已打开 */
    COMMON_ERROR_NOCMD,                 /* 命令不允许 */
    COMMON_ERROR_PARAM,                 /* 参数错误 */
    COMMON_ERROR_NOMEM,                 /* 没有足够内存 */
    COMMON_ERROR_THREADSTART,           /* 线程启动错误 */
    COMMON_ERROR_THREADSTARTED,         /* 线程已启动 */
    COMMON_ERROR_SETTHREADNAME,         /* 设置线程名错误 */
    COMMON_ERROR_SETPTHREADAFFINITY,    /* 设置线程运行属性错误 */
    COMMON_ERROR_FILE_OPEN,             /* 打开文件错误 */
    COMP_ERROR_START = 32,              /* 用户模块起始错误号 */
} COMMON_ERROR_E;

/******************************************************************************
 * 公共命令定义: 用于模块xxx_ioctrl(..., cmd, ...)中cmd的定义
 * CMD域中0~31为公共命令,32~255可由模块自定义
 * 控制命令采用枚举定义,格式为COMP_CMD_XXX,全部大写,各模块按此标准定义
 *****************************************************************************/
#define CMDNO(cmd, id)  (((id) << 16) | (cmd))

typedef enum
{
    COMMON_CMD_GETVERSION = CMDNO(0, COMP_COMMON),  /* 获取模块版本 */
    COMMON_CMD_RESET,       /* 复位当前句柄 */
    COMMON_CMD_GETHEALTHY,  /* 获取句柄健康状况,param(sint32 *),按位表示各通道状态(0-正常,1-故障) */
    COMMON_CMD_GETPID,      /* 获取句柄线程ID,param(sint32 *) */
    COMMON_CMD_GETPROCINFO, /* 获取各模块的运行信息 */
} COMMON_CMD_E;

typedef struct
{
    uint64 usec_next;       /* 下一次执行时间,单位微秒(根据当前时间和usec_interval算出的时间,初始化时为0) */
    uint32 usec_interval;   /* 想要的时间间隔,单位微秒(0-不做USLEEP,非0-将根据时间间隔做相应的休眠) */
    uint32 reserved;        /* 保留位 */
} TIME_INTERVAL_S, *PTIME_INTERVAL_S;
#define TIME_INTERVAL_S_LEN sizeof(TIME_INTERVAL_S)

#define MAX_MTD_DEV_NUMS        16
#define MAX_MTD_DEV_NAME_LEN    16

typedef struct
{
    uint8 upgrade;
    uint8 reserved[3];
    sint32 size;
    sint32 free;
    char name[MAX_MTD_DEV_NAME_LEN];
} MTD_DEV_ATTR_S, *PMTD_DEV_ATTR_S;
#define MTD_DEV_ATTR_S_LEN  sizeof(MTD_DEV_ATTR_S)

typedef struct
{
    uint8 dev_nums;
    uint8 reserved[3];
    MTD_DEV_ATTR_S attr[MAX_MTD_DEV_NUMS];
} MTD_DEVS_ATTR_S, *PMTD_DEVS_ATTR_S;
#define MTD_DEVS_ATTR_S_LEN sizeof(MTD_DEVS_ATTR_S)

/******************************************************************************
 * 函数介绍: 数据处理回调
 * 输入参数: idx: 通道号
 *           fifo: 数据缓存FIFO
 *           data: 数据接收缓冲区
 *           size: 数据接收缓冲区大小
 * 输出参数: 无
 * 返回值  : sint32
 * 注: 当fifo为0时,数据存储于data中,size表示数据长度
 *     当fifo不为0时,数据存储于fifo中,data表示用户缓冲区地址,size表示缓冲区长度
 *     当fifo,data都为空时,表示无数据需要处理,也即数据处理结束标记
 *****************************************************************************/
typedef sint32 (*datacallback)(sint32 idx, sint32 fifo, char *data, sint32 size);

/******************************************************************************
 * 函数介绍: 消息通知回调
 * 输入参数: wparam: 参数
 *           lparam: 参数
 * 输出参数: 无
 * 返回值  : 无
 *****************************************************************************/
typedef void (*notifycallback)(sint32 wparam, sint32 lparam);

#ifndef _WIN32
/******************************************************************************
 * 函数介绍: 线程启动函数
 * 输入参数: priority: 线程优先级(0-按默认值处理,不做优先级设置,1~99-线程优先级别)
 *           func: 线程执行函数体
 *           wparam: 函数参数
 * 输出参数: pid: 线程ID
 * 返回值  : 0-成功,<0-错误代码
 *****************************************************************************/
sint32 pthread_start(sint32 *pid, sint32 priority, notifycallback func, sint32 wparam);

/******************************************************************************
 * 函数功能: 按一定格式设置线程名字(长度不超过MAX_THREAD_NAME_LEN个字符)
 * 输入参数: fmt: 名字格式
 * 输出参数: 无
 * 返回值  : >=0-成功,<0-错误代码
 *****************************************************************************/
sint32 pthread_set_thread_name(const char *fmt, ...);

/******************************************************************************
 * 函数功能: 多核平台线程绑定指定CPU核上运行,绑定的CPU核可以是多个
 * 输入参数: pid: 线程号PID(SET_CURRENT_THREAD_PID-表示设置当前线程,函数内部会自动获取当前线程号后设置)
 *           cpuid_mask: CPU序号ID掩码,按位表示(CPUID_MASK_E)
 * 输出参数: 无
 * 返回值  : 0-成功,<0-错误代码
 *****************************************************************************/
sint32 pthread_set_affinity_np(pthread_t pid, CPUID_MASK_E cpuid_mask);
#endif /* _WIN32 */

/******************************************************************************
 * 函数介绍: 计算该月份总天数
 * 输入参数: year: 年
 *           month: 月份
 * 输出参数: 无
 * 返回值  : 当前月份的天数
 *****************************************************************************/
sint32 time_dayofmonth(uint16 year, uint8 month);

/******************************************************************************
 * 函数介绍: 计算该日期的星期数
 * 输入参数: year: 年
 *           month: 月份
 *           day: 日期
 * 输出参数: 无
 * 返回值  : 1~7: 星期一~星期天
 *****************************************************************************/
sint32 time_weekofdate(uint16 year, uint8 month, uint8 day);

/******************************************************************************
 * 函数介绍: 计算自1970.1.1 00:00:00至当前日期时间的总秒数,包含当秒
 * 输入参数: sys_tm: 系统时间定义结构体
 * 输出参数: 无
 * 返回值  : 自1970.1.1 00:00:00至日期的总秒数
 *****************************************************************************/
time_t time_time2sec(SYSTIME sys_tm);

/******************************************************************************
 * 函数介绍: 系统时间总秒数到系统时间转换
 * 输入参数: sec: 系统时间总秒数
 *           psystm: 系统时间结构体指针
 * 输出参数: psystm: 系统时间结构体指针,系统时间
 * 返回值  : 0-成功,<0-错误代码
 *****************************************************************************/
uint32 time_sec2time(uint32 sec, PSYSTIME psystm);

#ifdef _WIN32
sint32 gettimeofday(struct timeval *tp, sint32 *tzp);

#else
/******************************************************************************
 * 函数介绍: 从RTC同步系统当前时间(暂未实现)
 * 输入参数: 无
 * 输出参数: 无
 * 返回值  : 0-成功,<0-失败
 *****************************************************************************/
sint32 time_utcfromrtc(void);

/******************************************************************************
 * 函数介绍: 从系统当前时间同步RTC(暂未实现)
 * 输入参数: 无
 * 输出参数: 无
 * 返回值  : 0-成功,<0-失败
 *****************************************************************************/
sint32 time_utctortc(void);

/******************************************************************************
 * 函数介绍: 获取系统当前的当地时间
 * 输入参数: 无
 * 输出参数: time
 * 返回值  : >0-成功(1~7:分别表示星期一到星期天),<0-失败
 *****************************************************************************/
sint32 time_get(PSYSTIME psys_tm);

/******************************************************************************
 * 函数介绍: 设置系统当前的当地时间,并未同步RTC
 * 输入参数: time
 * 输出参数: 无
 * 返回值  : 0-成功,<0-失败
 *****************************************************************************/
sint32 time_set(PSYSTIME psys_tm);

/******************************************************************************
 * 函数介绍: 获取系统启动时间,即系统上电经历时间
 * 输入参数: pup_tm_sec: 系统上电经历时间秒数
 *           pup_tm_msec: 系统上电经历时间毫秒数
 * 输出参数: pup_tm_sec: 系统上电经历时间秒数
 *           pup_tm_msec: 系统上电经历时间毫秒数
 * 返回值  : 0-成功,<0-失败
 *****************************************************************************/
sint32 uptime_get(sint32 *pup_tm_sec, sint32 *pup_tm_msec);

/******************************************************************************
 * 函数介绍: 根据前后两次函数调用,计算程序执行时间,单位毫秒
 * 输入参数: pzero_init: 值为0时,获取初始值(即系统上电经历时间),返回值为0
 * 输出参数: pusr_tm: user time
 *           psys_tm: sys time
 * 返回值  : 两次调用的时间间隔real time
 *****************************************************************************/
sint32 runtime_get_msec(sint32 *pzero_init, sint32 *pusr_tm, sint32 *psys_tm);

/******************************************************************************
 * 函数介绍: 根据前后两次执行的系统时间和间隔时间做系统微秒级的休眠
 * 输入参数: ptm_interval
 * 输出参数: 无
 * 返回值  : 0-成功,<0-失败
 *****************************************************************************/
sint32 time_interval_usleep(PTIME_INTERVAL_S ptm_interval);

/******************************************************************************
 * 函数介绍: SWVERSION编译时间设置(取当前系统时间),版本号需要手动事先填好,再根据模块ID号打印出来
 * 输入参数: compid: 模块ID号
 *           pswver: 模块软件版本结构体指针
 * 输出参数: pswver: 模块软件版本结构体
 * 返回值  : 无
 *****************************************************************************/
void swver_set(COMPID compid, PSWVERSION pswver);

/******************************************************************************
 * 函数介绍: 根据目标和源字符串长度拷贝源字符串到目标字符串中,长度取其小者
 * 输入参数: dst: 目标字符串指针
 *           src: 源字符串指针
 *           dst_len: 目标字符串长度
 * 输出参数: dst: 目标字符串
 * 返回值  : >=0-表示拷贝字符串长度
 *****************************************************************************/
size_t strl_cpy(char *dst, const char *src, size_t dst_len);

/******************************************************************************
 * 函数介绍: 数据16进制打印
 * 输入参数: p: 目标字符串指针
 *           s: 源字符串指针
 * 返回值  : 无
 *****************************************************************************/
void data_hex_dump(char *p, sint32 s);

/******************************************************************************
 * 函数介绍: 获取MTD设备属性信息
 * 输入参数: pdevs_attr: MTD设备属性结构体指针
 * 输出参数: pdevs_attr: MTD设备属性结构体指针,MTD设备属性
 * 返回值  : >0-成功(1~7:分别表示星期一到星期天),<0-失败
 *****************************************************************************/
sint32 get_mtd_devs_attr(PMTD_DEVS_ATTR_S pdevs_attr);
#endif /* _WIN32 */

/**
 * base64_encode - Base64 encode
 * @src: Data to be encoded
 * @len: Length of the data to be encoded
 * @out_len: Pointer to output length variable, or %NULL if not used
 * Returns: Allocated buffer of out_len bytes of encoded data,
 * or %NULL on failure
 *
 * Caller is responsible for freeing the returned buffer. Returned buffer is
 * nul terminated to make it easier to use as a C string. The nul terminator is
 * not included in out_len.
 */
unsigned char *base64_encode(const unsigned char *src, size_t len, size_t *out_len);

/**
 * base64_decode - Base64 decode
 * @src: Data to be decoded
 * @len: Length of the data to be decoded
 * @out_len: Pointer to output length variable
 * Returns: Allocated buffer of out_len bytes of decoded data,
 * or %NULL on failure
 *
 * Caller is responsible for freeing the returned buffer.
 */
unsigned char *base64_decode(const unsigned char *src, size_t len, size_t *out_len);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _COMMON_H_ */
