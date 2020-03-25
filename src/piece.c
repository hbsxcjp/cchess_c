#include "head/piece.h"

// 棋子内部实现类型
struct Piece {
    PieceColor color;
    PieceKind kind;
    bool onBoard;
};

// 一副棋子
struct Pieces {
    Piece piece[PIECENUM];
};

static const wchar_t* PieceChars[PIECECOLORNUM] = { L"KABNRCP", L"kabnrcp" };

const wchar_t* PieceNames[PIECECOLORNUM] = { L"帅仕相马车炮兵", L"将士象马车炮卒" };

const wchar_t BLANKCHAR = L'_';

static struct Piece _BLANKPIECE = { NOTCOLOR, NOTKIND, false };
Piece BLANKPIECE = &_BLANKPIECE;

Pieces newPieces(void)
{
    Pieces pieces = malloc(sizeof(struct Pieces));
    int count[PIECEKINDNUM] = { 1, 2, 2, 2, 2, 2, 5 }; // 每种棋子的数量，共16个
    int index = 0;
    for (int c = RED; c <= BLACK; ++c) {
        for (int k = KING; k <= PAWN; ++k)
            for (int i = 0; i < count[k]; ++i) {
                Piece piece = malloc(sizeof(struct Piece));
                piece->color = c;
                piece->kind = k;
                piece->onBoard = false;
                pieces->piece[index++] = piece;
            }
    }
    return pieces;
}

void freePieces(Pieces pieces)
{
    for (int i = 0; i < PIECENUM; ++i)
        free(pieces->piece[i]);
    free(pieces);
}

inline PieceColor getColor(Piece piece) { return piece->color; }

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

Piece getPiece_i(CPieces pieces, int index)
{
    assert(index >= 0 && index < PIECENUM);
    return pieces->piece[index];
}

Piece getPiece_ch(CPieces pieces, wchar_t ch)
{
    if (ch != BLANKCHAR) {
        for (int i = 0; i < PIECENUM; ++i) {
            Piece piece = pieces->piece[i];
            if (piece->onBoard == false && getChar(piece) == ch)
                return piece;
        }
        assert(!"没有找到合适的棋子。");
    }
    return BLANKPIECE;
}

Piece getOtherPiece(CPieces pieces, Piece piece)
{
    if (piece != BLANKPIECE) {
        for (int i = 0; i < PIECENUM; ++i)
            if (piece == pieces->piece[i])
                return pieces->piece[(i + PIECENUM / 2) % PIECENUM];
        //assert(!L"没有找到对方棋子。");
    }
    return BLANKPIECE;
}

Piece setOnBoard(Piece piece, bool onBoard)
{
    if (piece != BLANKPIECE)
        piece->onBoard = onBoard;
    return piece;
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
        int sideNum = PIECENUM / PIECECOLORNUM;
        for (int i = 0; i < sideNum; ++i) {
            Piece piece = pieces->piece[c * sideNum + i];
            fwprintf(fout, L"%s%s ",
                getPieString(pstr, piece),
                getPieString(opstr, getOtherPiece(pieces, piece)));
        }
        fwprintf(fout, L"%s\n", getPieString(pstr, BLANKPIECE));
    }
    freePieces(pieces);
}
