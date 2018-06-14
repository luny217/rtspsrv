include ../../../../../Makefile.param

CFLAGS =-g -O2 -D_ISOC99_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_POSIX_C_SOURCE=200112 \
				-D_XOPEN_SOURCE=600 -DPIC -DHAVE_AV_CONFIG_H -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 \
				-I./ -I../ -I../libavcodec -I../libavformat
CFLAGS +=  -std=c99 -fomit-frame-pointer -fPIC -pthread -Wno-unused -fno-strict-aliasing -Wno-missing-field-initializers

TARGET_LIB=libnvrrtsp.a

SRCS =	nvr_rtsp.c \
				session.c \
				threadpool.c				


include $(AUTO_DEP_MK)
