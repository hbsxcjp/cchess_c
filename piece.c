#include "piece.h"

// 一副棋子的全局常量
const Pieces pieces = { { { { RED, KING }, { RED, ADVISOR }, { RED, ADVISOR }, { RED, BISHOP }, { RED, BISHOP }, { RED, KNIGHT }, { RED, KNIGHT }, { RED, ROOK }, { RED, ROOK }, { RED, CANNON }, { RED, CANNON }, { RED, PAWN }, { RED, PAWN }, { RED, PAWN }, { RED, PAWN }, { RED, PAWN } },
    { { BLACK, KING }, { BLACK, ADVISOR }, { BLACK, ADVISOR }, { BLACK, BISHOP }, { BLACK, BISHOP }, { BLACK, KNIGHT }, { BLACK, KNIGHT }, { BLACK, ROOK }, { BLACK, ROOK }, { BLACK, CANNON }, { BLACK, CANNON }, { BLACK, PAWN }, { BLACK, PAWN }, { BLACK, PAWN }, { BLACK, PAWN }, { BLACK, PAWN } } } };

wchar_t getChar(const Piece* piece)
{
    static wchar_t redChars[] = L"KABNRCP", blackChars[] = L"kabnrcp";
    return piece->color == RED ? redChars[piece->kind] : blackChars[piece->kind];
}

wchar_t getName(const Piece* piece)
{
    static wchar_t redNames[] = L"帅仕相车马炮兵", blackNames[] = L"将士象车马炮卒";
    return piece->color == RED ? redNames[piece->kind] : blackNames[piece->kind];
}

void testPiece(void)
{
    for (int k = 0; k < PIECEKINDNUM; ++k) {
        for (int i = 0; i < PIECENUM; ++i) {
            const Piece* pie = &(pieces.piece[k][i]);
            wprintf(L"%2d:%c%c ",
                pie->color, getChar(pie), getName(pie));
        }
        wprintf(L"\n");
    }
}
