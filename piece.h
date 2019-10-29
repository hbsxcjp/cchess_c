#ifndef PIECE_H
#define PIECE_H

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>

// 棋子颜色类型
enum PieceColor {
    RED,
    BLACK
};

// 棋子种类类型
enum PieceKind {
    KING = L'K',
    ADVISOR = L'A',
    BISHOP = L'B',
    KNIGHT = L'N',
    ROOK = L'R',
    CANNON = L'C',
    PAWN = L'P'
};

// 棋子结构类型
typedef struct {
    enum PieceColor color;
    enum PieceKind kind;
} Piece;

// 一副棋子结构类型
typedef struct {
    Piece redPiece[16];
    Piece blackPiece[16];
} Pieces;

const Pieces pieces = {
    { { RED, KING }, { RED, ADVISOR }, { RED, ADVISOR },
        { RED, BISHOP }, { RED, BISHOP }, { RED, KNIGHT }, { RED, KNIGHT },
        { RED, ROOK }, { RED, ROOK }, { RED, CANNON }, { RED, CANNON },
        { RED, PAWN }, { RED, PAWN }, { RED, PAWN }, { RED, PAWN }, { RED, PAWN } },
    { { BLACK, KING }, { BLACK, ADVISOR }, { BLACK, ADVISOR },
        { BLACK, BISHOP }, { BLACK, BISHOP }, { BLACK, KNIGHT }, { BLACK, KNIGHT },
        { BLACK, ROOK }, { BLACK, ROOK }, { BLACK, CANNON }, { BLACK, CANNON },
        { BLACK, PAWN }, { BLACK, PAWN }, { BLACK, PAWN }, { BLACK, PAWN }, { BLACK, PAWN } }
};

wchar_t getChar(const Piece* piece)
{
    return piece->color == RED ? piece->kind : towlower(piece->kind);
}

wchar_t getName(const Piece* piece)
{
    switch (piece->kind) {
    case KING:
        return piece->color == RED ? L'帅' : L'将';
    case ADVISOR:
        return piece->color == RED ? L'仕' : L'士';
    case BISHOP:
        return piece->color == RED ? L'相' : L'象';
    case ROOK:
        return L'车';
    case KNIGHT:
        return L'马';
    case CANNON:
        return L'炮';
    default:
        return piece->color == RED ? L'兵' : L'卒';
    }
}

void testPiece(void)
{
    for (int i = 0; i < 16; ++i) {
        const Piece *redPie = &(pieces.redPiece[i]), *blackPie = &(pieces.blackPiece[i]);
        wprintf(L"%2d red: %lc%lc black: %lc%lc \n",
            i + 1, getChar(redPie), getName(redPie), getChar(blackPie), getName(blackPie));
    }
}


#endif