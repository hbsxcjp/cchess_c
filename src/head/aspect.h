#ifndef ASPECT_H
#define ASPECT_H

#include "base.h"
#include <stdbool.h>

// 着法记录类型
typedef struct MoveRec* MoveRec;
typedef struct MoveReces* MoveReces;

// 新建、删除局面类型
Aspects newAspects(int size);

void delAspects(Aspects aspects);

int getAspectsLength(Aspects aspects);

// 获取着法记录（或空记录）
MoveReces getMoveReces(Aspects asps, Board board, PieceColor* color, ChangeType* leftRightCt);

// 存储文件到局面记录库
void appendAspectsFromCMfile(Aspects aspects, const char* fileName);

// 批量存储目录到局面记录库
void appendAspectsFromCMdir(Aspects aspects, const char* dirName);

// 读取局面数据文件加入局面库类型
Aspects getAspectsFromAspsfile(const char* fileName);

// 输出局面字符串，仅供查看
void writeAspectsShow(char* fileName, CAspects aspects);

// 存储局面数据，可查看可读取数据
void storeAspectsFEN(char* fileName, CAspects aspects);

// 分析局面库并储存状况
void analyzeAspects(char* fileName, CAspects aspects);

// 检查数据文件的数据记录是否一致
bool aspects_equal(CAspects asps0, Aspects asps1);

#endif