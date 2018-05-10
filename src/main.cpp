#include "Socks5Server.h"
#include <iostream>
#include <fstream>
#include "resolve.h"
#include "string.h"

using namespace std;

std::string readFile(const std::string& fn) {
    std::ifstream t(fn);
    return std::string{(std::istreambuf_iterator<char>(t)),
                     std::istreambuf_iterator<char>()};
}

struct ConfigError {
   std::string message;
};


Config::Config(char **argv) {
    for (auto ptr = argv; *ptr; ptr++) {
        if (strcmp(*ptr, "-p") == 0) {
            if (!ptr[1]) {
                throw ConfigError{"Missing -p parameter"};
            }
            port = atoi(ptr[1]);
        }
        if (strcmp(*ptr, "--dns") == 0) {
            if (!ptr[1]) {
                throw ConfigError{"Missing --dns parameter"};
            }
            secondaryDns = ptr[1];
        }
    }
}


int main(int argc, char **argv)
{

    try {
        Config config{argv};

        clog << "Port: " << config.port << endl;
        clog << "DNS:  " << config.secondaryDns << endl;

        Socks5Server{config}.run();
        return EXIT_SUCCESS;
    } catch (const ConfigError& e) {
        clog << "Arg error: " << e.message << endl;
        return EXIT_FAILURE;
    } catch (const std::exception& e) {
        clog << "Exception: " << e.what() << endl;
        return EXIT_FAILURE;
    }
}
