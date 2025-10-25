#ifndef PROTOCOL_H
#define PROTOCOL_H

#define MSG_SIZE 256 


typedef enum {
    MSG_INIT, // Boas vi n da s
    MSG_ACTION_REQ, // S o l i c i t a c a o de acao
    MSG_ACTION_RES, // Cli e n t e re sponde com acao e s c o l h i d a
    MSG_BATTLE_RESULT, // Re sul tado do tu rno
    MSG_INVENTORY, // A t uali za cao de i n v e n t a r i o
    MSG_GAME_OVER, // Fim de jogo
    MSG_ESCAPE // Jogador f u gi u   
} MessageType;


typedef struct {
    int type;                
    int client_action;       
    int server_action;       
    int client_hp;           
    int server_hp;           
    int client_torpedoes;    
    int client_shields;      
    char message[MSG_SIZE];  
} BattleMessage;

#endif 