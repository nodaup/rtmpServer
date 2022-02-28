#include "manager.h"
#include "ConvertH264.hpp"

//int Manager::saveRtpPkt(uint8_t* pkt, int pktsize) {
//
//    as->recvPktMtx.lock();
//    auto temp = new uint8_t[pktsize + 1]();
//    memcpy(temp, pkt, pktsize);
//    as->pktList.push_back(std::pair<uint8_t*, int>(temp, pktsize));
//    as->recvPktMtx.unlock();
//    pktCond.notify_one();
//    return 0;
//}

void Manager::asio_audio_thread() {
    //audio
    as_audio = new rtpServer("127.0.0.1", 1234, true);
    auto recvCallBack2 = [&](uint8_t* pkt, int pktsize, int32_t ssrc, int32_t ts, int32_t seqnum, int32_t pt) {  
        auto temp = new uint8_t[pktsize + 1]();
        memcpy(temp, pkt, pktsize);

        //decode
        adecoder.addPacket(pkt, pktsize, ts, seqnum);
        //save frame
        auto recvFrame = adecoder.getFrame();
        I_LOG("111!");
        if (recvFrame) {
            I_LOG("recv AUDIO!");
            std::unique_lock<std::mutex> lk1(encodePktMtx_a);
            sendList_a.push(audioFrame{ recvFrame, pktsize });
            sendpktCond_a.notify_one();
            lk1.unlock();
        }
    };
    as_audio->setCallBack(recvCallBack2);
    as_audio->start();
}

int Manager::init() {
    //init video decoder
    decoder = new theia::VideoEngine::imp_87::Decoder87();
    decoder->setPixFmt(AV_PIX_FMT_YUV420P);
    decoder->init();
    ConvertH264Util* ch = nullptr;

    //init audio decoder

    AudioInfo in;
    in.sample_rate = decoderInfo.inSampleRate;
    in.channels = decoderInfo.inChannels;
    in.sample_fmt = (AVSampleFormat)(decoderInfo.inFormate);
    in.channel_layout = av_get_default_channel_layout(decoderInfo.inChannels);
    if (decoderInfo.cdtype == CodecType::PCMA) {
        in.pcmaTimeSeg = 30;
    }
    AudioInfo out;
    out.sample_rate = decoderInfo.outSampleRate;
    out.channels = decoderInfo.outChannels;
    out.sample_fmt = (AVSampleFormat)(decoderInfo.outFormate);
    out.channel_layout = av_get_default_channel_layout(decoderInfo.outChannels);

    CoderInfo decoderinfo;
    decoderinfo.outChannels = 1;
    decoderinfo.outSampleRate = 32000;
    decoderinfo.outFormate = MyAVSampleFormat::AV_SAMPLE_FMT_S16;

    decoderinfo.inChannels = 1;
    decoderinfo.inSampleRate = 32000;
    decoderinfo.inFormate = MyAVSampleFormat::AV_SAMPLE_FMT_FLTP;
    decoderinfo.cdtype = CodecType::AAC;
    decoderinfo.muxType = theia::audioEngine::MuxType::ADTS;

    decoderCodecType = decoderinfo.cdtype;

    info.setInfo(decoderinfo.inSampleRate, (AVSampleFormat)decoderinfo.inFormate,
        decoderinfo.inChannels);
    outInfo.setInfo(decoderinfo.outSampleRate, (AVSampleFormat)decoderinfo.outFormate,
        decoderinfo.outChannels);

    adecoder.init(in, out, decoderCodecType, 0);
    adecoder.setDemuxType(MuxType::None);


    //init rtmp server
    netManager = std::make_shared<NetManager>();
    netManager->setRtmpUrl("rtmp://10.28.197.125:1935");

    if (netManager->rtmpInit(0) == -1) {
        return 0;
    }

    //videoSender / audioSender init encoder
    videoSender = std::make_unique<VideoSender>(netManager);
    VideoDefinition vd = VideoDefinition(640, 360);
    videoSender->initEncoder(vd, 20);
    videoPacketTime = 1000 / 20;
    long long startTime = seeker::Time::currentTime();
    videoSender->setStartTime(startTime);

    //encoder info
    CoderInfo encoderinfo;
    encoderinfo.inChannels = 1;
    encoderinfo.inSampleRate = 32000;
    encoderinfo.inFormate = MyAVSampleFormat::AV_SAMPLE_FMT_S16;
    encoderinfo.outChannels = 1;
    encoderinfo.outSampleRate = 32000;
    encoderinfo.outFormate = MyAVSampleFormat::AV_SAMPLE_FMT_FLTP;
    encoderinfo.cdtype = CodecType::AAC;
    encoderinfo.muxType = theia::audioEngine::MuxType::ADTS;
    int buffSize = 1024 * 2;

    audioPacketTime = 1024 * 1000 / encoderinfo.outSampleRate;

    audioSender = std::make_unique<AudioSender>(netManager);
    audioSender->initAudioEncoder(encoderinfo);
    audioSender->setStartTime(startTime);

    if (netManager->rtmpInit(1) == -1) {
        return 0;
    }


    std::thread decodeThread{ &Manager::decodeTh, this };
    threadMap["decodeTh"] = std::move(decodeThread);

    std::thread encodeThread{ &Manager::encodeTh, this };
    threadMap["encodeTh"] = std::move(encodeThread);

    std::thread audioEncodeThread{ &Manager::encodeAudioTh, this };
    threadMap["encodeAudioTh"] = std::move(audioEncodeThread);

    std::thread audioRecv{ &Manager::asio_audio_thread, this };
    threadMap["audioRecvTh"] = std::move(audioRecv);

    //video
    as = new rtpServer("127.0.0.1", 30502, false);
    auto recvCallBack = [&](uint8_t* pkt, int pktsize, int32_t ssrc, int32_t ts, int32_t seqnum, int32_t pt) {
        recvPktMtx.lock();
        auto temp = new uint8_t[pktsize + 1]();
        memcpy(temp, pkt, pktsize);
        pktList.push_back(std::pair<uint8_t*, int>(temp, pktsize));
        recvPktMtx.unlock();
        pktCond.notify_one();
    };
    as->setCallBack(recvCallBack);
    as->start();
}


int Manager::decodeTh() {

    ConvertH264Util* ch = nullptr;

    while (true)
    {
        list<std::pair<uint8_t*, int>> tempPktList;

        unique_lock<mutex> lock{ recvPktMtx };
        pktCond.wait(lock, [&]() {return pktList.size() > 0; });

        auto pkt = pktList.front();
        pktList.pop_front();
        lock.unlock();
        //I_LOG("PKT:{}", pkt.second);
        decoder->push(pkt.first, pkt.second, 0);
        AVFrame* frame = av_frame_alloc();
        int rst = decoder->poll(frame);
        //I_LOG("decoder poll");
        if (rst != 0 || frame->width <= 0 || frame->height <= 0) {
            //I_LOG("no :{}, width:{}", rst, frame->width);
            av_frame_free(&frame);
            continue;
        }
         /*if (rst == 0) {
             if (ch == nullptr) {
                 I_LOG("rst :{}", rst);
                 ch = new ConvertH264Util(frame->width, frame->height, SAVEFILENAME);
             }
             I_LOG("write frame 1");
             ch->convertFrame(frame);
         }*/

        if (rst == 0) {
            //push to encode list
            std::unique_lock<std::mutex> lk1(encodePktMtx);
            sendList.push(mixFrame{ frame, frame->width, frame->height });
            sendpktCond.notify_one();
            lk1.unlock();
        }
        free(pkt.first);
    }
}

int Manager::encodeTh() {
    I_LOG("encoder thread start");
    while (true)
    {
        std::unique_lock<std::mutex> lk(encodePktMtx);
        sendpktCond.wait(lk, [this]() {return sendList.size() > 0; });
        if (stopFlag)
        {
            lk.unlock();
            break;
        }
        auto dstData = sendList.front();
        sendList.pop();
        lk.unlock();
        if (dstData.data != NULL) {
            videoSender->send(getYUVData(dstData.data), dstData.width, dstData.height, AV_PIX_FMT_YUV420P);
            free(yuvData);
            av_free(dstData.data);
        }
    }
}

uint8_t* Manager::getYUVData(AVFrame* frame) {
    int frame_height = frame->height;
    int frame_width = frame->width;
    int i = 0;
    int j = 0;
    int k = 0;
    int size = avpicture_get_size(AV_PIX_FMT_YUV420P, frame_width, frame_height);
    int video_decode_size = avpicture_get_size(AV_PIX_FMT_YUV420P, frame_width, frame_height);
    yuvData = (uint8_t*)malloc(size * 3 * sizeof(char));
    uint8_t* video_decode_buf = (uint8_t*)malloc(video_decode_size * 3 * sizeof(char));
    if (frame) {
        for (i = 0; i < frame_height; i++)
        {
            memcpy(video_decode_buf + frame_width * i,
                frame->data[0] + frame->linesize[0] * i,
                frame_width);
        }
        for (j = 0; j < frame_height / 2; j++)
        {
            memcpy(video_decode_buf + frame_width * i + frame_width / 2 * j,
                frame->data[1] + frame->linesize[1] * j,
                frame_width / 2);
        }
        for (k = 0; k < frame_height / 2; k++)
        {
            memcpy(video_decode_buf + frame_width * i + frame_width / 2 * j + frame_width / 2 * k,
                frame->data[2] + frame->linesize[2] * k,
                frame_width / 2);
        }

        if (video_decode_buf)
        {
            memcpy(yuvData, video_decode_buf, video_decode_size);
            free(video_decode_buf);
            video_decode_buf = nullptr;
        }
        else {
            free(yuvData);
            yuvData = nullptr;
        }

    }
    return yuvData;
}


int Manager::encodeAudioTh() {
    I_LOG("encoder audio thread start");
    FILE* pushFile = fopen("C:/Users/97017/Desktop/out_audio.pcm", "wb");
    long long last = 0;
    while (true)
    {
        std::unique_lock<std::mutex> lk(encodePktMtx_a);
        sendpktCond_a.wait(lk, [this]() {return sendList_a.size() > 0; });
        if (stopFlag)
        {
            lk.unlock();
            break;
        }
        auto dstData = sendList_a.front();
        sendList_a.pop();
        lk.unlock();
        long long now = seeker::Time::currentTime();
        if ( dstData.data ) {
            audioSender->send(dstData.data->data[0], dstData.len);
            I_LOG("SEND AUDIO!");
            fwrite(dstData.data->data[0], 1, dstData.len, pushFile);
            free(dstData.data);
            last = now;
        }
    }
    fclose(pushFile);
}