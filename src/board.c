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

static bool isValidRow__(int row) { return row >= RowLowIndex_ && row <= RowUpIndex_; }
static bool isValidCol__(int col) { return col >= ColLowIndex_ && col <= ColUpIndex_; }

Board newBoard(void)
{
    Board board = malloc(sizeof(struct Board));
    assert(board);
    board->pieces = newPieces();
    for (int r = 0; r < BOARDROW; ++r)
        for (int c = 0; c < BOARDCOL; ++c) {
            Seat seat = getSeat_rc(board, r, c);
            seat->row = r;
            seat->col = c;
            seat->piece = getBlankPiece();
        }
    board->bottomColor = RED;
    return board;
}

void delBoard(Board board)
{
    delPieces(board->pieces);
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
    /*
    if (!(row >= 0 && row < BOARDROW && col >= 0 && col < BOARDCOL)) {
        wchar_t wstr[2 * WIDEWCHARSIZE];
        wprintf(L"%srow:%d col:%d\n", getBoardString(wstr, board), row, col);
    }
    //*/
    assert(isValidRow__(row) && isValidCol__(col));
    return &board->seats[row][col];
}
inline Seat getSeat_rowcol(Board board, int rowcol) { return getSeat_rc(board, getRow_rowcol(rowcol), getCol_rowcol(rowcol)); }

inline Piece getPiece_s(CSeat seat) { return seat->piece; }
inline Piece getPiece_rc(Board board, int row, int col) { return getPiece_s(getSeat_rc(board, row, col)); }
inline Piece getPiece_rowcol(Board board, int rowcol) { return getPiece_s(getSeat_rowcol(board, rowcol)); }

// 置入某棋盘内某位置一个棋子
static void setPiece_s__(Seat seat, Piece piece)
{
    if (seat)
        setSeat(seat->piece = piece, seat);
}

// 置入某棋盘内某行、某列位置一个棋子
static void setPiece_rc__(Board board, int row, int col, Piece piece) { setPiece_s__(getSeat_rc(board, row, col), piece); }

wchar_t* getPieChars_board(wchar_t* pieChars, Board board)
{
    int index = 0;
    for (int row = BOARDROW - 1; row >= 0; --row)
        for (int col = 0; col < BOARDCOL; ++col)
            pieChars[index++] = getChar(getPiece_rc(board, row, col));
    pieChars[SEATNUM] = L'\x0';
    return pieChars;
}

wchar_t* getFEN_pieChars(wchar_t* FEN, const wchar_t* pieChars)
{
    int fi = 0, pi = 0;
    for (int row = 0; row < BOARDROW; ++row) {
        int blankNum = 0;
        for (int col = 0; col < BOARDCOL; ++col, ++pi) {
            if (iswalpha(pieChars[pi])) {
                if (blankNum > 0)
                    FEN[fi++] = L'0' + blankNum;
                FEN[fi++] = pieChars[pi];
                blankNum = 0;
            } else //if (pieChars[pi] == getBlankChar()) // 肯定为真
                blankNum++;
        }
        if (blankNum > 0)
            FEN[fi++] = L'0' + blankNum;
        FEN[fi++] = L'/'; // 插入行分隔符
    }
    FEN[--fi] = L'\x0';
    return FEN;
}

wchar_t* getFEN_board(wchar_t* FEN, Board board)
{
    wchar_t pieChars[SEATNUM + 1];
    return getFEN_pieChars(FEN, getPieChars_board(pieChars, board));
}

wchar_t* getPieChars_FEN(wchar_t* pieChars, const wchar_t* FEN)
{
    int len = wcslen(FEN);
    for (int i = 0, index = 0; i < len && index < SEATNUM; ++i) {
        wchar_t ch = FEN[i];
        if (iswdigit(ch))
            for (int j = ch - L'0'; j > 0; --j)
                pieChars[index++] = getBlankChar();
        else if (iswalpha(ch))
            pieChars[index++] = ch;
    }
    pieChars[SEATNUM] = L'\x0';
    return pieChars;
}

static void resetPiece__(Piece piece, void* ptr)
{
    Seat seat = getSeat_p(piece);
    setNullSeat(piece);
    setPiece_s__(seat, getBlankPiece());
}

void resetBoard(Board board)
{
    piecesMap(board->pieces, resetPiece__, NULL);
}

void setBoard_pieChars(Board board, const wchar_t* pieChars)
{
    assert(wcslen(pieChars) == SEATNUM);
    resetBoard(board);
    int index = 0;
    for (int row = BOARDROW - 1; row >= 0; --row)
        for (int col = 0; col < BOARDCOL; ++col)
            setPiece_rc__(board, row, col, getPiece_ch(board->pieces, pieChars[index++]));
    board->bottomColor = getRow_s(getKingSeat(board, RED)) < RowLowUpIndex_ ? RED : BLACK;
}

void setBoard_FEN(Board board, const wchar_t* FEN)
{
    wchar_t pieChars[SEATNUM + 1];
    setBoard_pieChars(board, getPieChars_FEN(pieChars, FEN));
}

inline bool isBottomSide(CBoard board, PieceColor color) { return board->bottomColor == color; }

Seat getKingSeat(Board board, PieceColor color) { return getSeat_p(getKingPiece(board->pieces, color)); }

inline static bool isName__(Seat seat, wchar_t name, int col) { return getPieName(getPiece_s(seat)) == name; }

inline static bool isNameCol__(Seat seat, wchar_t name, int col) { return isName__(seat, name, col) && getCol_s(seat) == col; }

static int seatCmp__(const void* first, const void* second)
{
    Seat aseat = (Seat)first, bseat = (Seat)second;
    int rowdiff = getRow_s(aseat) - getRow_s(bseat);
    if (rowdiff == 0)
        return getCol_s(aseat) - getCol_s(bseat);
    else
        return rowdiff;
}

static int filterLiveSeats__(Seat* seats, int count, bool (*func)(Seat, wchar_t, int), wchar_t name, int col)
{
    int index = 0;
    while (index < count) {
        if (func(seats[index], name, col))
            ++index;
        else
            seats[index] = seats[--count];
    }
    qsort(seats, index, sizeof(Seat*), seatCmp__);
    return index;
}

int getLiveSeats_cn(Seat* seats, Board board, PieceColor color, wchar_t name)
{
    int count = getLiveSeats_c(seats, board->pieces, color);
    return filterLiveSeats__(seats, count, isName__, name, 0);
}

int getLiveSeats_cnc(Seat* seats, Board board, PieceColor color, wchar_t name, int col)
{
    int count = getLiveSeats_c(seats, board->pieces, color);
    return filterLiveSeats__(seats, count, isNameCol__, name, col);
}

int getSortPawnLiveSeats(Seat* seats, Board board, PieceColor color, wchar_t name)
{
    Seat pawnSeats[SIDEPIECENUM] = { NULL };
    int liveCount = getLiveSeats_cn(pawnSeats, board, color, name),
        colCount[BOARDCOL] = { 0 };
    Seat colSeats[BOARDCOL][BOARDROW] = { NULL };
    for (int i = 0; i < liveCount; ++i) {
        int col = getCol_s(pawnSeats[i]);
        colSeats[col][colCount[col]++] = pawnSeats[i];
    }
    int count = 0;
    if (isBottomSide(board, color)) {
        for (int col = BOARDCOL - 1; col >= 0; --col)
            if (colCount[col] > 1) // 筛除小于2个的列
                for (int i = colCount[col] - 1; i >= 0; --i)
                    seats[count++] = colSeats[col][i];
    } else {
        for (int col = 0; col < BOARDCOL; ++col)
            if (colCount[col] > 1) // 筛除小于2个的列
                for (int i = 0; i < colCount[col]; ++i)
                    seats[count++] = colSeats[col][i];
    }
    return count;
}

bool kingIsFace(Board board, PieceColor color)
{
    bool isBottom = isBottomSide(board, color);
    Seat kingSeat = getKingSeat(board, color),
         othKingSeat = getKingSeat(board, getOtherColor(color));
    int fcol = getCol_s(kingSeat);
    if (fcol != getCol_s(othKingSeat))
        return false;
    int frow = getRow_s(kingSeat), trow = getRow_s(othKingSeat),
        rowLow = isBottom ? frow : trow, rowUp = isBottom ? trow : frow;
    for (int row = rowLow + 1; row < rowUp; ++row)
        if (!isBlankPiece(getPiece_rc(board, row, fcol)))
            return false;
    // 将帅之间全空，没有间隔棋子
    return true;
}

bool kingIsKilled(Board board, PieceColor color)
{
    Seat kingSeat = getKingSeat(board, color), seats[SIDEPIECENUM], mseats[BOARDROW + BOARDCOL];
    int count = getLiveSeats_cs(seats, board->pieces, getOtherColor(color));
    for (int i = 0; i < count; ++i) {
        int mcount = moveSeats(mseats, board, seats[i]);
        for (int m = 0; m < mcount; ++m)
            // 本方将帅位置在对方强子可走位置范围内
            if (mseats[m] == kingSeat)
                return true;
    }
    return false;
}

bool isUnableMove(Board board, PieceColor color)
{
    Seat fseats[SIDEPIECENUM], mseats[BOARDROW + BOARDCOL];
    int count = getLiveSeats_c(fseats, board->pieces, color);
    for (int i = 0; i < count; ++i)
        if (moveSeats(mseats, board, fseats[i]) > 0)
            return false;
    return true;
}

bool isFail(Board board, PieceColor color)
{
    return kingIsFace(board, color) || kingIsKilled(board, color) || isUnableMove(board, color);
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

static int filterMoveSeats__(Seat* seats, int count, Board board, Seat fseat,
    bool (*filterFunc__)(Board, Seat, Seat), bool reverse)
{
    int index = 0;
    while (index < count) {
        bool result = filterFunc__(board, fseat, seats[index]);
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

static bool moveSameColor__(Board board, Seat fseat, Seat tseat)
{
    return getColor(getPiece_s(fseat)) == getColor(getPiece_s(tseat));
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
                if (isBlankPiece(getPiece_rc(board, (frow + trow) / 2, (fcol + tcol) / 2)))
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
        for (int i = 0; i < sizeof(trowcols) / sizeof(trowcols[0]); ++i) {
            int trow = trowcols[i][0], tcol = trowcols[i][1];
            if (isValidRow__(trow) && isValidCol__(tcol)) {
                int legRow = frow + (abs(trow - frow) == 1 ? 0 : (trow > frow ? 1 : -1)),
                    legCol = fcol + (abs(tcol - fcol) == 1 ? 0 : (tcol > fcol ? 1 : -1));
                if (isBlankPiece(getPiece_rc(board, legRow, legCol)))
                    seats[count++] = getSeat_rc(board, trow, tcol);
            }
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
                    if (!isBlankPiece(getPiece_rc(board, trow, tcol)))
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
                        if (isBlankPiece(tpiece))
                            seats[count++] = tseat;
                        else
                            skip = true;
                    } else if (!isBlankPiece(tpiece)) {
                        seats[count++] = tseat;
                        break;
                    }
                }
            }
        }
    } break;
    }
    return filterMoveSeats__(seats, count, board, fseat, moveSameColor__, false);
}

static bool moveToFail__(Board board, Seat fseat, Seat tseat)
{
    PieceColor fcolor = getColor(getPiece_s(fseat));
    Piece eatPiece = movePiece(fseat, tseat, getBlankPiece());
    bool fail = isFail(board, fcolor);
    movePiece(tseat, fseat, eatPiece);
    return fail;
}

int ableMoveSeats(Seat* seats, int count, Board board, Seat fseat)
{
    return filterMoveSeats__(seats, count, board, fseat, moveToFail__, false);
}

int unableMoveSeats(Seat* seats, int count, Board board, Seat fseat)
{
    return filterMoveSeats__(seats, count, board, fseat, moveToFail__, true);
}

Piece movePiece(Seat fseat, Seat tseat, Piece eatPiece)
{
    Piece tpiece = getPiece_s(tseat);
    setNullSeat(tpiece);
    setPiece_s__(tseat, getPiece_s(fseat));
    setPiece_s__(fseat, eatPiece);
    return tpiece;
}

static void exchangePiece__(Piece piece, void* ptr)
{
    assert(ptr);
    if (getColor(piece) == BLACK)
        return; // 只执行一半棋子即已完成交换
    Pieces pieces = (Pieces)ptr;
    Piece othPiece = getOtherPiece(pieces, piece);
    Seat seat = getSeat_p(piece),
         othSeat = getSeat_p(othPiece);
    setNullSeat(piece);
    setNullSeat(othPiece);
    setPiece_s__(seat, othPiece);
    setPiece_s__(othSeat, piece);
}

void changeBoard(Board board, ChangeType ct)
{
    switch (ct) {
    case EXCHANGE: {
        struct Board boardBak = *board; // 复制结构
        piecesMap(boardBak.pieces, exchangePiece__, boardBak.pieces);
        board->bottomColor = !(board->bottomColor);
    } break;
    case ROTATE:
        for (int row = 0; row < BOARDROW / 2; ++row)
            for (int col = 0; col < BOARDCOL; ++col) {
                int othRow = getOtherRow_r(row), othCol = getOtherCol_c(col);
                Piece piece = getPiece_rc(board, row, col),
                      othPiece = getPiece_rc(board, othRow, othCol);
                setNullSeat(piece);
                setNullSeat(othPiece);
                setPiece_rc__(board, row, col, othPiece);
                setPiece_rc__(board, othRow, othCol, piece);
            }
        board->bottomColor = !(board->bottomColor);
        break;
    case SYMMETRY:
        for (int row = 0; row < BOARDROW; ++row)
            for (int col = 0; col < BOARDCOL / 2; ++col) {
                int othCol = getOtherCol_c(col);
                Piece piece = getPiece_rc(board, row, col),
                      othPiece = getPiece_rc(board, row, othCol);
                setNullSeat(piece);
                setNullSeat(othPiece);
                setPiece_rc__(board, row, col, othPiece);
                setPiece_rc__(board, row, othCol, piece);
            }
        break;
    default:
        break;
    }
}

wchar_t* getSeatString(wchar_t* seatStr, CSeat seat)
{
    wchar_t str[WCHARSIZE];
    swprintf(seatStr, WCHARSIZE, L"%02x%s", getRowCol_s(seat), getPieString(str, getPiece_s(seat)));
    return seatStr;
}

wchar_t* getBoardString_pieceChars(wchar_t* boardStr, const wchar_t* pieChars)
{ //*
    static const wchar_t boardStr_t[] = L"┏━┯━┯━┯━┯━┯━┯━┯━┓\n"
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
            wchar_t ch = *pieChars++;
            if (ch != getBlankChar())
                boardStr[row * 2 * (BOARDCOL * 2) + col * 2] = getPieName_T_ch(ch);
        }
    return boardStr;
}

wchar_t* getBoardString(wchar_t* boardStr, Board board)
{
    wchar_t pieChars[SEATNUM + 1];
    return getBoardString_pieceChars(boardStr, getPieChars_board(pieChars, board));
}

// 棋盘上边标识字符串
wchar_t* getBoardPreString(wchar_t* preStr, CBoard board)
{
    const wchar_t* PRESTR[] = {
        L"　　　　　　　黑　方　　　　　　　\n１　２　３　４　５　６　７　８　９\n",
        L"　　　　　　　红　方　　　　　　　\n一　二　三　四　五　六　七　八　九\n"
    };
    return wcscpy(preStr, PRESTR[board->bottomColor]);
}

// 棋盘下边标识字符串
wchar_t* getBoardSufString(wchar_t* sufStr, CBoard board)
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
        getPieChars_FEN(pieChars, FENs[i]);
        fwprintf(fout, L"     FEN:%s size:%d\nPieChars:%s size:%d\n",
            FENs[i], wcslen(FENs[i]), pieChars, wcslen(pieChars));
        //*/

        //* 设置棋局，生成PieChars，转换成FEN
        setBoard_pieChars(board, pieChars);

        getPieChars_board(pieChars, board);
        wchar_t FEN[SEATNUM + 1];
        getFEN_pieChars(FEN, pieChars);
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
            getSeatString(seatStr, getKingSeat(board, board->bottomColor)));
        //*// 打印棋局
        for (int ct = EXCHANGE; ct <= SYMMETRY; ++ct) {
            changeBoard(board, ct);
            fwprintf(fout, L"%s%s%sboard：@%p bottomColor:%d %s ct:%d\n",
                getBoardPreString(preStr, board),
                getBoardString(boardStr, board),
                getBoardSufString(sufStr, board),
                board,
                board->bottomColor,
                getSeatString(seatStr, getKingSeat(board, board->bottomColor)),
                ct);
        }
        //*/

        //* 取得将帅位置
        Seat rkseat = getKingSeat(board, RED),
             bkseat = getKingSeat(board, BLACK);
        fwprintf(fout, L"%s <==> %s\n",
            getSeatString(preStr, rkseat), getSeatString(sufStr, bkseat));
        //*/

        /* 取得各棋子的可放置位置
        for (int color = RED; color <= BLACK; ++color) {
            Seat pseats[SIDEPIECENUM];
            int pcount = getLiveSeats_c(pseats, board->pieces, color);
            for (int i = 0; i < pcount; ++i) {
                Piece piece = getPiece_s(pseats[i]);
                Seat seats[BOARDROW * BOARDCOL] = { 0 };
                int count = putSeats(seats, board, true, getKind(piece));
                fwprintf(fout, L"%s =【", getPieString(preStr, piece));
                for (int i = 0; i < count; ++i)
                    fwprintf(fout, L" %s ", getSeatString(preStr, seats[i]));
                fwprintf(fout, L"】%d\n", count);
            }
        }
        //*/

        //* 取得各种条件下活的棋子
        for (int color = RED; color <= BLACK; ++color) {
            Seat lvseats[PIECENUM] = { NULL };
            int count = getLiveSeats_c(lvseats, board->pieces, color);
            fwprintf(fout, L"%c：", color == RED ? L'红' : L'黑');
            for (int i = 0; i < count; ++i)
                fwprintf(fout, L"%s ", getSeatString(preStr, lvseats[i]));
            fwprintf(fout, L"count:%d\n", count);
        }
        //*/

        //* 取得各活棋子的可移动位置
        for (int color = RED; color <= BLACK; ++color) {
            Seat lvseats[PIECENUM] = { NULL };
            int count = getLiveSeats_c(lvseats, board->pieces, color);
            for (int i = 0; i < count; ++i) {
                Seat fseat = lvseats[i];
                fwprintf(fout, L"%s >>【", getSeatString(preStr, fseat));

                Seat mseats[BOARDROW + BOARDCOL] = { NULL };
                int mcount = moveSeats(mseats, board, fseat);
                for (int i = 0; i < mcount; ++i)
                    fwprintf(fout, L"%s ", getSeatString(preStr, mseats[i]));
                fwprintf(fout, L"】%d", mcount);

                fwprintf(fout, L" =【");
                int cmcount = ableMoveSeats(mseats, mcount, board, fseat);
                for (int i = 0; i < cmcount; ++i)
                    fwprintf(fout, L"%s ", getSeatString(preStr, mseats[i]));
                fwprintf(fout, L"】%d", cmcount);

                fwprintf(fout, L" +【");
                int kmcount = unableMoveSeats(mseats, mcount, board, fseat);
                for (int i = 0; i < kmcount; ++i)
                    fwprintf(fout, L"%s ", getSeatString(preStr, mseats[i]));
                fwprintf(fout, L"】%d\n", kmcount);
            }
        }
        //*/
        fwprintf(fout, L"\n");
    }
    delBoard(board);
}
