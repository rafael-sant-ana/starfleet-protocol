#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define MAX_DATA_SIZE 256

int main(int argc, char *argv[]) {
    int sockfd, numbytes;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
    char buf[MAX_DATA_SIZE];
    
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <hostname/IP> <porta>\n", argv[0]);
        exit(1);
    }
    
    const char *hostname = argv[1];
    const char *port = argv[2];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;  
    hints.ai_socktype = SOCK_STREAM; // TCP

    if ((rv = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: falha ao conectar\n");
        return 2;
    }

    printf("Conectado ao servidor.\n");
    
    if ((numbytes = recv(sockfd, buf, MAX_DATA_SIZE - 1, 0)) == -1) {
        perror("recv");
        exit(1);
    }
    
    if (numbytes > 0) {
        buf[numbytes] = '\0';
        printf("Mensagem do Servidor: %s\n", buf);
    } else {
        printf("Servidor fechou a conexão.\n");
    }

    freeaddrinfo(servinfo);
    close(sockfd);

    return 0;
}