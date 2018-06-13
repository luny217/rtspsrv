/******************************************************************************
 * ģ������: COMMON-COMMONģ��
 * �޸ļ�¼: 2010-03-17 V1.0.0 ��������,ȥ������Ϣ(��ѧΰ)
 *  1. 2010-08-26 V1.0.1
 *  1). common.h�����ATM����ģ�����ģ��ID,ģ������ģ��������
 *  2. 2010-09-19 V1.0.2
 *  1). �޸ĺ���pthread_start����priority����0ʱ,�����ȼ�����,С�ڵ���0ʱ,��Ĭ�����ȼ�����
 *  3. 2012-04-18 V1.0.3
 *  1). ���Ӻ���pthread_set_thread_name�ӿ������߳���
 *  4. 2012-05-24 V1.0.4
 *  1). �رպ���pthread_start���ȼ�(���԰汾)
 *  5. 2012-05-31 V1.0.5
 *  1). �޸ĺ���pthread_start���ȷ�ʽSCHED_FIFOģʽΪSCHED_RRģʽ
 *  6. 2012-06-08 V1.0.6
 *  1). �޸ĺ���time_get�ڲ�ʵ�ַ�ʽ,��ͨ������localtimeʵ�ָ�Ϊ����gmtimeʵ��,����localtimeʱû��ʱ���ĸ������������
 *  7. 2012-08-31 V1.0.7
 *  1). �Ż�����time_get�ڲ�ʵ�ַ�ʽ,��ֹ��ȡʱ�����������
 *  8. 2012-08-31 V1.0.8
 *  1). ��Ӻ���time_interval_usleep֧��
 *  9. 2012-10-08 V1.0.9
 *  1). �رպ���pthread_start���ȼ�
 * 10. 2013-01-30 V1.1.0
 *  1). ��Ӻ���strl_cpy֧��
 * 11. 2013-02-26 V1.1.1
 *  1). �޸ĺ���time_interval_usleep���㲻׼������
 * 12. 2013-04-26 V1.2.0
 *  1). ��Ӻ���time_time2sec��time_sec2time�ӿ�֧��
 * 13. 2013-04-26 V1.3.1
 *  1). �޸ĺ���runtime_get_msec�淶��
 * 14. 2014-06-05 V1.4.0
 *  1). ��Ӻ���pthread_set_affinity_np֧��
 *****************************************************************************/
#include <errno.h>
#ifndef _WIN32
#include <sys/prctl.h>
#include <sys/time.h>
#include <sys/times.h>
#include <pthread.h>
#endif /* _WIN32 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "sstring.h"

#define COMP_NENT   COMP_COMMON
//#define USE_PTHREAD_PRIORITY
#ifdef USE_PTHREAD_PRIORITY
#define PTREAD_SCHED    SCHED_FIFO /* SCHED_FIFO SCHED_RR */
#endif /* USE_PTHREAD_PRIORITY */

#define TIME_INIT_FLAG  0x454D4954  /* "TIME" */
#if defined(PLATFORM_AMBARELLA) || defined(PLATFORM_MONTAVISTA)
#define TIME_USLEEP     (ONE_MSEC_PER_USEC * 10)
#define TIME_SUB_USEC   (ONE_MSEC_PER_USEC * 0)
#else
#define TIME_USLEEP     (ONE_MSEC_PER_USEC * 0)
#define TIME_SUB_USEC   (ONE_MSEC_PER_USEC * 10)
#endif /* PLATFORM_AMBARELLA || PLATFORM_MONTAVISTA */

/* �����ж�,�ܱ�4�����Ҳ��ܱ�100����,�����ܱ�400���� */
#define RUNYEAR(x)          (((((x) & 3) == 0) & (((x) % 100) != 0)) | (((x) % 400) == 0))
/* ����������������ڼ�, 1�º�2����Ϊ13�º�14�������� */
#define DAY2WEEK(y, m, d)   (((d) + 2 * (m) + 3 * ((m) + 1) / 5 + (y) + (y) / 4 - (y) / 100 + (y) / 400) % 7)

#define PROC_CMDLINE_MAX_LEN                512
#define PROC_CMDLINE_FILE_PATH              "/proc/cmdline"
#define PROC_CMDLINE_MTDPARTS_FLAG          "mtdparts"
#define PROC_CMDLINE_DELIM_STR              ":,"
#define PROC_CMDLINE_SIZE_NAME_DELIM_STR    "()"

/* ������ĸ������������� */
static uint8 day_table[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static sint32 SYS_CLK_TCK_PER_SEC = 0;
static const unsigned char base64_table[65] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

#ifndef _WIN32
static sint32 _common_sysconf(void)
{
    if (SYS_CLK_TCK_PER_SEC <= 0)
    {
        if ((SYS_CLK_TCK_PER_SEC = sysconf(_SC_CLK_TCK)) <= 0) /* Get clock ticks/second */
        {
            TRACE_ERR("SYS_CLK_TCK_PER_SEC %d, %s\n", SYS_CLK_TCK_PER_SEC, strerror(errno));
            return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
        }
    }

    return 0;
}

/******************************************************************************
 * ��������: �߳���������
 * �������: priority: �߳����ȼ�(0-��Ĭ��ֵ����,�������ȼ�����,1~99-�߳����ȼ���)
 *           func: �߳�ִ�к�����
 *           wparam: ��������
 * �������: pid: �߳�ID
 * ����ֵ  : 0-�ɹ�,<0-�������
 *****************************************************************************/
sint32 pthread_start(sint32 *pid, sint32 priority, notifycallback func, sint32 wparam)
{
    sint32 rtval = 0;
    
#ifdef USE_PTHREAD_PRIORITY
    if (priority > 0)
    {
        sint32 min_prio = sched_get_priority_min(PTREAD_SCHED);
        sint32 max_prio = sched_get_priority_max(PTREAD_SCHED);
        struct sched_param param = {MUX(priority < min_prio, min_prio, MUX(priority > max_prio, max_prio, priority))};
        pthread_attr_t attr;

        pthread_attr_init(&attr);
        pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED); /* ���̳и��߳����� */
        pthread_attr_setschedpolicy(&attr, PTREAD_SCHED); /* ���õ��Ȳ��� */
        pthread_attr_setschedparam(&attr, &param); /* �������ȼ� */
        TRACE_DBG("minprio(%d) maxprio(%d) PTREAD_SCHED(%d)\n", min_prio, max_prio, PTREAD_SCHED);
        if ((rtval = pthread_create((pthread_t *)pid, &attr, (void *)func, (void *)wparam)))
        {
            pthread_attr_destroy(&attr);
            TRACE_ERR("pthread_create error: %d, %s\n", errno, strerror(errno));
            return ERRNO(COMMON_ERROR_THREADSTART, COMP_NENT);
        }
        pthread_attr_destroy(&attr);
    }
    else
    {
        if ((rtval = pthread_create((pthread_t *)pid, NULL, (void *)func, (void *)wparam)))
        {
            TRACE_ERR("pthread_create error: %d, %s\n", errno, strerror(errno));
            return ERRNO(COMMON_ERROR_THREADSTART, COMP_NENT);
        }
    }
#else
    if ((rtval = pthread_create((pthread_t *)pid, NULL, (void *)func, (void *)wparam)))
    {
        TRACE_ERR("pthread_create error: %d, %s\n", errno, strerror(errno));
        return ERRNO(COMMON_ERROR_THREADSTART, COMP_NENT);
    }
#endif /* USE_PTHREAD_PRIORITY */

    return 0;
}

/******************************************************************************
 * ��������: ��һ����ʽ�����߳�����(���Ȳ�����MAX_THREAD_NAME_LEN���ַ�)
 * �������: fmt: ���ָ�ʽ
 * �������: ��
 * ����ֵ  : 0-�ɹ�,<0-�������
 *****************************************************************************/
sint32 pthread_set_thread_name(const char *fmt, ...)
{
    va_list ap;
    char name[MAX_THREAD_NAME_LEN] = {0};
    sint32 rtval = 0;
    
    va_start(ap, fmt);
    vsnprintf(name, sizeof(name), fmt, ap);
    va_end(ap);
    if ((rtval = prctl(PR_SET_NAME, name)) < 0)
    {
        TRACE_ERR("prctl(PR_SET_NAME)[%s] error: %d, %s\n", name, errno, strerror(errno));
        return ERRNO(COMMON_ERROR_SETTHREADNAME, COMP_NENT);
    }

    return 0;
}

/******************************************************************************
 * ��������: ���ƽ̨�̰߳�ָ��CPU��������,�󶨵�CPU�˿����Ƕ��
 * �������: pid: �̺߳�PID(SET_CURRENT_THREAD_PID-��ʾ���õ�ǰ�߳�,�����ڲ����Զ���ȡ��ǰ�̺߳ź�����
 *           cpuid_mask: CPU���ID����,��λ��ʾ(CPUID_MASK_E)
 * �������: ��
 * ����ֵ  : 0-�ɹ�,<0-�������
 *****************************************************************************/
sint32 pthread_set_affinity_np(pthread_t pid, CPUID_MASK_E cpuid_mask)
{
    cpu_set_t cpu_set_mask;
    sint32 rtval = 0, cpuid = 0;
    slng32 cpunum = sysconf(_SC_NPROCESSORS_CONF);

    TRACE_DBG("_SC_NPROCESSORS_CONF(%ld) _SC_NPROCESSORS_ONLN(%ld)\n", 
        cpunum, sysconf(_SC_NPROCESSORS_ONLN));
    if (SET_CURRENT_THREAD_PID == pid)
    {
        pid = pthread_self();
        TRACE_DBG("set current thread(%ld) running cpuid(%d)\n", pid, cpuid);
    }
    CPU_ZERO(&cpu_set_mask);
    for (cpuid = 0; cpuid < cpunum; cpuid++)
    {
        if (cpuid_mask & BIT32(cpuid))
        {
            CPU_SET(cpuid, &cpu_set_mask);
        }
    }
    if ((rtval = pthread_setaffinity_np(pid, sizeof(cpu_set_mask), &cpu_set_mask)) < 0)
    {
        TRACE_ERR("pthread_setaffinity_np pid(%ld) cpuid_mask(0x%x) error: %d, %s\n", 
            pid, cpuid_mask, errno, strerror(errno));
        return ERRNO(COMMON_ERROR_SETPTHREADAFFINITY, COMP_NENT);
    }

    return 0;
}
#endif /* _WIN32 */

/******************************************************************************
 * ��������: ������·�������
 * �������: year: ��
 *           month: �·�
 * �������: ��
 * ����ֵ  : ��ǰ�·ݵ�����
 *****************************************************************************/
sint32 time_dayofmonth(uint16 year, uint8 month)
{
    /* ���·�Ϊ2ʱ,����������������һ�� */
    return day_table[month - 1] + ((month == 2) ? RUNYEAR(year) : 0);
}

/******************************************************************************
 * ��������: ��������ڵ�������
 * �������: year: ��
 *           month: �·�
 *           day: ����
 * �������: ��
 * ����ֵ  : 1~7: ����һ~������
 *****************************************************************************/
sint32 time_weekofdate(uint16 year, uint8 month, uint8 day)
{
#if 1
    sint32 i = 0, week = 0, year_count = 0, month_count = 0;

    /* ��������һ��������� */
    year--;
    year_count = year * 365 + ((year >> 2) - (year / 100) + (year / 400));

    /* ���㵱�����ϸ��µ������� */
    month--;
    for (i = 0; i < month; i++)
    {
        month_count += day_table[i];
    }

    /* �����������������7��������Ϊ���������� */
    year++;
    week = (year_count + month_count + day + ((month > 1) ? RUNYEAR(year) : 0)) % 7;
    return (week ? week : 7);
#else
    /* January and February are treated as month 13 and 14, respectively, from the year before. */
    if ((month == 1) || (month == 2))
    {
        month += 12;
        year--;
    }

    return (DAY2WEEK(year, month, day) + 1);
#endif
}

/******************************************************************************
 * ��������: ������1970.1.1 00:00:00����ǰ����ʱ���������,��������
 * �������: sys_tm: ϵͳʱ�䶨��ṹ��
 * �������: ��
 * ����ֵ  : ��1970.1.1 00:00:00�����ڵ�������
 *****************************************************************************/
time_t time_time2sec(SYSTIME sys_tm)
{
    sint16 year = sys_tm.year + 2000;
    sint8 month = sys_tm.month;
    
    if (0 >= (month -= 2))
    {
        /* 1..12 -> 3..10,11,12,1,2 */
        month += 12;  /* Puts Feb last since it has leap day */
        year -= 1;
    }

    return ((((time_t)(year/4 - year/100 + year/400 + 367*month/12 + sys_tm.day) 
        + year*365 - 719499)*24 + sys_tm.hour)*60 + sys_tm.min)*60 + sys_tm.sec; /* finally seconds */
}

/******************************************************************************
 * ��������: ϵͳʱ����������ϵͳʱ��ת��
 * �������: sec: ϵͳʱ��������
 *           psystm: ϵͳʱ��ṹ��ָ��
 * �������: psystm: ϵͳʱ��ṹ��ָ��,ϵͳʱ��
 * ����ֵ  : 0-�ɹ�,<0-�������
 *****************************************************************************/
uint32 time_sec2time(uint32 sec, PSYSTIME psystm)
{
    struct tm tm_tmp = {0};

#ifdef _WIN32
    time_t sec_tmp = sec;

    gmtime_s(&tm_tmp, &sec_tmp);
    printf("sec(%d) => %d-%02d-%02d %02d:%02d:%02d\n", sec, 
        tm_tmp.tm_year, tm_tmp.tm_mon, tm_tmp.tm_mday, tm_tmp.tm_hour, tm_tmp.tm_min, tm_tmp.tm_sec);
#else
    gmtime_r((void *)&sec, &tm_tmp);
#endif /* */
    psystm->year = tm_tmp.tm_year - 100;
    psystm->month = tm_tmp.tm_mon + 1;
    psystm->day = tm_tmp.tm_mday;
    psystm->hour = tm_tmp.tm_hour;
    psystm->min = tm_tmp.tm_min;
    psystm->sec = tm_tmp.tm_sec;

    return 0;
}

#ifdef _WIN32
sint32 gettimeofday(struct timeval *tp, sint32 *tzp)
{
    if (tp)
    {
        time_t time_now = 0;
        SYSTEMTIME wtm = {0};

        time(&time_now);
        GetLocalTime(&wtm);
        tp->tv_sec = (long)time_now;
        tp->tv_usec = wtm.wMilliseconds * 1000;
		/*printf("%d-%02d-%02d %02d:%02d:%02d msec(%d) sec(%d) usec(%d)\n",
			wtm.wYear, wtm.wMonth, wtm.wDay, wtm.wHour, wtm.wMinute,
			wtm.wSecond, wtm.wMilliseconds, tp->tv_sec, tp->tv_usec);*/
    }
    if (tzp)
    {
        long time_zone = 0;

        _get_timezone(&time_zone);
        *tzp = time_zone / 3600;
        printf("time_zone(%d-%d)\n", time_zone, *tzp);
    }

    return 0;
}
#else
/******************************************************************************
 * ��������: ��RTCͬ��ϵͳ��ǰʱ��(��δʵ��)
 * �������: ��
 * �������: ��
 * ����ֵ  : 0-�ɹ�,<0-ʧ��
 *****************************************************************************/
sint32 time_utcfromrtc(void)
{
    return 0;
}

/******************************************************************************
 * ��������: ��ϵͳ��ǰʱ��ͬ��RTC(��δʵ��)
 * �������: ��
 * �������: ��
 * ����ֵ  : 0-�ɹ�,<0-ʧ��
 *****************************************************************************/
sint32 time_utctortc(void)
{
    return 0;
}

/******************************************************************************
 * ��������: ��ȡϵͳ��ǰ�ĵ���ʱ��
 * �������: psys_tm: ϵͳʱ�䶨��ṹ��ָ��
 * �������: psys_tm: ϵͳʱ�䶨��ṹ��ָ��,ϵͳʱ��
 * ����ֵ  : >0-�ɹ�(1~7:�ֱ��ʾ����һ��������),<0-ʧ��
 *****************************************************************************/
sint32 time_get(PSYSTIME psys_tm)
{
    time_t time_tmp;
    struct tm tm_tmp, *ptm_tmp = NULL;

    time(&time_tmp), ptm_tmp = gmtime(&time_tmp), tm_tmp = *ptm_tmp;
    psys_tm->year = tm_tmp.tm_year - 100;
    psys_tm->month = tm_tmp.tm_mon + 1;
    psys_tm->day = tm_tmp.tm_mday;
    psys_tm->hour = tm_tmp.tm_hour;
    psys_tm->min = tm_tmp.tm_min;
    psys_tm->sec = tm_tmp.tm_sec;

    return (tm_tmp.tm_wday ? tm_tmp.tm_wday : 7);
}

/******************************************************************************
 * ��������: ����ϵͳ��ǰ�ĵ���ʱ��,��δͬ��RTC
 * �������: psys_tm: ϵͳʱ�䶨��ṹ��ָ��
 * �������: ��
 * ����ֵ  : 0-�ɹ�,<0-ʧ��
 *****************************************************************************/
sint32 time_set(PSYSTIME psys_tm)
{
    struct tm tp;
    struct timeval tv;
    struct timezone tz;

    tp.tm_year = psys_tm->year + 100;
    tp.tm_mon = psys_tm->month - 1;
    tp.tm_mday = psys_tm->day;
    tp.tm_hour = psys_tm->hour;
    tp.tm_min = psys_tm->min;
    tp.tm_sec = psys_tm->sec;
    gettimeofday(&tv, &tz);
    tv.tv_sec = mktime(&tp);
    settimeofday(&tv, &tz);

    return 0;
}

/******************************************************************************
 * ��������: ��ȡϵͳ����ʱ��,��ϵͳ�ϵ羭��ʱ��
 * �������: pup_tm_sec: ϵͳ�ϵ羭��ʱ������
 *           pup_tm_msec: ϵͳ�ϵ羭��ʱ�������
 * �������: pup_tm_sec: ϵͳ�ϵ羭��ʱ������
 *           pup_tm_msec: ϵͳ�ϵ羭��ʱ�������
 * ����ֵ  : >=0-�ɹ�,ϵͳ�ϵ羭��ʱ�������,<0-ʧ��
 *****************************************************************************/
sint32 uptime_get(sint32 *pup_tm_sec, sint32 *pup_tm_msec)
{
    struct tms tmstm;
    clock_t clk_tm = 0;
    sint32 rtval = 0, up_tm_sec = 0;

    if ((rtval = _common_sysconf()) < 0)
    {
        return rtval;
    }
    clk_tm = times(&tmstm);
    up_tm_sec = clk_tm / SYS_CLK_TCK_PER_SEC;
    if (pup_tm_sec)
    {
        *pup_tm_sec = up_tm_sec;
    }
    if (pup_tm_msec)
    {
        *pup_tm_msec = clk_tm % SYS_CLK_TCK_PER_SEC;
        if (*pup_tm_msec < 0)
        {
            *pup_tm_msec = -*pup_tm_msec;
        }
    }
    
    return clk_tm * (ONE_SEC_PER_MSEC / SYS_CLK_TCK_PER_SEC);
}

/******************************************************************************
 * ��������: ����ǰ�����κ�������,�������ִ��ʱ��,��λ����
 * �������: pzero_init: ֵΪ0ʱ,��ȡ��ʼֵ(��ϵͳ�ϵ羭��ʱ��),����ֵΪ0
 * �������: pusr_tm: user time
 *           psys_tm: sys time
 * ����ֵ  : ���ε��õ�ʱ����real time
 *****************************************************************************/
sint32 runtime_get_msec(sint32 *pzero_init, sint32 *pusr_tm, sint32 *psys_tm)
{
    struct tms tmstm;
    clock_t clk_tm = 0, clk_intlv = 0;
    sint32 rtval = 0;

    if ((NULL == pzero_init) && (NULL == pusr_tm) && (NULL == psys_tm))
    {
        return ERRNO(COMMON_ERROR_PARAM, COMP_NENT);
    }
    if ((rtval = _common_sysconf()) < 0)
    {
        return rtval;
    }
    clk_tm = times(&tmstm);
    if (pzero_init)
    {
        if (0 == *pzero_init)
        {
            if (pusr_tm)
            {
                *pusr_tm = tmstm.tms_utime;
            }
            if (psys_tm)
            {
                *psys_tm = tmstm.tms_stime;
            }
            *pzero_init = clk_tm;
            
            return 0;
        }
        else
        {
            clk_intlv = clk_tm - *pzero_init;
            if (pusr_tm)
            {
                *pusr_tm = (tmstm.tms_utime - *pusr_tm) * (1000 / SYS_CLK_TCK_PER_SEC);
            }
            if (psys_tm)
            {
                *psys_tm = (tmstm.tms_stime - *psys_tm) * (1000 / SYS_CLK_TCK_PER_SEC);
            }
        }
    }
    
    return (clk_intlv * (1000 / SYS_CLK_TCK_PER_SEC));
}

/******************************************************************************
 * ��������: ����ǰ������ִ�е�ϵͳʱ��ͼ��ʱ����ϵͳ΢�뼶������
 * �������: ptm_interval
 * �������: ��
 * ����ֵ  : 0-�ɹ�,<0-ʧ��
 *****************************************************************************/
sint32 time_interval_usleep(PTIME_INTERVAL_S ptm_interval)
{
    struct timeval tv_curt;
    uint64 usec_curt = 0;
    sint64 usec_temp = 0;
    
    gettimeofday(&tv_curt, NULL);
    usec_curt = ((uint64)tv_curt.tv_sec) * ONE_SEC_PER_USEC + (uint64)tv_curt.tv_usec;
    if (ptm_interval->usec_next > 0)
    {
        usec_temp = ptm_interval->usec_next - usec_curt;
        if (usec_temp < 0)
        {
            usleep(TIME_USLEEP); /* ��С��֤����10���� */
            ptm_interval->usec_next = usec_curt + ptm_interval->usec_interval;
        }
        else
        {
            if (usec_temp >= TIME_SUB_USEC)
            {
                usleep(usec_temp - TIME_SUB_USEC); /* ��С��֤����10���� */
            }
            else if ((usec_temp > (TIME_SUB_USEC / 2)) && (usec_temp < TIME_SUB_USEC))
            {
                usleep(TIME_USLEEP); /* ��С��֤����10���� */
            }
            ptm_interval->usec_next += ptm_interval->usec_interval;
        }
    }
    else /* ��һ��ִ��ʱ */
    {
        ptm_interval->usec_next = usec_curt + ptm_interval->usec_interval;
    }
    TRACE_DBG("tv_curt(%ld-%ld) usec_curt(%lld) usec_next(%lld) usec_temp(%lld)\n", 
        tv_curt.tv_sec, tv_curt.tv_usec, usec_curt, ptm_interval->usec_next, usec_temp);

    return 0;
}

/******************************************************************************
 * ��������: SWVERSION����ʱ������(ȡ��ǰϵͳʱ��),�汾����Ҫ�ֶ��������,�ٸ���ģ��ID�Ŵ�ӡ����
 * �������: compid: ģ��ID��
 *           pswver: ģ������汾�ṹ��ָ��
 * �������: pswver: ģ������汾�ṹ��
 * ����ֵ  : ��
 *****************************************************************************/
void swver_set(COMPID compid, PSWVERSION pswver)
{
    const char month_short[12][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    char month_get[12] = {0};
    sint32 year = 0, day = 0, i = 0;

    sscanf(__DATE__, "%s %d %d", month_get, &day, &year);
    pswver->year = year - 2000;
    pswver->day = day;
    for (i = 0; i < 12; i++)
    {
        if (0 == strncmp(&month_short[i][0], month_get, 4))
        {
            pswver->month = i + 1;
            break;
        }
    }
    TRACE_SWVER(compid, *pswver);
}

/******************************************************************************
 * ��������: ����Ŀ���Դ�ַ������ȿ���Դ�ַ�����Ŀ���ַ�����,����ȡ��С��
 * �������: dst: Ŀ���ַ���ָ��
 *           src: Դ�ַ���ָ��
 *           dst_len: Ŀ���ַ�������
 * �������: dst: Ŀ���ַ���
 * ����ֵ  : >=0-��ʾ�����ַ�������   
 *****************************************************************************/
size_t strl_cpy(char *dst, const char *src, size_t dst_len)
{
    size_t src_len = strlen(src);

    if (dst_len > 0)
    {
        if (src_len < dst_len)
        {
            strcpy(dst, src);
        }
        else
        {
            src_len = dst_len - 1;
            strncpy(dst, src, src_len);
            dst[src_len] = 0;
        }
        
        return src_len;
    }
    
    return 0;
}



/******************************************************************************
 * ��������: ��ȡMTD�豸������Ϣ
 * �������: pdevs_attr: MTD�豸���Խṹ��ָ��
 * �������: pdevs_attr: MTD�豸���Խṹ��ָ��,MTD�豸����
 * ����ֵ  : >0-�ɹ�(1~7:�ֱ��ʾ����һ��������),<0-ʧ��
 *****************************************************************************/
sint32 get_mtd_devs_attr(PMTD_DEVS_ATTR_S pdevs_attr)
{
    FILE *proc_fd = NULL;
    sint32 rtval = 0, size_name_nums = 0, i = 0, j = 0, k = 0;
    char temp_buff[PROC_CMDLINE_MAX_LEN], *ptemp = NULL;
    char size[MAX_MTD_DEV_NUMS][MAX_MTD_DEV_NAME_LEN] = {{0}};
    char size_name[MAX_MTD_DEV_NUMS][MAX_MTD_DEV_NAME_LEN] = {{0}};

    if (NULL == (proc_fd = fopen(PROC_CMDLINE_FILE_PATH, "r")))
    {
        TRACE_ERR("fopen[%s] error: %d, %s\n", PROC_CMDLINE_FILE_PATH, errno, strerror(errno));
        return ERRNO(COMMON_ERROR_FILE_OPEN, COMP_NENT);
    }
    while (NULL != fgets(temp_buff, PROC_CMDLINE_MAX_LEN, proc_fd))
    {   /* mem=500M console=ttyAMA0,115200 root=/dev/ram0 initrd=0x81000000,17000000
         * mtdparts=hi_sfc:512k(m-boot),3M(kernel);hinand:16M(rootfs),64M(hb0),64M(hb1),112M(hb2) */
        char *phaystack = sstrstr(temp_buff, PROC_CMDLINE_MTDPARTS_FLAG);

        TRACE_DBG("temp_buff(%d) %s", rtval, temp_buff);
        TRACE_DBG("phaystack %s", phaystack);
        if ((phaystack))
        {
            ptemp = sstrtok(phaystack, PROC_CMDLINE_DELIM_STR);
            TRACE_DBG("ptemp[%s]\n", ptemp);
            while ((ptemp = sstrtok(NULL, PROC_CMDLINE_DELIM_STR)))
            {
                TRACE_DBG("ptemp[%s]\n", ptemp);
                sstrcpy(size_name[size_name_nums++], ptemp);
            }
            pdevs_attr->dev_nums = size_name_nums;
            TRACE_DBG("dev_nums(%d)\n", pdevs_attr->dev_nums);
            for (i = 0; i < size_name_nums; i++)
            {
                ptemp = sstrtok(size_name[i], PROC_CMDLINE_SIZE_NAME_DELIM_STR);
                sstrcpy(size[i], ptemp);
                ptemp = sstrtok(NULL, PROC_CMDLINE_SIZE_NAME_DELIM_STR);
                sstrcpy(pdevs_attr->attr[i].name, ptemp);
                for (j = 0, k = sstrnlen(size[i], 16); j < k; j++)
                {
                    switch (size[i][j])
                    {
                    case 'k':
                    case 'K':
                        pdevs_attr->attr[i].size = KB_TO_BYTE(atoi(size[i]));
                        break;
                    case 'm':
                    case 'M':
                        pdevs_attr->attr[i].size = MB_TO_BYTE(atoi(size[i]));
                        break;
                    default:
                        break;
                    }
                    if (pdevs_attr->attr[i].size)
                    {
                        pdevs_attr->attr[i].free = pdevs_attr->attr[i].size;
                        break;
                    }
                }
                TRACE_DBG("size[%s-%d] free(%d) name[%s]\n", size[i], pdevs_attr->attr[i].size, 
                    pdevs_attr->attr[i].free, pdevs_attr->attr[i].name);
            }
        }
        else
        {
            TRACE_ERR("No find PROC_CMDLINE_MTDPARTS_FLAG[%s]\n", PROC_CMDLINE_MTDPARTS_FLAG);
            rtval = ERRNO(COMMON_ERROR_FILE_OPEN, COMP_NENT);
        }
    }
    fclose(proc_fd);

    return rtval;
}
#endif /* _WIN32 */

/******************************************************************************
* ��������: ����16���ƴ�ӡ
* �������: p: Ŀ���ַ���ָ��
*           s: Դ�ַ���ָ��
* ����ֵ  : ��
*****************************************************************************/
void data_hex_dump(char *p, sint32 s)
{
#define DATA_HEX_DUMP_COUNT    16
	if (s > 0)
	{
		sint32 i = 0, dump_nums = s / DATA_HEX_DUMP_COUNT;
		sint32 j = 0, dump_count = s % DATA_HEX_DUMP_COUNT;

		if (dump_nums)
		{
			for (; i < dump_nums; i++)
			{
				for (j = 0; j < DATA_HEX_DUMP_COUNT; j++)
				{
					TRACE_ALWAYS("%02x ", p[i * DATA_HEX_DUMP_COUNT + j]);
				}
				TRACE_ALWAYS("\n");
			}
		}
		for (j = 0; j < dump_count; j++)
		{
			TRACE_ALWAYS("%02x ", p[i * DATA_HEX_DUMP_COUNT + j]);
		}
		TRACE_ALWAYS("\n");
	}
}

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
unsigned char *base64_encode(const unsigned char *src, size_t len, size_t *out_len)
{
	unsigned char *out, *pos;
	const unsigned char *end, *in;
	size_t olen;
	int line_len;

	olen = len * 4 / 3 + 4; /* 3-byte blocks to 4-byte */
	olen += olen / 72; /* line feeds */
	olen++; /* nul termination */
	if (olen < len)
		return NULL; /* integer overflow */
	out = malloc(olen);
	if (out == NULL)
		return NULL;

	end = src + len;
	in = src;
	pos = out;
	line_len = 0;
	while (end - in >= 3) {
		*pos++ = base64_table[in[0] >> 2];
		*pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
		*pos++ = base64_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
		*pos++ = base64_table[in[2] & 0x3f];
		in += 3;
		line_len += 4;
		if (line_len >= 72) {
			*pos++ = '\n';
			line_len = 0;
		}
	}

	if (end - in) {
		*pos++ = base64_table[in[0] >> 2];
		if (end - in == 1) {
			*pos++ = base64_table[(in[0] & 0x03) << 4];
			*pos++ = '=';
		} else {
			*pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
			*pos++ = base64_table[(in[1] & 0x0f) << 2];
		}
		*pos++ = '=';
		line_len += 4;
	}

	if (line_len)
		*pos++ = '\n';

	*pos = '\0';
	if (out_len)
		*out_len = pos - out;

	return out;
}

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
unsigned char *base64_decode(const unsigned char *src, size_t len, size_t *out_len)
{
	unsigned char dtable[256], *out, *pos, in[4], block[4], tmp;
	size_t i, count, olen;

	memset(dtable, 0x80, 256);
	for (i = 0; i < sizeof(base64_table) - 1; i++)
		dtable[base64_table[i]] = (unsigned char)i;
	dtable['='] = 0;

	count = 0;
	for (i = 0; i < len; i++) {
		if (dtable[src[i]] != 0x80)
			count++;
	}

	if (count == 0 || count % 4)
		return NULL;

	olen = count / 4 * 3;
	pos = out = malloc(olen);
	if (out == NULL)
		return NULL;

	count = 0;
	for (i = 0; i < len; i++) {
		tmp = dtable[src[i]];
		if (tmp == 0x80)
			continue;

		in[count] = src[i];
		block[count] = tmp;
		count++;
		if (count == 4) {
			*pos++ = (block[0] << 2) | (block[1] >> 4);
			*pos++ = (block[1] << 4) | (block[2] >> 2);
			*pos++ = (block[2] << 6) | block[3];
			count = 0;
		}
	}

	if (pos > out) {
		if (in[2] == '=')
			pos -= 2;
		else if (in[3] == '=')
			pos--;
	}

    if (out_len)
	    *out_len = pos - out;

	return out;
}
