#pragma once

#include <sys/un.h>

#include "Select.h"

class AfUnix {
public:
    AfUnix(const char* filename);
    ~AfUnix();

protected:
    struct sockaddr_un ipcFile;
    int mSocket = -1;
};

struct IpcCommand {
    int mark = 0x3fab;
    int type;
    union payload {
        struct IpcCommand_GetEntries {

        };
        struct IpcCommand_GetStats {

        };


    };
} __attribute__((packed));

struct IpcResponse {
    int type;
} __attribute__((packed));


class IpcServer : public AfUnix {
public:
    IpcServer(const char* filename);
    ~IpcServer();
    void registerSockets(Select& select);
    void doIO(Select& select);

private:
    IpcCommand mCmd;
    size_t mCmdOffset = 0;
    size_t mCmdUsed = 0;
    char mWriteBuffer[64*1024];
    size_t mWriteOffset = 0;
    size_t mWriteUsed = 0;
};

