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
// 棋子数组长度
//#define BOARDLEN 0x99

//=================================================================
//棋盘相关的类型
//=================================================================

// 棋子马的移动方向
typedef enum {
    SW,
    SE,
    NW,
    NE,
    WS,
    ES,
    WN,
    EN
} MoveDirection;

// 棋盘位置类型
typedef int Seat;

// 一副棋盘结构类型
typedef struct _Board {
    //Piece pieces[BOARDLEN]; // 位置0x00, 高四位表示行，低四位表示列
    Piece pieces[SEATNUM];
    PieceColor bottomColor;
} Board, *PBoard;

// 棋盘变换类型
typedef enum {
    EXCHANGE,
    ROTATE,
    SYMMETRY
} ChangeType;

// 根据行、列值获取seat
Seat getSeat_rc(int row, int col);

// 获取行值
int getRow_s(Seat seat);

// 获取列值
int getCol_s(Seat seat);

// 获取对称行值
int getOtherRow_s(Seat seat);

// 获取对称列值
int getOtherCol_s(Seat seat);

// 根据seat获取棋子
Piece getPiece_s(const PBoard board, Seat seat);

// 获取某棋盘内某行、某列位置的一个棋子
Piece getPiece_rc(const PBoard board, int row, int col);

// 置入某棋盘内某行、某列位置一个棋子
void setPiece_rc(PBoard board, int row, int col, Piece piece);

// 新建一个board
PBoard newBoard(void);

// FEN字符串转换成pieChars表示的棋盘局面, pieChars包含90个字符
wchar_t* setPieCharsFromFEN(wchar_t* pieChars, const wchar_t* FEN, size_t n);

// 取得棋盘局面的字符串表示, pieChars包含90个字符
wchar_t* getPieChars_board(wchar_t* pieChars, const PBoard board);

// pieChars表示的棋盘局面转换成FEN字符串，返回FEN, pieChars包含90个字符
wchar_t* setFENFromPieChars(wchar_t* FEN, const wchar_t* pieChars);

// 使用一个字符串设置棋盘局面, pieChars包含90个字符
void setBoard(PBoard board, const wchar_t* pieChars);

// 设置棋盘底边的颜色
void setBottomColor(PBoard board);

// 给定颜色是否在棋盘底边
bool isBottomSide(const PBoard board, PieceColor color);

// 取得某方将帅的位置seat
Seat getKingSeat(const PBoard board, PieceColor color);

// 取得某方全部活的棋子位置seats
// 当name == L'\x0', findCol == -1, getStronge == false时，查找全部棋子；
// 否则，按给定参数查找棋子。
int getLiveSeats(Seat* pseats, size_t n, const PBoard board, PieceColor color,
    wchar_t name, int findCol, bool getStronge);

// 取得某方"兵"的棋子位置seats
int getSortPawnLiveSeats(Seat* pseats, const PBoard board, PieceColor color, wchar_t name);

// 某方棋子是否被将军
bool isKilled(const PBoard board, PieceColor color);

// 某方棋子是否已输棋
bool isDied(PBoard board, PieceColor color);

// 某棋盘红黑定方向后某种棋子可置入位置的集合，返回位置个数（至少90个）
int putSeats(Seat* pseats, bool isBottom, PieceKind kind);

// 某棋盘局面下某位置的棋子“行走规则(筛除同色，未筛将军)”可移入位置的集合（至多17个）
int moveSeats(Seat* pseats, const PBoard board, Seat fseat);

// 某棋盘局面下某位置的棋子在“行走规则”基础上筛除被将军位置后可移入位置的集合（至多17个）
int getMoveSeats(Seat* pseats, int count, PBoard board, Seat fseat);

// 移动棋子，返回目的地棋子
Piece movePiece(PBoard board, Seat fseat, Seat tseat, Piece eatPiece);

// 按某种变换类型变换棋盘局面
void changeBoard(PBoard board, ChangeType ct);

// 输出某棋盘局面的文本字符串，长度小于1024
wchar_t* getBoardString(wchar_t* boardStr, const PBoard board);
// 棋盘上边标识字符串
wchar_t* getBoardPreString(wchar_t* preStr, const PBoard board);
// 棋盘下边标识字符串
wchar_t* getBoardSufString(wchar_t* sufStr, const PBoard board);

// 测试本翻译单元各种对象、函数
void testBoard(FILE* fout);

#endif