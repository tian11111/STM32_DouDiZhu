#ifndef GRAPHICS_DATA_H
#define GRAPHICS_DATA_H

#include "lvgl.h"

// 花色符号
typedef enum {
    SUIT_CLUBS = 0,    // 梅花
    SUIT_DIAMONDS,     // 方片
    SUIT_HEARTS,       // 红桃
    SUIT_SPADES,       // 黑桃
    SUIT_JOKER_SMALL,  // 小王
    SUIT_JOKER_BIG     // 大王
} SuitType;

// 点数
typedef enum {
    POINT_3 = 0,
    POINT_4, POINT_5, POINT_6, POINT_7,
    POINT_8, POINT_9, POINT_10,
    POINT_J, POINT_Q, POINT_K, POINT_A, POINT_2,
    POINT_SMALL_JOKER, POINT_BIG_JOKER
} PointType;

// 牌面数据结构
typedef struct {
    SuitType suit;
    PointType point;
    char display_name[8];
} CardData;

const char* get_suit_symbol(SuitType suit);
const char* get_point_name(PointType point);
lv_color_t get_suit_color(SuitType suit);

#endif