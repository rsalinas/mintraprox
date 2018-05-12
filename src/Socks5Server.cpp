#include "Socks5Server.h"

#include <signal.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <thread>
#include <string.h>
#include "log.h"

#include "Socks5Client.h"
#include "Select.h"
#include <arpa/inet.h>
#include "netcommon.h"

using namespace std;

static const int cAlarmInterval = 60;

int syscallToExcept(int result, const std::string& msg) {
    int savedErrno = errno;
    if (result < 0) {
        throw NetworkError{msg + ": " + strerror(savedErrno)};
    }
        //    clog << "Connection from " << ipv4ToDotted(&client.sin_addr) << endl;
    return result;
}


void Socks5Server::setSignalHandlers() {
    signal(SIGPIPE, SIG_IGN);
    sigset_t mask;
    sigemptyset(&mask);
    for (auto signo : {SIGINT, SIGQUIT, SIGALRM, SIGUSR1}) {
        sigaddset(&mask, signo);
    }
    alarm(cAlarmInterval);

    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
        throw NetworkError("sigprocmask");
    mSignalFd = syscallToExcept(signalfd(-1, &mask, 0),
                                "signalfd");
}

Socks5Server::Socks5Server(const Config& config)
    : server_fd(syscallToExcept(socket(AF_INET, SOCK_STREAM, 0), "Could not create socket"))
    , mConfig(config)
{
    setSignalHandlers();

    if (pipe(mPipe) < 0) {
        throw NetworkError("cannot pipe()");
    }

    struct sockaddr_in server;

    server.sin_family = AF_INET;
    server.sin_port = htons(config.port);
    server.sin_addr.s_addr = htonl(config.bindToLocalhost ? INADDR_LOOPBACK : INADDR_ANY);

    static const int opt_val = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof opt_val);

    if (bind(server_fd, (struct sockaddr *) &server, sizeof(server)) != 0)
        throw NetworkError{std::string{"Could not bind socket: "} + strerror(errno)};

    if (listen(server_fd, 128) < 0)
        throw NetworkError{std::string{"Could not listen on socket: "} + strerror(errno)};

    clog << "Server is listening on port " << config.port << endl;
}

Socks5Server::~Socks5Server() {
}

bool Socks5Server::handleAlarm() {
    //    clog << "ALARM!" << endl;
    // TODO check for stale connections etc
    return true;
}

void Socks5Server::sigUSR1() {
    clog << "## Externally resolved hosts in /etc/hosts format" << endl;
    for (const auto& pair : mExternalDnsCache) {
        clog << pair.first << "   ";
        for (const auto& address : pair.second) {
            clog  << " " << ipv4ToDotted(&address);
        }
        clog << endl;

    }
    clog << "# End" << endl;
}

void Socks5Server::handleSignal() {
    struct signalfd_siginfo fdsi;
    int ret;
    if ((ret = read(mSignalFd, &fdsi, sizeof fdsi)) != sizeof fdsi) {
        abort();
    }
    switch (fdsi.ssi_signo) {
    case SIGALRM:
        if (handleAlarm()) {
            alarm(cAlarmInterval);
        }
        break;
    case SIGUSR1:
        sigUSR1();
        break;
    default:
        clog << "Signal "  << strsignal(fdsi.ssi_signo) << endl;
        keepWorking = false;
    }
}


void Socks5Server::handleNewIncomingConnection() {
    struct sockaddr_in clientAddress;
    auto fd = acceptClient(clientAddress);
    auto pipeWriteFd = mPipe[1];
    auto config = mConfig;
    std::thread{[this, fd, pipeWriteFd, clientAddress, config]() {
            try {
                std::unique_ptr<Socks5Client> client{new Socks5Client{config, *this, fd, clientAddress}};
                if (client->negotiate()) {
                    auto clientRaw = client.release();
                    if (write(pipeWriteFd, &clientRaw, sizeof clientRaw) != sizeof clientRaw) {
                        abort();
                    }
                }
            } catch (const std::exception& e) {
                clog << "Exception in negotiation: " << e.what() << endl;
            }
            //FIXME what happens with these threads when we close the main program?!
        }}.detach();

}

void Socks5Server::run() {

    Select select{5 /* seconds */};
    while (keepWorking) {
        select.reset();
        select.watchReadability(server_fd);
        select.watchReadability(mPipe[0]);
        select.watchReadability(mSignalFd);
        for (auto client : mClients) {
            client->addSelectWatches(select);
        }

        switch (select.waitForEvent()) {
        case -1:
            throw NetworkError("select()");
        case 0:
            // timeout
            break;
        default:
            if (select.isReadable(mSignalFd)) {
                handleSignal();
            }
            if (select.isReadable(mPipe[0])) {
                Socks5Client* client;
                if (read(mPipe[0], &client, sizeof client) != sizeof client) {
                    abort();
                }
                mClients.push_back(client);
            }
            if (select.isReadable(server_fd)) {
                handleNewIncomingConnection();
            }
            for (auto it = mClients.begin(); it != mClients.end(); ) {
                auto client = *it;
                if (!client->checkSelectResults(select)) {
                    // EOF, end of story
                    delete client;
                    it = mClients.erase(it);
                    continue;
                }
                ++it;
            }
        }
    }
}

int Socks5Server::acceptClient(struct sockaddr_in& client) {
    socklen_t client_len = sizeof(client);
    return syscallToExcept(accept(server_fd, (struct sockaddr *) &client, &client_len),
                           "Could not accept new connection");
}

void Socks5Server::registerDnsEntry(const std::string& hostname, const std::vector<in_addr_t>& addressList) {
   mExternalDnsCache[hostname] = addressList;
}
