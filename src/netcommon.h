#pragma once

#include <string>

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


