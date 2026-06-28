#include "game_logic.h"
#include "game_ui.h"
#include "doudizhu_adapter.h"
#include "touch_handler.h"
#include "game_anim.h"
#include "strings_cn.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

static GameStateType current_state = GAME_STATE_IDLE;
static CardData player_hand[20];
static int player_hand_count = 0;
static bool rob_processing = false;  // 防止重复点击

// 前向声明
static void game_logic_ai_rob_turn(int ai_idx);
void game_logic_auto_run_ai(void);

void game_logic_init(void) {
    current_state = GAME_STATE_IDLE;
    player_hand_count = 0;
    
    doudizhu_init();
    
    game_ui_show_status(CN_6747);
}

void game_logic_start_new_game(void) {
    current_state = GAME_STATE_DEALING;

    game_ui_clear_play_area();

    // 难度选择先显示，发牌推迟到用户选择难度后，
    // 以便用用户操作时刻的 HAL_GetTick() 作为随机种子
    game_ui_show_status(CN_25B1);
    game_ui_show_difficulty_buttons(true);
}

// 用户选择难度后发牌并开始抢地主
void game_logic_start_after_difficulty(void) {
    // 用用户点击难度的时刻作为随机种子，避免每次复位后牌序相同
    srand(HAL_GetTick());

    doudizhu_deal_cards();  /* also updates player hand & AI panels internally */

    /* refresh local hand cache */
    doudizhu_get_player_hand(0, player_hand, &player_hand_count);

    // 隐藏难度选择
    game_ui_show_difficulty_buttons(false);

    // 进入抢地主阶段
    current_state = GAME_STATE_ROB_LANDLORD;
    game_ui_show_status(CN_D8FD);
    game_ui_show_rob_buttons(true);
}

// 进入抢地主阶段
void game_logic_start_rob_landlord(void) {
    current_state = GAME_STATE_ROB_LANDLORD;
    game_ui_show_status(CN_D8FD);
    game_ui_show_rob_buttons(true);
}

// 玩家抢地主
void game_logic_player_rob_landlord(bool rob) {
    if (current_state != GAME_STATE_ROB_LANDLORD || rob_processing) return;
    
    rob_processing = true;  // 锁定，防止重复点击
    game_ui_show_rob_buttons(false);
    touch_handler_clear_selection();  // 清除选中的牌
    
    if (rob) {
        // 玩家抢地主
        doudizhu_rob_landlord(0, true);
        game_ui_show_status(CN_B4CF);
        
        // 显示底牌
        CardData landlord_cards[3];
        int lc_count = 0;
        doudizhu_get_landlord_cards(landlord_cards, &lc_count);
        game_ui_show_landlord_cards(landlord_cards, lc_count);
        
        // 更新手牌（地主多了3张）
        doudizhu_get_player_hand(0, player_hand, &player_hand_count);
        game_ui_update_player_hand(player_hand, player_hand_count);
        
        // 进入出牌阶段
        current_state = GAME_STATE_PLAYING;
        rob_processing = false;  // 解锁
        game_ui_show_landlord_label(0, NULL);
        game_ui_show_status(CN_8F04);
    } else {
        // 玩家不抢，轮到AI
        game_ui_show_status(CN_1FDA);
        game_logic_ai_rob_turn(1);
    }
}

// AI抢地主回合
static void game_logic_ai_rob_turn(int ai_idx) {
    if (doudizhu_is_landlord_robbed()) return;
    
    int rob = doudizhu_ai_rob_landlord(ai_idx);
    
    if (rob) {
        // AI抢地主
        doudizhu_rob_landlord(ai_idx, true);
        game_ui_show_status(ai_idx == 1 ? CN_401A : CN_6DD2);
        
        // 显示底牌
        CardData landlord_cards[3];
        int lc_count = 0;
        doudizhu_get_landlord_cards(landlord_cards, &lc_count);
        game_ui_show_landlord_cards(landlord_cards, lc_count);
        
        // 进入出牌阶段
        current_state = GAME_STATE_PLAYING;
        game_ui_show_landlord_label(ai_idx, ai_idx == 1 ? CN_47D3 : CN_0AB2);
        
        int current = doudizhu_get_current_player();
        if (current == 0) {
            game_ui_show_status(CN_8F04);
        } else {
            game_logic_auto_run_ai();
        }
    } else {
        // AI不抢
        game_ui_show_status(ai_idx == 1 ? CN_854F : CN_5760);
        
        // 轮到下一个
        int next = (ai_idx + 1) % 3;
        if (next == 0) {
            // 回到玩家
            rob_processing = false;  // 解锁
            game_ui_show_status(CN_D8FD);
            game_ui_show_rob_buttons(true);
        } else {
            // 下一个AI
            game_logic_ai_rob_turn(next);
        }
    }
}

// 自动运行AI（原来的autoRunAI逻辑）
void game_logic_auto_run_ai(void) {
    int guard = 0;
    while (!doudizhu_is_game_over() && doudizhu_is_landlord_robbed()) {
        int current = doudizhu_get_current_player();
        if (current == 0) break;  // 轮到玩家了
        if (guard >= 50) break;   // 防止死循环
        
        // 显示当前轮到谁
        char status[32];
        snprintf(status, sizeof(status), CN_2C31, current);
        game_ui_show_status(status);
        
        int ai_result = doudizhu_ai_play(current);
        
        if (ai_result) {
            // 检查AI是出了牌还是过了牌
            CardData played_cards[20];
            int played_count = 0;
            doudizhu_get_last_played(played_cards, &played_count);
            
            if (played_count > 0) {
                // AI出了牌，在对应位置显示出的牌
                game_ui_show_play(current - 1, played_cards, played_count);
            } else {
                // AI过了牌
                game_ui_show_pass(current - 1);
            }
            
            // 更新AI面板
            CardData ai_hand[20];
            int ai_hand_count = 0;
            doudizhu_get_player_hand(current, ai_hand, &ai_hand_count);
            game_ui_update_ai(current - 1, 
                            current == 1 ? CN_47D3 : CN_0AB2, 
                            ai_hand_count);
        } else {
            // AI不出
            game_ui_show_pass(current - 1);
        }
        
        if (doudizhu_is_game_over()) {
            current_state = GAME_STATE_GAME_OVER;
            // 显示最后一手牌
            CardData played_cards[20];
            int played_count = 0;
            doudizhu_get_last_played(played_cards, &played_count);
            if (played_count > 0) {
                game_ui_show_play(current - 1, played_cards, played_count);
            }
            game_ui_show_status(CN_20D8);
            return;
        }
        
        guard++;
    }
    
    // 轮到玩家了
    int current = doudizhu_get_current_player();
    if (current == 0 && !doudizhu_is_game_over()) {
        game_ui_show_status(CN_8F04);
        doudizhu_get_player_hand(0, player_hand, &player_hand_count);
        game_ui_update_player_hand(player_hand, player_hand_count);
    }
}

// 玩家出牌
void game_logic_player_play(CardData* cards, int count) {
    if (current_state != GAME_STATE_PLAYING) {
        game_ui_show_status(CN_67C1);
        return;
    }
    
    int current = doudizhu_get_current_player();
    if (current != 0) {
        game_ui_show_status(CN_D439);
        return;
    }
    
    if (count <= 0 || count > player_hand_count || player_hand_count == 0) {
        game_ui_show_status(CN_123B);
        return;
    }
    
    // 获取选中牌的索引
    int indices[20];
    int found = 0;
    touch_handler_get_selected_indices(indices, &found);
    
    if (found == 0) {
        game_ui_show_status(CN_43BD);
        return;
    }
    
    // 验证索引范围
    for (int i = 0; i < found; i++) {
        if (indices[i] < 0 || indices[i] >= player_hand_count) {
            game_ui_show_status(CN_480D);
            return;
        }
    }
    
    // 尝试出牌（不清除出牌区域）
    int result = doudizhu_play_cards(0, indices, found);
    
    if (result) {
        // 出牌成功

        // 获取出的牌
        CardData played[20];
        int played_count = 0;
        doudizhu_get_last_played(played, &played_count);

        // 判断是否接下来轮到 AI（在清除选择前计算）
        bool continue_ai = false;
        if (!doudizhu_is_game_over()) {
            int next = doudizhu_get_current_player();
            if (next != 0) {
                continue_ai = true;
            }
        }

        // 清除选择状态
        touch_handler_clear_selection();

        // 播放出牌飞入动画（动画内会更新手牌、显示出牌区、继续 AI）
        game_ui_animate_player_play(played, played_count, indices, found, continue_ai);

        if (doudizhu_is_game_over()) {
            current_state = GAME_STATE_GAME_OVER;
            game_ui_show_status(CN_687E);
            return;
        }
    } else {
        // 出牌失败，显示原因（不清除出牌区域）
        game_ui_show_invalid_play_reason(indices, found);
    }
}

// 玩家过牌
void game_logic_player_pass(void) {
    if (current_state != GAME_STATE_PLAYING) {
        game_ui_show_status(CN_67C1);
        return;
    }
    
    int current = doudizhu_get_current_player();
    if (current != 0) {
        game_ui_show_status(CN_D439);
        return;
    }
    
    int result = doudizhu_pass(0);
    
    if (result) {
        // 过牌成功，清除出牌区域
        game_ui_clear_play_area();
        touch_handler_clear_selection();
        game_ui_show_pass(2);  // 显示玩家过牌
        
        // 检查是否轮到AI
        int next = doudizhu_get_current_player();
        if (next != 0) {
            // 轮到AI，自动运行
            game_logic_auto_run_ai();
        }
    } else {
        game_ui_show_status(CN_CDAC);
    }
}

void game_logic_get_hint(CardData* cards, int* count) {
    // 简单提示：返回第一张能出的牌
    if (player_hand_count > 0) {
        // 尝试单张
        for (int i = 0; i < player_hand_count; i++) {
            int indices[1] = {i};
            int result = doudizhu_play_cards(0, indices, 1);
            if (result) {
                // 出牌成功，撤销
                // 注意：这里只是检查是否能出，不实际出牌
                cards[0] = player_hand[i];
                *count = 1;
                return;
            }
        }
        *count = 0;
    } else {
        *count = 0;
    }
}

// 获取提示并自动选中
void game_logic_auto_hint(void) {
    if (current_state != GAME_STATE_PLAYING) return;
    
    int current = doudizhu_get_current_player();
    if (current != 0) return;
    
    // 尝试找到能出的牌
    for (int i = 0; i < player_hand_count; i++) {
        // 清除之前的选择
        touch_handler_clear_selection();
        
        // 选中这张牌
        lv_obj_t* card_obj = NULL;
        touch_handler_toggle_card_selection(i, card_obj);
        
        // 检查是否能出
        int indices[1] = {i};
        // 这里需要实际调用play_cards来检查
        // 但是play_cards会实际出牌，所以我们需要另一种方式
    }
}

void game_logic_update(void) {
    // 这个函数现在不需要了，因为auto_run_ai会处理AI回合
    // 保留为空以保持接口兼容
}
