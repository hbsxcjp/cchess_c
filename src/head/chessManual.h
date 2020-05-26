#ifndef CHESSMANUAL_H
#define CHESSMANUAL_H

#include "base.h"
#include "sqlite3.h"

// 新建chessManual
ChessManual newChessManual(const char* fileName);

// 从文件读取重置chessManual
ChessManual resetChessManual(ChessManual* cm, const char* fileName);

// 删除chessManual
void delChessManual(ChessManual cm);

// 添加/删除一个info条目
void addInfoItem(ChessManual cm, const wchar_t* name, const wchar_t* value);
void delInfoItem(ChessManual cm, const wchar_t* name);

// 将PGN_CC格式的move、remark信息写入字符串
void writeMove_PGN_CCtoWstr(wchar_t** pmoveStr, ChessManual cm);
void writeRemark_PGN_CCtoWstr(wchar_t** premStr, ChessManual cm);

// 从chessManual存储到文件
void writeChessManual(ChessManual cm, const char* fileName);

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
// 前进或后退数步
void goInc(ChessManual cm, int inc);

// 转变棋局实例
void changeChessManual(ChessManual cm, ChangeType ct);
// 某着从头至尾的着法图示
void writeAllMoveStr(FILE* fout, ChessManual cm, const Move amove);

// 存储文件到局面记录库
void appendAspects_file(Aspects aspects, const char* fileName); // 使用void*参数，目的是使其可以作为operateDir调用的函数参数
// 批量存储目录到局面记录库
void appendAspects_dir(Aspects aspects, const char* dirName);

// 批量转换棋局存储格式
void transDir(const char* dirName, RecFormat fromfmt, RecFormat tofmt);
// 批量转换目录的存储格式
void testTransDir(const char** chessManualDirName, int size, int toDir, int fmtEnd, int toFmtEnd);

// 测试本翻译单元各种对象、函数
void testChessManual(FILE* fout);

#endif