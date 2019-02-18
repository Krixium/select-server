#include "main.h"

#include <sys/sysinfo.h>

// initializing library
#define UWULIB_IMPL
#define UWULIBNET_IMPL
#include "uwuLib/uwuLib.h"
#include "uwuLib/uwuLibNet.h"

int main(int argc, char *argv[])
{
    unsigned int numProcs = get_nprocs();
    pthread_t *workers;
    struct worker_args args;

    short port;

    // get command line arguements
    if (argc != 3)
    {
        printHelp(argv[0]);
        return 0;
    }
    port = atoi(argv[1]);
    args.bufferLength = atoi(argv[2]);

    // create listening socket
    if (!uwuCreateBoundSocket(&args.listenSocket, port))
    {
        perror("could not bind socket");
        return 1;
    }

    // initialize network variables
    bzero(&args.bundle.addresses, sizeof(struct sockaddr_in) * FD_SETSIZE);
    args.bundle.maxfd = args.listenSocket;
    args.bundle.clientSize = -1;
    FD_ZERO(&args.bundle.set);
    for (int i = 0; i < FD_SETSIZE; i++)
    {
        args.bundle.clients[i] = -1;
        args.bundle.requests[i] = 0;
        args.bundle.dataSent[i] = 0;
    }

    // listen for connections
    listen(args.listenSocket, 5);
    FD_SET(args.listenSocket, &args.bundle.set);

    // allocate workers
    workers = calloc(sizeof(pthread_t), numProcs);
    for (int i = 0; i < numProcs; i++)
    {
        if (uwuCreateThread(&workers[i], ioWorkerRoutine, (void *)&args))
        {
            perror("could not create worker thread");
            return 1;
        }
    }
    for (int i = 0; i < numProcs; i++)
    {
        uwuJoinThread(workers[i]);
    }
    // free workers
    free(workers);

    return 0;
}

void printHelp(const char *name)
{
    fprintf(stdout, "Usage: %s port size\n", name);
    fprintf(stdout, "Arguements:\n");
    fprintf(stdout, "\tport - The port to listen on.\n");
    fprintf(stdout, "\tsize - The size of the packet.\n");
}

