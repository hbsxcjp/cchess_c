#ifndef ASPECT_H
#define ASPECT_H

#include "base.h"

// 着法记录类型
typedef struct MoveRec* MoveRec;
// 局面记录类型
typedef struct Aspect* Aspect;
typedef struct AspectAnalysis* AspectAnalysis;

// 新建、删除局面哈希表
Aspects newAspects(void);
void delAspects(Aspects aspects);

void setAspects_m(Move move, void* aspects, void* board);
Aspects getAspects_bm(Board board, Move rootMove);
Aspects getAspects_fs(const char* fileName);
Aspects getAspects_fb(const char* fileName);

int getAspects_length(Aspects aspects);
// 取得局面的最近的着法记录
MoveRec getAspect(CAspects aspects, const void* aspSource, SourceType st);
// 删除某局面下的某着法，如果局面下已没有着法则删除局面
//bool removeAspect(Aspects aspects, const wchar_t* FEN, CMove move);

// 取得局面循环着法的距离
//int getLoopBoutCount(CAspects aspects, const wchar_t* FEN);

// 遍历每个局面
void aspectsMap(CAspects aspects, void apply(Aspect, void*), void* ptr);

// 输出局面字符串
void writeAspects(FILE* fout, CAspects aspects);
// 存储局面数据
void storeAspects(FILE* fout, CAspects aspects);
// 存储局面MD5数据
void storeAspectMD5(FILE* fout, CAspects aspects);
// 分析局面库储存状况
void analyzeAspects(FILE* fout, CAspects aspects);

#endif