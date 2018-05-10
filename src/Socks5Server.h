#pragma once

#include <vector>
#include "Config.h"

class Socks5Client;

class Socks5Server {
public:
    Socks5Server(const Config& config);
    ~Socks5Server();
    void run();

private:
    int acceptClient(struct sockaddr_in& client);
    int mSignalFd;
    void setSignalHandlers();
    void handleNewIncomingConnection();
    bool handleAlarm();
    void handleSignal();
    const int server_fd;
    std::vector<Socks5Client*> mClients;
    int mPipe[2];
    bool keepWorking = true;
    const Config mConfig;
};
