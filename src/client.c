#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <protocol.h>

#define MAX_INPUT_LINE 16 

#define MIN_ACTION 0
#define MAX_ACTION 4

#define MAX_DATA_SIZE 256

int get_client_input();

int main(int argc, char *argv[]) {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    
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

    printf("conectado.\n");

    BattleMessage received_msg;
    BattleMessage client_response;
    int game_running = 1;
    int turn_number=0;

    while (game_running) {
        ssize_t numbytes = recv(sockfd, &received_msg, sizeof(BattleMessage), 0);
        if (numbytes == sizeof(BattleMessage)) {
            switch (received_msg.type) {
                case MSG_INIT:
                    printf("Conectado ao servidor.\n"); 
                    printf("Sua nave: SS-42 Voyager (HP: %d)\n", received_msg.client_hp); 
                    
                    break;
                
                case MSG_ACTION_REQ:
                    printf("Escolha sua acao:\n");
                    printf(" 0 Laser Attack\n");
                    printf(" 1 Photon Torpedo\n");
                    printf(" 2 Shields Up\n");
                    printf(" 3 Cloaking\n");
                    printf(" 4 Hyper Jump\n");
                    
                    int chosen_action = get_client_input(); 
                    turn_number++;
                    
                    memset(&client_response, 0, sizeof(BattleMessage));
                    client_response.type = MSG_ACTION_RES;
                    client_response.client_action = chosen_action;
                    
                    if (send(sockfd, &client_response, sizeof(BattleMessage), 0) == -1) {
                        perror("send action error");
                        game_running = 0;
                    }
                    break;
                    
                case MSG_BATTLE_RESULT:
                    printf("\n%s\n", received_msg.message);
                    break;
                    
                case MSG_GAME_OVER:
                case MSG_ESCAPE:
                    printf("%s\n", received_msg.message);
                    
                    printf("Inventario final:\n");
                    printf("HP restante da sua nave: %d\n", received_msg.client_hp);
                    printf("HP restante da nave inimiga: %d\n", received_msg.server_hp);
                    printf("Torpedos usados: %d\n", received_msg.client_torpedoes);
                    printf("Escudos usados: %d\n", received_msg.client_shields);
                    printf("Turnos jogados: %d\n", turn_number);
                    printf("Obrigado por jogar!\n");
                    
                    game_running = 0;
                    break;

                default:
                    fprintf(stderr, "Erro: Mensagem de tipo desconhecido (%d) recebida.\n", received_msg.type);
                    game_running = 0;
                    break;
            }
        } 
        else if (numbytes == 0) {
            printf("\nServidor fechou a conexão.\n");
            game_running = 0;
        } else if (numbytes == -1) {
            perror("recv error");
            game_running = 0;
        } else {
             fprintf(stderr, "Erro: Leitura parcial de mensagem.\n");
             game_running = 0;
        }
    }

    freeaddrinfo(servinfo);
    close(sockfd);

    return 0;
}

int get_client_input() {
    char input_line[MAX_INPUT_LINE];
    int action = -1;
    char extra_char;
    
    while (1) {
        printf("> ");
        fflush(stdout);

        if (fgets(input_line, MAX_INPUT_LINE, stdin) == NULL) {
            fprintf(stderr, "nao deu pra ler entradas.\n");
            exit(EXIT_FAILURE); 
        }

        sscanf(input_line, "%d%c", &action, &extra_char);

        if (action >= MIN_ACTION && action <= MAX_ACTION) {
            return action;
        } else {
            fprintf(stderr, "\nErro: escolha inválida!\n"); 
            fprintf(stderr, "Por favor selecione um valor entre %d e %d.\n", MIN_ACTION, MAX_ACTION); 
            printf("Escolha sua acao:\n"); 
            printf(" 0 Laser Attack\n 1 Photon Torpedo\n 2 Shields Up\n 3 Cloaking\n 4 Hyper Jump\n");
        }
    }
}
