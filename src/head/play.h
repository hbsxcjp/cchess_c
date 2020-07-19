#ifndef PLAY_H
#define PLAY_H

#include "chessManual.h"

// 添加/删除一个info条目
void addInfoItem(ChessManual cm, const wchar_t* name, const wchar_t* value);
void delInfoItem(ChessManual cm, const wchar_t* name);

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


#endif