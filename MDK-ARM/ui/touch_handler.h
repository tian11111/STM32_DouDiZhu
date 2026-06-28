#ifndef TOUCH_HANDLER_H
#define TOUCH_HANDLER_H

#include "lvgl.h"
#include "game_ui.h"

#define MAX_SELECTED_CARDS 20

typedef struct {
    int card_index;
    lv_obj_t* card_obj;
    bool is_selected;
} CardSelection;

void touch_handler_init(lv_obj_t* player_hand_panel, lv_obj_t* control_panel);
void touch_handler_register_card_events(void);
void touch_handler_register_button_events(void);
void touch_handler_get_selected_cards(CardData* cards, int* count);
void touch_handler_get_selected_indices(int* indices, int* count);
void touch_handler_clear_selection(void);
bool touch_handler_is_card_selected(int index);
void touch_handler_toggle_card_selection(int index, lv_obj_t* card_obj);
void touch_handler_update_hand(CardData* cards, int count);

#endif