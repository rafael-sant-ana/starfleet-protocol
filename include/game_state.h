#ifndef GAME_STATE_H
#define GAME_STATE_H

typedef enum {
  ACTION_LASER = 0,
  ACTION_TORPEDO = 1,
  ACTION_SHIELDS = 2,
  ACTION_CLOAKING = 3,
  ACTION_HYPERJUMP = 4,
  ACTION_COUNT = 5, // temos 5 actions. a gente vai usar isso aqui pra fazer % ACTION_COUNT pra ter uma action de 0 a 4
  SERVER_ACTION_COUNT = 4 // servidor nao tem hyperjump, entao so 4 actions pra ele
} Action;

typedef struct {
    int client_hp;
    int server_hp;
    int client_torpedoes;
    int client_shields;
    int turns;
} GameState;

#endif
