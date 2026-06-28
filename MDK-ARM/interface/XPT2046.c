#include "XPT2046.h"
#include "spi.h"
#include <math.h>
#include <stdio.h>

#define LCD_W 240
#define LCD_H 320

// XPT2046 命令定义 (12-bit, diff mode)
#define XPT2046_CMD_X   0xD0
#define XPT2046_CMD_Y   0x90

// 校准参数
static float xfac = 1.0f;
static float yfac = 1.0f;
static short xoff = 0;
static short yoff = 0;

// 片选引脚定义
#define T_CS_PORT   T_CS_GPIO_Port
#define T_CS_PIN    T_CS_Pin

static void XPT2046_CS_Low(void) {
    HAL_GPIO_WritePin(T_CS_PORT, T_CS_PIN, GPIO_PIN_RESET);
}

static void XPT2046_CS_High(void) {
    HAL_GPIO_WritePin(T_CS_PORT, T_CS_PIN, GPIO_PIN_SET);
}

// 发送命令并读取 12 位数据
static uint16_t XPT2046_ReadData(uint8_t cmd) {
    uint8_t tx_buf[3] = {cmd, 0x00, 0x00};
    uint8_t rx_buf[3] = {0};
    
    XPT2046_CS_Low();
    HAL_SPI_TransmitReceive(&hspi2, tx_buf, rx_buf, 3, 100);
    XPT2046_CS_High();
    
    uint16_t data = ((uint16_t)rx_buf[1] << 4) | (rx_buf[2] >> 4);
    return data;
}

// 读取原始坐标（带滤波）
static uint16_t XPT2046_ReadRaw(uint8_t cmd) {
    #define READ_TIMES 5
    #define LOST_VAL 1
    
    uint16_t buf[READ_TIMES];
    uint16_t sum = 0;
    uint16_t temp;
    
    for (uint8_t i = 0; i < READ_TIMES; i++) {
        buf[i] = XPT2046_ReadData(cmd);
    }
    
    for (uint8_t i = 0; i < READ_TIMES - 1; i++) {
        for (uint8_t j = i + 1; j < READ_TIMES; j++) {
            if (buf[i] > buf[j]) {
                temp = buf[i];
                buf[i] = buf[j];
                buf[j] = temp;
            }
        }
    }
    
    sum = 0;
    for (uint8_t i = LOST_VAL; i < READ_TIMES - LOST_VAL; i++) {
        sum += buf[i];
    }
    temp = sum / (READ_TIMES - 2 * LOST_VAL);
    
    return temp;
}

void XPT2046_Init(void) {
    XPT2046_CS_High();

    // SPI=1.3125MHz 实测四角原始值
    //    UL(243,1872) -> 屏幕(20,20)
    //    UR(1722,1850) -> 屏幕(220,20)
    //    LL(245,333)   -> 屏幕(20,300)
    //    LR(1745,245)  -> 屏幕(220,300)
    //
    // X用左上+右上(同行)，Y用左上+左下(同列)
    // xfac = (screen_w-40) / (raw_right - raw_left)
    // xoff = (screen_w - xfac * (raw_right+raw_left)) / 2

    xfac = (float)(LCD_W - 40) / (1722 - 243);
    xoff = (LCD_W - xfac * (1722 + 243)) / 2;

    yfac = (float)(LCD_H - 40) / (333 - 1872);
    yoff = (LCD_H - yfac * (333 + 1872)) / 2;

    printf("XPT2046 Cal: xfac=%.4f, xoff=%d, yfac=%.4f, yoff=%d\r\n",
           (double)xfac, xoff, (double)yfac, yoff);
}

void XPT2046_IRQ_Callback(void) {
}

bool XPT2046_IsTouched(void) {
    if (HAL_GPIO_ReadPin(CTP_INT_GPIO_Port, CTP_INT_Pin) != GPIO_PIN_RESET) {
        return false;
    }

    uint16_t raw_x = XPT2046_ReadData(XPT2046_CMD_X);
    uint16_t raw_y = XPT2046_ReadData(XPT2046_CMD_Y);

    return (raw_x > 100 && raw_x < 2000 && raw_y > 100 && raw_y < 2000);
}

void XPT2046_SetCalibration(float x_fac, float y_fac, short x_off, short y_off) {
    xfac = x_fac;
    yfac = y_fac;
    xoff = x_off;
    yoff = y_off;
}

void XPT2046_Read(uint16_t *x, uint16_t *y) {
    uint16_t raw_x = XPT2046_ReadRaw(XPT2046_CMD_X);
    uint16_t raw_y = XPT2046_ReadRaw(XPT2046_CMD_Y);
    
    printf("XPT2046 Raw: X=%d, Y=%d\r\n", raw_x, raw_y);
    
    int16_t mapped_x = (int16_t)(xfac * raw_x + xoff);
    int16_t mapped_y = (int16_t)(yfac * raw_y + yoff);
    
    if (mapped_x < 0) mapped_x = 0;
    if (mapped_x >= LCD_W) mapped_x = LCD_W - 1;
    if (mapped_y < 0) mapped_y = 0;
    if (mapped_y >= LCD_H) mapped_y = LCD_H - 1;
    
    *x = (uint16_t)mapped_x;
    *y = (uint16_t)mapped_y;
    
    printf("XPT2046 Mapped: X=%d, Y=%d\r\n", *x, *y);
}
