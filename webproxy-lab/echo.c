void echo(int connfd)
{
    /* The server uses RIO to rad and echo text lines until EOF(end of file) condition is encountered.
        - EOF condition is caused by client calling close(clientfd) */
    size_t n;
    char buf[MAXLINE];
    rio_t rio;

    /* reads a file over the file descriptor */
    Rio_readinitb(&rio, connfd);
    /* if its an non zero number it echos back by writing it
    if its a 0 then it exits the little loop. 
    */
    whle((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
        printf("server received %d bytes\n", (int)n);
        Rio_writen(connfd, buf, n);
    }
}