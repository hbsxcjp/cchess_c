#ifndef BOARD_H
#define BOARD_H

#include "base.h"
#include "piece.h"

// 棋盘行数
#define BOARDROW 10
// 棋盘列数
#define BOARDCOL 9
// 棋盘位置个数
#define SEATNUM (BOARDROW * BOARDCOL)

//=================================================================
//棋盘相关的类型
//=================================================================

// 棋盘变换类型
typedef enum {
    EXCHANGE,
    ROTATE,
    SYMMETRY
} ChangeType;

// 棋盘位置类型
typedef struct Seat* Seat;
// 一副棋盘结构类型
typedef struct Board* Board;

// 新建一个board
Board newBoard(void);

// 释放board
void freeBoard(Board board);

// 获取行值
int getRow_s(Seat seat);

// 获取列值
int getCol_s(Seat seat);

// 获取行列整数值
int getRowCol_s(Seat seat);
int getRow_rc(int rowcol);
int getCol_rc(int rowcol);

// 获取对称行值
int getOtherRow_s(Seat seat);

// 获取对称列值
int getOtherCol_s(Seat seat);

// 根据行、列值获取seat
Seat getSeat_rc(const Board board, int row, int col);

// 获取某棋盘内某行、某列位置的一个棋子
Piece getPiece_rc(const Board board, int row, int col);

// 根据seat获取棋子
Piece getPiece_s(const Board board, Seat seat);

// 置入某棋盘内某行、某列位置一个棋子
void setPiece_rc(Board board, int row, int col, Piece piece);

// 置入某棋盘内某位置一个棋子
void setPiece_s(Board board, Seat seat, Piece piece);

// FEN字符串转换成pieChars表示的棋盘局面, pieChars包含90个字符
wchar_t* setPieCharsFromFEN(wchar_t* pieChars, const wchar_t* FEN);

// 取得棋盘局面的字符串表示, pieChars包含90个字符
wchar_t* getPieChars_board(wchar_t* pieChars, const Board board);

// pieChars表示的棋盘局面转换成FEN字符串，返回FEN, pieChars包含90个字符
wchar_t* setFENFromPieChars(wchar_t* FEN, const wchar_t* pieChars);

// 棋盘复位
void resetBoard(Board board);

// 使用一个字符串设置棋盘局面, pieChars包含90个字符
void setBoard(Board board, const wchar_t* pieChars);

// 给定颜色是否在棋盘底边
bool isBottomSide(const Board board, PieceColor color);

// 设置棋盘底边的颜色
void setBottomColor(Board board);

// 取得某方将帅的位置seat
//Seat getKingSeat(const Board board, PieceColor color);

// 按给定参数查找活的棋子
int getLiveSeats(Seat* pseats, const Board board, PieceColor color,
    wchar_t name, int findCol, bool getStronge);

// 取得某方"兵"的棋子位置seats
int getSortPawnLiveSeats(Seat* pseats, const Board board, PieceColor color, wchar_t name);

// 某方棋子是否被将军
bool isKilled(const Board board, PieceColor color);

// 某方棋子是否已输棋
bool isDied(Board board, PieceColor color);

// 某棋盘红黑定方向后某种棋子可置入位置的集合，返回位置个数（至少90个）
int putSeats(Seat* pseats, const Board board, bool isBottom, PieceKind kind);

// 某棋盘局面下某位置的棋子“行走规则(筛除同色，未筛将军)”可移入位置的集合（至多17个）
int moveSeats(Seat* pseats, const Board board, Seat fseat);

// 某棋盘局面下某位置的棋子在“行走规则”基础上筛除被将军位置后可移入位置的集合（至多17个）
int canMoveSeats(Seat* pseats, int count, Board board, Seat fseat);

// 某棋盘局面下某位置的棋子在“行走规则”基础上，移动后将被将军位置的集合（至多17个）
int killedMoveSeats(Seat* pseats, int count, Board board, Seat fseat);

// 移动棋子，返回目的地棋子
Piece movePiece(Board board, Seat fseat, Seat tseat, Piece eatPiece);

// 按某种变换类型变换棋盘局面
void changeBoard(Board board, ChangeType ct);

// 取得表示位置字符串的名称
wchar_t* getSeatString(wchar_t* seatStr, const Board board, Seat seat);

// 输出某棋盘局面的文本字符串，长度小于1024
wchar_t* getBoardString(wchar_t* boardStr, const Board board);
// 棋盘上边标识字符串
wchar_t* getBoardPreString(wchar_t* preStr, const Board board);
// 棋盘下边标识字符串
wchar_t* getBoardSufString(wchar_t* sufStr, const Board board);

// 测试本翻译单元各种对象、函数
void testBoard(FILE* fout);

#endif