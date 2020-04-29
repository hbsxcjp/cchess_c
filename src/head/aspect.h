#ifndef ASPECT_H
#define ASPECT_H

#include "base.h"

// 着法记录类型
typedef struct MoveRec* MoveRec;
// 局面记录类型
typedef struct Aspect* Aspect;

// 新建、删除局面哈希表
Aspects newAspects(void);
void delAspects(Aspects aspects);

int getAspects_length(Aspects aspects);
// 取得局面的最近的着法记录
MoveRec getAspect(CAspects aspects, const wchar_t* FEN);
// 置入一个局面(如已存在相同局面则不需置入)，并返回最近的着法记录
MoveRec putAspect_bm(Aspects aspects, Board board, CMove move);
MoveRec putAspect_fs(Aspects aspects, const wchar_t* FEN, const wchar_t* rcStr);
// 删除某局面下的某着法，如果局面下已没有着法则删除局面
//bool removeAspect(Aspects aspects, const wchar_t* FEN, CMove move);

// 取得局面循环着法的距离
int getLoopBoutCount(CAspects aspects, const wchar_t* FEN);

// 遍历每个局面
void aspectsMap(CAspects aspects, void apply(Aspect, void*), void* ptr);

// 输出局面字符串
void writeAspectsStr(FILE* fout, CAspects aspects);
// 存储局面数据
void storeAspects(FILE* fout, CAspects aspects);

#endif