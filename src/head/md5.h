#ifndef MD5_H
#define MD5_H
#include <stdbool.h>

// 取得字符串的MD5值数组[16]
void getMD5(unsigned char* digest, char* source);

// 比较MD5值数组是否相等
bool isSame(unsigned char* src, unsigned char* des);

#endif
