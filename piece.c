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

static const wchar_t *PieceChars[PIECECOLORNUM] = {L"KABNRCP", L"kabnrcp"};

static const wchar_t *PieceNames[PIECECOLORNUM] = {L"帅仕相马车炮兵", L"将士象马车炮卒"};

static const wchar_t *PieceNames_b[PIECECOLORNUM] = {L"帅仕相马车炮兵", L"将士象馬車砲卒"};

wchar_t getChar(const Piece *piece)
{
    return (piece == NULL ? BLANKCHAR : PieceChars[piece->color][piece->kind]);
}

wchar_t getPieName(const Piece *piece)
{
    return PieceNames[piece->color][piece->kind];
}

wchar_t getPieName_T(const Piece *piece)
{
    return PieceNames_b[piece->color][piece->kind];
}

wchar_t *getPieString(wchar_t* pieString, size_t n, const Piece *piece)
{
    swprintf(pieString, 20, L"%c%c%c%c",
             piece->color == RED ? L'红' : L'黑',
             getPieName(piece), getPieName_T(piece), getChar(piece));
    return pieString;
}

void testPiece(FILE *fout)
{
    wchar_t pieString[TEMPSTR_SIZE];
    fwprintf(fout, L"\ntestPiece：\n");
    for (int k = 0; k < PIECECOLORNUM; ++k)
    {
        for (int i = 0; i < PIECENUM; ++i)
        {
            const Piece *piece = &(pieces.piece[k][i]);
            fwprintf(fout, L"%s ", getPieString(pieString, TEMPSTR_SIZE, piece));
        }
        fwprintf(fout, L"\n");
    }
}
