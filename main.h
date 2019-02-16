#ifndef MAIN_H
#define MAIN_H

#include "uwuLib/uwuLib.h"
#include "uwuLib/uwuLibNet.h"

struct select_bundle 
{
    int maxfd;
    int clientSize;
    int clients[FD_SETSIZE];
    struct sockaddr_in addresses[FD_SETSIZE];
    fd_set set;
};

void printHelp(const char* name);

void *ioWorkerRoutine(void *param);

#endif // MAIN_H