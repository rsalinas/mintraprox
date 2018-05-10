#include "resolve.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <iostream>
#include <memory>
#include <chrono>

using namespace std;

std::string resolveHost(const char* name) {

    struct hostent *hp = gethostbyname(name);

    if (hp == NULL) {
        clog << "gethostbyname() failed" << endl;
        return "";
    } else {
        printf("%s = ", hp->h_name);
        unsigned int i=0;
        while ( hp -> h_addr_list[i] != NULL) {
            printf( "%s ", inet_ntoa( *( struct in_addr*)( hp -> h_addr_list[i])));
            i++;
        }
        printf("\n");
    }
    return "";

}

HostCmd::HostCmd(const std::string& resolver) : mResolver(resolver) {
}

std::vector<std::string> HostCmd::resolveHost(const std::string& host) {

    auto command = std::string{"host -W 1 "} + host + " " + mResolver;
    std::unique_ptr<FILE, void(*)(FILE*)> f{popen(command.c_str(), "r"), [](FILE* f){pclose(f);}};
    char buffer[80];

    std::vector<std::string> ret;
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
    using namespace std::chrono;
    high_resolution_clock::time_point t1 = high_resolution_clock::now();

    HostCmd resolver{dnsServer};
    for (const auto& r : resolver.resolveHost(hostname)) {
        ret.push_back(inet_addr(r.c_str()));
    }
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    double dif = duration_cast<milliseconds>( t2 - t1 ).count();
    fprintf (stderr, "Cost of DNS resolving: %lf\n", dif );
    return ret;
}
