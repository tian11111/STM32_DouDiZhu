#ifndef CARD_RENDER_H
#define CARD_RENDER_H

#include "lvgl.h"
#include "graphics_data.h"

#define CARD_WIDTH  18
#define CARD_HEIGHT 26

void card_render(lv_obj_t* parent, CardData* card, int x, int y);
void card_render_selected(lv_obj_t* parent, CardData* card, int x, int y);
void card_clear(lv_obj_t* parent, int x, int y);

// 在已有牌对象上绘制牌面内容（用于动画临时对象）
void render_card_content(lv_obj_t* card_obj, CardData* card);

#endif