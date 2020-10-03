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
#else
//#ifdef WINVER
#include <conio.h>
#include <io.h>
#include <windows.h>
//printf("Windows\n");
#endif

// 文件信息结构数组指针
typedef struct FileInfo {
    char* name;
    unsigned int attrib;
    unsigned long size;
} * FileInfo;

typedef struct FileInfos {
    FileInfo* fis;
    int size;
    int count;
} * FileInfos;

bool isPrime(int n);
int getPrimes(int* primes, int bitCount);
// 取得大于等于size的质数
int getPrime(int size);

// 哈希函数
unsigned int BKDRHash_c(const char* src, int size);
unsigned int BKDRHash_w(const wchar_t* wstr);
unsigned int DJBHash(const wchar_t* wstr);
unsigned int SDBMHash(const wchar_t* wstr);

// 比较相同长度的字节串是否相等
bool chars_equal(const char* dst, const char* src, int len);

// 取得hash的字符串表示
void hashToStr(char* str, unsigned char* hash, int length);

// 设置结构内宽字符字段的值
wchar_t* setStruct_wstrField(wchar_t** pfield, const wchar_t* value);

// 根据给定的参数、对象、过滤函数和是否确认值，过滤掉给定数组中的对象
int filterObjects(void** objs, int count, void* arg1, void* arg2, bool (*filterFunc__)(void*, void*, void*), bool sure);

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

// 字符串连接，根据需要重新分配内存空间
void supper_wcscat(wchar_t** pwstr, size_t* size, const wchar_t* wstr);

// 从输入文件获取全部内容至宽字符串
wchar_t* getWString(FILE* fin);

// 从一个字符串中提取子字符串
void copySubStr(wchar_t* subStr, const wchar_t* srcWstr, int first, int last);
wchar_t* getSubStr(const wchar_t* srcWstr, int first, int last);

// 针对wchar_t的pcre包装函数
void* pcrewch_compile(const wchar_t* wstr, int n, const char** error, int* erroffset, const unsigned char* s);

/*
// 返回值 <0:表示匹配发生error，==0:没有匹配上，>0:返回匹配到的元素数量
ovector是一个int型数组，其长度必须设定为3的倍数，若为3n，则最多返回n个元素，显然有rc<=n
其中ovector[0],[1]为整个匹配上的字符串的首尾偏移；其他[2*i][2*i+1]为对应第i个匹配上的子串的偏移,
子串意思是正则表达式中被第i个()捕获的字符串，计数貌似是按照(出现的顺序。
如正则式/abc((.*)cf(exec))test/,在目标字符串11111abcword1cfexectest11111中匹配，将返回4个元素，其首尾偏移占用ovector的0~7位
元素0=abcword1cfexectest,
元素1=word1cfexec
元素2=word1
元素3=exec
ovector的最后1/3个空间，即[2n~3n-1]，貌似为pcre正则匹配算法预留，不返回结果
//*/
int pcrewch_exec(const void* reg, const void* p, const wchar_t* wstr, int len, int x, int y, int* ovector, int size);
void pcrewch_free(void* reg);

// 创建目录
int makeDir(const char* dirName);

// 复制文件
int copyFile(const char* SourceFile, const char* NewFile);

#ifdef __linux
//代码转换:从一种编码转为另一种编码
int code_convert(const char* from_charset, const char* to_charset, char* inbuf, char* outbuf, size_t* outlen);
#endif

// 循环目录下文件，调用操作函数
void operateDir(const char* dirName, void operateFile(void*, void*), void* ptr, bool recursive);

// 新建删除文件信息结构组
FileInfos newFileInfos(void);
void delFileInfos(FileInfos fileInfos);

// 提取目录下的文件信息
void getFileInfos(FileInfos fileInfos, const char* dirName, bool recursive);

// 测试函数
void writeFileInfos(FILE* fout, const char* dirName);

#endif