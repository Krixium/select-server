#include "main.h"

#include <signal.h>
#include <sys/sysinfo.h>

// initializing library
#define UWULIB_IMPL
#define UWULIBNET_IMPL
#include "uwuLib/uwuLib.h"
#include "uwuLib/uwuLibNet.h"

unsigned int numProcs;
pthread_t *workers;

int main(int argc, char *argv[])
{
    struct worker_args args;

    short port;

    numProcs = get_nprocs();

    // connect signal handler
    signal(SIGINT, catchSig);

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
        perror("Could not bind socket");
        return 1;
    }

    // set worker arguements
    if ((args.logFile = uwuOpenFile("select-server.log", "w+")) == NULL)
    {
        perror("Could not create log file");
        return 1;
    }
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
            perror("Could not create worker thread");
            return 1;
        }
    }
    for (int i = 0; i < numProcs; i++)
    {
        uwuJoinThread(workers[i]);
    }

    // free workers
    free(workers);
    fprintf(stdout, "Removing workers\n");
    
    // close log file
    fflush(args.logFile);
    fclose(args.logFile);
    fprintf(stdout, "Closing log file\n");

    return 0;
}

void printHelp(const char *name)
{
    fprintf(stdout, "Usage: %s port size\n", name);
    fprintf(stdout, "Arguements:\n");
    fprintf(stdout, "\tport - The port to listen on.\n");
    fprintf(stdout, "\tsize - The size of the packet.\n");
}

void catchSig(int sig)
{
    fprintf(stdout, "Stopping server\n");
    for (int i = 0; i < numProcs; i++)
    {
        pthread_cancel(workers[i]);
    }
}