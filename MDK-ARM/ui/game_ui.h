#ifndef GAME_UI_H
#define GAME_UI_H

#include "lvgl.h"
#include "card_render.h"

/* 横屏 320x240 布局定义 */
#define SCREEN_WIDTH        320
#define SCREEN_HEIGHT       240

#define AI_PANEL_WIDTH      60
#define AI_PANEL_HEIGHT     44

#define PLAY_AREA_Y         45
#define PLAY_AREA_HEIGHT    65

#define STATUS_LABEL_Y      3

#define PLAYER_HAND_Y       145
#define PLAYER_HAND_HEIGHT  45

#define CONTROL_PANEL_Y     210
#define CONTROL_PANEL_HEIGHT 30

typedef struct {
    lv_obj_t* scr;
    lv_obj_t* ai_left_panel;
    lv_obj_t* ai_right_panel;
    lv_obj_t* center_area;
    lv_obj_t* player_hand;
    lv_obj_t* control_panel;
    lv_obj_t* status_label;
    
    lv_obj_t* ai_left_name;
    lv_obj_t* ai_left_count;
    lv_obj_t* ai_right_name;
    lv_obj_t* ai_right_count;
    
    lv_obj_t* play_area_left;
    lv_obj_t* play_area_center;
    lv_obj_t* play_area_right;
    
    lv_obj_t* landlord_cards_area;
    lv_obj_t* landlord_label;
    
    lv_obj_t* btn_play;
    lv_obj_t* btn_pass;
    lv_obj_t* btn_hint;
    lv_obj_t* btn_restart;
    lv_obj_t* btn_rob;
    lv_obj_t* btn_no_rob;
    
    lv_obj_t* btn_diff_easy;
    lv_obj_t* btn_diff_normal;
    lv_obj_t* btn_diff_hard;
} GameUI;

void game_ui_init(void);
void game_ui_update_ai(int ai_index, const char* name, int card_count);
void game_ui_update_player_hand(CardData* cards, int count);
void game_ui_show_play(int player_idx, CardData* cards, int count);
void game_ui_show_status(const char* message);
void game_ui_clear_play_area(void);
void game_ui_show_rob_buttons(bool show);
void game_ui_show_landlord_cards(CardData* cards, int count);
void game_ui_show_landlord_label(int player_idx, const char* name);
void game_ui_show_pass(int player_idx);
void game_ui_show_difficulty_buttons(bool show);
void game_ui_show_invalid_play_reason(int* indices, int count);
void game_ui_animate_landlord_cards_to_hand(CardData* landlord_cards, int landlord_count, CardData* player_hand, int hand_count);

// 玩家出牌飞入动画：selected_indices 为当前手牌中选中的索引，played_cards 为出的牌
void game_ui_animate_player_play(CardData* played_cards, int played_count, int* selected_indices, int selected_count, bool continue_ai);

lv_obj_t* game_ui_get_player_hand(void);
lv_obj_t* game_ui_get_ai_panel(int ai_idx);

#endif
