#include "Socks5Server.h"
#include <iostream>
#include <fstream>

using namespace std;

std::string readFile(const std::string& fn) {
    std::ifstream t(fn);
    return std::string{(std::istreambuf_iterator<char>(t)),
                std::istreambuf_iterator<char>()};
}

int main(int, char **argv)
{
    try {
        Config config{argv[0]};
        config.parse(argv+1);

        if (config.enterServiceLoop) {
            config.print();
            Socks5Server{config}.run();
        }
        return EXIT_SUCCESS;
    } catch (const ConfigError& e) {
        clog << "Arg error: " << e.message << endl;
        return EXIT_FAILURE;
    } catch (const std::exception& e) {
        clog << "Exception: " << e.what() << endl;
        return EXIT_FAILURE;
    }
}
