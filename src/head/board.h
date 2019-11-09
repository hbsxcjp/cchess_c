#ifndef BOARD_H
#define BOARD_H

#include "base.h"

// 新建一个board
Board* newBoard(void);

// 根据seat获取const seat*
const Seat* getSeat_s(const Seat seat);

// FEN字符串转换成pieChars表示的棋盘局面, pieChars包含90个字符
wchar_t* getPieChars_F(wchar_t* pieChars, wchar_t* FEN, size_t n);

// 取得棋盘局面的字符串表示, pieChars包含90个字符
wchar_t* getPieChars_B(wchar_t* pieChars, const Board* board);

// pieChars表示的棋盘局面转换成FEN字符串，返回FEN长度, pieChars包含90个字符
wchar_t* getFEN(wchar_t* FEN, const wchar_t* pieChars);

// 使用一个字符串设置棋盘局面, pieChars包含90个字符
void setBoard(Board* board, wchar_t* pieChars);

// 设置棋盘底边的颜色
void setBottomColor(Board* board);

// 给定颜色是否在棋盘底边
bool isBottomSide(const Board* board, PieceColor color);

// 取得某方将帅的位置seat
const Seat* getKingSeat(const Board* board, PieceColor color);

// 取得某方全部活的棋子位置seats
int getLiveSeats(const Seat** pseats, size_t n, const Board* board, PieceColor color,
    const wchar_t name, const int findCol, bool getStronge);
// 当name == L'\x0', findCol == -1, getStronge == false时，查找全部棋子；
// 否则，按给定参数查找棋子。

// 某方棋子是否被将军
bool isKilled(Board* board, PieceColor color);

// 某方棋子是否已输棋
bool isDied(Board* board, PieceColor color);

// 某棋盘红黑定方向后某种棋子可置入位置的集合，返回位置个数（至少90个）
int putSeats(const Seat** pseats, bool isBottom, PieceKind kind);

// 某棋盘局面下某位置的棋子“行走规则(筛除同色，未筛将军)”可移入位置的集合（至多17个）
int moveSeats(const Seat** pseats, Board* board, const Seat* fseat);

// 某棋盘局面下某位置的棋子在“行走规则”基础上筛除被将军位置后可移入位置的集合（至多17个）
int getMoveSeats(const Seat** pseats, int count, Board* board, const Seat* fseat);

// 移动棋子，返回目的地棋子
const Piece* seatMoveTo(Board* board, const Seat* fseat, const Seat* tseat, const Piece* eatPiece);

// 取得某方"兵"的棋子位置seats
int getSortPawnLiveSeats(const Seat** pseats, size_t n, const Board* board, PieceColor color); //?

// 根据中文着法取得内部着法表示
bool getMove(Move* move, const Board* board, const wchar_t* zhStr, size_t n); //?

// 根据内部着法表示取得中文着法
wchar_t* getZhStr(wchar_t* zhStr, size_t n, const Board* board, const Move* move); //?

// 按某种变换类型变换棋盘局面
bool changeBoardSide(Board* board, ChangeType ct); //?

// 输出某棋盘局面的文本字符串，长度小于1024
wchar_t* getBoardString(wchar_t* boardStr, const Board* board);

// 测试本翻译单元各种对象、函数
void testBoard(FILE* fout);

#endif