#ifndef ASPECT_H
#define ASPECT_H

#include "base.h"

// 着法记录类型
typedef struct MoveRec* MoveRec;
// 局面记录类型
typedef struct Aspect* Aspect;

// 哈希函数
unsigned int BKDRHash(const wchar_t* wstr);
// 新建、删除局面哈希表
AspectTable newAspectTable(void);
void delAspectTable(AspectTable table);

int aspectTable_length(AspectTable table);
// 取得局面的最近的着法记录
MoveRec aspectTable_get(AspectTable table, const wchar_t* FEN);
// 置入一个局面(如已存在相同局面则不需置入)，并返回最近的着法记录
MoveRec aspectTable_put(AspectTable table, const wchar_t* FEN, CMove move);
// 删除某局面下的某着法，如果局面下已没有着法则删除局面
bool aspectTable_remove(AspectTable table, const wchar_t* FEN, CMove move);

// 取得局面最近已循环次数
int getLoopCount(AspectTable table, const wchar_t* FEN);

// 输出着法记录的字符串
wchar_t* getMoveRecStr(wchar_t* wstr, MoveRec mrc);
// 输出局面记录的字符串
wchar_t* getAspectStr(wchar_t* wstr, Aspect asp);
// 输出局面哈希表的字符串
wchar_t* getAspectTableStr(wchar_t* wstr, AspectTable table);

#endif