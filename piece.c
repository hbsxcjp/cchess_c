#include "piece.h"

// 一副棋子的全局常量
const Pieces pieces = {{{{RED, KING},
                         {RED, ADVISOR},
                         {RED, ADVISOR},
                         {RED, BISHOP},
                         {RED, BISHOP},
                         {RED, KNIGHT},
                         {RED, KNIGHT},
                         {RED, ROOK},
                         {RED, ROOK},
                         {RED, CANNON},
                         {RED, CANNON},
                         {RED, PAWN},
                         {RED, PAWN},
                         {RED, PAWN},
                         {RED, PAWN},
                         {RED, PAWN}},
                        {{BLACK, KING},
                         {BLACK, ADVISOR},
                         {BLACK, ADVISOR},
                         {BLACK, BISHOP},
                         {BLACK, BISHOP},
                         {BLACK, KNIGHT},
                         {BLACK, KNIGHT},
                         {BLACK, ROOK},
                         {BLACK, ROOK},
                         {BLACK, CANNON},
                         {BLACK, CANNON},
                         {BLACK, PAWN},
                         {BLACK, PAWN},
                         {BLACK, PAWN},
                         {BLACK, PAWN},
                         {BLACK, PAWN}}}};

wchar_t getChar(const Piece *piece)
{
    static wchar_t *rbChars[] = {L"KABNRCP", L"kabnrcp"};
    return (piece == NULL ? BLANKCHAR : rbChars[piece->color][piece->kind]);
}

wchar_t getName(const Piece *piece)
{
    static wchar_t *rbNames[] = {L"帅仕相马车炮兵", L"将士象马车炮卒"};
    return rbNames[piece->color][piece->kind];
}

wchar_t getTextName(const Piece *piece)
{
    static wchar_t blackTextNames[] = L"将士象馬車砲卒";
    return piece->color == RED ? getName(piece) : blackTextNames[piece->kind];
}

void testPiece(void)
{
    for (int k = 0; k < PIECECOLORNUM; ++k)
    {
        for (int i = 0; i < PIECENUM; ++i)
        {
            const Piece *pie = &(pieces.piece[k][i]);
            wprintf(L"=>%2d_%c%c%c ",
                    pie->color, getChar(pie), getName(pie), getTextName(pie));
        }
        wprintf(L"\n");
    }
}
