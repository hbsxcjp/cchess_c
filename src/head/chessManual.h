#ifndef CHESSMANUAL_H
#define CHESSMANUAL_H

#include "base.h"

// 新建chessManual
ChessManual newChessManual(const char* fileName);

// 从文件读取重置chessManual
ChessManual resetChessManual(ChessManual* cm, const char* fileName);

// 删除chessManual
void delChessManual(ChessManual cm);

Move getRootMove(ChessManual cm);

// 取得正则表达式所需的中文字符组
wchar_t* getZhWChars(wchar_t* ZhWChars);

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

// 添加着法
void appendMove(ChessManual cm, const wchar_t* wstr, RecFormat fmt, wchar_t* remark, bool isOther);

// 添加/删除一个info条目
void addInfoItem(ChessManual cm, const wchar_t* name, const wchar_t* value);
void delInfoItem(ChessManual cm, const wchar_t* name);

// 增删改move后，更新zhStr、行列数值
void setMoveNumZhStr(ChessManual cm);

// 将PGN_CC格式的info、move、remark信息写入字符串
void writeInfo_PGNtoWstr(wchar_t** pinfoStr, ChessManual cm);

void writeMove_PGN_CC(wchar_t* moveStr, int colNum, CMove rootMove);
void writeRemark_PGN_CC(wchar_t** premarkStr, size_t* premSize, CMove rootMove);

void writeMove_PGN_CCtoWstr(wchar_t** pmoveStr, ChessManual cm);
void writeRemark_PGN_CCtoWstr(wchar_t** premStr, ChessManual cm);
void writePGN_CCtoWstr(wchar_t** pStr, ChessManual cm);

// 从chessManual存储到文件，根据文件扩展名选择存储格式
void writeChessManual(ChessManual cm, const char* fileName);

// 遍历每个着法
void moveMap(ChessManual cm, void apply(Move, Board, void*), void* ptr);

// 转变棋局实例
void changeChessManual(ChessManual cm, ChangeType ct);

// 批量转换棋局存储格式
void transDir(const char* dirName, RecFormat fromfmt, RecFormat tofmt, bool isPrint);

// 取得棋谱有关的数据
void getChessManualNumStr(char* str, ChessManual cm);

// 调试：取得棋谱的着法字符串(iccs)
const char* getIccsStr(char* iccsStr, ChessManual cm);

bool chessManual_equal(ChessManual cm0, ChessManual cm1);

#endif