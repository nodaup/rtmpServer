
#include "netManager.h"
#include "VideoSender.h"
#include "AudioSender.h"
#include "manager.h"
#include <chrono>

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

	int mixVideo();

	int mixAudio();

	int addManager(Manager* m);

	uint8_t* getYUVData(AVFrame* frame);

	std::shared_ptr<NetManager> netManager = nullptr;
	std::unique_ptr<VideoSender> videoSender = nullptr;
	std::unique_ptr<AudioSender> audioSender = nullptr;

	int audioPacketTime;
	int videoPacketTime;

	//Æ´½Ó
	theia::VideoEngine::PictureMixer* mixer_file;

	vector<Manager*> managerList;

};