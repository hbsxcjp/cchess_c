#ifndef TOOLS_H
#define TOOLS_H

#include "base.h"
#include <iconv.h>

#ifdef __linux
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
//printf("Linux\n");
#endif
#ifdef __sun
#ifdef __sparc
printf("Sun SPARC\n");
#else
printf("Sun X86\n");
#endif
#endif
#ifdef _AIX
printf("AIX\n");
#endif
#ifdef WINVER
#include <conio.h>
#include <io.h>
#include <windows.h>
//printf("Windows\n");
#endif

// 文件信息结构数组指针
//typedef struct _finddata_t* FileInfo;
typedef struct FileInfo* FileInfo;
typedef struct FileInfos* FileInfos;

bool isPrime(int n);
int getPrimes(int* primes, int bitCount);
// 取得大于等于size的质数
int getPrime(int size);

// 哈希函数
unsigned int BKDRHash_c(const char* src, int size);
unsigned int BKDRHash_s(const wchar_t* wstr);
unsigned int DJBHash(const wchar_t* wstr);
unsigned int SDBMHash(const wchar_t* wstr);

// 比较相同长度的字节串是否相等
bool charIsSame(const char* dst, const char* src, int len);

// 取得hash的字符串表示
void hashToStr(char* str, unsigned char* hash, int length);

// 去掉字符串前后的空白字符
char* trim(char* str);

// 去掉宽字符串前后的空白字符
wchar_t* wtrim(wchar_t* wstr);

// 从文件名提取目录名
void getDirName(char* dirName, const char* fileName);
// 从文件名提取纯文件名，去掉目录
const char* getFileName(const char* filename);
// 从文件名提取扩展名
const char* getExtName(const char* filename);
// 转换文件的扩展名
void transFileExtName(char* filename, const char* extname);

// 从文件当前指针至尾部获取宽字符串
wchar_t* getWString(FILE* fin);

// 字符串连接，根据需要重新分配内存空间
void appendWString(wchar_t** pstr, size_t* size, const wchar_t* wstr);

// 创建目录
int makeDir(const char* dirName);

// 复制文件
int copyFile(const char* SourceFile, const char* NewFile);

//代码转换:从一种编码转为另一种编码
int code_convert(const char* from_charset, const char* to_charset, char* inbuf, char* outbuf);

// 循环目录下文件，调用操作函数
void operateDir(const char* dirName, void operateFile(FileInfo, void*), void* ptr, bool recursive);

// 新建删除文件信息结构组
FileInfos newFileInfos(void);
void delFileInfos(FileInfos fileInfos);
// 提取目录下的文件信息
void getFileInfos(FileInfos fileInfos, const char* dirName, bool recursive);
void getFileInfoName(char* fileName, FileInfo fileInfo);

// 测试函数
void testTools(FILE* fout, const char** chessManualDirName, int size, const char* ext);

#endif