#include "board.h"
#include "piece.h"

// 着法相关的字符数组全局变量
const wchar_t NAMECHAR[] = L"帅将仕士相象马车炮兵卒";
const wchar_t PRECHAR[] = L"前中后";
const wchar_t MOVCHAR[] = L"退平进";
const wchar_t NUMCHAR[PIECECOLORNUM][BOARDCOL + 1] = {L"一二三四五六七八九", L"１２３４５６７８９"};
const wchar_t ICCSCHAR[] = L"abcdefghi";
const wchar_t FEN_0[] = L"rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR";

static wchar_t pieChars[SEATNUM + 1];
static wchar_t FEN[SEATNUM + 1];

// 棋盘行列相关的全局变量
static const int RowLowIndex_ = 0, RowLowMidIndex_ = 2, RowLowUpIndex_ = 4,
                 RowUpLowIndex_ = 5, RowUpMidIndex_ = 7, RowUpIndex_ = 9,
                 ColLowIndex_ = 0, ColMidLowIndex_ = 3, ColMidUpIndex_ = 5, ColUpIndex_ = 8;

wchar_t *getPieChars_F(wchar_t *FEN, size_t n)
{
    //static wchar_t pieChars[SEATNUM + 1];
    for (int index = 0, i = 0; i < n; ++i)
    {
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

void setPiece(Board *board, Seat *seat, const Piece *piece)
{
    board->piece[seat->row][seat->col] = piece;
}

bool setBoard(Board *board, wchar_t *pieChars)
{
    bool isSet = false, used[PIECECOLORNUM][PIECENUM] = {false};
    for (int index = 0; index < SEATNUM; ++index)
    {
        wchar_t ch = pieChars[index];
        if (iswalpha(ch))
        {
            PieceColor color = iswupper(ch) ? RED : BLACK;
            const Piece *rbpiece = pieces.piece[color];
            for (int i = 0; i < PIECENUM; ++i)
            {
                if (!used[color][i] && ch == getChar(&rbpiece[i]))
                {
                    board->piece[RowUpIndex_ - index / BOARDCOL][index % BOARDCOL] = &(rbpiece[i]);
                    used[color][i] = true;
                    break;
                }
            }
        }
        else
            board->piece[BOARDCOL - index / BOARDCOL][index % BOARDCOL] = NULL;
    }
    for (int row = RowLowIndex_; row <= RowLowMidIndex_; ++row)
    {
        for (int col = ColMidLowIndex_; col <= ColMidUpIndex_; ++col)
        {
            const Piece *pie = board->piece[row][col];
            if (pie && pie->kind == KING)
            {
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

Seat getKingSeat(const Board *board, PieceColor color)
{
    Seat seat = {};
    bool isBottom = board->bottomColor == color, isGet = false;
    int rowLow = isBottom ? RowLowIndex_ : RowUpMidIndex_,
        rowUp = isBottom ? RowLowMidIndex_ : RowUpIndex_;
    for (int row = rowLow; row <= rowUp; ++row)
    {
        for (int col = ColMidLowIndex_; col <= ColMidUpIndex_; ++col)
        {
            const Piece *pie = board->piece[row][col];
            if (pie && pie->kind == KING)
            {
                seat = (Seat){row, col};
                isGet = true;
                break;
            }
        }
        if (isGet)
            break;
    }
    assert(seat.col > 0);
    return seat;
}

int getLiveSeats(Seat *seats, size_t n, const Board *board, PieceColor color)
{
    int count = 0;
    for (int row = RowLowIndex_; row <= RowUpIndex_; ++row)
        for (int col = ColLowIndex_; col <= ColUpIndex_; ++col)
        {
            const Piece *pie = board->piece[row][col];
            if (pie && pie->color == color)
                seats[count++] = (Seat){row, col};
        }
    return count;
}

int getSortPawnLiveSeats(Seat *seats, size_t n, const Board *board, PieceColor color)
{
    int count = 0;
    return count;
}

bool isKilled(const Board *board, PieceColor color)
{
    bool killed = false;
    return killed;
}

bool isDied(const Board *board, PieceColor color)
{
    bool died = false;
    return died;
}

bool changeSide(const Board *board, ChangeType ct)
{
    bool changed = false;
    return changed;
}

wchar_t *getPieChars_B(const Board *board)
{
    for (int i = 0; i < SEATNUM; ++i)
    {
        pieChars[i] = getChar(board->piece[RowUpIndex_ - i / BOARDCOL][i % BOARDCOL]);
    }
    return pieChars;
}

wchar_t *getFEN(const wchar_t *pieChars)
{
    int index = 0, size = 0;
    for (int row = RowUpIndex_; row >= RowLowIndex_; --row)
    {
        int blankNum = 0;
        for (int col = ColLowIndex_; col <= ColUpIndex_; ++col)
        {
            if (iswalpha(pieChars[index]))
            {
                if (blankNum > 0)
                    FEN[size++] = L'0' + blankNum;
                FEN[size++] = pieChars[index++];
                blankNum = 0;
            }
            else if (pieChars[index++] == BLANKCHAR)
                blankNum++;
        }
        if (blankNum > 0)
            FEN[size++] = L'0' + blankNum;
        FEN[size++] = L'/';
    }
    FEN[--size] = L'\0';
    return FEN;
}

int putSeats(Seat *seats, bool isBottom, const Piece *piece)
{
    int count = 0, rowLow, rowUp, cross, lfrow, ufrow, ltrow, utrow;
    switch (piece->kind)
    {
    case KING:
        rowLow = isBottom ? RowLowIndex_ : RowUpMidIndex_;
        rowUp = isBottom ? RowLowMidIndex_ : RowUpIndex_;
        for (int row = rowLow; row <= rowUp; ++row)
            for (int col = ColMidLowIndex_; col <= ColMidUpIndex_; ++col)
            {
                Seat tseat = {row, col};
                seats[count++] = tseat;
            }
        break;
    case ADVISOR:
        rowLow = isBottom ? RowLowIndex_ : RowUpMidIndex_;
        rowUp = isBottom ? RowLowMidIndex_ : RowUpIndex_;
        cross = isBottom ? 1 : 0; // 行列和的奇偶值
        for (int row = rowLow; row <= rowUp; ++row)
            for (int col = ColMidLowIndex_; col <= ColMidUpIndex_; ++col)
                if ((col + row) % 2 == cross)
                {
                    Seat tseat = {row, col};
                    seats[count++] = tseat;
                }
        break;
    case BISHOP:
        rowLow = isBottom ? RowLowIndex_ : RowUpLowIndex_;
        rowUp = isBottom ? RowLowUpIndex_ : RowUpIndex_;
        for (int row = rowLow; row <= rowUp; row += 2)
            for (int col = ColLowIndex_; col <= ColUpIndex_; col += 2)
            {
                cross = row - col;
                if ((isBottom && (cross == 2 || cross == -2 || cross == -6)) || (!isBottom && (cross == 7 || cross == 3 || cross == -1)))
                {
                    Seat tseat = {row, col};
                    seats[count++] = tseat;
                }
            }
        break;
    case PAWN:
        lfrow = isBottom ? RowLowUpIndex_ - 1 : RowUpLowIndex_;
        ufrow = isBottom ? RowLowUpIndex_ : RowUpLowIndex_ + 1;
        ltrow = isBottom ? RowUpLowIndex_ : RowLowIndex_;
        utrow = isBottom ? RowUpIndex_ : RowLowUpIndex_;
        for (int row = lfrow; row <= ufrow; ++row)
            for (int col = ColLowIndex_; col <= ColUpIndex_; col += 2)
            {
                Seat tseat = {row, col};
                seats[count++] = tseat;
            }
        for (int row = ltrow; row <= utrow; ++row)
            for (int col = ColLowIndex_; col <= ColUpIndex_; ++col)
            {
                Seat tseat = {row, col};
                seats[count++] = tseat;
            }
        break;
    default: // ROOK, KNIGHT, CANNON
        for (int row = 0; row < BOARDROW; ++row)
            for (int col = 0; col < BOARDCOL; ++col)
            {
                Seat tseat = {row, col};
                seats[count++] = tseat;
            }
        break;
    }
    return count;
}

int moveSeats(Seat *seats, const Board *board, const Seat *seat)
{
    int count = 0, frow = seat->row, fcol = seat->col;
    const Piece *fpiece = board->piece[frow][fcol];
    bool isBottom = fpiece->color == board->bottomColor;
    switch (fpiece->kind)
    {
    case KING:
    {
        Seat tseats[] = {{frow, fcol - 1},  //W
                         {frow, fcol + 1},  //E
                         {frow - 1, fcol},  //S
                         {frow + 1, fcol}}; //N
        //           方向   W,   E,    S,    N
        bool select[] = {true, true, true, true};
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
    }
    break;
    case ADVISOR:
    {
        Seat tseats[] = {{frow - 1, fcol - 1},
                         {frow - 1, fcol + 1},
                         {frow + 1, fcol - 1},
                         {frow + 1, fcol + 1},
                         {(isBottom ? RowLowIndex_ : RowUpMidIndex_) + 1,
                          ColMidLowIndex_ + 1}};
        bool select[] = {true, true, true, true, true};
        if (fcol == ColMidLowIndex_ + 1)
            select[4] = false;
        else
            select[0] = select[1] = select[2] = select[3] = false;

        for (int i = 0; i < sizeof(tseats) / sizeof(tseats[0]); ++i)
            if (select[i])
                seats[count++] = tseats[i];
    }
    break;
    case BISHOP:
    {
        Seat tseats[] = {{frow - 2, fcol - 2},  //SW
                         {frow - 2, fcol + 2},  //SE
                         {frow + 2, fcol - 2},  //NW
                         {frow + 2, fcol + 2}}; //NE
        bool select[] = {true, true, true, true};
        if (fcol == ColLowIndex_)
            select[SW] = select[NW] = false;
        else if (fcol == ColUpIndex_)
            select[SE] = select[NE] = false;
        if (frow == (isBottom ? RowLowIndex_ : RowUpLowIndex_))
            select[SW] = select[SE] = false;
        else if (frow == (isBottom ? RowLowUpIndex_ : RowUpIndex_))
            select[NW] = select[NE] = false;

        for (int i = 0; i < sizeof(tseats) / sizeof(tseats[0]); ++i)
            if (select[i] && board->piece[(frow + tseats[i].row) / 2][(fcol + tseats[i].col) / 2] == NULL)
                seats[count++] = tseats[i];
    }
    break;
    case KNIGHT:
    {
        Seat tseats[] = {{frow - 2, fcol - 1},  //SW
                         {frow - 2, fcol + 1},  //SE
                         {frow + 2, fcol - 1},  //NW
                         {frow + 2, fcol + 1},  //NE
                         {frow - 1, fcol - 2},  //WS
                         {frow - 1, fcol + 2},  //ES
                         {frow + 1, fcol - 2},  //WN
                         {frow + 1, fcol + 2}}; //EN
        //             // {SW, SE, NW, NE, WS, ES, WN, EN}
        bool select[] = {true, true, true, true, true, true, true, true};
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
        }
        else if (fcol == ColUpIndex_) // 最右列
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
        }
        else if (fcol == ColLowIndex_ + 1) // 最左第二列
        {
            select[WS] = select[WN] = false;
            if (frow < RowLowIndex_ + 2)
                select[SW] = select[SE] = false;
            else if (frow > RowUpIndex_ - 2)
                select[NW] = select[NE] = false;
        }
        else if (fcol == ColUpIndex_ - 1) // 最右第二列
        {
            select[ES] = select[EN] = false;
            if (frow < RowLowIndex_ + 2)
                select[SW] = select[SE] = false;
            else if (frow > RowUpIndex_ - 2)
                select[NW] = select[NE] = false;
        }
        else
        {
            if (frow == RowLowIndex_)
                select[SW] = select[WS] = select[SE] = select[ES] = false;
            else if (frow == RowUpIndex_)
                select[NE] = select[EN] = select[WN] = select[NW] = false;
            else if (frow == RowLowIndex_ + 1)
                select[SW] = select[SE] = false;
            else if (frow == RowUpIndex_ - 1)
                select[NW] = select[NE] = false;
        }

        for (int i = 0; i < sizeof(tseats) / sizeof(tseats[0]); ++i)
        {
            int legRow = (abs(tseats[i].col - fcol) == 2
                              ? frow
                              : (tseats[i].row > frow ? frow + 1 : frow - 1)),
                legCol = (abs(tseats[i].row - frow) == 2
                              ? fcol
                              : (tseats[i].col > fcol ? fcol + 1 : fcol - 1));
            if (select[i] && (board->piece[legRow][legCol] == NULL))
                seats[count++] = tseats[i];
        }
    }
    break;
    case PAWN:
    {
        Seat tseats[] = {{frow, fcol - 1},  //W
                         {frow, fcol + 1},  //E
                         {frow - 1, fcol},  //S
                         {frow + 1, fcol}}; //N
        //           方向   W,   E,    S,    N
        bool select[] = {true, true, true, true};
        if (isBottom)
        {
            select[2] = false;
            if (frow == RowUpIndex_)
                select[3] = false;
        }
        else
        {
            select[3] = false;
            if (frow == RowLowIndex_)
                select[2] = false;
        }
        if ((isBottom && frow <= RowLowUpIndex_) || (!isBottom && frow > RowLowUpIndex_))
            select[0] = select[1] = false; // 兵没有过河
        else
        { // 兵已过河
            if (fcol == ColLowIndex_)
                select[0] = false;
            else if (fcol == ColUpIndex_)
                select[1] = false;
        }

        for (int i = 0; i < sizeof(tseats) / sizeof(tseats[0]); ++i)
            if (select[i])
                seats[count++] = tseats[i];
    }
    break;
    default: // ROOK CANNON
    {
        // 建立四个方向的位置数组
        Seat tseats[4][BOARDROW] = {};
        int tscount[4] = {};
        for (int col = fcol - 1; col >= ColLowIndex_; --col)
            tseats[0][tscount[0]++] = (Seat){frow, col};
        for (int col = fcol + 1; col <= ColUpIndex_; ++col)
            tseats[1][tscount[1]++] = (Seat){frow, col};
        for (int row = frow - 1; row >= RowLowIndex_; --row)
            tseats[2][tscount[2]++] = (Seat){row, fcol};
        for (int row = frow + 1; row <= RowUpIndex_; ++row)
            tseats[3][tscount[3]++] = (Seat){row, fcol};

        // 区分车、炮分别查找
        if (fpiece->kind == ROOK)
        {
            for (int line = 0; line < 4; ++line)
                for (int i = 0; i < tscount[line]; ++i)
                {
                    Seat tseat = tseats[line][i];
                    const Piece *tpiece = board->piece[tseat.row][tseat.col];
                    if (tpiece == NULL)
                        seats[count++] = tseat;
                    else
                    {
                        if (tpiece->color != fpiece->color)
                            seats[count++] = tseat;
                        break;
                    }
                }
        }
        else // CANNON
        {
            for (int line = 0; line < 4; ++line)
            {
                bool skip = false;
                for (int i = 0; i < tscount[line]; ++i)
                {
                    Seat tseat = tseats[line][i];
                    const Piece *tpiece = board->piece[tseat.row][tseat.col];
                    if (!skip)
                    {
                        if (tpiece == NULL)
                            seats[count++] = tseat;
                        else
                            skip = true;
                    }
                    else if (tpiece != NULL)
                    {
                        if (tpiece->color != fpiece->color)
                            seats[count++] = tseat;
                        break;
                    }
                }
            }
        }
    }
    break;
    }

    // 剔除己方棋子所占位置
    int index = 0;
    while (index < count)
    {
        const Piece *tpiece = board->piece[seats[index].row][seats[index].col];
        if (tpiece && tpiece->color == fpiece->color)
        {
            if (index < --count)             // 减少count, 检查index后是否还有seat？
                seats[index] = seats[count]; // 将最后一个seat代替index处seat，重新循环
        }
        else
            ++index;
    }

    // 移动后被将军位置

    return count;
}

bool getMove(const Board *board, const wchar_t *zhStr, size_t n, Move *move)
{
    bool isOk = false;
    return isOk;
}

wchar_t *getZhStr(const Board *board, size_t n, const Move *move)
{
    static wchar_t zhStr[8];
    return zhStr;
}

wchar_t *getBoardString(const Board *board)
{
    static const wchar_t *BOARDSTR = L"┏━┯━┯━┯━┯━┯━┯━┯━┓\n"
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
    static wchar_t boardStr[TEMPSTR_SIZE]; 
    wcscpy(boardStr, BOARDSTR);
    for (int row = 0; row < BOARDROW; ++row)
    {
        for (int col = 0; col < BOARDCOL; ++col)
        {
            const Piece *piece = board->piece[row][col];
            if (piece)
                boardStr[(BOARDROW - row - 1) * 2 * (BOARDCOL * 2) + col * 2] = getPieName_T(piece);
        }
    }
    return boardStr;
}

// 测试本翻译单元各种对象、函数
void testBoard(FILE *fout)
{
    fwprintf(fout, L"\ntestBoard：\n");
    wchar_t *FENs[] = {
        L"rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR",
        L"5a3/4ak2r/6R2/8p/9/9/9/B4N2B/4K4/3c5",
        L"2b1kab2/4a4/4c4/9/9/3R5/9/1C7/4r4/2BK2B2",
        L"4k4/P3a4/5r3/8p/2b6/9/9/7R1/4K4/9"};
    for (int i = 0; i < 4; ++i)
    {
        fwprintf(fout, L"第%d个FEN ===================================================\n", i);
        //*
        wchar_t *pieChars = getPieChars_F(FENs[i], wcslen(FENs[i]));
        fwprintf(fout, L"     FEN:%s size:%d\nPieChars:%s size:%d\n",
                 FENs[i], wcslen(FENs[i]), pieChars, wcslen(pieChars));
        //*/
        //*
        Board board = {};
        setBoard(&board, pieChars);

        pieChars = getPieChars_B(&board);
        wchar_t *FEN = getFEN(pieChars);
        fwprintf(fout, L"PieChars:%s size:%d\n     FEN:%s size:%d\n",
                 pieChars, wcslen(pieChars), FEN, wcslen(FEN));
        //*/
        //*
        fwprintf(fout, L"%s&board：%p\n", getBoardString(&board), &board);
        //*/
        //*
        for (int row = 0; row < BOARDROW; ++row)
            for (int col = 0; col < BOARDCOL; ++col)
            {
                const Piece *piece = board.piece[row][col];
                if (piece)
                {
                    fwprintf(fout, L"%s %d%d moveSeats:【", getPieString(piece), row, col);                    
                    Seat seat = {row, col};
                    Seat seats[BOARDROW + BOARDCOL] = {};
                    int count = moveSeats(seats, &board, &seat);
                    for (int i = 0; i < count; ++i)
                        fwprintf(fout, L" %d%d", seats[i].row, seats[i].col);
                    fwprintf(fout, L"】 count: %d\n", count);
                }
            }
        //*/
        /*
        for (int bottom = 0; bottom < 2; ++bottom)
        {
            fwprintf(fout, L"bottom: %d\n", bottom);
            for (int color = 0; color < 2; ++color)
            {
                for (int index = 0; index < PIECENUM; ++index)
                {
                    const Piece *pie = &(pieces.piece[color][index]);
                    Seat seats[BOARDROW * BOARDCOL];
                    int count = putSeats(seats, bottom, pie);
                    fwprintf(fout, L"%s putSeats:【", getPieString(pie));
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
