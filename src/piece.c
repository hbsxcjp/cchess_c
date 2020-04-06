#include "head/piece.h"

// 棋子内部实现类型
struct Piece {
    PieceColor color;
    PieceKind kind;
    Seat seat;
};

// 一副棋子
struct Pieces {
    struct Piece piece[PIECECOLORNUM][SIDEPIECENUM];
    int indexCount[PIECEKINDNUM][2];
};

static const wchar_t* PieceChars[PIECECOLORNUM] = { L"KABNRCP", L"kabnrcp" };

const wchar_t* PieceNames[PIECECOLORNUM] = { L"帅仕相马车炮兵", L"将士象马车炮卒" };

const wchar_t BLANKCHAR = L'_';

static struct Piece BLANKPIECE_ = { NOTCOLOR, NOTKIND, NULL };
Piece BLANKPIECE = &BLANKPIECE_;

Pieces newPieces(void)
{
    Pieces pieces = malloc(sizeof(struct Pieces));
    assert(pieces);
    int index[PIECEKINDNUM] = { 0, 1, 3, 5, 7, 9, 11 }; // 每种棋子的起始序号
    int count[PIECEKINDNUM] = { 1, 2, 2, 2, 2, 2, 5 }; // 每种棋子的数量，共16个
    for (int color = RED; color < NOTCOLOR; ++color)
        for (int k = KING; k < NOTKIND; ++k) {
            pieces->indexCount[k][0] = index[k];
            pieces->indexCount[k][1] = count[k];
            for (int i = 0; i < count[k]; ++i) {
                Piece piece = getPiece_cki(pieces, color, k, i);
                piece->color = color;
                piece->kind = k;
                piece->seat = NULL;
            }
        }
    return pieces;
}

void piecesMap(Pieces pieces, void (*func)(Pieces, Piece))
{
    for (int color = RED; color < NOTCOLOR; ++color)
        for (int k = KING; k < NOTKIND; ++k)
            for (int i = 0; i < pieces->indexCount[k][1]; ++i)
                func(pieces, getPiece_cki(pieces, color, k, i));
}

inline PieceColor getColor(CPiece piece) { return piece->color; }
inline PieceColor getColor_ch(wchar_t ch) { return islower(ch) ? BLACK : RED; }

inline PieceColor getOtherColor(CPiece piece) { return !getColor(piece); }

inline PieceKind getKind(CPiece piece) { return piece->kind; }
inline PieceKind getKind_ch(wchar_t ch) { return wcschr(PieceChars[getColor_ch(ch)], ch) - PieceChars[getColor_ch(ch)]; }

inline Seat getSeat_p(CPiece piece) { return piece->seat; }

inline wchar_t getChar(CPiece piece) { return piece->color == NOTCOLOR ? BLANKCHAR : PieceChars[getColor(piece)][getKind(piece)]; }

inline wchar_t getPieName(CPiece piece) { return PieceNames[getColor(piece)][getKind(piece)]; }

wchar_t getPieName_T(CPiece piece)
{
    static const wchar_t PieceNames_t[] = L"将士象馬車砲卒";
    return getColor(piece) == RED ? getPieName(piece) : PieceNames_t[getKind(piece)];
}

inline bool isPieceName(wchar_t name) { return wcschr(PieceNames[RED], name) || wcschr(PieceNames[BLACK], name); }

inline bool isLinePieceName(wchar_t name) { return wcschr(L"将帅车炮兵卒", name); }

inline bool isPawnPieceName(wchar_t name) { return PieceNames[RED][PAWN] == name || PieceNames[BLACK][PAWN] == name; }

inline bool isKnightPieceName(wchar_t name) { return PieceNames[RED][KNIGHT] == name; }

inline bool isStronge(CPiece piece) { return getKind(piece) >= KNIGHT; }

inline Piece getKingPiece(Pieces pieces, PieceColor color) { return getPiece_cki(pieces, color, KING, 0); }

Piece getPiece_cki(Pieces pieces, PieceColor color, PieceKind kind, int index)
{
    return &pieces->piece[color][pieces->indexCount[kind][0] + index];
}

Piece getOtherPiece(Pieces pieces, Piece piece)
{
    PieceColor color = getColor(piece), othColor = getOtherColor(piece);
    PieceKind kind = getKind(piece);
    for (int i = 0; i < pieces->indexCount[kind][1]; ++i)
        if (piece == getPiece_cki(pieces, color, kind, i))
            return getPiece_cki(pieces, othColor, kind, i);
    assert(!L"没有找到对应的棋子");
    return NULL;
}

Piece getPiece_ch(Pieces pieces, wchar_t ch)
{
    if (ch != BLANKCHAR) {
        PieceColor color = getColor_ch(ch);
        PieceKind kind = getKind_ch(ch);
        for (int i = 0; i < pieces->indexCount[kind][1]; ++i) {
            Piece piece = getPiece_cki(pieces, color, kind, i);
            if (piece->seat == NULL && getChar(piece) == ch)
                return piece;
        }
        assert(!"没有找到合适的棋子。");
    }
    return BLANKPIECE;
}

void setSeat(Piece piece, Seat seat)
{
    if (piece != BLANKPIECE)
        piece->seat = seat;
}

int getLiveSeats_c(Seat* seats, Pieces pieces, PieceColor color)
{
    int count = 0;
    for (int k = KING; k < NOTKIND; ++k)
        for (int i = 0; i < pieces->indexCount[k][1]; ++i) {
            Seat seat = getPiece_cki(pieces, color, k, i)->seat;
            if (seat)
                seats[count++] = seat;
        }
    return count;
}

extern int getRowCol_s(CSeat seat);
wchar_t* getPieString(wchar_t* pieStr, Piece piece)
{
    if (piece != BLANKPIECE)
        swprintf(pieStr, WCHARSIZE, L"%c%c%c@%02X", // %c
            getColor(piece) == RED ? L'红' : L'黑', getPieName_T(piece), getChar(piece), piece->seat ? getRowCol_s(piece->seat) : 0xFF); // getPieName(piece),
    else
        pieStr = L"空";
    return pieStr;
}

void testPiece(FILE* fout)
{
    fwprintf(fout, L"testPiece：\n");
    Pieces pieces = newPieces();
    wchar_t pstr[WCHARSIZE];
    for (PieceColor color = RED; color < NOTCOLOR; ++color) {
        for (int k = KING; k < NOTKIND; ++k) {
            for (int i = 0; i < pieces->indexCount[k][1]; ++i) {
                Piece piece = getPiece_cki(pieces, color, k, i);
                fwprintf(fout, L"%s ", getPieString(pstr, piece));
            }
        }
        fwprintf(fout, L"%s\n", getPieString(pstr, BLANKPIECE));
    }
    free(pieces);
}
