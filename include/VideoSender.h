
#include <netManager.h>
#include <videoEngine/VideoEngine_imp_87.h>
#include "VideoUtil.h"

class VideoSender {
public:
    //��ز���ͨ�����캯������
    VideoSender(std::shared_ptr<NetManager> _netManager);
    ~VideoSender();
    int stop();
    void initEncoder(const VideoDefinition& captureSize, int frameRate);
    int send(uint8_t* data, int width, int height, AVPixelFormat fmt);
    int sendFrame(AVFrame* frame);
    void setStartTime(long long _startTime);
private:
    //netManager
    std::shared_ptr<NetManager> netManager = nullptr;

    //�������
    theia::VideoEngine::imp_87::Encoder87* encoder = nullptr;
    std::unique_ptr<std::mutex> mutex4Encoder = nullptr;
    VideoDefinition maxDefinition = VideoDefinition(640, 480);

    int count = 0;
    long long startTime;
    int frameRate;
    int lastPts = 0;
};