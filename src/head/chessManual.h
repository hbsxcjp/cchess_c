#ifndef CHESSMANUAL_H
#define CHESSMANUAL_H

#include "base.h"

// 新建chessManual
ChessManual newChessManual(const char* filename);

// 从文件读取重置chessManual
ChessManual resetChessManual(ChessManual* cm, const char* filename);

// 删除chessManual
void delChessManual(ChessManual cm);

// 添加/删除一个info条目
void addInfoItem(ChessManual cm, const wchar_t* name, const wchar_t* value);
void delInfoItem(ChessManual cm, const wchar_t* name);

// 将PGN_CC格式的move、remark信息写入字符串
void writeMove_PGN_CCtoWstr(wchar_t** pmoveStr, ChessManual cm);
void writeRemark_PGN_CCtoWstr(wchar_t** premStr, ChessManual cm);

// 从chessManual存储到文件
void writeChessManual(ChessManual cm, const char* filename);

// 前进一步
void go(ChessManual cm);
// 前进到变着
void goOther(ChessManual cm);
void goEnd(ChessManual cm);
// 前进至指定move
void goTo(ChessManual cm, Move move);

// 后退一步
void back(ChessManual cm);
void backNext(ChessManual cm);
void backOther(ChessManual cm);
void backFirst(ChessManual cm);
// 后退至指定move
void backTo(ChessManual cm, Move move);

// 前进数步
void goInc(ChessManual cm, int inc);

// 转变棋局实例
void changeChessManual(ChessManual cm, ChangeType ct);

// 某着从头至尾的着法图示
void writeAllMoveStr(FILE* fout, ChessManual cm, const Move amove);

// 转换棋局存储格式
void transDir(const char* dirfrom, RecFormat fmt);
// 批量转换目录的存储格式
void testTransDir(int fromDir, int toDir,
    int fromFmtStart, int fromFmtEnd, int toFmtStart, int toFmtEnd);

// 测试本翻译单元各种对象、函数
void testChessManual(FILE* fout);

#endif