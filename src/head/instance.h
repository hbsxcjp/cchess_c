#ifndef INSTANCE_H
#define INSTANCE_H

#include "base.h"

// 新建instance
Instance* newInstance(void);

// 删除instance
void delInstance(Instance* ins);

// 添加一个info条目
void addInfoItem(Instance* ins, wchar_t* name, wchar_t* value);

// 从文件读取到instance
void read(Instance* ins, const char* filename);

// 从instance存储到文件
void write(const Instance* ins, const char* filename);

// 前进一步
void go(Instance* ins);

// 后退一步
void back(Instance* ins);

// 后退至指定move
void backTo(Instance* ins, const Move* move);

// 前进到变着
void goOther(Instance* ins);

// 前进数步
void goInc(Instance* ins, int inc);

// 转换棋局实例
void changeInstanceSide(Instance* ins, ChangeType ct);

// 测试本翻译单元各种对象、函数
void testInstance(FILE* fout);

#endif