/******************************************************************************
 * 模块名称: COMMON-SSTRING字符串安全处理模块
 * 修改记录: 2014-07-04 V1.0.0 完成基本功能
 *  1. 2014-07-04 V1.0.0
 *  1). 
 *****************************************************************************/
#include <string.h>

#include "sstring.h"

#define COMP_NENT   COMP_SSTRING

#ifndef _WIN32
/******************************************************************************
 * 函数介绍: 忽略大小写比较字符串,用来比较参数ps1和ps2字符串,比较时会自动忽略大小写的差异
 * 输入参数: ps1 - 字符串指针
 *           ps2 - 字符串指针
 * 输出参数: 无
 * 返回值  : 0 - 若参数ps1和ps2字符串相同或错误;>0 - ps1长度大于ps2长度;<0 - ps1长度若小于ps2长度
 *****************************************************************************/
sint32 sstrcasecmp(const char *ps1, const char *ps2)
{
    if (ps1 && ps2)
    {
        return strcasecmp(ps1, ps2);
    }
    TRACE_REL("ps1 or ps2 is NULL\n");

    return 0;
}

/******************************************************************************
 * 函数介绍: 忽略大小写比较字符串,用来比较参数ps1和ps2字符串前n个字符,
 *           比较时会自动忽略大小写的差异
 * 输入参数: ps1 - 字符串指针
 *           ps2 - 字符串指针
 *           n - 字符个数
 * 输出参数: 无
 * 返回值  : 0 - 若参数ps1和ps2字符串相同或错误;>0 - ps1长度大于ps2长度;<0 - ps1长度若小于ps2长度
 *****************************************************************************/
sint32 sstrncasecmp(const char *ps1, const char *ps2, sint32 n)
{
    if (ps1 && ps2 && (n > 0))
    {
        return strncasecmp(ps1, ps2, n);
    }
    TRACE_REL("ps1 or ps2 is NULL, or n(%d) <= 0\n", n);

    return 0;
}
#endif /* _WIN32 */

/******************************************************************************
 * 函数介绍: 连接两字符串,将参数psrc字符串拷贝到参数pdst所指的字符串尾;如果参数pdst所
 *           指的内存空间不够大,psrc的内容将被截取,并将pdst最后一个字符填充'\0'
 * 输入参数: pdst - 字符串指针
 *           psrc - 字符串指针
 * 输出参数: 无
 * 返回值  : NULL - 错误,否则 - 参数pdst的字符串起始地址
 *****************************************************************************/
char *sstrcat(char *pdst, const char *psrc)
{
    if (pdst && psrc)
    {
        return strcat(pdst, psrc);
    }
    TRACE_REL("pdst or psrc is NULL\n");

    return pdst;
}

/******************************************************************************
 * 函数介绍: 连接两字符串,将参数psrc字符串拷贝到参数pdst所指的字符串尾;如果参数pdst所
 *           指的内存空间不够大,psrc的内容将被截取,并将pdst最后一个字符填充'\0'
 * 输入参数: pdst - 字符串指针
 *           psrc - 字符串指针
 *           dst_size - 目标字符串空间大小
 * 输出参数: 无
 * 返回值  : NULL - 错误,否则 - 参数pdst的字符串起始地址
 *****************************************************************************/
char *slstrcat(char *pdst, const char *psrc, sint32 dst_size)
{
    sint32 dst_len = 0, src_len = 0, cat_len = 0;
    
    if (pdst && psrc && (dst_size > 0))
    {
        dst_len = strnlen(pdst, dst_size);
        src_len = strnlen(psrc, dst_size - 1);
        if ((cat_len = IMIN(dst_size - dst_len - 1, src_len)) >= 0)
        {
            pdst[dst_len + cat_len] = '\0';
            TRACE_REL("cat_len(%d) dst_size(%d) dst_len(%d) src_len(%d)\n", 
                cat_len, dst_size, dst_len, src_len);
            return strncat(pdst, psrc, cat_len);
        }
        TRACE_ERR("cat_len(%d) dst_size(%d) dst_len(%d) src_len(%d)\n", 
            cat_len, dst_size, dst_len, src_len);
    }
    TRACE_REL("pdst or psrc is NULL, or dst_size(%d) <= 0\n", dst_size);

    return pdst;
}

/******************************************************************************
 * 函数介绍: 连接两字符串,会将参数psrc字符串拷贝n个字符到参数pdst所指的字符串尾;
 *           如果参数pdst所指的内存空间不够大,psrc的内容将被截取,并将pdst最后一个字符填充'\0'
 * 输入参数: pdst - 字符串指针
 *           psrc - 字符串指针
 *           n - 字符个数
 * 输出参数: 无
 * 返回值  : NULL - 错误,否则 - 参数pdst的字符串起始地址
 *****************************************************************************/
char *sstrncat(char *pdst, const char *psrc, sint32 n)
{
    if (pdst && psrc && (n > 0))
    {
        return strncat(pdst, psrc, n);
    }
    TRACE_REL("pdst or psrc is NULL, or n(%d) <= 0\n", n);

    return pdst;
}

/******************************************************************************
 * 函数介绍: 连接两字符串,将参数psrc字符串拷贝到参数pdst所指的字符串尾;如果参数pdst所
 *           指的内存空间不够大,psrc的内容将被截取,并将pdst最后一个字符填充'\0'
 * 输入参数: pdst - 字符串指针
 *           psrc - 字符串指针
 *           dst_size - 目标字符串空间大小
 *           n - 字符个数
 * 输出参数: 无
 * 返回值  : NULL - 错误,否则 - 参数pdst的字符串起始地址
 *****************************************************************************/
char *slstrncat(char *pdst, const char *psrc, sint32 dst_size, sint32 n)
{
    sint32 dst_len = 0, src_len = 0, cat_len = 0;
    
    if (pdst && psrc && (dst_size > 0) && (n > 0))
    {
        dst_len = strnlen(pdst, dst_size);
        src_len = strnlen(psrc, n);
        if ((cat_len = IMIN(dst_size - dst_len - 1, src_len)) >= 0)
        {
            pdst[dst_len + cat_len] = '\0';
            TRACE_REL("cat_len(%d) dst_size(%d) dst_len(%d) src_len(%d) n(%d)\n", 
                cat_len, dst_size, dst_len, src_len, n);
            return strncat(pdst, psrc, cat_len);
        }
        TRACE_ERR("cat_len(%d) dst_size(%d) dst_len(%d) src_len(%d) n(%d)\n", 
            cat_len, dst_size, dst_len, src_len, n);
    }
    TRACE_REL("pdst or psrc is NULL, or dst_size(%d) <= 0, or n(%d) <= 0\n", dst_size, n);

    return pdst;
}

/******************************************************************************
 * 函数介绍: 查找字符串中第一个出现的指定字符,用来找出参数ps字符串中第一个
 *           出现的参数c地址,然后将该字符出现的地址返回
 * 输入参数: ps - 字符串指针
 *           c - 字符
 * 输出参数: 无
 * 返回值  : NULL - 未找到指定的字符或错误,否则 - 找到指定的字符则返回该字符所在地址
 *****************************************************************************/
char *sstrchr(const char *ps, sint32 c)
{
    if (ps)
    {
        return strchr(ps, c);
    }
    TRACE_REL("ps is NULL\n");

    return NULL;
}

/******************************************************************************
 * 函数介绍: 在一字符串中查找指定的字符串,从字符串phaystack中搜寻字符串pneedle,并将第一次出现的地址返回
 * 输入参数: phaystack - 字符串指针
 *           pneedle - 字符串指针
 * 输出参数: 无
 * 返回值  : NULL - 未找到指定的字符串或错误,否则 - 找到指定的字符串则返回该字符串所在地址
 *****************************************************************************/
char *sstrstr(const char *phaystack, const char *pneedle)
{
    if (phaystack && pneedle)
    {
        return strstr(phaystack, pneedle);
    }
    TRACE_REL("phaystack or pneedle is NULL\n");

    return NULL;
}

/******************************************************************************
 * 函数介绍: 用来将字符串分割成一个个片段,参数ps指向欲分割的字符串,参数pdelim则为分割字符串,
 *           当在参数ps的字符串中发现到参数pdelim的分割字符时则会将该字符改为\0 字符;
 *           在第一次调用时,必需给予参数ps字符串,往后的调用则将参数ps设置成NULL
 * 输入参数: ps - 字符串指针
 *           pdelim - 字符串指针
 * 输出参数: 无
 * 返回值  : NULL - 已无从分割或错误,否则 - 下一个分割后的字符串指针
 *****************************************************************************/
char *sstrtok(char *ps, const char *pdelim)
{
    if (pdelim)
    {
        return strtok(ps, pdelim);
    }
    TRACE_ERR("ps or pdelim is NULL\n");

    return NULL;
}

/******************************************************************************
 * 函数介绍: 比较字符串,用来比较参数ps1和ps2字符串;字符串大小的比较是以ASCII
 *           码表上的顺序来决定,此顺序亦为字符的值;首先将ps1第一个字符值减去
 *           ps2第一个字符值;若差值为0则再继续比较下个字符,若差值不为0则将差值返回
 * 输入参数: ps1 - 字符串指针
 *           ps2 - 字符串指针
 * 输出参数: 无
 * 返回值  : 0 - ps1等于ps2或错误;>0 - ps1大于ps2; <0 - ps1小于ps2
 *****************************************************************************/
sint32 sstrcmp(const char *ps1, const char *ps2)
{
    if (ps1 && ps2)
    {
        return strcmp(ps1, ps2);
    }
    TRACE_REL("ps1 or ps2 is NULL\n");

    return 0;
}

/******************************************************************************
 * 函数介绍: 比较字符串,用来比较参数ps1和ps2字符串;字符串大小的比较是以ASCII
 *           码表上的顺序来决定,此顺序亦为字符的值;首先将ps1第一个字符值减去
 *           ps2第一个字符值;若差值为0则再继续比较下个字符,若差值不为0则将差值返回
 * 输入参数: ps1 - 字符串指针
 *           ps2 - 字符串指针
 *           n - 字符个数
 * 输出参数: 无
 * 返回值  : 0 - ps1等于ps2或错误;>0 - ps1大于ps2; <0 - ps1小于ps2
 *****************************************************************************/
sint32 sstrncmp(const char *ps1, const char *ps2, sint32 n)
{
    if (ps1 && ps2 && n > 0)
    {
        return strncmp(ps1, ps2, n);
    }
    TRACE_REL("ps1 or ps2 is NULL, or n(%d) <= 0\n", n);

    return 0;
}

/******************************************************************************
 * 函数介绍: 采用目前区域的字符排列次序来比较字符串,会依环境变量LC_COLLATE
 *           所指定的文字排列次序来比较ps1和ps2字符串;若LC_COLLATE为"POSIX'或"C",
 *           则此函数与sstrcmp作用完全相同
 * 输入参数: ps1 - 字符串指针
 *           ps2 - 字符串指针
 * 输出参数: 无
 * 返回值  : 0 - ps1等于ps2;>0 - ps1大于ps2; <0 - ps1小于ps2
 *****************************************************************************/
sint32 sstrcoll(const char *ps1, const char *ps2)
{
    if (ps1 && ps2)
    {
        return strcoll(ps1, ps2);
    }
    TRACE_REL("ps1 or ps2 is NULL\n");

    return 0;
}

/******************************************************************************
 * 函数介绍: 拷贝字符串,将参数psrc字符串拷贝至参数pdst所指的地址;如果参数pdst所
 *           指的内存空间不够大,psrc的内容将被截取,并将pdst最后一个字符填充'\0'
 * 输入参数: pdst - 字符串指针
 *           psrc - 字符串指针
 * 输出参数: 无
 * 返回值  : NULL - 错误,否则 - 参数pdst的字符串起始地址
 *****************************************************************************/
char *sstrcpy(char *pdst, const char *psrc)
{
    if (pdst && psrc)
    {
        return strcpy(pdst, psrc);
    }
    TRACE_REL("pdst or psrc is NULL\n");

    return pdst;
}

/******************************************************************************
 * 函数介绍: 拷贝字符串,将参数psrc字符串拷贝至参数pdst所指的地址;如果参数pdst所
 *           指的内存空间不够大,psrc的内容将被截取,并将pdst最后一个字符填充'\0'
 * 输入参数: pdst - 字符串指针
 *           psrc - 字符串指针
 *           dst_size - 目标字符串空间大小
 * 输出参数: 无
 * 返回值  : NULL - 错误,否则 - 参数pdst的字符串起始地址
 *****************************************************************************/
char *slstrcpy(char *pdst, const char *psrc, sint32 dst_size)
{
    sint32 cpy_len = 0;
    
    if (pdst && psrc && (dst_size > 0))
    {
        if ((cpy_len = strnlen(psrc, dst_size - 1)) >= 0)
        {
            pdst[cpy_len] = '\0';
            TRACE_REL("cpy_len(%d) dst_size(%d)\n", cpy_len, dst_size);
            return strncpy(pdst, psrc, cpy_len);
        }
        TRACE_ERR("cpy_len(%d) dst_size(%d)\n", cpy_len, dst_size);
    }
    TRACE_REL("pdst or psrc is NULL, or dst_size(%d) <= 0\n", dst_size);

    return pdst;
}

/******************************************************************************
 * 函数介绍: 拷贝字符串,将参数psrc字符串拷贝至参数pdst所指的地址;如果参数pdst所
 *           指的内存空间不够大,psrc的内容将被截取,并将pdst最后一个字符填充'\0'
 * 输入参数: pdst - 字符串指针
 *           psrc - 字符串指针
 *           n - 字符个数
 * 输出参数: 无
 * 返回值  : NULL - 错误,否则 - 参数pdst的字符串起始地址
 *****************************************************************************/
char *sstrncpy(char *pdst, const char *psrc, sint32 n)
{
    if (pdst && psrc && (n > 0))
    {
        return strncpy(pdst, psrc, n);
    }
    TRACE_REL("pdst or psrc is NULL, or n(%d) < 0\n", n);

    return pdst;
}

/******************************************************************************
 * 函数介绍: 拷贝字符串,将参数psrc字符串拷贝至参数pdst所指的地址;如果参数pdst所
 *           指的内存空间不够大,psrc的内容将被截取,并将pdst最后一个字符填充'\0'
 * 输入参数: pdst - 字符串指针
 *           psrc - 字符串指针
 *           dst_size - 目标字符串空间大小
 *           n - 字符个数
 * 输出参数: 无
 * 返回值  : NULL - 错误,否则 - 参数pdst的字符串起始地址
 *****************************************************************************/
char *slstrncpy(char *pdst, const char *psrc, sint32 dst_size, sint32 n)
{
    sint32 cpy_len = 0;
    
    if (pdst && psrc && (dst_size > 0) && (n > 0))
    {
        if ((cpy_len = IMIN(dst_size - 1, strnlen(psrc, n))) >= 0)
        {
            pdst[cpy_len] = '\0';
            TRACE_REL("cpy_len(%d) dst_size(%d) n(%d)\n", cpy_len, dst_size, n);
            return strncpy(pdst, psrc, cpy_len);
        }
        TRACE_ERR("cpy_len(%d) dst_size(%d) n(%d)\n", cpy_len, dst_size, n);
    }
    TRACE_REL("pdst or psrc is NULL, or dst_size(%d) <= 0 or n(%d) <= 0\n", dst_size, n);

    return pdst;
}

/******************************************************************************
 * 函数介绍: 返回字符串中连续不含指定字符串内容的字符数,从参数ps字符串的开头计
 *           算连续的字符,而这些字符都完全不在参数preject所指的字符串中;
 *           简单地说,若此函数返回的数值为n,则代表字符串ps开头连续有n个字符
 *           都不含字符串preject内的字符
 * 输入参数: ps - 字符串指针
 *           preject - 字符串指针
 * 输出参数: 无
 * 返回值  : >=0 - 字符串ps开头连续不含字符串preject内的字符数目,<0 - 错误号
 *****************************************************************************/
sint32 sstrcspn(const char *ps, const char *preject)
{
    if (ps && preject)
    {
        return strcspn(ps, preject);
    }
    TRACE_REL("ps or preject is NULL\n");

    return 0;
}

/******************************************************************************
 * 函数介绍: 复制字符串,先用maolloc配置与参数ps字符串相同的空间大小,然后将参数
 *           ps字符串的内容复制到该内存地址,然后把该地址返回;该地址最后需要用
 *           free来释放,否则将造成内存泄露
 * 输入参数: ps - 字符串指针
 * 输出参数: 无
 * 返回值  : NULL - 错误,可能是内存不足,否则 - 字符串指针,该指针指向复制后的新字符串地址
 *****************************************************************************/
char *sstrdup(const char *ps)
{
    if (ps)
    {
        return _strdup(ps);
    }
    TRACE_REL("ps is NULL\n");

    return NULL;
}

/******************************************************************************
 * 函数介绍: 返回字符串长度,用来计算指定的字符串s的长度,不包括结束字符"\0"
 * 输入参数: ps - 字符串指针
 * 输出参数: 无
 * 返回值  : >=0 - 字符串ps的字符数,<0 - 错误号
 *****************************************************************************/
sint32 sstrlen(const char *ps)
{
    if (ps)
    {
        return strlen(ps);
    }
    TRACE_REL("ps is NULL\n");

    return 0;
}

/******************************************************************************
 * 函数介绍: 返回字符串长度,用来计算指定的字符串s的长度,不包括结束字符"\0"
 * 输入参数: ps - 字符串指针
 *           n - 字符个数
 * 输出参数: 无
 * 返回值  : >=0 - 字符串ps的字符数,<0 - 错误号
 *****************************************************************************/
sint32 sstrnlen(const char *ps, sint32 n)
{
    if (ps && (n > 0))
    {
        return strnlen(ps, n);
    }
    TRACE_REL("ps is NULL, or n(%d) < 0\n", n);

    return 0;
}
