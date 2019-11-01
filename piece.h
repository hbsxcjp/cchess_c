#ifndef PIECE_H
#define PIECE_H

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>

// 两方
#define PIECEKINDNUM 2 
// 一方棋子个数
#define PIECENUM 16 

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

// 棋子结构类型
typedef struct
{
    const PieceColor color;
    const PieceKind kind;
} Piece;

// 一副棋子结构类型
typedef struct
{
    const Piece piece[PIECEKINDNUM][PIECENUM];
} Pieces;

// 一副棋子的全局常量
const Pieces pieces;

//  取得表示棋子的字符
wchar_t getChar(const Piece* piece);

// 取得表示棋子的名称
wchar_t getName(const Piece* piece);

// 测试本翻译单元各种对象、函数
void testPiece(void);

#endif