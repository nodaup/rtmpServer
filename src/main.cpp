#include "seeker/logger.h"
#include "INIReader.h"

//void http_start(int port) {
//
//    HttpServer* pServer = new HttpServer("0.0.0.0", port);
//    pServer->start();
//}

int main() {
    seeker::Logger::init();
    printf("start start start\n");
    INIReader reader("application.ini");
    int http_port = std::stoi(reader.Get("this", "http_port", "30503"));




    /*std::thread httpThread{ http_start, http_port };
    httpThread.join();*/
    //UdpReceiver& pReceiver = UdpReceiver::getInstance();

    //pReceiver.start();

    return 0;
}