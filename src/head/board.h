#ifndef BOARD_H
#define BOARD_H

#include "base.h"

// 根据行、列值获取seat
#define getSeat_rc(row, col) ((row) << 4 | (col))
// 获取行值
#define getRow_s(seat) ((seat) >> 4)
// 获取列值
#define getCol_s(seat) ((seat)&0x0F)
// 根据seat获取棋子
#define getPiece_s(board, seat) ((board)->pieces[(seat)])
// 在seat位置设置棋子
#define setPiece_s(board, seat, piece) ((board)->pieces[(seat)] = (piece))

// 新建一个board
Board* newBoard(void);

// FEN字符串转换成pieChars表示的棋盘局面, pieChars包含90个字符
wchar_t* getPieChars_F(wchar_t* pieChars, wchar_t* FEN, size_t n);

// 取得棋盘局面的字符串表示, pieChars包含90个字符
wchar_t* getPieChars_B(wchar_t* pieChars, const Board* board);

// pieChars表示的棋盘局面转换成FEN字符串，返回FEN长度, pieChars包含90个字符
wchar_t* getFEN(wchar_t* FEN, const wchar_t* pieChars);

// 使用一个字符串设置棋盘局面, pieChars包含90个字符
void setBoard(Board* board, const wchar_t* pieChars);

// 设置棋盘底边的颜色
void setBottomColor(Board* board);

// 给定颜色是否在棋盘底边
bool isBottomSide(const Board* board, PieceColor color);

// 取得某方将帅的位置seat
Seat getKingSeat(const Board* board, PieceColor color);

// 取得某方全部活的棋子位置seats
int getLiveSeats(Seat* pseats, size_t n, const Board* board, PieceColor color,
    const wchar_t name, const int findCol, bool getStronge);
// 当name == L'\x0', findCol == -1, getStronge == false时，查找全部棋子；
// 否则，按给定参数查找棋子。

// 取得某方"兵"的棋子位置seats
int getSortPawnLiveSeats(Seat* pseats, const Board* board, PieceColor color, wchar_t name);

// 某方棋子是否被将军
bool isKilled(Board* board, PieceColor color);

// 某方棋子是否已输棋
bool isDied(Board* board, PieceColor color);

// 某棋盘红黑定方向后某种棋子可置入位置的集合，返回位置个数（至少90个）
int putSeats(Seat* pseats, bool isBottom, PieceKind kind);

// 某棋盘局面下某位置的棋子“行走规则(筛除同色，未筛将军)”可移入位置的集合（至多17个）
int moveSeats(Seat* pseats, Board* board, Seat fseat);

// 某棋盘局面下某位置的棋子在“行走规则”基础上筛除被将军位置后可移入位置的集合（至多17个）
int getMoveSeats(Seat* pseats, int count, Board* board, Seat fseat);

// 移动棋子，返回目的地棋子
Piece moveTo(Board* board, Seat fseat, Seat tseat, Piece eatPiece);

// 按某种变换类型变换棋盘局面
void changeBoardSide(Board* board, ChangeType ct);

// 输出某棋盘局面的文本字符串，长度小于1024
wchar_t* getBoardString(wchar_t* boardStr, const Board* board);

// 测试本翻译单元各种对象、函数
void testBoard(FILE* fout);

#endif