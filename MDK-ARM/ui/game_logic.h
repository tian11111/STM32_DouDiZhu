#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include "game_ui.h"

typedef enum {
    GAME_STATE_IDLE = 0,
    GAME_STATE_DEALING,
    GAME_STATE_ROB_LANDLORD,
    GAME_STATE_PLAYING,
    GAME_STATE_GAME_OVER
} GameStateType;

void game_logic_init(void);
void game_logic_start_new_game(void);
void game_logic_start_after_difficulty(void);
void game_logic_start_rob_landlord(void);
void game_logic_player_play(CardData* cards, int count);
void game_logic_player_pass(void);
void game_logic_get_hint(CardData* cards, int* count);
void game_logic_update(void);
void game_logic_player_rob_landlord(bool rob);
void game_logic_auto_run_ai(void);

#endif
