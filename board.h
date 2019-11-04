#ifndef BOARD_H
#define BOARD_H

#include "base.h"

// FEN字符串转换成pieChars表示的棋盘局面, pieChars包含90个字符
wchar_t* getPieChars_F(wchar_t* FEN, size_t n);

// 置入某棋盘内某行、某列位置一个棋子
inline void setPiece(Board* board, Seat* seat, const Piece* piece);

// 使用一个字符串设置棋盘局面, pieChars包含90个字符
bool setBoard(Board* board, wchar_t* pieChars);

// 取得某方将帅的位置seat
Seat getKingSeat(const Board* board, PieceColor color);

// 取得某方全部活的棋子位置seats
int getLiveSeats(Seat* seats, size_t n, const Board* board, PieceColor color);

// 取得某方"兵"的棋子位置seats
int getSortPawnLiveSeats(Seat* seats, size_t n, const Board* board, PieceColor color);

// 某方棋子是否被将军
bool isKilled(const Board* board, PieceColor color);

// 某方棋子是否被将死
bool isDied(const Board* board, PieceColor color);

// 按某种变换类型变换棋盘局面
bool changeSide(const Board* board, ChangeType ct);

// 取得棋盘局面的字符串表示, pieChars包含90个字符
wchar_t* getPieChars_B(const Board* board);

// pieChars表示的棋盘局面转换成FEN字符串，返回FEN长度, pieChars包含90个字符
wchar_t* getFEN(const wchar_t* pieChars);

// 某棋盘红黑定方向后某棋子可置入位置的集合，返回位置个数（至少90个）
int putSeats(Seat* seats, bool isBottom, const Piece* piece);

// 某棋盘局面下某位置的棋子可移入位置的集合，返回位置个数（至少17个）
int moveSeats(Seat* seats, const Board* board, const Seat* seat);

// 根据中文着法取得内部着法表示
bool getMove(const Board* board, const wchar_t* zhStr, size_t n, Move* move);

// 根据内部着法表示取得中文着法
wchar_t* getZhStr(const Board* board, size_t n, const Move* move);

// 输出某棋盘局面的文本字符串，长度
wchar_t* getBoardString(const Board* board, size_t n);

// 测试本翻译单元各种对象、函数
wchar_t* testBoard(void);

#endif