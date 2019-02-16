#include "main.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/sysinfo.h>

// initializing library
#define UWULIB_IMPL
#define UWULIBNET_IMPL
#include "uwuLib/uwuLib.h"
#include "uwuLib/uwuLibNet.h"

unsigned int BUFFER_LEN;
int listenSocket;

int main(int argc, char *argv[])
{
    pthread_t *workers;
    unsigned int numProcs = get_nprocs();

    short port;
    struct select_bundle selectBundle;

    // get command line arguements
    if (argc != 3)
    {
        printHelp(argv[0]);
        return 0;
    }
    port = atoi(argv[1]);
    BUFFER_LEN = atoi(argv[2]);

    // create listening socket
    if (!uwuCreateBoundSocket(&listenSocket, port))
    {
        perror("could not bind socket");
        return 1;
    }

    // initialize select variables
    bzero(&selectBundle.addresses, sizeof(struct sockaddr_in) * FD_SETSIZE);
    selectBundle.maxfd = listenSocket;
    selectBundle.clientSize = -1;
    FD_ZERO(&selectBundle.set);
    for (int i = 0; i < FD_SETSIZE; i++)
    {
        selectBundle.clients[i] = -1;
    }

    // listen for connections
    listen(listenSocket, 5);
    FD_SET(listenSocket, &selectBundle.set);

    // allocate workers
    workers = calloc(sizeof(pthread_t), numProcs);
    for (int i = 0; i < numProcs; i++)
    {
        if (uwuCreateThread(&workers[i], ioWorkerRoutine, (void *)&selectBundle))
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

void *ioWorkerRoutine(void *param)
{
    struct select_bundle *bundle = (struct select_bundle *)param;

    int i;
    int num;
    fd_set readSet;

    int sock;
    int newSocket;
    struct sockaddr_in newClient;

    char *buffer;
    buffer = calloc(sizeof(char), BUFFER_LEN);

    fprintf(stdout, "starting io worker thread[%ld]\n", pthread_self());
    while (1)
    {
        readSet = bundle->set;
        num = select(bundle->maxfd + 1, &readSet, NULL, NULL, NULL);

        // new connection
        if (FD_ISSET(listenSocket, &readSet))
        {
            if (!uwuAcceptSocket(listenSocket, &newSocket, &newClient))
            {
                perror("could not accept new client");
                exit(1);
            }

            fprintf(stdout, "new client accepted, remote address: %s\n", inet_ntoa(newClient.sin_addr));

            for (i = 0; i < FD_SETSIZE; i++)
            {
                if (bundle->clients[i] < 0)
                {
                    bundle->clients[i] = newSocket;
                    bundle->addresses[i] = newClient;
                    break;
                }
            }

            if (i == FD_SETSIZE)
            {
                perror("too many clients\n");
                exit(1);
            }

            FD_SET(newSocket, &bundle->set);

            if (newSocket > bundle->maxfd)
            {
                bundle->maxfd = newSocket;
            }

            if (i > bundle->clientSize)
            {
                bundle->clientSize = i;
            }

            if (--num <- 0)
            {
                continue;
            }
        }

        for (i = 0; i <= bundle->clientSize; i++)
        {
            if ((sock = bundle->clients[i]) < 0)
            {
                continue;
            }

            if (FD_ISSET(sock, &readSet))
            {
                if (uwuReadAllFromSocket(sock, buffer, BUFFER_LEN) > 0)
                {
                    fprintf(stdout, "read [%s] from %s\n", buffer, inet_ntoa(bundle->addresses[i].sin_addr));
                    write(sock, buffer, BUFFER_LEN);
                }
                else
                {
                    fprintf(stdout, "connection closed by remote host %s\n", inet_ntoa(bundle->addresses[i].sin_addr));
                    close(sock);
                    FD_CLR(sock, &bundle->set);
                    bundle->clients[i] = -1;
                }
            }

            if (--num <= 0)
            {
                break;
            }
        }
    }

    free(buffer);

    return NULL;
}
