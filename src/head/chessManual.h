#ifndef CHESSMANUAL_H
#define CHESSMANUAL_H

#include "base.h"

// 新建chessManual
ChessManual* newChessManual(void);

// 删除chessManual
void delChessManual(ChessManual* cm);

// 添加一个info条目
void addInfoItem(ChessManual* cm, const wchar_t* name, const wchar_t* value);

// 增删改move后，更新ins、move的行列数值
void setMoveNums(ChessManual* cm, Move* move);

// 从文件读取到chessManual
ChessManual* readChessManual(ChessManual* cm, const char* filename);

// 将PGN_CC格式的moves信息写入字符串
void writeMove_PGN_CCtoWstr(ChessManual* cm, wchar_t** plineStr);

// 将PGN_CC格式的remark信息写入字符串
void writeRemark_PGN_CCtoWstr(ChessManual* cm, wchar_t** premarkStr);

// 从chessManual存储到文件
void writeChessManual(ChessManual* cm, const char* filename);

// 是否开始
bool isStart(const ChessManual* cm);

// 当前着法有无后着
bool hasNext(const ChessManual* cm);

// 当前着法有无前着
bool hasPre(const ChessManual* cm);

// 当前着法有无变着
bool hasOther(const ChessManual* cm);

COORD getMoveCoord(PMove move);

wchar_t* getMoveStr(wchar_t* wstr, PMove move);

// 前进一步
void go(ChessManual* cm);
void goEnd(ChessManual* cm);

// 后退一步
void back(ChessManual* cm);
void backFirst(ChessManual* cm);

// 后退至指定move
void backTo(ChessManual* cm, Move* move);

// 前进到变着
void goOther(ChessManual* cm);

// 前进数步
void goInc(ChessManual* cm, int inc);

// 转变棋局实例
void changeChessManual(ChessManual* cm, ChangeType ct);

// 转换棋局存储格式
void transDir(const char* dirfrom, RecFormat fmt);

// 批量转换目录的存储格式
void testTransDir(int fromDir, int toDir,
    int fromFmtStart, int fromFmtEnd, int toFmtStart, int toFmtEnd);

// 测试本翻译单元各种对象、函数
void testChessManual(FILE* fout);

#endif