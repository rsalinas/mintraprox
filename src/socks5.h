#pragma once

#include <string>

struct ClientInitialSalutation {
    uint8_t version;
    uint8_t supportedMethodCount;
    uint8_t supportedMethods[0];
    enum State {
      VERSION, SUPPORTEDMETHODCOUNT, SUPPORTEDMETHODS
    };
};

struct ServerSalutationResponse {
    uint8_t version = 5;
    uint8_t chosenMethod = 0;
};

struct ClientRequest {
    //    campo 1: número de versión socks, 1 byte, debe ser 0x05 para esta versión
    uint8_t version;
    enum CommandType {
        CMD_STREAM = 1,
        CMD_BINDING = 2,
        CMD_UDP = 3,
    };
    //    campo 2: código de comando, 1 byte:-
    //      0x01 = establecer una conexión stream tcp/ip
    //      0x02 = establecer  un enlazado(binding) de puerto tcp/ip
    //      0x03 = asociar un puerto udp
    uint8_t cmd;
    //    campo 3: reservado, debe ser 0x00
    uint8_t reserved;


    //    campo 4: tipo de dirección, 1 byte:-
    //      0x01 = dirección IP V4 (el campo de direcciones tiene una longitud de 4 bytes)
    //      0x03 = Nombre de dominio (el campo dirección es variable)
    //      0x04 = dirección IP V6 (el campo de direcciones tiene una longitud de 16 bytes)

    enum AddressType {
        ADDRESSTYPE_IPV4 = 1,
        ADDRESSTYPE_DOMAINNAME = 3,
        ADDRESSTYPE_IPV6 = 4,
    };

    uint8_t addressType;

    //    campo 5: dirección destino, 4/16 bytes o longitud de nombre 1+dominio.
    //      Si el tipo de dirección es 0x03 entonces la dirección consiste en un byte de longitud seguido del nombre de dominio.
    union  {
        char ipv4[4];
        char ipv6[16];
        struct {
            uint8_t length;
        } name;
    };
    //    campo 6: número de puerto en el orden de bytes de la red, 2 bytes
    uint16_t port;
};

struct ServerResponse {
    enum ConnectionState {
        STATE_SUCCESS = 0,
        STATE_GENERAL_SOCKS_SERVER_FAILURE,
        STATE_FORBIDDEN_BY_RULESET,
        STATE_NETWORK_UNREACHABLE,
        STATE_HOST_UNREACHABLE,
        STATE_REFUSED,
        STATE_TTL_EXPIRED,
        STATE_COMMAND_NOT_SUPPORTED,
        STATE_ADDRESS_TYPE_NOT_SUPPORTED,
    };
    //    campo 1: versión de protocolo socks, 1 byte, 0x05 para esta versión
    uint8_t version = 5;
    //    campo 2: estado, 1 byte:-
    //      0x00 = petición concedida,
    //      0x01 = fallo general,
    //      0x02 = la conexión no se permitió por el conjunto de reglas(ruleset)
    //      0x03 = red inalcanzable
    //      0x04 = host inalcanzable
    //      0x05 = conexión rechazada por el host destino
    //      0x06 = TTL expirado
    //      0x07 = comando no soportado/ error de protocolo
    //      0x08 = tipo de dirección no soportado

    uint8_t state;
    //    campo 3: reservado, 0x00

    uint8_t reserved = 0;
    //    campo 4: tipo de dirección, 1 byte:-
    //      0x01 = dirección IP V4 (el campo de direcciones tiene una longitud de 4 bytes)
    //      0x03 = Nombre de dominio (el campo dirección es variable)
    //      0x04 = dirección IP V6 (el campo de direcciones tiene una longitud de 16 bytes)

    uint8_t addressType = 3;
    //    campo 5: dirección destino, 4/16 bytes o longitud de nombre 1+dominio.
    //      Si el tipo de dirección es 0x03 entonces la dirección consiste en un byte de longitud seguido del nombre de dominio.

    union  {
        char ipv4[4];
        char ipv6[16];
        struct {
            uint8_t length;
        } name;
    };
    //    campo 6: número de puerto en el orden de bytes de la red, 2 bytes
    uint16_t port;
};


class NetworkError : public std::exception  {
public:
    NetworkError(const char* s) : mMessage(s) {

    }
    NetworkError(const std::string& s) : mMessage(s) {

    }
    const char* what() const noexcept {
        return mMessage.c_str();
    }

private:
    const std::string mMessage;
};

class ClientProtocolException : public std::exception  {
public:
    ClientProtocolException(const char* s) : mMessage(s) {

    }
    ClientProtocolException(const std::string& s) : mMessage(s) {

    }
    const char* what() const noexcept {
        return mMessage.c_str();
    }

private:
    const std::string mMessage;
};


class WrongRequestException : public std::exception  {
public:
    WrongRequestException(ServerResponse::ConnectionState socks5state, const char* s)
        : mSocks5state(socks5state)
        , mMessage(s) {

    }
    WrongRequestException(ServerResponse::ConnectionState socks5state, const std::string& s)
        : mSocks5state(socks5state)
        , mMessage(s) {

    }
    const char* what() const noexcept {
        return mMessage.c_str();
    }

    ServerResponse::ConnectionState getSocks5state() const {
        return mSocks5state;
    }

private:
    const ServerResponse::ConnectionState  mSocks5state;
    const std::string mMessage;
};
