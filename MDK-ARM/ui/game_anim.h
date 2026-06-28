#ifndef GAME_ANIM_H
#define GAME_ANIM_H

#include "lvgl.h"

// 初始化动画模块
void game_anim_init(void);

// 选中牌时的上移动画
void anim_card_select(lv_obj_t* card);

// 取消选中时的下移动画
void anim_card_deselect(lv_obj_t* card);

// 出牌时的飞入动画
void anim_card_play(lv_obj_t* card, lv_coord_t target_x, lv_coord_t target_y);

// 状态提示的淡入动画
void anim_status_fade_in(lv_obj_t* label);

// 状态提示的淡出动画
void anim_status_fade_out(lv_obj_t* label);

// 抖动动画（错误提示）
void anim_shake(lv_obj_t* obj);

// 弹跳动画（抢地主成功）
void anim_bounce(lv_obj_t* obj);

// 脉冲动画（强调效果）
void anim_pulse(lv_obj_t* obj);

// 滑入动画（从右侧滑入）
void anim_slide_in_right(lv_obj_t* obj, lv_coord_t target_x);

// 滑入动画（从左侧滑入）
void anim_slide_in_left(lv_obj_t* obj, lv_coord_t target_x);

#endif
