#include "head/piece.h"

static const wchar_t* PieceChars[PIECECOLORNUM] = { L"KABNRCP", L"kabnrcp" };

const wchar_t* PieceNames[PIECECOLORNUM] = { L"帅仕相马车炮兵", L"将士象马车炮卒" };

inline PieceColor getColor(Piece piece) { return (bool)(piece & 0x10); }

inline PieceColor getOtherColor(Piece piece) { return getColor(piece) == RED ? BLACK : RED; }

inline PieceKind getKind(Piece piece) { return piece & 0x0F; }

inline PieceKind getOtherPiece(Piece piece) { return (getOtherColor(piece) << 4) | getKind(piece); }

inline wchar_t getChar(Piece piece) { return piece == BLANKPIECE ? BLANKCHAR : PieceChars[getColor(piece)][getKind(piece)]; }

inline PieceColor getColor_ch(wchar_t ch) { return (bool)islower(ch); }

Piece getPiece_ch(wchar_t ch)
{
    PieceColor color = getColor_ch(ch);
    return (ch == BLANKCHAR) ? BLANKPIECE : ((color << 4) | (wcschr(PieceChars[color], ch) - PieceChars[color]));
}

inline wchar_t getPieName(Piece piece) { return PieceNames[getColor(piece)][getKind(piece)]; }

wchar_t getPieName_T(Piece piece)
{
    static const wchar_t PieceNames_t[] = L"将士象馬車砲卒";
    return getColor(piece) == RED ? getPieName(piece) : PieceNames_t[getKind(piece)];
}

wchar_t* getPieString(wchar_t* pieStr, size_t n, Piece piece)
{
    if (piece != BLANKPIECE)
        swprintf(pieStr, n, L"%c%c%c", // %c
            getColor(piece) == RED ? L'红' : L'黑', getPieName_T(piece), getChar(piece)); // getPieName(piece),
    else
        pieStr = L"空";
    return pieStr;
}

void testPiece(FILE* fout)
{
    wchar_t pieString[WIDEWCHARSIZE];
    fwprintf(fout, L"testPiece：\n");
    for (int k = 0; k < PIECECOLORNUM; ++k) {
        for (int i = 0; i < PIECEKINDNUM; ++i)
            fwprintf(fout, L"%s ",
                getPieString(pieString, WIDEWCHARSIZE, getPiece_ch(PieceChars[k][i])));
        fwprintf(fout, L"\n");
    }
}
