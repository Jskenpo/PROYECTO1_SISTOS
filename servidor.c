#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "chat_protocol.pb-c.h" // Importar el archivo de protocolo generado por Protocol Buffers

#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024
#define DEFAULT_PORT 8888

typedef struct {
    int socket;
    char username[50];
    char status[20];
} Client;

typedef struct {
    Client clients[MAX_CLIENTS];
    int client_count;
    pthread_mutex_t mutex;
} ServerInfo;

ServerInfo server;

void *handle_client(void *arg);

void handle_request(int client_socket);

void register_user(int client_socket, const char *username);

void send_connected_users(int client_socket);

void change_status(int client_socket, const char *new_status);

void send_user_info(int client_socket, const char *username);

void send_response(int client_socket, int option, int code, const char *message);

int main(int argc, char *argv[]) {
    int server_socket, client_socket, port;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    pthread_t tid;

    port = (argc < 2) ? DEFAULT_PORT : atoi(argv[1]);

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Error creating server socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding server socket");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 5) < 0) {
        perror("Error listening on server socket");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", port);

    server.client_count = 0;
    pthread_mutex_init(&server.mutex, NULL);

    while (1) {
        if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len)) < 0) {
            perror("Error accepting connection");
            exit(EXIT_FAILURE);
        }

        if (pthread_create(&tid, NULL, handle_client, &client_socket) != 0) {
            perror("Error creating thread");
            exit(EXIT_FAILURE);
        }
    }

    close(server_socket);
    pthread_mutex_destroy(&server.mutex);

    return 0;
}

void *handle_client(void *arg) {
    int client_socket = *((int *)arg);
    handle_request(client_socket);
    close(client_socket);
    pthread_exit(NULL);
}

void handle_request(int client_socket) {
    ClientPetition *request = NULL;
    uint8_t buffer[BUFFER_SIZE];
    ssize_t len;

    while ((len = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        request = client_petition__unpack(NULL, len, buffer);
        if (request == NULL) {
            perror("Error unpacking client request");
            continue;
        }

        switch (request->option) {
            case 1: // Registro de Usuarios
                register_user(client_socket, request->registration->username);
                break;
            case 2: // Usuarios Conectados
                send_connected_users(client_socket);
                break;
            case 3: // Cambio de Estado
                change_status(client_socket, request->change->status);
                break;
            case 5: // Información de un usuario en particular
                send_user_info(client_socket, request->users->user);
                break;
            default:
                send_response(client_socket, request->option, 500, "Invalid option");
                break;
        }

        client_petition__free_unpacked(request, NULL);
    }

    if (len == 0) {
        printf("Client disconnected\n");
    } else {
        perror("Error receiving from client");
    }
}

void register_user(int client_socket, const char *username) {
    int code;
    char message[BUFFER_SIZE];

    pthread_mutex_lock(&server.mutex);

    for (int i = 0; i < server.client_count; i++) {
        if (strcmp(server.clients[i].username, username) == 0) {
            code = 500; // Usuario ya registrado
            sprintf(message, "Error: User '%s' already exists", username);
            send_response(client_socket, 1, code, message);
            pthread_mutex_unlock(&server.mutex);
            return;
        }
    }

    strcpy(server.clients[server.client_count].username, username);
    strcpy(server.clients[server.client_count].status, "ACTIVE");
    server.clients[server.client_count].socket = client_socket;
    server.client_count++;

    code = 200;
    sprintf(message, "User '%s' registered successfully", username);
    send_response(client_socket, 1, code, message);

    pthread_mutex_unlock(&server.mutex);
}

void send_connected_users(int client_socket) {
    ConnectedUsersResponse response = CONNECTED_USERS_RESPONSE__INIT;
    UserInfo **user_infos = NULL;
    size_t num_users = server.client_count;

    user_infos = malloc(num_users * sizeof(UserInfo *));
    if (user_infos == NULL) {
        perror("Error allocating memory");
        return;
    }

    for (int i = 0; i < num_users; i++) {
        user_infos[i] = malloc(sizeof(UserInfo));
        user_info__init(user_infos[i]);
        user_infos[i]->username = server.clients[i].username;
        user_infos[i]->status = server.clients[i].status;
    }

    response.connectedusers = user_infos;
    response.n_connectedusers = num_users;

    size_t response_len = connected_users_response__get_packed_size(&response);
    uint8_t *response_buffer = malloc(response_len);
    if (response_buffer == NULL) {
        perror("Error allocating memory");
        return;
    }

    connected_users_response__pack(&response, response_buffer);

    send(client_socket, response_buffer, response_len, 0);

    free(response_buffer);
    for (int i = 0; i < num_users; i++) {
        free(user_infos[i]);
    }
    free(user_infos);
}

void change_status(int client_socket, const char *new_status) {
    pthread_mutex_lock(&server.mutex);
    for (int i = 0; i < server.client_count; i++) {
        if (server.clients[i].socket == client_socket) {
            strcpy(server.clients[i].status, new_status);
            send_response(client_socket, 3, 200, "Status updated successfully");
            break;
        }
    }
    pthread_mutex_unlock(&server.mutex);
}

void send_user_info(int client_socket, const char *username) {
    int user_found = 0;
    UserInfo *user_info = NULL;

    pthread_mutex_lock(&server.mutex);

    for (int i = 0; i < server.client_count; i++) {
        if (strcmp(server.clients[i].username, username) == 0) {
            user_info = malloc(sizeof(UserInfo));
            user_info__init(user_info);
            user_info->username = server.clients[i].username;
            user_info->status = server.clients[i].status;
            user_found = 1;
            break;
        }
    }

    pthread_mutex_unlock(&server.mutex);

    if (user_found) {
        size_t user_info_len = user_info__get_packed_size(user_info);
        uint8_t *user_info_buffer = malloc(user_info_len);
        if (user_info_buffer == NULL) {
            perror("Error allocating memory");
            return;
        }

        user_info__pack(user_info, user_info_buffer);

        send(client_socket, user_info_buffer, user_info_len, 0);

        free(user_info_buffer);
        free(user_info);
    } else {
        send_response(client_socket, 5, 500, "User not found");
    }
}

void send_response(int client_socket, int option, int code, const char *message) {
    ServerResponse response = SERVER_RESPONSE__INIT;
    response.option = option;
    response.code = code;
    response.servermessage = message;

    size_t response_len = server_response__get_packed_size(&response);
    uint8_t *response_buffer = malloc(response_len);
    if (response_buffer == NULL) {
        perror("Error allocating memory");
        return;
    }

    server_response__pack(&response, response_buffer);

    send(client_socket, response_buffer, response_len, 0);

    free(response_buffer);
}
