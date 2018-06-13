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

#define SET_CURRENT_THREAD_PID  (-1)    /* ���õ�ǰ�߳�PID */
#define MAX_THREAD_NAME_LEN     32

typedef enum
{
    CPUID_MASK_1ST = BIT32(0), 
    CPUID_MASK_2ND = BIT32(1), 
    CPUID_MASK_3RD = BIT32(2), 
    CPUID_MASK_4TH = BIT32(3), 
} CPUID_MASK_E;

/* ģ��ID���� */
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
    COMP_ALARM          = 29,   /* ����ģ�� */
    COMP_MSG            = 30,   /* ��Ϣ���� */
    COMP_SEM            = 31,   /* ���̼��źŵ� */
    COMP_SHM            = 32,   /* ���̼乲���ڴ� */
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
    COMP_CMS            = 44,   /* ���й���ƽ̨ */
    COMP_HVO            = 45,   /* ������Ƶ��� */
    COMP_NETAPI         = 46,   /* ����ӿ�ģ�� */
    COMP_SSMTP          = 47,   /* SSMTPģ�� */
    COMP_VIDEOAD        = 48,   /* ��ƵADģ�� */
    COMP_PCIV           = 49,   /* ����Ƭ��ͨѶPCIVģ�� */
    COMP_CDRAPP         = 50,   /* ��¼ģ�� */
    COMP_PCIVMSG        = 51,   /* PCIVģ�� */
    COMP_VIDEOCFG       = 52,   /* ��Ƶ����ģ�������ģ�� */
    COMP_ATMI           = 53,   /* ATM����ģ�� */
    COMP_SOFTRAID       = 54, 
    COMP_VIDEOVOSD      = 55,   /* ��Ƶ����ģ���OSDģ�� */
    COMP_FTPUPGRADE     = 56,   /* FTP����ģ�� */
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
    COMP_DEVLIB         = 75,   /* ��Χ�豸�����ģ�� */
    COMP_TOAVI          = 76, 
    COMP_MEMMANAGE      = 77,   /* �ڴ����ģ�� */
    COMP_GUITRANS       = 78,   /* GUIת���� */
    COMP_NETCOMM        = 79,   /* �������ģ�� */
    COMP_CYASSL         = 80,   /* �ʼ��ϴ�����ģ�� */
    COMP_HUAKE          = 81,   /* ����ƽ̨ */
    COMP_TOMP4          = 82,   /* MP4ת��ģ�� */
    COMP_BLOWFISH       = 83,   /* BLOWFISH�ӽ���ģ�� */
    COMP_RC4            = 84,   /* RC4�ӽ���ģ�� */
    COMP_MD5            = 85,   /* MD5����ģ�� */
    COMP_BASE64         = 86,   /* BASE64�ӽ���ģ�� */
    COMP_MFSBTREE       = 87,   /* MFS_BTREEģ�� */
    COMP_PROCCOMM       = 88,   /* PROC_COMMģ�� */
    COMP_MFSLINK        = 89,   /* MFS_LINKģ�� */
    COMP_MFSFCLK        = 90,   /* MFS_FCLKģ�� */
    COMP_MFSFILE        = 91,   /* MFS_FILEģ�� */
    COMP_MFSNFS         = 92,   /* MFS_NFSģ�� */
    COMP_UPNP           = 93,   /* UPNPģ�� */
    COMP_TEST           = 94,   /* ����ģ�� */
    COMP_IPDEV          = 95,   /* IPDEVģ�� */
    COMP_RTSP           = 96,   /* RTSPģ�� */
    COMP_PNP            = 97,   /* IPC���弴�� */
    COMP_FTPAPI         = 98,   /* FTPӦ�ýӿ�ģ�� */
    COMP_DHCP           = 99,   /* DHCP������ */
    COMP_POE            = 100,  /* POEģ�� */
    COMP_MFSMKFS        = 101,  /* MFS_MKFSģ�� */
    COMP_FLASHLOG       = 102,  /* дFLASHLOGģ�� */
    COMP_VPN            = 103,  /* VPNģ�� */
    COMP_SSTRING        = 104,  /* SSTRINGģ�� */
    COMP_THREADPOOL     = 105,  /* THREADPOOLģ�� */
    COMP_MEMPOOL        = 106,  /* MEMPOOLģ�� */
    COMP_DIAGNOSIS      = 107,  /* �������ģ�� */
    COMP_RTMP           = 108,  /* RTMPģ�� */
    COMP_ISAPI          = 109,  /* ISAPIģ�� */
    COMP_AUDIOCFG       = 110,  /* ��Ƶ����ģ�������ģ�� */
    COMP_RTSPSRV        = 111,  /* RTSP SERVERģ��*/
    COMP_SDKAUDIOPRO    = 112,  /* ��Ƶ����ģ��,��Ҫ������,�Խ���Ƶ */
    COMP_POECTRL        = 113,  /* POE���ʿ���ģ�� */
    COMP_NDCMP          = 114,  /* �豸��������ģ�� */
    COMP_SSOCKET        = 115,  /* �����׽��ֻ���ģ�� */
    COMP_NDCMPSRV       = 116,  /* �豸�������������ģ�� */
    COMP_DANALE         = 117,  /* ���ô�͸���� */
    COMP_MAX
} COMPID, *PCOMPID;

/* ģ�����ƶ��� */
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
    "ALARM",        /* ����ģ�� */
    "MSG",          /* 30 ��Ϣ���� */
    "SEM",          /* ���̼��źŵ� */
    "SHM",          /* ���̼乲���ڴ� */
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
    "CDRAPP",       /* 50 ��¼ģ�� */
    "PCIVMSG", 
    "VIDEOCFG", 
    "ATMI",         /* ATM����ģ�� */
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
    "BLOWFISH",     /* BLOWFISH�ӽ���ģ�� */
    "RC4",          /* RC4�ӽ���ģ�� */
    "MD5",          /* MD5����ģ�� */
    "BASE64",       /* BASE64�ӽ���ģ�� */
    "MFSBTREE",     /* MFS_BTREEģ�� */
    "PROCCOMM",     /* PROC_COMMģ�� */
    "MFSLINK",      /* MFS_LINKģ�� */
    "MFSFCLK",      /* MFS_FCLKģ�� */
    "MFSFILE",      /* MFS_FILEģ�� */
    "MFSNFS",       /* MFS_NFSģ�� */
    "UPNP",         /* UPNPģ�� */
    "TEST",         /* ����ģ�� */
    "IPDEV",        /* IPDEVģ�� */
    "RTSP",         /* RTSPģ�� */
    "PNP",          /* IPC���弴�� */
    "FTPAPI",       /* FTPӦ�ýӿ�ģ�� */
    "DHCP",         /* DHCP������ */
    "POE",          /* POEģ�� */
    "MFSMKFS",      /* MFS_MKFSģ�� */
    "FLASHLOG",     /* дFLASHLOGģ�� */
    "VPN",          /* VPNģ�� */
    "SSTRING",      /* SSTRINGģ�� */
    "THREADPOOL",   /* THREADPOOLģ�� */
    "MEMPOOL",      /* MEMPOOLģ�� */
    "DIAGNOSIS",    /* �������ģ�� */
    "RTMP",         /* RTMPģ�� */
    "ISAPI",        /* ISAPIģ�� */
    "AUDIOCFG",     /* ��Ƶ����ģ�������ģ�� */
    "RTSPSRV",      /* RTSP SERVERģ�� */
    "SDKAUDIOPRO",  /* ��Ƶ����ģ��,��Ҫ������,�Խ���Ƶ */
    "POECTRL",      /* POE���ʿ��� */
    "NDCMP",        /* �豸��������ģ�� */
    "SSOCKET",      /* �����׽��ֻ���ģ�� */
    "NDCMPSRV",     /* �豸�������������ģ�� */
    "DANALE",       /* ���ô�͸���� */
    NULL            /* �ַ������� */
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
    sint32 pb, pb_net;  /* �ط�ģ����(����,����)*/
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
    sint32 blowfish;    /* BLOWFISH�ӽ���ģ�� */
    sint32 rc4;         /* RC4�ӽ���ģ�� */
    sint32 md5;         /* MD5����ģ�� */
    sint32 base64;      /* BASE64�ӽ���ģ�� */
    sint32 mfsbtree;    /* MFS_BTREEģ�� */
    sint32 proccomm;    /* PROC_COMMģ�� */
    sint32 mfslink;     /* MFS_LINKģ�� */
    sint32 mfsfclk;     /* MFS_FCLKģ�� */
    sint32 mfsfile;     /* MFS_FILEģ�� */
    sint32 mfsnfs;      /* MFS_NFSģ�� */
    sint32 upnp;        /* UPNPģ�� */
    sint32 test;        /* ����ģ�� */
    sint32 ipdev;       /* IPDEVģ�� */
    sint32 rtsp;        /* RTSPģ�� */
    sint32 pnp;         /* IPC���弴�� */
    sint32 ftpapi;      /* FTPӦ�ýӿ�ģ�� */
    sint32 dhcp;        /* DHCP������ */
    sint32 poe;         /* POEģ�� */
    sint32 mfsmkfs;     /* MFS_MKFSģ�� */
    sint32 flashlog;    /* дFLASHLOGģ�� */
    sint32 vpn;         /* VPNģ�� */
    sint32 sstring;     /* SSTRINGģ�� */
    sint32 threadpool;  /* THREADPOOLģ�� */
    sint32 mempool;     /* MEMPOOLģ�� */
	sint32 diagnosis;   /* �������ģ�� */
    sint32 rtmp;        /* RTMPģ�� */
    sint32 isapi;       /* ISAPIģ�� */
    sint32 audiocfg;    /* ��Ƶ����ģ�������ģ�� */
    sint32 rtspsrv;     /* rtsp serverģ�� */
    sint32 sdkaudiopro; /* ��Ƶ����ģ��,��Ҫ������,�Խ���Ƶ */
    sint32 poectrl;     /* POE���ʿ��� */
    sint32 ndcmp;       /* �豸��������ģ�� */
    sint32 ssocket;     /* �����׽��ֻ���ģ�� */
    sint32 ndcmpsrv;    /* �豸�������������ģ�� */
    sint32 danale;      /* ���ô�͸���� */
} COMPFD, *PCOMPFD;

COMPFD sys_handle;
#endif /* NOUSE_COMPFD */

/******************************************************************************
 * ������붨��: ����Ϊsint32,�����������Ǹ���,�������λ��1,32λ��������: 
 *  +-+-----+-------+---------------+---------------+---------------+
 *  |1|0 9 8|7 6 5 4|3 2 1 0 9 8 7 6|5 4 3 2 1 0 9 8|7 6 5 4 3 2 1 0|
 *  +-+-----+-------+---------------+---------------+---------------+
 *  |1|LEVEL|   R   |    COMPID     |            ERRORNO            |
 *  +-+-----+-------+---------------+---------------+---------------+
 * ����: LEVEL:���󼶱�,R:����λ,COMPID:ģ���,ERRORNO:�����
 * �û�ģ�����Ŵ�32��ʼ����,�����ش���ʱ,����ERRNO(errno, compid)��ʽ
 * ����������ö�ٶ���,��ʽΪCOMP_ERROR_XXX,ȫ����д,��ģ�鰴�˱�׼����
 *****************************************************************************/
#define ERRNO_L(level, error, id)   (0x80000000 | ((level) << 28) | ((id) << 16) | (error))

/*
 * ���󼶱�
 * HB_ERROR_LEVEL_MODULE: ����ĳһģ��������Ϳ��Խ��������
 * HB_ERROR_LEVEL_SYSTEM: ����Ӧ�����ϵͳ�����ſ��Խ��������
 * HB_ERROR_LEVEL_OS: ���������ſ��Խ��������
 */
#define HB_ERROR_LEVEL_MODULE   0   /* ģ�鼶����� */
#define HB_ERROR_LEVEL_SYSTEM   1   /* ϵͳ������� */
#define HB_ERROR_LEVEL_OS       2   /* ����ϵͳ������� */

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
    COMMON_ERROR_HANDLEINVAL = 1,       /* ��������ַǷ� */
    COMMON_ERROR_HANDLEOPEND,           /* ����Ѵ� */
    COMMON_ERROR_NOCMD,                 /* ������� */
    COMMON_ERROR_PARAM,                 /* �������� */
    COMMON_ERROR_NOMEM,                 /* û���㹻�ڴ� */
    COMMON_ERROR_THREADSTART,           /* �߳��������� */
    COMMON_ERROR_THREADSTARTED,         /* �߳������� */
    COMMON_ERROR_SETTHREADNAME,         /* �����߳������� */
    COMMON_ERROR_SETPTHREADAFFINITY,    /* �����߳��������Դ��� */
    COMMON_ERROR_FILE_OPEN,             /* ���ļ����� */
    COMP_ERROR_START = 32,              /* �û�ģ����ʼ����� */
} COMMON_ERROR_E;

/******************************************************************************
 * ���������: ����ģ��xxx_ioctrl(..., cmd, ...)��cmd�Ķ���
 * CMD����0~31Ϊ��������,32~255����ģ���Զ���
 * �����������ö�ٶ���,��ʽΪCOMP_CMD_XXX,ȫ����д,��ģ�鰴�˱�׼����
 *****************************************************************************/
#define CMDNO(cmd, id)  (((id) << 16) | (cmd))

typedef enum
{
    COMMON_CMD_GETVERSION = CMDNO(0, COMP_COMMON),  /* ��ȡģ��汾 */
    COMMON_CMD_RESET,       /* ��λ��ǰ��� */
    COMMON_CMD_GETHEALTHY,  /* ��ȡ�������״��,param(sint32 *),��λ��ʾ��ͨ��״̬(0-����,1-����) */
    COMMON_CMD_GETPID,      /* ��ȡ����߳�ID,param(sint32 *) */
    COMMON_CMD_GETPROCINFO, /* ��ȡ��ģ���������Ϣ */
} COMMON_CMD_E;

typedef struct
{
    uint64 usec_next;       /* ��һ��ִ��ʱ��,��λ΢��(���ݵ�ǰʱ���usec_interval�����ʱ��,��ʼ��ʱΪ0) */
    uint32 usec_interval;   /* ��Ҫ��ʱ����,��λ΢��(0-����USLEEP,��0-������ʱ��������Ӧ������) */
    uint32 reserved;        /* ����λ */
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
 * ��������: ���ݴ���ص�
 * �������: idx: ͨ����
 *           fifo: ���ݻ���FIFO
 *           data: ���ݽ��ջ�����
 *           size: ���ݽ��ջ�������С
 * �������: ��
 * ����ֵ  : sint32
 * ע: ��fifoΪ0ʱ,���ݴ洢��data��,size��ʾ���ݳ���
 *     ��fifo��Ϊ0ʱ,���ݴ洢��fifo��,data��ʾ�û���������ַ,size��ʾ����������
 *     ��fifo,data��Ϊ��ʱ,��ʾ��������Ҫ����,Ҳ�����ݴ���������
 *****************************************************************************/
typedef sint32 (*datacallback)(sint32 idx, sint32 fifo, char *data, sint32 size);

/******************************************************************************
 * ��������: ��Ϣ֪ͨ�ص�
 * �������: wparam: ����
 *           lparam: ����
 * �������: ��
 * ����ֵ  : ��
 *****************************************************************************/
typedef void (*notifycallback)(sint32 wparam, sint32 lparam);

#ifndef _WIN32
/******************************************************************************
 * ��������: �߳���������
 * �������: priority: �߳����ȼ�(0-��Ĭ��ֵ����,�������ȼ�����,1~99-�߳����ȼ���)
 *           func: �߳�ִ�к�����
 *           wparam: ��������
 * �������: pid: �߳�ID
 * ����ֵ  : 0-�ɹ�,<0-�������
 *****************************************************************************/
sint32 pthread_start(sint32 *pid, sint32 priority, notifycallback func, sint32 wparam);

/******************************************************************************
 * ��������: ��һ����ʽ�����߳�����(���Ȳ�����MAX_THREAD_NAME_LEN���ַ�)
 * �������: fmt: ���ָ�ʽ
 * �������: ��
 * ����ֵ  : >=0-�ɹ�,<0-�������
 *****************************************************************************/
sint32 pthread_set_thread_name(const char *fmt, ...);

/******************************************************************************
 * ��������: ���ƽ̨�̰߳�ָ��CPU��������,�󶨵�CPU�˿����Ƕ��
 * �������: pid: �̺߳�PID(SET_CURRENT_THREAD_PID-��ʾ���õ�ǰ�߳�,�����ڲ����Զ���ȡ��ǰ�̺߳ź�����)
 *           cpuid_mask: CPU���ID����,��λ��ʾ(CPUID_MASK_E)
 * �������: ��
 * ����ֵ  : 0-�ɹ�,<0-�������
 *****************************************************************************/
sint32 pthread_set_affinity_np(pthread_t pid, CPUID_MASK_E cpuid_mask);
#endif /* _WIN32 */

/******************************************************************************
 * ��������: ������·�������
 * �������: year: ��
 *           month: �·�
 * �������: ��
 * ����ֵ  : ��ǰ�·ݵ�����
 *****************************************************************************/
sint32 time_dayofmonth(uint16 year, uint8 month);

/******************************************************************************
 * ��������: ��������ڵ�������
 * �������: year: ��
 *           month: �·�
 *           day: ����
 * �������: ��
 * ����ֵ  : 1~7: ����һ~������
 *****************************************************************************/
sint32 time_weekofdate(uint16 year, uint8 month, uint8 day);

/******************************************************************************
 * ��������: ������1970.1.1 00:00:00����ǰ����ʱ���������,��������
 * �������: sys_tm: ϵͳʱ�䶨��ṹ��
 * �������: ��
 * ����ֵ  : ��1970.1.1 00:00:00�����ڵ�������
 *****************************************************************************/
time_t time_time2sec(SYSTIME sys_tm);

/******************************************************************************
 * ��������: ϵͳʱ����������ϵͳʱ��ת��
 * �������: sec: ϵͳʱ��������
 *           psystm: ϵͳʱ��ṹ��ָ��
 * �������: psystm: ϵͳʱ��ṹ��ָ��,ϵͳʱ��
 * ����ֵ  : 0-�ɹ�,<0-�������
 *****************************************************************************/
uint32 time_sec2time(uint32 sec, PSYSTIME psystm);

#ifdef _WIN32
sint32 gettimeofday(struct timeval *tp, sint32 *tzp);

#else
/******************************************************************************
 * ��������: ��RTCͬ��ϵͳ��ǰʱ��(��δʵ��)
 * �������: ��
 * �������: ��
 * ����ֵ  : 0-�ɹ�,<0-ʧ��
 *****************************************************************************/
sint32 time_utcfromrtc(void);

/******************************************************************************
 * ��������: ��ϵͳ��ǰʱ��ͬ��RTC(��δʵ��)
 * �������: ��
 * �������: ��
 * ����ֵ  : 0-�ɹ�,<0-ʧ��
 *****************************************************************************/
sint32 time_utctortc(void);

/******************************************************************************
 * ��������: ��ȡϵͳ��ǰ�ĵ���ʱ��
 * �������: ��
 * �������: time
 * ����ֵ  : >0-�ɹ�(1~7:�ֱ��ʾ����һ��������),<0-ʧ��
 *****************************************************************************/
sint32 time_get(PSYSTIME psys_tm);

/******************************************************************************
 * ��������: ����ϵͳ��ǰ�ĵ���ʱ��,��δͬ��RTC
 * �������: time
 * �������: ��
 * ����ֵ  : 0-�ɹ�,<0-ʧ��
 *****************************************************************************/
sint32 time_set(PSYSTIME psys_tm);

/******************************************************************************
 * ��������: ��ȡϵͳ����ʱ��,��ϵͳ�ϵ羭��ʱ��
 * �������: pup_tm_sec: ϵͳ�ϵ羭��ʱ������
 *           pup_tm_msec: ϵͳ�ϵ羭��ʱ�������
 * �������: pup_tm_sec: ϵͳ�ϵ羭��ʱ������
 *           pup_tm_msec: ϵͳ�ϵ羭��ʱ�������
 * ����ֵ  : 0-�ɹ�,<0-ʧ��
 *****************************************************************************/
sint32 uptime_get(sint32 *pup_tm_sec, sint32 *pup_tm_msec);

/******************************************************************************
 * ��������: ����ǰ�����κ�������,�������ִ��ʱ��,��λ����
 * �������: pzero_init: ֵΪ0ʱ,��ȡ��ʼֵ(��ϵͳ�ϵ羭��ʱ��),����ֵΪ0
 * �������: pusr_tm: user time
 *           psys_tm: sys time
 * ����ֵ  : ���ε��õ�ʱ����real time
 *****************************************************************************/
sint32 runtime_get_msec(sint32 *pzero_init, sint32 *pusr_tm, sint32 *psys_tm);

/******************************************************************************
 * ��������: ����ǰ������ִ�е�ϵͳʱ��ͼ��ʱ����ϵͳ΢�뼶������
 * �������: ptm_interval
 * �������: ��
 * ����ֵ  : 0-�ɹ�,<0-ʧ��
 *****************************************************************************/
sint32 time_interval_usleep(PTIME_INTERVAL_S ptm_interval);

/******************************************************************************
 * ��������: SWVERSION����ʱ������(ȡ��ǰϵͳʱ��),�汾����Ҫ�ֶ��������,�ٸ���ģ��ID�Ŵ�ӡ����
 * �������: compid: ģ��ID��
 *           pswver: ģ������汾�ṹ��ָ��
 * �������: pswver: ģ������汾�ṹ��
 * ����ֵ  : ��
 *****************************************************************************/
void swver_set(COMPID compid, PSWVERSION pswver);

/******************************************************************************
 * ��������: ����Ŀ���Դ�ַ������ȿ���Դ�ַ�����Ŀ���ַ�����,����ȡ��С��
 * �������: dst: Ŀ���ַ���ָ��
 *           src: Դ�ַ���ָ��
 *           dst_len: Ŀ���ַ�������
 * �������: dst: Ŀ���ַ���
 * ����ֵ  : >=0-��ʾ�����ַ�������
 *****************************************************************************/
size_t strl_cpy(char *dst, const char *src, size_t dst_len);

/******************************************************************************
 * ��������: ����16���ƴ�ӡ
 * �������: p: Ŀ���ַ���ָ��
 *           s: Դ�ַ���ָ��
 * ����ֵ  : ��
 *****************************************************************************/
void data_hex_dump(char *p, sint32 s);

/******************************************************************************
 * ��������: ��ȡMTD�豸������Ϣ
 * �������: pdevs_attr: MTD�豸���Խṹ��ָ��
 * �������: pdevs_attr: MTD�豸���Խṹ��ָ��,MTD�豸����
 * ����ֵ  : >0-�ɹ�(1~7:�ֱ��ʾ����һ��������),<0-ʧ��
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
