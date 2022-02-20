#include "seeker/logger.h"
#include "INIReader.h"
#include "rtpServer.h"

INIReader reader("application.ini");


void asio_video_thread() {
    rtpServer as("127.0.0.1", 30502, false);
    as.start();
}


int main(int argc, char* argv[]) {

    if (reader.ParseError() != 0) {
        std::cout << "Can't load 'application.ini'\n";
        return 1;
    } 

    seeker::Logger::init();
    I_LOG("Hello! This is Loki!");
   
    //ffmpeg -re -i C:/Users/97017/Desktop/3.h264 -vcodec copy -f rtp rtp://127.0.0.1:30502


    std::thread t2(asio_video_thread);
    t2.join();

    
}