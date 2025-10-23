#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#define BACKLOG 1        
#define HELLO_MESSAGE "Bem-vindo ao StarFleet Protocol, Capitão! (MSG_INIT)\n"

void *get_in_addr(struct sockaddr *sa) {
    // Helper function to get sockaddr, IPv4 or IPv6:
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
    int server_fd, new_fd;  
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; 
    socklen_t sin_size;
    int status;
    char s[INET6_ADDRSTRLEN];
    
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <v4|v6> <porta>\n", argv[0]);
        exit(1);
    }
    
    const char *protocol_arg = argv[1]; // v4 ou v6
    const char *port_arg = argv[2];     // 5000

    memset(&hints, 0, sizeof hints);
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; 

    if (strcmp(protocol_arg, "v4") == 0) {
        hints.ai_family = AF_INET; // IPv4
    } else if (!strcmp(protocol_arg, "v6")) {
        hints.ai_family = AF_INET6; // IPv6
    } else {
        fprintf(stderr, "Erro: protocolo inválido. Use 'v4' ou 'v6'.\n");
        exit(1);
    }

    status = getaddrinfo(NULL, port_arg, &hints, &servinfo;
    if (status) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return 1;
    }

    int yes = 1;
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((server_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
        }

        if (bind(server_fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(server_fd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo);

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }

    if (listen(server_fd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    printf("Servidor StarFleet Protocol inicializado (%s:%s). Aguardando conexão...\n", protocol_arg, port_arg);

    new_fd = accept(server_fd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1) {
        perror("accept");
        close(server_fd);
        exit(1);
    }

    inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
    printf("Conexão aceita de %s\n", s);

    int len = strlen(HELLO_MESSAGE);
    if (send(new_fd, HELLO_MESSAGE, len, 0) == -1) {
        perror("send");
    } else {
        printf("Mensagem de 'hello' enviada ao cliente.\n");
    }

    close(new_fd);
    close(server_fd);

    return 0;
}