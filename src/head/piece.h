#ifndef PIECE_H
#define PIECE_H

#include "base.h"

// 两方
#define PIECECOLORNUM 2
// 棋子种类数量
#define PIECEKINDNUM 7
// 一方棋子个数
#define PIECENUM 32

//=================================================================
//棋子相关的类型
//=================================================================

// 棋子颜色类型
typedef enum {
    RED,
    BLACK
} PieceColor;

// 棋子种类类型
typedef enum {
    KING,
    ADVISOR,
    BISHOP,
    KNIGHT,
    ROOK,
    CANNON,
    PAWN
} PieceKind;

// 棋子指针类型（不透明）
typedef struct Piece* Piece; // 与结构同名
typedef struct Pieces* Pieces; // 与结构同名

// 设置一副棋子
Pieces getPieces(void);

// 释放一副棋子
void freePieces(Pieces pieces);

// 取得表示棋子的颜色(判断棋子值的高四位)
PieceColor getColor(Piece piece);

// 取得对方棋子的颜色
PieceColor getOtherColor(Piece piece);

//  取得表示棋子的种类(取棋子值的低四位)
PieceKind getKind(Piece piece);

//  取得表示棋子的字符
wchar_t getChar(Piece piece);

// 取得表示棋子的名称
wchar_t getPieName(Piece piece);

// 取得表示棋子文本的名称
wchar_t getPieName_T(Piece piece);

// 取得空棋子字符
wchar_t getBlankChar(void);

// 取得空棋子
Piece getBlankPiece(Pieces pieces);

//  取得棋子
Piece getPiece_ch(Pieces pieces, wchar_t ch);

//  取得表示相同种类的对方棋子
Piece getOtherPiece(Pieces pieces, Piece piece);

// 取得表示棋子表示字符串的名称
wchar_t* getPieString(wchar_t* pieStr, size_t n, Piece piece);

// 测试本翻译单元各种对象、函数
void testPiece(FILE* fout);

#endif