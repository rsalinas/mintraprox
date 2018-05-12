#include <iostream>

#include "Socks5Server.h"

using namespace std;

int main(int, char **argv)
{
    try {
        Config config{argv[0]};
        if (!config.parse(argv+1)) {
            return EXIT_FAILURE;
        }

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
