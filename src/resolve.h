#pragma once

#include <arpa/inet.h>
#include <string>
#include <vector>

class HostCmd {
  public:
    HostCmd(const std::string& server);
    std::vector<std::string> resolveHost(const std::string& host);
private:
    std::string mDnsServer;
};

std::vector<in_addr_t> resolveAlt(const std::string& dnsServer, const std::string& hostname);
std::vector<in_addr_t> resolveDirect(const std::string& hostname);
