#include "head/board.h"
#include "head/piece.h"

// 棋盘位置的全局常量
static const Seat SEATS[BOARDROW][BOARDCOL] = {
    { { 0, 0 }, { 0, 1 }, { 0, 2 }, { 0, 3 }, { 0, 4 }, { 0, 5 }, { 0, 6 }, { 0, 7 }, { 0, 8 } },
    { { 1, 0 }, { 1, 1 }, { 1, 2 }, { 1, 3 }, { 1, 4 }, { 1, 5 }, { 1, 6 }, { 1, 7 }, { 1, 8 } },
    { { 2, 0 }, { 2, 1 }, { 2, 2 }, { 2, 3 }, { 2, 4 }, { 2, 5 }, { 2, 6 }, { 2, 7 }, { 2, 8 } },
    { { 3, 0 }, { 3, 1 }, { 3, 2 }, { 3, 3 }, { 3, 4 }, { 3, 5 }, { 3, 6 }, { 3, 7 }, { 3, 8 } },
    { { 4, 0 }, { 4, 1 }, { 4, 2 }, { 4, 3 }, { 4, 4 }, { 4, 5 }, { 4, 6 }, { 4, 7 }, { 4, 8 } },
    { { 5, 0 }, { 5, 1 }, { 5, 2 }, { 5, 3 }, { 5, 4 }, { 5, 5 }, { 5, 6 }, { 5, 7 }, { 5, 8 } },
    { { 6, 0 }, { 6, 1 }, { 6, 2 }, { 6, 3 }, { 6, 4 }, { 6, 5 }, { 6, 6 }, { 6, 7 }, { 6, 8 } },
    { { 7, 0 }, { 7, 1 }, { 7, 2 }, { 7, 3 }, { 7, 4 }, { 7, 5 }, { 7, 6 }, { 7, 7 }, { 7, 8 } },
    { { 8, 0 }, { 8, 1 }, { 8, 2 }, { 8, 3 }, { 8, 4 }, { 8, 5 }, { 8, 6 }, { 8, 7 }, { 8, 8 } },
    { { 9, 0 }, { 9, 1 }, { 9, 2 }, { 9, 3 }, { 9, 4 }, { 9, 5 }, { 9, 6 }, { 9, 7 }, { 9, 8 } }
};

// 着法相关的字符数组静态全局变量
/* static const wchar_t NAMECHAR[] = L"帅将仕士相象马车炮兵卒";
static const wchar_t PRECHAR[] = L"前中后";
static const wchar_t MOVCHAR[] = L"退平进";
static const wchar_t NUMCHAR[PIECECOLORNUM][BOARDCOL + 1] = {
    L"一二三四五六七八九", L"１２３４５６７８９"
};
static const wchar_t FEN_0[] = L"rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR";
*/

// 棋盘行列相关的静态全局变量
static const int RowLowIndex_ = 0, RowLowMidIndex_ = 2, RowLowUpIndex_ = 4,
                 RowUpLowIndex_ = 5, RowUpMidIndex_ = 7, RowUpIndex_ = 9,
                 ColLowIndex_ = 0, ColMidLowIndex_ = 3, ColMidUpIndex_ = 5, ColUpIndex_ = 8;

const Seat* getSeat_rc(int row, int col)
{
    return &SEATS[row][col];
}

const Seat* getSeat_i(int rowcol)
{
    return getSeat_rc(rowcol / 10, rowcol % 10);
}

// 根据seat获取字面行列整数
inline static int getRowcol(const Seat* seat)
{
    return seat->row * 10 + seat->col;
}

// 从某棋盘内某行、某列位置取得一个棋子
inline static const Piece* getPiece_rc(const Board* board,
    int row, int col)
{
    return board->pieces[row][col];
}

inline static const Piece* getPiece_s(const Board* board, const Seat* seat)
{
    return getPiece_rc(board, seat->row, seat->col);
}

// 置入某棋盘内某行、某列位置一个棋子
inline static void setPiece_rc(Board* board, int row, int col, const Piece* piece)
{
    board->pieces[row][col] = piece;
}

inline static void setPiece_s(Board* board, const Seat* seat, const Piece* piece)
{
    setPiece_rc(board, seat->row, seat->col, piece);
}

Board* newBoard(void)
{
    Board* board = malloc(sizeof(Board));
    memset(board, 0, sizeof(Board));
    board->bottomColor = RED;
    return board;
}

// 根据seat获取const seat*
const Seat* getSeat_s(const Seat seat)
{
    return getSeat_rc(seat.row, seat.col);
}

wchar_t* getPieChars_F(wchar_t* pieChars, wchar_t* FEN, size_t n)
{
    pieChars[0] = L'\x0';
    for (int index = 0, i = 0; i < n && index < SEATNUM; ++i) {
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

wchar_t* getPieChars_B(wchar_t* pieChars, const Board* board)
{
    pieChars[0] = L'\x0';
    for (int i = 0; i < SEATNUM; ++i)
        pieChars[i] = getChar(getPiece_rc(board,
            RowUpIndex_ - i / BOARDCOL, i % BOARDCOL));
    pieChars[SEATNUM] = L'\x0';
    return pieChars;
}

wchar_t* getFEN(wchar_t* FEN, const wchar_t* pieChars)
{
    FEN[0] = L'\x0';
    int index_F = 0;
    for (int row = 0; row <= RowUpIndex_; ++row) { // 从最高行开始
        int blankNum = 0;
        for (int col = ColLowIndex_; col <= ColUpIndex_; ++col) {
            int index_p = row * BOARDCOL + col;
            if (iswalpha(pieChars[index_p])) {
                if (blankNum > 0)
                    FEN[index_F++] = L'0' + blankNum;
                FEN[index_F++] = pieChars[index_p];
                blankNum = 0;
            } else if (pieChars[index_p] == BLANKCHAR) // 肯定为真, index_p+1
                blankNum++;
        }
        if (blankNum > 0)
            FEN[index_F++] = L'0' + blankNum;
        FEN[index_F++] = L'/'; // 插入行分隔符
    }
    FEN[--index_F] = L'\x0';
    return FEN;
}

void setBoard(Board* board, const wchar_t* pieChars)
{
    extern const Piece PIECES[PIECECOLORNUM][PIECEKINDNUM];
    int seatNum = wcslen(pieChars);
    assert(seatNum == SEATNUM);
    for (int index = 0; index < seatNum; ++index) {
        wchar_t ch = pieChars[index];
        setPiece_rc(board, RowUpIndex_ - index / BOARDCOL, index % BOARDCOL,
            iswalpha(ch) ? &(PIECES[getColor(ch)][getKind(ch)]) : NULL);
    }
    setBottomColor(board);
}

void setBottomColor(Board* board)
{
    for (int row = RowLowIndex_; row <= RowLowMidIndex_; ++row)
        for (int col = ColMidLowIndex_; col <= ColMidUpIndex_; ++col) {
            const Piece* pie = getPiece_rc(board, row, col);
            if (pie && pie->kind == KING) {
                board->bottomColor = pie->color;
                return;
            }
        }
}

bool isBottomSide(const Board* board, PieceColor color)
{
    return board->bottomColor == color;
}

const Seat* getKingSeat(const Board* board, PieceColor color)
{
    const Seat* seats[SEATNUM] = {};
    int count = putSeats(seats, isBottomSide(board, color), KING);
    for (int i = 0; i < count; ++i) {
        const Piece* pie = getPiece_s(board, seats[i]);
        if (pie && pie->kind == KING)
            return seats[i];
    }
    assert(false); // 永远不该运行
    return NULL;
}

int getLiveSeats(const Seat** pseats, size_t n, const Board* board, PieceColor color,
    const wchar_t name, const int findCol, bool getStronge)
{
    int count = 0;
    for (int row = RowLowIndex_; row <= RowUpIndex_; ++row)
        for (int col = ColLowIndex_; col <= ColUpIndex_; ++col) {
            const Piece* pie = getPiece_rc(board, row, col);
            if ((pie != NULL && pie->color == color)
                && (name == L'\x0' || name == getPieName(pie))
                && (findCol == -1 || col == findCol)
                && (!getStronge || pie->kind >= KNIGHT))
                pseats[count++] = getSeat_rc(row, col);
        }
    return count;
}

bool isKilled(Board* board, PieceColor color)
{
    PieceColor othColor = (color == RED ? BLACK : RED);
    const Seat *kingSeat = getKingSeat(board, color),
               *othKingSeat = getKingSeat(board, othColor);
    int frow = kingSeat->row, fcol = kingSeat->col,
        trow = othKingSeat->row, tcol = othKingSeat->col;
    if (fcol == tcol) {
        bool isBottom = isBottomSide(board, color), isBlank = true;
        int rowLow = isBottom ? frow : trow,
            rowUp = isBottom ? trow : frow;
        for (int row = rowLow + 1; row < rowUp; ++row)
            if (getPiece_rc(board, row, fcol)) {
                isBlank = false;
                break;
            }
        // 将帅之间全空，没有间隔棋子
        if (isBlank)
            return true;
    }

    const Seat* pseats[PIECENUM] = {};
    int count = getLiveSeats(pseats, PIECENUM, board,
        othColor, L'\x0', -1, true);
    for (int i = 0; i < count; ++i) {
        const Seat* mseats[BOARDROW + BOARDCOL] = {};
        int mcount = moveSeats(mseats, board, pseats[i]);
        for (int m = 0; m < mcount; ++m)
            // 本方将帅位置在对方强子可走位置范围内
            if (mseats[m]->row == frow && mseats[m]->col == fcol)
                return true;
    }
    return false;
}

bool isDied(Board* board, PieceColor color)
{
    const Seat *fseats[PIECENUM], *mseats[BOARDROW + BOARDCOL];
    int count = getLiveSeats(fseats, PIECENUM, board,
        color, L'\x0', -1, false);
    for (int i = 0; i < count; ++i) {
        const Seat* fseat = fseats[i];
        int mcount = moveSeats(mseats, board, fseat);
        // 有棋子可走
        if (getMoveSeats(mseats, mcount, board, fseat) > 0)
            return false;
    }
    return true;
}

int putSeats(const Seat** pseats, bool isBottom, PieceKind kind)
{
    int count = 0, rowLow, rowUp, cross, lfrow, ufrow, ltrow, utrow;
    switch (kind) {
    case KING:
        rowLow = isBottom ? RowLowIndex_ : RowUpMidIndex_;
        rowUp = isBottom ? RowLowMidIndex_ : RowUpIndex_;
        for (int row = rowLow; row <= rowUp; ++row)
            for (int col = ColMidLowIndex_; col <= ColMidUpIndex_; ++col)
                pseats[count++] = getSeat_rc(row, col);
        break;
    case ADVISOR:
        rowLow = isBottom ? RowLowIndex_ : RowUpMidIndex_;
        rowUp = isBottom ? RowLowMidIndex_ : RowUpIndex_;
        cross = isBottom ? 1 : 0; // 行列和的奇偶值
        for (int row = rowLow; row <= rowUp; ++row)
            for (int col = ColMidLowIndex_; col <= ColMidUpIndex_; ++col)
                if ((col + row) % 2 == cross)
                    pseats[count++] = getSeat_rc(row, col);
        break;
    case BISHOP:
        rowLow = isBottom ? RowLowIndex_ : RowUpLowIndex_;
        rowUp = isBottom ? RowLowUpIndex_ : RowUpIndex_;
        for (int row = rowLow; row <= rowUp; row += 2)
            for (int col = ColLowIndex_; col <= ColUpIndex_; col += 2) {
                cross = row - col;
                if ((isBottom && (cross == 2 || cross == -2 || cross == -6))
                    || (!isBottom && (cross == 7 || cross == 3 || cross == -1)))
                    pseats[count++] = getSeat_rc(row, col);
            }
        break;
    case PAWN:
        lfrow = isBottom ? RowLowUpIndex_ - 1 : RowUpLowIndex_;
        ufrow = isBottom ? RowLowUpIndex_ : RowUpLowIndex_ + 1;
        ltrow = isBottom ? RowUpLowIndex_ : RowLowIndex_;
        utrow = isBottom ? RowUpIndex_ : RowLowUpIndex_;
        for (int row = lfrow; row <= ufrow; ++row)
            for (int col = ColLowIndex_; col <= ColUpIndex_; col += 2)
                pseats[count++] = getSeat_rc(row, col);
        for (int row = ltrow; row <= utrow; ++row)
            for (int col = ColLowIndex_; col <= ColUpIndex_; ++col)
                pseats[count++] = getSeat_rc(row, col);
        break;
    default: // ROOK, KNIGHT, CANNON
        for (int row = 0; row < BOARDROW; ++row)
            for (int col = 0; col < BOARDCOL; ++col)
                pseats[count++] = getSeat_rc(row, col);
        break;
    }
    return count;
}

int moveSeats(const Seat** pseats, Board* board, const Seat* fseat)
{
    int count = 0, frow = fseat->row, fcol = fseat->col;
    const Piece* fpiece = getPiece_rc(board, frow, fcol);
    PieceColor color = fpiece->color;
    bool isBottom = isBottomSide(board, color);
    switch (fpiece->kind) {
    case KING: {
        const Seat tseats[] = {
            { frow, fcol - 1 }, //W
            { frow, fcol + 1 }, //E
            { frow - 1, fcol }, //S
            { frow + 1, fcol } //N
        };
        //           方向   W,   E,    S,    N
        bool select[] = { true, true, true, true };
        if (fcol == ColMidLowIndex_) // 最左列
            select[0] = false;
        else if (fcol == ColMidUpIndex_)
            select[1] = false;
        if (frow == (isBottom ? RowLowIndex_ : RowUpMidIndex_))
            select[2] = false;
        else if (frow == (isBottom ? RowLowMidIndex_ : RowUpIndex_))
            select[3] = false;

        for (int i = 0; i < sizeof(tseats) / sizeof(tseats[0]); ++i)
            if (select[i])
                pseats[count++] = getSeat_s(tseats[i]);
    } break;
    case ADVISOR: {
        const Seat tseats[] = {
            { frow - 1, fcol - 1 },
            { frow - 1, fcol + 1 },
            { frow + 1, fcol - 1 },
            { frow + 1, fcol + 1 },
            { (isBottom ? RowLowIndex_ : RowUpMidIndex_) + 1,
                ColMidLowIndex_ + 1 }
        };
        bool select[] = { true, true, true, true, true };
        if (fcol == ColMidLowIndex_ + 1)
            select[4] = false;
        else
            select[0] = select[1] = select[2] = select[3] = false;

        for (int i = 0; i < sizeof(tseats) / sizeof(tseats[0]); ++i)
            if (select[i])
                pseats[count++] = getSeat_s(tseats[i]);
    } break;
    case BISHOP: {
        const Seat tseats[] = {
            { frow - 2, fcol - 2 }, //SW
            { frow - 2, fcol + 2 }, //SE
            { frow + 2, fcol - 2 }, //NW
            { frow + 2, fcol + 2 } //NE
        };
        bool select[] = { true, true, true, true };
        if (fcol == ColLowIndex_)
            select[SW] = select[NW] = false;
        else if (fcol == ColUpIndex_)
            select[SE] = select[NE] = false;
        if (frow == (isBottom ? RowLowIndex_ : RowUpLowIndex_))
            select[SW] = select[SE] = false;
        else if (frow == (isBottom ? RowLowUpIndex_ : RowUpIndex_))
            select[NW] = select[NE] = false;

        for (int i = 0; i < sizeof(tseats) / sizeof(tseats[0]); ++i)
            if (select[i] && (getPiece_rc(board, (frow + tseats[i].row) / 2, (fcol + tseats[i].col) / 2) == NULL))
                pseats[count++] = getSeat_s(tseats[i]);
    } break;
    case KNIGHT: {
        const Seat tseats[] = {
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
        if (fcol == ColLowIndex_) // 最左列
        {
            select[WS] = select[WN] = select[SW] = select[NW] = false;
            if (frow == RowLowIndex_) // 最底行
                select[SE] = select[ES] = false;
            else if (frow == RowUpIndex_) // 最顶行
                select[NE] = select[EN] = false;
            else if (frow == RowLowIndex_ + 1) // 最底第二行
                select[SE] = false;
            else if (frow == RowUpIndex_ - 1) // 最顶第二行
                select[NE] = false;
        } else if (fcol == ColUpIndex_) // 最右列
        {
            select[ES] = select[EN] = select[SE] = select[NE] = false;
            if (frow == RowLowIndex_)
                select[SW] = select[WS] = false;
            else if (frow == RowUpIndex_)
                select[WN] = select[NW] = false;
            else if (frow == RowLowIndex_ + 1)
                select[SW] = false;
            else if (frow == RowUpIndex_ - 1)
                select[NW] = false;
        } else if (fcol == ColLowIndex_ + 1) // 最左第二列
        {
            select[WS] = select[WN] = false;
            if (frow < RowLowIndex_ + 2)
                select[SW] = select[SE] = false;
            else if (frow > RowUpIndex_ - 2)
                select[NW] = select[NE] = false;
        } else if (fcol == ColUpIndex_ - 1) // 最右第二列
        {
            select[ES] = select[EN] = false;
            if (frow < RowLowIndex_ + 2)
                select[SW] = select[SE] = false;
            else if (frow > RowUpIndex_ - 2)
                select[NW] = select[NE] = false;
        } else {
            if (frow == RowLowIndex_)
                select[SW] = select[WS] = select[SE] = select[ES] = false;
            else if (frow == RowUpIndex_)
                select[NE] = select[EN] = select[WN] = select[NW] = false;
            else if (frow == RowLowIndex_ + 1)
                select[SW] = select[SE] = false;
            else if (frow == RowUpIndex_ - 1)
                select[NW] = select[NE] = false;
        }

        for (int i = 0; i < sizeof(tseats) / sizeof(tseats[0]); ++i) {
            int legRow = (abs(tseats[i].col - fcol) == 2
                        ? frow
                        : (tseats[i].row > frow ? frow + 1 : frow - 1)),
                legCol = (abs(tseats[i].row - frow) == 2
                        ? fcol
                        : (tseats[i].col > fcol ? fcol + 1 : fcol - 1));
            if (select[i] && (getPiece_rc(board, legRow, legCol) == NULL))
                pseats[count++] = getSeat_s(tseats[i]);
        }
    } break;
    case PAWN: {
        const Seat tseats[] = {
            { frow, fcol - 1 }, //W
            { frow, fcol + 1 }, //E
            { frow - 1, fcol }, //S
            { frow + 1, fcol } //N
        };
        //           方向   W,   E,    S,    N
        bool select[] = { true, true, true, true };
        if (isBottom) {
            select[2] = false;
            if (frow == RowUpIndex_)
                select[3] = false;
        } else {
            select[3] = false;
            if (frow == RowLowIndex_)
                select[2] = false;
        }
        if ((isBottom && frow <= RowLowUpIndex_)
            || (!isBottom && frow > RowLowUpIndex_))
            select[0] = select[1] = false; // 兵没有过河
        else { // 兵已过河
            if (fcol == ColLowIndex_)
                select[0] = false;
            else if (fcol == ColUpIndex_)
                select[1] = false;
        }

        for (int i = 0; i < sizeof(tseats) / sizeof(tseats[0]); ++i)
            if (select[i])
                pseats[count++] = getSeat_s(tseats[i]);
    } break;
    default: // ROOK CANNON
    {
        Seat tseats[4][BOARDROW] = {}; // 建立四个方向的位置数组
        int tscount[4] = {}; // 每个方向的个数
        for (int col = fcol - 1; col >= ColLowIndex_; --col)
            tseats[0][tscount[0]++] = (Seat){ frow, col };
        //tseats[0][tscount[0]++] = getSeat_rc(frow, col);
        for (int col = fcol + 1; col <= ColUpIndex_; ++col)
            tseats[1][tscount[1]++] = (Seat){ frow, col };
        //tseats[1][tscount[1]++] = getSeat_rc(frow, col);
        for (int row = frow - 1; row >= RowLowIndex_; --row)
            tseats[2][tscount[2]++] = (Seat){ row, fcol };
        // tseats[2][tscount[2]++] = getSeat_rc(row, fcol);
        for (int row = frow + 1; row <= RowUpIndex_; ++row)
            tseats[3][tscount[3]++] = (Seat){ row, fcol };
        // tseats[3][tscount[3]++] = getSeat_rc(row, fcol);

        // 区分车、炮分别查找
        if (fpiece->kind == ROOK) {
            for (int line = 0; line < 4; ++line)
                for (int i = 0; i < tscount[line]; ++i) {
                    const Seat tseat = tseats[line][i];
                    const Piece* tpiece = getPiece_s(board, &tseat);
                    if (tpiece == NULL)
                        pseats[count++] = getSeat_s(tseat);
                    else {
                        if (tpiece->color != fpiece->color)
                            pseats[count++] = getSeat_s(tseat);
                        break;
                    }
                }
        } else { // CANNON
            for (int line = 0; line < 4; ++line) {
                bool skip = false;
                for (int i = 0; i < tscount[line]; ++i) {
                    const Seat tseat = tseats[line][i];
                    const Piece* tpiece = getPiece_s(board, &tseat);
                    if (!skip) {
                        if (tpiece == NULL)
                            pseats[count++] = getSeat_s(tseat);
                        else
                            skip = true;
                    } else if (tpiece != NULL) {
                        if (tpiece->color != fpiece->color)
                            pseats[count++] = getSeat_s(tseat);
                        break;
                    }
                }
            }
        }
    } break;
    }

    int index = 0;
    while (index < count) {
        // 筛除己方棋子所占位置
        const Seat** ptseat = &(pseats[index]);
        const Piece* tpiece = getPiece_s(board, *ptseat);
        if (tpiece && (tpiece->color == fpiece->color)) {
            --count; // 减少count
            if (index < count) // 检查index后是否还有seat？
                *ptseat = pseats[count]; // 将最后一个seat代替index处seat
        } else
            ++index; // 非己方棋子，index前进一个
    }
    return count;
}

int getMoveSeats(const Seat** pseats, int count, Board* board, const Seat* fseat)
{
    int index = 0;
    PieceColor color = getPiece_s(board, fseat)->color;
    while (index < count) {
        // 筛除移动后被将军位置
        const Seat** ptseat = &(pseats[index]);
        const Piece* eatPiece = moveTo(board, fseat, *ptseat, NULL);
        bool isKill = isKilled(board, color);
        moveTo(board, *ptseat, fseat, eatPiece);
        if (isKill) {
            --count; // 减少count
            if (index < count) // 检查index后是否还有seat？
                *ptseat = pseats[count]; // 将最后一个seat代替index处seat
        } else
            ++index; // 不会将军后，index前进一个
    }
    return count;
}

const Piece* moveTo(Board* board, const Seat* fseat, const Seat* tseat, const Piece* eatPiece)
{
    const Piece* piece = getPiece_s(board, tseat);
    setPiece_s(board, tseat, getPiece_s(board, fseat));
    setPiece_s(board, fseat, eatPiece);
    return piece;
}

int getSortPawnLiveSeats(const Seat** pseats, size_t n, const Board* board, PieceColor color)
{
    int count = 0;
    return count;
}

bool getMove(Move* move, const Board* board, const wchar_t* zhStr, size_t n)
{
    bool isOk = false;
    return isOk;
}

wchar_t* getZhStr(wchar_t* zhStr, size_t n, const Board* board, const Move* move)
{
    zhStr[0] = L'\x0';
    return zhStr;
}

bool changeBoardSide(Board* board, ChangeType ct)
{
    bool changed = false;
    return changed;
}

wchar_t* getBoardString(wchar_t* boardStr, const Board* board)
{
    static const wchar_t* BOARDSTR = L"┏━┯━┯━┯━┯━┯━┯━┯━┓\n"
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
                                     "┗━┷━┷━┷━┷━┷━┷━┷━┛\n"; // 边框粗线
    wcscpy(boardStr, BOARDSTR);
    for (int row = 0; row < BOARDROW; ++row)
        for (int col = 0; col < BOARDCOL; ++col) {
            const Piece* piece = getPiece_rc(board, row, col);
            if (piece)
                boardStr[(BOARDROW - row - 1) * 2 * (BOARDCOL * 2) + col * 2] = getPieName_T(piece);
        }
    return boardStr;
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
    for (int i = 0; i < 4; ++i) {
        fwprintf(fout, L"第%d个FEN =============================================\n", i);

        //* FEN转换成PieChars
        wchar_t pieChars[SEATNUM + 1];
        getPieChars_F(pieChars, FENs[i], wcslen(FENs[i]));
        fwprintf(fout, L"     FEN:%s size:%d\nPieChars:%s size:%d\n",
            FENs[i], wcslen(FENs[i]), pieChars, wcslen(pieChars));
        //*/

        //* 设置棋局，生成PieChars，转换成FEN
        Board* board = newBoard();
        setBoard(board, pieChars);

        getPieChars_B(pieChars, board);
        wchar_t FEN[SEATNUM + 1];
        getFEN(FEN, pieChars);
        fwprintf(fout, L"PieChars:%s size:%d\n     FEN:%s size:%d\n",
            pieChars, wcslen(pieChars), FEN, wcslen(FEN));
        //*/

        //* 取得将帅位置，打印棋局
        wchar_t boardStr[TEMPSTR_SIZE], pieString[SEATNUM];
        const Seat *rkseat = getKingSeat(board, RED),
                   *bkseat = getKingSeat(board, BLACK);
        // 一条语句内不要包含多个可改变局部变量值且返回局部变量指针的函数(因为可能返回同一个指针地址？)
        fwprintf(fout, L"%sboard：@%p ",
            getBoardString(boardStr, board), *board);
        fwprintf(fout, L"%s%d%d @%p <==> ",
            getPieString(pieString, SEATNUM, getPiece_s(board, rkseat)),
            rkseat->row, rkseat->col, rkseat);
        fwprintf(fout, L"%s%d%d @%p\n",
            getPieString(pieString, SEATNUM, getPiece_s(board, bkseat)),
            bkseat->row, bkseat->col, bkseat);
        //*/

        //* 取得各种条件下活的棋子
        for (int color = RED; color <= BLACK; ++color)
            for (int stronge = 0; stronge <= 1; ++stronge) {
                const Seat* lvseats[PIECENUM] = {};
                int count = getLiveSeats(lvseats, PIECENUM, board,
                    color, L'\x0', -1, stronge);
                fwprintf(fout, L"%c%c：",
                    color == RED ? L'红' : L'黑', stronge == 1 ? L'强' : L'全');
                for (int i = 0; i < count; ++i)
                    fwprintf(fout, L"%s%d%d ",
                        getPieString(pieString, SEATNUM,
                            getPiece_s(board, lvseats[i])),
                        lvseats[i]->row, lvseats[i]->col);
                fwprintf(fout, L"count:%d\n", count);
            }
        //*/

        //* 取得各活棋子的可移动位置
        for (int color = RED; color <= BLACK; ++color) {
            const Seat* lvseats[PIECENUM] = {};
            int count = getLiveSeats(lvseats, PIECENUM, board,
                color, L'\x0', -1, false);
            for (int i = 0; i < count; ++i) {
                const Seat* fseat = lvseats[i];
                const Piece* piece = getPiece_s(board, fseat);
                fwprintf(fout, L"%s %d%d >>【",
                    getPieString(pieString, SEATNUM, piece),
                    fseat->row, fseat->col);
                const Seat* mseats[BOARDROW + BOARDCOL] = {};
                int mcount = moveSeats(mseats, board, fseat);
                for (int i = 0; i < mcount; ++i)
                    fwprintf(fout, L" %d%d",
                        mseats[i]->row, mseats[i]->col);
                fwprintf(fout, L"】%d ", mcount);

                fwprintf(fout, L">>【");
                mcount = getMoveSeats(mseats, mcount, board, fseat);
                for (int i = 0; i < mcount; ++i)
                    fwprintf(fout, L" %d%d",
                        mseats[i]->row, mseats[i]->col);
                fwprintf(fout, L"】%d\n", mcount);
            }
        }
        //*/

        /* 取得各棋子的可放置位置
        extern const Piece PIECES[PIECECOLORNUM][PIECEKINDNUM];
        for (int color = 0; color < 2; ++color) {
            for (int index = 0; index < PIECEKINDNUM; ++index) {
                const Piece* pie = &(PIECES[color][index]);
                const Seat* seats[BOARDROW * BOARDCOL];
                int count = putSeats(seats, true, pie->kind);
                fwprintf(fout, L"%s ==【", getPieString(pieString, SEATNUM, pie));
                for (int i = 0; i < count; ++i)
                    fwprintf(fout, L" %d%d", seats[i]->row, seats[i]->col);
                fwprintf(fout, L"】%d\n", count);
            }
        }
        //*/
        fwprintf(fout, L"\n");
        free(board);
    }
}
