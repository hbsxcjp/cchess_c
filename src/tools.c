#include "head/tools.h"
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>

bool isPrime(int n)
{
    if (n <= 3)
        return n > 1;
    for (int i = 2, sn = sqrt(n); i <= sn; i++)
        if (n % i == 0)
            return false;
    return true;
}

int getPrimes(int* primes, int bitCount)
{
    int index = 0;
    for (int i = 1; i < bitCount; ++i) {
        int n = (1 << i) - 1;
        while (n > 1 && !isPrime(n))
            n--;
        primes[index++] = n;
    }
    return index;
}
/*
    int primes[32];
    int count = getPrimes(primes, 32);
    for (int i = 0; i < count; i++)
        fwprintf(fout, L"%d, ", primes[i]);
//*/

int getPrime(int size)
{
    //static int primes[] = { 509, 509, 1021, 2053, 4093, 8191, 16381, 32771, 65521, INT_MAX };
    static int primes[] = { 61, 127, 251, 509, 1021, 2039, 4093, 8191, 16381, 32749, 65521, //3, 7, 13, 31,
        131071, 262139, 524287, 1048573, 2097143, 4194301, 8388593, 16777213, 33554393,
        67108859, 134217689, 268435399, 536870909, 1073741789, INT32_MAX };
    int i = 0;
    while (primes[i] < size)
        i++;
    return primes[i];
}

// BKDR Hash Function

unsigned int BKDRHash_c(const char* src, int size)
{
    unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
    unsigned int hash = 0;

    for (int i = 0; i < size; ++i)
        hash = hash * seed + src[i];

    return (hash & 0x7FFFFFFF);
}

unsigned int BKDRHash_w(const wchar_t* wstr)
{
    /*
    int len = wcstombs(NULL, wstr, 0)+1;
    char str[len];
    wcstombs(str, wstr, len);
    return BKDRHash_c(str, strlen(str));
    //*/

    //*
    unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
    unsigned int hash = 0;

    while (*wstr) {
        hash = hash * seed + (*wstr++);
    }

    return (hash & 0x7FFFFFFF);
    //*/
}

unsigned int DJBHash(const wchar_t* wstr)
{
    unsigned int hash = 5381;
    int len = wcslen(wstr);
    for (int i = 0; i < len; i++) {
        hash = ((hash << 5) + hash) + wstr[i];
    }
    return hash;
}

unsigned int SDBMHash(const wchar_t* wstr)
{
    unsigned int hash = 0;
    int len = wcslen(wstr);
    for (int i = 0; i < len; i++) {
        hash = (hash << 5) + (hash << 16) - hash + wstr[i];
    }
    return hash;
}

bool chars_equal(const char* dst, const char* src, int len)
{
    for (int i = 0; i < len; ++i)
        if (dst[i] != src[i])
            return false;
    return true;
}

void hashToStr(char* str, unsigned char* hash, int length)
{
    str[0] = '\x0';
    char tmpStr[3];
    for (int i = 0; i < length; i++) {
        sprintf(tmpStr, "%02x", hash[i]);
        strcat(str, tmpStr);
    }
}

char* trim(char* str)
{
    size_t first = 0, last = strlen(str);
    while (isspace(str[--last]))
        str[last] = '\x0';
    while (first < last && isspace(str[first]))
        ++first;
    return str + first;
}

wchar_t* wtrim(wchar_t* wstr)
{
    size_t first = 0, last = wcslen(wstr);
    while (iswspace(wstr[--last]))
        wstr[last] = '\x0';
    while (first < last && iswspace(wstr[first]))
        ++first;
    return wstr + first;
}

void copySubStr(wchar_t* subStr, const wchar_t* srcWstr, int first, int last)
{
    int len = last - first;
    wcsncpy(subStr, srcWstr + first, len);
    subStr[len] = L'\x0';
}

wchar_t* getSubStr(const wchar_t* srcWstr, int first, int last)
{
    wchar_t* subStr = NULL;
    if (last - first > 0) {
        subStr = malloc((last - first + 1) * sizeof(wchar_t));
        assert(subStr);
        copySubStr(subStr, srcWstr, first, last);
    }
    return subStr;
}

void supper_wcscat(wchar_t** pwstr, size_t* size, const wchar_t* wstr)
{
    // 如字符串分配的长度不够，则增加长度
    if (*size < wcslen(*pwstr) + wcslen(wstr) + 1) {
        *size = *size * 2 + wcslen(wstr) + 1;
        *pwstr = realloc(*pwstr, *size * sizeof(wchar_t));
        assert(*pwstr);
    }

    wcscat(*pwstr, wstr);
}

void printWstr(wchar_t* wstr, FILE* fout, void* _0, void* _1)
{
    fwprintf(fout, L"%ls\n", wstr);
}

wchar_t* setPwstr_value(wchar_t** pwstr, const wchar_t* value)
{
    free(*pwstr);
    *pwstr = NULL;
    if (value) {
        *pwstr = malloc((wcslen(value) + 1) * sizeof(wchar_t));
        wcscpy(*pwstr, value);
    }
    return *pwstr;
}

int filterObjects(void** objs, int count, void* arg1, void* arg2,
    bool (*filterFunc__)(void*, void*, void*), bool sure)
{
    int index = 0;
    while (index < count) {
        // 如符合条件，先减少count再比较序号，如小于则需要交换；如不小于则指向同一元素，不需要交换；
        if (filterFunc__(arg1, arg2, objs[index]) == sure && index < --count) {
            void* tempObj = objs[count];
            objs[count] = objs[index];
            objs[index] = tempObj;
        } else
            ++index; // 不符合筛选条件，index前进一个
    }
    return count; // 以返回的count分界，大于count的对象，均为已过滤对象
}