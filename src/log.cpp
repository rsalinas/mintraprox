#include "log.h"

#include <arpa/inet.h>

std::string ipv4ToDotted(const void* addr) {
    char buf[4*3+3+1];
    inet_ntop(AF_INET, addr, buf, sizeof buf);
    return std::string{buf};
}
