#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define MAXLINE 4096
#define LISTENQ 10
 

// Escreve requisicao para o servidor.
void send_request_to_server(int s, char * request_type){
    int response_code = write(s, request_type, MAXLINE);
}

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
 
   if (connect(s, servinfo->ai_addr, servinfo->ai_addrlen) < 0){
       perror("connect call error");
   }

   char request_json_str[MAXLINE];
   // Formata request para envio, no formato {request_type: request_number} 
   // e armazena em request_json_str. A request pode assumir valor entre 1 a 7.
   sprintf(request_json_str, "{\"%s\": %d}", "request_type", 1);
   send_request_to_server(s, request_json_str);

 
   if(close(s) < 0){
       return -1;
   };
 
   return 0;
}
