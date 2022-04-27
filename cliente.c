#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
 
int main(int argc, char *argv[])
{
   int status;
   struct addrinfo hints;
   struct addrinfo *servinfo; // will point to the results
 
   memset(&hints, 0, sizeof(hints)); // make sure the struct is empty
   hints.ai_family = AF_INET; //don't care IPv4 or IPv6
   hints.ai_socktype = SOCK_STREAM; //TCP stream sockets
   hints.ai_flags = AI_PASSIVE; // fill in my IP for me
 
   if ((status = getaddrinfo("localhost", "9000", &hints, &servinfo)) != 0) {
       fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
       exit(1);
   }
 
   int s = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
   printf("%d",s);
 
   if (connect(s, servinfo->ai_addr, servinfo->ai_addrlen) < 0){
       perror("connect call error");
   }
 
   if(close(s) < 0){
       return -1;
   };
 
   return 0;
}
