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
wchar_t* getICCS(wchar_t* str, size_t n, const Move* move);

// 取得Zh字符串
wchar_t* getZH(wchar_t* str, size_t n, const Move* move);

// 执行move
void moveDo(Instance* ins, const Move* move);

// 反向执行move
void moveUndo(Instance* ins, const Move* move);

// 根据seats设置move
void setMoveFromSeats(Move* move, const Seat fseat, const Seat tseat,
    const wchar_t* remark);

// 根据str设置move
void setMoveFromStr(Move* move,
    const wchar_t* str, RecFormat fmt, const wchar_t* remark);

// 取得表示move的字符串
wchar_t* getMovString(wchar_t* str, size_t n, const Move* move);

#endif