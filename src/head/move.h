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

// 取得ICCS字符串
wchar_t* getICCS(wchar_t* ICCSStr, size_t n, const Move* move);

// 取得Zh字符串
wchar_t* getZH(wchar_t* ZHStr, size_t n, const Move* move);

// 设置remark
void setRemark(Move* move, const wchar_t* remark);

// 取得表示move的字符串
wchar_t* getMovString_iccszh(wchar_t* MovStr, size_t n, const Move* move, RecFormat fmt);

#endif