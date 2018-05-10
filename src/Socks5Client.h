#pragma once

#include "Select.h"
#include "socks5.h"
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "Config.h"

class Socks5Client {
public:
    Socks5Client(const Config& config, int fd, const struct sockaddr_in& clientAddress) : mConfig(config), clientEndpoint(fd), mClientAddress(clientAddress) {
    }

    void addSelectWatches(Select& select);
    bool checkSelectResults(const Select& select);
    bool negotiate();

private:
    const Config mConfig;
    enum ConnectionState {
        STATE_RECEIVING_AUTH_OFFER,
        ANSWERING_AUTH,
        RECEIVING_REQUEST,
        RESOLVING_NAME,
        ANSWERING_REQUEST,
        COPYLOOP,
        CLOSING,
        FINISHED,
    };

    bool onData(char ch);

    bool receiveAuthRequest();
    void receiveProxyRequest(ClientRequest& cr);
    std::string readBasicString(size_t length);
    void connectToHostname();
    void connectToIpv4(const ClientRequest& cr);
    void sendReqResponse(ServerResponse::ConnectionState state, const ClientRequest& cr);
    void respondWantedMethod(uint8_t method);

    std::string hostname;
    int port = 0;
    ClientInitialSalutation cis;

    class EndPoint {
    public:
        EndPoint() = default;
        EndPoint(int fd)
            : fd(fd) {
        }
        ~EndPoint();


        bool isDataPending() const {
            return offset != used;
        }
        bool sendData();
        bool attendNewData(EndPoint& other);

        int fd = -1;


    private:

        char buffer[4096];
        size_t offset = 0;
        size_t used = 0;

        size_t nbytes = 0;

    };

    EndPoint peerEndpoint;
    EndPoint clientEndpoint;
    const struct sockaddr_in mClientAddress;
    ConnectionState state = STATE_RECEIVING_AUTH_OFFER;
};



