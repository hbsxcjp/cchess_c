#include "head/board.h"
#include "head/piece.h"

// 棋子马的移动方向
typedef enum {
    SW,
    SE,
    NW,
    NE,
    WS,
    ES,
    WN,
    EN
} MoveDirection;

struct Seat {
    int row, col;
    Piece piece;
};

struct Board {
    Pieces pieces;
    struct Seat seats[BOARDROW][BOARDCOL];
    PieceColor bottomColor;
};

// 棋盘行列相关的静态全局变量
static const int RowLowIndex_ = 0, RowLowMidIndex_ = 2, RowLowUpIndex_ = 4,
                 RowUpLowIndex_ = 5, RowUpMidIndex_ = 7, RowUpIndex_ = BOARDROW - 1,
                 ColLowIndex_ = 0, ColMidLowIndex_ = 3, ColMidUpIndex_ = 5, ColUpIndex_ = BOARDCOL - 1;

const wchar_t ALLPIENAME = L'\x0';
const int ALLCOL = -1;

static int seatCmp(const void* first, const void* second)
{
    Seat aseat = (Seat)first, bseat = (Seat)second;
    return aseat < bseat ? -1 : (aseat > bseat ? 1 : 0);
}

static bool moveSameColor(Board board, Seat fseat, Seat tseat);

static bool moveKilled(Board board, Seat fseat, Seat tseat);

static int filterMoveSeats(Seat* seats, int count, Board board, Seat fseat,
    bool (*__filterFunc)(Board, Seat, Seat), bool reverse);

Board newBoard(void)
{
    Board board = malloc(sizeof(struct Board));
    board->pieces = newPieces();
    for (int r = 0; r < BOARDROW; ++r)
        for (int c = 0; c < BOARDCOL; ++c) {
            Seat seat = getSeat_rc(board, r, c);
            seat->row = r;
            seat->col = c;
            seat->piece = BLANKPIECE;
        }
    board->bottomColor = RED;
    return board;
}

void freeBoard(Board board)
{
    free(board->pieces);
    free(board);
}

inline int getRow_s(CSeat seat) { return seat->row; }
inline int getCol_s(CSeat seat) { return seat->col; }

inline int getOtherRow_r(int row) { return BOARDROW - 1 - row; }
inline int getOtherCol_c(int col) { return BOARDCOL - 1 - col; }

inline int getOtherRow_s(CSeat seat) { return getOtherRow_r(getRow_s(seat)); }
inline int getOtherCol_s(CSeat seat) { return getOtherCol_c(getCol_s(seat)); }

inline int getRowCol_s(CSeat seat) { return (getRow_s(seat) << 4) | getCol_s(seat); }
inline int getRow_rowcol(int rowcol) { return (rowcol & 0xF0) >> 4; }
inline int getCol_rowcol(int rowcol) { return rowcol & 0x0F; }

inline Seat getSeat_rc(Board board, int row, int col)
{
    if (!(row >= 0 && row < BOARDROW && col >= 0 && col < BOARDCOL)) {
        wchar_t wstr[2 * WIDEWCHARSIZE];
        wprintf(L"%srow:%d col:%d\n", getBoardString(wstr, board), row, col);
    }
    assert(row >= 0 && row < BOARDROW && col >= 0 && col < BOARDCOL);
    return &board->seats[row][col];
}
inline Seat getSeat_rowcol(Board board, int rowcol) { return getSeat_rc(board, getRow_rowcol(rowcol), getCol_rowcol(rowcol)); }

inline Piece getPiece_s(CSeat seat) { return seat->piece; }
inline Piece getPiece_rc(Board board, int row, int col) { return getPiece_s(getSeat_rc(board, row, col)); }
inline Piece getPiece_rowcol(Board board, int rowcol) { return getPiece_s(getSeat_rowcol(board, rowcol)); }

// 置入某棋盘内某位置一个棋子
static void setPiece_s(Seat seat, Piece piece)
{
    if (seat != NULL)
        setSeat(seat->piece = piece, seat);
}

// 置入某棋盘内某行、某列位置一个棋子
static void setPiece_rc(Board board, int row, int col, Piece piece) { setPiece_s(getSeat_rc(board, row, col), piece); }

wchar_t* setPieCharsFromFEN(wchar_t* pieChars, const wchar_t* FEN)
{
    int len = wcslen(FEN);
    for (int i = 0, index = 0; i < len && index < SEATNUM; ++i) {
        wchar_t ch = FEN[i];
        if (iswdigit(ch))
            for (int j = ch - L'0'; j > 0; --j)
                pieChars[index++] = BLANKCHAR;
        else if (iswalpha(ch))
            pieChars[index++] = ch;
    }
    pieChars[SEATNUM] = L'\x0';
    return pieChars;
}

wchar_t* getPieChars_board(wchar_t* pieChars, Board board)
{
    int index = 0;
    for (int row = BOARDROW - 1; row >= 0; --row)
        for (int col = 0; col < BOARDCOL; ++col)
            pieChars[index++] = getChar(getPiece_rc(board, row, col));
    pieChars[SEATNUM] = L'\x0';
    return pieChars;
}

wchar_t* setFENFromPieChars(wchar_t* FEN, const wchar_t* pieChars)
{
    int index_F = 0;
    for (int row = 0; row < BOARDROW; ++row) {
        int blankNum = 0;
        for (int col = 0; col < BOARDCOL; ++col) {
            int index_p = row * BOARDCOL + col;
            if (iswalpha(pieChars[index_p])) {
                if (blankNum > 0)
                    FEN[index_F++] = L'0' + blankNum;
                FEN[index_F++] = pieChars[index_p];
                blankNum = 0;
            } else //if (pieChars[index_p] == BLANKCHAR) // 肯定为真
                blankNum++;
        }
        if (blankNum > 0)
            FEN[index_F++] = L'0' + blankNum;
        FEN[index_F++] = L'/'; // 插入行分隔符
    }
    FEN[--index_F] = L'\x0';
    return FEN;
}

void resetBoard(Board board)
{
    for (PieceColor color = RED; color < NOTCOLOR; ++color)
        for (int i = 0; i < SIDEPIECENUM; ++i) {
            Piece piece = getPiece_i(board->pieces, color, i);
            Seat seat = getSeat_p(piece);
            setSeat(piece, NULL);
            setPiece_s(seat, BLANKPIECE);
        }
}

void setBoard(Board board, const wchar_t* pieChars)
{
    assert(wcslen(pieChars) == SEATNUM);
    resetBoard(board);
    int index = 0;
    for (int row = BOARDROW - 1; row >= 0; --row)
        for (int col = 0; col < BOARDCOL; ++col)
            setPiece_rc(board, row, col, getPiece_ch(board->pieces, pieChars[index++]));
    board->bottomColor = getRow_s(getKingSeat(board, RED)) < BOARDROW / 2 ? RED : BLACK;
}

inline bool isBottomSide(Board board, PieceColor color) { return board->bottomColor == color; }

Seat getKingSeat(Board board, PieceColor color) { return getSeat_p(getKingPiece(board->pieces, color)); }

int getLiveSeats(Seat* seats, Board board, PieceColor color, wchar_t name, int findCol)
{
    int count = 0;
    for (int i = 0; i < SIDEPIECENUM; ++i) {
        Piece piece = getPiece_i(board->pieces, color, i);
        Seat seat = getSeat_p(piece);
        if (seat != NULL
            && (name == ALLPIENAME || name == getPieName(piece))
            && (findCol == ALLCOL || getCol_s(seat) == findCol))
            seats[count++] = seat;
    }
    qsort(seats, count, sizeof(Seat*), seatCmp);
    return count;
}

int getSortPawnLiveSeats(Seat* seats, Board board, PieceColor color, wchar_t name)
{
    Seat pawnSeats[SIDEPIECENUM] = { NULL };
    int liveCount = getLiveSeats(pawnSeats, board, color, name, ALLCOL),
        colCount[BOARDCOL] = { 0 };
    Seat colSeats[BOARDCOL][BOARDROW] = { NULL };
    for (int i = 0; i < liveCount; ++i) {
        int col = getCol_s(pawnSeats[i]);
        colSeats[col][colCount[col]++] = pawnSeats[i];
    }
    bool isBottom = isBottomSide(board, color);
    int fcol = isBottom ? ColUpIndex_ : ColLowIndex_,
        tcol = isBottom ? ColLowIndex_ : ColUpIndex_,
        step = isBottom ? -1 : 1,
        count = 0;
    for (int col = fcol; col != tcol; col += step)
        if (colCount[col] > 1) // 筛除小于2个的列
            for (int i = 0; i < colCount[col]; ++i)
                seats[count++] = colSeats[col][i];
    return count;
}

bool isKilled(Board board, PieceColor color)
{
    bool isBottom = isBottomSide(board, color);
    Seat kingSeat = getKingSeat(board, color),
         othKingSeat = getKingSeat(board, !color);
    int frow = getRow_s(kingSeat), fcol = getCol_s(kingSeat),
        trow = getRow_s(othKingSeat), tcol = getCol_s(othKingSeat);
    if (fcol == tcol) {
        bool isBlank = true;
        int rowLow = isBottom ? frow : trow,
            rowUp = isBottom ? trow : frow;
        for (int row = rowLow + 1; row < rowUp; ++row)
            if (getPiece_rc(board, row, fcol) != BLANKPIECE) {
                isBlank = false;
                break;
            }
        // 将帅之间全空，没有间隔棋子
        if (isBlank)
            return true;
    }

    Seat seats[SIDEPIECENUM] = { NULL };
    int count = getLiveSeats(seats, board, !color, ALLPIENAME, ALLCOL);
    for (int i = 0; i < count; ++i) {
        if (!isStronge(getPiece_s(seats[i])))
            continue;
        Seat mseats[BOARDROW + BOARDCOL] = { NULL };
        int mcount = moveSeats(mseats, board, seats[i]);
        for (int m = 0; m < mcount; ++m)
            // 本方将帅位置在对方强子可走位置范围内
            if (mseats[m] == kingSeat)
                return true;
    }
    return false;
}

bool isDied(Board board, PieceColor color)
{
    Seat fseats[SIDEPIECENUM] = { NULL }, mseats[BOARDROW + BOARDCOL] = { NULL };
    int count = getLiveSeats(fseats, board, color, ALLPIENAME, ALLCOL);
    for (int i = 0; i < count; ++i) {
        Seat fseat = fseats[i];
        int mcount = moveSeats(mseats, board, fseat);
        // 有棋子可走
        if (canMoveSeats(mseats, mcount, board, fseat) > 0)
            return false;
    }
    return true;
}

int putSeats(Seat* seats, Board board, bool isBottom, PieceKind kind)
{
    int count = 0, rowLow, rowUp, cross, lfrow, ufrow, ltrow, utrow;
    switch (kind) {
    case KING:
        rowLow = isBottom ? RowLowIndex_ : RowUpMidIndex_;
        rowUp = isBottom ? RowLowMidIndex_ : RowUpIndex_;
        for (int row = rowLow; row <= rowUp; ++row)
            for (int col = ColMidLowIndex_; col <= ColMidUpIndex_; ++col)
                seats[count++] = getSeat_rc(board, row, col);
        break;
    case ADVISOR:
        rowLow = isBottom ? RowLowIndex_ : RowUpMidIndex_;
        rowUp = isBottom ? RowLowMidIndex_ : RowUpIndex_;
        cross = isBottom ? 1 : 0; // 行列和的奇偶值
        for (int row = rowLow; row <= rowUp; ++row)
            for (int col = ColMidLowIndex_; col <= ColMidUpIndex_; ++col)
                if ((col + row) % 2 == cross)
                    seats[count++] = getSeat_rc(board, row, col);
        break;
    case BISHOP:
        rowLow = isBottom ? RowLowIndex_ : RowUpLowIndex_;
        rowUp = isBottom ? RowLowUpIndex_ : RowUpIndex_;
        for (int row = rowLow; row <= rowUp; row += 2)
            for (int col = ColLowIndex_; col <= ColUpIndex_; col += 2) {
                cross = row - col;
                if ((isBottom && (cross == 2 || cross == -2 || cross == -6))
                    || (!isBottom && (cross == 7 || cross == 3 || cross == -1)))
                    seats[count++] = getSeat_rc(board, row, col);
            }
        break;
    case PAWN:
        lfrow = isBottom ? RowLowUpIndex_ - 1 : RowUpLowIndex_;
        ufrow = isBottom ? RowLowUpIndex_ : RowUpLowIndex_ + 1;
        ltrow = isBottom ? RowUpLowIndex_ : RowLowIndex_;
        utrow = isBottom ? RowUpIndex_ : RowLowUpIndex_;
        for (int row = lfrow; row <= ufrow; ++row)
            for (int col = 0; col < BOARDCOL; col += 2)
                seats[count++] = getSeat_rc(board, row, col);
        for (int row = ltrow; row <= utrow; ++row)
            for (int col = 0; col < BOARDCOL; ++col)
                seats[count++] = getSeat_rc(board, row, col);
        break;
    default: // ROOK, KNIGHT, CANNON
        for (int row = 0; row < BOARDROW; ++row)
            for (int col = 0; col < BOARDCOL; ++col)
                seats[count++] = getSeat_rc(board, row, col);
        break;
    }
    return count;
}

int moveSeats(Seat* seats, Board board, Seat fseat)
{
    int count = 0, frow = getRow_s(fseat), fcol = getCol_s(fseat);
    Piece fpiece = getPiece_rc(board, frow, fcol);
    PieceColor color = getColor(fpiece);
    bool isBottom = isBottomSide(board, color);
    switch (getKind(fpiece)) {
    case KING: {
        int trowcols[][2] = {
            { frow, fcol - 1 }, //SW
            { frow, fcol + 1 }, //SE
            { frow - 1, fcol }, //NW
            { frow + 1, fcol } //NE
        };
        bool select[] = { true, true, true, true };
        if (fcol == ColMidLowIndex_)
            select[SW] = false;
        else if (fcol == ColMidUpIndex_)
            select[SE] = false;
        if (frow == (isBottom ? RowLowIndex_ : RowUpMidIndex_))
            select[NW] = false;
        else if (frow == (isBottom ? RowLowMidIndex_ : RowUpIndex_))
            select[NE] = false;
        for (int i = 0; i < sizeof(trowcols) / sizeof(trowcols[0]); ++i)
            if (select[i])
                seats[count++] = getSeat_rc(board, trowcols[i][0], trowcols[i][1]);
    } break;
    case ADVISOR: {
        int trowcols[][2] = {
            { frow - 1, fcol - 1 }, //SW
            { frow - 1, fcol + 1 }, //SE
            { frow + 1, fcol - 1 }, //NW
            { frow + 1, fcol + 1 } //NE
        };
        bool select[] = { true, true, true, true };
        if (fcol == ColMidLowIndex_)
            select[SW] = select[NW] = false;
        else if (fcol == ColMidUpIndex_)
            select[SE] = select[NE] = false;
        if (frow == (isBottom ? RowLowIndex_ : RowUpMidIndex_))
            select[SE] = select[SW] = false;
        else if (frow == (isBottom ? RowLowMidIndex_ : RowUpIndex_))
            select[NE] = select[NW] = false;
        for (int i = 0; i < sizeof(trowcols) / sizeof(trowcols[0]); ++i)
            if (select[i])
                seats[count++] = getSeat_rc(board, trowcols[i][0], trowcols[i][1]);
    } break;
    case BISHOP: {
        int trowcols[][2] = {
            { frow - 2, fcol - 2 }, //SW
            { frow - 2, fcol + 2 }, //SE
            { frow + 2, fcol - 2 }, //NW
            { frow + 2, fcol + 2 } //NE
        };
        bool select[] = { true, true, true, true };
        if (fcol == ColLowIndex_) // 最左列
            select[SW] = select[NW] = false;
        else if (fcol == ColUpIndex_) // 最右列
            select[SE] = select[NE] = false;
        if (frow == (isBottom ? RowLowIndex_ : RowUpLowIndex_)) // 最底行
            select[SW] = select[SE] = false;
        else if (frow == (isBottom ? RowLowUpIndex_ : RowUpIndex_)) // 最顶行
            select[NW] = select[NE] = false;

        for (int i = 0; i < sizeof(trowcols) / sizeof(trowcols[0]); ++i)
            if (select[i]) {
                int trow = trowcols[i][0], tcol = trowcols[i][1];
                if (getPiece_rc(board, (frow + trow) / 2, (fcol + tcol) / 2) == BLANKPIECE)
                    seats[count++] = getSeat_rc(board, trow, tcol);
            }
    } break;
    case KNIGHT: {
        int trowcols[][2] = {
            { frow - 2, fcol - 1 }, //SW
            { frow - 2, fcol + 1 }, //SE
            { frow + 2, fcol - 1 }, //NW
            { frow + 2, fcol + 1 }, //NE
            { frow - 1, fcol - 2 }, //WS
            { frow - 1, fcol + 2 }, //ES
            { frow + 1, fcol - 2 }, //WN
            { frow + 1, fcol + 2 } //EN
        };
        //             // {SW, SE, NW, NE, WS, ES, WN, EN}
        bool select[] = { true, true, true, true, true, true, true, true };
        if (fcol == ColLowIndex_) { // 最左列
            select[WS] = select[WN] = select[SW] = select[NW] = false;
            if (frow == RowLowIndex_) // 最底行
                select[SE] = select[ES] = false;
            else if (frow == RowLowIndex_ + 1) // 最底第二行
                select[SE] = false;
            else if (frow == RowUpIndex_) // 最顶行
                select[NE] = select[EN] = false;
            else if (frow == RowUpIndex_ - 1) // 最顶第二行
                select[NE] = false;
        } else if (fcol == ColUpIndex_) { // 最右列
            select[ES] = select[EN] = select[SE] = select[NE] = false;
            if (frow == RowLowIndex_)
                select[SW] = select[WS] = false;
            else if (frow == RowLowIndex_ + 1)
                select[SW] = false;
            else if (frow == RowUpIndex_)
                select[WN] = select[NW] = false;
            else if (frow == RowUpIndex_ - 1)
                select[NW] = false;
        } else if (fcol == ColLowIndex_ + 1) { // 最左第二列
            select[WS] = select[WN] = false;
            if (frow == RowLowIndex_)
                select[SW] = select[SE] = select[ES] = false;
            else if (frow == RowLowIndex_ + 1)
                select[SW] = select[SE] = false;
            else if (frow == RowUpIndex_)
                select[NW] = select[NE] = select[EN] = false;
            else if (frow == RowUpIndex_ - 1)
                select[NW] = select[NE] = false;
        } else if (fcol == ColUpIndex_ - 1) { // 最右第二列
            select[ES] = select[EN] = false;
            if (frow == RowLowIndex_)
                select[SW] = select[SE] = select[WS] = false;
            else if (frow == RowLowIndex_ + 1)
                select[SW] = select[SE] = false;
            else if (frow == RowUpIndex_)
                select[NW] = select[NE] = select[WN] = false;
            else if (frow == RowUpIndex_ - 1)
                select[NW] = select[NE] = false;
        } else {
            if (frow == RowLowIndex_) // 最底行
                select[SW] = select[WS] = select[SE] = select[ES] = false;
            else if (frow == RowLowIndex_ + 1) // 最底第二行
                select[SW] = select[SE] = false;
            else if (frow == RowUpIndex_) // 最顶行
                select[NE] = select[EN] = select[WN] = select[NW] = false;
            else if (frow == RowUpIndex_ - 1) // 最顶第二行
                select[NW] = select[NE] = false;
        }
        for (int i = 0; i < sizeof(trowcols) / sizeof(trowcols[0]); ++i)
            if (select[i]) {
                int trow = trowcols[i][0], tcol = trowcols[i][1];
                int legRow = abs(tcol - fcol) == 2 ? frow : (trow > frow ? frow + 1 : frow - 1),
                    legCol = abs(trow - frow) == 2 ? fcol : (tcol > fcol ? fcol + 1 : fcol - 1);
                if (getPiece_rc(board, legRow, legCol) == BLANKPIECE)
                    seats[count++] = getSeat_rc(board, trow, tcol);
            }
    } break;
    case PAWN: {
        int trowcols[][2] = {
            { frow, fcol - 1 }, //SW
            { frow, fcol + 1 }, //SE
            { frow - 1, fcol }, //NW
            { frow + 1, fcol } //NE
        };
        bool select[] = { true, true, true, true };
        if (isBottom) {
            if (frow != RowUpIndex_)
                select[NE] = false;
        } else if (frow != RowLowIndex_)
            select[NW] = false;
        if ((isBottom && frow > RowLowUpIndex_)
            || (!isBottom && frow <= RowLowUpIndex_)) { // 兵已过河
            if (fcol == ColLowIndex_)
                select[SW] = false;
            else if (fcol == ColUpIndex_)
                select[SE] = false;
        } else // 兵未过河
            select[SW] = select[SE] = false;
        for (int i = 0; i < sizeof(trowcols) / sizeof(trowcols[0]); ++i)
            if (select[i])
                seats[count++] = getSeat_rc(board, trowcols[i][0], trowcols[i][1]);
    } break;
    default: { // ROOK CANNON
        int trowcols[4][BOARDROW][2] = { 0 }; // 建立四个方向的位置数组
        int tcount[4] = { 0 }; // 每个方向的个数
        for (int col = fcol - 1; col >= ColLowIndex_; --col) {
            trowcols[0][tcount[0]][0] = frow;
            trowcols[0][tcount[0]++][1] = col;
        }
        for (int col = fcol + 1; col <= ColUpIndex_; ++col) {
            trowcols[1][tcount[1]][0] = frow;
            trowcols[1][tcount[1]++][1] = col;
        }
        for (int row = frow - 1; row >= RowLowIndex_; --row) {
            trowcols[2][tcount[2]][0] = row;
            trowcols[2][tcount[2]++][1] = fcol;
        }
        for (int row = frow + 1; row <= RowUpIndex_; ++row) {
            trowcols[3][tcount[3]][0] = row;
            trowcols[3][tcount[3]++][1] = fcol;
        }

        // 区分车、炮分别查找
        if (getKind(fpiece) == ROOK) {
            for (int line = 0; line < 4; ++line)
                for (int i = 0; i < tcount[line]; ++i) {
                    int trow = trowcols[line][i][0], tcol = trowcols[line][i][1];
                    seats[count++] = getSeat_rc(board, trow, tcol);
                    if (getPiece_rc(board, trow, tcol) != BLANKPIECE)
                        break;
                }
        } else { // CANNON
            for (int line = 0; line < 4; ++line) {
                bool skip = false;
                for (int i = 0; i < tcount[line]; ++i) {
                    int trow = trowcols[line][i][0], tcol = trowcols[line][i][1];
                    Seat tseat = getSeat_rc(board, trow, tcol);
                    Piece tpiece = getPiece_rc(board, trow, tcol);
                    if (!skip) {
                        if (tpiece == BLANKPIECE)
                            seats[count++] = tseat;
                        else
                            skip = true;
                    } else if (tpiece != BLANKPIECE) {
                        seats[count++] = tseat;
                        break;
                    }
                }
            }
        }
    } break;
    }
    return filterMoveSeats(seats, count, board, fseat, moveSameColor, false);
}

static bool moveSameColor(Board board, Seat fseat, Seat tseat)
{
    return getColor(getPiece_s(fseat)) == getColor(getPiece_s(tseat));
}

static bool moveKilled(Board board, Seat fseat, Seat tseat)
{
    PieceColor fcolor = getColor(getPiece_s(fseat));
    Piece eatPiece = movePiece(board, fseat, tseat, BLANKPIECE);
    bool isKill = isKilled(board, fcolor);
    movePiece(board, tseat, fseat, eatPiece);
    return isKill;
}

static int filterMoveSeats(Seat* seats, int count, Board board, Seat fseat,
    bool (*__filterFunc)(Board, Seat, Seat), bool reverse)
{
    int index = 0;
    while (index < count) {
        bool result = __filterFunc(board, fseat, seats[index]);
        if (reverse)
            result = !result;
        // 如符合条件，先减少count再比较序号，如小于则需要交换；如不小于则指向同一元素，不需要交换；
        if (result && index < --count) {
            Seat tempSeat = seats[count];
            seats[count] = seats[index];
            seats[index] = tempSeat;
        } else
            ++index; // 不符合筛选条件，index前进一个
    }
    return count;
}

int canMoveSeats(Seat* seats, int count, Board board, Seat fseat)
{
    return filterMoveSeats(seats, count, board, fseat, moveKilled, false);
}

int killedMoveSeats(Seat* seats, int count, Board board, Seat fseat)
{
    return filterMoveSeats(seats, count, board, fseat, moveKilled, true);
}

Piece movePiece(Board board, Seat fseat, Seat tseat, Piece eatPiece)
{
    Piece tpiece = getPiece_s(tseat);
    setSeat(tpiece, NULL);
    setPiece_s(tseat, getPiece_s(fseat));
    setPiece_s(fseat, eatPiece);
    return tpiece;
}

void changeBoard(Board board, ChangeType ct)
{
    switch (ct) {
    case EXCHANGE: {
        struct Board boardBak = *board; // 复制结构
        for (int i = 0; i < SIDEPIECENUM; ++i) {
            Piece piece = getPiece_i(boardBak.pieces, RED, i),
                  othPiece = getPiece_i(boardBak.pieces, BLACK, i);
            Seat seat = getSeat_p(piece),
                 othSeat = getSeat_p(othPiece);
            setSeat(piece, NULL);
            setSeat(othPiece, NULL);
            setPiece_s(seat, othPiece);
            setPiece_s(othSeat, piece);
        }
        board->bottomColor = !(board->bottomColor);
    } break;
    case ROTATE:
        for (int row = 0; row < BOARDROW / 2; ++row)
            for (int col = 0; col < BOARDCOL; ++col) {
                int othRow = getOtherRow_r(row), othCol = getOtherCol_c(col);
                Piece piece = getPiece_rc(board, row, col),
                      othPiece = getPiece_rc(board, othRow, othCol);
                setSeat(piece, NULL);
                setSeat(othPiece, NULL);
                setPiece_rc(board, row, col, othPiece);
                setPiece_rc(board, othRow, othCol, piece);
            }
        board->bottomColor = !(board->bottomColor);
        break;
    case SYMMETRY:
        for (int row = 0; row < BOARDROW; ++row)
            for (int col = 0; col < BOARDCOL / 2; ++col) {
                int othCol = getOtherCol_c(col);
                Piece piece = getPiece_rc(board, row, col),
                      othPiece = getPiece_rc(board, row, othCol);
                setSeat(piece, NULL);
                setSeat(othPiece, NULL);
                setPiece_rc(board, row, col, othPiece);
                setPiece_rc(board, row, othCol, piece);
            }
        break;
    default:
        break;
    }
}

wchar_t* getSeatString(wchar_t* seatStr, Board board, Seat seat)
{
    wchar_t str[WCHARSIZE];
    swprintf(seatStr, WCHARSIZE, L"%02x%s", getRowCol_s(seat), getPieString(str, getPiece_s(seat)));
    return seatStr;
}

wchar_t* getBoardString(wchar_t* boardStr, Board board)
{ //*
    const wchar_t boardStr_t[] = L"┏━┯━┯━┯━┯━┯━┯━┯━┓\n"
                                 "┃　│　│　│╲│╱│　│　│　┃\n"
                                 "┠─┼─┼─┼─╳─┼─┼─┼─┨\n"
                                 "┃　│　│　│╱│╲│　│　│　┃\n"
                                 "┠─╬─┼─┼─┼─┼─┼─╬─┨\n"
                                 "┃　│　│　│　│　│　│　│　┃\n"
                                 "┠─┼─╬─┼─╬─┼─╬─┼─┨\n"
                                 "┃　│　│　│　│　│　│　│　┃\n"
                                 "┠─┴─┴─┴─┴─┴─┴─┴─┨\n"
                                 "┃　　　　　　　　　　　　　　　┃\n"
                                 "┠─┬─┬─┬─┬─┬─┬─┬─┨\n"
                                 "┃　│　│　│　│　│　│　│　┃\n"
                                 "┠─┼─╬─┼─╬─┼─╬─┼─┨\n"
                                 "┃　│　│　│　│　│　│　│　┃\n"
                                 "┠─╬─┼─┼─┼─┼─┼─╬─┨\n"
                                 "┃　│　│　│╲│╱│　│　│　┃\n"
                                 "┠─┼─┼─┼─╳─┼─┼─┼─┨\n"
                                 "┃　│　│　│╱│╲│　│　│　┃\n"
                                 "┗━┷━┷━┷━┷━┷━┷━┷━┛\n"; // 边框粗线，输出文本文件使用
    //*/
    /*/
    wchar_t boardStr_t[] = L"　　＋－－－－－－－－－－－－－－－＋\n"
                           "　　︱　︱　︱　︱＼︱／︱　︱　︱　︱\n"
                           "　　︱－＋－＋－＋－＋－＋－＋－＋－︱\n"
                           "　　︱　︱　︱　︱／︱＼︱　︱　︱　︱\n"
                           "　　︱－＋－＋－＋－＋－＋－＋－＋－︱\n"
                           "　　︱　︱　︱　︱　︱　︱　︱　︱　︱\n"
                           "　　︱－＋－＋－＋－＋－＋－＋－＋－︱\n"
                           "　　︱　︱　︱　︱　︱　︱　︱　︱　︱\n"
                           "　　︱－－－－－－－－－－－－－－－︱\n"
                           "　　︱　　　　　　　　　　　　　　　︱\n"
                           "　　︱－－－－－－－－－－－－－－－︱\n"
                           "　　︱　︱　︱　︱　︱　︱　︱　︱　︱\n"
                           "　　︱－＋－＋－＋－＋－＋－＋－＋－︱\n"
                           "　　︱　︱　︱　︱　︱　︱　︱　︱　︱\n"
                           "　　︱－＋－＋－＋－＋－＋－＋－＋－︱\n"
                           "　　︱　︱　︱　︱＼︱／︱　︱　︱　︱\n"
                           "　　︱－＋－＋－＋－＋－＋－＋－＋－︱\n"
                           "　　︱　︱　︱　︱／︱＼︱　︱　︱　︱\n"
                           "　　＋－－－－－－－－－－－－－－－＋\n"; //全角字符，输出控制台屏幕使用
    //*/
    wcscpy(boardStr, boardStr_t);
    for (int row = 0; row < BOARDROW; ++row)
        for (int col = 0; col < BOARDCOL; ++col) {
            Piece piece = getPiece_rc(board, row, col);
            if (piece != BLANKPIECE)
                boardStr[(BOARDROW - row - 1) * 2 * (BOARDCOL * 2) + col * 2] = getPieName_T(piece);
        }
    return boardStr;
}

// 棋盘上边标识字符串
wchar_t* getBoardPreString(wchar_t* preStr, Board board)
{
    const wchar_t* PRESTR[] = {
        L"　　　　　　　黑　方　　　　　　　\n１　２　３　４　５　６　７　８　９\n",
        L"　　　　　　　红　方　　　　　　　\n一　二　三　四　五　六　七　八　九\n"
    };
    return wcscpy(preStr, PRESTR[board->bottomColor]);
}

// 棋盘下边标识字符串
wchar_t* getBoardSufString(wchar_t* sufStr, Board board)
{
    const wchar_t* SUFSTR[] = {
        L"九　八　七　六　五　四　三　二　一\n　　　　　　　红　方　　　　　　　\n",
        L"９　８　７　６　５　４　３　２　１\n　　　　　　　黑　方　　　　　　　\n"
    };
    return wcscpy(sufStr, SUFSTR[board->bottomColor]);
}

// 测试本翻译单元各种对象、函数
void testBoard(FILE* fout)
{
    fwprintf(fout, L"\ntestBoard：\n");
    wchar_t* FENs[] = {
        L"rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR",
        L"5a3/4ak2r/6R2/8p/9/9/9/B4N2B/4K4/3c5",
        L"2b1kab2/4a4/4c4/9/9/3R5/9/1C7/4r4/2BK2B2",
        L"4kab2/4a4/4b4/3N5/9/4N4/4n4/4B4/4A4/3AK1B2"
    };
    Board board = newBoard();
    for (int i = 0; i < 4; ++i) {
        fwprintf(fout, L"第%d个FEN =============================================\n", i);

        //* FEN转换成PieChars
        wchar_t pieChars[SEATNUM + 1];
        setPieCharsFromFEN(pieChars, FENs[i]);
        fwprintf(fout, L"     FEN:%s size:%d\nPieChars:%s size:%d\n",
            FENs[i], wcslen(FENs[i]), pieChars, wcslen(pieChars));
        //*/

        //* 设置棋局，生成PieChars，转换成FEN
        setBoard(board, pieChars);

        getPieChars_board(pieChars, board);
        wchar_t FEN[SEATNUM + 1];
        setFENFromPieChars(FEN, pieChars);
        fwprintf(fout, L"PieChars:%s size:%d\n     FEN:%s size:%d\n",
            pieChars, wcslen(pieChars), FEN, wcslen(FEN));
        //*/

        wchar_t boardStr[SUPERWIDEWCHARSIZE], preStr[WCHARSIZE], sufStr[WCHARSIZE], seatStr[WCHARSIZE];
        fwprintf(fout, L"%s%s%sboard：@%p bottomColor:%d %s\n",
            getBoardPreString(preStr, board),
            getBoardString(boardStr, board),
            getBoardSufString(sufStr, board),
            board,
            board->bottomColor,
            getSeatString(seatStr, board, getKingSeat(board, board->bottomColor)));
        //*// 打印棋局
        for (int ct = EXCHANGE; ct <= SYMMETRY; ++ct) {
            changeBoard(board, ct);
            fwprintf(fout, L"%s%s%sboard：@%p bottomColor:%d %s ct:%d\n",
                getBoardPreString(preStr, board),
                getBoardString(boardStr, board),
                getBoardSufString(sufStr, board),
                board,
                board->bottomColor,
                getSeatString(seatStr, board, getKingSeat(board, board->bottomColor)),
                ct);
        }
        //*/

        //* 取得将帅位置
        Seat rkseat = getKingSeat(board, RED),
             bkseat = getKingSeat(board, BLACK);
        fwprintf(fout, L"%s <==> %s\n",
            getSeatString(preStr, board, rkseat), getSeatString(sufStr, board, bkseat));
        //*/

        /* 取得各棋子的可放置位置
        for (int color = RED; color <= BLACK; ++color)
            for (int index = 0; index < SIDEPIECENUM; ++index) {
                Piece piece = getPiece_i(board->pieces, color, index);
                Seat seats[BOARDROW * BOARDCOL] = { 0 };
                int count = putSeats(seats, board, true, getKind(piece));
                fwprintf(fout, L"%s =【", getPieString(preStr, piece));
                for (int i = 0; i < count; ++i)
                    fwprintf(fout, L" %s ", getSeatString(preStr, board, seats[i]));
                fwprintf(fout, L"】%d\n", count);
            }
        //*/

        //* 取得各种条件下活的棋子
        for (int color = RED; color <= BLACK; ++color) {
            Seat lvseats[PIECENUM] = { NULL };
            int count = getLiveSeats(lvseats, board, color, ALLPIENAME, ALLCOL);
            fwprintf(fout, L"%c：", color == RED ? L'红' : L'黑');
            for (int i = 0; i < count; ++i)
                fwprintf(fout, L"%s ", getSeatString(preStr, board, lvseats[i]));
            fwprintf(fout, L"count:%d\n", count);
        }
        //*/

        //* 取得各活棋子的可移动位置
        for (int color = RED; color <= BLACK; ++color) {
            Seat lvseats[PIECENUM] = { NULL };
            int count = getLiveSeats(lvseats, board, color, ALLPIENAME, ALLCOL);
            for (int i = 0; i < count; ++i) {
                Seat fseat = lvseats[i];
                fwprintf(fout, L"%s >>【", getSeatString(preStr, board, fseat));

                Seat mseats[BOARDROW + BOARDCOL] = { NULL };
                int mcount = moveSeats(mseats, board, fseat);
                for (int i = 0; i < mcount; ++i)
                    fwprintf(fout, L"%s ", getSeatString(preStr, board, mseats[i]));
                fwprintf(fout, L"】%d", mcount);

                fwprintf(fout, L" =【");
                int cmcount = canMoveSeats(mseats, mcount, board, fseat);
                for (int i = 0; i < cmcount; ++i)
                    fwprintf(fout, L"%s ", getSeatString(preStr, board, mseats[i]));
                fwprintf(fout, L"】%d", cmcount);

                fwprintf(fout, L" +【");
                int kmcount = killedMoveSeats(mseats, mcount, board, fseat);
                for (int i = 0; i < kmcount; ++i)
                    fwprintf(fout, L"%s ", getSeatString(preStr, board, mseats[i]));
                fwprintf(fout, L"】%d\n", kmcount);
            }
        }
        //*/
        fwprintf(fout, L"\n");
    }
    freeBoard(board);
}
