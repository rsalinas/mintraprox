#include "Config.h"

#include <iostream>
#include "string.h"
#include <map>
#include <functional>

using namespace std;

Config::Config(const char* progName) : mProgName(progName) {

}

void Config::printHelp() {
    clog << mProgName << ":" << endl;
    clog << " -p       - Port" << endl;
    clog << "--dns     - Auxiliary DNS server" << endl;
    clog << "-h|--help - Show this help" << endl;
}

bool Config::parse(char **argv) {
    auto helpFunc = [this](char **&)  {
        printHelp();
        enterServiceLoop = false;
    };
    std::map<std::string, std::function<void(char**&ptr)> > cmds{
        {"-p", [this](char **&ptr)  {
                if (!ptr[1])
                    throw ConfigError{"Missing -p parameter"};
                port = atoi(ptr++[1]);
            }},
        {"--dns", [this](char **&ptr)  {
                if (!ptr[1])
                    throw ConfigError{"Missing --dns parameter"};
                secondaryDns = ptr++[1];
            }},
        {"--help", helpFunc},
        {"-h", helpFunc},
    };

    for (auto ptr = argv; *ptr; ptr++) {
        auto pair = cmds.find(*ptr);
        if (pair != cmds.end()) {
            pair->second(ptr);
        } else {
            clog << "Bad argument: " << *ptr <<endl
                 << "Use --help for help" << endl;
            return false;
        }
    }
    return true;
}

void Config::print() const {
    clog << "Port: " << port << endl;
    clog << "DNS:  " << secondaryDns << endl;
}
