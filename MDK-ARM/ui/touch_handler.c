#include "touch_handler.h"
#include "game_logic.h"
#include "doudizhu_adapter.h"
#include "game_anim.h"
#include "strings_cn.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>

static CardSelection selected_cards[MAX_SELECTED_CARDS];
static int selected_count = 0;
static CardData current_hand[MAX_SELECTED_CARDS];
static int current_hand_count = 0;
static lv_obj_t* g_player_hand_panel = NULL;
static lv_obj_t* g_control_panel = NULL;

// 滑动选牌状态
static bool slide_mode = false;
static lv_point_t slide_press_start;
static int slide_last_index = -1;
#define SLIDE_THRESHOLD 8  // 进入滑动模式的最小移动距离（像素）

// 计算对象在屏幕上的绝对坐标
static void get_obj_abs_pos_local(lv_obj_t* obj, lv_coord_t* x, lv_coord_t* y) {
    *x = 0;
    *y = 0;
    while (obj) {
        *x += lv_obj_get_x(obj);
        *y += lv_obj_get_y(obj);
        obj = lv_obj_get_parent(obj);
    }
}

// 查找触摸点下方对应的牌索引（不用 lv_obj_hit_test，因为牌已被设为 non-clickable）
static int find_card_under_point(lv_obj_t* panel, const lv_point_t* p) {
    if (!panel || !lv_obj_is_valid(panel)) return -1;

    lv_coord_t panel_abs_x = 0, panel_abs_y = 0;
    get_obj_abs_pos_local(panel, &panel_abs_x, &panel_abs_y);

    int child_count = (int)lv_obj_get_child_count(panel);
    for (int i = 0; i < child_count; i++) {
        lv_obj_t* card = lv_obj_get_child(panel, i);
        if (!card || !lv_obj_is_valid(card)) continue;

        lv_coord_t cx = panel_abs_x + lv_obj_get_x(card);
        lv_coord_t cy = panel_abs_y + lv_obj_get_y(card);
        lv_coord_t cw = lv_obj_get_width(card);
        lv_coord_t ch = lv_obj_get_height(card);

        if (p->x >= cx && p->x < cx + cw && p->y >= cy && p->y < cy + ch) {
            return i;
        }
    }
    return -1;
}

// 选中指定牌（不取消）
static void select_card_internal(int index, lv_obj_t* card_obj) {
    if (index < 0 || index >= current_hand_count) return;
    if (!card_obj || !lv_obj_is_valid(card_obj)) return;

    // 已在选中列表中
    for (int i = 0; i < selected_count; i++) {
        if (selected_cards[i].card_index == index) {
            if (!selected_cards[i].is_selected) {
                selected_cards[i].is_selected = true;
                selected_cards[i].card_obj = card_obj;
                lv_obj_set_style_border_width(card_obj, 2, LV_PART_MAIN);
                lv_obj_set_style_border_color(card_obj, lv_color_hex(0xFFD700), LV_PART_MAIN);
                lv_obj_set_style_bg_color(card_obj, lv_color_hex(0xFFE4B5), LV_PART_MAIN);
                anim_card_select(card_obj);
            }
            return;
        }
    }

    // 新增选中项
    if (selected_count < MAX_SELECTED_CARDS) {
        selected_cards[selected_count].card_index = index;
        selected_cards[selected_count].card_obj = card_obj;
        selected_cards[selected_count].is_selected = true;
        selected_count++;

        lv_obj_set_style_border_width(card_obj, 2, LV_PART_MAIN);
        lv_obj_set_style_border_color(card_obj, lv_color_hex(0xFFD700), LV_PART_MAIN);
        lv_obj_set_style_bg_color(card_obj, lv_color_hex(0xFFE4B5), LV_PART_MAIN);
        anim_card_select(card_obj);
    }
}

// 手牌区事件回调：支持点击单张和滑动多选
static void hand_panel_event_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t* indev = lv_indev_get_act();
    if (!indev) return;

    lv_point_t p;
    lv_indev_get_point(indev, &p);

    if (code == LV_EVENT_PRESSED) {
        slide_mode = false;
        slide_press_start = p;
        slide_last_index = -1;
    }
    else if (code == LV_EVENT_PRESSING) {
        lv_coord_t dx = p.x - slide_press_start.x;
        lv_coord_t dy = p.y - slide_press_start.y;

        if (!slide_mode && (dx * dx + dy * dy > SLIDE_THRESHOLD * SLIDE_THRESHOLD)) {
            slide_mode = true;
        }

        if (slide_mode) {
            int idx = find_card_under_point(g_player_hand_panel, &p);
            if (idx >= 0 && idx != slide_last_index) {
                lv_obj_t* card_obj = lv_obj_get_child(g_player_hand_panel, idx);
                // 滑动时切换选中状态：划过已选中的取消，未选中的选中
                touch_handler_toggle_card_selection(idx, card_obj);
                slide_last_index = idx;
            }
        }
    }
    else if (code == LV_EVENT_RELEASED) {
        if (!slide_mode) {
            // 点击模式：切换按下时对应牌的选择状态
            int idx = find_card_under_point(g_player_hand_panel, &slide_press_start);
            if (idx >= 0) {
                lv_obj_t* card_obj = lv_obj_get_child(g_player_hand_panel, idx);
                touch_handler_toggle_card_selection(idx, card_obj);
            }
        }
        slide_mode = false;
        slide_last_index = -1;
    }
}

static void btn_play_click_cb(lv_event_t* e) {
    (void)e;
    
    if (selected_count == 0) {
        game_ui_show_status(CN_43BD);
        return;
    }
    
    CardData selected[MAX_SELECTED_CARDS];
    int count = 0;
    touch_handler_get_selected_cards(selected, &count);
    
    // 调用游戏逻辑出牌
    game_logic_player_play(selected, count);
}

static void btn_pass_click_cb(lv_event_t* e) {
    (void)e;
    
    touch_handler_clear_selection();
    game_logic_player_pass();
}

static void btn_hint_click_cb(lv_event_t* e) {
    (void)e;
    
    // 清除之前的选择
    touch_handler_clear_selection();
    
    // 使用AI的出牌逻辑获取提示
    int indices[20];
    int count = 0;
    
    if (doudizhu_get_hint(indices, &count)) {
        // 选中提示的牌
        for (int i = 0; i < count; i++) {
            int idx = indices[i];
            lv_obj_t* card_obj = NULL;
            int child_count = (int)lv_obj_get_child_count(g_player_hand_panel);
            if (idx < child_count) {
                card_obj = lv_obj_get_child(g_player_hand_panel, idx);
            }
            
            if (card_obj) {
                touch_handler_toggle_card_selection(idx, card_obj);
            }
        }
        
        char status[32];
        snprintf(status, sizeof(status), CN_83BD, count);
        game_ui_show_status(status);
    } else {
        game_ui_show_status(CN_6BE6);
    }
}

void touch_handler_init(lv_obj_t* player_hand_panel, lv_obj_t* control_panel) {
    memset(selected_cards, 0, sizeof(selected_cards));
    selected_count = 0;
    current_hand_count = 0;
    g_player_hand_panel = player_hand_panel;
    g_control_panel = control_panel;

    if (g_player_hand_panel && lv_obj_is_valid(g_player_hand_panel)) {
        lv_obj_add_flag(g_player_hand_panel, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(g_player_hand_panel, hand_panel_event_cb, LV_EVENT_PRESSED, NULL);
        lv_obj_add_event_cb(g_player_hand_panel, hand_panel_event_cb, LV_EVENT_PRESSING, NULL);
        lv_obj_add_event_cb(g_player_hand_panel, hand_panel_event_cb, LV_EVENT_RELEASED, NULL);
    }

    touch_handler_register_button_events();
}

void touch_handler_register_card_events(void) {
    if (g_player_hand_panel == NULL) {
        return;
    }

    // 牌本身不接收点击事件，全部交给手牌区面板处理，
    // 否则事件会被子对象拦截，导致面板收不到滑动/点击
    int child_count = (int)lv_obj_get_child_count(g_player_hand_panel);
    for (int i = 0; i < child_count; i++) {
        lv_obj_t* card_obj = lv_obj_get_child(g_player_hand_panel, i);
        if (card_obj != NULL) {
            lv_obj_clear_flag(card_obj, LV_OBJ_FLAG_CLICKABLE);
        }
    }
}

void touch_handler_register_button_events(void) {
    if (g_control_panel == NULL) {
        return;
    }
    
    int child_count = (int)lv_obj_get_child_count(g_control_panel);
    for (int i = 0; i < child_count; i++) {
        lv_obj_t* btn = lv_obj_get_child(g_control_panel, i);
        if (btn == NULL) continue;
        
        lv_obj_t* label = lv_obj_get_child(btn, 0);
        if (label == NULL) continue;
        
        const char* text = lv_label_get_text(label);
        
        if (strcmp(text, CN_50FE) == 0) {
            lv_obj_add_event_cb(btn, btn_play_click_cb, LV_EVENT_CLICKED, NULL);
        } else if (strcmp(text, CN_7244) == 0) {
            lv_obj_add_event_cb(btn, btn_pass_click_cb, LV_EVENT_CLICKED, NULL);
        } else if (strcmp(text, CN_02D9) == 0) {
            lv_obj_add_event_cb(btn, btn_hint_click_cb, LV_EVENT_CLICKED, NULL);
        }
    }
}

void touch_handler_get_selected_cards(CardData* cards, int* count) {
    *count = 0;
    for (int i = 0; i < selected_count; i++) {
        if (selected_cards[i].is_selected && selected_cards[i].card_index < current_hand_count) {
            cards[*count] = current_hand[selected_cards[i].card_index];
            (*count)++;
        }
    }
}

void touch_handler_get_selected_indices(int* indices, int* count) {
    *count = 0;
    for (int i = 0; i < selected_count; i++) {
        if (selected_cards[i].is_selected && selected_cards[i].card_index < current_hand_count) {
            indices[*count] = selected_cards[i].card_index;
            (*count)++;
        }
    }
}

void touch_handler_clear_selection(void) {
    for (int i = 0; i < selected_count; i++) {
        if (selected_cards[i].card_obj != NULL && lv_obj_is_valid(selected_cards[i].card_obj)) {
            lv_obj_set_style_border_width(selected_cards[i].card_obj, 0, LV_PART_MAIN);
            lv_obj_set_style_bg_color(selected_cards[i].card_obj, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        }
    }
    
    memset(selected_cards, 0, sizeof(selected_cards));
    selected_count = 0;
}

bool touch_handler_is_card_selected(int index) {
    for (int i = 0; i < selected_count; i++) {
        if (selected_cards[i].card_index == index && selected_cards[i].is_selected) {
            return true;
        }
    }
    return false;
}

void touch_handler_toggle_card_selection(int index, lv_obj_t* card_obj) {
    for (int i = 0; i < selected_count; i++) {
        if (selected_cards[i].card_index == index) {
            if (selected_cards[i].is_selected) {
                selected_cards[i].is_selected = false;
                lv_obj_set_style_border_width(card_obj, 0, LV_PART_MAIN);
                lv_obj_set_style_bg_color(card_obj, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
                // 取消选中动画
                anim_card_deselect(card_obj);
            } else {
                selected_cards[i].is_selected = true;
                lv_obj_set_style_border_width(card_obj, 2, LV_PART_MAIN);
                lv_obj_set_style_border_color(card_obj, lv_color_hex(0xFFD700), LV_PART_MAIN);
                lv_obj_set_style_bg_color(card_obj, lv_color_hex(0xFFE4B5), LV_PART_MAIN);
                // 选中动画
                anim_card_select(card_obj);
            }
            return;
        }
    }
    
    if (selected_count < MAX_SELECTED_CARDS) {
        selected_cards[selected_count].card_index = index;
        selected_cards[selected_count].card_obj = card_obj;
        selected_cards[selected_count].is_selected = true;
        selected_count++;
        
        lv_obj_set_style_border_width(card_obj, 2, LV_PART_MAIN);
        lv_obj_set_style_border_color(card_obj, lv_color_hex(0xFFD700), LV_PART_MAIN);
        lv_obj_set_style_bg_color(card_obj, lv_color_hex(0xFFE4B5), LV_PART_MAIN);
        // 选中动画
        anim_card_select(card_obj);
    }
}

void touch_handler_update_hand(CardData* cards, int count) {
    current_hand_count = count;
    if (count > MAX_SELECTED_CARDS) {
        count = MAX_SELECTED_CARDS;
    }
    memcpy(current_hand, cards, count * sizeof(CardData));
    
    touch_handler_clear_selection();
    touch_handler_register_card_events();
}