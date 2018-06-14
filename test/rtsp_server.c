

#include "ffconfig.h"
#if !HAVE_CLOSESOCKET
#define closesocket close
#endif
#include <stdlib.h>
#include <stdio.h>
#include "libavformat/avformat.h"
#include "libavformat/avformat_network.h"
#include "libavutil/avutil_time.h"
#ifndef _WIN32
#include <unistd.h>
#include <sys/time.h>
#endif
#include <time.h>
#include <pthread.h>
#include <mshead.h>
#include <fifo.h>
#include "rtsp.h"
#include "rtspsrv.h"
#include "rtspsrv_session.h"

FILE * log_fp = NULL;

//#define  DEBUG_FILE

#define RTSP_PORT 554

#define MAIN_STREAM_URI "MainStream"
#define SUB_STREAM_URI  "SubStream"

pthread_t main_stream_pt;
pthread_t sub_stream_pt;

rsrv_fifo_t fifos = {0};

typedef struct
{
    uint32 fifo;        /* FIFO句柄 */
    uint32 fifo_size;   /* FIFO大小 */

    uint8 head_flag;    /* 包模式下,如果头没写进去,数据不应该写进去 */
    uint8 ifr_flag;     /* I帧丢弃的情况下,后面的P帧都应该丢掉 */
    uint8 strm_type;    /* 数据流类型(SDK_STREAM_TYPE_E) */
    uint8 vencode;
    uint8 aencode;
    uint32 width;       /* 写入FIFO视频帧高宽 */
    uint32 height;

    sint32 stat_cnt;    /* 码流状态计数*/
    uint32 byte_cnt;    /* */
    uint32 byte_rate;   /* 通道码率byte/s */

    uint32 ifr_cnt;     /* I帧总数 */
    uint32 vfr_cnt;     /* 视频帧总数 */
    uint32 afr_cnt;     /* 音频帧总数 */

    uint32 fr_lose;     /* 丢弃的视频总帧数 */
    uint32 cu_lose;     /* 最近一次丢弃的视频帧数 */
    SYSTIME systm;      /* 最近一次丢弃的视频帧时间 */
    sint32 sec, msec;   /* 最近一次丢弃的视频帧时间 */

    sint32 ch;          /* 当前通道号 */
} STREAM_CH_S;


#define DEBUG

struct ipc_info
{
	sint32  enabled;
	sint32	close_flag;
	sint8 * url;
	sint8 * username;
	sint8 * password;
	sint32  session_idx;
	sint32  video_mshead;
	sint32  fifo_handle;
	sint64	video_cnt;
	STREAM_CH_S strm_ch;
	chstrm_type_e strm_type;

} ipc_info_list[] =
{
	{
		.enabled = 1,
		.close_flag = 0,
		.url = "rtsp://192.168.30.92:554/Streaming/Channels/1?transportmode=unicast&profile=Profile_1",//Hik H264
		.username = "admin",
		.password = "12345",
		.session_idx = -1,
		.video_mshead = -1,
		.fifo_handle = -1,
		.video_cnt = 0,
		.strm_type = MAIN_STREAM,
	},
	{
		.enabled = 1,
		.close_flag = 0,
		.url = "rtsp://192.168.30.92:554/Streaming/Channels/2?transportmode=unicast&profile=Profile_1",//Hik H264
		.username = "admin",
		.password = "12345",
		.session_idx = -1,
		.video_mshead = -1,
		.fifo_handle = -1,
		.video_cnt = 0,
		.strm_type = SUB_STREAM1,
	},

	#if 0
{
		.enabled = 1,
		.url = "rtsp://192.168.30.176:8554/MainStream",//HB H264
		.username = "admin",
		.password = "888888",
		.session_idx = -1,
		.video_mshead = -1,
		.fifo_handle = -1,
		.video_cnt = 0,
	},
	{
		.enabled = 1,
		.url = "rtsp://192.168.30.176:8554/SubStream",//HB H264
		.username = "admin",
		.password = "888888",
		.session_idx = -1,
		.video_mshead = -1,
		.fifo_handle = -1,
		.video_cnt = 0,
	},
#endif
};

static int close_flag = 0;

void rtsp_cb_handler(E_RTSP_EVT evt, int session_idx, void * priv_data, RTSPCallbackInfo * rtsp_cb);
void rtsp_cb_handler(E_RTSP_EVT evt, int session_idx, void * priv_data, RTSPCallbackInfo * rtsp_cb)
{
	int i, ret;
	struct ipc_info * iinfo = NULL;

	/*Find ipc info struct*/
	for (i = 0; i < (int)(sizeof(ipc_info_list) / sizeof(ipc_info_list[0])); i++)
	{
		if (ipc_info_list[i].session_idx == session_idx)
		{
			iinfo = &ipc_info_list[i];
		}
	}
	if (!iinfo)
	{
		av_log(NULL, AV_LOG_ERROR, "Can not find iinfo for session#%d\n", session_idx);
		return;
	}

	if (evt == RTSP_EVT_NEW_FRAME)
	{
		sint8 * data = rtsp_cb->data;
		sint32 size = rtsp_cb->size;
		sint32 fifo_fd = iinfo->fifo_handle;


		if (rtsp_cb->frame_type == RTSP_CODEC_ID_H264 || rtsp_cb->frame_type == RTSP_CODEC_ID_H265)
		{
			//av_log(NULL, AV_LOG_WARNING, "[id:%d] iinfo: %p size: %d\n", session_idx, iinfo, size);

			ret = mshead_writeext(iinfo->video_mshead, ((rtsp_cb->key_frame) ? MSHEAD_STREAM_FRAME_VIDEO_I : MSHEAD_STREAM_FRAME_VIDEO_P),
				HB_FALSE, (sint8 *)data, size, rtsp_cb->time_sec, rtsp_cb->time_msec, rtsp_cb->width, rtsp_cb->height, rtsp_cb->fps);
			if (ret < 0)
			{
				av_log(NULL, AV_LOG_ERROR, "mshead_writeext err %x\n", ret);
			}

			ret = mshead_update_checksum(iinfo->video_mshead, (sint8 *)data, MSHEAD_GETMSDSIZE(iinfo->video_mshead));
			if (ret < 0)
			{
				av_log(NULL, AV_LOG_ERROR, "mshead_update_checksum err %x\n", ret);
			}

			if (iinfo->video_mshead)
			{
				sint32 free_size = 0, frame_size = 0;

				if (ISMSHEAD(iinfo->video_mshead))
				{
					iinfo->strm_ch.head_flag = 1;
				}
				if (ISKEYFRAME(iinfo->video_mshead))
				{
					iinfo->strm_ch.ifr_flag = 1;
				}
				frame_size = MSHEAD_GETFRAMESIZE(iinfo->video_mshead);

				/* 判断空间大小是否足够 */
				if ((ret = fifo_ioctrl(fifo_fd, FIFO_CMD_GETFREESIZE, ALL_CHANNEL, &free_size, sizeof(sint32))) >= 0)
				{
					sint32 thres_hold = frame_size;

					if (frame_size > (1 << 19))
					{
						av_log(NULL, AV_LOG_ERROR, "main_stream fifo(0x%x)\n"
							"free_size:%d, thres_hold:%d, frame_size:%d\n",
							iinfo->strm_ch.fifo, free_size, thres_hold, frame_size);
					}
					if (free_size <= IMAX(frame_size, thres_hold))
					{
						/* 只要有丢帧,则丢到下一个I帧 */
						iinfo->strm_ch.head_flag = 0;
						iinfo->strm_ch.ifr_flag = 0;
						iinfo->strm_ch.cu_lose = 0;
						av_log(NULL, AV_LOG_ERROR, "fifo(0x%x) free_size(%d) frame_size(%d)\n", fifo_fd, free_size, frame_size);
						{
							int fr_cnt;
							fifo_ioctrl(fifo_fd, FIFO_CMD_GETVFRAMECOUNT, 0, &fr_cnt, sizeof(sint32));
							av_log(NULL, AV_LOG_WARNING, "consumer0 fr_cnt %d\n", fr_cnt);

							fifo_ioctrl(fifo_fd, FIFO_CMD_GETVFRAMECOUNT, 1, &fr_cnt, sizeof(sint32));
							av_log(NULL, AV_LOG_WARNING, "consumer1 fr_cnt %d\n", fr_cnt);

							fifo_ioctrl(fifo_fd, FIFO_CMD_GETVFRAMECOUNT, 2, &fr_cnt, sizeof(sint32));
							av_log(NULL, AV_LOG_WARNING, "consumer2 fr_cnt %d\n", fr_cnt);
						}
					}
				}

				if (iinfo->strm_ch.head_flag && iinfo->strm_ch.ifr_flag)
				{
					if (data)
					{
						if ((ret = fifo_writeext(fifo_fd, (char *)iinfo->video_mshead, MSHEAD_GETMSHSIZE(iinfo->video_mshead))) >= 0)
						{
							/*av_hex_dump(NULL, video_mshead, MSHEAD_GETMSHSIZE(video_mshead));
							av_log(NULL, AV_LOG_ERROR, "\n");
							av_hex_dump(NULL, video_pkt.data, 32);*/

							ret = fifo_write(fifo_fd, data, MSHEAD_GETMSDSIZE(iinfo->video_mshead));
							if (ret < 0)
							{
								av_log(NULL, AV_LOG_ERROR, "fifo_write err\n");
								return;
							}
							iinfo->video_cnt++;
						}
					}
				}
				else
				{
					if (ISVIDEOFRAME(iinfo->video_mshead))
					{
						iinfo->strm_ch.fr_lose++;
						iinfo->strm_ch.cu_lose++;
						if (1 == iinfo->strm_ch.cu_lose) /* 打印丢失视频的第一帧信息 */
						{
							MSHEAD_GET_TIMESTAMP(iinfo->video_mshead, iinfo->strm_ch.sec, iinfo->strm_ch.msec);
						}
						/*av_log(NULL, AV_LOG_ERROR, "strm_type(%d) lose frame fifo:%#x [%d-%d] fcount:%d\n",
						strm_ch.strm_type, strm_ch.fifo, MSHEAD_GET_VIDEO_WIDTH(mshead_main),
						MSHEAD_GET_VIDEO_HEIGHT(mshead_main), MSHEAD_GET_VIDEO_FCOUNT(mshead_main));*/
					}
				}
			}
		}	
		else if (rtsp_cb->frame_type == RTSP_CODEC_ID_G711U || rtsp_cb->frame_type == RTSP_CODEC_ID_G711A)
		{

		}
	}
	else if (evt == RTSP_EVT_EOF)
	{
		av_log(NULL, AV_LOG_ERROR, "Session#%d connection was closed by remote server\n", session_idx);
		//rtsp_close(session_idx);
		//iinfo->session_idx = rtsp_open(iinfo->url, iinfo->username, iinfo->password, NULL);
		iinfo->close_flag = 1;
	}
	else if (evt == RTSP_EVT_READ_FRAME_TIMEOUT)
	{
		av_log(NULL, AV_LOG_ERROR, "Session#%d read frame timed out\n", session_idx);
	}
	else if (evt == RTSP_EVT_RESOLUTION_CHANGED)
	{
		av_log(NULL, AV_LOG_ERROR, "Session#%d resulution changed\n", session_idx);
	}
	else if (evt == RTSP_EVT_CLOSED)
	{
		av_log(NULL, AV_LOG_ERROR, "Session#%d RTSP_EVT_CLOSED\n", session_idx);
	}
	else
	{
		// printf("Session#%d unhandled event %d\n", session_idx, evt);
	}
}

int nvr_open(int num);

int nvr_open(int num)
{
	int i, open_ok;
	/*open remote ipc rtsp stream*/
	for (i = 0, open_ok = 0; open_ok < num && i < (int)(sizeof(ipc_info_list) / sizeof(ipc_info_list[0])); i++)
	{
		struct ipc_info * iinfo = &ipc_info_list[i];
		if (!iinfo->enabled)
		{
			continue;
		}

		iinfo->session_idx = rtsp_open(iinfo->url, iinfo->username, iinfo->password, rtsp_cb_handler, NULL);
		if (iinfo->session_idx < 0)
		{
			av_log(NULL, AV_LOG_ERROR, "Open %s failed\n", iinfo->url);
		}
		else
		{
			av_log(NULL, AV_LOG_WARNING, "Open OK:#%d, session_idx=%d, url=%s\n", open_ok, iinfo->session_idx, iinfo->url);
		}
	}
	return 0;
}

int nvr_close(int num);

int nvr_close(int num)
{
	int i;

	/*close all rtsp stream opened*/
	for (i = 0; i < (int)(sizeof(ipc_info_list) / sizeof(ipc_info_list[0])); i++)
	{
		struct ipc_info * iinfo = &ipc_info_list[i];
		if (iinfo->session_idx >= 0)
		{
			rtsp_close(iinfo->session_idx);
			printf("session#%d session_idx=%d closed\n", i, iinfo->session_idx);
		}
	}
	return 0;
}


#if 0
int main_stream_thread()
{
    AVPacket video_pkt, audio_pkt;
    AVFormatContext * video_infile = NULL, * audio_infile = NULL;
	char video_filename[] = "HB_1080P_4M.h264"; //"MainStream.hevc";
	char audio_filename[] = "main1.al";	//"gee.al";
    int ret, ret1, ret2, video_mshead, audio_mshead;
    int video_type = MSHEAD_ALGORITHM_VIDEO_H265_GENERAL;
    int audio_type = MSHEAD_ALGORITHM_AUDIO_G711A;
    struct timeval tv;
    STREAM_CH_S strm_ch = { 0 };
    int video_cnt = 0, audio_cnt = 0;
    sint64 frame_time, frame_gap;

    ret = avformat_open_input(&video_infile, video_filename, NULL, NULL);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Could not open '%s': %s\n", video_filename, av_err2str(ret));
        return -1;
    }

    //ret = avformat_open_input(&audio_infile, audio_filename, NULL, NULL);
    //if (ret < 0)
    //{
    //    av_log(NULL, AV_LOG_ERROR, "Could not open '%s': %s\n", audio_filename, av_err2str(ret));
    //    /*return -1;*/
    //}

    video_mshead = mshead_open(video_type, MSHEAD_LEN + MSHEAD_SEG_VIDEO_S_LEN + MSHEAD_SEG_LEN, 0);
    if (video_mshead < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "video_mshead open err %x\n", video_mshead);
        return -1;
    }

    audio_mshead = mshead_open(audio_type, MSHEAD_LEN, 0);
    if (audio_mshead < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "audio_mshead open err %x\n", audio_mshead);
        return -1;
    }

    while (1)
    {
        frame_time = av_gettime() / 1000;

        ret1 = av_read_frame(video_infile, &video_pkt);
        /*ret2 = av_read_frame(audio_infile, &audio_pkt);*/
        if (ret1 < 0 /*|| ret2 < 0*/)
        {
            if (ret1 == AVERROR(EAGAIN) /*|| ret2 == AVERROR(EAGAIN)*/)
            {
                /* input not ready, come back later */
            }
            else if (ret1 == AVERROR_EOF /*|| ret2 == AVERROR_EOF*/)
            {
                avformat_close_input(&video_infile);
                /*avformat_close_input(&audio_infile);*/

                if ((ret = avformat_open_input(&video_infile, video_filename, NULL, NULL)) < 0)
                {
                    av_log(NULL, AV_LOG_ERROR, "Could not open '%s': %s\n", video_filename, av_err2str(ret));
                    return -1;
                }

				/*ret = avformat_open_input(&audio_infile, audio_filename, NULL, NULL);
				if (ret < 0)
				{
					av_log(NULL, AV_LOG_ERROR, "Could not open '%s': %s\n", audio_filename, av_err2str(ret));
					return -1;
				}*/
            }
            else
            {
                av_log(video_infile, AV_LOG_ERROR, "av_read_frame error: %d %s\n", ret, av_err2str(ret));
            }
        }
        else
        {
            gettimeofday(&tv, NULL);

            ret = mshead_writeext(video_mshead, ((video_pkt.key_frame) ? MSHEAD_STREAM_FRAME_VIDEO_I : MSHEAD_STREAM_FRAME_VIDEO_P),
                                  HB_FALSE, (sint8 *)video_pkt.data, video_pkt.size, tv.tv_sec, tv.tv_usec / 1000, 1920, 1080, 25);
            if (ret < 0)
            {
                av_log(NULL, AV_LOG_ERROR, "mshead_writeext err %x\n", ret);
            }

            ret = mshead_update_checksum(video_mshead, (sint8 *)video_pkt.data, MSHEAD_GETMSDSIZE(video_mshead));
            if (ret < 0)
            {
                av_log(NULL, AV_LOG_ERROR, "mshead_update_checksum err %x\n", ret);
            }


            /*ret = mshead_writeext(audio_mshead, MSHEAD_STREAM_FRAME_AUDIO,
                                  HB_TRUE, (sint8 *)audio_pkt.data, audio_pkt.size, tv.tv_sec, tv.tv_usec / 1000, 0, 0, 25);
            if (ret < 0)
            {
                av_log(NULL, AV_LOG_ERROR, "mshead_writeext audio_mshead err %x\n", ret);
            }

            ret = mshead_update_checksum(audio_mshead, (sint8 *)audio_pkt.data, MSHEAD_GETMSDSIZE(audio_mshead));
            if (ret < 0)
            {
                av_log(NULL, AV_LOG_ERROR, "mshead_update_checksum audio_mshead err %x\n", ret);
            }*/

            if (video_mshead /*&& audio_mshead*/)
            {
                sint32 free_size = 0, frame_size = 0;

                if (ISMSHEAD(video_mshead))
                {
                    strm_ch.head_flag = 1;
                }
                if (ISKEYFRAME(video_mshead))
                {
                    strm_ch.ifr_flag = 1;
                }
                frame_size = MSHEAD_GETFRAMESIZE(video_mshead);

                /* 判断空间大小是否足够 */
                if ((ret = fifo_ioctrl(fifos.pri_handle, FIFO_CMD_GETFREESIZE, ALL_CHANNEL, &free_size, sizeof(sint32))) >= 0)
                {
                    sint32 thres_hold = frame_size;

                    if (frame_size > (1 << 19))
                    {
                        av_log(NULL, AV_LOG_ERROR, "main_stream fifo(0x%x)\n"
                               "free_size:%d, thres_hold:%d, frame_size:%d\n",
                               strm_ch.fifo, free_size, thres_hold, frame_size);
                    }
                    if (free_size <= IMAX(frame_size, thres_hold))
                    {
                        /* 只要有丢帧,则丢到下一个I帧 */
                        strm_ch.head_flag = 0;
                        strm_ch.ifr_flag = 0;
                        strm_ch.cu_lose = 0;
                        av_log(NULL, AV_LOG_ERROR, "main fifo(0x%x) free_size(%d) frame_size(%d)\n", fifos.pri_handle, free_size, frame_size);
                        {
                            int fr_cnt;
                            fifo_ioctrl(fifos.pri_handle, FIFO_CMD_GETVFRAMECOUNT, 0, &fr_cnt, sizeof(sint32));
                            av_log(NULL, AV_LOG_WARNING, "consumer0 fr_cnt %d\n", fr_cnt);

                            fifo_ioctrl(fifos.pri_handle, FIFO_CMD_GETVFRAMECOUNT, 1, &fr_cnt, sizeof(sint32));
                            av_log(NULL, AV_LOG_WARNING, "consumer1 fr_cnt %d\n", fr_cnt);

                            fifo_ioctrl(fifos.pri_handle, FIFO_CMD_GETVFRAMECOUNT, 2, &fr_cnt, sizeof(sint32));
                            av_log(NULL, AV_LOG_WARNING, "consumer2 fr_cnt %d\n", fr_cnt);
                        }
                    }
                }

                if (strm_ch.head_flag && strm_ch.ifr_flag)
                {
                    if (video_pkt.data)
                    {
                        if ((ret = fifo_writeext(fifos.pri_handle, (char *)video_mshead, MSHEAD_GETMSHSIZE(video_mshead))) >= 0)
                        {
							/*av_hex_dump(NULL, video_mshead, MSHEAD_GETMSHSIZE(video_mshead));
							av_log(NULL, AV_LOG_ERROR, "\n");
							av_hex_dump(NULL, video_pkt.data, 32);*/

                            ret = fifo_write(fifos.pri_handle, video_pkt.data, MSHEAD_GETMSDSIZE(video_mshead));
                            if (ret < 0)
                            {
                                av_log(NULL, AV_LOG_ERROR, "fifo_write err\n");
                                return -1;
                            }
                            video_cnt++;
                        }
                    }

                    //if (audio_pkt.data)
                    //{
                    //    if ((ret = fifo_writeext(fifos.pri_handle, (char *)audio_mshead, MSHEAD_GETMSHSIZE(audio_mshead))) >= 0)
                    //    {
                    //        //av_hex_dump(NULL, pkt.data, 80);
                    //        //av_log(audio_infile, AV_LOG_ERROR, "audio size: %d\n", audio_pkt.size);
                    //        ret = fifo_write(fifos.pri_handle, audio_pkt.data, MSHEAD_GETMSDSIZE(audio_mshead));
                    //        if (ret < 0)
                    //        {
                    //            av_log(NULL, AV_LOG_ERROR, "fifo_write audio_pkt err 0x%x\n", ret);
                    //            return -1;
                    //        }
                    //        audio_cnt++;
                    //    }
                    //}
                }
                else
                {
                    if (ISVIDEOFRAME(video_mshead))
                    {
                        strm_ch.fr_lose++;
                        strm_ch.cu_lose++;
                        if (1 == strm_ch.cu_lose) /* 打印丢失视频的第一帧信息 */
                        {
                            MSHEAD_GET_TIMESTAMP(video_mshead, strm_ch.sec, strm_ch.msec);
                        }
                        /*av_log(NULL, AV_LOG_ERROR, "strm_type(%d) lose frame fifo:%#x [%d-%d] fcount:%d\n",
                               strm_ch.strm_type, strm_ch.fifo, MSHEAD_GET_VIDEO_WIDTH(mshead_main),
                               MSHEAD_GET_VIDEO_HEIGHT(mshead_main), MSHEAD_GET_VIDEO_FCOUNT(mshead_main));*/
                    }
                }
            }
        }

        frame_gap = av_gettime() / 1000 - frame_time;

        while (frame_gap < 50)
        {
            av_usleep(1000);
            frame_gap = av_gettime() / 1000 - frame_time;
        }
        /*av_log(NULL, AV_LOG_WARNING, "-----------------------------video_cnt=%d size=%d audio_cnt=%d size=%d time=%dms\n",
        		video_cnt, video_pkt.size, audio_cnt, audio_pkt.size, frame_gap);*/

        av_free_packet(&video_pkt);
        /*av_free_packet(&audio_pkt);*/
    }
}

int sec_stream_thread()
{
    AVPacket pkt;
    AVFormatContext * infile = NULL;
    char feed_filename[] = "SubStream.hevc"; //"SubStream.hevc";
    int ret, mshead;
    int video_type = MSHEAD_ALGORITHM_VIDEO_H265_GENERAL;
    struct timeval tv;
    STREAM_CH_S strm_ch = { 0 };
    static int frame_cnt = 1;
	sint64 frame_time, frame_gap;

    ret = avformat_open_input(&infile, feed_filename, NULL, NULL);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Could not open '%s': %s\n", feed_filename, av_err2str(ret));
        return -1;
    }

    mshead = mshead_open(video_type, MSHEAD_LEN + MSHEAD_SEG_VIDEO_S_LEN + MSHEAD_SEG_LEN, 0);
    if (mshead < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "mshead_open err %x\n", mshead);
        return -1;
    }

    while (1)
    {
		frame_time = av_gettime() / 1000;

        ret = av_read_frame(infile, &pkt);
        if (ret < 0)
        {
            if (ret == AVERROR(EAGAIN))
            {
                /* input not ready, come back later */
            }
            else if (ret == AVERROR_EOF)
            {
                avformat_close_input(&infile);

                if ((ret = avformat_open_input(&infile, feed_filename, NULL, NULL)) < 0)
                {
                    av_log(NULL, AV_LOG_ERROR, "Could not open '%s': %s\n", feed_filename, av_err2str(ret));
                    return -1;
                }
            }
            else
            {
                av_log(infile, AV_LOG_ERROR, "av_read_frame error: %d %s\n", ret, av_err2str(ret));
            }
        }
        else
        {
            gettimeofday(&tv, NULL);

            ret = mshead_writeext(mshead, ((pkt.key_frame) ? MSHEAD_STREAM_FRAME_VIDEO_I : MSHEAD_STREAM_FRAME_VIDEO_P),
                                  HB_FALSE, (sint8 *)pkt.data, pkt.size, tv.tv_sec, tv.tv_usec / 1000, 704, 576, 25);
            if (ret < 0)
            {
                av_log(NULL, AV_LOG_ERROR, "mshead_writeext err %x\n", ret);
            }

            ret = mshead_update_checksum(mshead, (sint8 *)pkt.data, MSHEAD_GETMSDSIZE(mshead));
            if (ret < 0)
            {
                av_log(NULL, AV_LOG_ERROR, "mshead_update_checksum err %x\n", ret);
            }

            if (mshead)
            {
                sint32 free_size = 0, frame_size = 0;

                if (ISMSHEAD(mshead))
                {
                    strm_ch.head_flag = 1;
                }
                if (ISKEYFRAME(mshead))
                {
                    strm_ch.ifr_flag = 1;
                }
                frame_size = MSHEAD_GETFRAMESIZE(mshead);

                /*ret = fifo_ioctrl(fifos.sec_handle, FIFO_CMD_CONSUMERSTAT, 0, &consumer_stat, sizeof(sint32));
                if (ret > 0)
                {
                    for (consumer = 0; consumer < 8; consumer++)
                    {
                        consumer_stat >> consumer
                        ret = fifo_ioctrl(fifos.sec_handle, FIFO_CMD_GETFREESIZE, ALL_CHANNEL, &free_size, sizeof(sint32))
                    }
                }*/

                /* 判断空间大小是否足够 */
                if ((ret = fifo_ioctrl(fifos.sec_handle, FIFO_CMD_GETFREESIZE, ALL_CHANNEL, &free_size, sizeof(sint32))) >= 0)
                {
                    sint32 thres_hold = frame_size;

                    if (frame_size > (1 << 19))
                    {
                        av_log(NULL, AV_LOG_ERROR, "sec_stream fifo(0x%x)\n"
                               "free_size:%d, thres_hold:%d, frame_size:%d\n",
                               fifos.sec_handle, free_size, thres_hold, frame_size);
                    }
                    if (free_size <= IMAX(frame_size, thres_hold))
                    {
                        /* 只要有丢帧,则丢到下一个I帧 */
                        strm_ch.head_flag = 0;
                        strm_ch.ifr_flag = 0;
                        strm_ch.cu_lose = 0;
                        av_log(NULL, AV_LOG_ERROR, "sec fifo(0x%x) free_size(%d) frame_size(%d)\n", fifos.sec_handle, free_size, frame_size);
                    }
                    //av_log(NULL, AV_LOG_WARNING, "free_size = %d %x\n", free_size);
                }
                else
                {

                }

                if (strm_ch.head_flag && strm_ch.ifr_flag)
                {
                    if (pkt.data)
                    {
                        if ((ret = fifo_writeext(fifos.sec_handle, (char *)mshead, MSHEAD_GETMSHSIZE(mshead))) >= 0)
                        {
                            //av_hex_dump(NULL, pkt.data, 80);
                            //av_log(infile, AV_LOG_ERROR, "frame size: %d\n", pkt.size);
                            ret = fifo_write(fifos.sec_handle, pkt.data, MSHEAD_GETMSDSIZE(mshead));
                            if (ret < 0)
                            {
                                av_log(NULL, AV_LOG_ERROR, "fifo_write err\n");
                                return -1;
                            }
                            //av_log(infile, AV_LOG_ERROR, "-----------------------------frame_cnt: %d is_key:%d size:%d\n", frame_cnt++, pkt.key_frame, pkt.size);

                            if (pkt.key_frame)
                            {
                                frame_cnt = 1;
                            }
                        }
                    }
                }
                else
                {
                    if (ISVIDEOFRAME(mshead))
                    {
                        strm_ch.fr_lose++;
                        strm_ch.cu_lose++;
                        if (1 == strm_ch.cu_lose) /* 打印丢失视频的第一帧信息 */
                        {
                            MSHEAD_GET_TIMESTAMP(mshead, strm_ch.sec, strm_ch.msec);
                        }
                        /*av_log(NULL, AV_LOG_ERROR, "strm_type(%d) lose frame fifo:%#x [%d-%d] fcount:%d\n",
                        strm_ch.strm_type, strm_ch.fifo, MSHEAD_GET_VIDEO_WIDTH(mshead_main),
                        MSHEAD_GET_VIDEO_HEIGHT(mshead_main), MSHEAD_GET_VIDEO_FCOUNT(mshead_main));*/
                    }
                }
            }
            av_free_packet(&pkt);
        }

		frame_gap = av_gettime() / 1000 - frame_time;

		while (frame_gap < 50)
		{
			av_usleep(1000);
			frame_gap = av_gettime() / 1000 - frame_time;
		}
    }
}
#endif

void rtspsrv_log(void * ptr, int level, const char * fmt, va_list vl)
{
    unsigned tint = 0;
    if (level >= 0)
    {
        tint = level & 0xff00;
        level &= 0xff;
    }

    if (level > AV_LOG_WARNING)
        return;

    if (log_fp)
    {
        vfprintf(log_fp, fmt, vl);
        fflush(log_fp);
    }
}

static void get_cmd(char * pcmd)
{
	int i = 0;

	while ('\n' != (pcmd[i] = getchar()))
	{
		i++;
	}
}

int main(int argc, char ** argv)
{
    int ret, i;
	int  ncmd = 0;
	char uccmd[8];
	int video_type = MSHEAD_ALGORITHM_VIDEO_H264_STANDARD;

    av_register_all();

#if 0 //def _WIN32
    WSADATA wsaData;
    int nResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (nResult != NO_ERROR)
    {
        fprintf(stderr, "WSAStartup() failed.\n");
        return -1;
    }
#endif

    av_log_set_level(AV_LOG_WARNING);

#ifdef DEBUG_FILE
	log_fp = fopen("rtspsrv.log", "w+");
    av_log_set_callback(rtspsrv_log);
#endif // DEBUG	

	rtsp_init(0);

    fifos.pri_handle = fifo_openmulti(0, FIFO_CONSUMER_NUM, MAIN_REC_FIFO_SIZE,
                                      FIFO_HTWATER_SIZE, FIFO_HIWATER_SIZE, FIFO_LOWATER_SIZE, 0);

    if (fifos.pri_handle < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "fifo_openmulti err %x\n", fifos.pri_handle);
        return -1;
    }

    fifos.sec_handle = fifo_openmulti(0, FIFO_CONSUMER_NUM, SUB_REC_FIFO_SIZE,
                                      FIFO_HTWATER_SIZE, FIFO_HIWATER_SIZE, FIFO_LOWATER_SIZE, 0);

    if (fifos.sec_handle < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "fifo_openmulti err %x\n", fifos.sec_handle);
        return -1;
    }

#if 0
    if ((pthread_create(&main_stream_pt, NULL, (void *)&main_stream_thread, NULL)) != 0)
    {
        av_log(NULL, AV_LOG_ERROR, "create main_stream_thread failed\n");
    }

    if ((pthread_create(&sub_stream_pt, NULL, (void *)&sec_stream_thread, NULL)) != 0)
    {
        av_log(NULL, AV_LOG_ERROR, "create sec_stream_thread failed\n");
    }
#endif

	for (i = 0; i < (int)(sizeof(ipc_info_list) / sizeof(ipc_info_list[0])); i++)
	{
		struct ipc_info * iinfo = &ipc_info_list[i];
		if (!iinfo->enabled)
		{
			continue;
		}

		if (iinfo->strm_type == MAIN_STREAM)
		{
			iinfo->fifo_handle = fifos.pri_handle;
		}
		else if (iinfo->strm_type == SUB_STREAM1)
		{
			iinfo->fifo_handle = fifos.sec_handle;
		}
		else
		{
			av_log(NULL, AV_LOG_ERROR, "strm_type err %d\n", iinfo->strm_type);
			return -1;
		}

		iinfo->video_mshead = mshead_open(video_type, MSHEAD_LEN + MSHEAD_SEG_VIDEO_S_LEN + MSHEAD_SEG_LEN, 0);
		if (iinfo->video_mshead < 0)
		{
			av_log(NULL, AV_LOG_ERROR, "video_mshead open err %x\n", iinfo->video_mshead);
			return -1;
		}

		iinfo->session_idx = rtsp_open(iinfo->url, iinfo->username, iinfo->password, rtsp_cb_handler, (void*)iinfo);
		if (iinfo->session_idx < 0)
		{
			av_log(NULL, AV_LOG_ERROR, "Open %s failed\n", iinfo->url);
		}
		else
		{
			av_log(NULL, AV_LOG_WARNING, "Open OK: session_idx=%d, url=%s\n", iinfo->session_idx, iinfo->url);
		}
	}

    ret = rtspsrv_open(1, 1, 554, &fifos);
    if (ret < 0)
    {
        return -1;
    }

    ret = rtspsrv_start();
    if (ret < 0)
    {
        return -1;
    }


    while (1)
    {
		for (i = 0; i < (int)(sizeof(ipc_info_list) / sizeof(ipc_info_list[0])); i++)
		{
			struct ipc_info * iinfo = &ipc_info_list[i];
			if (!iinfo->enabled)
			{
				continue;
			}

			if (iinfo->close_flag == 1)
			{
				if (iinfo->session_idx > 0)
				{
					rtsp_close(iinfo->session_idx);
					iinfo->session_idx = -1;
				}

				iinfo->session_idx = rtsp_open(iinfo->url, iinfo->username, iinfo->password, rtsp_cb_handler, (void*)iinfo);
				if (iinfo->session_idx < 0)
				{
					av_log(NULL, AV_LOG_ERROR, "Open %s failed\n", iinfo->url);
				}
				else
				{
					av_log(NULL, AV_LOG_WARNING, "Open OK: session_idx=%d, url=%s\n", iinfo->session_idx, iinfo->url);
				}
				iinfo->close_flag = 0;
			}
		}
		av_usleep(10000);
    }

	rtsp_deinit();

    return 0;
}

