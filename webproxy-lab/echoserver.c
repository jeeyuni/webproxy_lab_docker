#include "csapp.h"
void echo(int connfd);


int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr; /* Enough room for any addr.
    It's part of the gneeric API, for an api that is big enough to store name of the host */
    char client_hostname[MAXLINE], client_port[MAXLINE];

    listenfd = Open_listenfd(argv[1]);
    while (1) { /* You gotta control c to quit this program */
        clientlen = sizeof(struct sockaddr_storage); /* Important! */
        /* info about the client : IP Address*/
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        /* pass in client address, andd it comes back with the domain name of the client*/
        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0); 
        /* client_hostname client_port are both buffers (passing their length here MAXLINE) filling those in with the string representations of the hostname & the port */
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
        echo(connfd);
        Close(connfd);
    }
    exit(0);
}