#include "head/piece.h"

// 棋子内部实现类型
struct Piece {
    PieceColor color;
    PieceKind kind;
    Seat seat;
};

// 一副棋子
struct Pieces {
    Piece piece[PIECECOLORNUM][PIECEKINDNUM]; //某种棋子首地址
    int count[PIECEKINDNUM];
};

inline static Piece getPieces__(Pieces pieces, PieceColor color, PieceKind kind) { return pieces->piece[color][kind]; }

Pieces newPieces(void)
{
    Pieces pieces = malloc(sizeof(struct Pieces));
    assert(pieces);
    int count[PIECEKINDNUM] = { 1, 2, 2, 2, 2, 2, 5 }; // 每种棋子的数量，共16个
    for (int c = RED; c < NOTCOLOR; ++c) {
        for (int k = KING; k < NOTKIND; ++k) {
            Piece apiece = pieces->piece[c][k] = malloc(count[k] * sizeof(struct Piece));
            pieces->count[k] = count[k];
            for (int i = 0; i < count[k]; ++i) {
                Piece piece = apiece + i;
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
        for (int k = KING; k < NOTKIND; ++k) {
            Piece apiece = getPieces__(pieces, c, k);
            for (int i = 0; i < pieces->count[k]; ++i)
                apply(apiece++, ptr);
        }
}

static PieceColor getColor_ch(wchar_t ch) { return islower(ch) ? BLACK : RED; }
inline PieceColor getColor(CPiece piece) { return piece->color; }
inline PieceColor getOtherColor(PieceColor color) { return color == RED ? BLACK : RED; }

static const wchar_t* Chars__[] = { L"KABNRCP", L"kabnrcp" };
static PieceKind getKind_ch(wchar_t ch)
{
    const wchar_t* pch = Chars__[getColor_ch(ch)];
    return wcschr(pch, ch) - pch;
}
inline PieceKind getKind(CPiece piece) { return piece->kind; }

inline wchar_t getBlankChar() { return L'_'; }
inline wchar_t getChar(CPiece piece) { return piece == getBlankPiece() ? getBlankChar() : Chars__[getColor(piece)][getKind(piece)]; }

static const wchar_t* Names__[] = { L"帅仕相马车炮兵", L"将士象马车炮卒", L"将士象馬車砲卒" };
static wchar_t getPieName_ch(wchar_t ch) { return Names__[getColor_ch(ch)][getKind_ch(ch)]; }
inline wchar_t getPieName(CPiece piece) { return getPieName_ch(getChar(piece)); }
wchar_t getPieName_T_ch(wchar_t ch) { return getColor_ch(ch) == RED ? getPieName_ch(ch) : Names__[BLACK + 1][getKind_ch(ch)]; }
wchar_t getPieName_T(CPiece piece) { return getPieName_T_ch(getChar(piece)); }
const wchar_t* getPieceNames(void) { return L"帅仕相马车炮兵将士象卒"; }

inline bool isPieceName(wchar_t name) { return wcschr(Names__[RED], name) || wcschr(Names__[BLACK], name); }
inline bool isLinePieceName(wchar_t name) { return wcschr(L"将帅车炮兵卒", name); }
inline bool isPawnPieceName(wchar_t name) { return Names__[RED][PAWN] == name || Names__[BLACK][PAWN] == name; }
inline bool isKnightPieceName(wchar_t name) { return Names__[RED][KNIGHT] == name; }
inline bool isStronge(CPiece piece) { return getKind(piece) >= KNIGHT; }
inline bool isBlankPiece(CPiece piece) { return piece == getBlankPiece(); }

static struct Piece BLANKPIECE_ = { NOTCOLOR, NOTKIND, NULL };
inline Piece getBlankPiece() { return &BLANKPIECE_; }
inline Piece getKingPiece(Pieces pieces, PieceColor color) { return getPieces__(pieces, color, KING); }

Piece getOtherPiece(Pieces pieces, CPiece piece)
{
    assert(piece);
    PieceColor color = getColor(piece), othColor = getOtherColor(color);
    PieceKind kind = getKind(piece);
    Piece apiece = getPieces__(pieces, color, kind);
    for (int i = 0; i < pieces->count[kind]; ++i)
        if (piece == apiece + i)
            return getPieces__(pieces, othColor, kind) + i;
    assert(!L"没有找到对应的棋子");
    return NULL;
}

Piece getPiece_ch(Pieces pieces, wchar_t ch)
{
    if (ch != getBlankChar()) {
        PieceColor color = getColor_ch(ch);
        PieceKind kind = getKind_ch(ch);
        Piece apiece = getPieces__(pieces, color, kind);
        for (int i = 0; i < pieces->count[kind]; ++i) {
            Piece piece = apiece + i;
            if (getSeat_p(piece) == NULL) // && getChar(piece) == ch 不需要判断，必定成立
                return piece;
        }
        assert(!L"没有找到合适的棋子。");
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
    for (int k = KING; k < NOTKIND; ++k) {
        Piece apiece = getPieces__(pieces, color, k);
        for (int i = 0; i < pieces->count[k]; ++i) {
            Piece piece = apiece + i;
            Seat seat = getSeat_p(piece);
            if (func(seat, piece))
                seats[count++] = seat;
        }
    }
    return count;
}

int getLiveSeats_c(Seat* seats, Pieces pieces, PieceColor color) { return filterLiveSeats__(seats, pieces, color, isNotNullSeat__); }

int getLiveSeats_cs(Seat* seats, Pieces pieces, PieceColor color) { return filterLiveSeats__(seats, pieces, color, isNotNullSeat_stronge__); }

wchar_t* getPieString(wchar_t* pieStr, CPiece piece)
{
    extern int getRowCol_s(CSeat seat);
    if (!isBlankPiece(piece))
        swprintf(pieStr, WCHARSIZE, L"%c%c%c@%02X",
            getColor(piece) == RED ? L'红' : L'黑',
            getPieName_T(piece),
            getChar(piece),
            getSeat_p(piece) ? getRowCol_s(getSeat_p(piece)) : 0xFF);
    else
        wcscpy(pieStr, L"空");
    return pieStr;
}

static void printPiece__(Piece piece, void* wstr)
{
    wchar_t pieStr[WCHARSIZE];
    wcscat(wstr, getPieString(pieStr, piece));
    wcscat(wstr, L" ");
}

void testPieceString(wchar_t* wstr)
{
    wstr[0] = L'\x0';
    Pieces pieces = newPieces();
    piecesMap(pieces, printPiece__, wstr);
    free(pieces);
}
