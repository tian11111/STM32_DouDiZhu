# STM32 斗地主

基于 LVGL v9 的 STM32F407 单机斗地主游戏，支持触摸操作、AI 对战、记分。移植自 LVGL PC 模拟器版斗地主。

## 硬件

| 组件 | 型号 / 参数 |
|------|-------------|
| 主控 | STM32F407VET6（168 MHz） |
| LCD | 2.8" ILI9341 SPI 240×320，横屏模式（MADCTL=0x68） |
| 触摸 | XPT2046 电阻式触摸 |
| 颜色 | RGB565（16-bit） |
| 背光 | PWM 可调 |
| 编译 | Keil MDK v5，ARMCC V5 |

## 引脚定义

### LCD（SPI1 + GPIO）

屏幕引脚	STM32 引脚	信号
1. T_IRQ	PB13	触摸中断
2. T_DO	PB14	触摸 MISO
3. T_DIN	PB15	触摸 MOSI
4. T_CS	PB12	触摸片选
5. T_CLK	PB10	触摸 SPI 时钟
6. SDO	悬空	不接
7. LED	PB1	背光
8. SCK	PA5	LCD SPI 时钟
9. SDI	PA7	LCD 数据
10. DC	PA4	数据/命令
11. RESET	PA6	复位
12. CS	PB0	LCD 片选
13. GND	GND	地
14. VCC	3.3V	电源

### 调试串口

| 引脚 | 功能 |
|------|------|
| PA9 | USART1 TX（115200 bps） |
| PA10 | USART1 RX |

## 刷机

1. 用 Keil MDK 打开 `MDK-ARM/P01_lvgl_stm32.uvprojx`
2. 把 `MDK-ARM/ui/` 加入 Include Paths
3. 确保以下源文件已加入工程：
   - `doudizhu_adapter.c`
   - `game_logic.c`
   - `game_ui.c`
   - `card_render.c`
   - `touch_handler.c`
   - `game_anim.c`
   - `graphics_data.c`
   - `suit_image.c`
   - `doudizhu_font.c`
4. 编译→下载

## 玩法

### 出牌阶段

| 按钮 | 功能 |
|------|------|
| **不出** | 过牌 |
| **提示** | AI 建议出牌 |
| **出牌** | 打出选中的牌 |
| **重开** | 开始新一局 |

点击手牌选中 / 取消。已选中的牌高亮显示。

### 抢地主阶段

开局先选难度（简单 / 普通 / 困难），然后轮流叫地主。玩家可在「抢地主」和「不抢」之间选择。

### 牌型

单张、对子、三张、三带一、三带二、顺子、连对、飞机、飞机带单、飞机带对、炸弹、王炸

### 积分

| 结果 | 地主 | 农民 |
|------|------|------|
| 地主胜 | +2 | 各 −1 |
| 农民胜 | −2 | 各 +1 |

## 目录结构

```
├── Core/                  # STM32CubeMX HAL 外设驱动
├── Drivers/               # STM32F4xx HAL 库
├── lvgl/                  # LVGL v9.5.0-dev
│   ├── lv_conf.h          # LVGL 配置（16 位色，内存池 64KB）
│   └── porting/
│       ├── lv_port_lcd_stm32.c   # ILI9341 SPI+DMA 显示驱动
│       └── lv_port_indev.c       # XPT2046 触摸驱动
└── MDK-ARM/
    ├── interface/
    │   ├── XPT2046.c/h     # 触摸芯片驱动（含校准）
    └── ui/
        ├── doudizhu_adapter.c/h  # 游戏引擎核心
        ├── game_logic.c/h        # 游戏状态机
        ├── game_ui.c/h           # LVGL 界面布局
        ├── card_render.c/h       # 牌面渲染（18×26 px）
        ├── touch_handler.c/h     # 选牌 & 按钮事件
        ├── game_anim.c/h         # 动画（脉冲、淡入、抖动）
        ├── graphics_data.c/h     # 花色 & 点数定义
        ├── suit_image.c/h        # 8×8 A8 花色图标
        ├── strings_cn.h          # 中文宏（UTF-8 hex 转义）
        └── doudizhu_font.c       # 中文字体（SimHei 16px，4bpp）
```

## 常见问题

### 中文乱码 / 不显示

项目使用 `strings_cn.h` 中的 `\x` 十六进制转义序列存放所有中文字符串，**不依赖编译器源码编码**。ARMCC V5 无需额外配置即可正确编译。

### 触摸错位

触摸坐标在 `lv_port_indev.c` 中已做 XY 交换 + X 镜像处理，匹配 MADCTL=0x68 横屏模式。

### 重启后牌不变

每局开始前调用 `srand(HAL_GetTick())` 重新播种，确保每局发牌不同。

## License

MIT
