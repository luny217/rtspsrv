include ../../../Makefile.param

CFLAGS += -DHAVE_AV_CONFIG_H -I./src -I./src/libavcodec -I./src/libavformat -I./src/libavutil -I./src/librtspsrv -I../../../tool/cyassl

TARGET_LIB = librtspsrv.a

SRCS =	src/libavutil/avutil_avstring.c \
				src/libavutil/avutil_mathematics.c \
				src/libavutil/avutil_random_seed.c \
				src/libavutil/avutil_time.c \
				src/libavutil/avutil_utils.c \
				src/librtspsrv/rtspsrv.c \
				src/librtspsrv/rtspsrv_adapter.c \
				src/librtspsrv/rtspsrv_epoll.c \
				src/librtspsrv/rtspsrv_handler.c \
				src/librtspsrv/rtspsrv_nlist.c \
				src/librtspsrv/rtspsrv_stream.c \
				src/librtspsrv/rtspsrv_utils.c
	
include $(AUTO_DEP_MK)

