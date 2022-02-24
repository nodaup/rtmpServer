#pragma once
#include "config.h"
#include "seeker/loggerApi.h"
#include <string>
#include <mutex>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include "libavdevice/avdevice.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include "libavutil/imgutils.h"
#include <libavutil/time.h>
#include "libavutil/audio_fifo.h"
#include "libavutil/avstring.h"
#include "libswresample/swresample.h"
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
}

//#include "videocapture/VideoCaptureImpl6.h"


//视频分辨率
struct VideoDefinition {
    int width;
    int height;
    VideoDefinition(int _width, int _height) : width(_width), height(_height) {};

    VideoDefinition getEven() const {
        int evenWidth = ((int)width % 2 == 0) ? (int)width : (int)width - 1;
        int evenHeight = ((int)height % 2 == 0) ? (int)height : (int)height - 1;
        VideoDefinition evenDefinition = { evenWidth, evenHeight };
        return evenDefinition;
    }

    std::string getString() const {
        std::string str = std::to_string(width) + "x" + std::to_string(height);
        return str;
    }

    bool operator==(const VideoDefinition& definition) const {
        if ((definition.width == this->width) && (definition.height == this->height)) {
            return true;
        }
        else {
            return false;
        }
    }

    bool operator!=(const VideoDefinition& definition) const {
        if ((definition.width != this->width) || (definition.height != this->height)) {
            return true;
        }
        else {
            return false;
        }
    }

    bool operator<=(const VideoDefinition& definition) const {
        if ((this->width <= definition.width) && (this->height <= definition.height)) {
            return true;
        }
        else {
            return false;
        }
    }
};

class VideoSenderUtil {
public:
    static VideoDefinition calEncoderSize(const VideoDefinition& captureSize,
        const VideoDefinition& maxDefinition) {
        if (captureSize <= maxDefinition) {
            return captureSize.getEven();
        }
        else {
            double captureProportion = (double)captureSize.width / (double)captureSize.height;
            double maxProportion = (double)maxDefinition.width / (double)maxDefinition.height;
            if (captureProportion > maxProportion) {
                VideoDefinition dstDefinition = { (int)maxDefinition.width,
                                                 (int)((double)maxDefinition.width / captureProportion) };
                return dstDefinition.getEven();
            }
            else {
                VideoDefinition dstDefinition = { (int)((double)maxDefinition.height * captureProportion),
                                                 (int)maxDefinition.height };
                return dstDefinition.getEven();
            }
        }

    }

    static int calBitRate(const VideoDefinition& encoderSize) {
        int bitRate = 0;
        if (encoderSize.height <= 144) {
            bitRate = 100000;
        }
        else if (144 < encoderSize.height && encoderSize.height <= 240) {
            bitRate = 200000;
        }
        else if (240 < encoderSize.height && encoderSize.height <= 360) {
            bitRate = 400000;
        }
        else if (360 < encoderSize.height && encoderSize.height <= 480) {
            bitRate = 600000;
        }
        else if (480 < encoderSize.height && encoderSize.height <= 720) {
            bitRate = 1200000;
        }
        else {
            bitRate = 2000000;
        }
        return bitRate;
    }
};

class VideoRenderUtil {
public:
    //计算图片缩放后的大小：将图片按比例缩放,缩放后长边等于或略大于panel大小（panel内有留白），且width是N的整数倍
    static void calScale2Panel(int srcWidth, int srcHeight, int panelWidth,
        int panelHeight, float& dstWidth, float& dstHeight,
        int N) {
        float srcProportion = (float)srcWidth / (float)srcHeight;
        float panelProportion = (float)panelWidth / (float)panelHeight;
        if (srcProportion >= panelProportion) {
            dstWidth = panelWidth;
            if (panelWidth % N != 0) {
                dstWidth = panelWidth / N * N + N;
            }
            dstHeight = dstWidth / srcProportion;
        }
        else {
            dstHeight = panelHeight;
            dstWidth = dstHeight * srcProportion;
            int tpwidth = dstWidth;
            if (tpwidth % N != 0) {
                dstWidth = tpwidth / N * N + N;
            }
            dstHeight = dstWidth / srcProportion;
        }
    };

    //计算图片缩放后的大小，将图片按比例缩放，缩放后短边等于或略大于panel大小（panel内无留白），且width是N的整数倍
    static void calScale2Panel2(int srcWidth, int srcHeight, int panelWidth, int panelHeight,
        float& dstWidth, float& dstHeight, int N) {
        float srcProportion = (float)srcWidth / (float)srcHeight;
        float panelProportion = (float)panelWidth / (float)panelHeight;
        if (srcProportion > panelProportion) {
            dstHeight = panelHeight;
            dstWidth = dstHeight * srcProportion;
            int tpwidth = dstWidth;
            if (tpwidth % N != 0) {
                dstWidth = (tpwidth / N + 1) * N;
            }
            dstHeight = dstWidth / srcProportion;
        }
        else {
            dstWidth = panelWidth;
            if (panelWidth % N != 0) {
                dstWidth = (panelWidth / N + 1) * N;
            }
            dstHeight = dstWidth / srcProportion;
        }
    }

    //计算图片裁剪后的大小：将图片裁剪后刚好放入panel
    static void calCrop2Panel(int srcWidth, int srcHeight, int panelWidth, int panelHeight,
        int& croppedWidth, int& croppedHeight, int& cropX, int& cropY) {
        if (srcWidth >= panelWidth && srcHeight <= panelHeight) {
            croppedWidth = panelWidth;
            croppedHeight = srcHeight;
            cropX = (srcWidth - panelWidth) / 2;
            cropY = 0;
        }
        else if (srcWidth <= panelWidth && srcHeight >= panelHeight) {
            croppedHeight = panelHeight;
            croppedWidth = srcWidth;
            cropX = 0;
            cropY = (srcHeight - panelHeight) / 2;
        }
        else {
            croppedWidth = panelWidth;
            croppedHeight = panelHeight;
            cropX = (srcWidth - panelWidth) / 2;
            cropY = (srcHeight - panelHeight) / 2;
        }
    };

};