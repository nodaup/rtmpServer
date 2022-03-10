#include "manager.h"
#include "ConvertH264.hpp"

int32_t ssrc1;
int32_t ssrc2;

void writeAdtsHeaders(uint8_t* header, int dataLength, int channel,
    int sample_rate) {

    uint8_t profile = 0x02;  // AAC LC
    uint8_t channelCfg = channel;
    uint32_t packetLength = dataLength + 7;
    uint8_t freqIdx;  // 22.05 KHz

    switch (sample_rate) {
    case 96000:
        freqIdx = 0x00;
        break;
    case 88200:
        freqIdx = 0x01;
        break;
    case 64000:
        freqIdx = 0x02;
        break;
    case 48000:
        freqIdx = 0x03;
        break;
    case 44100:
        freqIdx = 0x04;
        break;
    case 32000:
        freqIdx = 0x05;
        break;
    case 24000:
        freqIdx = 0x06;
        break;
    case 22050:
        freqIdx = 0x07;
        break;
    case 16000:
        freqIdx = 0x08;
        break;
    case 12000:
        freqIdx = 0x09;
        break;
    case 11025:
        freqIdx = 0x0A;
        break;
    case 8000:
        freqIdx = 0x0B;
        break;
    case 7350:
        freqIdx = 0x0C;
        break;
    default:
        std::cout << "addADTStoPacket: unsupported sampleRate: {}" << std::endl;
        break;
    }

    header[0] = (uint8_t)0xFF;
    header[1] = (uint8_t)0xF1;

    header[2] =
        (uint8_t)(((profile - 1) << 6) + (freqIdx << 2) + (channelCfg >> 2));
    header[3] = (uint8_t)(((channelCfg & 3) << 6) + (packetLength >> 11));
    header[4] = (uint8_t)((packetLength & 0x07FF) >> 3);
    header[5] = (uint8_t)(((packetLength & 0x0007) << 5) + 0x1F);
    header[6] = (uint8_t)0xFC;
}

void Manager::asio_audio_thread() {
    //audio
    as_audio = new rtpServer("127.0.0.1", 1234, true);
    //FILE* pushFile = fopen("C:/Users/97017/Desktop/out_audio.aac", "wb");
    //string mixPath = "C:/Users/97017/Desktop/audio_" + std::to_string(seeker::Time::currentTime());
    //writeRecv.open(mixPath + ".pcm", std::ofstream::binary);
    auto recvCallBack2 = [&](uint8_t* pkt, int pktsize, int32_t ssrc, int32_t ts, int32_t seqnum, int32_t pt) {  
        //decode
        uint8_t* adts_hdr = new uint8_t[7];
        int aac_len = pktsize - 4 + 7;
        writeAdtsHeaders(adts_hdr, aac_len - 7, 2, 16000);
        uint8_t* payload = new uint8_t[aac_len];
        for (int i = 0; i < 7; i++) {
            payload[i] = adts_hdr[i];
        }
        for (int i = 0; i < aac_len - 7; i++) {
            payload[i + 7] = pkt[i+4];
        }
        //fwrite(payload, 1, aac_len, pushFile);
        
        adecoder.addPacket(payload, aac_len, ts, seqnum);
        //save frame
        auto recvFrame = adecoder.getFrame();
        if (recvFrame && recvFrame->data[0]) {
            std::unique_lock<std::mutex> lk1(encodePktMtx_a);
            int l = recvFrame->nb_samples * av_get_bytes_per_sample(static_cast<AVSampleFormat>(recvFrame->format)) * recvFrame->channels;
            //I_LOG("recvFrame->channels:{}, recvFrame->format:{}", recvFrame->channels, recvFrame->format);
            //writeRecv.write(reinterpret_cast<const char*>(recvFrame->data[0]), l);
            sendList_a.push(audioFrame{ recvFrame, l });
            ssrc1 = ssrc;
            sendpktCond_a.notify_one();
            lk1.unlock();
        }
        delete[] adts_hdr;
        delete[] payload;
        
    };
    as_audio->setCallBack(recvCallBack2);
    as_audio->start();
    //fclose(pushFile);
    //writeRecv.close();
}

void Manager::asio_audio_thread2() {
    //audio
    as_audio2 = new rtpServer("127.0.0.1", 1236, true);   
    FILE* pushFile = fopen("C:/Users/97017/Desktop/out_audio.aac", "wb");
    auto recvCallBack2 = [&](uint8_t* pkt, int pktsize, int32_t ssrc, int32_t ts, int32_t seqnum, int32_t pt) {
        //decode
        uint8_t* adts_hdr = new uint8_t[7];
        int aac_len = pktsize - 4 + 7;
        writeAdtsHeaders(adts_hdr, aac_len - 7, 2, 16000);
        uint8_t* payload = new uint8_t[aac_len];
        for (int i = 0; i < 7; i++) {
            payload[i] = adts_hdr[i];
        }
        for (int i = 0; i < aac_len - 7; i++) {
            payload[i + 7] = pkt[i + 4];
        }
        fwrite(payload, 1, aac_len, pushFile);
        adecoder2.addPacket(payload, aac_len, ts, seqnum);
        //save frame
        auto recvFrame = adecoder2.getFrame();
        if (recvFrame && recvFrame->data[0]) {
            std::unique_lock<std::mutex> lk1(encodePktMtx_a);
            int l = recvFrame->nb_samples * av_get_bytes_per_sample(static_cast<AVSampleFormat>(recvFrame->format)) * recvFrame->channels;        
            sendList_a2.push(audioFrame{ recvFrame, l });
            ssrc2 = ssrc;
            sendpktCond_a.notify_one();
            lk1.unlock();
        }
        delete[] adts_hdr;
        delete[] payload;
    };
    as_audio2->setCallBack(recvCallBack2);
    as_audio2->start();
    fclose(pushFile);
}

void Manager::video_recv_2() {
    //video
    as2 = new rtpServer("127.0.0.1", 30504, false);
    auto recvCallBack = [&](uint8_t* pkt, int pktsize, int32_t ssrc, int32_t ts, int32_t seqnum, int32_t pt) {
        recvPktMtx2.lock();
        auto temp = new uint8_t[pktsize + 1]();
        memcpy(temp, pkt, pktsize);
        std::pair<uint8_t*, int>second (temp, pktsize);
        std::pair< int32_t, std::pair<uint8_t*, int>> insert (ssrc, second);
        pktList2.push_back(insert);
        recvPktMtx2.unlock();
        pktCond2.notify_one();
    };
    as2->setCallBack(recvCallBack);
    as2->start();
}

int Manager::init() {
    //init video decoder
    decoder = new theia::VideoEngine::imp_87::Decoder87();
    decoder->setPixFmt(AV_PIX_FMT_YUV420P);
    decoder->init();

    decoder2 = new theia::VideoEngine::imp_87::Decoder87();
    decoder2->setPixFmt(AV_PIX_FMT_YUV420P);
    decoder2->init();
    ConvertH264Util* ch = nullptr;

    //init audio decoder

    AudioInfo in;
    in.sample_rate = 16000;
    in.channels = 2;
    in.sample_fmt = (AVSampleFormat)(MyAVSampleFormat::AV_SAMPLE_FMT_FLTP);
    in.channel_layout = av_get_default_channel_layout(2);

    AudioInfo in2;
    in2.sample_rate = 16000;
    in2.channels = 2;
    in2.sample_fmt = (AVSampleFormat)(MyAVSampleFormat::AV_SAMPLE_FMT_FLTP);
    in2.channel_layout = av_get_default_channel_layout(2);

    AudioInfo out;
    out.sample_rate = 22050;
    out.channels = 1;
    out.sample_fmt = (AVSampleFormat)(MyAVSampleFormat::AV_SAMPLE_FMT_S16);
    out.channel_layout = av_get_default_channel_layout(1);

    decoderCodecType = CodecType::AAC;
    adecoder.init(in, out, decoderCodecType, 0);
    adecoder.setDemuxType(MuxType::None);
    adecoder2.init(in2, out, decoderCodecType, 0);
    adecoder2.setDemuxType(MuxType::None);

    //init rtmp server
    netManager = std::make_shared<NetManager>();
    netManager->setRtmpUrl("C:/Users/97017/Desktop/output.flv");//"C:/Users/97017/Desktop/output.flv"

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
    encoderinfo.inSampleRate = 22050;
    encoderinfo.inFormate = MyAVSampleFormat::AV_SAMPLE_FMT_S16;
    encoderinfo.outChannels = 1;
    encoderinfo.outSampleRate = 22050;
    encoderinfo.outFormate = MyAVSampleFormat::AV_SAMPLE_FMT_FLTP;
    encoderinfo.cdtype = CodecType::AAC;
    encoderinfo.muxType = theia::audioEngine::MuxType::ADTS;
    //1024 * 1000 / samplerate (ms
    //video 1000/ Ö¡Êý
    audioSender = std::make_unique<AudioSender>(netManager);
    audioSender->initAudioEncoder(encoderinfo);
    audioSender->setStartTime(startTime);

    if (netManager->rtmpInit(1) == -1) {
        return 0;
    }

    // init video mix
    mixer_file.setCanvasSize(640, 360);
    mixer_file.setCanvasColor(0, 0, 0, 1);
    mixer_file.init();
    //init audio mix
    MixerImpler = std::make_shared<MixerImpl>();
    AudioInfo out_info;
    out_info.setInfo(22050, AV_SAMPLE_FMT_S16, 1);
    MixerImpler->init(out_info, CodecType::AAC);

    std::thread decodeThread{ &Manager::decodeTh, this };
    threadMap["decodeTh"] = std::move(decodeThread);

    std::thread encodeThread{ &Manager::encodeTh, this };
    threadMap["encodeTh"] = std::move(encodeThread);

    std::thread audioEncodeThread{ &Manager::encodeAudioTh, this };
    threadMap["encodeAudioTh"] = std::move(audioEncodeThread);

    std::thread audioRecv{ &Manager::asio_audio_thread, this };
    threadMap["audioRecvTh"] = std::move(audioRecv);

    std::thread audioRecv2{ &Manager::asio_audio_thread2, this };
    threadMap["audio_recv_2"] = std::move(audioRecv2);

    std::thread videoRecv2{ &Manager::video_recv_2, this };
    threadMap["video_recv_2"] = std::move(videoRecv2);

    std::thread decodeThread2{ &Manager::decodeTh2, this };
    threadMap["decodeTh2"] = std::move(decodeThread2);

    //video
    as = new rtpServer("127.0.0.1", 30502, false);
    auto recvCallBack = [&](uint8_t* pkt, int pktsize, int32_t ssrc, int32_t ts, int32_t seqnum, int32_t pt) {
        recvPktMtx.lock();
        auto temp = new uint8_t[pktsize + 1]();
        memcpy(temp, pkt, pktsize);
        std::pair<uint8_t*, int>second(temp, pktsize);
        std::pair< int32_t, std::pair<uint8_t*, int>> insert(ssrc, second);
        pktList.push_back(insert);
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

        unique_lock<mutex> lock{ recvPktMtx };
        pktCond.wait(lock, [&]() {return pktList.size() > 0; });

        std::pair<int32_t, std::pair<uint8_t*, int>> pkt = pktList.front();
        pktList.pop_front();
        lock.unlock();

        decoder->push(pkt.second.first, pkt.second.second, 0);
        AVFrame* frame = av_frame_alloc();
        int rst = decoder->poll(frame);
        if (rst != 0 || frame->width <= 0 || frame->height <= 0) {
            av_frame_free(&frame);
            continue;
        }

        if (rst == 0) {
            //push to encode list
            std::unique_lock<std::mutex> lk1(encodePktMtx);
            sendList.push(mixFrame{ frame, frame->width, frame->height });
            sendpktCond.notify_one();
            lk1.unlock();
        }
     
         //if (rst == 0) {
         //    if (ch == nullptr) {
         //        I_LOG("rst :{}", rst);
         //        ch = new ConvertH264Util(frame->width, frame->height, SAVEFILENAME);
         //    }
         //    I_LOG("write frame 1");
         //    ch->convertFrame(frame);
         //} 
        if (pkt.second.first) delete[]pkt.second.first;
    }
}

int Manager::decodeTh2() {
    while (true)
    {

        unique_lock<mutex> lock{ recvPktMtx2 };
        pktCond2.wait(lock, [&]() {return pktList2.size() > 0; });

        std::pair<int32_t, std::pair<uint8_t*, int>> pkt = pktList2.front();
        pktList2.pop_front();
        lock.unlock();

        decoder2->push(pkt.second.first, pkt.second.second, 0);
        AVFrame* frame = av_frame_alloc();
        int rst = decoder2->poll(frame);
        if (rst != 0 || frame->width <= 0 || frame->height <= 0) {
            av_frame_free(&frame);
            continue;
        }

        if (rst == 0) {
            //push to encode list
            std::unique_lock<std::mutex> lk1(encodePktMtx);
            sendList2.push(mixFrame{ frame, frame->width, frame->height });
            sendpktCond.notify_one();
            lk1.unlock();
        }
        if (pkt.second.first) delete[]pkt.second.first;
    }
}

int Manager::encodeTh() {
    I_LOG("encoder thread start");
    while (true)
    {
        std::unique_lock<std::mutex> lk(encodePktMtx);
        sendpktCond.wait(lk, [this]() {return sendList.size() > 0 && sendList2.size() > 0; });
        if (stopFlag)
        {
            lk.unlock();
            break;
        }
        mixFrame dstData = sendList.front();
        sendList.pop();
        mixFrame dstData2 = sendList2.front();
        sendList2.pop();
        lk.unlock();
        if (dstData.data != NULL && dstData2.data != NULL) {
            //to do mix
            auto yuvData1 = getYUVData(dstData.data);          
            auto yuvData2 = getYUVData(dstData2.data);  
            mixer_file.setPicture(0, 0, 0, 320, 180);
            mixer_file.setPicture(1, 320, 0, 320, 180);
            mixer_file.push(0, yuvData1, dstData.width, dstData.height, AV_PIX_FMT_YUV420P);
            mixer_file.push(1, yuvData2, dstData2.width, dstData2.height, AV_PIX_FMT_YUV420P);
            uint8_t* frameData = mixer_file.getCanvas();
            videoSender->send(frameData, 640, 360, AV_PIX_FMT_RGB24);
            
            free(yuvData);
        }
        if (dstData.data)
            av_frame_free(&dstData.data);
        if (dstData2.data)
            av_frame_free(&dstData2.data);
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
    long long last = 0;
    string mixPath = "C:/Users/97017/Desktop/audio_" + std::to_string(seeker::Time::currentTime());
    writeRecv.open(mixPath + ".pcm", std::ofstream::binary);

    AudioInfo* in_1 = new AudioInfo();
    in_1->setInfo(22050, AV_SAMPLE_FMT_S16, 1);

    while (true)
    {
        std::unique_lock<std::mutex> lk(encodePktMtx_a);
        sendpktCond_a.wait(lk, [this]() {return sendList_a.size() > 0 && sendList_a2.size() > 0; });
        if (stopFlag)
        {
            lk.unlock();
            break;
        }
        auto dstData = sendList_a.front();
        sendList_a.pop();
        auto dstData2 = sendList_a2.front();
        sendList_a2.pop();
        lk.unlock();

        long long now = seeker::Time::currentTime();
        if (dstData.data && dstData2.data) {
            //todo mix
            MixerImpler->updateAudioInfo(ssrc1, *in_1);
            MixerImpler->updateAudioInfo(ssrc2, *in_1);
            MixerImpler->pushData(ssrc1, dstData.data->data[0], dstData.len);
            MixerImpler->pushData(ssrc2, dstData2.data->data[0], dstData2.len);

            AVFrame* mixFrame = MixerImpler->getFrame();
            if (mixFrame != nullptr && mixFrame->data[0] != nullptr) {
                //            I_LOG("get not null frame ---------------");
                int datalength = mixFrame->nb_samples *
                    av_get_bytes_per_sample(static_cast<AVSampleFormat>(mixFrame->format));
                writeRecv.write(reinterpret_cast<const char*>(mixFrame->data[0]), datalength);
                int rst = audioSender->send(mixFrame->data[0], datalength);
                if (rst == 0) {
                    last = now;
                }
            }
            /*if (mixFrame) {
                if (mixFrame->data[0])
                    delete[] mixFrame->data[0];
                av_frame_free(&mixFrame);
            }*/
        }

        if (dstData.data)
            av_frame_free(&dstData.data);
        if (dstData2.data)
            av_frame_free(&dstData2.data);
    }
    writeRecv.close();
}