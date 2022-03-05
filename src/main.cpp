#include "seeker/logger.h"
#include "INIReader.h"
#include "rtpServer.h"
#include "manager.h"
#include "mixManager.h"
INIReader reader("application.ini");


//void asio_video_thread() {
//    rtpServer as("127.0.0.1", 30502, false);
//    as.start();
//}
MixManager* mix = new MixManager();

void manager_thread() {
    Manager* m1 = new Manager("127.0.0.1", "127.0.0.1", 30502, 1234);
    mix->addManager(m1);
}

void manager_thread2() {
    Manager* m2 = new Manager("127.0.0.1", "127.0.0.1", 30504, 1236);
    mix->addManager(m2);
}

void mix_thread() {
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

    std::thread t1(manager_thread);
    t1.detach();
    std::thread t2(manager_thread2);
    t2.detach();

    std::thread t3(mix_thread);
    t3.join();
}