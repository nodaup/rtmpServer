
#include "netManager.h"
#include "VideoSender.h"
#include "AudioSender.h"
#include "manager.h"
#include <chrono>
#include "videoEngine/VideoEngine_imp_87.h"
using PictureMixer = theia::VideoEngine::imp_87::PictureMixer7;

#pragma once
extern "C" {
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>

}

class MixManager {
public:

	MixManager();
	~MixManager();
	//init netManager videoSender audiosender
	int init();

	void mixVideo();

	uint8_t* getYUVData(AVFrame* frame);

	std::shared_ptr<NetManager> netManager = nullptr;
	std::unique_ptr<VideoSender> videoSender = nullptr;
	std::unique_ptr<AudioSender> audioSender = nullptr;

	int audioPacketTime;
	int videoPacketTime;

	//Æ´½Ó
	PictureMixer mixer_file;
	std::unordered_map<std::string, std::thread> map = {};
	std::map<int32_t, mixFrame> mixList;
	std::mutex mixPktMtx;
	condition_variable mixCond;
};