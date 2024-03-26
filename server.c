#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024
#define DEFAULT_PORT 8888

// Structure to represent a client
typedef struct {
    int socket;
    char username[50];
    char ip[INET_ADDRSTRLEN];
    char status[20];
} Client;

// Structure to store server information
typedef struct {
    Client clients[MAX_CLIENTS];
    int client_count;
    pthread_mutex_t mutex;
} ServerInfo;

ServerInfo server;

// Function prototypes
void *handle_client(void *arg);
void handle_request(int client_socket, int option);
void register_user(int client_socket, char *username, char *ip);
void send_connected_users(int client_socket);
void change_status(char *username, char *status);
void send_message(int client_socket, char *recipient, char *message_text);
void send_user_info(int client_socket, char *username);
void send_response(int client_socket, int option, int code, char *message);

int main(int argc, char *argv[]) {
    int server_socket, client_socket, port;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    pthread_t tid;

    // Check if a port was provided as an argument, otherwise use the default port
    if (argc < 2) {
        port = DEFAULT_PORT;
    } else {
        port = atoi(argv[1]);
    }

    // Create server socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    // Set up server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind server socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to bind");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, 5) < 0) {
        perror("Failed to listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", port);

    // Initialize server structure
    server.client_count = 0;
    pthread_mutex_init(&server.mutex, NULL);

    while (1) {
        // Accept incoming connection
        if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len)) < 0) {
            perror("Failed to accept");
            exit(EXIT_FAILURE);
        }

        // Create a thread to handle the client
        if (pthread_create(&tid, NULL, handle_client, &client_socket) != 0) {
            perror("Failed to create thread");
            exit(EXIT_FAILURE);
        }
    }

    close(server_socket);
    pthread_mutex_destroy(&server.mutex);

    return 0;
}

// Function to handle client connections
void *handle_client(void *arg) {
    int client_socket = *((int *)arg);
    char buffer[BUFFER_SIZE];
    int option;

    // Read the client's option
    if (recv(client_socket, &option, sizeof(int), 0) <= 0) {
        perror("Error receiving option from client");
        close(client_socket);
        pthread_exit(NULL);
    }

    // Handle the client's request
    handle_request(client_socket, option);

    close(client_socket);
    pthread_exit(NULL);
}

// Function to handle client requests
void handle_request(int client_socket, int option) {
    switch (option) {
        case 1: // User Registration
            {
                char username[50], ip[INET_ADDRSTRLEN];
                // Receive client's registration data
                if (recv(client_socket, &username, sizeof(username), 0) <= 0 || recv(client_socket, &ip, sizeof(ip), 0) <= 0) {
                    perror("Error receiving user registration data from client");
                    close(client_socket);
                    pthread_exit(NULL);
                }
                register_user(client_socket, username, ip);
            }
            break;
        case 2: // Connected Users
            {
                send_connected_users(client_socket);
            }
            break;
        case 3: // Status Change
            {
                char username[50], status[20];
                // Receive client's status change data
                if (recv(client_socket, &username, sizeof(username), 0) <= 0 || recv(client_socket, &status, sizeof(status), 0) <= 0) {
                    perror("Error receiving status change data from client");
                    close(client_socket);
                    pthread_exit(NULL);
                }
                change_status(username, status);
            }
            break;
        case 4: // Messages
            {
                char recipient[50], message_text[BUFFER_SIZE];
                // Receive client's message data
                if (recv(client_socket, &recipient, sizeof(recipient), 0) <= 0 || recv(client_socket, &message_text, sizeof(message_text), 0) <= 0) {
                    perror("Error receiving message data from client");
                    close(client_socket);
                    pthread_exit(NULL);
                }
                send_message(client_socket, recipient, message_text);
            }
            break;
        case 5: // Information of a particular user
            {
                char username[50];
                // Receive client's username
                if (recv(client_socket, &username, sizeof(username), 0) <= 0) {
                    perror("Error receiving username from client");
                    close(client_socket);
                    pthread_exit(NULL);
                }
                send_user_info(client_socket, username);
            }
            break;
        default:
            // Invalid option
            int code = 500;
            char message[BUFFER_SIZE];
            sprintf(message, "Error: Invalid option");
            send_response(client_socket, option, code, message);
            break;
    }
}

// Function to register a new user
void register_user(int client_socket, char *username, char *ip) {
    int code;
    char message[BUFFER_SIZE];

    // Lock access to the server structure to prevent race conditions
    pthread_mutex_lock(&server.mutex);

    // Check if the user is already registered
    for (int i = 0; i < server.client_count; i++) {
        if (strcmp(server.clients[i].username, username) == 0) {
            code = 500; // User already registered
            sprintf(message, "Error: User '%s' already exists", username);
            send_response(client_socket, 1, code, message);
            pthread_mutex_unlock(&server.mutex);
            return;
        }
    }

    // Check limits to prevent buffer overflows
    if (strlen(username) >= sizeof(server.clients[0].username) || strlen(ip) >= INET_ADDRSTRLEN) {
        code = 500; // Invalid registration data
        sprintf(message, "Error: Invalid user registration data");
        send_response(client_socket, 1, code, message);
        pthread_mutex_unlock(&server.mutex);
        return;
    }

    // Register the new user
    strcpy(server.clients[server.client_count].username, username);
    strcpy(server.clients[server.client_count].ip, ip);
    strcpy(server.clients[server.client_count].status, "ACTIVE");
    server.clients[server.client_count].socket = client_socket;
    server.client_count++;

    // Send successful response to client
    code = 200;
    sprintf(message, "User '%s' registered successfully", username);
    send_response(client_socket, 1, code, message);

    pthread_mutex_unlock(&server.mutex);
}

// Function to send the list of connected users to the client
void send_connected_users(int client_socket) {
    int count = server.client_count;
    send(client_socket, &count, sizeof(int), 0);
    for (int i = 0; i < server.client_count; i++) {
        send(client_socket, &server.clients[i], sizeof(Client), 0);
    }
}

// Function to change a user's status
void change_status(char *username, char *status) {
    pthread_mutex_lock(&server.mutex);

    // Search for the user in the client list
    for (int i = 0; i < server.client_count; i++) {
        if (strcmp(server.clients[i].username, username) == 0) {
            // Update the user's status
            strcpy(server.clients[i].status, status);
            break;
        }
    }

    pthread_mutex_unlock(&server.mutex);
}

// Function to send a message to a user
void send_message(int client_socket, char *recipient, char *message_text) {
    pthread_mutex_lock(&server.mutex);

    // Check limits to prevent buffer overflows
    if (strlen(recipient) >= sizeof(server.clients[0].username) || strlen(message_text) >= BUFFER_SIZE) {
        pthread_mutex_unlock(&server.mutex);
        return;
    }

    // Search for the recipient in the client list
    for (int i = 0; i < server.client_count; i++) {
        if (strcmp(server.clients[i].username, recipient) == 0) {
            // Send the message to the recipient
            send(server.clients[i].socket, message_text, strlen(message_text), 0);
            break;
        }
    }

    pthread_mutex_unlock(&server.mutex);
}

// Function to send a user's information to the client
void send_user_info(int client_socket, char *username) {
    pthread_mutex_lock(&server.mutex);

    // Check limits to prevent buffer overflows
    if (strlen(username) >= sizeof(server.clients[0].username)) {
        pthread_mutex_unlock(&server.mutex);
        return;
    }

    // Search for the user in the client list
    for (int i = 0; i < server.client_count; i++) {
        if (strcmp(server.clients[i].username, username) == 0) {
            // Send the user's information to the client
            send(client_socket, &server.clients[i], sizeof(Client), 0);
            break;
        }
    }

    pthread_mutex_unlock(&server.mutex);
}

// Function to send a response to the client
void send_response(int client_socket, int option, int code, char *message) {
    send(client_socket, &option, sizeof(int), 0);
    send(client_socket, &code, sizeof(int), 0);
    send(client_socket, message, strlen(message), 0);
}