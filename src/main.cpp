#include "seeker/logger.h"
#include "INIReader.h"
#include "rtpServer.h"
#include "manager.h"
#include "mixManager.h"
INIReader reader("application.ini");


void mix_thread() {
    MixManager* mix = new MixManager();

    mix->init();
}


int main(int argc, char* argv[]) {

    if (reader.ParseError() != 0) {
        std::cout << "Can't load 'application.ini'\n";
        return 1;
    }

    seeker::Logger::init();
    I_LOG("Hello! This is Loki!");

    //ffmpeg -re -i C:/Users/97017/Desktop/3.h264 -vcodec copy -f rtp rtp://127.0.0.1:30502

    std::thread t3(mix_thread);
    t3.join();
}