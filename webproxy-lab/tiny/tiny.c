/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

/* 
* serve_static serves static content to a client.
*Helper function of the code 
*/
void serve_static(int fd, char *filename, int filesize)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  /* Send response headers to client
  receives a request with a particular file name from the uri
  it looks up OS call on how big the file is (how many bytes)
  given a file descriptor for the connection.
  This function supports not just text but also GIFS, JPEG and etc.
  */
  get_filetype(filename, filetype); // which type of file is it? (GIF, JPEG ... )
  /* formatting a header information */
  /*writes the header information out to the client */
  sprintf(buf, "HTTP/1.0 200 OK\r\n"); // every line has to finish with character return and a new line
  sprintf(buf, "%sServer: Tiny Web Server\r\n"), buf;
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);   // IMPORTANT
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype); // IMPORTANT
  /* Sending back to the client */
  Rio_writen(fd, buf, strlen(buf));

  /* Send response body to client. These can be done with regular read and writes*/
  /* Get the file, open it up */
  srcfd = Open(filename, O_RDONLY, 0);
  /* mmap you can avoid opening a file , passing a pointer directly to where the file is located (avoids one step of buffering)*/
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  Close(srcfd);
  /*sending it off to the client */
  Rio_writen(fd, srcp, filesize);
  Munmap(srcp, filesize);

  /* length here(idk where uhh   ) is the lengh of the file ( doesn't include the bytes in the header )*/
}

/*
* serve_dynamic serves dynamic content to a client.
*/
void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  char buf[MAXLINE], *emptylist[] = {NULL};

  /* Return first par tof HTTP respnse */
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  if (Fork() == 0)
  { /* Child */
    /* Real server would set all CGI vars here */
    setenv("QUERY_STRING", cgiargs, 1);   /* environment variable */
    DUp2(fd, STDOUT_FILENO);              /* Redirect stdout to client */
    Execve(filename, emptylist, environ); /* Run CGI program */
  }
  Wait(NULL); /* Parent waits for and reaps child */
}


/*
*  doit function handles one HTTP transaction (request and response).
*
*   fd: connected socket descriptor passed from main
*
*   is_static : integer flag that will be set to 1 if the requested content is static, and 0 if it's dynamic
* 
*   sbuf : A struct stat variable. The structure is used to store information about a file( ex size, permissions ..)
*          obtained using stat() system call. 
*
*   buf: A general purpose buffer to read lines from the socket. 
*
*   method, uri, version : Buffers to store the components of the HTTP request line (ex "GET", "/index.html", "HTTP/1.0")
*
*   filename : A buffer to store the actual path to the file on the server's file system 
*              that corresponds to the requested uri
*
*   cgiargs : A buffer to store any arguments passed to a CGI program
*
*   rio: A rio_t structure. This is a robust I/O buffer (from csapp.h) that allows for efficient reading of 
*         text lines and binary data from the socket, handling short counts and partial reads
*
*/
void doit(int fd)
{
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  /*
  *   1. Read Request Line and Headers 
  *   
  *   Rio_readinitb(&rio, fd) : Initializes the rio( robust I/O buffer). It associatesthe rio struct
  *                             with the given file descriptor fd, preparing it for buffered reading.
  * 
  *   Rio_readineb(&rio, buf, MAXLINE): Reads the first line of the HTTP request from the client's socket into buf. 
  *                                     The request line typically looks like GET /index.html HTTP/1.0
  * 
  *   sscanf() : parses the request line. 
  *              - Extracts the HTTP method(ex "GET", "POST"), the uri("/index.html"), and the HTTP version("HTTP/1.0")
  * 
  */
  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE);
  printf("Request headers: \n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version);
  /*
  * 
  *  Tiny Server is designed to only handle "GET" requests, so this if statement checks for the "GET".
  * 
  *  Then, read_requesthdrs gets called to read and discard the reamining HTTP request 
  *  headers sent by the client. These headers contain additional information about the request. 
  * 
  */
  if (strcasecmp(method, "GET"))
  {
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }
  read_requesthdrs(&rio);

  /*
  *   2. Parse URI and Check File Existence
  *   
  *   is_static = parse_uri(uri, filename, cgiargs) : This crucial function determines whether the requested uri corresponds to a static file
  *                                                   or a dynamic CGI program.
  *                                                   - Ths function populates filename -> local file path, 
  *                                                                             cgiargs -> cgi request, 
  *                                                   - returns 1 if static, 0 if dynamic.
  * 
  *  if(stat(filename, &sbuf) < 0) : After determining the filename, the stat() system call is used to retrieve 
  *  informatio about the file(or directory) and stores it in sbuf.                                                   
  * 
  */
  is_static = parse_uri(uri, filename, cgiargs);
  if (stat(filename, &sbuf) < 0)
  {
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
    return;
  }
/*
*   3. Serve Content Based on Type - The server decides how to serve the content based on static/ dynamic
*
*   For static file, it checks if the path points to a regular file. if it's a directory or a special file, it's an error.
*                    OR it checks if the user has read permission on the file. The server process needs to be able to read the file.
*   
*   For dynamic file, it checks for a regular file (again),
*                    OR checks if the user has execute permission on the file. 
*
*   When all checks pass, the function is called to execute the CGI program. It sets up environment variables,
*   forks a child process, redirects the child's standard output to the client socket then executes CGI program.
*
*/
  if (is_static)
  { /* Serve static content*/
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) 
    {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
      return;
    }
  }
  else
  { /* Serve dynamic content */
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
    {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
      return;
    }
    serve_dynamic(fd, filename, cgiargs);
  }
}

/*
* clienterror sends an error message to the client
*/
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  /* Build the HTPP response body */
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor = "
                "ffffff"
                ">\r\n",
          body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  /* Print the HTTP response*/
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/htlm\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}

/*
* read_requesthdrs reads and ignores request headers 
*/
void read_request(rio_t *rp)
{
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  while (strcmp(buf, "\r\n"))
  {
    Rio_readlineb(rp, buf, MAXLINE);
    prinf("%s", buf);
  }
  return;
}

/*
* Tiny parse_uri parses an HTTP URI.
*/
int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;

  if (!strstr(uri, "cgi-bin"))
  { /* Static content*/
    strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    if (uri[strlen(uri) - 1] == '/')
      strcat(filename, "home.html");
    return 1;
  }
  else
  { /* Dynamic content */
    ptr = index(uri, '?');
    if (ptr)
    {
      strcpy(cgiargs, ptr + 1);
      *ptr = '\0';
    }
    else
    {
        strcpy(cgiargs, ""); // what is the indent for???????????????????????????
      strcpy(filename, ".");
      strcat(filename, uri);
      return 0;
    }
  }
}

  int main(int argc, char **argv)
  {
    /* 
    *   listenfd : holds listening socket descriptor. This is the socket that 
    *              the server uses to wait for incoming client connection requests.
    * 
    *   connfd : holds connected socket descriptor. Once a client connects, the server creates
    *            the srver creates a new socket (connfd) specifically for communication with the client. 
    * 
    *   hostname, port : Character arrays ( buffers ) to store the client's hostname and port number, respectively.
    *                    MAXLINE is a macro (in caspp.h) representing the max line length. 
    * 
    *   clientlen : A socketlen_t variable to store the size of the client's socket address structure.
    * 
    *   clientaddr : A struct sockaddr_storage variable. This is a generic socket address structure
    *                that can hold different types of socket addresses (ex IPv4, IPv6) making the server
    *                more flexible. 
    */
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    /*
    *   1. Command-Line Argument Check
    *   
    *   argc( argument count) : should be 2. one for the program name(argv[0]) and one for the port number(argv[1]).
    *   
    *   If the correct number of arguments isn't provided, error message is printed to stderr(standard error),
    *   shows the correct usage, and the program exits with an error status (exit 1).
    *   
    *   The server requries a port number to know which port to listen on. 
    * 
    */
    if (argc != 2)
    {
      fprintf(stderr, "usage: %s <port>\n", argv[0]);
      exit(1);
    }
    /*
    *   2.Creating the Listening Socket
    *
    *   Open_listenfd(argv[1]) : is a wrapper function (in csapp.h) that simplifies the process of creating a listening socket.
    *                            - It takes the port number (argv[1]) as an argument
    *                            - Then, it creates a socket using socket(). Sets socket options, binds the socket,
    *                              puts socket into listening state using listen(), if successful, returns the listenng socket descriptor
    *                              (listenfd). if there's an error it handles it. 
    * 
    */
    listenfd = Open_listenfd(argv[1]);
    /*
    *    3. The Main Server Loop
    *     
    *     This creates an infinite loop. A Web server is designed to run continuously. 
    *     It waits for and serves requests from multiple clients over time. It doesn't typically exit
    *     unless explicitly stopped by an administrator or due to a critical error.
    * 
    */
    while (1)
    {
      /*
      *   4. Accepting Client Connections
      *
      *   clientlen : initialized wih the size of the clientaddr structure. This is required by accept() 
      *               to know how much space it can fill. 
      * 
      *   Accept(listenfd, SA *)&clientaddr, &clientlen : another wrapper function. It blocks the server's execution
      *                                                   until a client connects to the listenfd socket. 
      * 
      *   When a client connection arrives, Accept performs the accept() system call. 
      *     - It creates a new connected socket descriptor(connfd) dedicated to this specific client communication.
      *     - It fills the clientaddr structure with the client's network address information (IP address and port)
      *     - It updates clientlen with the actual size of the filled address structure. 
      * 
      *   The listenfd reamins open and continues to listen for new connections, while connfd is used 
      *   for the current client's interaction.
      */    
      clientlen = sizeof(clientaddr);
      
      connfd = Accept(listenfd, (SA *)&clientaddr,
                      &clientlen); // line:netp:tiny:accept
      
      /*
      *   5. Logging Client Information
      *   
      *   Getnameinfo(...) : This function (or its wrapper Getnameinfo) is used to convert the binary network address
      *                      in clientaddr into human-readable hostname and port strings.
      * 
      *   The server then prints a message to the console (printf) indicating that a connection has been accepted
      *   and shows the client's hostname and port. This is useful for debugging and monitoring server activity.
      */
      Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                  0);
      printf("Accepted connection from (%s, %s)\n", hostname, port);

      /*
      *   6. Handling the Client Request
      *   
      *   doit(connfd): The core function that handles HTTP request from the connected client.
      *                   - Inside doit, the server reads the HTTP request, parses the URI, determines if the request is for
      *                     static or dynamic content. checks file permissions, and then serves the appropriate content (or error).
      *                   - This function encapsulates all the logic for processing a single HTTP transactions.
      *
      */
      doit(connfd);  // line:netp:tiny:doit

      /*
      *   7. Closing the connection
      *    
      *   Close(conffd): after the doit funciton has finished processing the client's request and sending response,
      *                  this wrapper function closes the connfd. This realeses the resources associated with 
      *                  the specific client connection.
      */
      Close(connfd); // line:netp:tiny:close
    }
  }
