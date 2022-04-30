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
#include <json-c/json.h>

#define MAXLINE 4096
#define LISTENQ 10

struct json_object * read_json_from_string(char * buffer) {
    struct json_object *json;
    json = json_tokener_parse(buffer);
    return json;
}

int read_request_from_json(struct json_object *json){
    struct json_object *json_request;
    int request_type;
    json_object_object_get_ex(json, "request_type", &json_request);
    request_type = json_object_get_int(json_request);
    return request_type;
}

int read_client_request(int new_fd){
    char buffer[MAXLINE];
    int readResult = read(new_fd, buffer, MAXLINE);
    json_object * request_json = read_json_from_string(buffer);
    int request = read_request_from_json(request_json);
    printf("The request was: %i", request);
    return request;
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
           read_client_request(new_fd);
           /* str_echo(new_fd);  process the request */
           exit(0);
       }
       close(new_fd); /* parent closes connected socket */
   }
   return 0;
}
