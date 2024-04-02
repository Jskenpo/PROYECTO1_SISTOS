#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "chat_protocol.pb-c.h" // Incluimos el archivo generado por el compilador de Protocol Buffers

#define BUFFER_SIZE 1024
#define MAX_USERNAME_LENGTH 50

void displayMenu() {
    printf("\nOpciones disponibles:\n");
    printf("1. Registrar\n");
    printf("2. Enviar mensaje privado\n");
    printf("3. Cambiar de estado\n");
    printf("4. Listar los usuarios conectados\n");
    printf("5. Obtener información de un usuario\n");
    printf("6. Chatear grupalmente\n");
    printf("7. Ayuda\n");
    printf("8. Salir\n");
    printf("Elige una opción: ");
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <nombredeusuario> <IPdelservidor> <puertodelservidor>\n", argv[0]);
        exit(1);
    }

    char *username = argv[1];
    char *ip = argv[2];
    int port = atoi(argv[3]);

    int sock;
    struct sockaddr_in addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("[-]Socket error");
        exit(1);
    }

    printf("[+]TCP server socket created.\n");

    memset(&addr, '\0', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Connection failed");
        exit(1);
    }
    printf("Connected to the server.\n");

    while (1) {
        displayMenu();

        int option;
        scanf("%d", &option);
        getchar();

        switch (option) {
            case 1: {
                // Construimos el mensaje de registro
                ClientRequest request = CLIENT_REQUEST__INIT;
                request.option = 1;

                UserRegistration registration = USER_REGISTRATION__INIT;
                registration.username = username;
                registration.ip = ip;

                request.registration = &registration;

                // Serializamos el mensaje
                size_t request_len = client_request__get_packed_size(&request);
                uint8_t *request_buffer = malloc(request_len);
                client_request__pack(&request, request_buffer);

                // Enviamos el mensaje
                send(sock, request_buffer, request_len, 0);

                printf("Ya está registrado: %s\n", username);
                break;
            }

            // Resto de los casos...
        }
    }

    return 0;
}
