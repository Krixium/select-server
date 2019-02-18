#include "server.h"

void *ioWorkerRoutine(void *param)
{
    struct worker_args *args = (struct worker_args *)param;
    struct net_bundle *bundle = &args->bundle;

    int numSelected = 0;
    fd_set readSet;

    char *buffer;
    buffer = calloc(sizeof(char), args->bufferLength);

    fprintf(stdout, "starting io worker thread[%ld]\n", pthread_self());
    while (1)
    {
        readSet = bundle->set;
        numSelected = select(bundle->maxfd + 1, &readSet, NULL, NULL, NULL);

        // new connection
        if (FD_ISSET(args->listenSocket, &readSet))
        {
            handleNewConnection(bundle, &readSet, args->listenSocket);

            if (--numSelected <= 0)
            {
                continue;
            }
        }

        // read all incoming data and handle it
        handleIncomingData(bundle, &readSet, numSelected, buffer, args->bufferLength);
    }

    free(buffer);

    return NULL;
}

void handleNewConnection(struct net_bundle *bundle, fd_set *set, const int listenSocket)
{
    int i = 0;
    int newSocket = -1;
    struct sockaddr_in newClient;

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
}

void handleIncomingData(struct net_bundle *bundle, fd_set *set, int numSelected, char *buffer, const unsigned int bufferLength)
{
    int sock = -1;
    unsigned int dataRead = -1;

    for (int i = 0; i <= bundle->clientSize; i++)
    {
        if ((sock = bundle->clients[i]) < 0)
        {
            continue;
        }

        if (FD_ISSET(sock, set))
        {
            dataRead = uwuReadAllFromSocket(sock, buffer, bufferLength);
            if (dataRead > 0)
            {
                write(sock, buffer, bufferLength);
                fprintf(stdout, "read [%s] from %s\n", buffer, inet_ntoa(bundle->addresses[i].sin_addr));
                bundle->dataSent[i] += dataRead;
                bundle->requests[i]++;
            }
            else
            {
                fprintf(stdout, "connection closed by remote host %s\n", inet_ntoa(bundle->addresses[i].sin_addr));
                close(sock);
                FD_CLR(sock, &bundle->set);
                bundle->clients[i] = -1;
            }
        }

        if (--numSelected <= 0)
        {
            break;
        }
    }
}