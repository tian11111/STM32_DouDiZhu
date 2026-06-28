#include "doudizhu_adapter.h"
#include "game_ui.h"
#include "strings_cn.h"
#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

// AI难度
typedef enum {
    DIFF_EASY = 0,
    DIFF_NORMAL,
    DIFF_HARD
} Difficulty;

static Difficulty ai_difficulty = DIFF_NORMAL;

// 斗地主牌型定义
typedef enum {
    PLAY_PASS = 0,
    PLAY_SINGLE,
    PLAY_PAIR,
    PLAY_TRIPLE,
    PLAY_TRIPLE_ONE,
    PLAY_TRIPLE_PAIR,
    PLAY_STRAIGHT,
    PLAY_DOUBLE_STRAIGHT,
    PLAY_AIRCRAFT,
    PLAY_AIRCRAFT_ONE,
    PLAY_AIRCRAFT_PAIR,
    PLAY_BOMB,
    PLAY_ROCKET
} PlayType;

// 斗地主牌结构
typedef struct {
    SuitType suit;
    PointType point;
    char name[8];
} DoudizhuCard;

// 出牌结构
typedef struct {
    PlayType type;
    int point;
    int length;
    int cardIndices[20];
    int cardCount;
} DoudizhuPlay;

// 玩家结构
typedef struct {
    char name[20];
    DoudizhuCard hand[20];
    int cardCount;
    bool isLandlord;
    int team;  // 0=地主队, 1=农民队
    int score;
} DoudizhuPlayer;

// 游戏状态
static DoudizhuPlayer players[3];
static DoudizhuPlay lastPlay;
static int lastPlayer = -1;
static int passCount = 0;
static int gameRound = 1;
static int currentPlayer = 0;
static bool gameOver = false;
static int landlordIndex = -1;
static bool landlordRobbed = false;
static DoudizhuCard landlordCards[3];
static char lastPlayedText[256] = {0};
static char stateBuffer[4096] = {0};
static CardData lastPlayedCards[20] = {0};
static int lastPlayedCount = 0;

// 牌型名称
static const char* suitSymbols[] = {CN_4EDC, CN_C262, CN_D088, CN_9CBA, CN_391B, CN_AB18};
static const char* pointNames[] = {"3","4","5","6","7","8","9","10","J","Q","K","A","2",CN_DA57,CN_4D2D};

// 转换函数：DoudizhuCard -> CardData
static void convert_to_carddata(const DoudizhuCard* src, CardData* dst) {
    dst->suit = src->suit;
    dst->point = src->point;
    strncpy(dst->display_name, src->name, sizeof(dst->display_name) - 1);
    dst->display_name[sizeof(dst->display_name) - 1] = '\0';
}

// 转换函数：CardData -> DoudizhuCard
static void convert_from_carddata(const CardData* src, DoudizhuCard* dst) {
    dst->suit = src->suit;
    dst->point = src->point;
    strncpy(dst->name, src->display_name, sizeof(dst->name) - 1);
    dst->name[sizeof(dst->name) - 1] = '\0';
}

// 初始化牌堆
static void initialize_deck(DoudizhuCard* deck) {
    int index = 0;
    for (int suit = SUIT_CLUBS; suit <= SUIT_SPADES; suit++) {
        for (int point = POINT_3; point <= POINT_2; point++) {
            deck[index].suit = (SuitType)suit;
            deck[index].point = (PointType)point;
            sprintf(deck[index].name, "%s%s", pointNames[point], suitSymbols[suit]);
            index++;
        }
    }
    // 小王
    deck[index].suit = SUIT_JOKER_SMALL;
    deck[index].point = POINT_SMALL_JOKER;
    strcpy(deck[index].name, CN_DA57);
    index++;
    // 大王
    deck[index].suit = SUIT_JOKER_BIG;
    deck[index].point = POINT_BIG_JOKER;
    strcpy(deck[index].name, CN_4D2D);
}

// 洗牌
static void shuffle_deck(DoudizhuCard* deck) {
    for (int i = 53; i > 0; i--) {
        int j = rand() % (i + 1);
        DoudizhuCard temp = deck[i];
        deck[i] = deck[j];
        deck[j] = temp;
    }
}

// 手牌排序
static void sort_hand(DoudizhuPlayer* player) {
    for (int i = 0; i < player->cardCount - 1; i++) {
        for (int j = 0; j < player->cardCount - i - 1; j++) {
            if (player->hand[j].point > player->hand[j + 1].point) {
                DoudizhuCard temp = player->hand[j];
                player->hand[j] = player->hand[j + 1];
                player->hand[j + 1] = temp;
            }
        }
    }
}

// 分析牌型
static DoudizhuPlay analyze_play(DoudizhuPlayer* player, int selected[], int count) {
    DoudizhuPlay play = {0};
    play.cardCount = count;

    if (count == 0) {
        play.type = PLAY_PASS;
        return play;
    }

    // 提取点数并排序
    int points[20];
    for (int i = 0; i < count; i++) {
        points[i] = player->hand[selected[i]].point;
    }
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (points[j] > points[j + 1]) {
                int temp = points[j];
                points[j] = points[j + 1];
                points[j + 1] = temp;
            }
        }
    }

    // 复制索引
    for (int i = 0; i < count; i++) {
        play.cardIndices[i] = selected[i];
    }

    // 王炸
    if (count == 2 && points[0] == POINT_SMALL_JOKER && points[1] == POINT_BIG_JOKER) {
        play.type = PLAY_ROCKET;
        play.point = POINT_BIG_JOKER;
        play.length = 2;
        return play;
    }

    // 对子
    if (count == 2 && points[0] == points[1]) {
        play.type = PLAY_PAIR;
        play.point = points[0];
        play.length = 2;
        return play;
    }

    // 单张
    if (count == 1) {
        play.type = PLAY_SINGLE;
        play.point = points[0];
        play.length = 1;
        return play;
    }

    // 三张
    if (count == 3 && points[0] == points[2]) {
        play.type = PLAY_TRIPLE;
        play.point = points[0];
        play.length = 3;
        return play;
    }

    // 三带一
    if (count == 4) {
        if ((points[0] == points[2] && points[2] != points[3]) ||
            (points[1] == points[3] && points[0] != points[1])) {
            play.type = PLAY_TRIPLE_ONE;
            play.point = (points[0] == points[2]) ? points[0] : points[1];
            play.length = 4;
            return play;
        }
    }

    // 三带二
    if (count == 5) {
        if (points[0] == points[2] && points[3] == points[4]) {
            play.type = PLAY_TRIPLE_PAIR;
            play.point = points[0];
            play.length = 5;
            return play;
        }
        if (points[0] == points[1] && points[2] == points[4]) {
            play.type = PLAY_TRIPLE_PAIR;
            play.point = points[2];
            play.length = 5;
            return play;
        }
    }

    // 炸弹
    if (count == 4 && points[0] == points[3]) {
        play.type = PLAY_BOMB;
        play.point = points[0];
        play.length = 4;
        return play;
    }

    // 连对
    if (count >= 6 && count % 2 == 0) {
        bool isDoubleStraight = true;
        for (int i = 0; i < count; i += 2) {
            if (points[i] != points[i + 1]) {
                isDoubleStraight = false;
                break;
            }
            if (points[i] >= POINT_2) {
                isDoubleStraight = false;
                break;
            }
            if (i >= 2 && points[i] != points[i - 2] + 1) {
                isDoubleStraight = false;
                break;
            }
        }
        if (isDoubleStraight) {
            play.type = PLAY_DOUBLE_STRAIGHT;
            play.point = points[0];
            play.length = count;
            return play;
        }
    }

    // 顺子
    if (count >= 5) {
        bool isStraight = true;
        for (int i = 0; i < count - 1; i++) {
            if (points[i + 1] != points[i] + 1) {
                isStraight = false;
                break;
            }
        }
        if (isStraight && points[0] >= POINT_3 && points[count - 1] <= POINT_A) {
            play.type = PLAY_STRAIGHT;
            play.point = points[0];
            play.length = count;
            return play;
        }
    }

    // 飞机
    if (count >= 6 && count % 3 == 0) {
        int tripleCount = count / 3;
        bool isAircraft = true;
        for (int i = 0; i < tripleCount; i++) {
            int baseIdx = i * 3;
            if (points[baseIdx] != points[baseIdx + 1] || 
                points[baseIdx + 1] != points[baseIdx + 2]) {
                isAircraft = false;
                break;
            }
            if (points[baseIdx] >= POINT_2) {
                isAircraft = false;
                break;
            }
            if (i > 0 && points[baseIdx] != points[baseIdx - 3] + 1) {
                isAircraft = false;
                break;
            }
        }
        if (isAircraft) {
            play.type = PLAY_AIRCRAFT;
            play.point = points[0];
            play.length = count;
            return play;
        }
    }

    // 飞机带单张
    if (count >= 8 && count % 4 == 0) {
        int tripleCount = count / 4;
        int aircraftPoints[20];
        int extraCards[20];
        int extraCount = 0;
        int foundTriples = 0;

        for (int i = 0; i < count && foundTriples < tripleCount; ) {
            if (i + 2 < count && 
                points[i] == points[i + 1] && 
                points[i + 1] == points[i + 2] &&
                points[i] < POINT_2) {
                aircraftPoints[foundTriples] = points[i];
                foundTriples++;
                i += 3;
            } else {
                extraCards[extraCount++] = points[i];
                i++;
            }
        }

        bool isAircraftWithSingle = (foundTriples == tripleCount);
        if (isAircraftWithSingle) {
            for (int i = 1; i < foundTriples; i++) {
                if (aircraftPoints[i] != aircraftPoints[i - 1] + 1) {
                    isAircraftWithSingle = false;
                    break;
                }
            }
        }

        if (isAircraftWithSingle && extraCount == tripleCount) {
            play.type = PLAY_AIRCRAFT_ONE;
            play.point = aircraftPoints[0];
            play.length = count;
            return play;
        }
    }

    // 飞机带对子
    if (count >= 10 && count % 5 == 0) {
        int tripleCount = count / 5;
        int aircraftPoints[20];
        int foundTriples = 0;
        int countMap[15] = {0};

        for (int i = 0; i < count; i++) {
            countMap[points[i]]++;
        }

        for (int p = 0; p < POINT_2; p++) {
            if (countMap[p] >= 3 && foundTriples < tripleCount) {
                aircraftPoints[foundTriples++] = p;
                countMap[p] -= 3;
            }
        }

        bool isAircraftWithPair = (foundTriples == tripleCount);
        if (isAircraftWithPair) {
            for (int i = 1; i < foundTriples; i++) {
                if (aircraftPoints[i] != aircraftPoints[i - 1] + 1) {
                    isAircraftWithPair = false;
                    break;
                }
            }
        }

        if (isAircraftWithPair) {
            int foundPairs = 0;
            for (int p = 0; p < 15 && foundPairs < tripleCount; p++) {
                if (countMap[p] >= 2) {
                    foundPairs++;
                    countMap[p] -= 2;
                }
            }
            if (foundPairs == tripleCount) {
                play.type = PLAY_AIRCRAFT_PAIR;
                play.point = aircraftPoints[0];
                play.length = count;
                return play;
            }
        }
    }

    play.type = PLAY_PASS;
    return play;
}

// 判断当前牌能否打过上家
static bool can_play_beat(DoudizhuPlay* current, DoudizhuPlay* last) {
    if (last->type == PLAY_PASS) {
        return true;
    }

    if (current->type == PLAY_ROCKET) {
        return true;
    }
    if (last->type == PLAY_ROCKET) {
        return false;
    }

    if (current->type == PLAY_BOMB && last->type != PLAY_ROCKET) {
        if (last->type == PLAY_BOMB) {
            return current->point > last->point;
        }
        return true;
    }
    if (last->type == PLAY_BOMB && current->type != PLAY_BOMB && current->type != PLAY_ROCKET) {
        return false;
    }

    // 必须牌型相同才能压
    if (current->type != last->type) {
        return false;
    }

    // 顺子、连对、飞机需要长度相同
    if (current->type == PLAY_STRAIGHT || current->type == PLAY_DOUBLE_STRAIGHT || 
        current->type == PLAY_AIRCRAFT || current->type == PLAY_AIRCRAFT_ONE || 
        current->type == PLAY_AIRCRAFT_PAIR) {
        if (current->length != last->length) {
            return false;
        }
    }
    
    return current->point > last->point;
}

// 检查队伍是否获胜
static bool check_team_win(void) {
    if (landlordIndex < 0) return false;

    if (players[landlordIndex].cardCount == 0) {
        return true;
    }

    for (int i = 0; i < 3; i++) {
        if (!players[i].isLandlord && players[i].cardCount == 0) {
            return true;
        }
    }

    return false;
}

// 更新玩家队伍
static void update_player_team(void) {
    for (int i = 0; i < 3; i++) {
        if (players[i].isLandlord) {
            players[i].team = 0;  // 地主队
        } else {
            players[i].team = 1;  // 农民队
        }
    }
}

// 分配地主底牌
static void assign_landlord_cards(void) {
    if (landlordIndex == -1) return;

    for (int i = 0; i < 3; i++) {
        players[landlordIndex].hand[players[landlordIndex].cardCount++] = landlordCards[i];
    }
    sort_hand(&players[landlordIndex]);
}

// 清空出牌文本
static void clear_last_played_text(void) {
    lastPlayedText[0] = '\0';
}

// 构建出牌文本
static void build_played_text(DoudizhuPlayer* player, int selected[], int count) {
    clear_last_played_text();
    int offset = 0;
    for (int i = 0; i < count; i++) {
        if (i > 0) {
            offset += sprintf(lastPlayedText + offset, " ");
        }
        offset += sprintf(lastPlayedText + offset, "%s", player->hand[selected[i]].name);
    }
}

// AI出牌逻辑（参考原来的逻辑）
static int get_best_play(int playerIdx) {
    DoudizhuPlayer* player = &players[playerIdx];
    int bestSelected[20] = {0};
    int bestCount = 0;
    int bestScore = -1000000000;
    bool found = false;

    // 先手出牌（lastPlay.type == PLAY_PASS）
    if (lastPlay.type == PLAY_PASS) {
        // 尝试单张
        for (int i = 0; i < player->cardCount; i++) {
            int sel[1] = {i};
            DoudizhuPlay play = analyze_play(player, sel, 1);
            if (play.type != PLAY_PASS) {
                int score = 100 - player->hand[i].point * 10;
                if (player->cardCount - 1 == 0) score += 100000;
                
                // 难度调节
                if (ai_difficulty == DIFF_EASY) {
                    score += rand() % 800 - 400;
                } else if (ai_difficulty == DIFF_HARD) {
                    if (player->cardCount - 1 <= 5) score += 300;
                }
                
                if (!found || score > bestScore) {
                    found = true;
                    bestScore = score;
                    bestCount = 1;
                    bestSelected[0] = i;
                }
            }
        }

        // 尝试对子
        for (int i = 0; i < player->cardCount - 1; i++) {
            if (player->hand[i].point == player->hand[i + 1].point) {
                int sel[2] = {i, i + 1};
                DoudizhuPlay play = analyze_play(player, sel, 2);
                if (play.type != PLAY_PASS) {
                    int score = 80 - player->hand[i].point * 10;
                    if (player->cardCount - 2 == 0) score += 100000;
                    if (!found || score > bestScore) {
                        found = true;
                        bestScore = score;
                        bestCount = 2;
                        bestSelected[0] = i;
                        bestSelected[1] = i + 1;
                    }
                }
            }
        }

        // 尝试三张
        for (int i = 0; i < player->cardCount - 2; i++) {
            if (player->hand[i].point == player->hand[i + 2].point) {
                int sel[3] = {i, i + 1, i + 2};
                DoudizhuPlay play = analyze_play(player, sel, 3);
                if (play.type != PLAY_PASS) {
                    int score = 60 - player->hand[i].point * 10;
                    if (player->cardCount - 3 == 0) score += 100000;
                    if (!found || score > bestScore) {
                        found = true;
                        bestScore = score;
                        bestCount = 3;
                        bestSelected[0] = i;
                        bestSelected[1] = i + 1;
                        bestSelected[2] = i + 2;
                    }
                }
            }
        }
    } else {
        // 跟牌：根据lastPlay.type来枚举能接住的牌型
        
        // 单张跟牌
        if (lastPlay.type == PLAY_SINGLE) {
            for (int i = 0; i < player->cardCount; i++) {
                int sel[1] = {i};
                DoudizhuPlay play = analyze_play(player, sel, 1);
                if (play.type == PLAY_SINGLE && can_play_beat(&play, &lastPlay)) {
                    int score = 100 - player->hand[i].point * 10;
                    if (player->cardCount - 1 == 0) score += 100000;
                    if (!found || score > bestScore) {
                        found = true;
                        bestScore = score;
                        bestCount = 1;
                        bestSelected[0] = i;
                    }
                }
            }
        }
        // 对子跟牌
        else if (lastPlay.type == PLAY_PAIR) {
            for (int i = 0; i < player->cardCount - 1; i++) {
                if (player->hand[i].point == player->hand[i + 1].point) {
                    int sel[2] = {i, i + 1};
                    DoudizhuPlay play = analyze_play(player, sel, 2);
                    if (play.type == PLAY_PAIR && can_play_beat(&play, &lastPlay)) {
                        int score = 80 - player->hand[i].point * 10;
                        if (player->cardCount - 2 == 0) score += 100000;
                        if (!found || score > bestScore) {
                            found = true;
                            bestScore = score;
                            bestCount = 2;
                            bestSelected[0] = i;
                            bestSelected[1] = i + 1;
                        }
                    }
                }
            }
        }
        // 三张跟牌
        else if (lastPlay.type == PLAY_TRIPLE) {
            for (int i = 0; i < player->cardCount - 2; i++) {
                if (player->hand[i].point == player->hand[i + 2].point) {
                    int sel[3] = {i, i + 1, i + 2};
                    DoudizhuPlay play = analyze_play(player, sel, 3);
                    if (play.type == PLAY_TRIPLE && can_play_beat(&play, &lastPlay)) {
                        int score = 60 - player->hand[i].point * 10;
                        if (player->cardCount - 3 == 0) score += 100000;
                        if (!found || score > bestScore) {
                            found = true;
                            bestScore = score;
                            bestCount = 3;
                            bestSelected[0] = i;
                            bestSelected[1] = i + 1;
                            bestSelected[2] = i + 2;
                        }
                    }
                }
            }
        }
        // 连对跟牌
        else if (lastPlay.type == PLAY_DOUBLE_STRAIGHT) {
            int needCount = lastPlay.length;
            for (int i = 0; i <= player->cardCount - needCount; i++) {
                int sel[20];
                int selCount = 0;
                int pairCount = 0;
                int lastPoint = -1;
                
                for (int j = i; j < player->cardCount && selCount < needCount; j++) {
                    if (j + 1 < player->cardCount && 
                        player->hand[j].point == player->hand[j + 1].point &&
                        player->hand[j].point < POINT_2 &&
                        player->hand[j].point != lastPoint) {
                        sel[selCount++] = j;
                        sel[selCount++] = j + 1;
                        lastPoint = player->hand[j].point;
                        pairCount++;
                        j++;
                    }
                }
                
                if (pairCount == needCount / 2) {
                    DoudizhuPlay play = analyze_play(player, sel, selCount);
                    if (play.type == PLAY_DOUBLE_STRAIGHT && can_play_beat(&play, &lastPlay)) {
                        int score = 70 - play.point * 10;
                        if (!found || score > bestScore) {
                            found = true;
                            bestScore = score;
                            bestCount = selCount;
                            for (int k = 0; k < selCount; k++) bestSelected[k] = sel[k];
                        }
                    }
                }
            }
        }
        // 顺子跟牌
        else if (lastPlay.type == PLAY_STRAIGHT) {
            int needCount = lastPlay.length;
            for (int i = 0; i <= player->cardCount - needCount; i++) {
                int sel[20];
                int selCount = 0;
                bool valid = true;
                
                for (int j = i; j < i + needCount && j < player->cardCount; j++) {
                    if (player->hand[j].point >= POINT_2) {
                        valid = false;
                        break;
                    }
                    sel[selCount++] = j;
                }
                
                if (valid && selCount == needCount) {
                    DoudizhuPlay play = analyze_play(player, sel, selCount);
                    if (play.type == PLAY_STRAIGHT && can_play_beat(&play, &lastPlay)) {
                        int score = 70 - play.point * 10;
                        if (!found || score > bestScore) {
                            found = true;
                            bestScore = score;
                            bestCount = selCount;
                            for (int k = 0; k < selCount; k++) bestSelected[k] = sel[k];
                        }
                    }
                }
            }
        }
        
        // 炸弹兜底（可以压任何牌型）
        for (int i = 0; i < player->cardCount - 3; i++) {
            if (player->hand[i].point == player->hand[i + 3].point) {
                int sel[4] = {i, i + 1, i + 2, i + 3};
                DoudizhuPlay play = analyze_play(player, sel, 4);
                if (play.type == PLAY_BOMB && can_play_beat(&play, &lastPlay)) {
                    int score = 200 - player->hand[i].point * 10;
                    if (player->cardCount - 4 == 0) score += 100000;
                    if (!found || score > bestScore) {
                        found = true;
                        bestScore = score;
                        bestCount = 4;
                        bestSelected[0] = i;
                        bestSelected[1] = i + 1;
                        bestSelected[2] = i + 2;
                        bestSelected[3] = i + 3;
                    }
                }
            }
        }

        // 王炸兜底
        bool hasSmall = false, hasBig = false;
        int smallIdx = -1, bigIdx = -1;
        for (int i = 0; i < player->cardCount; i++) {
            if (player->hand[i].point == POINT_SMALL_JOKER) {
                hasSmall = true;
                smallIdx = i;
            }
            if (player->hand[i].point == POINT_BIG_JOKER) {
                hasBig = true;
                bigIdx = i;
            }
        }
        if (hasSmall && hasBig) {
            int sel[2] = {smallIdx, bigIdx};
            DoudizhuPlay play = analyze_play(player, sel, 2);
            if (play.type == PLAY_ROCKET) {
                int score = 500;
                if (player->cardCount - 2 == 0) score += 100000;
                if (!found || score > bestScore) {
                    found = true;
                    bestScore = score;
                    bestCount = 2;
                    bestSelected[0] = smallIdx;
                    bestSelected[1] = bigIdx;
                }
            }
        }
    }

    if (found) {
        // 出牌
        DoudizhuPlay play = analyze_play(player, bestSelected, bestCount);
        build_played_text(player, bestSelected, bestCount);

        // 在移除手牌之前存储要出的牌
        lastPlayedCount = bestCount;
        for (int i = 0; i < bestCount; i++) {
            convert_to_carddata(&player->hand[bestSelected[i]], &lastPlayedCards[i]);
        }

        // 移除手牌（从后往前删除）
        for (int k = bestCount - 1; k >= 0; k--) {
            int idx = bestSelected[k];
            for (int j = idx; j < player->cardCount - 1; j++) {
                player->hand[j] = player->hand[j + 1];
            }
            player->cardCount--;
        }

        lastPlay = play;
        lastPlayer = playerIdx;
        passCount = 0;
        currentPlayer = (playerIdx + 1) % 3;

        if (check_team_win()) {
            gameOver = true;
        }

        gameRound++;
        return 1;
    } else {
        // 过牌
        if (lastPlay.type != PLAY_PASS) {
            strcpy(lastPlayedText, CN_8853);
            passCount++;
            lastPlayer = playerIdx;
            currentPlayer = (playerIdx + 1) % 3;
            
            // 清除最后出的牌
            lastPlayedCount = 0;
            memset(lastPlayedCards, 0, sizeof(lastPlayedCards));

            if (passCount >= 2) {
                // 两人都过，重置lastPlay，开始新一轮
                memset(&lastPlay, 0, sizeof(lastPlay));
                lastPlay.type = PLAY_PASS;
                lastPlayer = -1;
                passCount = 0;
                clear_last_played_text();
            }

            gameRound++;
            return 1;
        }
        return 0;
    }
}

// 适配器函数实现
void doudizhu_init(void) {
    /* srand() is called in game_logic_start_after_difficulty() using HAL_GetTick() */
    
    // 重置游戏状态
    for (int i = 0; i < 3; i++) {
        players[i].cardCount = 0;
        players[i].isLandlord = false;
        players[i].team = 1;
        players[i].score = 0;
        memset(players[i].hand, 0, sizeof(players[i].hand));
        memset(players[i].name, 0, sizeof(players[i].name));
    }
    
    lastPlay.type = PLAY_PASS;
    lastPlay.cardCount = 0;
    lastPlayer = -1;
    passCount = 0;
    gameRound = 1;
    currentPlayer = 0;
    gameOver = false;
    landlordIndex = -1;
    landlordRobbed = false;
    memset(landlordCards, 0, sizeof(landlordCards));
    clear_last_played_text();
    lastPlayedCount = 0;
    memset(lastPlayedCards, 0, sizeof(lastPlayedCards));
    
    // 设置玩家名称
    strcpy(players[0].name, CN_069A);
    strcpy(players[1].name, "AI-1");
    strcpy(players[2].name, "AI-2");
}

void doudizhu_deal_cards(void) {
    static DoudizhuCard deck[54];

    /* reset player hands before dealing */
    for (int i = 0; i < 3; i++) {
        players[i].cardCount = 0;
        players[i].isLandlord = false;
        memset(players[i].hand, 0, sizeof(players[i].hand));
        memset(players[i].name, 0, sizeof(players[i].name));
    }
    strcpy(players[0].name, CN_1681);
    strcpy(players[1].name, CN_47D3);
    strcpy(players[2].name, CN_0AB2);

    initialize_deck(deck);
    shuffle_deck(deck);
    
    // 发牌
    for (int i = 0; i < 51; i++) {
        int playerIdx = i % 3;
        players[playerIdx].hand[players[playerIdx].cardCount++] = deck[i];
    }
    
    // 保存底牌
    for (int i = 0; i < 3; i++) {
        landlordCards[i] = deck[51 + i];
    }
    
    // 排序
    for (int i = 0; i < 3; i++) {
        sort_hand(&players[i]);
    }
    
    // 不自动分配地主，等待抢地主
    landlordRobbed = false;
    currentPlayer = 0;
    
    // 更新UI
    static CardData cardData[20];
    for (int i = 0; i < players[0].cardCount; i++) {
        convert_to_carddata(&players[0].hand[i], &cardData[i]);
    }
    game_ui_update_player_hand(cardData, players[0].cardCount);
    game_ui_update_ai(0, players[1].name, players[1].cardCount);
    game_ui_update_ai(1, players[2].name, players[2].cardCount);
}

// 抢地主
void doudizhu_rob_landlord(int player_idx, bool rob) {
    if (landlordRobbed) return;
    
    if (rob) {
        // 设置地主
        landlordIndex = player_idx;
        players[player_idx].isLandlord = true;
        // 分配地主底牌
        assign_landlord_cards();
        // 更新队伍
        update_player_team();
        // 抢地主完成
        landlordRobbed = true;
        // 地主先出牌
        currentPlayer = landlordIndex;
        
        // 更新UI显示底牌
        CardData cardData[20];
        for (int i = 0; i < players[0].cardCount; i++) {
            convert_to_carddata(&players[0].hand[i], &cardData[i]);
        }
        game_ui_update_player_hand(cardData, players[0].cardCount);
        game_ui_update_ai(0, players[1].name, players[1].cardCount);
        game_ui_update_ai(1, players[2].name, players[2].cardCount);
    }
}

// AI抢地主逻辑
int doudizhu_ai_rob_landlord(int player_idx) {
    if (landlordRobbed) return 0;
    
    // 根据手牌判断是否抢地主
    int bombCount = 0;
    bool hasRocket = false;
    
    // 检查炸弹
    for (int i = 0; i < players[player_idx].cardCount - 3; i++) {
        if (players[player_idx].hand[i].point == players[player_idx].hand[i + 3].point) {
            bombCount++;
        }
    }
    
    // 检查王炸
    bool hasSmall = false, hasBig = false;
    for (int i = 0; i < players[player_idx].cardCount; i++) {
        if (players[player_idx].hand[i].point == POINT_SMALL_JOKER) hasSmall = true;
        if (players[player_idx].hand[i].point == POINT_BIG_JOKER) hasBig = true;
    }
    if (hasSmall && hasBig) hasRocket = true;
    
    // 随机决定是否抢
    int randVal = rand() % 100;
    if ((bombCount > 0 || hasRocket) && randVal < 80) {
        return 1;  // 抢地主
    } else if (randVal < 20) {
        return 1;  // 抢地主
    }
    
    return 0;  // 不抢
}

void doudizhu_get_landlord_cards(CardData* cards, int* count) {
    *count = 3;
    for (int i = 0; i < 3; i++) {
        convert_to_carddata(&landlordCards[i], &cards[i]);
    }
}

bool doudizhu_is_landlord_robbed(void) {
    return landlordRobbed;
}

int doudizhu_get_landlord_index(void) {
    return landlordIndex;
}

int doudizhu_play_cards(int player_idx, int* card_indices, int count) {
    if (gameOver || player_idx != currentPlayer || !landlordRobbed) {
        return 0;
    }
    
    DoudizhuPlayer* player = &players[player_idx];
    
    // 验证索引
    for (int i = 0; i < count; i++) {
        if (card_indices[i] < 0 || card_indices[i] >= player->cardCount) {
            return 0;
        }
    }
    
    // 分析牌型
    DoudizhuPlay play = analyze_play(player, card_indices, count);
    if (play.type == PLAY_PASS) {
        return 0;
    }
    
    // 检查是否能打过上家
    if (!can_play_beat(&play, &lastPlay)) {
        return 0;
    }
    
    // 构建出牌文本
    build_played_text(player, card_indices, count);
    
    // 在移除手牌之前存储要出的牌
    lastPlayedCount = count;
    for (int i = 0; i < count; i++) {
        convert_to_carddata(&player->hand[card_indices[i]], &lastPlayedCards[i]);
    }
    
    // 移除手牌
    for (int k = count - 1; k >= 0; k--) {
        int idx = card_indices[k];
        for (int j = idx; j < player->cardCount - 1; j++) {
            player->hand[j] = player->hand[j + 1];
        }
        player->cardCount--;
    }
    
    lastPlay = play;
    lastPlayer = player_idx;
    passCount = 0;
    currentPlayer = (player_idx + 1) % 3;
    
    if (check_team_win()) {
        gameOver = true;
        game_ui_show_status(player_idx == 0 ? CN_AA0A : CN_20D8);
    } else {
        game_ui_show_play(player_idx, lastPlayedCards, lastPlayedCount);
    }
    
    gameRound++;
    return 1;
}

int doudizhu_pass(int player_idx) {
    if (gameOver || player_idx != currentPlayer || !landlordRobbed) {
        return 0;
    }
    
    if (lastPlay.type == PLAY_PASS) {
        return 0;
    }
    
    strcpy(lastPlayedText, CN_8853);
    passCount++;
    lastPlayer = player_idx;
    currentPlayer = (player_idx + 1) % 3;
    
    if (passCount >= 2) {
        lastPlay.type = PLAY_PASS;
        lastPlayer = -1;
        passCount = 0;
        clear_last_played_text();
    }
    
    gameRound++;
    game_ui_show_status(CN_7244);
    return 1;
}

int doudizhu_ai_play(int player_idx) {
    if (gameOver || player_idx != currentPlayer || !landlordRobbed) {
        return 0;
    }
    
    game_ui_show_status(CN_E55D);
    int result = get_best_play(player_idx);
    
    if (gameOver) {
        game_ui_show_status(player_idx == landlordIndex ? CN_6FFE : CN_DED4);
    } else if (currentPlayer == 0) {
        game_ui_show_status(CN_8F04);
    }
    
    // 更新AI手牌显示
    game_ui_update_ai(player_idx - 1, players[player_idx].name, players[player_idx].cardCount);
    
    return result;
}

const char* doudizhu_get_state_json(void) {
    int offset = 0;
    
    offset += sprintf(stateBuffer + offset, "{");
    offset += sprintf(stateBuffer + offset, "\"currentPlayer\":%d,", currentPlayer);
    offset += sprintf(stateBuffer + offset, "\"gameOver\":%s,", gameOver ? "true" : "false");
    offset += sprintf(stateBuffer + offset, "\"landlordIndex\":%d,", landlordIndex);
    offset += sprintf(stateBuffer + offset, "\"landlordRobbed\":%s,", landlordRobbed ? "true" : "false");
    
    // 玩家手牌
    offset += sprintf(stateBuffer + offset, "\"hand\":[");
    for (int i = 0; i < players[0].cardCount; i++) {
        if (i > 0) offset += sprintf(stateBuffer + offset, ",");
        offset += sprintf(stateBuffer + offset, "\"%s\"", players[0].hand[i].name);
    }
    offset += sprintf(stateBuffer + offset, "],");
    
    // 各玩家牌数
    offset += sprintf(stateBuffer + offset, "\"playerCount\":%d,", players[0].cardCount);
    offset += sprintf(stateBuffer + offset, "\"ai1Count\":%d,", players[1].cardCount);
    offset += sprintf(stateBuffer + offset, "\"ai2Count\":%d,", players[2].cardCount);
    
    // 上一手牌
    offset += sprintf(stateBuffer + offset, "\"lastPlayType\":%d,", lastPlay.type);
    offset += sprintf(stateBuffer + offset, "\"lastPlayedText\":\"%s\"", lastPlayedText);
    offset += sprintf(stateBuffer + offset, "}");
    
    return stateBuffer;
}

void doudizhu_get_player_hand(int player_idx, CardData* cards, int* count) {
    if (player_idx < 0 || player_idx >= 3) {
        *count = 0;
        return;
    }
    
    *count = players[player_idx].cardCount;
    for (int i = 0; i < players[player_idx].cardCount; i++) {
        convert_to_carddata(&players[player_idx].hand[i], &cards[i]);
    }
}

int doudizhu_get_current_player(void) {
    return currentPlayer;
}

int doudizhu_is_game_over(void) {
    return gameOver ? 1 : 0;
}

void doudizhu_get_last_played(CardData* cards, int* count) {
    *count = lastPlayedCount;
    for (int i = 0; i < lastPlayedCount; i++) {
        cards[i] = lastPlayedCards[i];
    }
}

void doudizhu_set_difficulty(int level) {
    if (level >= 0 && level <= 2) {
        ai_difficulty = (Difficulty)level;
    }
}

// 获取提示 - 返回能出的牌的索引
int doudizhu_get_hint(int* indices, int* count) {
    *count = 0;
    
    DoudizhuPlayer* player = &players[0];  // 玩家
    
    if (player->cardCount == 0) {
        return 0;
    }
    
    // 先手出牌
    if (lastPlay.type == PLAY_PASS) {
        // 尝试单张
        for (int i = 0; i < player->cardCount; i++) {
            int sel[1] = {i};
            DoudizhuPlay play = analyze_play(player, sel, 1);
            if (play.type != PLAY_PASS) {
                indices[0] = i;
                *count = 1;
                return 1;
            }
        }
        
        // 尝试对子
        for (int i = 0; i < player->cardCount - 1; i++) {
            if (player->hand[i].point == player->hand[i + 1].point) {
                int sel[2] = {i, i + 1};
                DoudizhuPlay play = analyze_play(player, sel, 2);
                if (play.type != PLAY_PASS) {
                    indices[0] = i;
                    indices[1] = i + 1;
                    *count = 2;
                    return 1;
                }
            }
        }
        
        // 尝试三张
        for (int i = 0; i < player->cardCount - 2; i++) {
            if (player->hand[i].point == player->hand[i + 2].point) {
                int sel[3] = {i, i + 1, i + 2};
                DoudizhuPlay play = analyze_play(player, sel, 3);
                if (play.type != PLAY_PASS) {
                    indices[0] = i;
                    indices[1] = i + 1;
                    indices[2] = i + 2;
                    *count = 3;
                    return 1;
                }
            }
        }
    } else {
        // 跟牌：根据lastPlay.type来枚举能接住的牌型
        
        // 单张跟牌
        if (lastPlay.type == PLAY_SINGLE) {
            for (int i = 0; i < player->cardCount; i++) {
                int sel[1] = {i};
                DoudizhuPlay play = analyze_play(player, sel, 1);
                if (play.type == PLAY_SINGLE && can_play_beat(&play, &lastPlay)) {
                    indices[0] = i;
                    *count = 1;
                    return 1;
                }
            }
        }
        // 对子跟牌
        else if (lastPlay.type == PLAY_PAIR) {
            for (int i = 0; i < player->cardCount - 1; i++) {
                if (player->hand[i].point == player->hand[i + 1].point) {
                    int sel[2] = {i, i + 1};
                    DoudizhuPlay play = analyze_play(player, sel, 2);
                    if (play.type == PLAY_PAIR && can_play_beat(&play, &lastPlay)) {
                        indices[0] = i;
                        indices[1] = i + 1;
                        *count = 2;
                        return 1;
                    }
                }
            }
        }
        // 三张跟牌
        else if (lastPlay.type == PLAY_TRIPLE) {
            for (int i = 0; i < player->cardCount - 2; i++) {
                if (player->hand[i].point == player->hand[i + 2].point) {
                    int sel[3] = {i, i + 1, i + 2};
                    DoudizhuPlay play = analyze_play(player, sel, 3);
                    if (play.type == PLAY_TRIPLE && can_play_beat(&play, &lastPlay)) {
                        indices[0] = i;
                        indices[1] = i + 1;
                        indices[2] = i + 2;
                        *count = 3;
                        return 1;
                    }
                }
            }
        }
        // 连对跟牌
        else if (lastPlay.type == PLAY_DOUBLE_STRAIGHT) {
            int needCount = lastPlay.length;
            for (int i = 0; i <= player->cardCount - needCount; i++) {
                int sel[20];
                int selCount = 0;
                int pairCount = 0;
                int lastPoint = -1;
                
                for (int j = i; j < player->cardCount && selCount < needCount; j++) {
                    if (j + 1 < player->cardCount && 
                        player->hand[j].point == player->hand[j + 1].point &&
                        player->hand[j].point < POINT_2 &&
                        player->hand[j].point != lastPoint) {
                        sel[selCount++] = j;
                        sel[selCount++] = j + 1;
                        lastPoint = player->hand[j].point;
                        pairCount++;
                        j++;
                    }
                }
                
                if (pairCount == needCount / 2) {
                    DoudizhuPlay play = analyze_play(player, sel, selCount);
                    if (play.type == PLAY_DOUBLE_STRAIGHT && can_play_beat(&play, &lastPlay)) {
                        for (int k = 0; k < selCount; k++) {
                            indices[k] = sel[k];
                        }
                        *count = selCount;
                        return 1;
                    }
                }
            }
        }
        // 顺子跟牌
        else if (lastPlay.type == PLAY_STRAIGHT) {
            int needCount = lastPlay.length;
            for (int i = 0; i <= player->cardCount - needCount; i++) {
                int sel[20];
                int selCount = 0;
                bool valid = true;
                
                for (int j = i; j < i + needCount && j < player->cardCount; j++) {
                    if (player->hand[j].point >= POINT_2) {
                        valid = false;
                        break;
                    }
                    sel[selCount++] = j;
                }
                
                if (valid && selCount == needCount) {
                    DoudizhuPlay play = analyze_play(player, sel, selCount);
                    if (play.type == PLAY_STRAIGHT && can_play_beat(&play, &lastPlay)) {
                        for (int k = 0; k < selCount; k++) {
                            indices[k] = sel[k];
                        }
                        *count = selCount;
                        return 1;
                    }
                }
            }
        }
        
        // 炸弹兜底
        for (int i = 0; i < player->cardCount - 3; i++) {
            if (player->hand[i].point == player->hand[i + 3].point) {
                int sel[4] = {i, i + 1, i + 2, i + 3};
                DoudizhuPlay play = analyze_play(player, sel, 4);
                if (play.type == PLAY_BOMB && can_play_beat(&play, &lastPlay)) {
                    indices[0] = i;
                    indices[1] = i + 1;
                    indices[2] = i + 2;
                    indices[3] = i + 3;
                    *count = 4;
                    return 1;
                }
            }
        }
        
        // 王炸兜底
        bool hasSmall = false, hasBig = false;
        int smallIdx = -1, bigIdx = -1;
        for (int i = 0; i < player->cardCount; i++) {
            if (player->hand[i].point == POINT_SMALL_JOKER) {
                hasSmall = true;
                smallIdx = i;
            }
            if (player->hand[i].point == POINT_BIG_JOKER) {
                hasBig = true;
                bigIdx = i;
            }
        }
        if (hasSmall && hasBig) {
            int sel[2] = {smallIdx, bigIdx};
            DoudizhuPlay play = analyze_play(player, sel, 2);
            if (play.type == PLAY_ROCKET) {
                indices[0] = smallIdx;
                indices[1] = bigIdx;
                *count = 2;
                return 1;
            }
        }
    }
    
    return 0;  // 没有能出的牌
}
