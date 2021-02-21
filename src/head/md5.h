#ifndef MD5_H
#define MD5_H

#include <stdbool.h>

#define MD5HashSize 16

#define MD5Hash

#ifdef MD5Hash
#define HashSize MD5HashSize
#define getHashFun ustrToMD5
#else
#define HashSize SHA1HashSize
#define getHashFun ustrToSHA1
#endif

// 取得字符串的md5
void ustrToMD5(unsigned char* md5, unsigned char* source);

// 比较相同长度的字节串是否相等(中间字节可为0值,因此不是字符串)
bool chars_equal(const char* dst, const char* src, int len);

// 取得hash的字符串表示
void hashToStr(char* str, unsigned char* hash, int length);

void testMD5_1(void);

// md5函数测试 如存在文件则取文件的md5，否则取字符串的md5
int testMD5_2(const char* argv);

#endif
