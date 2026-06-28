#include "game_ui.h"
#include "touch_handler.h"
#include "game_logic.h"
#include "doudizhu_adapter.h"
#include "game_anim.h"
#include "strings_cn.h"
#include <stdio.h>
#include <string.h>

static GameUI game_ui;

// 颜色定义（经典斗地主风格）
#define COLOR_BG_TOP        0x0b3d18  // 顶部深绿
#define COLOR_BG_BOTTOM     0x1e7a38  // 底部浅绿（渐变终点）
#define COLOR_BG_PANEL      0x0d3018  // 面板半透明深绿
#define COLOR_BG_CARD       0xffffff  // 白色牌面
#define COLOR_BORDER_CARD   0x8b6914  // 金色边框
#define COLOR_TEXT_WHITE     0xffffff
#define COLOR_TEXT_YELLOW    0xffd700
#define COLOR_TEXT_GRAY      0xaaaaaa
#define COLOR_BTN_GREEN      0x2e8b57
#define COLOR_BTN_ORANGE     0xff8c00
#define COLOR_BTN_RED        0xdc143c
#define COLOR_ACCENT         0xffd700  // 金色强调色

extern const lv_font_t doudizhu_font;

static lv_font_t* get_font(void) {
    return (lv_font_t*)&doudizhu_font;
}

// 面板样式（带圆角和阴影）
static void apply_panel_style(lv_obj_t* panel, lv_color_t bg_color) {
    lv_obj_set_style_bg_color(panel, bg_color, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(panel, LV_OPA_90, LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(panel, lv_color_hex(COLOR_ACCENT), LV_PART_MAIN);
    lv_obj_set_style_radius(panel, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(panel, 4, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(panel, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(panel, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(panel, LV_OPA_30, LV_PART_MAIN);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
}

// 区域样式
static void apply_area_style(lv_obj_t* area, lv_color_t bg_color) {
    lv_obj_set_style_bg_color(area, bg_color, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(area, LV_OPA_80, LV_PART_MAIN);
    lv_obj_set_style_border_width(area, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(area, lv_color_hex(0x2d5016), LV_PART_MAIN);
    lv_obj_set_style_radius(area, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_all(area, 3, LV_PART_MAIN);
    lv_obj_clear_flag(area, LV_OBJ_FLAG_SCROLLABLE);
}

// 创建AI头像+牌数面板
static void create_ai_panel(lv_obj_t* parent, lv_obj_t** panel,
                           lv_obj_t** name_label, lv_obj_t** count_label,
                           lv_align_t align, int x_off) {
    *panel = lv_obj_create(parent);
    lv_obj_set_size(*panel, AI_PANEL_WIDTH, AI_PANEL_HEIGHT);
    lv_obj_align(*panel, align, x_off, 5);
    apply_panel_style(*panel, lv_color_hex(COLOR_BG_PANEL));

    // AI名称
    *name_label = lv_label_create(*panel);
    lv_label_set_text(*name_label, "AI");
    lv_obj_set_style_text_color(*name_label, lv_color_hex(COLOR_TEXT_WHITE), LV_PART_MAIN);
    lv_obj_set_style_text_font(*name_label, get_font(), LV_PART_MAIN);
    lv_obj_align(*name_label, LV_ALIGN_TOP_MID, 0, 2);

    // 牌数（大号金色数字）
    *count_label = lv_label_create(*panel);
    lv_label_set_text(*count_label, "17");
    lv_obj_set_style_text_color(*count_label, lv_color_hex(COLOR_TEXT_YELLOW), LV_PART_MAIN);
    lv_obj_set_style_text_font(*count_label, get_font(), LV_PART_MAIN);
    lv_obj_align(*count_label, LV_ALIGN_BOTTOM_MID, 0, -2);
    
    // 添加脉冲效果
    anim_pulse(*count_label);
}

static void create_play_area(lv_obj_t* parent, lv_obj_t** area,
                             lv_coord_t x, lv_coord_t w) {
    *area = lv_obj_create(parent);
    lv_obj_set_size(*area, w, PLAY_AREA_HEIGHT);
    lv_obj_set_pos(*area, x, PLAY_AREA_Y);
    apply_area_style(*area, lv_color_hex(COLOR_BG_PANEL));
    // 添加内部发光效果
    lv_obj_set_style_shadow_width(*area, 12, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(*area, lv_color_hex(0x00ff00), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(*area, LV_OPA_10, LV_PART_MAIN);
}

// 创建玩家手牌区
static void create_player_hand_area(lv_obj_t* parent) {
    game_ui.player_hand = lv_obj_create(parent);
    lv_obj_set_size(game_ui.player_hand, SCREEN_WIDTH - 10, PLAYER_HAND_HEIGHT + 20);  // 增加高度
    lv_obj_align(game_ui.player_hand, LV_ALIGN_TOP_MID, 0, PLAYER_HAND_Y);
    apply_area_style(game_ui.player_hand, lv_color_hex(COLOR_BG_PANEL));
    // 启用水平滚动
    lv_obj_set_scroll_dir(game_ui.player_hand, LV_DIR_NONE);
    lv_obj_set_scroll_snap_x(game_ui.player_hand, LV_SCROLL_SNAP_CENTER);
    lv_obj_clear_flag(game_ui.player_hand, LV_OBJ_FLAG_SCROLLABLE);
    // 设置内边距
    lv_obj_set_style_pad_top(game_ui.player_hand, 5, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(game_ui.player_hand, 5, LV_PART_MAIN);
}

// 创建状态标签
static void create_status_label(lv_obj_t* parent) {
    game_ui.status_label = lv_label_create(parent);
    lv_label_set_text(game_ui.status_label, CN_3371);
    lv_obj_set_style_text_color(game_ui.status_label, lv_color_hex(COLOR_TEXT_YELLOW), LV_PART_MAIN);
    lv_obj_set_style_text_font(game_ui.status_label, get_font(), LV_PART_MAIN);
    lv_obj_set_style_text_letter_space(game_ui.status_label, 2, LV_PART_MAIN);
    lv_obj_align(game_ui.status_label, LV_ALIGN_TOP_MID, 0, STATUS_LABEL_Y);
}

static void btn_event_cb(lv_event_t* e) {
    lv_obj_t* label = (lv_obj_t*)lv_event_get_user_data(e);
    if (!label) return;
    const char* text = lv_label_get_text(label);

    if (strcmp(text, CN_5027) == 0) {
        game_logic_start_new_game();
    }
}

// 创建按钮（带渐变效果）
static lv_obj_t* create_button(lv_obj_t* parent, const char* text, lv_coord_t x, lv_coord_t w, lv_color_t bg_color) {
    lv_obj_t* btn = lv_btn_create(parent);
    lv_obj_set_size(btn, w, 22);
    lv_obj_set_pos(btn, x, 0);
    lv_obj_set_style_bg_color(btn, bg_color, LV_PART_MAIN);
    lv_obj_set_style_radius(btn, 6, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 4, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(btn, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_20, LV_PART_MAIN);

    lv_obj_t* label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_hex(COLOR_TEXT_WHITE), LV_PART_MAIN);
    lv_obj_set_style_text_font(label, get_font(), LV_PART_MAIN);
    lv_obj_center(label);

    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, label);
    return btn;
}

// 抢地主按钮回调
static void rob_btn_event_cb(lv_event_t* e) {
    lv_obj_t* label = (lv_obj_t*)lv_event_get_user_data(e);
    if (!label) return;
    const char* text = lv_label_get_text(label);

    if (strcmp(text, CN_20D3) == 0) {
        game_logic_player_rob_landlord(true);
    } else if (strcmp(text, CN_8464) == 0) {
        game_logic_player_rob_landlord(false);
    }
}

// 难度按钮回调
static void diff_btn_event_cb(lv_event_t* e) {
    lv_obj_t* label = (lv_obj_t*)lv_event_get_user_data(e);
    if (!label) return;
    const char* text = lv_label_get_text(label);

    if (strcmp(text, CN_D517) == 0) {
        doudizhu_set_difficulty(0);
        game_ui_show_status(CN_AFEC);
    } else if (strcmp(text, CN_3524) == 0) {
        doudizhu_set_difficulty(1);
        game_ui_show_status(CN_4E0B);
    } else if (strcmp(text, CN_D6F3) == 0) {
        doudizhu_set_difficulty(2);
        game_ui_show_status(CN_3666);
    }

    game_logic_start_after_difficulty();
}

static void create_control_panel(lv_obj_t* parent) {
    game_ui.control_panel = lv_obj_create(parent);
    lv_obj_set_size(game_ui.control_panel, SCREEN_WIDTH, CONTROL_PANEL_HEIGHT);
    lv_obj_align(game_ui.control_panel, LV_ALIGN_TOP_MID, 0, CONTROL_PANEL_Y);
    apply_area_style(game_ui.control_panel, lv_color_hex(COLOR_BG_PANEL));

    // 使用更合理的按钮布局
    int btn_w = 60;
    int btn_gap = 10;
    int total_w = 4 * btn_w + 3 * btn_gap;
    int start_x = (SCREEN_WIDTH - total_w) / 2;

    game_ui.btn_pass = create_button(game_ui.control_panel, CN_7244, start_x, btn_w, lv_color_hex(COLOR_BTN_RED));
    game_ui.btn_hint = create_button(game_ui.control_panel, CN_02D9, start_x + btn_w + btn_gap, btn_w, lv_color_hex(COLOR_BTN_ORANGE));
    game_ui.btn_play = create_button(game_ui.control_panel, CN_50FE, start_x + 2 * (btn_w + btn_gap), btn_w, lv_color_hex(COLOR_BTN_GREEN));
    game_ui.btn_restart = create_button(game_ui.control_panel, CN_5027, start_x + 3 * (btn_w + btn_gap), btn_w, lv_color_hex(0x666666));
    
    // 抢地主按钮（初始隐藏）- 放在中央出牌区下方
    game_ui.btn_rob = lv_btn_create(parent);
    lv_obj_set_size(game_ui.btn_rob, 90, 35);
    lv_obj_align(game_ui.btn_rob, LV_ALIGN_TOP_MID, -55, 155);
    lv_obj_set_style_bg_color(game_ui.btn_rob, lv_color_hex(COLOR_BTN_ORANGE), LV_PART_MAIN);
    lv_obj_set_style_radius(game_ui.btn_rob, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(game_ui.btn_rob, 6, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(game_ui.btn_rob, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(game_ui.btn_rob, LV_OPA_30, LV_PART_MAIN);
    lv_obj_t* rob_label = lv_label_create(game_ui.btn_rob);
    lv_label_set_text(rob_label, CN_20D3);
    lv_obj_set_style_text_color(rob_label, lv_color_hex(COLOR_TEXT_WHITE), LV_PART_MAIN);
    lv_obj_set_style_text_font(rob_label, get_font(), LV_PART_MAIN);
    lv_obj_center(rob_label);
    lv_obj_add_event_cb(game_ui.btn_rob, rob_btn_event_cb, LV_EVENT_CLICKED, rob_label);
    lv_obj_add_flag(game_ui.btn_rob, LV_OBJ_FLAG_HIDDEN);
    
    game_ui.btn_no_rob = lv_btn_create(parent);
    lv_obj_set_size(game_ui.btn_no_rob, 90, 35);
    lv_obj_align(game_ui.btn_no_rob, LV_ALIGN_TOP_MID, 55, 155);
    lv_obj_set_style_bg_color(game_ui.btn_no_rob, lv_color_hex(0x666666), LV_PART_MAIN);
    lv_obj_set_style_radius(game_ui.btn_no_rob, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(game_ui.btn_no_rob, 6, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(game_ui.btn_no_rob, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(game_ui.btn_no_rob, LV_OPA_30, LV_PART_MAIN);
    lv_obj_t* no_rob_label = lv_label_create(game_ui.btn_no_rob);
    lv_label_set_text(no_rob_label, CN_8464);
    lv_obj_set_style_text_color(no_rob_label, lv_color_hex(COLOR_TEXT_WHITE), LV_PART_MAIN);
    lv_obj_set_style_text_font(no_rob_label, get_font(), LV_PART_MAIN);
    lv_obj_center(no_rob_label);
    lv_obj_add_event_cb(game_ui.btn_no_rob, rob_btn_event_cb, LV_EVENT_CLICKED, no_rob_label);
    lv_obj_add_flag(game_ui.btn_no_rob, LV_OBJ_FLAG_HIDDEN);
    
    // 难度选择按钮（初始隐藏）
    int diff_btn_w = 65;
    int diff_btn_gap = 15;
    int diff_total_w = 3 * diff_btn_w + 2 * diff_btn_gap;
    int diff_start_x = (SCREEN_WIDTH - diff_total_w) / 2;

    game_ui.btn_diff_easy = lv_btn_create(parent);
    lv_obj_set_size(game_ui.btn_diff_easy, diff_btn_w, 25);
    lv_obj_align(game_ui.btn_diff_easy, LV_ALIGN_TOP_LEFT, diff_start_x, 145);
    lv_obj_set_style_bg_color(game_ui.btn_diff_easy, lv_color_hex(0x4CAF50), LV_PART_MAIN);
    lv_obj_set_style_radius(game_ui.btn_diff_easy, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(game_ui.btn_diff_easy, 4, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(game_ui.btn_diff_easy, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(game_ui.btn_diff_easy, LV_OPA_20, LV_PART_MAIN);
    lv_obj_t* easy_label = lv_label_create(game_ui.btn_diff_easy);
    lv_label_set_text(easy_label, CN_D517);
    lv_obj_set_style_text_color(easy_label, lv_color_hex(COLOR_TEXT_WHITE), LV_PART_MAIN);
    lv_obj_set_style_text_font(easy_label, get_font(), LV_PART_MAIN);
    lv_obj_center(easy_label);
    lv_obj_add_event_cb(game_ui.btn_diff_easy, diff_btn_event_cb, LV_EVENT_CLICKED, easy_label);
    lv_obj_add_flag(game_ui.btn_diff_easy, LV_OBJ_FLAG_HIDDEN);
    
    game_ui.btn_diff_normal = lv_btn_create(parent);
    lv_obj_set_size(game_ui.btn_diff_normal, diff_btn_w, 25);
    lv_obj_align(game_ui.btn_diff_normal, LV_ALIGN_TOP_LEFT, diff_start_x + diff_btn_w + diff_btn_gap, 145);
    lv_obj_set_style_bg_color(game_ui.btn_diff_normal, lv_color_hex(COLOR_BTN_ORANGE), LV_PART_MAIN);
    lv_obj_set_style_radius(game_ui.btn_diff_normal, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(game_ui.btn_diff_normal, 4, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(game_ui.btn_diff_normal, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(game_ui.btn_diff_normal, LV_OPA_20, LV_PART_MAIN);
    lv_obj_t* normal_label = lv_label_create(game_ui.btn_diff_normal);
    lv_label_set_text(normal_label, CN_3524);
    lv_obj_set_style_text_color(normal_label, lv_color_hex(COLOR_TEXT_WHITE), LV_PART_MAIN);
    lv_obj_set_style_text_font(normal_label, get_font(), LV_PART_MAIN);
    lv_obj_center(normal_label);
    lv_obj_add_event_cb(game_ui.btn_diff_normal, diff_btn_event_cb, LV_EVENT_CLICKED, normal_label);
    lv_obj_add_flag(game_ui.btn_diff_normal, LV_OBJ_FLAG_HIDDEN);
    
    game_ui.btn_diff_hard = lv_btn_create(parent);
    lv_obj_set_size(game_ui.btn_diff_hard, diff_btn_w, 25);
    lv_obj_align(game_ui.btn_diff_hard, LV_ALIGN_TOP_LEFT, diff_start_x + 2 * (diff_btn_w + diff_btn_gap), 145);
    lv_obj_set_style_bg_color(game_ui.btn_diff_hard, lv_color_hex(COLOR_BTN_RED), LV_PART_MAIN);
    lv_obj_set_style_radius(game_ui.btn_diff_hard, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(game_ui.btn_diff_hard, 4, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(game_ui.btn_diff_hard, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(game_ui.btn_diff_hard, LV_OPA_20, LV_PART_MAIN);
    lv_obj_t* hard_label = lv_label_create(game_ui.btn_diff_hard);
    lv_label_set_text(hard_label, CN_D6F3);
    lv_obj_set_style_text_color(hard_label, lv_color_hex(COLOR_TEXT_WHITE), LV_PART_MAIN);
    lv_obj_set_style_text_font(hard_label, get_font(), LV_PART_MAIN);
    lv_obj_center(hard_label);
    lv_obj_add_event_cb(game_ui.btn_diff_hard, diff_btn_event_cb, LV_EVENT_CLICKED, hard_label);
    lv_obj_add_flag(game_ui.btn_diff_hard, LV_OBJ_FLAG_HIDDEN);
}

void game_ui_init(void) {
    game_ui.scr = lv_screen_active();

    /* 用本地样式直接设置屏幕背景，优先级高于默认主题 */
    lv_obj_set_style_bg_color(game_ui.scr, lv_color_hex(0x0d5e1e), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(game_ui.scr, LV_OPA_COVER, LV_PART_MAIN);

    /* 地主标签（在状态文字下方，初始隐藏） */
    game_ui.landlord_label = lv_label_create(game_ui.scr);
    lv_obj_set_style_text_color(game_ui.landlord_label,
                                 lv_color_hex(0xffd700), 0);
    lv_obj_set_style_text_font(game_ui.landlord_label, get_font(), 0);
    lv_obj_align(game_ui.landlord_label, LV_ALIGN_TOP_MID, 0, 22);
    lv_obj_add_flag(game_ui.landlord_label, LV_OBJ_FLAG_HIDDEN);

    create_ai_panel(game_ui.scr, &game_ui.ai_left_panel, &game_ui.ai_left_name,
                   &game_ui.ai_left_count, LV_ALIGN_TOP_LEFT, 5);
    create_ai_panel(game_ui.scr, &game_ui.ai_right_panel, &game_ui.ai_right_name,
                   &game_ui.ai_right_count, LV_ALIGN_TOP_RIGHT, -5);

    create_play_area(game_ui.scr, &game_ui.play_area_left, 5, 95);
    create_play_area(game_ui.scr, &game_ui.play_area_center, 105, 110);
    create_play_area(game_ui.scr, &game_ui.play_area_right, 220, 95);
    game_ui.center_area = game_ui.play_area_center;

    create_player_hand_area(game_ui.scr);
    create_status_label(game_ui.scr);
    create_control_panel(game_ui.scr);

    touch_handler_init(game_ui.player_hand, game_ui.control_panel);
}

void game_ui_update_ai(int ai_index, const char* name, int card_count) {
    lv_obj_t* name_label = NULL;
    lv_obj_t* count_label = NULL;
    char count_str[8];

    if (ai_index == 0) {
        name_label = game_ui.ai_left_name;
        count_label = game_ui.ai_left_count;
    } else if (ai_index == 1) {
        name_label = game_ui.ai_right_name;
        count_label = game_ui.ai_right_count;
    } else {
        return;
    }

    if (name_label) lv_label_set_text(name_label, name);
    snprintf(count_str, sizeof(count_str), "%d", card_count);
    if (count_label) lv_label_set_text(count_label, count_str);
}

void game_ui_update_player_hand(CardData* cards, int count) {
    if (game_ui.player_hand && lv_obj_is_valid(game_ui.player_hand)) {
        lv_obj_clean(game_ui.player_hand);
    }

    int area_w = SCREEN_WIDTH - 16;  // 手牌区域宽度（减去内边距）
    int card_w = CARD_WIDTH + 2;    // 每张牌的宽度（包含间距）
    
    // 如果牌太多，缩小间距
    if (count * card_w > area_w) {
        card_w = area_w / count;
        if (card_w < CARD_WIDTH / 2) card_w = CARD_WIDTH / 2;
    }
    
    int total_w = count * card_w;
    int start_x = (area_w - total_w) / 2 + 4;  // 居中显示
    if (start_x < 2) start_x = 2;

    for (int i = 0; i < count && i < 20; i++) {
        card_render(game_ui.player_hand, &cards[i], start_x + i * card_w, 3);
    }

    touch_handler_update_hand(cards, count);
}

// 出牌动画静态状态
static lv_obj_t* s_play_anim_cards[20];
static CardData s_play_anim_played[20];
static int s_play_anim_count = 0;
static bool s_play_anim_continue_ai = false;

// 计算对象在屏幕上的绝对坐标
static void get_obj_abs_pos(lv_obj_t* obj, lv_coord_t* x, lv_coord_t* y) {
    *x = 0;
    *y = 0;
    while (obj) {
        *x += lv_obj_get_x(obj);
        *y += lv_obj_get_y(obj);
        obj = lv_obj_get_parent(obj);
    }
}

// 出牌动画完成回调：删除临时牌，显示最终出牌，继续 AI 回合
static void play_anim_ready_cb(lv_anim_t * a) {
    (void)a;

    if (s_play_anim_count <= 0) return;

    // 删除临时飞行牌
    for (int i = 0; i < s_play_anim_count; i++) {
        if (s_play_anim_cards[i] && lv_obj_is_valid(s_play_anim_cards[i])) {
            lv_obj_del(s_play_anim_cards[i]);
        }
        s_play_anim_cards[i] = NULL;
    }

    // 在出牌区显示最终出的牌
    game_ui_show_play(2, s_play_anim_played, s_play_anim_count);

    // 如果需要，继续 AI 回合
    if (s_play_anim_continue_ai) {
        s_play_anim_continue_ai = false;
        game_logic_auto_run_ai();
    }

    s_play_anim_count = 0;
}

// 玩家出牌飞入动画
void game_ui_animate_player_play(CardData* played_cards, int played_count, int* selected_indices, int selected_count, bool continue_ai) {
    if (played_count <= 0 || played_count > 20 || selected_count <= 0 || selected_count > 20) {
        // 参数异常，回退到直接显示
        game_ui_clear_play_area();
        game_ui_show_play(2, played_cards, played_count);
        if (continue_ai) game_logic_auto_run_ai();
        return;
    }

    // 保存出的牌数据和后续标志
    memcpy(s_play_anim_played, played_cards, sizeof(CardData) * played_count);
    s_play_anim_count = played_count;
    s_play_anim_continue_ai = continue_ai;

    // 记录选中牌对象当前的屏幕坐标
    lv_coord_t start_x[20] = {0};
    lv_coord_t start_y[20] = {0};
    int valid_count = 0;

    for (int i = 0; i < selected_count && i < 20; i++) {
        int idx = selected_indices[i];
        if (idx < 0 || idx >= (int)lv_obj_get_child_count(game_ui.player_hand)) continue;

        lv_obj_t* card_obj = lv_obj_get_child(game_ui.player_hand, idx);
        if (!card_obj || !lv_obj_is_valid(card_obj)) continue;

        get_obj_abs_pos(card_obj, &start_x[valid_count], &start_y[valid_count]);
        valid_count++;
    }

    if (valid_count == 0) {
        // 没有取到有效起始位置，直接显示
        game_ui_clear_play_area();
        game_ui_show_play(2, s_play_anim_played, s_play_anim_count);
        if (s_play_anim_continue_ai) game_logic_auto_run_ai();
        s_play_anim_count = 0;
        return;
    }

    // 更新手牌显示（移除已出的牌）
    CardData hand[20];
    int hand_count = 0;
    doudizhu_get_player_hand(0, hand, &hand_count);
    game_ui_update_player_hand(hand, hand_count);

    // 清空出牌区
    game_ui_clear_play_area();

    // 计算目标出牌区在屏幕上的绝对位置
    lv_obj_t* target_area = game_ui.play_area_center;
    lv_coord_t area_abs_x = 0, area_abs_y = 0;
    get_obj_abs_pos(target_area, &area_abs_x, &area_abs_y);

    int area_w = lv_obj_get_width(target_area) - 8;
    int card_w = CARD_WIDTH + 1;
    int max_cards = area_w / card_w;
    if (max_cards < 1) max_cards = 1;
    if (played_count > max_cards) {
        card_w = area_w / played_count;
        if (card_w < CARD_WIDTH / 2) card_w = CARD_WIDTH / 2;
        max_cards = played_count;
    }
    int total_w = played_count * card_w;
    int start_tx = (area_w - total_w) / 2 + 2;
    if (start_tx < 2) start_tx = 2;

    lv_obj_t* top = lv_layer_top();

    // 在顶层创建临时牌，从手牌位置飞到出牌区
    for (int i = 0; i < played_count && i < valid_count; i++) {
        s_play_anim_cards[i] = NULL;

        lv_obj_t* temp = lv_obj_create(top);
        if (!temp) continue;

        lv_obj_set_size(temp, CARD_WIDTH, CARD_HEIGHT);
        lv_obj_set_pos(temp, start_x[i], start_y[i]);
        lv_obj_set_style_bg_color(temp, lv_color_white(), 0);
        lv_obj_set_style_bg_opa(temp, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(temp, 1, 0);
        lv_obj_set_style_border_color(temp, lv_color_make(0x33, 0x33, 0x33), 0);
        lv_obj_set_style_radius(temp, 2, 0);
        lv_obj_set_style_pad_all(temp, 1, 0);
        lv_obj_clear_flag(temp, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

        render_card_content(temp, &played_cards[i]);

        s_play_anim_cards[i] = temp;

        lv_coord_t target_x = area_abs_x + start_tx + i * card_w;
        lv_coord_t target_y = area_abs_y + 3;

        // X 轴飞入
        lv_anim_t a_x;
        lv_anim_init(&a_x);
        lv_anim_set_var(&a_x, temp);
        lv_anim_set_values(&a_x, start_x[i], target_x);
        lv_anim_set_time(&a_x, 250);
        lv_anim_set_path_cb(&a_x, lv_anim_path_ease_in_out);
        lv_anim_set_exec_cb(&a_x, (lv_anim_exec_xcb_t)lv_obj_set_x);
        lv_anim_start(&a_x);

        // Y 轴飞入（最后一张设置完成回调）
        lv_anim_t a_y;
        lv_anim_init(&a_y);
        lv_anim_set_var(&a_y, temp);
        lv_anim_set_values(&a_y, start_y[i], target_y);
        lv_anim_set_time(&a_y, 250);
        lv_anim_set_path_cb(&a_y, lv_anim_path_ease_in_out);
        lv_anim_set_exec_cb(&a_y, (lv_anim_exec_xcb_t)lv_obj_set_y);
        if (i == played_count - 1 || i == valid_count - 1) {
            lv_anim_set_ready_cb(&a_y, play_anim_ready_cb);
        }
        lv_anim_start(&a_y);

        // 缩放：飞入时轻微缩小
        lv_anim_t a_zoom;
        lv_anim_init(&a_zoom);
        lv_anim_set_var(&a_zoom, temp);
        lv_anim_set_values(&a_zoom, 256, 240);  // 100% -> 94%
        lv_anim_set_time(&a_zoom, 250);
        lv_anim_set_path_cb(&a_zoom, lv_anim_path_ease_in);
        lv_anim_set_exec_cb(&a_zoom, (lv_anim_exec_xcb_t)lv_obj_set_style_transform_zoom);
        lv_anim_start(&a_zoom);
    }
}

void game_ui_show_play(int player_idx, CardData* cards, int count) {
    if (count <= 0 || cards == NULL) return;

    lv_obj_t* target_area = NULL;
    switch (player_idx) {
        case 0: target_area = game_ui.play_area_left; break;
        case 1: target_area = game_ui.play_area_right; break;
        case 2: target_area = game_ui.play_area_center; break;
        default: return;
    }

    if (!target_area || !lv_obj_is_valid(target_area)) return;

    lv_obj_clean(target_area);

    int area_w = lv_obj_get_width(target_area) - 8;
    int card_w = CARD_WIDTH + 1;
    int max_cards = area_w / card_w;
    if (max_cards < 1) max_cards = 1;

    // 如果牌太多，缩小间距
    if (count > max_cards) {
        card_w = area_w / count;
        if (card_w < CARD_WIDTH / 2) card_w = CARD_WIDTH / 2;
        max_cards = count;
    }

    int total_w = count * card_w;
    int start_x = (area_w - total_w) / 2 + 2;
    if (start_x < 2) start_x = 2;

    for (int i = 0; i < count && i < max_cards; i++) {
        card_render(target_area, &cards[i], start_x + i * card_w, 3);
    }
}

void game_ui_show_status(const char* message) {
    if (game_ui.status_label && lv_obj_is_valid(game_ui.status_label)) {
        lv_label_set_text(game_ui.status_label, message);
    }
}

void game_ui_clear_play_area(void) {
    if (game_ui.play_area_left && lv_obj_is_valid(game_ui.play_area_left) && 
        lv_obj_get_child_count(game_ui.play_area_left) > 0) {
        lv_obj_clean(game_ui.play_area_left);
    }
    if (game_ui.play_area_center && lv_obj_is_valid(game_ui.play_area_center) && 
        lv_obj_get_child_count(game_ui.play_area_center) > 0) {
        lv_obj_clean(game_ui.play_area_center);
    }
    if (game_ui.play_area_right && lv_obj_is_valid(game_ui.play_area_right) && 
        lv_obj_get_child_count(game_ui.play_area_right) > 0) {
        lv_obj_clean(game_ui.play_area_right);
    }
}

void game_ui_show_rob_buttons(bool show) {
    if (show) {
        // 清除出牌区域，避免遮挡按钮
        game_ui_clear_play_area();
        
        lv_obj_clear_flag(game_ui.btn_rob, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(game_ui.btn_no_rob, LV_OBJ_FLAG_HIDDEN);
        // 把按钮提到最前面
        lv_obj_move_foreground(game_ui.btn_rob);
        lv_obj_move_foreground(game_ui.btn_no_rob);
    } else {
        lv_obj_add_flag(game_ui.btn_rob, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(game_ui.btn_no_rob, LV_OBJ_FLAG_HIDDEN);
    }
}

void game_ui_show_difficulty_buttons(bool show) {
    if (show) {
        lv_obj_clear_flag(game_ui.btn_diff_easy, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(game_ui.btn_diff_normal, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(game_ui.btn_diff_hard, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(game_ui.btn_diff_easy, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(game_ui.btn_diff_normal, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(game_ui.btn_diff_hard, LV_OBJ_FLAG_HIDDEN);
    }
}

void game_ui_show_landlord_cards(CardData* cards, int count) {
    // 在中央出牌区显示底牌
    if (game_ui.play_area_center && lv_obj_is_valid(game_ui.play_area_center)) {
        lv_obj_clean(game_ui.play_area_center);
    }
    
    int area_w = lv_obj_get_width(game_ui.play_area_center) - 8;
    int card_w = CARD_WIDTH + 4;
    int total_w = count * card_w;
    int start_x = (area_w - total_w) / 2 + 2;
    if (start_x < 2) start_x = 2;
    
    for (int i = 0; i < count && i < 3; i++) {
        card_render(game_ui.play_area_center, &cards[i], start_x + i * card_w, 5);
    }
}

void game_ui_show_pass(int player_idx) {
    lv_obj_t* target_area = NULL;
    switch (player_idx) {
        case 0: target_area = game_ui.play_area_left; break;
        case 1: target_area = game_ui.play_area_right; break;
        case 2: target_area = game_ui.play_area_center; break;
        default: return;
    }
    
    if (!target_area || !lv_obj_is_valid(target_area)) return;
    
    // 清除该区域
    lv_obj_clean(target_area);
    
    // 显示CN_7244文字（灰色）
    lv_obj_t* pass_label = lv_label_create(target_area);
    if (pass_label) {
        lv_label_set_text(pass_label, CN_7244);
        lv_obj_set_style_text_color(pass_label, lv_color_hex(COLOR_TEXT_GRAY), LV_PART_MAIN);
        lv_obj_set_style_text_font(pass_label, get_font(), LV_PART_MAIN);
        lv_obj_center(pass_label);
        
        // 添加淡入效果
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, pass_label);
        lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
        lv_anim_set_time(&a, 150);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
        lv_anim_start(&a);
    }
}

void game_ui_show_invalid_play_reason(int* indices, int count) {
    // 获取选中的牌
    CardData cards[20];
    int card_count = 0;
    touch_handler_get_selected_cards(cards, &card_count);
    
    if (card_count == 0) {
        game_ui_show_status(CN_123B);
        return;
    }
    
    // 简单判断原因
    if (card_count == 1) {
        // 单张
        game_ui_show_status(CN_57AD);
    } else if (card_count == 2) {
        // 对子
        if (cards[0].point != cards[1].point) {
            game_ui_show_status(CN_E49E);
        } else {
            game_ui_show_status(CN_53CE);
        }
    } else if (card_count == 3) {
        // 三张
        if (cards[0].point == cards[1].point && cards[1].point == cards[2].point) {
            game_ui_show_status(CN_4D4E);
        } else {
            game_ui_show_status(CN_77A9);
        }
    } else {
        game_ui_show_status(CN_123B);
    }
}

void game_ui_show_landlord_label(int player_idx, const char* name) {
    if (!game_ui.landlord_label) return;
    if (player_idx < 0) {
        lv_obj_add_flag(game_ui.landlord_label, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    if (player_idx == 0) {
        lv_label_set_text_fmt(game_ui.landlord_label, "\xE4\xBD\xA0\xE6\x98\xAF\xE5\x9C\xB0\xE4\xB8\xBB"); /* 你是地主 */
    } else {
        lv_label_set_text_fmt(game_ui.landlord_label, "%s \xE6\x98\xAF\xE5\x9C\xB0\xE4\xB8\xBB", name); /* name 是地主 */
    }
    lv_obj_clear_flag(game_ui.landlord_label, LV_OBJ_FLAG_HIDDEN);
    anim_pulse(game_ui.landlord_label);
}

lv_obj_t* game_ui_get_player_hand(void) {
    return game_ui.player_hand;
}

lv_obj_t* game_ui_get_ai_panel(int ai_idx) {
    if (ai_idx == 0) return game_ui.ai_left_panel;
    if (ai_idx == 1) return game_ui.ai_right_panel;
    return NULL;
}

void game_ui_animate_landlord_cards_to_hand(CardData* landlord_cards, int landlord_count, CardData* player_hand, int hand_count) {
    // 先在中央区域显示底牌
    game_ui_clear_play_area();
    game_ui_show_landlord_cards(landlord_cards, landlord_count);
    
    // 延迟一下，让玩家看到底牌
    // 然后更新手牌显示（包含新增的底牌）
    game_ui_update_player_hand(player_hand, hand_count);
}
