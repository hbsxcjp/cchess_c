#ifndef INSTANCE_H
#define INSTANCE_H

#include "base.h"

// 新建instance
Instance* newInstance(void);

// 删除instance
void delInstance(Instance* ins);

// 添加一个info条目
void addInfoItem(Instance* ins, const wchar_t* name, const wchar_t* value);

// 增删改move后，更新ins、move的行列数值
void setMoveNums(Instance* ins, Move* move);

// 从文件读取到instance
Instance* readInstance(Instance*, const char* filename);

// 将PGN_CC格式ins的moves信息写入字符串
void writeMove_PGN_CCtoWstr(Instance* ins, wchar_t** plineStr);

// 将PGN_CC格式ins的remark信息写入字符串
void writeRemark_PGN_CCtoWstr(Instance* ins, wchar_t** premarkStr);

// 从instance存储到文件
void writeInstance(Instance* ins, const char* filename);

// 是否开始
bool isStart(const Instance* ins);

// 当前着法有无后着
bool hasNext(const Instance* ins);

// 当前着法有无前着
bool hasPre(const Instance* ins);

// 当前着法有无变着
bool hasOther(const Instance* ins);

// 前进一步
void go(Instance* ins);

// 后退一步
void back(Instance* ins);

// 后退至指定move
void backTo(Instance* ins, Move* move);

// 前进到变着
void goOther(Instance* ins);

// 前进数步
void goInc(Instance* ins, int inc);

// 转变棋局实例
void changeInstance(Instance* ins, ChangeType ct);

// 转换棋局存储格式
void transDir(const char* dirfrom, RecFormat fmt);

// 批量转换目录的存储格式
void testTransDir(int fromDir, int toDir,
    int fromFmtStart, int fromFmtEnd, int toFmtStart, int toFmtEnd);

// 测试本翻译单元各种对象、函数
void testInstance(FILE* fout);

#endif