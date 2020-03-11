#ifndef CHESSMANUAL_H
#define CHESSMANUAL_H

#include "base.h"

// 新建chessManual
PChessManual newChessManual(const char* filename);

// 从文件读取重置chessManual
PChessManual resetChessManual(PChessManual* cm, const char* filename);

// 删除chessManual
void delChessManual(PChessManual cm);

// 添加一个info条目
void addInfoItem(PChessManual cm, const wchar_t* name, const wchar_t* value);

// 将PGN_CC格式的moves信息写入字符串
void writeMove_PGN_CCtoWstr(const PChessManual cm, wchar_t** plineStr);

// 将PGN_CC格式的remark信息写入字符串
void writeRemark_PGN_CCtoWstr(const PChessManual cm, wchar_t** premarkStr);

// 从chessManual存储到文件
void writeChessManual(const PChessManual cm, const char* filename);

// 取得先手方颜色
PieceColor getFirstColor(const PChessManual cm);

// 是否开始
bool isStart(const PChessManual cm);

// 当前着法有无后着
bool hasNext(const PChessManual cm);

// 当前着法有无前着
bool hasPre(const PChessManual cm);

// 当前着法有无变着
bool hasOther(const PChessManual cm);

// 当前着法有无前变
bool hasPreOther(const PChessManual cm);

// 前进一步
void go(PChessManual cm);
// 前进到变着
void goOther(PChessManual cm);
void goEnd(PChessManual cm);
// 前进至指定move
void goTo(PChessManual cm, PMove move);

// 后退一步
void back(PChessManual cm);
void backNext(PChessManual cm);
void backOther(PChessManual cm);
void backFirst(PChessManual cm);
// 后退至指定move
void backTo(PChessManual cm, PMove  move);

// 前进数步
void goInc(PChessManual cm, int inc);

// 转变棋局实例
void changeChessManual(PChessManual cm, ChangeType ct);
RecFormat getRecFormat(const char* ext);
// 转换棋局存储格式
void transDir(const char* dirfrom, RecFormat fmt);

// 批量转换目录的存储格式
void testTransDir(int fromDir, int toDir,
    int fromFmtStart, int fromFmtEnd, int toFmtStart, int toFmtEnd);

// 测试本翻译单元各种对象、函数
void testChessManual(FILE* fout);

#endif