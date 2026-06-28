#include "card_render.h"
#include "suit_image.h"
#include <stdio.h>

void render_card_content(lv_obj_t* card_obj, CardData* card) {
    if (!card_obj || !card) return;
    
    // 检查对象是否有效
    if (!lv_obj_is_valid(card_obj)) return;
    
    // 获取花色颜色
    lv_color_t color = get_suit_color(card->suit);
    
    // 获取花色图像
    const lv_img_dsc_t* img = get_suit_image(card->suit);
    
    // 判断是否是王
    bool is_joker = (card->suit == SUIT_JOKER_SMALL || card->suit == SUIT_JOKER_BIG);
    
    if (is_joker) {
        // 王：用文字显示
        lv_obj_t* label = lv_label_create(card_obj);
        if (label && lv_obj_is_valid(label)) {
            lv_label_set_text(label, card->suit == SUIT_JOKER_SMALL ? "Jo" : "JO");
            lv_obj_set_style_text_color(label, color, 0);
            lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
            lv_obj_center(label);
        }
    } else {
        // 普通牌：花色在左上角，点数在中央
        if (img) {
            lv_obj_t* suit_img = lv_img_create(card_obj);
            if (suit_img && lv_obj_is_valid(suit_img)) {
                lv_img_set_src(suit_img, img);
                lv_obj_set_style_img_recolor(suit_img, color, 0);
                lv_obj_set_style_img_recolor_opa(suit_img, LV_OPA_COVER, 0);
                lv_obj_align(suit_img, LV_ALIGN_TOP_LEFT, 1, 1);
            }
        }
        
        // 创建点数标签
        const char* point_name = get_point_name(card->point);
        lv_obj_t* point_label = lv_label_create(card_obj);
        if (point_label && lv_obj_is_valid(point_label)) {
            lv_label_set_text(point_label, point_name);
            lv_obj_set_style_text_color(point_label, color, 0);
            lv_obj_set_style_text_font(point_label, &lv_font_montserrat_10, 0);
            lv_obj_align(point_label, LV_ALIGN_CENTER, 0, 3);
        }
    }
}

static lv_obj_t* create_card_base(lv_obj_t* parent, int x, int y, bool selected) {
    if (!parent || !lv_obj_is_valid(parent)) return NULL;
    
    lv_obj_t* card = lv_obj_create(parent);
    if (!card || !lv_obj_is_valid(card)) return NULL;
    
    lv_obj_set_size(card, CARD_WIDTH, CARD_HEIGHT);
    lv_obj_set_pos(card, x, selected ? y - 4 : y);
    
    // 牌面样式 - 白色背景
    lv_obj_set_style_bg_color(card, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    
    // 边框样式
    lv_obj_set_style_border_width(card, selected ? 2 : 1, 0);
    lv_obj_set_style_border_color(card, selected ? lv_color_make(0x00, 0x00, 0xFF) : lv_color_make(0x33, 0x33, 0x33), 0);
    
    // 圆角和内边距
    lv_obj_set_style_radius(card, 2, 0);
    lv_obj_set_style_pad_all(card, 1, 0);
    
    // 阴影效果
    if (selected) {
        lv_obj_set_style_shadow_width(card, 4, 0);
        lv_obj_set_style_shadow_color(card, lv_color_make(0x00, 0x00, 0xFF), 0);
        lv_obj_set_style_shadow_opa(card, LV_OPA_30, 0);
    }
    
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}

void card_render(lv_obj_t* parent, CardData* card, int x, int y) {
    if (!card || !parent) return;
    lv_obj_t* card_obj = create_card_base(parent, x, y, false);
    if (card_obj) {
        render_card_content(card_obj, card);
    }
}

void card_render_selected(lv_obj_t* parent, CardData* card, int x, int y) {
    if (!card || !parent) return;
    lv_obj_t* card_obj = create_card_base(parent, x, y, true);
    if (card_obj) {
        render_card_content(card_obj, card);
    }
}

void card_clear(lv_obj_t* parent, int x, int y) {
    if (!parent || !lv_obj_is_valid(parent)) return;
    
    lv_obj_t* card = lv_obj_create(parent);
    if (!card) return;
    
    lv_obj_set_size(card, CARD_WIDTH, CARD_HEIGHT);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_style_bg_color(card, lv_color_make(0x00, 0x80, 0x00), 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
}