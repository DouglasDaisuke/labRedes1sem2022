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

#define MAXLINE 4096
#define LISTENQ 100


// Func utilitaria para converter buffer recebido em objeto JSON.
// Recebe uma string, converte ela em objeto JSON e a retorna.
struct json_object * read_json_from_string(char * buffer) {
    struct json_object *json;
    json = json_tokener_parse(buffer);
    return json;
}

// Func utilitaria para ler JSON de requisicao do usuario e determinar tipo da op.
// Recebe o objeto JSON e retorna o numero representando a request feita pelo usuario. (Valores de 1 a 7).
int read_request_from_json(struct json_object *json){
    struct json_object *json_request;
    int request_type;
    json_object_object_get_ex(json, "request_type", &json_request);
    request_type = json_object_get_int(json_request);
    return request_type;
}

// Func para cadastrar filme de acordo com a entrada em JSON,
// Recebe o ID do socket criado, realiza uma leitura do que o cliente enviar em uma string,
// Em seguida, converte a string para objeto JSON, cria um nome de arquivo no formato {id.json}
// E salva este objeto JSON no arquivo de nome especificado acima.
void create_movie_from_client_request(int new_fd) {
    char buffer[MAXLINE];
    int readResult = read(new_fd, buffer, MAXLINE);
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
}

// Func para adicionar genero em filme de ID especificado pelo usuario.
// Recebe o ID do socket criado, realiza uma leitura do que o cliente enviar em uma string,
// Em seguida,  converte a string para objeto JSON, le qual o ID especificado e qual genero adicionar.
// Busca o filme de ID especificado no diretorio de filmes, se encontrar le o objeto JSON salvo,
// Verifica se tem menos que 2 generos, e se tiver salva o genero fornecido no objeto JSON e sobescreve o arquivo antigo.
// Caso contrario, imprime mensagem de erro.
void add_genre_from_client_request(int new_fd) {
    char buffer[MAXLINE];
    int readResult = read(new_fd, buffer, MAXLINE);
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
}

// Func para listar todos os titulos de id dos filmes
// Percorre o diretorio de filmes e para cada filme,se houver algum, então salva seu nome e id em uma lista
// Imprimi todos os nomes e id dos filmes dentro dessa lista, e caso não tenha nenhum filme, imprimi "lista vazia"
void list_all_title_and_id_movies_from_client_request(){
    int found = 0;
    DIR * dirp;
    struct dirent * entry;
    dirp = opendir("../movies");
    if (!dirp){
        perror ("Não foi possível abrir o diretorio");
        exit (1) ;
    }
    printf("Listagem dos Nomes e ID - ");
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
                printf("Nome: %s, ", movie_name);
                printf("ID: %s\n", movie_id);
                found++;
            }
        }
        if (found == 0) printf("Não foi encontrado nenhum filme");
    }
}

// Func que lista as informações dos filmes que são do genero especificado pelo cliente
// Recebe o ID do socket criado, realiza uma leitura do genero que o cliente enviar em uma string.
// Em seguida, converte a string para objeto JSON, le qual o genero especificado.
// Busca no diretorio de arquivos o filme com genero especificado e verifica se existe.
// Se existir adiciona as informacoes desse filme em uma lista, caso contrario, imprime mensagem de erro.
// Imprimi essa listagem de informacoes
void list_movies_informations_by_genre_from_client_request(int new_fd){
    char buffer[MAXLINE];
    int readResult = read(new_fd, buffer, MAXLINE);
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
                            printf("Informações sobre o filme - ");
                            printf("Nome: %s, ", movie_name);
                            printf("Diretor: %s, ", director);
                            printf("Ano: %s\n", year);
                            found++;
                        }
                    }
                }
            }
        }
        if (found == 0) printf("Os filmes com genero: %s, não foi encontrado", genre_target);
    }
};

// Func que lista as informacoes de todos os filmes.
// Percorre o diretorio de filmes, caso exista algum, lê os objetos JSON salvos e salva as informacoes dos filmes numa lista,
// Imprimi a lista de informacoes dos filmes, e caso não tenha nenhum filme, imprimi "lista vazia"
void list_all_movies_informations_from_client_request(){
    int found = 0;
    DIR * dirp;
    struct dirent * entry;
    dirp = opendir("../movies");
    if (!dirp){
        perror ("Não foi possível abrir o diretorio");
        exit (1) ;
    }
    printf("Listagem das informações dos filmes - ");
    while ((entry = readdir(dirp)) != NULL) {
        if (entry->d_type == DT_REG) { 
            char file_name[MAXLINE];
            sprintf(file_name, "../movies/%s", entry->d_name);
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
                printf("Nome: %s, ", movie_name);
                for (int arr_i = 0; arr_i < genre_array_length; arr_i++){
                    json_object * genre_array_obj = json_object_array_get_idx(genre_array_json, arr_i);
                    printf("Generos: %s, ", json_object_get_string(genre_array_obj));
                }
                printf("Diretor: %s, ", director);
                printf("Ano: %s\n", year);
                found++;
            }
        }
        if (found == 0) printf("Não foi encontrado nenhum filme");
    }
}  

// Func para listar as informacoes dos filmes que tiverem o id especificado pelo cliente.
// Recebe o ID do socket criado, realiza uma leitura do id do file que o cliente enviar em uma string,
// Busca no diretorio de arquivos o filme especificado e verifica se ele existe
// Se existir imprimi todas as suas informacoes, caso contrario, imprime mensagem de erro.
void list_movies_informations_by_id_from_client_request(int new_fd){
    char buffer[MAXLINE];
    int readResult = read(new_fd, buffer, MAXLINE);
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
            printf("Informações sobre o filme de id %d - ", id);
            printf("Nome: %s, ", movie_name);
            for (int arr_i = 0; arr_i < genre_array_length; arr_i++){
                json_object * genre_array_obj = json_object_array_get_idx(genre_array_json, arr_i);
                printf("Generos: %s, ", json_object_get_string(genre_array_obj));
            }
            printf("Diretor: %s, ", director);
            printf("Ano: %s\n", year);
        }
        else{
            printf("O filme com id: %d, não foi encontrado", id);
        }
    }
}

// Func para deletar filme de ID especificado pelo usuario.
// Recebe o ID do socket criado, realiza uma leitura do que o cliente enviar em uma string,
// Em seguida,  converte a string para objeto JSON, le qual o ID especificado.
// Busca no diretorio de arquivos o filme especificado e verifica se existe.
// Se existir deleta-o do diretorio de arquivos, caso contrario, imprime mensagem de erro.
void delete_movie_from_client_request(int new_fd) {
    char buffer[MAXLINE];
    int readResult = read(new_fd, buffer, MAXLINE);
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
}

// Logica para leitura das requisicoes do cliente.
// Recebe o ID do socket criado, e continuamente le as requisicoes do usuario,
// Ate receber um EOF ou erro de leitura, colocando cada requisicao na funcao apropriada para processamento.
// Operacoes possiveis:
// -1: Erro de leitura, 0: EOF, 1: Criar filme, 2: Adicionar genero, 4: Remover filme
void read_client_request(int new_fd){
    char buffer[MAXLINE];
    int request = 0;
    while(request != -1){
        int readResult = read(new_fd, buffer, MAXLINE);
        if(readResult >= 1){
            json_object * request_json = read_json_from_string(buffer);
            int request = read_request_from_json(request_json);
            switch (request)
            {
                case -1:
                    request = -1;
                break;

                case 0:
                    request = -1;
                break;

                case 1:
                    create_movie_from_client_request(new_fd);
                break;

                case 2:
                    add_genre_from_client_request(new_fd);
                break;

                case 3:
                    list_all_title_and_id_movies_from_client_request();
                break;

                case 4:
                    list_movies_informations_by_genre_from_client_request(new_fd);
                break;
                
                case 5:
                    list_all_movies_informations_from_client_request();
                break;

                case 6:
                    list_movies_informations_by_id_from_client_request(new_fd);
                break;

                case 7:
                    delete_movie_from_client_request(new_fd);
                break;
            }
        } else {
            request = -1;
        }
    }
}

// Func principal, que cuida de criar os processos filhos para lidar com as requisicoes de usuarios.
// Define que qualquer endereco IP pode se conectar com a porta 9000, e escuta requisicoes do usuario.
// Quando receber uma conexao, aceita a mesma e cria um filho para atender a requisicao,
// fechando o socket de escuta.

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
