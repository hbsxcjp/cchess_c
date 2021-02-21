#include "head/tools.h"
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>


char* trim(char* str)
{
    size_t first = 0, last = strlen(str);
    while (first < last && isspace(str[first]))
        ++first;
    while (first < last && isspace(str[last - 1]))
        str[--last] = '\x0';
    return str + first;
}

wchar_t* wtrim(wchar_t* wstr)
{
    size_t first = 0, last = wcslen(wstr);
    while (first < last && iswspace(wstr[first]))
        ++first;
    while (first < last && iswspace(wstr[last - 1]))
        wstr[--last] = L'\x0';
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