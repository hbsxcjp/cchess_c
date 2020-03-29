#include "head/piece.h"

// 棋子内部实现类型
struct Piece {
    PieceColor color;
    PieceKind kind;
    Seat seat;
};

// 一副棋子
struct Pieces {
    Piece piece[PIECECOLORNUM][SIDEPIECENUM];
};

static const wchar_t* PieceChars[PIECECOLORNUM] = { L"KABNRCP", L"kabnrcp" };

const wchar_t* PieceNames[PIECECOLORNUM] = { L"帅仕相马车炮兵", L"将士象马车炮卒" };

const wchar_t BLANKCHAR = L'_';

static struct Piece BLANKPIECE_ = { NOTCOLOR, NOTKIND, NULL };
Piece BLANKPIECE = &BLANKPIECE_;

const wchar_t ALLPIENAME = L'\x0';
const int ALLCOL = -1;

Pieces newPieces(void)
{
    Pieces pieces = malloc(sizeof(struct Pieces));
    int count[PIECEKINDNUM] = { 1, 2, 2, 2, 2, 2, 5 }; // 每种棋子的数量，共16个
    for (int c = RED; c < NOTCOLOR; ++c) {
        int index = 0;
        for (int k = KING; k < NOTKIND; ++k)
            for (int i = 0; i < count[k]; ++i) {
                Piece piece = malloc(sizeof(struct Piece));
                piece->color = c;
                piece->kind = k;
                piece->seat = NULL;
                pieces->piece[c][index++] = piece;
            }
    }
    return pieces;
}

void freePieces(Pieces pieces)
{
    for (int i = 0; i < PIECECOLORNUM; ++i)
        for (int j = 0; j < SIDEPIECENUM; ++j)
            free(pieces->piece[i][j]);
    free(pieces);
}

inline PieceColor getColor(Piece piece) { return piece->color; }
inline PieceColor getColor_ch(wchar_t ch) { return islower(ch) ? BLACK : RED; }

inline PieceColor getOtherColor(Piece piece) { return !getColor(piece); }

inline PieceKind getKind(Piece piece) { return piece->kind; }

inline wchar_t getChar(Piece piece) { return piece->color == NOTCOLOR ? BLANKCHAR : PieceChars[getColor(piece)][getKind(piece)]; }

inline wchar_t getPieName(Piece piece) { return PieceNames[getColor(piece)][getKind(piece)]; }

wchar_t getPieName_T(Piece piece)
{
    static const wchar_t PieceNames_t[] = L"将士象馬車砲卒";
    return getColor(piece) == RED ? getPieName(piece) : PieceNames_t[getKind(piece)];
}

inline bool isPieceName(wchar_t name) { return wcschr(PieceNames[RED], name) != NULL || wcschr(PieceNames[BLACK], name) != NULL; }

inline bool isLinePiece(wchar_t name) { return wcschr(L"将帅车炮兵卒", name) != NULL; }

inline bool isPawnPieceName(wchar_t name) { return PieceNames[RED][PAWN] == name || PieceNames[BLACK][PAWN] == name; }

inline bool isKnightPieceName(wchar_t name) { return PieceNames[RED][KNIGHT] == name; }

inline static bool isStronge(Piece piece) { return getKind(piece) >= KNIGHT; }

Piece getPiece_ch(Pieces pieces, wchar_t ch)
{
    if (ch != BLANKCHAR) {
        PieceColor color = getColor_ch(ch);
        for (int i = 0; i < SIDEPIECENUM; ++i) {
            Piece piece = pieces->piece[color][i];
            if (piece->seat == NULL && getChar(piece) == ch)
                return piece;
        }
        assert(!"没有找到合适的棋子。");
    }
    return BLANKPIECE;
}

Piece getOtherPiece(Pieces pieces, Piece piece)
{
    if (piece != BLANKPIECE) {
        PieceColor color = getColor(piece);
        for (int i = 0; i < SIDEPIECENUM; ++i)
            if (piece == pieces->piece[color][i])
                return pieces->piece[!color][i];
        //assert(!L"没有找到对方棋子。");
    }
    return BLANKPIECE;
}

Piece setOnBoard(Piece piece, Seat seat)
{
    if (piece != BLANKPIECE)
        piece->seat = seat;
    return piece;
}

extern int getCol_s(Seat seat);
int getLiveSeats_p(Seat* pseats, const Pieces pieces, PieceColor color,
    wchar_t name, int findCol, bool getStronge)
{
    int count = 0;
    for (int i = 0; i < SIDEPIECENUM; ++i) {
        Piece piece = pieces->piece[color][i];
        if (piece->seat != NULL
            && (name == ALLPIENAME || name == getPieName(piece))
            && (findCol == ALLCOL || getCol_s(piece->seat) == findCol)
            && (!getStronge || isStronge(piece)))
            pseats[count++] = piece->seat;
    }
    return count;
}

wchar_t* getPieString(wchar_t* pieStr, Piece piece)
{
    if (piece != BLANKPIECE)
        swprintf(pieStr, WCHARSIZE, L"%c%c%c", // %c
            getColor(piece) == RED ? L'红' : L'黑', getPieName_T(piece), getChar(piece)); // getPieName(piece),
    else
        pieStr = L"空";
    return pieStr;
}

void testPiece(FILE* fout)
{
    fwprintf(fout, L"testPiece：\n");
    Pieces pieces = newPieces();
    wchar_t pstr[WCHARSIZE], opstr[WCHARSIZE];
    for (int c = 0; c < PIECECOLORNUM; ++c) {
        for (int i = 0; i < SIDEPIECENUM; ++i) {
            Piece piece = pieces->piece[c][i];
            fwprintf(fout, L"%s%s ",
                getPieString(pstr, piece),
                getPieString(opstr, getOtherPiece(pieces, piece)));
        }
        fwprintf(fout, L"%s\n", getPieString(pstr, BLANKPIECE));
    }
    freePieces(pieces);
}
