#ifndef ASPECT_H
#define ASPECT_H

#include "base.h"
#include "md5.h"
#include "sha1.h"
#include <stdbool.h>

//#define MD5Hash

#ifdef MD5Hash
#define HashSize MD5HashSize
#define getHashFun ustrToMD5
#else
#define HashSize SHA1HashSize
#define getHashFun ustrToSHA1
#endif

// 着法记录类型
typedef struct MoveRec* MoveRec;
// 局面记录类型
typedef struct Aspect* Aspect;
typedef struct AspectAnalysis* AspectAnalysis;

// 新建、删除局面类型
Aspects newAspects(AspFormat st, int size);
void delAspects(Aspects aspects);

// 读取局面数据文件加入局面库类型
Aspects getAspects_fs(const char* fileName);
// 读取局面MD5数据文件加入局面库类型
Aspects getAspects_fb(const char* fileName);

// 存储文件到局面记录库
void appendAspects_file(Aspects aspects, const char* fileName);
// 批量存储目录到局面记录库
void appendAspects_dir(Aspects aspects, const char* dirName);

int getAspects_length(Aspects aspects);

// 输出局面字符串，仅供查看
void writeAspectShow(char* fileName, CAspects aspects);
// 存储局面数据，可查看可读取数据
void storeAspectFEN(char* fileName, CAspects aspects);
// 存储局面hash数据(二进制文件)，仅读取数据
void storeAspectHash(char* fileName, CAspects aspects);

// 分析局面库并储存状况
void analyzeAspects(char* fileName, CAspects aspects);

// 检查数据文件的数据记录是否一致
bool aspects_equal(CAspects asps0, CAspects asps1);

#endif