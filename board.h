#ifndef BOARD_H
#define BOARD_H

#include "base.h"

// FEN字符串转换成pieChars表示的棋盘局面, pieChars包含90个字符
void FENToPieChars(wchar_t *pieChars, wchar_t *FEN, int size);

// 使用一个字符串设置棋盘局面, pieChars包含90个字符
void setBoard(Board *board, wchar_t *pieChars);

// 取得棋盘局面的字符串表示, pieChars包含90个字符
void getPieceChars(wchar_t *pieChars, const Board *board);

// pieChars表示的棋盘局面转换成FEN字符串，返回FEN长度, pieChars包含90个字符
int pieCharsToFEN(wchar_t *pieChars, wchar_t *FEN);

// 某棋盘红黑定方向后某棋子可置入位置的集合，返回位置个数（至少90个）
int putSeats(Seat *seats, bool isBottom, const Piece *piece);

// 某棋盘局面下某位置的棋子可移入位置的集合，返回位置个数（至少17个）
int moveSeats(Seat *seats, const Board *board, const Seat *seat);

// 置入某棋盘内某行、某列位置一个棋子
void setPiece(Board *board, Seat *seat, const Piece *piece);

// 输出某棋盘局面的文本字符串
wchar_t *toString(wchar_t *wstr, Board *board);

// 测试本翻译单元各种对象、函数
void testBoard(void);

#endif