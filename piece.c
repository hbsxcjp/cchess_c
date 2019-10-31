#include "piece.h"

const Pieces pieces = {
    {{RED, KING}, {RED, ADVISOR}, {RED, ADVISOR}, {RED, BISHOP}, {RED, BISHOP}, {RED, KNIGHT}, {RED, KNIGHT}, {RED, ROOK}, {RED, ROOK}, {RED, CANNON}, {RED, CANNON}, {RED, PAWN}, {RED, PAWN}, {RED, PAWN}, {RED, PAWN}, {RED, PAWN}},
    {{BLACK, KING}, {BLACK, ADVISOR}, {BLACK, ADVISOR}, {BLACK, BISHOP}, {BLACK, BISHOP}, {BLACK, KNIGHT}, {BLACK, KNIGHT}, {BLACK, ROOK}, {BLACK, ROOK}, {BLACK, CANNON}, {BLACK, CANNON}, {BLACK, PAWN}, {BLACK, PAWN}, {BLACK, PAWN}, {BLACK, PAWN}, {BLACK, PAWN}}};

wchar_t getChar(const Piece *ppiece)
{
    return ppiece->color == RED ? ppiece->kind : towlower(ppiece->kind);
}

wchar_t getName(const Piece *ppiece)
{
    switch (ppiece->kind)
    {
    case KING:
        return ppiece->color == RED ? L'帅' : L'将';
    case ADVISOR:
        return ppiece->color == RED ? L'仕' : L'士';
    case BISHOP:
        return ppiece->color == RED ? L'相' : L'象';
    case ROOK:
        return L'车';
    case KNIGHT:
        return L'马';
    case CANNON:
        return L'炮';
    default:
        return ppiece->color == RED ? L'兵' : L'卒';
    }
}

void testPiece(void)
{
    for (int i = 0; i < PIECENUM; ++i)
    {
        const Piece *rppie = &(pieces.redPiece[i]),
                    *bppie = &(pieces.blackPiece[i]);
        wprintf(L"%2d %d:%c%c  %d:%c%c\n", i + 1,
                rppie->color, getChar(rppie), getName(rppie),
                bppie->color, getChar(bppie), getName(bppie));
    }
}
