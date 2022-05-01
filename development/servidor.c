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
#include <ctype.h>

#define MAXLINE 4096
#define LISTENQ 100


// Func utilitaria para converter buffer recebido em objeto JSON.
struct json_object * read_json_from_string(char * buffer) {
    struct json_object *json;
    json = json_tokener_parse(buffer);
    return json;
}

// Func utilitaria para ler JSON de requisicao do usuario e determinar tipo da op.
int read_request_from_json(struct json_object *json){
    struct json_object *json_request;
    int request_type;
    json_object_object_get_ex(json, "request_type", &json_request);
    request_type = json_object_get_int(json_request);
    printf("Request do tipo %i feita por usuario \n", request_type);
    return request_type;
}

// Func para cadastrar filme de acordo com a entrada em JSON,
// e armazena o arquivo com nome {id.json}
void create_movie_from_client_request(int new_fd) {
    char buffer[MAXLINE];
    int readResult = read(new_fd, buffer, MAXLINE);
    if(readResult > 1){
        struct json_object *movie = read_json_from_string(buffer);
        struct json_object *movie_name_json;
        struct json_object *id_json;
        json_object_object_get_ex(movie, "id", &id_json);
        json_object_object_get_ex(movie, "movie_name", &movie_name_json);
        int id = json_object_get_int(id_json);
        const char *movie_name = json_object_get_string(movie_name_json);
        char file_name[MAXLINE];
        sprintf(file_name, "../movies/%i%s", id, ".json");
        FILE *fp = fopen(file_name, "w+");
        if (fp){
            fputs(buffer, fp);
            printf("Filme de ID %i e nome %s cadastrado com sucesso! \n", id, movie_name);
        }
        fclose(fp);
    }
}

void read_client_request(int new_fd){
    char buffer[MAXLINE];
    int request = 0;
    while(request != -1){
        int readResult = read(new_fd, buffer, MAXLINE);
        if(readResult > 1){
            json_object * request_json = read_json_from_string(buffer);
            int request = read_request_from_json(request_json);
            if(request == 1){
                create_movie_from_client_request(new_fd);
            } else {
                request = -1;
            }
        } else {
            request = -1;
        }
    }
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
           read_client_request(new_fd);
           exit(0);
       }
       close(new_fd); /* parent closes connected socket */
   }
   return 0;
}
