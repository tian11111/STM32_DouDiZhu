#ifndef DOUDIZHU_ADAPTER_H
#define DOUDIZHU_ADAPTER_H

#include "graphics_data.h"

void doudizhu_init(void);
void doudizhu_deal_cards(void);
int doudizhu_play_cards(int player_idx, int* card_indices, int count);
int doudizhu_pass(int player_idx);
int doudizhu_ai_play(int player_idx);
const char* doudizhu_get_state_json(void);
void doudizhu_get_player_hand(int player_idx, CardData* cards, int* count);
int doudizhu_get_current_player(void);
int doudizhu_is_game_over(void);
void doudizhu_get_last_played(CardData* cards, int* count);

// 抢地主相关
void doudizhu_rob_landlord(int player_idx, bool rob);
int doudizhu_ai_rob_landlord(int player_idx);
void doudizhu_get_landlord_cards(CardData* cards, int* count);
bool doudizhu_is_landlord_robbed(void);
int doudizhu_get_landlord_index(void);

// 难度设置
void doudizhu_set_difficulty(int level);

// 获取提示 - 返回能出的牌的索引，返回0表示没有能出的牌
int doudizhu_get_hint(int* indices, int* count);

#endif
