#ifndef ASPECT_H
#define ASPECT_H

#include "base.h"

// 着法记录类型
typedef struct MoveRec* MoveRec;
// 局面记录类型
typedef struct Aspect* Aspect;
typedef struct AspectAnalysis* AspectAnalysis;

// 新建、删除局面类型
Aspects newAspects(SourceType st, int size);
void delAspects(Aspects aspects);

// 使用void*参数，目的是使其可以作为moveMap调用的函数参数
void appendAspects_mb(Move move, void* aspects, void* board);
// 读取局面数据文件加入局面类型
Aspects getAspects_fs(const char* fileName);
// 读取局面MD5数据文件加入局面类型
Aspects getAspects_fb(const char* fileName);

int getAspects_length(Aspects aspects);

// 取得局面循环着法的距离
//int getLoopBoutCount(CAspects aspects, const wchar_t* FEN);

// 遍历每个局面
void aspectsMap(CAspects aspects, void apply(Aspect, void*), void* ptr);

// 输出局面字符串，仅查看(应在Move对象的生命周期内调用)
void writeAspectStr(char* fileName, CAspects aspects);
// 存储局面数据，可查看可读取数据
void storeAspectLib(char* fileName, CAspects aspects);
// 存储局面MD5数据(二进制文件)，仅读取数据
void storeAspectMD5(char* fileName, CAspects aspects);

// 分析局面库储存状况
void analyzeAspects(char* fileName, CAspects aspects);

void testAspects(Aspects asps);

#endif