#ifndef TOOLS_H
#define TOOLS_H

#include "base.h"

// 去掉字符串前后的空白字符
char* trim(char* str);

// 去掉宽字符串前后的空白字符
wchar_t* wtrim(wchar_t* wstr);

// 从文件名去掉扩展名
char* getFileName_cut(char* filename);

// 从文件名提取扩展名
const char* getExt(const char* filename);

// 从文件当前指针至尾部获取宽字符串
wchar_t* getWString(FILE* fin);

// 复制文件
int copyFile(const char* SourceFile, const char* NewFile);

// 提取目录下的文件名列表
int getFiles(char* fileNames[],const char* path);

// 测试函数
const wchar_t* testTools(wchar_t* wstr);


#endif