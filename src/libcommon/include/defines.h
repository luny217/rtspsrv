#ifndef _DEFINES_H_
#define _DEFINES_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define ALL_CHANNEL (-1)    /* 参数获取和设置时,用该值表示全部通道 */
#define IPC_PARAM   0666

#define ONE_MSEC_PER_USEC   1000
#define ONE_SEC_PER_MSEC    1000
#define ONE_SEC_PER_USEC    (ONE_MSEC_PER_USEC * ONE_SEC_PER_MSEC)

/* 数据类型定义 */
typedef signed char         sint8;
typedef signed short        sint16;
typedef signed int          sint32;
typedef signed long         slng32;
typedef signed long long    sint64;
typedef unsigned char       uint8;
typedef unsigned short      uint16;
typedef unsigned int        uint32;
typedef unsigned long       ulng32;
typedef unsigned long long  uint64;
typedef enum
{
    HB_FALSE= 0, 
    HB_TRUE = 1
} HB_BOOL;

#define UMIN(a, b)      ((a) < (b) ? (a) : (b)) /* a <= b */
#define IMIN(a, b)      UMIN(a, b)
#define IMAX(a, b)      ((a) > (b) ? (a) : (b)) /* a > b */
#define CLIP(x, a, b)   ((x) < (a) ? (a) : ((x) > (b) ? (b) : (x))) /* a <= x <= b */
#define UCLIPI(x, b)    CLIP(x, 0, b)
#define IZERO(a, b)     ((a) ? (b) : 0)     /* a为0则0,否则为b */
#define MUX(x, a, b)    ((x) ? (a) : (b))   /* x不为0则为a,为0则为b */
#define LGTR(a, b)      ((a) > (b))         /* 左大于右*/
#define LGER(a, b)      ((a) >= (b))        /* 左大于等于右*/
#define LLTR(a, b)      ((a) < (b))         /* 左小于右 */
#define LLER(a, b)      ((a) <= (b))        /* 左小于等于右 */
#define LNER(a, b)      ((a) != (b))        /* 左不等于右 */
#define LEQR(a, b)      ((a) == (b))        /* 左等于右 */

#define BIT32(x)        (1 << (x))
#define BIT64(x)        (1ULL << (x))

#define SET_BIT32(x, n) ((x) | BIT32(n))        /* 设置数字某一位为1 */
#define GET_BIT32(x, n) (((x) >> (n)) & 0x1)    /* 获取数字某一位的值(0或1) */
#define CLR_BIT32(x, n) ((x) & ~(BIT32(n)))     /* 清楚数字某一位为0 */

#define IP4(a)  ((a) & 0xFF)
#define IP3(a)  (((a) >> 8) & 0xFF)
#define IP2(a)  (((a) >> 16) & 0xFF)
#define IP1(a)  (((a) >> 24) & 0xFF)

#define sleep_ms(ms)    usleep((ms) * 1000)
#define sleep_us(us)    usleep(us)

#define UBYTESEL(x,b)               (((x) >> ((b) << 3)) & 0xFF)
#define PACKBYTES(high, low)        ((((high) << 8) | (low)) & 0xFFFF)
#define PACK16LSB(a, b)             ((((a) << 16) & 0xFFFF0000) | ((b) & 0x0000FFFF))
#define U8TOU32(u24, u16, u8, u0)   ((((u24) & 0xFF) << 24) | (((u16) & 0xFF) << 16) | (((u8) & 0xFF) << 8) | ((u0) & 0xFF))
#define BCD2DEC(x)                  (((x) >> 4) & 0xF) * 10 + ((x) & 0xF)
#define DEC2BCD(x)                  (((x) / 10) << 4) | ((x) % 10)
#define PACKCOLOR(r, g, b)          ((((b) >> 3) & 0x1F) | (((g) << 2) & 0x3E0) | (((r) << 7) & 0x7C00))
#define ALIGN_N(x, align)           (((x) + ((align) - 1)) & ~((align) - 1))
#define CLOSE_ALIGN_BIT32(x, bit)   MUX((x) & BIT32(bit - 1), ((x) + (BIT32(bit) - 1)) & ~(BIT32(bit) - 1), (x) & ~(BIT32(bit) - 1))

#define BYTE_TO_KB(x)   ((x) >> 10)
#define BYTE_TO_MB(x)   ((x) >> 20)
#define KB_TO_BYTE(x)   ((x) << 10)
#define MB_TO_BYTE(x)   ((x) << 20)

/* returns the n'th least significant byte */
#define _GET_BYTE(x, n)     (((x) >> (8 * (n))) & 0xFF)
#define _PED_SWAP16(x)      ((_GET_BYTE(x, 0) << 8)	+ (_GET_BYTE(x, 1) << 0))
#define _PED_SWAP32(x)      ((_GET_BYTE(x, 0) << 24) + (_GET_BYTE(x, 1) << 16) \
    + (_GET_BYTE(x, 2) << 8) + (_GET_BYTE(x, 3) << 0))
#define _PED_SWAP64(x)      ((_GET_BYTE(x, 0) << 56) + (_GET_BYTE(x, 1) << 48) \
    + (_GET_BYTE(x, 2) << 40) + (_GET_BYTE(x, 3) << 32)	\
    + (_GET_BYTE(x, 4) << 24) + (_GET_BYTE(x, 5) << 16)	\
    + (_GET_BYTE(x, 6) << 8)  + (_GET_BYTE(x, 7) << 0))
#define PED_SWAP16(x)       ((uint16)_PED_SWAP16((uint16)(x)))
#define PED_SWAP32(x)       ((uint32)_PED_SWAP32((uint32)(x)))
#define PED_SWAP64(x)       ((uint64)_PED_SWAP64((uint64)(x)))

#ifdef WORDS_BIGENDIAN
#define PED_CPU_TO_LE16(x)  PED_SWAP16(x)
#define PED_CPU_TO_BE16(x)  (x)
#define PED_CPU_TO_LE32(x)  PED_SWAP32(x)
#define PED_CPU_TO_BE32(x)  (x)
#define PED_CPU_TO_LE64(x)  PED_SWAP64(x)
#define PED_CPU_TO_BE64(x)  (x)
#define PED_LE16_TO_CPU(x)  PED_SWAP16(x)
#define PED_BE16_TO_CPU(x)  (x)
#define PED_LE32_TO_CPU(x)  PED_SWAP32(x)
#define PED_BE32_TO_CPU(x)  (x)
#define PED_LE64_TO_CPU(x)  PED_SWAP64(x)
#define PED_BE64_TO_CPU(x)  (x)
#else /* !WORDS_BIGENDIAN */
#define PED_CPU_TO_LE16(x)  (x)
#define PED_CPU_TO_BE16(x)  PED_SWAP16(x)
#define PED_CPU_TO_LE32(x)  (x)
#define PED_CPU_TO_BE32(x)  PED_SWAP32(x)
#define PED_CPU_TO_LE64(x)  (x)
#define PED_CPU_TO_BE64(x)  PED_SWAP64(x)
#define PED_LE16_TO_CPU(x)  (x)
#define PED_BE16_TO_CPU(x)  PED_SWAP16(x)
#define PED_LE32_TO_CPU(x)  (x)
#define PED_BE32_TO_CPU(x)  PED_SWAP32(x)
#define PED_LE64_TO_CPU(x)  (x)
#define PED_BE64_TO_CPU(x)  PED_SWAP64(x)
#endif /* !WORDS_BIGENDIAN */

#define GET16(p)        (uint16)((((uint8 *)(p))[1] << 8) + (((uint8 *)(p))[0]))
#define GET32(p)        (uint32)((((uint8 *)(p))[3] << 24 ) + (((uint8 *)(p))[2] << 16) + \
    (((uint8 *)(p))[1] << 8) + (((uint8 *)(p))[0]))
#define SET16(p, v)     ((uint8 *)(p))[0] = ((v)) & 0xFF; ((uint8 *)(p))[1] = ((v) >> 8) & 0xFF
#define SET32(p, v)     ((uint8 *)(p))[0] = ((v)) & 0xFF; ((uint8 *)(p))[1] = ((v) >> 8) & 0xFF; \
    ((uint8 *)(p))[2] = ((v) >> 16) & 0xFF; ((uint8 *)(p))[3] = ((v) >> 24) & 0xFF
#define U64HICASTU32(a) ((uint64)(a) >> 32)
#define U64LOCASTU32(a) ((uint64)(a) & 0xFFFFFFFF)

#define HB_ASSERT_SIZEOF_STRUCT(s, n)           typedef char assert_sizeof_struct_##s[MUX(sizeof(struct s) == (n), 1, -1)]
#define HB_ASSERT_SIZEOF_TYPEDEF_STRUCT(s, n)   typedef char assert_sizeof_typedef_struct_##s[MUX(sizeof(s) == (n), 1, -1)]

#define MACRO_STRINGIFY(s)  #s
#define XMACRO_STRINGIFY(s) MACRO_STRINGIFY(s)
#ifdef SRC_REVISION
#define GET_SRC_REVISION(module_name)   do { char ver[128] = {0};\
    sprintf(ver, "%s\n", XMACRO_STRINGIFY(module_name)" : "XMACRO_STRINGIFY(SRC_REVISION)" @ "XMACRO_STRINGIFY(__DATE__)","XMACRO_STRINGIFY(__TIME__)); } while(0)
#else
#define GET_SRC_REVISION(module_name)
#endif /* SVN_VERSION_STRING */

#ifdef __GNUC__
#define MASK_NO_USED_WARNING    __attribute__((unused)) /* 屏蔽变量或函数未使用告警,慎重使用 */
/* 基础类型原子操作,在多线程情况共享下全局量用这几个宏来算术运算,以保证结果正确,或者也可加互斥锁 */
#define INCREMENT_ATOMIC(var, add_val)  __sync_add_and_fetch(&(var), add_val)
#define DECREMENT_ATOMIC(var, sub_val)  __sync_sub_and_fetch(&(var), sub_val)
#define OR_ATOMIC(var, or_val)          __sync_or_and_fetch(&(var), or_val)
#define AND_ATOMIC(var, and_val)        __sync_and_and_fetch(&(var), and_val)
#define XOR_ATOMIC(var, xor_val)        __sync_xor_and_fetch(&(var), xor_val)
#define NAND_ATOMIC(var, nand_val)      __sync_nand_and_fetch(&(var), nand_val) /* var = ~(var & nand_val)*/
#elif defined(_WIN32)
#define MASK_NO_USED_WARNING 
#define INCREMENT_ATOMIC(var, add_val)  InterlockedExchangeAdd((long volatile*)&(var), add_val)
#define DECREMENT_ATOMIC(var, sub_val)  InterlockedExchangeAdd((long volatile*)&(var), -sub_val)
#define OR_ATOMIC(var, or_val)          InterlockedOr((long volatile*)&(var), or_val)
#define AND_ATOMIC(var, and_val)        InterlockedAnd((long volatile*)(&(var), and_val)
#define XOR_ATOMIC(var, xor_val)        InterlockedXor((long volatile*)(&(var), xor_val)
#define NAND_ATOMIC(var, nand_val)      InterlockedExchange((long volatile*)(&(var), ~(var & nand_val)) /* var = ~(var & nand_val)*/
#else
#define MASK_NO_USED_WARNING
/* 需要使用对应的实现 */
#define INCREMENT_ATOMIC(var, add_val)  __sync_add_and_fetch(&(var), add_val)
#define DECREMENT_ATOMIC(var, sub_val)  __sync_sub_and_fetch(&(var), sub_val)
#define OR_ATOMIC(var, or_val)          __sync_or_and_fetch(&(var), or_val)
#define AND_ATOMIC(var, and_val)        __sync_and_and_fetch(&(var), and_val)
#define XOR_ATOMIC(var, xor_val)        __sync_xor_and_fetch(&(var), xor_val)
#define NAND_ATOMIC(var, nand_val)      __sync_nand_and_fetch(&(var), nand_val) /* var = ~(var & nand_val)*/
#endif /* __GNUC__ */

/******************************************************************************
 * 修改提示字符颜色,对应颜色表 
 * none         = "\033[0m" 
 * black        = "\033[0;30m"
 * dark_gray    = "\033[1;30m"
 * blue         = "\033[0;34m"
 * light_blue   = "\033[1;34m"
 * green        = "\033[0;32m"
 * light_green -= "\033[1;32m"
 * cyan         = "\033[0;36m"
 * light_cyan   = "\033[1;36m"
 * red          = "\033[0;31m"
 * light_red    = "\033[1;31m"
 * purple       = "\033[0;35m"
 * light_purple = "\033[1;35m"
 * brown        = "\033[0;33m"
 * yellow       = "\033[1;33m"
 * light_gray   = "\033[0;37m"
 * white        = "\033[1;37m"
 *****************************************************************************/
#define COLOR_STR_NONE          "\033[0m"
#define COLOR_STR_TWINKLE       "\033[5m"
#define COLOR_STR_BLACK         "\033[0;30m"
#define COLOR_STR_LIGHT_GRAY    "\033[0;37m"
#define COLOR_STR_DARK_GRAY     "\033[1;30m"
#define COLOR_STR_BLUE          "\033[0;34m"
#define COLOR_STR_LIGHT_BLUE    "\033[1;34m"
#define COLOR_STR_GREEN         "\033[0;32m"
#define COLOR_STR_LIGHT_GREEN   "\033[1;32m"
#define COLOR_STR_CYAN          "\033[0;36m"
#define COLOR_STR_LIGHT_CYAN    "\033[1;36m"
#define COLOR_STR_RED           "\033[0;31m"
#define COLOR_STR_LIGHT_RED     "\033[1;31m"
#define COLOR_STR_PURPLE        "\033[0;35m"
#define COLOR_STR_LIGHT_PURPLE  "\033[1;35m"
#define COLOR_STR_BROWN         "\033[0;33m"
#define COLOR_STR_YELLOW        "\033[1;33m"
#define COLOR_STR_WHITE         "\033[1;37m"

#define TIME_STR                "[%04d-%02d-%02d %02d:%02d:%02d]"
#define SYSTIME4TM_FMT(systime) (systime).year + 2000, (systime).month, (systime).day, \
    (systime).hour, (systime).min, (systime).sec

#define GET_DATE_YEAR   ((((__DATE__[7] - '0') * 10 + (__DATE__[8] - '0')) * 10 + \
    (__DATE__[9] - '0')) * 10 + (__DATE__[10] - '0'))
#define GET_DATE_MONTH  (__DATE__[2] == 'n' ? (__DATE__[1] == 'a' ? 0 : 5) : \
    __DATE__[2] == 'b' ? 1 : __DATE__[2] == 'r' ? (__DATE__[0] == 'M' ? 2 : 3) : \
    __DATE__[2] == 'y' ? 4 : __DATE__[2] == 'l' ? 6 : \
    __DATE__[2] == 'g' ? 7 : __DATE__[2] == 'p' ? 8 : \
    __DATE__[2] == 't' ? 9 : __DATE__[2] == 'v' ? 10 : 11)
#define GET_DATE_DAY    ((__DATE__[4] == ' ' ? 0 : __DATE__[4] - '0') * 10 + (__DATE__[5] - '0'))

#if 0
typedef struct
{
    sint32 x, y, w, h;
} RECT_S, *PRECT_S;

typedef struct
{
    sint32 x, y;
} POINT_S, *PPOINT_S;

typedef struct
{
    uint32 w;
    uint32 h;
} SIZE_S, *PSIZE_S;
#endif

typedef enum
{
    THREAD_IS_EXIT = 0x00,  /* 线程退出 */
    THREAD_IS_RUNNING,      /* 线程运行 */
    THREAD_IS_STOP,         /* 线程停止 */
} THREAD_STAT_E;

typedef enum
{
    BUFF_SIZE_32  = 32,   /* 字符串长度  32字节 */
    BUFF_SIZE_64  = 64,   /* 字符串长度  64字节 */
    BUFF_SIZE_128 = 128,  /* 字符串长度 128字节 */
    BUFF_SIZE_256 = 256,  /* 字符串长度 256字节 */
    BUFF_SIZE_512 = 512,  /* 字符串长度 512字节 */
    BUFF_SIZE_1024= 1024, /* 字符串长度1024字节 */
} BUFF_SIZE_E;

/* 模块软件版本定义 */
typedef struct
{
    uint32 minver : 6;  /* 0~63 */
    uint32 secver : 6;  /* 0~63 */
    uint32 majver : 5;  /* 0~31 */
    uint32 day : 5;     /* 日: 1~31 */
    uint32 month : 4;   /* 月: 1~12 */
    uint32 year : 6;    /* 年: 0~63 */
} SWVERSION, *PSWVERSION;
#define SWVERSION_LEN   sizeof(SWVERSION)

#define VINFO2STR(str, swver)   sprintf(str, "V%d.%d.%d build%04d%02d%02d\n", \
    ((SWVERSION)swver).majver, ((SWVERSION)swver).secver, ((SWVERSION)swver).minver, \
    ((SWVERSION)swver).year + 2000, ((SWVERSION)swver).month, ((SWVERSION)swver).day)

/* 系统时间定义 */
typedef struct
{
    uint32 sec : 6;     /* 秒: 0~63 */
    uint32 min : 6;     /* 分: 0~63 */
    uint32 hour : 5;    /* 时: 0~23 */
    uint32 day : 5;     /* 日: 1~31 */
    uint32 month : 4;   /* 月: 1~12 */
    uint32 year : 6;    /* 年: 0~63 */
} SYSTIME, *PSYSTIME;
#define SYSTIME_LEN sizeof(SYSTIME)

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _DEFINES_H_ */
