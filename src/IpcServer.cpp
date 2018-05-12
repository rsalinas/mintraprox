#include "IpcServer.h"

#include <sys/socket.h>
#include <unistd.h>
#include <iostream>

#include "netcommon.h"

using namespace std;

AfUnix::AfUnix(const char* filename) {
    memset(&ipcFile, 0, sizeof(ipcFile));
    ipcFile.sun_family = AF_UNIX;
    strcpy(ipcFile.sun_path, filename);

    mSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (bind(mSocket, (struct sockaddr *) &ipcFile, sizeof(ipcFile)) < 0) {
        throw NetworkError{"Cannot bind socket"};
    }
    // buf contains the data, buflen contains the number of bytes
    //    int bytes = write(mSocket, buf, buflen);
    //...

}

IpcServer::IpcServer(const char* filename) : AfUnix(filename) {


}


AfUnix::~AfUnix() {
    close(mSocket);
    unlink(ipcFile.sun_path);
}
void IpcServer::registerSockets(Select& select) {
    select.watchReadability(mSocket);
}
void IpcServer::doIO(Select& select) {
    if (select.isReadable(mSocket)) {
        auto nbytes = recv(mSocket, &mCmd, sizeof mCmd - mCmdOffset, 0);
        if (nbytes < 0) {
            perror("recv afunix");
            return;
        }
        if (nbytes == 0) {
            clog << "Client disconnected" << endl;
        }
        mCmdOffset += nbytes;
    }
}
