#ifndef INSTANCE_H
#define INSTANCE_H

#include "base.h"

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

// 重置instance到初始状态
void reset(Instance* ins);

const wchar_t* moveInfo(wchar_t* str, size_t n, const Instance* ins);

void changeInstanceSide(Instance* ins, ChangeType ct);

// 测试本翻译单元各种对象、函数
void testInstance(FILE* fout);

#endif