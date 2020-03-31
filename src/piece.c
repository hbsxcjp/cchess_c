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
    for (int color = RED; color < NOTCOLOR; ++color) {
        int index = 0;
        for (int k = KING; k < NOTKIND; ++k)
            for (int i = 0; i < count[k]; ++i) {
                Piece piece = getPiece_i(pieces, color, index);
                piece->color = color;
                piece->kind = k;
                piece->seat = NULL;
                index++;
            }
    }
    return pieces;
}

inline PieceColor getColor(CPiece  piece) { return piece->color; }
inline PieceColor getColor_ch(wchar_t ch) { return islower(ch) ? BLACK : RED; }

inline PieceColor getOtherColor(CPiece  piece) { return !getColor(piece); }

inline PieceKind getKind(CPiece  piece) { return piece->kind; }

inline Seat getSeat_p(CPiece  piece) { return piece->seat; }

inline wchar_t getChar(CPiece  piece) { return piece->color == NOTCOLOR ? BLANKCHAR : PieceChars[getColor(piece)][getKind(piece)]; }

inline wchar_t getPieName(CPiece  piece) { return PieceNames[getColor(piece)][getKind(piece)]; }

wchar_t getPieName_T(CPiece  piece)
{
    static const wchar_t PieceNames_t[] = L"将士象馬車砲卒";
    return getColor(piece) == RED ? getPieName(piece) : PieceNames_t[getKind(piece)];
}

inline bool isPieceName(wchar_t name) { return wcschr(PieceNames[RED], name) != NULL || wcschr(PieceNames[BLACK], name) != NULL; }

inline bool isLinePieceName(wchar_t name) { return wcschr(L"将帅车炮兵卒", name) != NULL; }

inline bool isPawnPieceName(wchar_t name) { return PieceNames[RED][PAWN] == name || PieceNames[BLACK][PAWN] == name; }

inline bool isKnightPieceName(wchar_t name) { return PieceNames[RED][KNIGHT] == name; }

inline bool isStronge(CPiece  piece) { return getKind(piece) >= KNIGHT; }

inline Piece getKingPiece(Pieces pieces, PieceColor color) { return getPiece_i(pieces, color, KING); }

inline Piece getPiece_i(Pieces pieces, PieceColor color, int index) { return &pieces->piece[color][index]; }

Piece getPiece_ch(Pieces pieces, wchar_t ch)
{
    if (ch != BLANKCHAR) {
        PieceColor color = getColor_ch(ch);
        for (int i = 0; i < SIDEPIECENUM; ++i) {
            Piece piece = getPiece_i(pieces, color, i);
            if (piece->seat == NULL && getChar(piece) == ch)
                return piece;
        }
        assert(!"没有找到合适的棋子。");
    }
    return BLANKPIECE;
}

void setSeat(Piece  piece, Seat seat)
{
    //if (piece != BLANKPIECE)
        piece->seat = seat;
}

extern int getRowCol_s(CSeat seat);
wchar_t* getPieString(wchar_t* pieStr, Piece piece)
{
    if (piece != BLANKPIECE)
        swprintf(pieStr, WCHARSIZE, L"%c%c%c@%02x", // %c
            getColor(piece) == RED ? L'红' : L'黑', getPieName_T(piece), getChar(piece), piece->seat != NULL ? getRowCol_s(piece->seat) : 0xFF); // getPieName(piece),
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
        for (int i = 0; i < SIDEPIECENUM; ++i) {
            Piece piece = getPiece_i(pieces, color, i);
            fwprintf(fout, L"%s ", getPieString(pstr, piece));
        }
        fwprintf(fout, L"%s\n", getPieString(pstr, BLANKPIECE));
    }
    free(pieces);
}
