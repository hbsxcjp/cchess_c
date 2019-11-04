#include "piece.h"

// 一副棋子的全局常量
const Pieces pieces = { { { { RED, KING },
                              { RED, ADVISOR },
                              { RED, ADVISOR },
                              { RED, BISHOP },
                              { RED, BISHOP },
                              { RED, KNIGHT },
                              { RED, KNIGHT },
                              { RED, ROOK },
                              { RED, ROOK },
                              { RED, CANNON },
                              { RED, CANNON },
                              { RED, PAWN },
                              { RED, PAWN },
                              { RED, PAWN },
                              { RED, PAWN },
                              { RED, PAWN } },
    { { BLACK, KING },
        { BLACK, ADVISOR },
        { BLACK, ADVISOR },
        { BLACK, BISHOP },
        { BLACK, BISHOP },
        { BLACK, KNIGHT },
        { BLACK, KNIGHT },
        { BLACK, ROOK },
        { BLACK, ROOK },
        { BLACK, CANNON },
        { BLACK, CANNON },
        { BLACK, PAWN },
        { BLACK, PAWN },
        { BLACK, PAWN },
        { BLACK, PAWN },
        { BLACK, PAWN } } } };

wchar_t getChar(const Piece* piece)
{
    static wchar_t* rbChars[] = { L"KABNRCP", L"kabnrcp" };
    return (piece == NULL ? BLANKCHAR : rbChars[piece->color][piece->kind]);
}

wchar_t getPieName(const Piece* piece)
{
    static wchar_t* rbNames[] = { L"帅仕相马车炮兵", L"将士象马车炮卒" };
    return rbNames[piece->color][piece->kind];
}

wchar_t getPieName_T(const Piece* piece)
{
    static wchar_t blackTextNames[] = L"将士象馬車砲卒";
    return piece->color == RED ? getPieName(piece) : blackTextNames[piece->kind];
}

wchar_t* getPieString(const Piece* piece)
{
    static wchar_t pieString[20];
    swprintf(pieString, 20, L"%d_%c%c%c",
        piece->color, getChar(piece), getPieName(piece), getPieName_T(piece));
    return pieString;
}

wchar_t* testPiece(void)
{
    static wchar_t testStr[32 * 20];
    swprintf(testStr, 20, L"\n%s\n", L"testPiece：");
    for (int k = 0; k < PIECECOLORNUM; ++k) {
        for (int i = 0; i < PIECENUM; ++i) {
            const Piece* piece = &(pieces.piece[k][i]);
            wcscat(testStr, getPieString(piece));
            wcscat(testStr, L" ");
        }
        wcscat(testStr, L"\n");
    }
    return testStr;
}
