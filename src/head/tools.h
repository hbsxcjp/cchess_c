#ifndef TOOLS_H
#define TOOLS_H

#include <stdbool.h>
#include <stdio.h>
#include <wchar.h>


// 去掉字符串前后的空白字符
char* trim(char* str);

// 去掉宽字符串前后的空白字符
wchar_t* wtrim(wchar_t* wstr);

// 从一个字符串中提取子字符串
void copySubStr(wchar_t* subStr, const wchar_t* srcWstr, int first, int last);

wchar_t* getSubStr(const wchar_t* srcWstr, int first, int last);

// 字符串连接，根据需要重新分配内存空间
void supper_wcscat(wchar_t** pwstr, size_t* size, const wchar_t* wstr);

// 用于遍历链表的打印字符串回调函数
void printWstr(wchar_t* wstr, FILE* fout, void* _0, void* _1);

// 设置结构内宽字符字段的值
wchar_t* setPwstr_value(wchar_t** pwstr, const wchar_t* value);

// 根据给定的参数、对象、过滤函数和函数结果是否确认值，过滤掉给定数组中的对象
int filterObjects(void** objs, int count, void* arg1, void* arg2,
    bool (*filterFunc__)(void*, void*, void*), bool sure);

#endif