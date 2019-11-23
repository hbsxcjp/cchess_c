#ifndef MOVE_H
#define MOVE_H

#include "base.h"

// 新建一个move
Move* newMove(void);

// 判断两个move是否为同一个
bool isSame(const Move* amove, const Move* bmove);

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
wchar_t* getICCS(wchar_t* ICCSStr, size_t n, const Move* move);

// 根据中文着法设置内部着法
void setMove_zh(Move* move, const Board* board, const wchar_t* zhStr, size_t n);

// 根据内部着法表示取得中文着法
wchar_t* getZhStr(wchar_t* zhStr, size_t n, const Board* board, const Move* move); 

// 设置remark
void setRemark(Move* move, wchar_t* remark);

#endif