#ifndef SERVER_H
#define SERVER_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "uwuLib/uwuLib.h"
#include "uwuLib/uwuLibNet.h"

struct net_bundle
{
    // select args
    int maxfd;
    int clientSize;
    int clients[FD_SETSIZE];
    struct sockaddr_in addresses[FD_SETSIZE];
    fd_set set;
    // tracking
    int requests[FD_SETSIZE];
    int dataSent[FD_SETSIZE];
};

struct worker_args {
    FILE *logFile;
    int listenSocket;
    unsigned int bufferLength;
    struct net_bundle bundle;
};

void *ioWorkerRoutine(void *param);

void handleNewConnection(struct net_bundle *bundle, fd_set *set, const int listenSocket);
void handleIncomingData(struct worker_args *args, fd_set *set, int numSelected, char *buffer);

#endif // SERVER_H