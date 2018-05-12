#include "Socks5Client.h"

#include <algorithm>

#include <fcntl.h>
#include <iostream>
#include <map>
#include <mutex>
#include <netdb.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <vector>

#include "log.h"
#include "resolve.h"
#include "Socks5Server.h"
#include "netcommon.h"

using namespace std;

bool setLinger(int fd, bool enabled, int seconds);

bool Socks5Client::receiveAuthRequest() {
    auto ret = recv(clientEndpoint.fd, &cis, sizeof cis + 16, 0);
    if (size_t(ret) != sizeof(cis) + cis.supportedMethodCount) {
        throw ClientProtocolException("Wrong ClientInitialSalutation");
    }
    if (cis.version != 0x05) {
        throw ClientProtocolException("Wrong client version, 5 expected");
    }
    if (cis.supportedMethodCount > ret - 2) {
        throw ClientProtocolException("Wrong method count");
    }
    std::vector<uint8_t> clientSupportedMethods{cis.supportedMethods, cis.supportedMethods + cis.supportedMethodCount};
    if (std::find(clientSupportedMethods.begin(), clientSupportedMethods.end(), uint8_t(0)) == clientSupportedMethods.end())  {
        clog << "Client doesn't support noauth" << endl;
        return false;
    }
    return true;
}

std::string Socks5Client::readBasicString(size_t length) {
    char namebuf[length+1];
    namebuf[length] = 0;
    if (recv(clientEndpoint.fd, namebuf, length, 0) != int(length)) {
        throw ClientProtocolException("could not read domain name");
    }
    return std::string{namebuf, length};
}

void Socks5Client::receiveProxyRequest(ClientRequest& cr) {
    int ret;
    if ((ret = recv(clientEndpoint.fd, &cr, 4, 0)) != 4) {
        auto lasterrno = errno;
        clog << "secondStage Size ret " << ret << " error: " << strerror(lasterrno) << endl;
        throw ClientProtocolException("Wrong ClientRequest size");
    }

    switch (cr.addressType) {
    case ClientRequest::ADDRESSTYPE_IPV4:
        if (recv(clientEndpoint.fd, cr.ipv4, 4, 0) != 4) {
            throw ClientProtocolException("could not read ipv4 address");
        }
        break;
    case ClientRequest::ADDRESSTYPE_DOMAINNAME:

        if (recv(clientEndpoint.fd, &cr.name.length, sizeof cr.name.length, 0) != 1) {
            throw ClientProtocolException("could not read ipv4 address");
        }
        hostname = readBasicString(cr.name.length);

        break;
    case ClientRequest::ADDRESSTYPE_IPV6:
        if (recv(clientEndpoint.fd, cr.ipv6, sizeof cr.ipv6, 0) != sizeof cr.ipv6) {
            throw ClientProtocolException("could not read ipv4 address");
        }
        break;
    }
    if (recv(clientEndpoint.fd, &cr.port, sizeof cr.port, 0) != sizeof cr.port) {
        throw ClientProtocolException("could not read ipv4 address");
    }
    port = ntohs(cr.port);
}


class AddressCache {
public:
    std::vector<in_addr_t> get(const std::string& hostname) {
        std::lock_guard<std::mutex> lock(mMutex);
        auto it = mCache.find(hostname);
        return it != mCache.end() ? it->second : std::vector<in_addr_t>();
    }
    void set(const std::string& hostname, const std::vector<in_addr_t>& address) {
        std::lock_guard<std::mutex> lock(mMutex);
        mCache[hostname] = address;
    }

private:
    std::mutex mMutex;
    std::map<std::string, std::vector<in_addr_t>> mCache;
};


void Socks5Client::connectToHostname() {
    if (hostname.size() == 0) {
        throw ClientProtocolException("no host name");
    }

    //    clog << ipv4ToDotted(&mClientAddress.sin_addr) << ":" << mClientAddress.sin_port << ": Connecting to host: " << hostname << ":" << port << endl;
    static AddressCache addressCache;

    std::vector<in_addr_t> addrs = addressCache.get(hostname);
    if (addrs.size()) {
        //        clog << "Cache hit: " << hostname << endl;
    } else {
        addrs = resolveDirect(hostname);
        if (addrs.empty()) {
            addrs = resolveAlt(mConfig.secondaryDns, hostname);
            if (addrs.size()) {
                mS5server.registerDnsEntry(hostname, addrs);

            }
        }
        if (!addrs.empty()) {
            //            clog << "Hostname resolved: " << hostname << " -> " << ipv4ToDotted(&addr);
        }
    }

    if (addrs.empty()) {
        //        clog << "Unknown host " << hostname << endl;
        throw WrongRequestException{ServerResponse::STATE_HOST_UNREACHABLE, "Host unknown: " + hostname};
    }
    addressCache.set(hostname, addrs);
    if ((peerEndpoint.fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        throw WrongRequestException{ServerResponse::STATE_GENERAL_SOCKS_SERVER_FAILURE, "cannot create socket"};
    }

    struct sockaddr_in remoteAddr;
    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_port = htons(port);
    bzero(&remoteAddr.sin_zero, 8);
    for (auto address : addrs) {
        remoteAddr.sin_addr = *reinterpret_cast<struct in_addr *>(&address);
        if (connect(peerEndpoint.fd, reinterpret_cast<const struct sockaddr *>(&remoteAddr), sizeof remoteAddr) == -1) {
            perror("connect()");
            continue;    // try next address
        }
        setLinger(peerEndpoint.fd, true, 3);
        return;
    }
    throw WrongRequestException{ServerResponse::STATE_HOST_UNREACHABLE, std::string{"cannot connect to host: "} + strerror(errno)};
}

void Socks5Client::connectToIpv4(const ClientRequest& cr) {

    clog << "Connecting to host: " << ipv4ToDotted(cr.ipv4) << ":" << port << endl;
    if ((peerEndpoint.fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        throw WrongRequestException{ServerResponse::STATE_GENERAL_SOCKS_SERVER_FAILURE, "cannot create socket"};
    }

    struct sockaddr_in their_addr;
    their_addr.sin_family = AF_INET;
    their_addr.sin_port = htons(port);
    their_addr.sin_addr = *reinterpret_cast<const struct in_addr *>(cr.ipv4);

    bzero(&their_addr.sin_zero, 8);

    if (connect(peerEndpoint.fd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1) {
        throw WrongRequestException{ServerResponse::STATE_HOST_UNREACHABLE, std::string{"cannot connect to host: "} + strerror(errno)};
    }
    setLinger(peerEndpoint.fd, true, 3);
}

bool Socks5Client::negotiate() {
    if (!receiveAuthRequest()) {
        respondWantedMethod(0xFF);
        return false;
    }
    respondWantedMethod(0x00);  //TODO support more methods, other than noauth
    ClientRequest cr;
    try {
        receiveProxyRequest(cr);
        switch (cr.addressType) {
        case ClientRequest::ADDRESSTYPE_DOMAINNAME:
            connectToHostname();
            break;
        case ClientRequest::ADDRESSTYPE_IPV4:
            connectToIpv4(cr);
            break;
        default:
            sendReqResponse(ServerResponse::STATE_ADDRESS_TYPE_NOT_SUPPORTED, cr);
        }
    } catch (const WrongRequestException& e) {
        clog << "WrongRequestException: " << e.what() << endl;
        sendReqResponse(e.getSocks5state(), cr);
        return false;
    }
    sendReqResponse(ServerResponse::STATE_SUCCESS, cr);
    if (fcntl(clientEndpoint.fd, F_SETFL, fcntl(clientEndpoint.fd, F_GETFL, 0)| O_NONBLOCK) < 0) {
        throw NetworkError{"Cannot set non-block on socket"};
    }
    setLinger(clientEndpoint.fd, true, 3);
    return true;
}

bool setLinger(int fd, bool enabled, int seconds) {
    if (fd < 0) {
        clog << "bad fd " << fd << endl;
        return false;
    }
    struct linger so_linger;
    so_linger.l_onoff = enabled;
    so_linger.l_linger = seconds;
    if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &so_linger, sizeof so_linger) < 0) {
        clog << "Cannot set linger" << endl;
        return false;
    }
    return true;
}

void Socks5Client::sendReqResponse(ServerResponse::ConnectionState state, const ClientRequest& cr) {
    ServerResponse sr;
    sr.state = state;
    sr.addressType = cr.addressType;
    if (send(clientEndpoint.fd, &sr, 4, 0) != 4) {
        throw ClientProtocolException("error sending first part of response");
    }
    switch (cr.addressType) {
    case ClientRequest::ADDRESSTYPE_IPV4:
        send(clientEndpoint.fd, cr.ipv4, sizeof cr.ipv4, 0);
        break;
    case ClientRequest::ADDRESSTYPE_IPV6:
        send(clientEndpoint.fd, cr.ipv6, sizeof cr.ipv6, 0);
        break;
    case ClientRequest::ADDRESSTYPE_DOMAINNAME:
        send(clientEndpoint.fd, &cr.name.length, 1, 0);
        send(clientEndpoint.fd, hostname.c_str(), cr.name.length, 0);
        break;
    default:
        abort();
    }
    send(clientEndpoint.fd, &cr.port, sizeof cr.port, 0);
}

void Socks5Client::respondWantedMethod(uint8_t method) {
    ServerSalutationResponse ssr;
    ssr.chosenMethod = method;
    send(clientEndpoint.fd, &ssr, sizeof ssr, 0);
}

void Socks5Client::addSelectWatches(Select& select) {
    select.watchReadability(peerEndpoint.fd);
    if (peerEndpoint.isDataPending())
        select.watchWriteability(peerEndpoint.fd);

   select.watchReadability(clientEndpoint.fd);
    if (clientEndpoint.isDataPending())
        select.watchWriteability(clientEndpoint.fd);
}

bool Socks5Client::checkSelectResults(const Select& select) {
    // Write first, maybe it empties the buffer and then we can read again
    // Note that the buffers can be partially written but not incrementally read
    if (select.isWriteable(peerEndpoint.fd) && !peerEndpoint.sendData()) {
        return false;
    }
    if (select.isWriteable(clientEndpoint.fd)  && !clientEndpoint.sendData()) {
        return false;
    }

    if (select.isReadable(peerEndpoint.fd) && !peerEndpoint.attendNewData(clientEndpoint)) {
        return false;
    }
    if (select.isReadable(clientEndpoint.fd) && !clientEndpoint.attendNewData(peerEndpoint)) {
        return false;
    }
    return true;

}

Socks5Client::EndPoint::~EndPoint() {
    if (fd >=0)
        close(fd);
}

bool Socks5Client::EndPoint::attendNewData(EndPoint& other) {
    if (other.isDataPending())
        return true;

    int nbytes = recv(fd, other.buffer, sizeof other.buffer, 0);
    if (nbytes <= 0) {
        return false;
    }
    other.used = nbytes;
    other.offset = 0;

    this->nbytes += nbytes;
    if (!other.sendData()) {
        return false;
    }

    return true;
}

bool Socks5Client::EndPoint::sendData() {
    int res = send(fd, buffer + offset, used - offset, 0);
    if (res < 0 ) {
        perror("send");
        return false;
    }
    offset += res;
    return true;
}


bool Socks5Client::onData(char ch) {
    //this is dead code at the moment
    int n= 0;
    int pos = 0;
    ClientInitialSalutation cr;
    switch (state) {
    case STATE_RECEIVING_AUTH_OFFER:
        switch (n++) {
        case ClientInitialSalutation::VERSION:
            cr.version = ch;
            break;
        case ClientInitialSalutation::SUPPORTEDMETHODCOUNT:
            cr.supportedMethodCount = ch;
            break;
        case ClientInitialSalutation::SUPPORTEDMETHODS:
            cr.supportedMethods[pos++] = ch;
            if (pos == cr.supportedMethodCount) {
                state = ANSWERING_AUTH;
//                answer();

            }

            break;
        default:
            break;
        }
        break;
    case ANSWERING_AUTH:
        break;
    case RECEIVING_REQUEST:
        break;
    case RESOLVING_NAME:
        break;
    case ANSWERING_REQUEST:
        break;
    case COPYLOOP:
        break;
    case CLOSING:
        break;
    case FINISHED:
        break;
    default:
        break;
    }
    return false;
}
