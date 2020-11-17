#ifndef OPERATEFILE_H
#define OPERATEFILE_H

#include <assert.h>
#include <ctype.h>
#include <iconv.h>
#include <locale.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

#ifdef __linux
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
//printf("Linux\n");
#else
//#ifdef WINVER
#include <conio.h>
#include <io.h>
#include <winsock2.h>
//printf("Windows\n");
#include <windows.h>
#endif

// 跨平台读取utf-8编码文件的打开函数
FILE* openFile_utf8(const char* fileName, const char* modes);

// 从文件名提取目录名
void getDirName(char* dirName, const char* fileName);

// 从文件名提取纯文件名，去掉目录
const char* getFileName(const char* filename);

// 从文件名提取扩展名
const char* getExtName(const char* filename);

// 转换文件的扩展名
void transFileExtName(char* filename, const char* extname);

// 从输入文件获取全部内容至宽字符串
wchar_t* getWString(FILE* fin);

// 创建目录
int makeDir(const char* dirName);

// 复制文件
int copyFile(const char* SourceFile, const char* NewFile);

#ifdef __linux
//代码转换:从一种编码转为另一种编码
int code_convert(const char* from_charset, const char* to_charset, char* inbuf, char* outbuf, size_t* outlen);
#endif

size_t mbstowcs_gbk(wchar_t* dest, char* src_gbk);

// 文件信息结构数组指针
typedef struct FileInfo* FileInfo;

struct FileInfo {
    char* name;
    unsigned int attrib;
    unsigned long size;
};

// 测试函数
void writeFileInfos(FILE* fout, const char* dirName);

// 循环目录下文件，调用操作函数
void operateDir(const char* dirName, void operateFile(void*, void*), void* ptr, bool recursive);

#endif