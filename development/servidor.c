#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <json-c/json.h>
#include <ctype.h>
#include <fcntl.h>
#include <poll.h>

#define MAXLINE 4096
#define LISTENQ 100
#define TIMEOUT_MILISEC 4000

// Func utilitaria para converter buffer recebido em objeto JSON.
// Recebe uma string, converte ela em objeto JSON e a retorna.
struct json_object * read_json_from_string(char * buffer) {
    struct json_object *json;
    json = json_tokener_parse(buffer);
    return json;
}

// Func utilitaria para enviar mensagens informativas ao cliente.
void send_message_to_client(int new_fd, struct addrinfo * servinfo, char * message){
    char buffer[MAXLINE];
    sprintf(buffer, "%s", message);
    int response_code = sendto(new_fd, buffer, MAXLINE, 0, servinfo->ai_addr, servinfo->ai_addrlen);
    if(response_code <= 0)
        exit(1);
}

// Func utilitaria para ler JSON de requisicao do usuario e determinar tipo da op.
// Recebe o objeto JSON e retorna o numero representando a request feita pelo usuario. (Valores de 1 a 7).
int read_request_from_json(struct json_object *json){
    struct json_object *json_request;
    int request_type;
    if (json_object_object_get_ex(json, "request_type", &json_request)){
        request_type = json_object_get_int(json_request);
        return request_type;
    }
    return -1;
}

// Func utilitaria para validar input do cliente e verificar se numero de bytes estao corretos e ordenados.
int validate_response(char * message){
    if(strlen(message) != 19)
        return -1;
    if(read_json_from_string(message) == NULL)
        return -1;
    struct json_object *json = read_json_from_string(message);
    int request_type = read_request_from_json(json);
    return request_type;
}

// Func utilitaria para verificar input do cliente pos validacao da response.
// Recebe tipo da request e mensagem lida do cliente de entrada, e retorna 1 se mensagem estiver correta,
// -1 se estiver incorreta.
int validate_client_content(int request_type, char * message){
    char check[MAXLINE][15] = {"id", "movie_name", "director", "genre", "year"};

    if(read_json_from_string(message) == NULL)
        return -1;

    if(request_type == 1){
        struct json_object *movie = read_json_from_string(message);
        struct json_object *validator;
        for(int i = 0; i < 5; i++){
            char *to_validate = check[i];
            if(json_object_object_get_ex(movie, to_validate, &validator) != true){
                return -1;
            }
        }
        return 1;
    }
    if(request_type == 2){
        struct json_object *json = read_json_from_string(message);
        struct json_object *id_json;
        if (json_object_object_get_ex(json, "id", &id_json) != true){
            return -1;
        }
        struct json_object *genre_json;
        if (json_object_object_get_ex(json, "genre", &genre_json) != true){
            return -1;
        }
        return 1;
    }

    if(request_type == 4){
        struct json_object *json = read_json_from_string(message);
        struct json_object *genre_json;
        if(json_object_object_get_ex(json, "genre", &genre_json) != true)
            return -1;
        return 1;
    }

    if(request_type == 6){
        struct json_object *json = read_json_from_string(message);        
        struct json_object *id_json;
        if (json_object_object_get_ex(json, "id", &id_json) != true){
            return -1;
        }
        return 1;
    }

    if(request_type == 7){
        struct json_object *json = read_json_from_string(message);
        struct json_object *id_json;
        json_object_object_get_ex(json, "id", &id_json);
        int id = json_object_get_int(id_json);
        if(json_object_object_get_ex(json, "id", &id_json) != true)
            return -1;
        return 1;
    }
    return -1;
}

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

// Func para cadastrar filme de acordo com a entrada em JSON,
// Recebe o ID do socket criado, realiza uma leitura do que o cliente enviar em uma string,
// Em seguida, converte a string para objeto JSON, cria um nome de arquivo no formato {id.json}
// E salva este objeto JSON no arquivo de nome especificado acima.
void create_movie_from_client_request(int new_fd, struct addrinfo * servinfo) {
    char buffer[MAXLINE] = {0};
    socklen_t fromlen;
    fromlen = sizeof  servinfo->ai_addr;
    int pollin_happened = start_polling(new_fd, POLLIN);
    if (pollin_happened) {
        int readResult = recvfrom(new_fd, buffer, MAXLINE,0, servinfo->ai_addr, &fromlen);

        if(validate_client_content(1, buffer) == -1)
            printf("lost or corrupted message - function 1\n");
        else{
            if(readResult >= 1){
                struct json_object *movie = read_json_from_string(buffer);
                struct json_object *movie_name_json;
                struct json_object *id_json;
                json_object_object_get_ex(movie, "id", &id_json);
                json_object_object_get_ex(movie, "movie_name", &movie_name_json);
                int id = json_object_get_int(id_json);
                const char *movie_name = json_object_get_string(movie_name_json);
                char file_name[MAXLINE];
                sprintf(file_name, "../movies/%i%s", id, ".json");
                json_object_to_file(file_name, movie);
                printf("Filme de ID %i e nome %s cadastrado com sucesso! \n", id, movie_name);
            }
            send_message_to_client(new_fd, servinfo, "servidor: Filme criado com sucesso!");
        }
    } else {
        printf("Unexpected event polling occurred : %d\n", pollin_happened);
    }
}

// Func para adicionar genero em filme de ID especificado pelo usuario.
// Recebe o ID do socket criado, realiza uma leitura do que o cliente enviar em uma string,
// Em seguida,  converte a string para objeto JSON, le qual o ID especificado e qual genero adicionar.
// Busca o filme de ID especificado no diretorio de filmes, se encontrar le o objeto JSON salvo,
// Verifica se tem menos que 2 generos, e se tiver salva o genero fornecido no objeto JSON e sobescreve o arquivo antigo.
// Caso contrario, imprime mensagem de erro.
void add_genre_from_client_request(int new_fd, struct addrinfo * servinfo) {
    char buffer[MAXLINE] = {0};
    socklen_t fromlen;
    fromlen = sizeof  servinfo->ai_addr;
    int pollin_happened = start_polling(new_fd, POLLIN);
    if (pollin_happened) {
        int readResult = recvfrom(new_fd, buffer, MAXLINE,0, servinfo->ai_addr, &fromlen);
        if(validate_client_content(2, buffer) == -1)
            printf("lost or corrupted message - function 2\n");
        else {
            if(readResult >= 1){
                struct json_object *json = read_json_from_string(buffer);
                struct json_object *id_json;
                json_object_object_get_ex(json, "id", &id_json);
                int id = json_object_get_int(id_json);
                struct json_object *genre_json;
                json_object_object_get_ex(json, "genre", &genre_json);
                const char *genre = json_object_get_string(genre_json);
                char file_name[MAXLINE];
                char file_contents[MAXLINE];
                sprintf(file_name, "../movies/%i%s", id, ".json");
                struct json_object *saved_movie = json_object_from_file(file_name);
                if(saved_movie != NULL){
                    struct json_object *genre_array;
                    json_object_object_get_ex(saved_movie, "genre", &genre_array);
                    int genre_array_length = json_object_array_length(genre_array);
                    if (genre_array_length < 2){
                        json_object_array_add(genre_array, genre_json);
                        json_object_to_file(file_name, saved_movie);
                        printf("Adicionado genero %s no filme de ID %i \n", genre, id);
                    } else {
                        printf("O filme de id %i ja possui 2 generos. \n", id);
                    }
                }
            }
            send_message_to_client(new_fd, servinfo, "servidor: Genero adicionado com sucesso!");
        }
    } else {
        printf("Unexpected event polling occurred : %d\n", pollin_happened);
    }
}

// Func para listar todos os titulos de id dos filmes
// Percorre o diretorio e para cada arquivo encontrado, irá ler o arquivo que contem informações do filme
// Lê o objeto JSON que esta o nome e o ID do filme e converte em string
// Imprimi o valor do nome do ID lido
void list_all_title_and_id_movies_from_client_request(int new_fd, struct addrinfo * servinfo){
    int found = 0;
    DIR * dirp;
    struct dirent * entry;
    dirp = opendir("../movies");
    if (!dirp){
        perror ("Não foi possível abrir o diretorio");
        exit (1) ;
    }
    char response[MAXLINE] = "servidor: {\"list_name_id\":[";
    while ((entry = readdir(dirp)) != NULL) {
        if (entry->d_type == DT_REG) { 
            char file_name[MAXLINE];
            sprintf(file_name, "../movies/%s", entry->d_name);
            struct json_object *saved_movie = json_object_from_file(file_name);
            if(saved_movie != NULL){
                struct json_object *movie_name_json;
                struct json_object *movie_id_json;
                json_object_object_get_ex(saved_movie, "movie_name", &movie_name_json);
                json_object_object_get_ex(saved_movie, "id", &movie_id_json);
                const char* movie_name = json_object_get_string(movie_name_json);
                const char* movie_id = json_object_get_string(movie_id_json);
                strcat(response, "{\"movie_name\":\"");
                strcat(response, movie_name);
                strcat(response, "\",\"id\":\"");
                strcat(response, movie_id);
                strcat(response, "\"},");
                found++;
            }
        }
    }
    strcat(response, "]}");
    printf("%s\n", response);
    if (found == 0) printf("servidor: Não foi encontrado nenhum filme");
        send_message_to_client(new_fd, servinfo, response);
}

// Func que lista as informações dos filmes que são do genero especificado pelo cliente
// Recebe o ID do socket criado, realiza uma leitura do genero que o cliente enviar em uma string.
// Percorre o diretorio e para cada arquivo encontrado, irá ler o arquivo que conterá as informações do filme
// Faz a leitura do objeto JSON e percorre a lista com os generos do filme e compara com o genero procurado
// Se os generos forem iguais, então fazemos a leitura das informações do filme e imprimimos
// Se não, continua percorrendo a lista de genero
// Se não for encontrado nenhum filme com o genero especificado em todo diretorio, é impresso uma mensagem avisando que não foi encontrado
void list_movies_informations_by_genre_from_client_request(int new_fd, struct addrinfo * servinfo){
    char buffer[MAXLINE] = {0};
    socklen_t fromlen;
    fromlen = sizeof  servinfo->ai_addr;
    int pollin_happened = start_polling(new_fd, POLLIN);
    if (pollin_happened) {
        int readResult = recvfrom(new_fd, buffer, MAXLINE,0, servinfo->ai_addr, &fromlen);
        if(validate_client_content(4, buffer) == -1)
            printf("lost or corrupted message - function 4\n");
        else {
            int found = 0;
            if(readResult >= 1){
                struct json_object *json = read_json_from_string(buffer);
                struct json_object *genre_json;
                json_object_object_get_ex(json, "genre", &genre_json);
                const char *genre_target = json_object_get_string(genre_json);
                DIR * dirp;
                struct dirent * entry;
                dirp = opendir("../movies");
                if (!dirp){
                    perror ("Não foi possível abrir o diretorio");
                    exit (1) ;
                }
                char response[MAXLINE] = "servidor: {\"list_info\":[";
                while ((entry = readdir(dirp)) != NULL) {
                    if (entry->d_type == DT_REG) {  // verifica se é um arquivo
                        char file_name[MAXLINE];
                        sprintf(file_name, "../movies/%s", entry->d_name);
                        struct json_object *saved_movie = json_object_from_file(file_name);
                        if(saved_movie != NULL){ 
                            struct json_object *genre_array_json;
                            json_object_object_get_ex(saved_movie, "genre", &genre_array_json);
                            int genre_array_length = json_object_array_length(genre_array_json);
                            for (int arr_i = 0; arr_i < genre_array_length; arr_i++){
                                struct json_object * genre_saved_movie = json_object_array_get_idx(genre_array_json, arr_i);
                                if (strcmp(genre_target,json_object_get_string(genre_saved_movie)) == 0){
                                    struct json_object *movie_name_json;
                                    struct json_object *director_json;
                                    struct json_object *year_json;
                                    json_object_object_get_ex(saved_movie, "movie_name", &movie_name_json);
                                    json_object_object_get_ex(saved_movie, "director", &director_json);
                                    json_object_object_get_ex(saved_movie, "year", &year_json);
                                    const char* movie_name = json_object_get_string(movie_name_json);
                                    const char* director = json_object_get_string(director_json);
                                    const char* year = json_object_get_string(year_json);
                                    strcat(response, "{\"movie_name\":\"");
                                    strcat(response, movie_name);
                                    strcat(response, "\",\"director\":\"");
                                    strcat(response, director);
                                    strcat(response, "\",\"year\":\"");
                                    strcat(response, year);
                                    strcat(response, "\"},");
                                    found++;
                                }
                            }
                        }
                    }
                }
                strcat(response, "]}");
                printf("%s\n", response);
                if (found == 0) printf("servidor: Os filmes com genero: %s, não foi encontrado", genre_target);
                    send_message_to_client(new_fd, servinfo, response);
            }
        }
    } else {
        printf("Unexpected event polling occurred : %d\n", pollin_happened);
    }
};

// Func que lista as informacoes de todos os filmes.
// Percorre o diretorio e para cada arquivo encontrado, irá ler o arquivo que contem informações do filme
// Lê o objeto JSON que contem as informações do filme e converte em string
// Imprimi o valor de todas as informações do filme
void list_all_movies_informations_from_client_request(int new_fd, struct addrinfo * servinfo){
    int found = 0;
    DIR * dirp;
    struct dirent * entry;
    dirp = opendir("../movies");
    if (!dirp){
        perror ("Não foi possível abrir o diretorio");
        exit (1) ;
    }
    char response[MAXLINE] = "servidor: {\"list_info\":[";
    while ((entry = readdir(dirp)) != NULL) {
        if (entry->d_type == DT_REG) { 
            char file_name[MAXLINE];
            sprintf(file_name, "../movies/%s", entry->d_name);
            struct json_object *saved_movie = json_object_from_file(file_name);
            if(saved_movie != NULL){
                struct json_object *movie_id_json;
                struct json_object *movie_name_json;
                struct json_object *genre_array_json;
                struct json_object *director_json;
                struct json_object *year_json;
                json_object_object_get_ex(saved_movie, "id", &movie_id_json);
                json_object_object_get_ex(saved_movie, "movie_name", &movie_name_json);
                json_object_object_get_ex(saved_movie, "genre", &genre_array_json);
                json_object_object_get_ex(saved_movie, "director", &director_json);
                json_object_object_get_ex(saved_movie, "year", &year_json);
                const char* id = json_object_get_string(movie_id_json);
                const char* movie_name = json_object_get_string(movie_name_json);
                int genre_array_length = json_object_array_length(genre_array_json);
                const char* director = json_object_get_string(director_json);
                const char* year = json_object_get_string(year_json);
                strcat(response, "{\"movie_name\":\"");
                strcat(response, movie_name);
                strcat(response, "\",\"genre\":[\"");
                for (int arr_i = 0; arr_i < genre_array_length; arr_i++){
                    json_object * genre_array_obj = json_object_array_get_idx(genre_array_json, arr_i);
                    if (arr_i > 0 || genre_array_length == 1){
                        strcat(response, json_object_get_string(genre_array_obj));
                    }else{
                        strcat(response, json_object_get_string(genre_array_obj));
                        strcat(response, "\",\"");
                    }
                    printf("%s, ", json_object_get_string(genre_array_obj));
                }
                strcat(response, "\"],\"director\":\"");
                strcat(response, director);
                strcat(response, "\",\"year\":\"");
                strcat(response, year);
                strcat(response, "\",\"id\":\"");
                strcat(response, id);
                strcat(response, "\"},");
                found++;
            }
        }
    }
    strcat(response, "]}");
    printf("%s\n", response);
    if (found == 0) printf("servidor: Não foi encontrado nenhum filme ");
        send_message_to_client(new_fd, servinfo, response);
}  

// Func para listar as informacoes dos filmes que tiverem o ID especificado pelo cliente.
// Recebe o ID do socket criado, realiza uma leitura do ID que o cliente enviar em uma string,
// Busca no diretorio de arquivos o filme com o ID especificado e verifica se ele existe
// Se existir imprimi todas as suas informacoes, caso contrario, imprime mensagem de erro.
void list_movies_informations_by_id_from_client_request(int new_fd, struct addrinfo * servinfo){
    char buffer[MAXLINE] = {0};
    socklen_t fromlen;
    fromlen = sizeof  servinfo->ai_addr;
    char response[MAXLINE] = "servidor: {\"list_info\":[";
    int pollin_happened = start_polling(new_fd, POLLIN);
    if (pollin_happened) {
        int readResult = recvfrom(new_fd, buffer, MAXLINE,0, servinfo->ai_addr, &fromlen);
        if(validate_client_content(6, buffer) == -1)
            printf("lost or corrupted message - function 6\n");
        else {
            if(readResult >= 1){
                struct json_object *json = read_json_from_string(buffer);        
                struct json_object *id_json;
                json_object_object_get_ex(json, "id", &id_json);
                int id = json_object_get_int(id_json);
                char file_name[MAXLINE];
                sprintf(file_name, "../movies/%i%s", id, ".json");
                struct json_object *saved_movie = json_object_from_file(file_name);
                if(saved_movie != NULL){
                    struct json_object *movie_name_json;
                    struct json_object *genre_array_json;
                    struct json_object *director_json;
                    struct json_object *year_json;
                    json_object_object_get_ex(saved_movie, "movie_name", &movie_name_json);
                    json_object_object_get_ex(saved_movie, "genre", &genre_array_json);
                    json_object_object_get_ex(saved_movie, "director", &director_json);
                    json_object_object_get_ex(saved_movie, "year", &year_json);
                    const char* movie_name = json_object_get_string(movie_name_json);
                    int genre_array_length = json_object_array_length(genre_array_json);
                    const char* director = json_object_get_string(director_json);
                    const char* year = json_object_get_string(year_json);
                    strcat(response, "{\"movie_name\":\"");
                    strcat(response, movie_name);
                    strcat(response, "\",\"genre\":[\"");
                    for (int arr_i = 0; arr_i < genre_array_length; arr_i++){
                        json_object * genre_array_obj = json_object_array_get_idx(genre_array_json, arr_i);
                        if (arr_i > 0 || genre_array_length == 1){
                            strcat(response, json_object_get_string(genre_array_obj));
                        }else{
                            strcat(response, json_object_get_string(genre_array_obj));
                            strcat(response, "\",\"");
                        }
                        printf("%s, ", json_object_get_string(genre_array_obj));
                    }
                    strcat(response, "\"],\"director\":\"");
                    strcat(response, director);
                    strcat(response, "\",\"year\":\"");
                    strcat(response, year);
                    strcat(response, "\"},");
                }
                else{
                    printf("servidor: O filme com id: %d, não foi encontrado", id);
                }
                strcat(response, "]}");
                printf("%s\n", response);
                send_message_to_client(new_fd, servinfo, response);
            }
        }
    } else {
        printf("Unexpected event polling occurred : %d\n", pollin_happened);
    }
}

// Func para deletar filme de ID especificado pelo usuario.
// Recebe o ID do socket criado, realiza uma leitura do que o cliente enviar em uma string,
// Em seguida,  converte a string para objeto JSON, le qual o ID especificado.
// Busca no diretorio de arquivos o filme especificado e verifica se existe.
// Se existir deleta-o do diretorio de arquivos, caso contrario, imprime mensagem de erro.
void delete_movie_from_client_request(int new_fd, struct addrinfo * servinfo) {
    char buffer[MAXLINE] = {0};
    socklen_t fromlen;
    fromlen = sizeof  servinfo->ai_addr;
    int pollin_happened = start_polling(new_fd, POLLIN);
    if (pollin_happened) {
        int readResult = recvfrom(new_fd, buffer, MAXLINE,0, servinfo->ai_addr, &fromlen);
        if(validate_client_content(7, buffer) == -1)
            printf("lost or corrupted message - function 7\n");
        else {
            if(readResult >= 1){
                struct json_object *json = read_json_from_string(buffer);
                struct json_object *id_json;
                json_object_object_get_ex(json, "id", &id_json);
                int id = json_object_get_int(id_json);
                char file_name[MAXLINE];
                sprintf(file_name, "../movies/%i%s", id, ".json");
                FILE *fp = fopen(file_name, "r");
                if (fp){
                    remove(file_name);
                    printf("Filme de ID %i removido com sucesso! \n", id);
                }
                else{
                    printf("O filme especificado nao esta cadastrado! \n");
                }
            }
            send_message_to_client(new_fd, servinfo, "Filme deletado com sucesso!");
        }
    }else {
        printf("Unexpected event polling occurred : %d\n", pollin_happened);
    }
}

// Logica para leitura das requisicoes do cliente.
// Recebe o ID do socket criado, e continuamente le as requisicoes do usuario,
// Ate receber um EOF ou erro de leitura, colocando cada requisicao na funcao apropriada para processamento.
// Operacoes possiveis:
// -1: Erro de leitura, 0: EOF, 1: Criar filme, 2: Adicionar genero, 4: Remover filme
void read_client_request(int new_fd, struct addrinfo * servinfo){
    char buffer[MAXLINE] = {0};
    char response[MAXLINE];
    int request = 0;
    int response_code, num_events, count = 0;
    socklen_t fromlen;
    fromlen = sizeof  servinfo->ai_addr;
    while(request != -1){
        //printf("count: %d\n", count);
        //if (count == 13){
        //    request = -1;
        //}
        //count = count+= 1;
        int pollin_happened = start_polling(new_fd, POLLIN);
        if (pollin_happened) {
            int readResult = recvfrom(new_fd, buffer, MAXLINE,0, servinfo->ai_addr, &fromlen);
            if(readResult >= 1){
                int request = validate_response(buffer);
                //printf("request: %d\n", request);
                switch (request)
                {
                    case -1:
                        request = -1;
                    break;

                    case 0:
                        request = -1;
                    break;

                    case 1:
                        create_movie_from_client_request(new_fd, servinfo);
                    break;

                    case 2:
                        add_genre_from_client_request(new_fd, servinfo);
                    break;

                    case 3:
                        list_all_title_and_id_movies_from_client_request(new_fd, servinfo);
                    break;

                    case 4:
                        list_movies_informations_by_genre_from_client_request(new_fd, servinfo);
                    break;
                    
                    case 5:
                        list_all_movies_informations_from_client_request(new_fd, servinfo);
                    break;

                    case 6:
                        list_movies_informations_by_id_from_client_request(new_fd, servinfo);
                    break;

                    case 7:
                        delete_movie_from_client_request(new_fd, servinfo);
                    break;
                }
            } else {
                request = -1;
            }
        } else {
            printf("Unexpected event polling occurred : %d\n", pollin_happened);
        }
        
        
    }
}

// Func principal, que ouve as as requisicoes de usuarios e retorna o desejado para o proprio usuario
// Define que qualquer endereco IP pode se conectar com a porta 9000, e escuta requisicoes do usuario.
// Quando receber uma conexao, aceita a mesma e fica esperando para receber a requisicao,


int main(int argc, char **argv)
{
    int status, sock_fd;
    struct addrinfo hints, *servinfo;
    struct sockaddr_in cliaddr, servaddr;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

    if ((status = getaddrinfo("localhost", "9000", &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }
    if ((sock_fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1) {
        perror("server: socket");
    }
    if ((bind(sock_fd, servinfo->ai_addr, servinfo->ai_addrlen)) == -1) {
        close(sock_fd);
        perror("server: bind");
    }
    while(true){
        read_client_request(sock_fd, servinfo);
        close(sock_fd);     
        exit(0);     
    }
    return 0;
}
