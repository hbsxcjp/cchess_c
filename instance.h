#ifndef INSTANCE_H
#define INSTANCE_H

#include "base.h"

// 加入下着
Move* addNext(Move* move);

// 加入变着
Move* addOther(Move* move);

// 删除move的所有下着move、变着move及自身
void delMove(Move* move);

// 切除move的下着或变着
void cutMove(Move* move, bool isNext);

// 取得ICCS字符串
wchar_t* getICCS(wchar_t* str, size_t n, const Move* move);

// 取得Zh字符串
wchar_t* getZH(wchar_t* str, size_t n, const Move* move);

// 执行move
void moveDo(const Move* move);

// 反向执行move
void moveUndo(const Move* move);

// 根据seats设置move
void setMoveFromSeats(Move* move, Seat fseat, Seat tseat, wchar_t* remark);

// 根据rowcol设置move
void setMoveFromRowcol(Move* move,
    int frowcol, int trowcol, const wchar_t* remark);

// 根据str设置move
void setMoveFromStr(Move* move,
    const wchar_t* str, RecFormat fmt, const wchar_t* remark);

// 取得move的表示字符串
wchar_t* getMovString(wchar_t* str, size_t n, const Move* move);

// 从文件读取到instance
void read(Instance* ins, const char* filename);

// 从instance存储到文件
void write(const Instance* ins, const char* filename);

// 前进一步
void go(Instance* ins);

// 后退一步
void back(Instance* ins);

// 后退至指定move
void backTo(Instance* ins, const Move* move);

// 前进到变着
void goOther(Instance* ins);

// 前进数步
void goInc(Instance* ins, int inc);

// 重置instance到初始状态
void reset(Instance* ins);

const wchar_t* moveInfo(wchar_t* str, size_t n, const Instance* ins);

void changeInstanceSide(Instance* ins, ChangeType ct);

// 测试本翻译单元各种对象、函数
void testInstance(FILE* fout);

#endif