#ifndef TOOLS_H
#define TOOLS_H

#include "base.h"

// 文件信息结构数组指针
typedef struct _finddata_t* FileInfo;
typedef struct FileInfos* FileInfos;

bool isPrime(int n);
int getPrimes(int* primes, int bitCount);
// 取得比size大的质数
int getPrime(int size);

// 哈希函数
unsigned int BKDRHash_c(char* src, int size);
unsigned int BKDRHash_s(const wchar_t* wstr);
unsigned int DJBHash(const wchar_t* wstr);
unsigned int SDBMHash(const wchar_t* wstr);

// 去掉字符串前后的空白字符
char* trim(char* str);

// 去掉宽字符串前后的空白字符
wchar_t* wtrim(wchar_t* wstr);

// 从文件名提取目录名
char* getDirName(char* filename);
// 从文件名提取纯文件名，去掉目录
char* getFileName(char* filename);
// 从文件名提取扩展名
const char* getExtName(const char* filename);
// 转换文件的扩展名
char* transFileExtName(char* filename, const char* extname);

// 从文件当前指针至尾部获取宽字符串
wchar_t* getWString(FILE* fin);

// 字符串连接，根据需要重新分配内存空间
void writeWString(wchar_t** pstr, int* size, const wchar_t* wstr);

// 复制文件
int copyFile(const char* SourceFile, const char* NewFile);
// 循环目录下文件，调用操作函数
void operateDir(const char* dirName, void operateFile(FileInfo, void*), void* ptr, bool recursive);

// 新建删除文件信息结构组
FileInfos newFileInfos(void);
void delFileInfos(FileInfos fileInfos);
// 提取目录下的文件信息
void getFileInfos(FileInfos fileInfos, const char* dirName, bool recursive);

// 测试函数
void testTools(FILE* fout, const char** chessManualDirName, int size, const char* ext);

#endif