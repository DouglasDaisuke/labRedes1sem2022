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
    if(response_code == -1)
        exit(1);
}

// Formata request para envio, no formato {request_type: request_number} 
// e armazena em request_json_str. A request pode assumir valor entre 1 a 7.
// Em seguida, formata JSON contendo informacoes do filme e ID numerico para cadastro.
void create_movies_request(int s){
    char request_json_str[MAXLINE];
    char *movie_names[5] = {"O resgate do Soldado Brian", "Djanko Livre", "Os Simpsons", "Tropa de Elite", "Cidade de Deus"};
    char *genres[5] = {"[\"Drama\", \"Guerra\"]", "[\"Acao\", \"Faoreste\"]", "[\"Comedia\"]", "[\"Guerra\"]", "[\"Documentario\"]"};
    char *directors[5] = {"Stephen Spielberg", "Quentin Tarantino", "Matt Groening", "Jose Padilha", "Fernando Meirelles"};
    char *year[5] = {"1999", "2013", "1989", "2009", "2002"};
    for(int i = 0; i < 5; i++){
        sprintf(request_json_str, "{\"%s\": %d}", "request_type", 1);
        send_request_to_server(s, request_json_str);
        printf("Enviando Filme para Cadastro \n");
        sprintf(request_json_str,
        "{\n\
        \"movie_name\": \"%s\",\n\
        \"genre\": %s,\n\
        \"director\": \"%s\",\n\
        \"year\": \"%s\",\n\
        \"id\": %i\n}", movie_names[i], genres[i], directors[i], year[i], i);
        send_request_to_server(s, request_json_str);
    }
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
    if (connect(s, servinfo->ai_addr, servinfo->ai_addrlen) < 0)
        perror("connect call error");

    create_movies_request(s);
 
   if(close(s) < 0){
       return -1;
   };
 
   return 0;
}
