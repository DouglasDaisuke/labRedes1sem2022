#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <poll.h>

#define MAXLINE 4096
#define LISTENQ 10
#define TIMEOUT_MILISEC 4000

// Func para ler resposta do servidor.
void read_response_from_server(int s, struct addrinfo * servinfo){
    char buffer[MAXLINE] = {0};
    int readResult;
    socklen_t fromlen;
    fromlen = sizeof  servinfo->ai_addr;
    readResult = recvfrom(s, buffer, MAXLINE, 0, servinfo->ai_addr, &fromlen);
    if(readResult <= 0)
        exit(1);
    printf("%s \n", buffer);
}

// Escreve requisicao para o servidor.
// Recebe o socket do cliente criado, e uma string em formato JSON.
// E envia para o servidor.
void send_request_to_server(int s, char * request_type,  struct addrinfo * servinfo){
    int response_code = sendto(s, request_type, MAXLINE, 0, servinfo->ai_addr, servinfo->ai_addrlen);
    if(response_code == -1){
        exit(1);
    }
}

// Func que implementa o poll()
// pfds é a array que armazena os descritores do socket que queremos monitorar e o tipo de evento que espera-se
// Se durante o tempo TIMEOUT_MILISEC não houver nenhum evento no socket monitorado, ocorre o timout e o programa é encerrado
// Caso um evento ocorra antes do timeout, ele é descrito no campo "pfds[0].revents"
// E retorna se o evento esperado ocorreu ou não
int start_polling(int sock_fd, short event){
    int num_events;
    struct pollfd pfds[1]; 
    pfds[0].fd = sock_fd;    // define descriptor socket
    pfds[0].events = event; //  event we're interested 
    if ( (num_events = poll(pfds, 1, TIMEOUT_MILISEC)) == -1){
        perror("poll"); // error occurred in poll()
    }
    if (num_events == 0) {
        printf("Poll timed out!\n");
        exit(1);
    }
    return pfds[0].revents & event; // return if polling happened or not
}

// Func para testar criacao de filmes.
// Formata request para envio, no formato {request_type: request_number} 
// e armazena em request_json_str. A request pode assumir valor entre 1 a 7.
// Envia a request feita para o servidor.
// Em seguida, formata uma string em JSON contendo informacoes do filme e ID numerico para cadastro.
// No formato: {"movie_name": "O resgate do Soldado Brian","genre": ["Drama", "Guerra"],"director": "Stephen Spielberg","year": "1999","id": 0}
// E envia esta string JSON para o servidor.
void create_movies_request(int s, struct addrinfo *servinfo){
    char request_json_str[MAXLINE];
    char *movie_names[5] = {"O resgate do Soldado Brian", "Djanko Livre", "Os Simpsons", "Tropa de Elite", "Cidade de Deus"};
    char *genres[5] = {"[\"Drama\", \"Guerra\"]", "[\"Acao\", \"Faoreste\"]", "[\"Comedia\"]", "[\"Guerra\"]", "[\"Documentario\"]"};
    char *directors[5] = {"Stephen Spielberg", "Quentin Tarantino", "Matt Groening", "Jose Padilha", "Fernando Meirelles"};
    char *year[5] = {"1999", "2013", "1989", "2009", "2002"};
    for(int i = 0; i < 5; i++){
        sprintf(request_json_str, "{\"%s\": %d}", "request_type", 1);
        send_request_to_server(s, request_json_str, servinfo);
        printf("Enviando Filme para Cadastro \n");
        sprintf(request_json_str,
        "{\n\
        \"movie_name\": \"%s\",\n\
        \"genre\": %s,\n\
        \"director\": \"%s\",\n\
        \"year\": \"%s\",\n\
        \"id\": %i\n}", movie_names[i], genres[i], directors[i], year[i], i);
        send_request_to_server(s, request_json_str, servinfo);
        int pollin_happened = start_polling(s, POLLIN);
        if (pollin_happened) {
            read_response_from_server(s, servinfo);
        } else {
            printf("Unexpected event polling occurred : %d\n", pollin_happened);
        }
    }
}

// Func para testar adicao de genero.
// Formata request para envio, no formato {request_type: request_number} 
// e armazena em request_json_str. A request pode assumir valor entre 1 a 7.
// Envia a request feita para o servidor.
// Em seguida, formata uma string JSON no formato {id: 0, genre: Drama}
// Para especificar a adicao do genero Drama no filme de ID 0.
// E envia esta string JSON para o servidor.
void add_genre_request(int s, struct addrinfo * servinfo){
    char request_json_str[MAXLINE];
    for(int i = 0; i < 5; i++){
        sprintf(request_json_str, "{\"%s\": %d}", "request_type", 2);
        send_request_to_server(s, request_json_str, servinfo);
        printf("Adicionando genero nos filmes \n");
        char *genre_to_add = "Drama";
        sprintf(request_json_str,
        "{\"id\": %i, \"genre\": \"%s\"}", i, genre_to_add);
        send_request_to_server(s, request_json_str, servinfo);
        int pollin_happened = start_polling(s, POLLIN);
        if (pollin_happened) {
            read_response_from_server(s, servinfo);
        } else {
            printf("Unexpected event polling occurred : %d\n", pollin_happened);
        }
    }
}

// Func para testar a listagem dos titulos junto aos id de todos os filmes
// Formata request para envio, no formato {request_type: request_number} 
// e armazena em request_json_str. A request pode assumir valor entre 1 a 7.
// Envia a request feita para o servidor.
void list_all_title_and_id_movies_request(int s, struct addrinfo * servinfo){
    char request_json_str[MAXLINE];
    printf("Listando os nomes e ID de todos os filmes\n");
    sprintf(request_json_str, "{\"%s\": %d}", "request_type", 3);
    send_request_to_server(s, request_json_str, servinfo);
    int pollin_happened = start_polling(s, POLLIN);
    if (pollin_happened) {
        read_response_from_server(s, servinfo);
    } else {
        printf("Unexpected event polling occurred : %d\n", pollin_happened);
    }
}

// Func para testar a listagem de informacoes dos filmes que tiverem o genero especificado pelo cliente.
// Formata request para envio, no formato {request_type: request_number} 
// e armazena em request_json_str. A request pode assumir valor entre 1 a 7.
// Envia a request feita para o servidor.
// Em seguida, formata uma string JSON no formato {genre: "Guerra"}
// Para especificar a listagem das informacoes do filme de genero = "Guerra"
// E envia esta string JSON para o servidor.
void list_movies_informations_by_genre_request(int s, struct addrinfo * servinfo){
    char request_json_str[MAXLINE];
    char *genre = "Guerra";
    sprintf(request_json_str, "{\"%s\": %d}", "request_type", 4);
    send_request_to_server(s, request_json_str, servinfo);
    printf("Listando informações do Filme do genero especificado\n");
    sprintf(request_json_str, "{\"genre\": \"%s\"}", genre);
    send_request_to_server(s, request_json_str, servinfo);
    int pollin_happened = start_polling(s, POLLIN);
    if (pollin_happened) {
        read_response_from_server(s, servinfo);
    } else {
        printf("Unexpected event polling occurred : %d\n", pollin_happened);
    }
}

// Func para testar a listagem as informações de todos os filmes
// Formata request para envio, no formato {request_type: request_number} 
// e armazena em request_json_str. A request pode assumir valor entre 1 a 7.
// Envia a request feita para o servidor.
void list_all_movies_informations_request(int s, struct addrinfo * servinfo){
    char request_json_str[MAXLINE];
    printf("Listando informações de todos os Filme\n");
    sprintf(request_json_str, "{\"%s\": %d}", "request_type", 5);
    send_request_to_server(s, request_json_str, servinfo);
    int pollin_happened = start_polling(s, POLLIN);
    if (pollin_happened) {
        read_response_from_server(s, servinfo);
    } else {
        printf("Unexpected event polling occurred : %d\n", pollin_happened);
    }
} 

// Func para testar a listagem de informacoes dos filmes que tiverem o id especificado pelo cliente.
// Formata request para envio, no formato {request_type: request_number} 
// e armazena em request_json_str. A request pode assumir valor entre 1 a 7.
// Envia a request feita para o servidor.
// Em seguida, formata uma string JSON no formato {id: 0}
// Para especificar a listagem das informacoes do filme de ID = 0
// E envia esta string JSON para o servidor.
void list_movies_informations_by_id_request(int s, struct addrinfo * servinfo){
    char request_json_str[MAXLINE];
    int movie_id;
    printf("Listando informações do Filme pelo id especificado\n");
    sprintf(request_json_str, "{\"%s\": %d}", "request_type", 6);
    send_request_to_server(s, request_json_str, servinfo);
    sprintf(request_json_str, "{\"id\": %i}", 3);
    send_request_to_server(s, request_json_str, servinfo);
    int pollin_happened = start_polling(s, POLLIN);
    if (pollin_happened) {
        read_response_from_server(s, servinfo);
    } else {
        printf("Unexpected event polling occurred : %d\n", pollin_happened);
    }
}

// Func para testar delete de um filme.
// Formata request para envio, no formato {request_type: request_number} 
// e armazena em request_json_str. A request pode assumir valor entre 1 a 7.
// Envia a request feita para o servidor.
// Em seguida, formata uma string JSON no formato {id: 0}
// Para especificar a remocao do filme de ID 0.
// E envia esta string JSON para o servidor.
void delete_movies_request(int s, struct addrinfo * servinfo){
    char request_json_str[MAXLINE];
    for(int i = 0; i < 5; i++){
        printf("Removendo Filme Cadastrado \n");
        sprintf(request_json_str, "{\"%s\": %d}", "request_type", 7);
        send_request_to_server(s, request_json_str, servinfo);
        sprintf(request_json_str, "{\"id\": %i}", i);
        send_request_to_server(s, request_json_str, servinfo);
        int pollin_happened = start_polling(s, POLLIN);
        if (pollin_happened) {
            read_response_from_server(s, servinfo);
        } else {
            printf("Unexpected event polling occurred : %d\n", pollin_happened);
        }
    }
}

// Funcao principal do programa que cuida de se conectar com o servidor
// Na porta 9000 atraves do getaddrinfo para descobrir as informacoes do servidor.
// E em seguida chamada do OS para criar o socket e realizar a conexao.
// Se a conexao foi bem sucedida, requisita as operacoes de manipular filmes.
// E posteriormente fecha a conexao.
int main(int argc, char *argv[])
{
    int status,s;
    struct addrinfo hints;
    struct addrinfo *servinfo; // will point to the results

    memset(&hints, 0, sizeof(hints)); // make sure the struct is empty
    hints.ai_family = AF_INET; //don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_DGRAM; //UDP stream sockets
    hints.ai_flags = AI_PASSIVE; // fill in my IP for me
    
    if ((status = getaddrinfo("localhost", "9000", &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }
    if ((s = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1)
        perror("client: socket");
 
    create_movies_request(s, servinfo);
    list_movies_informations_by_id_request(s, servinfo);
    list_movies_informations_by_genre_request(s, servinfo);
    list_all_title_and_id_movies_request(s, servinfo);
    list_all_movies_informations_request(s, servinfo);
    add_genre_request(s, servinfo);
    list_all_movies_informations_request(s, servinfo);
    delete_movies_request(s, servinfo);
    list_all_movies_informations_request(s, servinfo);

    if(close(s) < 0){
        return -1;
    };
    return 0;
}
