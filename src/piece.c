#include "head/piece.h"

// 棋子的全局常量
const Piece PIECES[PIECECOLORNUM][PIECEKINDNUM] = {
    { KING, ADVISOR, BISHOP, KNIGHT, ROOK, CANNON, PAWN },
    { king, advisor, bishop, knight, rook, cannon, pawn }
};

static const wchar_t* PieceChars[PIECECOLORNUM] = { L"KABNRCP", L"kabnrcp" };

static const wchar_t* PieceNames[PIECECOLORNUM] = { L"帅仕相马车炮兵", L"将士象马车炮卒" };

static const wchar_t* PieceNames_b[PIECECOLORNUM] = { L"帅仕相马车炮兵", L"将士象馬車砲卒" };

PieceColor getColor(Piece piece)
{
    return (bool)(piece & 0x10); // 取棋子值的高四位
}

int getKind(Piece piece)
{
    return piece & 0x0F; // 取棋子值的低四位
}

wchar_t getChar(Piece piece)
{
    return piece == BLANKPIECE ? BLANKCHAR : PieceChars[getColor(piece)][getKind(piece)];
}

Piece getPiece_ch(wchar_t ch)
{
    PieceColor color = (bool)islower(ch);
    return PIECES[color][wcschr(PieceChars[color], ch) - PieceChars[color]];
}

wchar_t getPieName(Piece piece)
{
    return PieceNames[getColor(piece)][getKind(piece)];
}

wchar_t getPieName_T(Piece piece)
{
    return PieceNames_b[getColor(piece)][getKind(piece)];
}

wchar_t* getPieString(wchar_t* pieStr, size_t n, Piece piece)
{
    if (piece != BLANKPIECE)
        swprintf(pieStr, n, L"%c%c%c%c",
            getColor(piece) == RED ? L'红' : L'黑',
            getPieName(piece), getPieName_T(piece), getChar(piece)); //
    else
        pieStr = L"空";
    return pieStr;
}

void testPiece(FILE* fout)
{
    wchar_t pieString[TEMPSTR_SIZE];
    fwprintf(fout, L"testPiece：\n");
    for (int k = 0; k < PIECECOLORNUM; ++k) {
        for (int i = 0; i < PIECEKINDNUM; ++i) {
            wchar_t ch = PieceChars[k][i];
            Piece piece1 = getPiece_ch(ch);
            Piece piece = PIECES[k][i];
            fwprintf(fout, L"%s ", getPieString(pieString, TEMPSTR_SIZE, piece1));
            fwprintf(fout, L"%s ", getPieString(pieString, TEMPSTR_SIZE, piece));
        }
        fwprintf(fout, L"\n");
    }
}
