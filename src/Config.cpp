#include "Config.h"

#include <iostream>
#include "string.h"
#include <map>
#include <functional>
#include <arpa/inet.h>

using namespace std;

Config::Config(const char* progName)
    : mProgName(progName) {
}

void Config::printHelp() {
    clog << mProgName << ":" << endl;
    clog << "-v             - Increase verbosity" << endl;
    clog << "-p <port>      - Port" << endl;
    clog << "-g             - Do not restrict incoming connections to localhost" << endl;
    clog << "--dns <server> - Auxiliary DNS server" << endl;
    clog << "-h|--help      - Show this help" << endl;
}

bool Config::parse(char **argv) {
    static const auto helpFunc = [this](char **&)  {
        printHelp();
        enterServiceLoop = false;
    };
    static const std::map<std::string, std::function<void(char**&ptr)> > cmds{
        {"-p", [this](char **&ptr) {
                if (!ptr[1])
                    throw ConfigError{"Missing -p parameter"};
                port = atoi(ptr[1]);
                if (!port || port >= 65536)
                    throw ConfigError{"Wrong port number given: -p " + std::string(ptr[1])};
                ptr++;
            }},
        {"--dns", [this](char **&ptr) {
                if (!ptr[1])
                    throw ConfigError{"Missing --dns parameter"};
                secondaryDns = ptr++[1];
                struct in_addr inp;
                if (!inet_aton(secondaryDns.c_str(), &inp)) {
                    throw ConfigError{"Wrong IP address given: --dns " + secondaryDns};
                }
            }},
        {"-v", [this](char **&) {
            verbosity++;
        }},
        {"-g", [this](char **&) {
            bindToLocalhost=false;
        }},
        {"--help", helpFunc},
        {"-h", helpFunc},
    };

    for (auto ptr = argv; *ptr; ptr++) {
        auto pair = cmds.find(*ptr);
        if (pair != cmds.end()) {
            pair->second(ptr);
        } else {
            clog << "Bad argument «" << *ptr << "». Use --help." << endl;
            return false;
        }
    }
    return true;
}

void Config::print() const {
    clog << "Verbosity:  " << verbosity << endl;
    clog << "Port:       " << port << endl;
    clog << "DNS:        " << secondaryDns << endl;
    clog << "Only local: " << (bindToLocalhost ? "yes" : "no" ) << endl;
}
