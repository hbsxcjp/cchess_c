#ifndef PCRE_WCH_H
#define PCRE_WCH_H

#include "pcre.h"

// 针对wchar_t的pcre包装函数
void* pcrewch_compile(const wchar_t* wstr, int n, const char** error, int* erroffset, const unsigned char* s);

/*
    返回值 <0:表示匹配发生error，==0:没有匹配上，>0:返回匹配到的元素数量
    ovector是一个int型数组，其长度必须设定为3的倍数，若为3n，则最多返回n个元素，显然有rc<=n
    其中ovector[0],[1]为整个匹配上的字符串的首尾偏移；其他[2*i][2*i+1]为对应第i个匹配上的子串的偏移,
    子串意思是正则表达式中被第i个()捕获的字符串，计数貌似是按照(出现的顺序。
    如正则式/abc((.*)cf(exec))test/,在目标字符串11111abcword1cfexectest11111中匹配，将返回4个元素，其首尾偏移占用ovector的0~7位
    元素0=abcword1cfexectest,
    元素1=word1cfexec
    元素2=word1
    元素3=exec
    ovector的最后1/3个空间，即[2n~3n-1]，貌似为pcre正则匹配算法预留，不返回结果
*/
int pcrewch_exec(const void* reg, const void* p, const wchar_t* wstr, int len, int x, int y, int* ovector, int size);

int pcrewch_copy_substring(const wchar_t* wstr, int* ovector, int subStrCount, int subStrNum,
    wchar_t* subStr, int subSize);

void pcrewch_free(void* reg);
// 正则类 结束 -------------------------------------------------------------------------------------- //

#endif