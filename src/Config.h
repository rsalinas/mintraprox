#pragma once

#include <string>

struct Config {
  int port = 1080;
  std::string secondaryDns;
  Config(char **argv);
};
