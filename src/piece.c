#include "head/piece.h"

// 空子的字符表示
#define BLANKCHAR L'_'
// 空子
#define BLANKPIECE 0xFF

// 棋子内部实现类型
typedef struct Piece {
    int value; // 高四位表示颜色，低四位表示种类
    bool onBoard;
} * Piece;

// 一副棋子
typedef struct Pieces {
    Piece piece[PIECENUM + 1]; //最后一个为空棋子
} * Pieces;

static const wchar_t* PieceChars[PIECECOLORNUM] = { L"KABNRCP", L"kabnrcp" };

static const wchar_t* PieceNames[PIECECOLORNUM] = { L"帅仕相马车炮兵", L"将士象马车炮卒" };

Pieces getPieces(void)
{
    Pieces pieces = malloc(sizeof(struct Pieces));
    int count[PIECEKINDNUM] = { 1, 2, 2, 2, 2, 2, 5 }; // 每种棋子的数量
    int index = 0;
    for (int c = 0; c < PIECECOLORNUM; ++c) {
        for (int k = 0; k < PIECEKINDNUM; ++k)
            for (int i = 0; i < count[k]; ++i) {
                Piece piece = malloc(sizeof(struct Piece));
                piece->value = (c << 4) | k;
                piece->onBoard = false;
                pieces->piece[index++] = piece;
            }
    }
    Piece blankPiece = malloc(sizeof(struct Piece));
    blankPiece->value = BLANKPIECE;
    blankPiece->onBoard = false;
    pieces->piece[PIECENUM] = blankPiece;
    return pieces;
}

void freePieces(Pieces pieces)
{
    for (int i = 0; i <= PIECENUM; ++i)
        free(pieces->piece[i]);
    free(pieces);
}

inline PieceColor getColor(Piece piece) { return (bool)(piece->value & 0x10); }

inline PieceColor getOtherColor(Piece piece) { return !getColor(piece); }

inline PieceKind getKind(Piece piece) { return piece->value & 0x0F; }

inline wchar_t getChar(Piece piece) { return piece->value == BLANKPIECE ? BLANKCHAR : PieceChars[getColor(piece)][getKind(piece)]; }

inline wchar_t getPieName(Piece piece) { return PieceNames[getColor(piece)][getKind(piece)]; }

wchar_t getPieName_T(Piece piece)
{
    static const wchar_t PieceNames_t[] = L"将士象馬車砲卒";
    return getColor(piece) == RED ? getPieName(piece) : PieceNames_t[getKind(piece)];
}

inline wchar_t getBlankChar(void) { return BLANKCHAR; }

inline Piece getBlankPiece(Pieces pieces) { return pieces->piece[PIECENUM]; }

Piece getPiece_ch(Pieces pieces, wchar_t ch)
{
    if (ch != BLANKCHAR) {
        PieceColor color = (bool)islower(ch);
        PieceKind kind = wcschr(PieceChars[color], ch) - PieceChars[color];
        for (int i = 0; i <= PIECENUM; ++i) {
            Piece piece = pieces->piece[i];
            if (piece->onBoard == false && getColor(piece) == color && getKind(piece) == kind)
                return piece;
        }
        assert(!L"没有未使用的棋子。");
    }
    return getBlankPiece(pieces);
}

Piece getOtherPiece(Pieces pieces, Piece piece)
{
    PieceColor otherColor = getOtherColor(piece);
    PieceKind kind = getKind(piece);
    for (int i = 0; i < PIECENUM; ++i) {
        Piece otherPiece = pieces->piece[i];
        if (getColor(otherPiece) == otherColor && getKind(otherPiece) == kind)
            return otherPiece;
    }
    assert(!L"没有找到对方棋子。");
    return getBlankPiece(pieces);
}

wchar_t* getPieString(wchar_t* pieStr, size_t n, Piece piece)
{
    if (piece->value != BLANKPIECE)
        swprintf(pieStr, n, L"%c%c%c", // %c
            getColor(piece) == RED ? L'红' : L'黑', getPieName_T(piece), getChar(piece)); // getPieName(piece),
    else
        pieStr = L"空";
    return pieStr;
}

void testPiece(FILE* fout)
{
    fwprintf(fout, L"testPiece：\n");
    Pieces pieces = getPieces();
    wchar_t pstr[WCHARSIZE], opstr[WCHARSIZE];
    for (int c = 0; c < PIECECOLORNUM; ++c) {
        int sideNum = PIECENUM / PIECECOLORNUM;
        for (int i = 0; i < sideNum; ++i) {
            Piece piece = pieces->piece[c * sideNum + i];
            fwprintf(fout, L"%s%s ",
                getPieString(pstr, WCHARSIZE, piece),
                getPieString(opstr, WCHARSIZE, getOtherPiece(pieces, piece)));
        }
        fwprintf(fout, L"%s\n", getPieString(pstr, WCHARSIZE, getBlankPiece(pieces)));
    }
    freePieces(pieces);
}
