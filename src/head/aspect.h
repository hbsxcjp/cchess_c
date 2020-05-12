#ifndef ASPECT_H
#define ASPECT_H

#include "base.h"

// 着法记录类型
typedef struct MoveRec* MoveRec;
// 局面记录类型
typedef struct Aspect* Aspect;
typedef struct AspectAnalysis* AspectAnalysis;

// 新建、删除局面哈希表
Aspects newAspects(SourceType st, int size);
void delAspects(Aspects aspects);

// 非MD5格式局面数据转换成MD5格式局面数据
void transToMD5Aspects(Aspects asps);

void setAspects_mb(Move move, void* aspects, void* board); // 使用void*参数，目的是使其可以作为moveMap调用的函数参数
void setAspects_fs(Aspects aspects, const char* fileName);
void setAspects_fb(Aspects aspects, const char* fileName);

int getAspects_length(Aspects aspects);

// 取得局面循环着法的距离
//int getLoopBoutCount(CAspects aspects, const wchar_t* FEN);

// 遍历每个局面
void aspectsMap(CAspects aspects, void apply(Aspect, void*), void* ptr);

// 输出局面字符串，仅查看
void writeAspectStr(FILE* fout, CAspects aspects);
// 存储局面数据，可查看可读取数据
void storeAspectLib(FILE* fout, CAspects aspects);
// 存储局面MD5数据(二进制文件)，仅读取数据
void storeAspectMD5(FILE* fout, CAspects aspects);

// 分析局面库储存状况
void analyzeAspects(FILE* fout, CAspects aspects);

#endif