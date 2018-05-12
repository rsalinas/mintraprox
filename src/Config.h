#pragma once

#include <string>

struct Config {
    Config(const char* progName);

    const char*const mProgName;
    bool enterServiceLoop = true;
    int port = 1080;
    std::string secondaryDns;

    bool parse(char **argv);
    void printHelp();
    void print() const;
};

struct ConfigError {
    std::string message;
};
