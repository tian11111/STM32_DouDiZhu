/**
 * @file lv_port_lcd_stm32_template.c
 *
 * Example implementation of the LVGL LCD display drivers on the STM32 platform
 */

/*Copy this file as "lv_port_disp.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
/* Include STM32Cube files here, e.g.:
#include "stm32f7xx_hal.h"
*/

#include "lv_port_lcd_stm32.h"
#include "./src/drivers/display/ili9341/lv_ili9341.h"
#include "./src/drivers/display/lcd/lv_lcd_generic_mipi.h"
#include "spi.h"
/*********************
 *      DEFINES
 *********************/
#define MY_DISP_HOR_RES 320
#define MY_DISP_VER_RES 240

#define LCD_RESET_GPIO_Port ST_RST_GPIO_Port
#define LCD_RESET_Pin ST_RST_Pin

#define BUS_SPI1_POLL_TIMEOUT 0x1000U

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lcd_color_transfer_ready_cb(SPI_HandleTypeDef *hspi);
static int32_t lcd_io_init(void);
static void lcd_send_cmd(lv_display_t *disp, const uint8_t *cmd, size_t cmd_size, const uint8_t *param,
                         size_t param_size);
static void lcd_send_color(lv_display_t *disp, const uint8_t *cmd, size_t cmd_size, uint8_t *param,
                           size_t param_size);

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_display_t *lcd_disp;
static volatile int lcd_bus_busy = 0;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_disp_init(void)
{
    /* Initialize LCD I/O */
    if (lcd_io_init() != 0)
        return;

    /* Create the LVGL display object and the ILI9341 LCD display driver */
    lcd_disp = lv_ili9341_create(MY_DISP_HOR_RES, MY_DISP_VER_RES, LV_LCD_FLAG_BGR, lcd_send_cmd, lcd_send_color);
    lv_lcd_generic_mipi_set_address_mode(lcd_disp, true, false, true, true);
    lv_lcd_generic_mipi_set_invert(lcd_disp, false);

    /* Example: two dynamically allocated buffers for partial rendering */
    // uint8_t *buf1 = NULL;
    // uint8_t *buf2 = NULL;

    // uint32_t buf_size = MY_DISP_HOR_RES * MY_DISP_VER_RES / 10 * lv_color_format_get_size(lv_display_get_color_format(lcd_disp));

    // buf1 = lv_malloc(buf_size);
    // if (buf1 == NULL)
    // {
    //     LV_LOG_ERROR("display draw buffer malloc failed");
    //     return;
    // }

    // buf2 = lv_malloc(buf_size);
    // if (buf2 == NULL)
    // {
    //     LV_LOG_ERROR("display buffer malloc failed");
    //     lv_free(buf1);
    //     return;
    // }
#define BUF_SIZE (MY_DISP_HOR_RES * 40)  /* 40 lines = 12,800 bytes for RGB565 */
    static uint8_t buf1[BUF_SIZE];
    static uint8_t buf2[BUF_SIZE];

    lv_display_set_buffers(lcd_disp, buf1, buf2, BUF_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/* Callback is called when background transfer finished */
static void lcd_color_transfer_ready_cb(SPI_HandleTypeDef *hspi)
{
    /* CS high */
    HAL_GPIO_WritePin(ST_CS_GPIO_Port, ST_CS_Pin, GPIO_PIN_SET);
    lcd_bus_busy = 0;
    lv_display_flush_ready(lcd_disp);
}

/* Initialize LCD I/O bus, reset LCD */
static int32_t lcd_io_init(void)
{
    /* Register SPI Tx Complete Callback */
    HAL_SPI_RegisterCallback(&hspi1, HAL_SPI_TX_COMPLETE_CB_ID, lcd_color_transfer_ready_cb);

    // �򿪱���
    HAL_GPIO_WritePin(ST_BLK_GPIO_Port, ST_BLK_Pin, GPIO_PIN_SET);
    HAL_Delay(100);
    /* reset LCD */
    HAL_GPIO_WritePin(LCD_RESET_GPIO_Port, LCD_RESET_Pin, GPIO_PIN_RESET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(LCD_RESET_GPIO_Port, LCD_RESET_Pin, GPIO_PIN_SET);
    HAL_Delay(100);

    HAL_GPIO_WritePin(ST_CS_GPIO_Port, ST_CS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(ST_DC_GPIO_Port, ST_DC_Pin, GPIO_PIN_SET);

    return HAL_OK;
}

/* Platform-specific implementation of the LCD send command function. In general this should use polling transfer. */
static void lcd_send_cmd(lv_display_t *disp, const uint8_t *cmd, size_t cmd_size, const uint8_t *param,
                         size_t param_size)
{
    LV_UNUSED(disp);
    while (lcd_bus_busy)
        ; /* wait until previous transfer is finished */
    /* Set the SPI in 8-bit mode */
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    HAL_SPI_Init(&hspi1);
    /* DCX low (command) */
    HAL_GPIO_WritePin(ST_DC_GPIO_Port, ST_DC_Pin, GPIO_PIN_RESET);
    /* CS low */
    HAL_GPIO_WritePin(ST_CS_GPIO_Port, ST_CS_Pin, GPIO_PIN_RESET);
    /* send command */
    if (HAL_SPI_Transmit(&hspi1, cmd, cmd_size, BUS_SPI1_POLL_TIMEOUT) == HAL_OK)
    {
        /* DCX high (data) */
        HAL_GPIO_WritePin(ST_DC_GPIO_Port, ST_DC_Pin, GPIO_PIN_SET);
        /* for short data blocks we use polling transfer */
        HAL_SPI_Transmit(&hspi1, (uint8_t *)param, (uint16_t)param_size, BUS_SPI1_POLL_TIMEOUT);
        /* CS high */
        HAL_GPIO_WritePin(ST_CS_GPIO_Port, ST_CS_Pin, GPIO_PIN_SET);
    }
}

/* Platform-specific implementation of the LCD send color function. For better performance this should use DMA transfer.
 * In case of a DMA transfer a callback must be installed to notify LVGL about the end of the transfer.
 */
static void lcd_send_color(lv_display_t *disp, const uint8_t *cmd, size_t cmd_size, uint8_t *param,
                           size_t param_size)
{
    LV_UNUSED(disp);
    while (lcd_bus_busy)
        ; /* wait until previous transfer is finished */
    /* Set the SPI in 8-bit mode */
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    HAL_SPI_Init(&hspi1);
    /* DCX low (command) */
    HAL_GPIO_WritePin(ST_DC_GPIO_Port, ST_DC_Pin, GPIO_PIN_RESET);
    /* CS low */
    HAL_GPIO_WritePin(ST_CS_GPIO_Port, ST_CS_Pin, GPIO_PIN_RESET);
    /* send command */
    if (HAL_SPI_Transmit(&hspi1, cmd, cmd_size, BUS_SPI1_POLL_TIMEOUT) == HAL_OK)
    {
        /* DCX high (data) */
        HAL_GPIO_WritePin(ST_DC_GPIO_Port, ST_DC_Pin, GPIO_PIN_SET);
        /* for color data use DMA transfer */
        /* Set the SPI in 16-bit mode to match endianness */
        hspi1.Init.DataSize = SPI_DATASIZE_16BIT;
        HAL_SPI_Init(&hspi1);
        lcd_bus_busy = 1;
        HAL_SPI_Transmit_DMA(&hspi1, param, (uint16_t)param_size / 2);
        /* NOTE: CS will be reset in the transfer ready callback */
    }
}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
