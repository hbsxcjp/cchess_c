#ifndef MD5_H
#define MD5_H

#define MD5HashSize 16

// 取得字符串的MD5值数组[16]
unsigned char* getMD5(char* source);

// 取得md5的字符串表示
void md5ToStr(unsigned char* md5, char* str);

// 比较MD5值数组[16]是否相等
int md5cmp(const char* src, const char* des);

#endif
