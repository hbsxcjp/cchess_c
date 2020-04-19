#include "head/piece.h"

// 棋子内部实现类型
struct Piece {
    PieceColor color;
    PieceKind kind;
    Seat seat;
};

// 一副棋子
struct Pieces {
    Piece piece[PIECECOLORNUM][PIECEKINDNUM];
    int count[PIECEKINDNUM];
};

inline static Piece getPiece__(Pieces pieces, PieceColor color, PieceKind kind, int index) { return &pieces->piece[color][kind][index]; }

Pieces newPieces(void)
{
    Pieces pieces = malloc(sizeof(struct Pieces));
    assert(pieces);
    int count[PIECEKINDNUM] = { 1, 2, 2, 2, 2, 2, 5 }; // 每种棋子的数量，共16个
    for (int c = RED; c < NOTCOLOR; ++c) {
        for (int k = KING; k < NOTKIND; ++k) {
            pieces->piece[c][k] = malloc(count[k] * sizeof(struct Piece));
            pieces->count[k] = count[k];
            for (int i = 0; i < count[k]; ++i) {
                Piece piece = getPiece__(pieces, c, k, i);
                piece->color = c;
                piece->kind = k;
                setNullSeat(piece);
            }
        }
    }
    return pieces;
}

void delPieces(Pieces pieces)
{
    for (int c = RED; c < NOTCOLOR; ++c)
        for (int k = KING; k < NOTKIND; ++k)
            free(pieces->piece[c][k]);
    free(pieces);
}

void piecesMap(Pieces pieces, void apply(Piece, void*), void* ptr)
{
    assert(apply);
    for (int c = RED; c < NOTCOLOR; ++c)
        for (int k = KING; k < NOTKIND; ++k)
            for (int i = 0; i < pieces->count[k]; ++i)
                apply(getPiece__(pieces, c, k, i), ptr);
}

inline PieceColor getColor(CPiece piece) { return piece->color; }
inline PieceColor getColor_ch(wchar_t ch) { return islower(ch) ? BLACK : RED; }
inline PieceColor getOtherColor(PieceColor color) { return !color; }

static const wchar_t* getChars__(PieceColor color) { return color == RED ? L"KABNRCP" : L"kabnrcp"; }
inline PieceKind getKind(CPiece piece) { return piece->kind; }
inline PieceKind getKind_ch(wchar_t ch) { return wcschr(getChars__(getColor_ch(ch)), ch) - getChars__(getColor_ch(ch)); }

inline wchar_t getBlankChar() { return L'_'; }
inline wchar_t getChar(CPiece piece) { return piece == getBlankPiece() ? getBlankChar() : getChars__(getColor(piece))[getKind(piece)]; }

static const wchar_t* getNames__(PieceColor color) { return color == RED ? L"帅仕相马车炮兵" : L"将士象马车炮卒"; }
inline wchar_t getPieName(CPiece piece) { return getNames__(getColor(piece))[getKind(piece)]; }
wchar_t getPieName_T(CPiece piece) { return getColor(piece) == RED ? getPieName(piece) : L"将士象馬車砲卒"[getKind(piece)]; }
const wchar_t* getPieceNames(void) { return L"帅仕相马车炮兵将士象卒"; }

inline bool isPieceName(wchar_t name) { return wcschr(getNames__(RED), name) || wcschr(getNames__(BLACK), name); }
inline bool isLinePieceName(wchar_t name) { return wcschr(L"将帅车炮兵卒", name); }
inline bool isPawnPieceName(wchar_t name) { return getNames__(RED)[PAWN] == name || getNames__(BLACK)[PAWN] == name; }
inline bool isKnightPieceName(wchar_t name) { return getNames__(RED)[KNIGHT] == name; }
inline bool isStronge(CPiece piece) { return getKind(piece) >= KNIGHT; }
inline bool isBlankPiece(CPiece piece) { return piece == getBlankPiece(); }

static struct Piece BLANKPIECE_ = { NOTCOLOR, NOTKIND, NULL };
inline Piece getBlankPiece() { return &BLANKPIECE_; }
inline Piece getKingPiece(Pieces pieces, PieceColor color) { return getPiece__(pieces, color, KING, 0); }

Piece getOtherPiece(Pieces pieces, CPiece piece)
{
    assert(piece);
    PieceColor color = getColor(piece), othColor = getOtherColor(color);
    PieceKind kind = getKind(piece);
    for (int i = 0; i < pieces->count[kind]; ++i)
        if (piece == getPiece__(pieces, color, kind, i))
            return getPiece__(pieces, othColor, kind, i);
    assert(!L"没有找到对应的棋子");
    return NULL;
}

Piece getPiece_ch(Pieces pieces, wchar_t ch)
{
    if (ch != getBlankChar()) {
        PieceColor color = getColor_ch(ch);
        PieceKind kind = getKind_ch(ch);
        for (int i = 0; i < pieces->count[kind]; ++i) {
            Piece piece = getPiece__(pieces, color, kind, i);
            if (getSeat_p(piece) == NULL) // && getChar(piece) == ch 不需要判断，必定成立
                return piece;
        }
        assert(!"没有找到合适的棋子。");
    }
    return getBlankPiece();
}

inline Seat getSeat_p(CPiece piece) { return piece->seat; }

inline void setNullSeat(Piece piece) { piece->seat = NULL; }

inline void setSeat(Piece piece, Seat seat) { piece->seat = seat; }

inline static bool isNotNullSeat__(Seat seat, Piece piece) { return seat; }

inline static bool isNotNullSeat_stronge__(Seat seat, Piece piece) { return seat && isStronge(piece); }

static int filterLiveSeats__(Seat* seats, Pieces pieces, PieceColor color, bool (*func)(Seat, Piece))
{
    int count = 0;
    for (int k = KING; k < NOTKIND; ++k)
        for (int i = 0; i < pieces->count[k]; ++i) {
            Piece piece = getPiece__(pieces, color, k, i);
            Seat seat = getSeat_p(piece);
            if (func(seat, piece))
                seats[count++] = seat;
        }
    return count;
}

int getLiveSeats_c(Seat* seats, Pieces pieces, PieceColor color) { return filterLiveSeats__(seats, pieces, color, isNotNullSeat__); }

int getLiveSeats_cs(Seat* seats, Pieces pieces, PieceColor color) { return filterLiveSeats__(seats, pieces, color, isNotNullSeat_stronge__); }

wchar_t* getPieString(wchar_t* pieStr, CPiece piece)
{
    extern int getRowCol_s(CSeat seat);
    if (!isBlankPiece(piece))
        swprintf(pieStr, WCHARSIZE, L"%c%c%c@%02X", // %c
            getColor(piece) == RED ? L'红' : L'黑', getPieName_T(piece), getChar(piece),
            getSeat_p(piece) ? getRowCol_s(getSeat_p(piece)) : 0xFF); // getPieName(piece),
    else
        pieStr = L"空";
    return pieStr;
}

static void printPiece__(Piece piece, void* ptr)
{
    FILE* fout = ptr;
    wchar_t pstr[WCHARSIZE];
    fwprintf(fout, L"%s ", getPieString(pstr, piece));
}

void testPiece(FILE* fout)
{
    fwprintf(fout, L"testPiece：\n");
    Pieces pieces = newPieces();
    piecesMap(pieces, printPiece__, fout);
    free(pieces);
}
