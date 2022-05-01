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
// Recebe o socket do cliente criado, e uma string em formato JSON.
// E envia para o servidor.
void send_request_to_server(int s, char * request_type){
    int response_code = write(s, request_type, MAXLINE);
    if(response_code == -1)
        exit(1);
}

// Func para testar criacao de filmes.
// Formata request para envio, no formato {request_type: request_number} 
// e armazena em request_json_str. A request pode assumir valor entre 1 a 7.
// Envia a request feita para o servidor.
// Em seguida, formata uma string em JSON contendo informacoes do filme e ID numerico para cadastro.
// No formato: {"movie_name": "O resgate do Soldado Brian","genre": ["Drama", "Guerra"],"director": "Stephen Spielberg","year": "1999","id": 0}
// E envia esta string JSON para o servidor.
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

//Func para testar adicao de genero.
// Formata request para envio, no formato {request_type: request_number} 
// e armazena em request_json_str. A request pode assumir valor entre 1 a 7.
// Envia a request feita para o servidor.
// Em seguida, formata uma string JSON no formato {id: 0, genre: Drama}
// Para especificar a adicao do genero Drama no filme de ID 0.
// E envia esta string JSON para o servidor.

void add_genre_request(int s){
    char request_json_str[MAXLINE];
    for(int i = 0; i < 5; i++){
        sprintf(request_json_str, "{\"%s\": %d}", "request_type", 2);
        send_request_to_server(s, request_json_str);
        printf("Adicionando genero nos filmes \n");
        char *genre_to_add = "Drama";
        sprintf(request_json_str,
        "{\"id\": %i, \"genre\": \"%s\"}", i, genre_to_add);
        send_request_to_server(s, request_json_str);
    }
}

//Func para testar delete de um filme.
// Formata request para envio, no formato {request_type: request_number} 
// e armazena em request_json_str. A request pode assumir valor entre 1 a 7.
// Envia a request feita para o servidor.
// Em seguida, formata uma string JSON no formato {id: 0}
// Para especificar a remocao do filme de ID 0.
// E envia esta string JSON para o servidor.

void delete_movies_request(int s){
    char request_json_str[MAXLINE];
    for(int i = 0; i < 5; i++){
        printf("Removendo Filme Cadastrado \n");
        sprintf(request_json_str, "{\"%s\": %d}", "request_type", 4);
        send_request_to_server(s, request_json_str);
        sprintf(request_json_str, "{\"id\": %i}", i);
        send_request_to_server(s, request_json_str);
    }
}

// Funcao principal do programa que cuida de se conectar com o servidor
// Na porta 9000 atraves do getaddrinfo para descobrir as informacoes do servidor.
// E em seguida chamada do OS para criar o socket e realizar a conexao.
// Se a conexao foi bem sucedida, requisita as operacoes de manipular filmes.
// E posteriormente fecha a conexao.
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
    add_genre_request(s);
    delete_movies_request(s);
 
   if(close(s) < 0){
       return -1;
   };
 
   return 0;
}
