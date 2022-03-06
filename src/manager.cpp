#include "manager.h"
#include "ConvertH264.hpp"


Manager::Manager(std::string vIP, std::string aIP, int vPort, int aPort) {
    videoIP = vIP;
    videoPort = vPort;
    audioIP = aIP;
    audioPort = aPort;
    init();
}

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
    as_audio = new rtpServer(audioIP, audioPort, true);
    FILE* pushFile = fopen("C:/Users/97017/Desktop/out_audio.aac", "wb");
    string mixPath = "C:/Users/97017/Desktop/audio_" + std::to_string(seeker::Time::currentTime());
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
        fwrite(payload, 1, aac_len, pushFile);

        adecoder.addPacket(payload, aac_len, ts, seqnum);
        //save frame
        auto recvFrame = adecoder.getFrame();
        I_LOG("222!");
        if (recvFrame && recvFrame->data[0]) {
            std::unique_lock<std::mutex> lk1(encodePktMtx_a);
            int l = recvFrame->nb_samples * av_get_bytes_per_sample(static_cast<AVSampleFormat>(recvFrame->format)) * recvFrame->channels;
            I_LOG("recvFrame->channels:{}, recvFrame->format:{}", recvFrame->channels, recvFrame->format);
            //writeRecv.write(reinterpret_cast<const char*>(recvFrame->data[0]), l);
            sendList_a.push(audioFrame{ recvFrame, l });
            //sendpktCond_a.notify_one();
            lk1.unlock();
        }
        delete[] adts_hdr;
        delete[] payload;

    };
    as_audio->setCallBack(recvCallBack2);
    as_audio->start();
    fclose(pushFile);
    //writeRecv.close();
}

int Manager::init() {
    //init video decoder
    decoder = new theia::VideoEngine::imp_87::Decoder87();
    decoder->setPixFmt(AV_PIX_FMT_YUV420P);
    decoder->init();
    ConvertH264Util* ch = nullptr;

    //init audio decoder

    CoderInfo decoderinfo;
    decoderinfo.outChannels = 1;
    decoderinfo.outSampleRate = 32000;
    decoderinfo.outFormate = MyAVSampleFormat::AV_SAMPLE_FMT_S16;

    AudioInfo in;
    in.sample_rate = 16000;
    in.channels = 2;
    in.sample_fmt = (AVSampleFormat)(MyAVSampleFormat::AV_SAMPLE_FMT_FLTP);
    in.channel_layout = av_get_default_channel_layout(2);

    AudioInfo out;
    out.sample_rate = 22050;
    out.channels = 1;
    out.sample_fmt = (AVSampleFormat)(MyAVSampleFormat::AV_SAMPLE_FMT_S16);
    out.channel_layout = av_get_default_channel_layout(1);

    decoderCodecType = CodecType::AAC;
    adecoder.init(in, out, decoderCodecType, 0);
    adecoder.setDemuxType(MuxType::None);

    std::thread decodeThread{ &Manager::decodeTh, this };
    threadMap["decodeTh"] = std::move(decodeThread);

    std::thread audioRecv{ &Manager::asio_audio_thread, this };
    threadMap["audioRecvTh"] = std::move(audioRecv);

    //video
    as = new rtpServer(videoIP, videoPort, false);
    auto recvCallBack = [&](uint8_t* pkt, int pktsize, int32_t ssrc, int32_t ts, int32_t seqnum, int32_t pt) {
        recvPktMtx.lock();
        auto temp = new uint8_t[pktsize + 1]();
        memcpy(temp, pkt, pktsize);
        auto it = pktList.find(ssrc);
        insertPktMtx.lock();
        if (it != pktList.end()) {
            it->second.data = temp;
            it->second.len = pktsize;
        }
        else {
            recvFrame f;
            f.data = temp;
            f.len = pktsize;
            pktList.insert(std::pair<int32_t, recvFrame>(ssrc, f));
        }
        insertPktMtx.unlock();
        //I_LOG("len:{}", pktsize);
        flag = true;
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
        pktCond.wait(lock, [&]() {return pktList.size() > 0 && flag == true; });
        for (auto iter = pktList.begin(); iter != pktList.end(); iter++) {
            if (iter->second.len > 0) {
                
                unique_lock<mutex> lk{ insertPktMtx };
                auto temp = new uint8_t[iter->second.len + 1]();
                memcpy(temp, iter->second.data, iter->second.len);
                lk.unlock();

                decoder->push(temp, iter->second.len, 0);
                AVFrame* frame = av_frame_alloc();
                int rst = decoder->poll(frame);
                if (rst != 0 || frame->width <= 0 || frame->height <= 0) {
                    I_LOG("fail");
                    av_frame_free(&frame);
                    continue;
                }
                if (rst == 0) {
                    I_LOG("SUCC");
                    if (ch == nullptr) {
                        ch = new ConvertH264Util(frame->width, frame->height, SAVEFILENAME);
                    }
                    ch->convertFrame(frame);
                    
                    cb(iter->first, frame, frame->width, frame->height);
                }
            }         
        }
        lock.unlock();
        flag = false;
    }
}

void Manager::setDecodeCB(decodeCB _callBack) {
    cb = _callBack;
}
