#ifndef PIECE_H
#define PIECE_H

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>
#include <locale.h>

#define PIECENUM 16 // 一方棋子个数

// 棋子颜色类型
typedef enum
{
    RED,
    BLACK
} PieceColor;

// 棋子种类类型
typedef enum
{
    KING = L'K',
    ADVISOR = L'A',
    BISHOP = L'B',
    KNIGHT = L'N',
    ROOK = L'R',
    CANNON = L'C',
    PAWN = L'P'
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
    const Piece redPiece[PIECENUM];
    const Piece blackPiece[PIECENUM];
} Pieces;

wchar_t getChar(const Piece *ppiece);

wchar_t getName(const Piece *ppiece);

void testPiece(void);

#endif