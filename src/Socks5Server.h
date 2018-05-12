#pragma once

#include <vector>
#include <map>
#include <arpa/inet.h>

#include "Config.h"

class Socks5Client;

class Socks5Server {
public:
    Socks5Server(const Config& config);
    ~Socks5Server();
    void run();
    void registerDnsEntry(const std::string& hostname, const std::vector<in_addr_t>& addressList);

private:
    int acceptClient(struct sockaddr_in& client);
    int mSignalFd;
    void setSignalHandlers();
    void handleNewIncomingConnection();
    bool handleAlarm();
    void handleSignal();
    void sigUSR1();
    const int server_fd;
    std::vector<Socks5Client*> mClients;
    int mPipe[2];
    bool keepWorking = true;
    const Config mConfig;
    std::map<std::string, std::vector<in_addr_t>> mExternalDnsCache;
};
