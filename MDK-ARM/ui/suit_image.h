#ifndef SUIT_IMAGE_H
#define SUIT_IMAGE_H

#include "lvgl.h"

// 花色图像初始化
void suit_image_init(void);

// 获取花色图像描述符
const lv_img_dsc_t* get_suit_image(int suit);

#endif
