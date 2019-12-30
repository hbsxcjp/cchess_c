#ifndef MOVE_H
#define MOVE_H

#include "base.h"

// 新建一个move
Move* newMove(void);

// 判断两个move是否为同一个
bool isSameMove(const Move* amove, const Move* bmove);

// 加入下着
Move* addNext(Move* move);

// 加入变着
Move* addOther(Move* move);

// 删除move的所有下着move、变着move及自身
void delMove(Move* move);

// 切除move的下着
void cutNextMove(Move* move);

// 切除move的变着
void cutOhterMove(Move* move);

// 根据iccs着法设置内部着法
void setMove_iccs(Move* move, const wchar_t* iccsStr);

// 取得ICCS字符串
wchar_t* getICCS(wchar_t* ICCSStr, const Move* move);

// 辅助函数：根据中文着法取得走棋方颜色
PieceColor getColor_zh(const wchar_t* zhStr);

// 根据中文着法设置内部着法
void setMove_zh(Move* move, const Board* board, const wchar_t* zhStr);

// 根据内部着法表示设置中文着法
void setZhStr(Move* move, const Board* board);
//wchar_t* getZhStr(wchar_t* zhStr, const Board* board, const Move* move); 

// 设置remark
void setRemark(Move* move, wchar_t* remark);

// 按某种变换类型变换着法记录
void changeMove(Move* move, ChangeType ct);

#endif