#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
 
#define MAXLINE 100
#define LISTENQ 10
 
void str_echo(int new_fd)
{
   ssize_t n;
   char buf[MAXLINE];
   again:
   while ( (n = read(new_fd, buf, MAXLINE)) > 0)
   write(new_fd, buf, n);
   if (n < 0 && errno == EINTR)
       goto again;
   else if (n < 0)
       perror("str_echo: read error");
}
 
int main(int argc, char **argv)
{
   int sock_fd, new_fd;
   pid_t childpid;
   socklen_t clilen;
   struct sockaddr_in cliaddr, servaddr;
   sock_fd = socket(PF_INET, SOCK_STREAM, 0);
   bzero(&servaddr, sizeof(servaddr));
   servaddr.sin_family = AF_INET;
   servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
   servaddr.sin_port = htons(9000);
   bind(sock_fd, (struct sockaddr *) &servaddr, sizeof(servaddr));
   listen(sock_fd, LISTENQ);
   while(true){
       clilen = sizeof(cliaddr);
       new_fd= accept(sock_fd, (struct sockaddr *) &cliaddr, &clilen);
       if ( (childpid = fork()) == 0) { /* child process */
           close(sock_fd); /* close listening socket */
           printf("Connected %i ", new_fd);
           /* str_echo(new_fd);  process the request */
           exit(0);
       }
       close(new_fd); /* parent closes connected socket */
   }
   return 0;
}
