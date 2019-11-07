#include "piece.h"

// 棋子的全局常量
const Piece PIECES[PIECECOLORNUM][PIECEKINDNUM] = {
    { { RED, KING }, { RED, ADVISOR }, { RED, BISHOP },
        { RED, KNIGHT }, { RED, ROOK }, { RED, CANNON }, { RED, PAWN } },
    { { BLACK, KING }, { BLACK, ADVISOR }, { BLACK, BISHOP },
        { BLACK, KNIGHT }, { BLACK, ROOK }, { BLACK, CANNON }, { BLACK, PAWN } }
};

static const wchar_t* PieceChars[PIECECOLORNUM] = { L"KABNRCP", L"kabnrcp" };

static const wchar_t* PieceNames[PIECECOLORNUM] = { L"帅仕相马车炮兵", L"将士象马车炮卒" };

static const wchar_t* PieceNames_b[PIECECOLORNUM] = { L"帅仕相马车炮兵", L"将士象馬車砲卒" };

wchar_t getChar(const Piece* piece)
{
    return (piece == NULL ? BLANKCHAR : PieceChars[piece->color][piece->kind]);
}

PieceKind getColor(wchar_t ch)
{
    return iswlower(ch) ? BLACK : RED;
}

PieceKind getKind(wchar_t ch)
{
    PieceColor color = getColor(ch);
    return wcschr(PieceChars[color], ch) - PieceChars[color];
}

wchar_t getPieName(const Piece* piece)
{
    return PieceNames[piece->color][piece->kind];
}

wchar_t getPieName_T(const Piece* piece)
{
    return PieceNames_b[piece->color][piece->kind];
}

wchar_t* getPieString(wchar_t* str, size_t n, const Piece* piece)
{
    if (piece)
        swprintf(str, n, L"%c%c%c",
            piece->color == RED ? L'红' : L'黑',
            getPieName_T(piece), getChar(piece)); // getPieName(piece),
    return str;
}

void testPiece(FILE* fout)
{
    wchar_t pieString[TEMPSTR_SIZE];
    fwprintf(fout, L"\ntestPiece：\n");
    for (int k = 0; k < PIECECOLORNUM; ++k) {
        for (int i = 0; i < PIECEKINDNUM; ++i) {
            const Piece* piece = &(PIECES[k][i]);
            fwprintf(fout, L"%s ", getPieString(pieString, TEMPSTR_SIZE, piece));
        }
        fwprintf(fout, L"\n");
    }
}
