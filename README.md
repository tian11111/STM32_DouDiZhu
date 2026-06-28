# 🃏 STM32_DouDiZhu

基于 LVGL v9 的 STM32 斗地主单机游戏，支持触摸操作和 AI 对战。

## 硬件平台

| 组件 | 型号 |
|------|------|
| 主控 | STM32F405 |
| 显示屏 | 2.8" ILI9341 SPI LCD (240×320) |
| 触摸 | XPT2046 电阻触摸 |
| 编译 | Keil MDK (ARMCC V5) |

## 功能特性

- 完整斗地主玩法：单张、对子、三张、顺子、连对、飞机、炸弹、王炸
- 3 级 AI 难度（简单 / 普通 / 困难）
- 抢地主流程，支持叫 / 不叫
- 触摸选牌、出牌、过牌、提示
- 游戏结算，多人计分
- 经典绿色桌面风格

## 项目结构

```
├── Core/                  # STM32CubeMX HAL 驱动
├── Drivers/               # STM32F4xx HAL 库
├── lvgl/                  # LVGL v9.5.0-dev 图形库
│   └── porting/           # 显示 & 触摸移植层
└── MDK-ARM/
    ├── interface/          # XPT2046 触摸驱动
    └── ui/                 # 斗地主游戏源码
        ├── doudizhu_adapter.c/h  # 游戏引擎（发牌、出牌、AI）
        ├── game_logic.c/h        # 游戏状态机
        ├── game_ui.c/h           # LVGL 界面
        ├── card_render.c/h       # 牌面渲染
        ├── touch_handler.c/h     # 触摸事件处理
        ├── game_anim.c/h         # 动画效果
        ├── strings_cn.h          # 中文字符串宏
        └── doudizhu_font.c       # 中文 bitmap 字体
```

## 编译

1. 用 Keil MDK 打开 `MDK-ARM/P01_lvgl_stm32.uvprojx`
2. 确保 `MDK-ARM/ui/` 已添加到 Include Paths
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
4. 编译下载

## 操作说明

| 按钮 | 功能 |
|------|------|
| 不出 | 过牌 |
| 提示 | AI 推荐出牌 |
| 出牌 | 打出选中的牌 |
| 重开 | 开始新一局 |

- 点击手牌选中 / 取消选中
- 抢地主阶段使用「抢地主」/「不抢」按钮

## 移植自

基于 [doudizhu_lvgl](https://github.com/tian11111/STM32_DouDiZhu) PC 模拟器版斗地主，适配 STM32 嵌入式平台。

## License

MIT
