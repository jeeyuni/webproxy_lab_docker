#include "csapp.h"


int main (int argc, char **argv) 
{
    int clientfd;    /* Initializing a buffer */
    char *host, *port, buf[MAXLINE]; 
    rio_t rio;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }
    host = argv[1];
    port = argv[2];

    clientfd = Open_clientfd(host,port);
    Rio_readinitb(&rio, clientfd);

    /* From client's perspective echoing is just read in what I typed */
    /* reading what has been typed on the standard input, and as long as thats not NULL*/
    while (Fgets(buf, MAXLINE, stdin) != NULL) {  
        /* write it to the client file descriptor (sending it over to the network)*/
        Rio_writen(clientfd,buf, strlen(buf)); 
        /*gettinng back from the server, read from the file descriptor, fill the result back in the buffer */
        Rio_readlineb( &rio, buf, MAXLINE); 
        /* prints ( new line is included )*/
        Fputs(buf, stdout);
    }
    Close(clientfd);
    exit(0);
}