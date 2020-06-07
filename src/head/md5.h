#ifndef MD5_H
#define MD5_H

#define MD5HashSize 16

// 取得字符串的md5
void getMD5(unsigned char* md5, char* source);

void testMD5_1(void);

// md5函数测试 如存在文件则取文件的md5，否则取字符串的md5
int testMD5_2(const char* argv);

#endif
