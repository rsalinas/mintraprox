#pragma once

#include <sys/select.h>

class Select {
public:
    Select(time_t timeout) noexcept
        : mTimeoutTv{timeout, suseconds_t(0)} {
        reset();
    }
    void reset() noexcept {
        FD_ZERO(&rfds);
        FD_ZERO(&wfds);
    }

    void watchReadability(int fd) noexcept {
        FD_SET(fd, &rfds);
        if (fd > maxfd)
            maxfd = fd;

    }
    void watchWriteability(int fd) noexcept {
        FD_SET(fd, &wfds);
        if (fd > maxfd)
            maxfd = fd;
    }

    bool isReadable(int fd) const noexcept {
        return FD_ISSET(fd, &rfds);
    }

    bool isWriteable(int fd) const noexcept {
        return FD_ISSET(fd, &wfds);
    }

    int waitForEvent() noexcept {
        auto tv = mTimeoutTv;
        return select(maxfd + 1, &rfds, &wfds, nullptr, &tv);
    }

private:
    const struct timeval mTimeoutTv;
    fd_set rfds;
    fd_set wfds;
    int maxfd = -1;
};
