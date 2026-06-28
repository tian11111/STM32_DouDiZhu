#include "game_anim.h"

void game_anim_init(void) {
    // LVGL动画系统会自动初始化
}

// 选中牌时的上移动画（带缩放回弹）
void anim_card_select(lv_obj_t* card) {
    if (!card) return;

    lv_coord_t start_y = lv_obj_get_y(card);
    lv_coord_t end_y = start_y - 10;

    // Y轴上移动画，带轻微回弹
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, card);
    lv_anim_set_values(&a, start_y, end_y);
    lv_anim_set_time(&a, 120);
    lv_anim_set_path_cb(&a, lv_anim_path_overshoot);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_start(&a);

    // 缩放动画：1.0 -> 1.05，让选中牌更突出
    lv_anim_t a_zoom;
    lv_anim_init(&a_zoom);
    lv_anim_set_var(&a_zoom, card);
    lv_anim_set_values(&a_zoom, 256, 269);  // 256=100%, 269≈105%
    lv_anim_set_time(&a_zoom, 120);
    lv_anim_set_path_cb(&a_zoom, lv_anim_path_ease_out);
    lv_anim_set_exec_cb(&a_zoom, (lv_anim_exec_xcb_t)lv_obj_set_style_transform_zoom);
    lv_anim_start(&a_zoom);
}

// 取消选中时的下移动画（回到原位，缩放还原）
void anim_card_deselect(lv_obj_t* card) {
    if (!card) return;

    lv_coord_t start_y = lv_obj_get_y(card);
    lv_coord_t end_y = start_y + 10;

    // Y轴下移动画
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, card);
    lv_anim_set_values(&a, start_y, end_y);
    lv_anim_set_time(&a, 100);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_start(&a);

    // 缩放还原：1.05 -> 1.0
    lv_anim_t a_zoom;
    lv_anim_init(&a_zoom);
    lv_anim_set_var(&a_zoom, card);
    lv_anim_set_values(&a_zoom, 269, 256);
    lv_anim_set_time(&a_zoom, 100);
    lv_anim_set_path_cb(&a_zoom, lv_anim_path_ease_out);
    lv_anim_set_exec_cb(&a_zoom, (lv_anim_exec_xcb_t)lv_obj_set_style_transform_zoom);
    lv_anim_start(&a_zoom);
}

// 出牌时的飞入动画（从手牌飞到出牌区，带缩放消失）
void anim_card_play(lv_obj_t* card, lv_coord_t target_x, lv_coord_t target_y) {
    if (!card) return;

    lv_coord_t start_x = lv_obj_get_x(card);
    lv_coord_t start_y = lv_obj_get_y(card);

    // X轴飞入
    lv_anim_t a_x;
    lv_anim_init(&a_x);
    lv_anim_set_var(&a_x, card);
    lv_anim_set_values(&a_x, start_x, target_x);
    lv_anim_set_time(&a_x, 200);
    lv_anim_set_path_cb(&a_x, lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&a_x, (lv_anim_exec_xcb_t)lv_obj_set_x);
    lv_anim_start(&a_x);

    // Y轴飞入
    lv_anim_t a_y;
    lv_anim_init(&a_y);
    lv_anim_set_var(&a_y, card);
    lv_anim_set_values(&a_y, start_y, target_y);
    lv_anim_set_time(&a_y, 200);
    lv_anim_set_path_cb(&a_y, lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&a_y, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_start(&a_y);

    // 缩放：出牌过程中稍微缩小，模拟放到桌面
    lv_anim_t a_zoom;
    lv_anim_init(&a_zoom);
    lv_anim_set_var(&a_zoom, card);
    lv_anim_set_values(&a_zoom, 256, 240);  // 100% -> ~94%
    lv_anim_set_time(&a_zoom, 200);
    lv_anim_set_path_cb(&a_zoom, lv_anim_path_ease_in);
    lv_anim_set_exec_cb(&a_zoom, (lv_anim_exec_xcb_t)lv_obj_set_style_transform_zoom);
    lv_anim_start(&a_zoom);
}

// 状态提示的淡入动画
void anim_status_fade_in(lv_obj_t* label) {
    if (!label) return;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, label);
    lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_time(&a, 200);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
    lv_anim_start(&a);
}

// 状态提示的淡出动画
void anim_status_fade_out(lv_obj_t* label) {
    if (!label) return;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, label);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&a, 200);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
    lv_anim_start(&a);
}

// 抖动动画（错误提示）
void anim_shake(lv_obj_t* obj) {
    if (!obj) return;

    lv_coord_t orig_x = lv_obj_get_x(obj);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, orig_x, orig_x + 5);
    lv_anim_set_time(&a, 40);
    lv_anim_set_playback_time(&a, 40);
    lv_anim_set_playback_delay(&a, 0);
    lv_anim_set_repeat_count(&a, 2);
    lv_anim_set_repeat_delay(&a, 0);
    lv_anim_set_path_cb(&a, lv_anim_path_linear);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);
    lv_anim_start(&a);
}

// 弹跳动画（抢地主成功）
void anim_bounce(lv_obj_t* obj) {
    if (!obj) return;

    lv_coord_t orig_y = lv_obj_get_y(obj);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, orig_y, orig_y - 15);
    lv_anim_set_time(&a, 150);
    lv_anim_set_playback_time(&a, 150);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_start(&a);
}

// 脉冲动画（强调效果）
void anim_pulse(lv_obj_t* obj) {
    if (!obj) return;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, 256, 270);  // 从100%到105%
    lv_anim_set_time(&a, 200);
    lv_anim_set_playback_time(&a, 200);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_transform_zoom);
    lv_anim_start(&a);
}

// 滑入动画（从右侧滑入）
void anim_slide_in_right(lv_obj_t* obj, lv_coord_t target_x) {
    if (!obj) return;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, target_x + 50, target_x);
    lv_anim_set_time(&a, 250);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);
    lv_anim_start(&a);
}

// 滑入动画（从左侧滑入）
void anim_slide_in_left(lv_obj_t* obj, lv_coord_t target_x) {
    if (!obj) return;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, target_x - 50, target_x);
    lv_anim_set_time(&a, 250);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);
    lv_anim_start(&a);
}
