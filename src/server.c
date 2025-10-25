#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <time.h>      // pra ter  srand()

#include "game_state.h"
#include "protocol.h"

#define BACKLOG 1
#define BASE_DAMAGE 20

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// so para encapsular a logica de ver se as mensagens foram enviadas;
int send_message(int fd, const BattleMessage *msg);

// encapsula colocar string na result msg 
void append_message(BattleMessage *result_msg, const char *string_to_append);

// processa de vdd os danos
void process_turn(const BattleMessage *client_action_msg, GameState *state, BattleMessage *result_msg);

int main(int argc, char *argv[]) {
    srand(time(NULL)); 

    int server_fd, new_fd;  
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; 
    socklen_t sin_size;
    int status;
    char s[INET6_ADDRSTRLEN];
    
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <v4|v6> <porta>\n", argv[0]);
        return 1;
    }
    
    const char *protocol_arg = argv[1]; 
    const char *port_arg = argv[2];     

    memset(&hints, 0, sizeof hints);
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; 

    if (strcmp(protocol_arg, "v4") == 0) {
        hints.ai_family = AF_INET; 
    } else if (strcmp(protocol_arg, "v6") == 0) {
        hints.ai_family = AF_INET6; 
    } else {
        fprintf(stderr, "Erro: protocolo invalido. Use 'v4' ou 'v6'.\n");
        return 1;
    }

    if ((status = getaddrinfo(NULL, port_arg, &hints, &servinfo)) != 0) {
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
        return 1;
    }

    printf("servidor StarFleet-Protocol inicializado em (%s:%s). aguardando conexao...\n", protocol_arg, port_arg);

    while (1) { 
        printf("aguardando conexao...\n");
        sin_size = sizeof their_addr;
        new_fd = accept(server_fd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("erro no aceite");
            continue;
        }

        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
        printf("conexao aceita de \"%s\". iniciando a batalha.\n", s);

        // estado inicial do jogo
        GameState state = {
            .client_hp = 100,
            .server_hp = 100,
            .client_torpedoes = 0,
            .client_shields = 0,
            .turns = 0
        };

        BattleMessage init_msg;
        memset(&init_msg, 0, sizeof(BattleMessage));
        init_msg.type = MSG_INIT; 
        init_msg.client_hp = state.client_hp; 
        init_msg.server_hp = state.server_hp; 
        strncpy(init_msg.message, "Bem-vindo ao StarFleet Protocol, Capitao! Que a batalha comece.", MSG_SIZE);

        if (send_message(new_fd, &init_msg) == -1) {
            close(new_fd);
            continue; 
        } 
        
        int game_over = 0;
        while (!game_over) {
            BattleMessage req_msg, res_msg, client_action_msg;
            ssize_t bytes_received;

            state.turns++; 

            memset(&req_msg, 0, sizeof(BattleMessage));
            req_msg.type = MSG_ACTION_REQ;
            snprintf(req_msg.message, MSG_SIZE, "turno %d. HPs: Voce (%d) | Inimigo (%d). Escolha sua açao.", 
                     state.turns, state.client_hp, state.server_hp);
            
            if (send_message(new_fd, &req_msg) == -1) {
                game_over = 1;  // pq deu erro, vamos parar
                break;
            }
            
            printf("turno %d: aguardando açao de quem ta conectado...\n", state.turns);
            // aqui a gnt ta recebendo
            bytes_received = recv(new_fd, &client_action_msg, sizeof(BattleMessage), 0);

            if (bytes_received == sizeof(BattleMessage) && client_action_msg.type == MSG_ACTION_RES) {
                int action = client_action_msg.client_action;
                printf("acao do jogador: %d\n", action);

                if (action < 0 || action > 4) {
                    fprintf(stderr, "aviso: cliente enviou uma açao que nao e valida (%d).\n", action);
                } else if (action == 4) { 
                    // termina o jogo prematuramente
                    game_over = 1;
                    memset(&res_msg, 0, sizeof(BattleMessage));
                    res_msg.type = MSG_ESCAPE;
                    res_msg.client_hp = state.client_hp;
                    res_msg.client_torpedoes = state.client_torpedoes;
                    res_msg.client_shields = state.client_shields;
                    snprintf(res_msg.message, MSG_SIZE, "voce acionou o Hyper Jump e escapou para o hiperespaço.");
                    send_message(new_fd, &res_msg);
                    break;
                }
                
                // acoes de 0 a 3 sao processadas aqui
                process_turn(&client_action_msg, &state, &res_msg);
                
                if (send_message(new_fd, &res_msg) == -1) {
                    game_over = 1; 
                }

                if (state.client_hp <= 0 || state.server_hp <= 0) {
                    game_over = 1; 
                }

            } else if (bytes_received == 0) {
                printf("cliente saiu\n");
                game_over = 1;
            } else {
                perror("recv error");
                game_over = 1;
            }
        } 

        printf("Fim de jogo!  fechando socket.\n");
        close(new_fd); 
    } 

    close(server_fd); 
    return 0;
}

int send_message(int fd, const BattleMessage *msg) {
    if (send(fd, msg, sizeof(BattleMessage), 0) == -1) {
        perror("erro no envio de msg");
        return -1;
    }
    return 0;
}

 void process_turn(const BattleMessage *client_action_msg, GameState *state, BattleMessage *result_msg) {
    
    Action client_action = (Action)client_action_msg->client_action;
    
    Action server_action = (Action)(rand() % SERVER_ACTION_COUNT);

    int client_damage = 0;
    int server_damage = 0;

    char formatted_string[128];

    // limpando as msg
    memset(result_msg->message, 0, MSG_SIZE);

    if (client_action == ACTION_TORPEDO) { 
        state->client_torpedoes++;
        append_message(result_msg, "Cliente carregou um Torpedo. ");
    } else if (client_action == ACTION_SHIELDS) { 
        state->client_shields++;
    }

    // --- Client ataca e servidor defende ---

    int client_attacked = (client_action == ACTION_LASER || client_action == ACTION_TORPEDO);
    if (client_attacked) {
        int server_blocked = 0;
        
        if (server_action == ACTION_SHIELDS) {
            snprintf(formatted_string, sizeof(formatted_string), "Ataque do cliente bloqueado pelos Escudos do inimigo! ");
            append_message(result_msg, formatted_string);
            server_blocked = 1;
        } else if (server_action == ACTION_CLOAKING && client_action == ACTION_TORPEDO) {
            snprintf(formatted_string, sizeof(formatted_string), "Torpedo do cliente evitado pela Camuflagem do inimigo! ");
            append_message(result_msg, formatted_string);
            server_blocked = 1;
        }

        if (!server_blocked) {
            server_damage += BASE_DAMAGE;
            snprintf(formatted_string, sizeof(formatted_string), "Cliente disparou %s! Acerto no inimigo. ", (client_action == ACTION_LASER ? "Laser" : "Torpedo"));
            append_message(result_msg, formatted_string);
        }
    }

    // server ataca e cliente defende
    int server_attacked = (server_action == ACTION_LASER || server_action == ACTION_TORPEDO);
    if (server_attacked) {
        int client_blocked = 0;

        if (client_action == ACTION_SHIELDS) {
            snprintf(formatted_string, sizeof(formatted_string), "Ataque do inimigo bloqueado pelos Escudos do cliente! ");
            append_message(result_msg, formatted_string);
            client_blocked = 1;
        } else if (client_action == ACTION_CLOAKING && server_action == ACTION_TORPEDO) {
            snprintf(formatted_string, sizeof(formatted_string), "Torpedo do inimigo evitado pela Camuflagem do cliente! ");
            append_message(result_msg, formatted_string);
            client_blocked = 1;
        }

        if (!client_blocked) {
            client_damage += BASE_DAMAGE;
            snprintf(formatted_string, sizeof(formatted_string), "Inimigo disparou %s! Acerto no cliente. ", (server_action == ACTION_LASER ? "Laser" : "Torpedo"));
            append_message(result_msg, formatted_string);
        }
    }

    // os 2 atacam. mas lembra que torpedo > laser (de acordo com o documento)
    if (client_action == ACTION_TORPEDO && server_action == ACTION_LASER) {
        // lciente ganha
        server_damage = BASE_DAMAGE;
        client_damage = 0;          
        char* msg = "CLIENTE Torpedo VENCE Laser! Inimigo sofre 20 HP. ";
        strncpy(result_msg->message, msg ,strlen(msg)+1); 
    } else if (client_action == ACTION_LASER && server_action == ACTION_TORPEDO) {
        // server ganha
        client_damage = BASE_DAMAGE;
        server_damage = 0;          
        char* msg = "INIMIGO Torpedo VENCE Laser! Cliente sofre 20 HP. ";
        strncpy(result_msg->message, msg, strlen(msg)+1); 
    } else if (client_action == server_action) {
        if (client_attacked) { 
            client_damage = BASE_DAMAGE;
            server_damage = BASE_DAMAGE;

            snprintf(formatted_string, sizeof(formatted_string), "ambos sofrem %d HP de dano. ", BASE_DAMAGE);
            append_message(result_msg, formatted_string);
        } else if (client_action == ACTION_SHIELDS || client_action == ACTION_CLOAKING) {
            snprintf(formatted_string, sizeof(formatted_string), "manobras defensivas mutuas. Sem dano. ");
            append_message(result_msg, formatted_string);
        }
    }

    state->client_hp -= client_damage;
    state->server_hp -= server_damage;
    
    if (state->client_hp < 0) state->client_hp = 0;
    if (state->server_hp < 0) state->server_hp = 0;

    if (strnlen(result_msg->message, MSG_SIZE) == 0) {
        snprintf(formatted_string, sizeof(formatted_string), "nenhuma interaçao de dano neste turno.");
        append_message(result_msg, formatted_string);
    }

    snprintf(formatted_string, sizeof(formatted_string), " | Placar: Voce %d x %d Inimigo", state->client_hp, state->server_hp);
        append_message(result_msg, formatted_string);
    
    result_msg->client_hp = state->client_hp;
    result_msg->server_hp = state->server_hp;
    result_msg->client_torpedoes = state->client_torpedoes;
    result_msg->client_shields = state->client_shields;
    result_msg->type = MSG_BATTLE_RESULT;
}


void append_message(BattleMessage *result_msg, const char *string_to_append) {
    if (!string_to_append) {
        return;
    }

    size_t current_len = strnlen(result_msg->message, MSG_SIZE);
    size_t string_len = strnlen(string_to_append, MSG_SIZE);
    size_t remaining_size = MSG_SIZE - current_len;

    if (remaining_size <= 1) {
        return;
    }

    strncat(result_msg->message, string_to_append, remaining_size - 1);
}
