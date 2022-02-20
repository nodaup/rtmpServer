#pragma once
#include <stdio.h>
#include <iostream>
#include <seeker/common.h>

#define __STDC_CONSTANT_MACROS
#define UNIX

#ifdef _WIN32
// Windows
extern "C" {
#include "libavutil/opt.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libavutil/time.h"
};
#else
// Linux...
#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#ifdef __cplusplus
};
#endif
#endif
class ConvertH264Util {
public:
	ConvertH264Util(int width, int height, std::string outFile) {
		out_file = outFile;
		std::cout << width << "::::" << height << std::endl;
		// Method1.
		pFormatCtx = avformat_alloc_context();
		// Guess Format
		fmt = av_guess_format(NULL, out_file.c_str(), NULL);
		pFormatCtx->oformat = fmt;

		// Method 2.
		// avformat_alloc_output_context2(&pFormatCtx, NULL, NULL, out_file);
		// fmt = pFormatCtx->oformat;

		start_time = seeker::Time::currentTime();
		// Open output file
		if (avio_open(&pFormatCtx->pb, out_file.c_str(), AVIO_FLAG_WRITE) < 0) {
			printf("Failed to open output file! \n");
		}

		// create output stream

		//  video_st = avformat_new_stream(pFormatCtx, pCodec);
		video_st = avformat_new_stream(pFormatCtx, NULL);
		if (!video_st) {
			printf("Can not create stream! \n");
		}
		pCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
		if (!pCodec) {
			printf("Can not find encoder! \n");
		}
		pCodecCtx = avcodec_alloc_context3(pCodec);
		pCodecCtx->codec_id = fmt->video_codec;
		pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
		pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;  //像素格式，这里是YUV
		pCodecCtx->width = width;
		pCodecCtx->height = height;
		pCodecCtx->bit_rate = 800000;  //采样码率越大，视频大小越大
		pCodecCtx->gop_size = 250;     //每多少s插入一个I帧

		//帧率，1/25
		pCodecCtx->time_base.num = 1;
		pCodecCtx->time_base.den = 25;
		// H264
		// pCodecCtx->me_range = 16;
		// pCodecCtx->max_qdiff = 4;
		// pCodecCtx->qcompress = 0.6;

		//最大和最小量化系数，啥意思？
		pCodecCtx->qmin = 10;
		pCodecCtx->qmax = 51;

		//两个非B帧之间允许出现多少个B帧数
		//设置0表示不使用B帧
		// b 帧越多，图片越小
		pCodecCtx->max_b_frames = 0;
		//设置参数
		AVDictionary* param = 0;
		if (pCodecCtx->codec_id == AV_CODEC_ID_H264) {
			//av_dict_set(&param, "preset", "slow", 0);
			//av_dict_set(&param, "tune", "zerolatency", 0);
			av_dict_set(&param, "preset", "ultrafast", 0);
			av_dict_set(&param, "tune", "zerolatency", 0);
			av_dict_set(&param, "crf", "25", 0);
			// av_dict_set(¶m, "profile", "main", 0);
		}

		//注意，这句一定要在设置完pcodeCtx的参数之后再写
		avcodec_parameters_from_context(video_st->codecpar, pCodecCtx);
		//初始化AVCodecContext
		if (avcodec_open2(pCodecCtx, pCodec, &param) < 0) {
			printf("Failed to open encoder! \n");
		}


		// pFrame = av_frame_alloc();
		////  picture_size = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width,
		////  pCodecCtx->height); picture_buf = (uint8_t *)av_malloc(picture_size);
		////  avpicture_fill((AVPicture *)pFrame, picture_buf, pCodecCtx->pix_fmt,
		/// pCodecCtx->width, /  pCodecCtx->height);
		// pFrame->width = pCodecCtx->width;
		// pFrame->height = pCodecCtx->height;
		// pFrame->format = AV_PIX_FMT_YUV420P;

		//初始化buffer大小
		picture_size =
			av_image_get_buffer_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, 1);
		// picture_buf = (uint8_t*)av_malloc(picture_size);
		/* av_image_fill_arrays(pFrame->data,
							  pFrame->linesize,
							  picture_buf,
							  pCodecCtx->pix_fmt,
							  pCodecCtx->width,
							  pCodecCtx->height,
							  1);*/

		if (avformat_write_header(pFormatCtx, NULL) < 0) {
			printf("write header fail");
		}

		// Show some Information
		av_dump_format(pFormatCtx, 0, out_file.c_str(), 1);

		//  av_init_packet(pkt);
		//pkt = (AVPacket*)av_malloc(picture_size);
		//  pkt = av_packet_alloc();

		y_size = pCodecCtx->width * pCodecCtx->height;
	}

	void convertFrame(AVFrame* pFrame) {
		pFrame->pts = current_frame_index++;
		avcodec_send_frame(pCodecCtx, pFrame);
		AVPacket* pkt = av_packet_alloc();
		int result = avcodec_receive_packet(pCodecCtx, pkt);
		if (result == 0) {
			pkt->stream_index = video_st->index;
			uint8_t* p = pkt->data;
			//      while(p){
			//        cout<< *p <<"";
			//        p++;
			//      }
			/*for (int j = 0; j < pkt->size; j++) {
			  cout << *p << "";
			  p++;
			}
			cout << "   end" << endl;*/
			int64_t lTimeStamp = seeker::Time::currentTime() - start_time;
			//I_LOG("time:{}", lTimeStamp);
			pkt->dts = (int64_t)25 * lTimeStamp / AV_TIME_BASE;
			pkt->pts = pkt->dts;
			result = av_write_frame(pFormatCtx, pkt);
			if (result < 0) {
				//printf("输出一帧数据失败");
			}
			//	printf("current:%d\n", current_frame_index);
			current_frame_index++;
			//      av_packet_free(&pkt);
		}
		//	av_frame_free(&pFrame);
		av_packet_free(&pkt);
		/*if (current_frame_index == 100) {
		  av_write_trailer(pFormatCtx);
		}*/
	}

	void writeTail() {
		av_write_trailer(pFormatCtx);
	}

	~ConvertH264Util() {
		if (video_st) {
			avcodec_close(video_st->codec);
		}

		avio_close(pFormatCtx->pb);
		avformat_free_context(pFormatCtx);
		avcodec_close(pCodecCtx);
	}
	//	AVPacket* getPacket() { return pkt; }

private:
	AVFormatContext* pFormatCtx;
	AVOutputFormat* fmt;
	AVStream* video_st;
	AVCodecContext* pCodecCtx;
	AVCodec* pCodec;
	//AVPacket* pkt = nullptr;
	int picture_size;
	int y_size;
	int framecnt = 0;
	int framenum = 1000;  // Frames to encode
	string out_file;
	int current_frame_index = 1;
	int64_t start_time = 0;
};