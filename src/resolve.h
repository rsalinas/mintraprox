#pragma once

#include <arpa/inet.h>
#include <string>
#include <vector>

std::string resolveHost(const char* name);

class Popen {
public:
    Popen();

};


class HostCmd {
  public:
    HostCmd(const std::string& resolver);

    std::vector<std::string> resolveHost(const std::string& host);
private:
    std::string mResolver;
};



std::vector<in_addr_t> resolveAlt(const std::string& dnsServer, const std::string& hostname);
std::vector<in_addr_t> resolveDirect(const std::string& hostname);
