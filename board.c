#include "board.h"
#include "piece.h"

// 着法相关的字符数组静态全局变量
static const wchar_t NAMECHAR[] = L"帅将仕士相象马车炮兵卒";
static const wchar_t PRECHAR[] = L"前中后";
static const wchar_t MOVCHAR[] = L"退平进";
static const wchar_t NUMCHAR[PIECECOLORNUM][BOARDCOL + 1] = { L"一二三四五六七八九", L"１２３４５６７８９" };
static const wchar_t ICCSCHAR[] = L"abcdefghi";
static const wchar_t FEN_0[] = L"rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR";

// 棋盘行列相关的全局变量
static const int RowLowIndex_ = 0, RowLowMidIndex_ = 2, RowLowUpIndex_ = 4,
                 RowUpLowIndex_ = 5, RowUpMidIndex_ = 7, RowUpIndex_ = 9,
                 ColLowIndex_ = 0, ColMidLowIndex_ = 3, ColMidUpIndex_ = 5, ColUpIndex_ = 8;

// 从某棋盘内某行、某列位置取得一个棋子
inline static const Piece* getPiece_rc(const Board* board, int row, int col)
{
    return board->piece[row][col];
}
inline static const Piece* getPiece_s(const Board* board, const Seat* seat)
{
    return getPiece_rc(board, seat->row, seat->col);
}

// 置入某棋盘内某行、某列位置一个棋子
inline static void setPiece_rc(Board* board, int row, int col, const Piece* piece)
{
    board->piece[row][col] = piece;
}
inline static void setPiece_s(Board* board, const Seat* seat, const Piece* piece)
{
    setPiece_rc(board, seat->row, seat->col, piece);
}

wchar_t* getPieChars_F(wchar_t* pieChars, wchar_t* FEN, size_t n)
{
    for (int index = 0, i = 0; i < n; ++i) {
        wchar_t ch = FEN[i];
        if (iswdigit(ch))
            for (int j = ch - L'0'; j > 0; --j)
                pieChars[index++] = BLANKCHAR;
        else if (iswalpha(ch))
            pieChars[index++] = ch;
        if (index == SEATNUM)
            break;
    }
    return pieChars;
}

bool setBoard(Board* board, wchar_t* pieChars)
{
    extern const Pieces pieces;
    bool isSet = false, used[PIECECOLORNUM][PIECENUM] = { false };
    for (int index = 0; index < SEATNUM; ++index) {
        wchar_t ch = pieChars[index];
        if (iswalpha(ch)) {
            PieceColor color = iswupper(ch) ? RED : BLACK;
            const Piece* rbpiece = pieces.piece[color];
            for (int i = 0; i < PIECENUM; ++i) {
                if (!used[color][i] && ch == getChar(&rbpiece[i])) {
                    setPiece_rc(board, RowUpIndex_ - index / BOARDCOL, index % BOARDCOL, &(rbpiece[i]));
                    used[color][i] = true;
                    break;
                }
            }
        } else
            setPiece_rc(board, BOARDCOL - index / BOARDCOL, index % BOARDCOL, NULL);
    }
    for (int row = RowLowIndex_; row <= RowLowMidIndex_; ++row) {
        for (int col = ColMidLowIndex_; col <= ColMidUpIndex_; ++col) {
            const Piece* pie = getPiece_rc(board, row, col);
            if (pie && pie->kind == KING) {
                board->bottomColor = pie->color;
                isSet = true;
                break;
            }
        }
        if (isSet)
            break;
    }
    return isSet;
}

Seat getKingSeat(const Board* board, PieceColor color)
{
    bool isBottom = (board->bottomColor == color);
    int rowLow = (isBottom ? RowLowIndex_ : RowUpMidIndex_),
        rowUp = (isBottom ? RowLowMidIndex_ : RowUpIndex_);
    for (int row = rowLow; row <= rowUp; ++row)
        for (int col = ColMidLowIndex_; col <= ColMidUpIndex_; ++col) {
            const Piece* pie = getPiece_rc(board, row, col);
            if (pie && pie->kind == KING)
                return ((Seat){ row, col });
        }
    assert(true);
    return ((Seat){ -1, -1 });
}

int getLiveSeats(Seat* seats, size_t n, const Board* board, PieceColor color,
    const wchar_t name, const int findCol, bool getStronge)
{
    int count = 0;
    wchar_t pieStr[TEMPSTR_SIZE];
    for (int row = RowLowIndex_; row <= RowUpIndex_; ++row)
        for (int col = ColLowIndex_; col <= ColUpIndex_; ++col) {
            const Piece* pie = getPiece_rc(board, row, col);
            if ((pie != NULL && pie->color == color)
                && (name == L'\x0' || name == getPieName(pie))
                && (findCol == -1 || col == findCol)
                && (!getStronge || pie->kind >= KNIGHT))
                SetSeat(seats[count++], row, col);
        }
    return count;
}

bool isKilled(Board* board, PieceColor color)
{
    PieceColor othColor = color == RED ? BLACK : RED;
    Seat kingSeat = getKingSeat(board, color),
         othKingSeat = getKingSeat(board, othColor);
    int frow = kingSeat.row, fcol = kingSeat.col;
    if (fcol == othKingSeat.col) {
        bool isBottom = board->bottomColor == color, isBlank = true;
        int rowLow = isBottom ? frow : othKingSeat.row,
            rowUp = isBottom ? othKingSeat.row : frow;
        for (int row = rowLow + 1; row < rowUp; ++row)
            if (getPiece_rc(board, row, fcol) != NULL) {
                isBlank = false;
                break;
            }
        if (isBlank) // 将帅之间没有间隔棋子
            return true;
    }
    // 获取对方可杀将棋子全部可走的位置
    Seat seats[PIECENUM] = {};
    // 取得活的强子
    int count = getLiveSeats(seats, PIECENUM, board, othColor, L'\x0', -1, true);
    for (int i = 0; i < count; ++i) {
        Seat tseats[BOARDROW + BOARDCOL] = {};
        int tcount = moveSeats(tseats, board, &(seats[i]));
        for (int t = 0; t < tcount; ++t)
            // 对方强子可走位置有本方将帅位置
            if (tseats[t].row == frow && tseats[t].col == fcol)
                return true;
    }
    return false;
}

bool isDied(Board* board, PieceColor color)
{
    Seat seats[PIECENUM];
    int count = getLiveSeats(seats, PIECENUM, board, color, L'\x0', -1, false);
    for (int i = 0; i < count; ++i) {
        Seat tseats[BOARDROW + BOARDCOL];
        if (moveSeats(tseats, board, &(seats[i])) > 0)
            return false;
    }
    return true;
}

wchar_t* getPieChars_B(wchar_t* pieChars, const Board* board)
{
    for (int i = 0; i < SEATNUM; ++i) {
        pieChars[i] = getChar(getPiece_rc(board, RowUpIndex_ - i / BOARDCOL, i % BOARDCOL));
    }
    return pieChars;
}

wchar_t* getFEN(wchar_t* FEN, const wchar_t* pieChars)
{
    int index = 0, size = 0;
    for (int row = RowUpIndex_; row >= RowLowIndex_; --row) {
        int blankNum = 0;
        for (int col = ColLowIndex_; col <= ColUpIndex_; ++col) {
            if (iswalpha(pieChars[index])) {
                if (blankNum > 0)
                    FEN[size++] = L'0' + blankNum;
                FEN[size++] = pieChars[index++];
                blankNum = 0;
            } else if (pieChars[index++] == BLANKCHAR)
                blankNum++;
        }
        if (blankNum > 0)
            FEN[size++] = L'0' + blankNum;
        FEN[size++] = L'/';
    }
    FEN[--size] = L'\0';
    return FEN;
}

int putSeats(Seat* seats, bool isBottom, const Piece* piece)
{
    int count = 0, rowLow, rowUp, cross, lfrow, ufrow, ltrow, utrow;
    switch (piece->kind) {
    case KING:
        rowLow = isBottom ? RowLowIndex_ : RowUpMidIndex_;
        rowUp = isBottom ? RowLowMidIndex_ : RowUpIndex_;
        for (int row = rowLow; row <= rowUp; ++row)
            for (int col = ColMidLowIndex_; col <= ColMidUpIndex_; ++col)
                SetSeat(seats[count++], row, col);
        break;
    case ADVISOR:
        rowLow = isBottom ? RowLowIndex_ : RowUpMidIndex_;
        rowUp = isBottom ? RowLowMidIndex_ : RowUpIndex_;
        cross = isBottom ? 1 : 0; // 行列和的奇偶值
        for (int row = rowLow; row <= rowUp; ++row)
            for (int col = ColMidLowIndex_; col <= ColMidUpIndex_; ++col)
                if ((col + row) % 2 == cross)
                    SetSeat(seats[count++], row, col);
        break;
    case BISHOP:
        rowLow = isBottom ? RowLowIndex_ : RowUpLowIndex_;
        rowUp = isBottom ? RowLowUpIndex_ : RowUpIndex_;
        for (int row = rowLow; row <= rowUp; row += 2)
            for (int col = ColLowIndex_; col <= ColUpIndex_; col += 2) {
                cross = row - col;
                if ((isBottom && (cross == 2 || cross == -2 || cross == -6)) || (!isBottom && (cross == 7 || cross == 3 || cross == -1)))
                    SetSeat(seats[count++], row, col);
            }
        break;
    case PAWN:
        lfrow = isBottom ? RowLowUpIndex_ - 1 : RowUpLowIndex_;
        ufrow = isBottom ? RowLowUpIndex_ : RowUpLowIndex_ + 1;
        ltrow = isBottom ? RowUpLowIndex_ : RowLowIndex_;
        utrow = isBottom ? RowUpIndex_ : RowLowUpIndex_;
        for (int row = lfrow; row <= ufrow; ++row)
            for (int col = ColLowIndex_; col <= ColUpIndex_; col += 2)
                SetSeat(seats[count++], row, col);
        for (int row = ltrow; row <= utrow; ++row)
            for (int col = ColLowIndex_; col <= ColUpIndex_; ++col)
                SetSeat(seats[count++], row, col);
        break;
    default: // ROOK, KNIGHT, CANNON
        for (int row = 0; row < BOARDROW; ++row)
            for (int col = 0; col < BOARDCOL; ++col)
                SetSeat(seats[count++], row, col);
        break;
    }
    return count;
}

int moveSeats(Seat* seats, Board* board, const Seat* fseat)
{
    int count = 0, frow = fseat->row, fcol = fseat->col;
    const Piece* fpiece = getPiece_rc(board, frow, fcol);
    PieceColor color = fpiece->color;
    bool isBottom = color == board->bottomColor;
    switch (fpiece->kind) {
    case KING: {
        Seat tseats[] = { { frow, fcol - 1 }, //W
            { frow, fcol + 1 }, //E
            { frow - 1, fcol }, //S
            { frow + 1, fcol } }; //N
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
                seats[count++] = tseats[i];
    } break;
    case ADVISOR: {
        Seat tseats[] = { { frow - 1, fcol - 1 },
            { frow - 1, fcol + 1 },
            { frow + 1, fcol - 1 },
            { frow + 1, fcol + 1 },
            { (isBottom ? RowLowIndex_ : RowUpMidIndex_) + 1,
                ColMidLowIndex_ + 1 } };
        bool select[] = { true, true, true, true, true };
        if (fcol == ColMidLowIndex_ + 1)
            select[4] = false;
        else
            select[0] = select[1] = select[2] = select[3] = false;

        for (int i = 0; i < sizeof(tseats) / sizeof(tseats[0]); ++i)
            if (select[i])
                seats[count++] = tseats[i];
    } break;
    case BISHOP: {
        Seat tseats[] = { { frow - 2, fcol - 2 }, //SW
            { frow - 2, fcol + 2 }, //SE
            { frow + 2, fcol - 2 }, //NW
            { frow + 2, fcol + 2 } }; //NE
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
            if (select[i] && getPiece_rc(board, (frow + tseats[i].row) / 2, (fcol + tseats[i].col) / 2) == NULL)
                seats[count++] = tseats[i];
    } break;
    case KNIGHT: {
        Seat tseats[] = { { frow - 2, fcol - 1 }, //SW
            { frow - 2, fcol + 1 }, //SE
            { frow + 2, fcol - 1 }, //NW
            { frow + 2, fcol + 1 }, //NE
            { frow - 1, fcol - 2 }, //WS
            { frow - 1, fcol + 2 }, //ES
            { frow + 1, fcol - 2 }, //WN
            { frow + 1, fcol + 2 } }; //EN
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
                seats[count++] = tseats[i];
        }
    } break;
    case PAWN: {
        Seat tseats[] = { { frow, fcol - 1 }, //W
            { frow, fcol + 1 }, //E
            { frow - 1, fcol }, //S
            { frow + 1, fcol } }; //N
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
        if ((isBottom && frow <= RowLowUpIndex_) || (!isBottom && frow > RowLowUpIndex_))
            select[0] = select[1] = false; // 兵没有过河
        else { // 兵已过河
            if (fcol == ColLowIndex_)
                select[0] = false;
            else if (fcol == ColUpIndex_)
                select[1] = false;
        }

        for (int i = 0; i < sizeof(tseats) / sizeof(tseats[0]); ++i)
            if (select[i])
                seats[count++] = tseats[i];
    } break;
    default: // ROOK CANNON
    {
        // 建立四个方向的位置数组
        Seat tseats[4][BOARDROW] = {};
        int tscount[4] = {};
        for (int col = fcol - 1; col >= ColLowIndex_; --col)
            SetSeat(tseats[0][tscount[0]++], frow, col);
        for (int col = fcol + 1; col <= ColUpIndex_; ++col)
            SetSeat(tseats[1][tscount[1]++], frow, col);
        for (int row = frow - 1; row >= RowLowIndex_; --row)
            SetSeat(tseats[2][tscount[2]++], row, fcol);
        for (int row = frow + 1; row <= RowUpIndex_; ++row)
            SetSeat(tseats[3][tscount[3]++], row, fcol);

        // 区分车、炮分别查找
        if (fpiece->kind == ROOK) {
            for (int line = 0; line < 4; ++line)
                for (int i = 0; i < tscount[line]; ++i) {
                    Seat tseat = tseats[line][i];
                    const Piece* tpiece = getPiece_s(board, &tseat);
                    if (tpiece == NULL)
                        seats[count++] = tseat;
                    else {
                        if (tpiece->color != fpiece->color)
                            seats[count++] = tseat;
                        break;
                    }
                }
        } else // CANNON
        {
            for (int line = 0; line < 4; ++line) {
                bool skip = false;
                for (int i = 0; i < tscount[line]; ++i) {
                    Seat tseat = tseats[line][i];
                    const Piece* tpiece = getPiece_s(board, &tseat);
                    if (!skip) {
                        if (tpiece == NULL)
                            seats[count++] = tseat;
                        else
                            skip = true;
                    } else if (tpiece != NULL) {
                        if (tpiece->color != fpiece->color)
                            seats[count++] = tseat;
                        break;
                    }
                }
            }
        }
    } break;
    }

    int index = 0;
    while (index < count) {
        Seat* tseat = &(seats[index]);
        // 筛除己方棋子所占位置
        const Piece* tpiece = getPiece_s(board, tseat);
        if (tpiece && (tpiece->color == fpiece->color)
            && (index < --count)) { // 减少count, 检查index后是否还有seat？
            *tseat = seats[count]; // 将最后一个seat代替index处seat
            continue;
        }

        //*// 筛除移动后被将军位置
        const Piece* eatPiece = seatMoveTo(board, fseat, tseat, NULL);
        bool isKill = isKilled(board, color);
        seatMoveTo(board, tseat, fseat, eatPiece);
        if (isKill && (index < --count)) // 同上
            *tseat = seats[count]; // 同上
        else
            ++index; // 满足非己方棋子、移动不会将军后，index前进一个
        //*/
    }
    return count;
}

const Piece* seatMoveTo(Board* board, const Seat* fseat, const Seat* tseat, const Piece* eatPiece)
{
    const Piece* piece = getPiece_s(board, tseat);
    setPiece_s(board, tseat, getPiece_s(board, fseat));
    setPiece_s(board, fseat, eatPiece);
    return piece;
}

int getSortPawnLiveSeats(Seat* seats, size_t n, const Board* board, PieceColor color)
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
    return zhStr;
}

bool changeSide(Board* board, ChangeType ct)
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
    for (int row = 0; row < BOARDROW; ++row) {
        for (int col = 0; col < BOARDCOL; ++col) {
            const Piece* piece = getPiece_rc(board, row, col);
            if (piece)
                boardStr[(BOARDROW - row - 1) * 2 * (BOARDCOL * 2) + col * 2] = getPieName_T(piece);
        }
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
        //*
        wchar_t pieChars[TEMPSTR_SIZE];
        getPieChars_F(pieChars, FENs[i], wcslen(FENs[i]));
        fwprintf(fout, L"     FEN:%s size:%d\nPieChars:%s size:%d\n",
            FENs[i], wcslen(FENs[i]), pieChars, wcslen(pieChars));
        //*/
        //*
        Board aboard = {}, *board = &aboard;
        setBoard(board, pieChars);

        getPieChars_B(pieChars, board);
        wchar_t FEN[TEMPSTR_SIZE];
        getFEN(FEN, pieChars);
        fwprintf(fout, L"PieChars:%s size:%d\n     FEN:%s size:%d\n",
            pieChars, wcslen(pieChars), FEN, wcslen(FEN));
        //*/
        //*
        wchar_t boardStr[TEMPSTR_SIZE], pieString[TEMPSTR_SIZE];
        const Seat rkseat = getKingSeat(board, RED),
                   bkseat = getKingSeat(board, BLACK);
        fwprintf(fout, L"%sboard：@%p ", getBoardString(boardStr, board), *board);
        fwprintf(fout, L"%s%d%d @%p <--> ",
            getPieString(pieString, TEMPSTR_SIZE, getPiece_s(board, &rkseat)),
            rkseat.row, rkseat.col, rkseat);
        // 下面这条语句不能与上条语句合并，因为同一条语句内，getPieString函数返回值相同？
        fwprintf(fout, L"%s%d%d @%p\n",
            getPieString(pieString, TEMPSTR_SIZE, getPiece_s(board, &bkseat)),
            bkseat.row, bkseat.col, bkseat);
        //*/
        //*
        for (int color = RED; color <= BLACK; ++color) {
            for (int stronge = 0; stronge <= 1; ++stronge) {
                Seat lvseats[PIECENUM] = {};
                int count = getLiveSeats(lvseats, PIECENUM, board,
                    color, L'\x0', -1, stronge == 0 ? false : true);
                fwprintf(fout, L"%c%c：",
                    color == RED ? L'红' : L'黑', stronge == 1 ? L'强' : L'全');
                for (int i = 0; i < count; ++i)
                    fwprintf(fout, L"%s%d%d - ",
                        getPieString(pieString, TEMPSTR_SIZE,
                            getPiece_s(board, &(lvseats[i]))),
                        lvseats[i].row, lvseats[i].col);
                fwprintf(fout, L"count：%d\n", count);
            }
        }
        //*/
        //*
        for (int row = 0; row < BOARDROW; ++row)
            for (int col = 0; col < BOARDCOL; ++col) {
                const Piece* piece = board->piece[row][col];
                if (piece) {
                    fwprintf(fout, L"%s%d%d moveSeats:【",
                        getPieString(pieString, TEMPSTR_SIZE, piece), row, col);
                    Seat seat = { row, col };
                    Seat seats[BOARDROW + BOARDCOL] = {};
                    int count = moveSeats(seats, board, &seat);
                    for (int i = 0; i < count; ++i)
                        fwprintf(fout, L" %d%d", seats[i].row, seats[i].col);
                    fwprintf(fout, L"】 count: %d\n", count);
                }
            }
        //*/
        /*
        for (int bottom = 0; bottom < 2; ++bottom) {
            extern const Pieces pieces;
            fwprintf(fout, L"bottom: %d\n", bottom);
            for (int color = 0; color < 2; ++color) {
                for (int index = 0; index < PIECENUM; ++index) {
                    const Piece* pie = &(pieces.piece[color][index]);
                    Seat seats[BOARDROW * BOARDCOL];
                    int count = putSeats(seats, bottom, pie);
                    fwprintf(fout, L"%s putSeats:【", getPieString(pieString, TEMPSTR_SIZE, pie));
                    for (int i = 0; i < count; ++i)
                        fwprintf(fout, L" %d%d", seats[i].row, seats[i].col);
                    fwprintf(fout, L"】 count: %d\n", count);
                }
            }
        }
        //*/
        fwprintf(fout, L"\n");
    }
}
