#include <stdio.h>
#include <csapp.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

void proxy_doit(int fd);

int main(int argc, char **argv)
{
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1)
  {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    proxy_doit(connfd);
    Close(connfd);
  }
}

/*
* proxy_doit - handle one HTTP proxy transaction
*/
void proxy_doit(int client_fd)
{
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char hostname[MAXLINE], path[MAXLINE];
  int port;
  rio_t rio_client;
  int server_fd;

  /* Read request line from client */
  Rio_readinitb(&rio_client, client_fd);
  if (Rio_readlineb(&rio_client, buf, MAXLINE) == 0) 
  {
    return; 
  }
  printf("Request from client:\n%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version );

  /* This proxy only supports GET method */
  if (strcasecmp(method, "GET"))
  {
    clienterror(client_fd, method, "501", "Not Implemented", "This proxy does not implement this method");
    return;
  }

  /* Parse URI to get hostname, port, and path */
  if (!parse_proxy_uri(uri, hostname, &port, path))
  {
    clienterror(client_fd, uri, "400", "Bad Request", "Invalid URI for proxying");
    return;
  }

  /* Connect to the destination server */
  char port_str[10];
  sprintf(port_str, "%d", port);
  server_fd = Open_clientfd(hostname, port_str);
  if (server_fd < 0)
  {
    fprintf(stderr, "Connection to %s:%d falied\n", hostname, port);
    return;
  }
  printf("Connected to destination server: %s:%d\n", hostname, port);

  /*Forward the request to the destination server */
  forward_request(server_fd, &rio_client, path, hostname);

  /*Receive the response from the server and forward it to the client */
  forward_response(client_fd, server_fd);

  /* Close the connection to the destination server */
  Close(server_fd);
  printf("Closed connection with destination server. \n\n");
  
}