#include "resolve.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <iostream>
#include <memory>

using namespace std;

HostCmd::HostCmd(const std::string& resolver)
    : mDnsServer(resolver) {
}

std::vector<std::string> HostCmd::resolveHost(const std::string& host) {
    std::vector<std::string> ret;
    if (mDnsServer.empty()) {
        clog << "Error: no auxiliary DNS server defined" << endl;
        return ret;
    }
    auto command = std::string{"host -W 1 "} + host + " " + mDnsServer;
    std::unique_ptr<FILE, void(*)(FILE*)> f{popen(command.c_str(), "r"), [](FILE* f){pclose(f);}};
    char buffer[80];


    while (!feof(f.get())) {
        if (!fgets(buffer, sizeof buffer, f.get()))
            break;
        std::string result{buffer};

        // Now we have to remove the text after this token:
        static const std::string token = " has address ";
        auto pos = result.find(token);
        if (pos != result.npos){
            ret.push_back(result.substr(pos + token.length()));
        }
    }
    return ret;
}


std::vector<in_addr_t> resolveDirect(const std::string& hostname) {
    const hostent* he = gethostbyname(hostname.c_str());
    std::vector<in_addr_t> ret;
    if (!he) {
        clog << "Error in direct resolve: " << hostname << endl;
        return ret;
    }
    for (auto ptr = reinterpret_cast<struct in_addr**>(he->h_addr_list); *ptr; ptr++) {
        ret.push_back(*reinterpret_cast<in_addr_t*>(*ptr));
    }
    return ret ;
}

std::vector<in_addr_t> resolveAlt(const std::string& dnsServer, const std::string& hostname) {
    std::vector<in_addr_t> ret;
    HostCmd resolver{dnsServer};
    for (const auto& r : resolver.resolveHost(hostname)) {
        ret.push_back(inet_addr(r.c_str()));
    }
    return ret;
}
