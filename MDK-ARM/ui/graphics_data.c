#include "graphics_data.h"

const char* get_suit_symbol(SuitType suit) {
    switch (suit) {
        case SUIT_CLUBS:    return "C";  // Club
        case SUIT_DIAMONDS: return "D";  // Diamond
        case SUIT_HEARTS:   return "H";  // Heart
        case SUIT_SPADES:   return "S";  // Spade
        case SUIT_JOKER_SMALL: return "X";  // Small Joker
        case SUIT_JOKER_BIG:   return "XX"; // Big Joker
        default: return "?";
    }
}

const char* get_point_name(PointType point) {
    switch (point) {
        case POINT_3: return "3";
        case POINT_4: return "4";
        case POINT_5: return "5";
        case POINT_6: return "6";
        case POINT_7: return "7";
        case POINT_8: return "8";
        case POINT_9: return "9";
        case POINT_10: return "10";
        case POINT_J: return "J";
        case POINT_Q: return "Q";
        case POINT_K: return "K";
        case POINT_A: return "A";
        case POINT_2: return "2";
        case POINT_SMALL_JOKER: return "Jo";
        case POINT_BIG_JOKER:   return "JO";
        default: return "?";
    }
}

lv_color_t get_suit_color(SuitType suit) {
    switch (suit) {
        case SUIT_DIAMONDS:
        case SUIT_HEARTS:
            return lv_color_make(0xFF, 0x00, 0x00);  // 红色
        case SUIT_CLUBS:
        case SUIT_SPADES:
            return lv_color_make(0x00, 0x00, 0x00);  // 黑色
        case SUIT_JOKER_SMALL:
            return lv_color_make(0x00, 0x00, 0xFF);  // 蓝色
        case SUIT_JOKER_BIG:
            return lv_color_make(0xFF, 0x00, 0x00);  // 红色
        default:
            return lv_color_make(0x00, 0x00, 0x00);
    }
}