#include "head/board.h"
#include "head/move.h"
#include "head/piece.h"
#include "head/tools.h"
#include <math.h>

extern FILE* test_out;

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

// 着法相关的字符数组静态全局变量
static const char SplitChar = '/'; // FEN的行分隔符;
static const wchar_t SPLITCHAR = L'/'; // FEN的行分隔符;
static const wchar_t* PRECHAR = L"前中后";
static const wchar_t* MOVCHAR = L"退平进";
static const wchar_t* NUMWCHAR[PIECECOLORNUM] = { L"一二三四五六七八九", L"１２３４５６７８９" };

static bool isValidRow__(int row) { return row >= RowLowIndex_ && row <= RowUpIndex_; }
static bool isValidCol__(int col) { return col >= ColLowIndex_ && col <= ColUpIndex_; }

inline int getOtherRow(int row) { return BOARDROW - 1 - row; }
inline int getOtherCol(int col) { return BOARDCOL - 1 - col; }

inline int getRow_s(CSeat seat) { return seat->row; }
inline int getCol_s(CSeat seat) { return seat->col; }

Seat getOtherSeat(Board board, Seat seat, ChangeType ct)
{
    if (ct == ROTATE)
        return getSeat_rc(board, getOtherRow(getRow_s(seat)), getOtherCol(getCol_s(seat)));
    else if (ct == SYMMETRY_H)
        return getSeat_rc(board, getRow_s(seat), getOtherCol(getCol_s(seat)));
    else if (ct == SYMMETRY_V)
        return getSeat_rc(board, getOtherRow(getRow_s(seat)), getCol_s(seat));
    else // ct==EXCHANGE, 则位置不需要更改
        return seat;
}

inline int getRowCol_rc(int row, int col) { return (row << 4) | col; }
inline int getRowCol_s(CSeat seat) { return getRowCol_rc(getRow_s(seat), getCol_s(seat)); }

inline int getRow_rowcol(int rowcol) { return rowcol >> 4; }
inline int getCol_rowcol(int rowcol) { return rowcol & 0x0F; }

int getRowCol_SYMMETRY_H(int rowcol)
{
    return getRowCol_rc(getRow_rowcol(rowcol), getOtherCol(getCol_rowcol(rowcol)));
}

inline static int getIndex__(const wchar_t* str, const wchar_t ch) { return wcschr(str, ch) - str; }
inline static int getNum__(PieceColor color, wchar_t numChar) { return getIndex__(NUMWCHAR[color], numChar) + 1; }
inline static wchar_t getNumChar__(PieceColor color, int num) { return NUMWCHAR[color][num - 1]; }
inline static int getCol__(bool isBottom, int num) { return isBottom ? BOARDCOL - num : num - 1; }
inline static int getNum_Col__(bool isBottom, int col) { return (isBottom ? getOtherCol(col) : col) + 1; }
inline static int getMovDir__(bool isBottom, wchar_t mch) { return (getIndex__(MOVCHAR, mch) - 1) * (isBottom ? 1 : -1); }
inline PieceColor getColor_zh(const wchar_t* zhStr) { return wcschr(NUMWCHAR[RED], zhStr[3]) ? RED : BLACK; }

static wchar_t getPreChar__(bool isBottom, int count, int index)
{
    if (isBottom)
        index = count - 1 - index;
    if (count == 2 && index == 1)
        index = 2;
    if (count <= 3)
        return PRECHAR[index];
    else
        return NUMWCHAR[RED][index];
}

static int getIndex_ch__(bool isBottom, int count, wchar_t wch)
{
    int index = 0;
    wchar_t* pwch = wcschr(PRECHAR, wch);
    if (pwch) {
        index = pwch - PRECHAR;
        if (index == 2 && count == 2)
            index = 1;
    } else {
        pwch = wcschr(NUMWCHAR[RED], wch);
        assert(pwch);
        index = pwch - NUMWCHAR[RED];
    }
    if (isBottom)
        index = count - 1 - index;
    return index;
}

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

inline Seat getSeat_rc(Board board, int row, int col)
{
    //*
    if (!(isValidRow__(row) && isValidCol__(col))) {
        printBoard(board, row, col, L"");
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
        FEN[fi++] = SPLITCHAR;
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

char* changeFEN_c(char* fen, ChangeType ct)
{
    if (ct == NOCHANGE)
        return fen;

    // 左右对称交换
    char tempFEN[SEATNUM];
    strcpy(tempFEN, fen);
    int len = strlen(tempFEN), index = 0;
    switch (ct) {
    case EXCHANGE: // 交换颜色
        for (int i = 0; i < len; ++i) {
            char c = tempFEN[i];
            fen[index++] = isalpha(c) ? (isupper(c) ? tolower(c) : toupper(c)) : c;
        }
        break;
    case ROTATE: // 旋转
        for (int i = len - 1; i >= 0; --i)
            fen[index++] = tempFEN[i];
        break;
    case SYMMETRY_H: // 左右对称交换
        for (char *lineStart = tempFEN, *lineEnd = strchr(lineStart, SplitChar);;
             lineStart = lineEnd + 1, lineEnd = strchr(lineStart, SplitChar)) {
            for (int lineLen = (lineEnd ? lineEnd : tempFEN + len) - lineStart - 1;
                 lineLen >= 0; --lineLen)
                fen[index++] = *(lineStart + lineLen);
            if (lineEnd)
                fen[index++] = SplitChar;
            else
                break;
        }
        break;
    case SYMMETRY_V: // 上下对称交换
        for (char *lineStart = strrchr(tempFEN, SplitChar), *lineEnd = tempFEN + len;;
             lineEnd = lineStart, *lineEnd = '\x0', lineStart = strrchr(tempFEN, SplitChar)) {
            int offset = 1;
            if (lineStart == NULL) {
                lineStart = tempFEN;
                offset = 0;
            }
            for (int lineLen = lineEnd - lineStart - offset, i = 0;
                 i < lineLen; ++i)
                fen[index++] = *(lineStart + offset + i);
            if (lineStart != tempFEN)
                fen[index++] = SplitChar;
            else
                break;
        }
        break;
    default:
        break;
    }

    return fen;
}

static void resetPiece__(Piece piece, void* ptr)
{
    Seat seat = getSeat_p(piece);
    setSeat(piece, NULL);
    if (seat)
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
            setPiece_rc__(board, row, col, getNotLivePiece_ch(board->pieces, pieChars[index++]));
    board->bottomColor = getRow_s(getKingSeat(board, RED)) < RowLowUpIndex_ ? RED : BLACK;
}

void setBoard_FEN(Board board, const wchar_t* FEN)
{
    wchar_t pieChars[SEATNUM + 1];
    setBoard_pieChars(board, getPieChars_FEN(pieChars, FEN));
}

inline bool isBottomSide(CBoard board, PieceColor color) { return board->bottomColor == color; }

Seat getKingSeat(Board board, PieceColor color) { return getSeat_p(getKingPiece(board->pieces, color)); }

//inline static bool isName__(Seat seat, wchar_t name, int col) { return getPieName(getPiece_s(seat)) == name; }

//inline static bool isNameCol__(Seat seat, wchar_t name, int col) { return isName__(seat, name, col) && getCol_s(seat) == col; }

// 按先行后列, 升序排列
static int seatCmp__(const void* first, const void* second)
{
    Seat aseat = (Seat)first, bseat = (Seat)second;
    int rowdiff = getRow_s(aseat) - getRow_s(bseat);
    return (rowdiff == 0) ? getCol_s(aseat) - getCol_s(bseat) : rowdiff;
}

int getLiveSeats_bc(Seat* seats, CBoard board, PieceColor color, bool filterStronge)
{
    return getLiveSeats_pieces(seats, board->pieces, color, filterStronge);
}

static int getLiveSeats_bcnc__(Seat* seats, Board board, PieceColor color, wchar_t name, int col)
{
    int index = 0,
        count = getLiveSeats_bc(seats, board, color, false);
    while (index < count) {
        if (getPieName(getPiece_s(seats[index])) == name
            && (col < 0 || col == getCol_s(seats[index])))
            ++index;
        else
            seats[index] = seats[--count];
    }
    qsort(seats, index, sizeof(Seat), seatCmp__);
    return index;
}

static int getLiveSeats_bcn__(Seat* seats, Board board, PieceColor color, wchar_t name)
{
    return getLiveSeats_bcnc__(seats, board, color, name, -1);
}

static int getSortPawnLiveSeats__(Seat* seats, Board board, PieceColor color, wchar_t name)
{
    Seat pawnSeats[SIDEPIECENUM] = { NULL };
    int liveCount = getLiveSeats_bcn__(pawnSeats, board, color, name),
        colCount[BOARDCOL] = { 0 };
    Seat colSeats[BOARDCOL][BOARDROW] = { NULL };
    for (int i = 0; i < liveCount; ++i) {
        int col = getCol_s(pawnSeats[i]);
        colSeats[col][colCount[col]++] = pawnSeats[i];
    }

    // 不需要考虑isBottom的问题，留待后续计算index时再考虑。2021.3.29
    int count = 0;
    for (int col = 0; col < BOARDCOL; ++col)
        // 仅选数量大于2个的列
        if (colCount[col] > 1)
            for (int i = 0; i < colCount[col]; ++i) {
                seats[count++] = colSeats[col][i];
                //wchar_t seatStr[WIDEWCHARSIZE];
                //fwprintf(test_out, L">%ls ", getSeatString(seatStr, colSeats[col][i]));
            }

    //wchar_t wstr[WIDEWCHARSIZE];
    //fwprintf(test_out, L"\n%ls\n", getBoardString(wstr, board));
    return count;
}

Piece movePiece(Seat fseat, Seat tseat, Piece eatPiece)
{
    Piece tpiece = getPiece_s(tseat);
    setSeat(tpiece, NULL);
    setPiece_s__(tseat, getPiece_s(fseat));
    setPiece_s__(fseat, eatPiece);
    return tpiece;
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

static bool moveSameColor__(void* arg1, Seat fseat, Seat tseat)
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
        bool select[] = { false, false, false, false };
        if (isBottom) {
            if (frow != RowUpIndex_)
                select[NE] = true;
        } else if (frow != RowLowIndex_)
            select[NW] = true;

        if ((isBottom && frow > RowLowUpIndex_)
            || (!isBottom && frow <= RowLowUpIndex_)) { // 兵已过河
            if (fcol != ColLowIndex_)
                select[SW] = true;
            if (fcol != ColUpIndex_)
                select[SE] = true;
        }
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
    return filterObjects((void**)seats, count, (void*)board, (void*)fseat,
        (bool (*)(void*, void*, void*))moveSameColor__, true);
}

// 着法走之后，判断是否将帅对面
static bool isFace__(Board board)
{
    Seat redKingSeat = getKingSeat(board, RED),
         blkKingSeat = getKingSeat(board, BLACK);
    int fcol = getCol_s(redKingSeat);
    if (fcol != getCol_s(blkKingSeat))
        return false;

    int frow = getRow_s(redKingSeat), trow = getRow_s(blkKingSeat),
        rowLow = fmin(frow, trow), rowUp = fmax(trow, frow);
    for (int row = rowLow + 1; row < rowUp; ++row)
        if (!isBlankPiece(getPiece_rc(board, row, fcol)))
            return false;

    // 将帅之间全空，没有间隔棋子
    return true;
}

// 判断某方是否被将军
static bool isPassiveKilling__(Board board, PieceColor color)
{
    Seat kingSeat = getKingSeat(board, color),
         seats[SIDEPIECENUM],
         mseats[BOARDROW + BOARDCOL];
    int count = getLiveSeats_bc(seats, board, getOtherColor(color), true);
    for (int i = 0; i < count; ++i) {
        int mcount = moveSeats(mseats, board, seats[i]);
        for (int m = 0; m < mcount; ++m)
            // 本方将帅位置在对方强子可走位置范围内
            if (mseats[m] == kingSeat)
                return true;
    }

    return false;
}

// 模拟移动棋子，判断取得棋子移动后的棋局状态是否符合指定函数意义
static bool isStatusIfMoved__(Board board, Seat fseat, Seat tseat,
    bool (*isFunc__)(Board board, PieceColor color), bool isFromColor)
{
    PieceColor color = getColor(getPiece_s(fseat));
    if (!isFromColor)
        color = getOtherColor(color);
    Piece eatPiece = movePiece(fseat, tseat, getBlankPiece());
    bool isStatus = isFunc__(board, color);
    movePiece(tseat, fseat, eatPiece);
    return isStatus;
}

// 是否处于将帅对面、被将军状态
static bool isFaceOrPassiveKilling__(Board board, PieceColor color)
{
    return isFace__(board) || isPassiveKilling__(board, color);
}

// 判断移动是否失败(将帅对面、自身被将则属失败)
static bool moveFailed__(Board board, Seat fseat, Seat tseat)
{
    return isStatusIfMoved__(board, fseat, tseat, isFaceOrPassiveKilling__, true); // 己方
}

// 获取可移动位置集合(规则允许着法，且排除移动失败情形)
int canMoveSeats(Seat* seats, Board board, Seat fseat, bool sure)
{
    int count = moveSeats(seats, board, fseat);
    return filterObjects((void**)seats, count, (void*)board, (void*)fseat,
        (bool (*)(void*, void*, void*))moveFailed__, sure);
}

// 28.1　将
//　　凡走子直接攻击对方帅(将) 者，称为“照将”，简称“将”。
bool isKilling(Board board, Seat fseat, Seat tseat)
{
    return isStatusIfMoved__(board, fseat, tseat, isPassiveKilling__, false); // 对方
}

// 某方是否已没有可移动位置(规则允许着法，且排除移动失败情形)
bool isFail(Board board, PieceColor color)
{
    Seat fseats[SIDEPIECENUM], mseats[BOARDROW + BOARDCOL];
    int count = getLiveSeats_bc(fseats, board, color, false);
    for (int i = 0; i < count; ++i)
        if (canMoveSeats(mseats, board, fseats[i], true) > 0)
            return false;

    return true;
}

// 判断是否照将或连续照将，将死对方者
static bool isContinuousKilling__(Board board, Seat fseat, Seat tseat)
{
    PieceColor color = getColor(getPiece_s(tseat));
    Piece eatTPiece = movePiece(fseat, tseat, getBlankPiece()); // 走一着
    bool killed = false;
    if (isPassiveKilling__(board, color)) {
        Seat tseats[SIDEPIECENUM], mtseats[BOARDROW + BOARDCOL];
        int tpieCount = getLiveSeats_bc(tseats, board, color, false);
        for (int i = 0; i < tpieCount; ++i) {
            int mtseatCount = canMoveSeats(mtseats, board, tseats[i], true);
            killed = mtseatCount == 0; // 对方无棋子可走，退出递归调用
            for (int j = 0; j < mtseatCount; ++j) {
                Piece eatFPiece = movePiece(tseats[i], mtseats[j], getBlankPiece()); // 走一着

                Seat fseats[SIDEPIECENUM], mfseats[BOARDROW + BOARDCOL];
                int fpieCount = getLiveSeats_bc(fseats, board, color, false);
                for (int k = 0; k < fpieCount; ++k) {
                    int mfseatCount = canMoveSeats(mfseats, board, fseats[k], true);
                    for (int l = 0; l < mfseatCount; ++l) {
                        killed = isContinuousKilling__(board, fseats[k], mfseats[l]); // 递归调用
                        if (killed)
                            break;
                    }
                    if (killed)
                        break;
                }

                movePiece(mtseats[j], tseats[i], eatFPiece);
                if (killed)
                    break;
            }
            if (killed)
                break;
        }
    }

    movePiece(tseat, fseat, eatTPiece);
    return killed;
}

// 28.2　杀
//　　凡走子企图在下一着照将或连续照将，将死对方者，称为“杀着”，简称“杀”。
bool isWillKill(Board board, Seat fseat, Seat tseat)
{
    return isContinuousKilling__(board, fseat, tseat);
}

// 是否子力
// (25.3　车、马、炮、过河兵(卒)、士、相(象)，均算“子力”。帅(将)、未过河兵(卒)，不算“子力”。“子力”简称“子”
// 子力价值是衡量子力得失的尺度，也是判断是否“捉子”的依据之一。原则上，一车相当于双马、双炮或一马一炮；
// 马炮相等；士相(象)相等；过河兵(卒)价值浮动，一兵换取数子或一子换取数兵均不算得子。)
static bool isForce__(Board board, Seat tseat)
{
    Piece piece = getPiece_s(tseat);
    PieceKind kind = getKind(piece);
    return !(kind == KING
        || (kind == PAWN
            && (getColor(piece) == board->bottomColor
                    ? getRow_s(tseat) > RowLowUpIndex_
                    : getRow_s(tseat) <= RowLowUpIndex_)));
}

// 是否有根子
// (28.16　有根子和无根子
//　　凡有己方其他棋子(包括暗根)充分保护的子，称为“有根子”；反之，称为“无根子”。
//　　形式上是根，实际上起不到充分保护作用，称为假根或少根、假根子和少根子按无根子处理。)
static bool isProtectedForce__(Board board, Seat fseat, Seat tseat)
{
    if (!isForce__(board, tseat))
        return false;

    Seat fseats[SIDEPIECENUM], mseats[BOARDROW + BOARDCOL];
    int pieCount = getLiveSeats_bc(fseats, board, getColor(getPiece_s(tseat)), false);
    for (int i = 0; i < pieCount; ++i) {
        int mseatCount = canMoveSeats(mseats, board, fseats[i], true);
        for (int j = 0; j < mseatCount; ++j) {
            if (mseats[j] == tseat) {
                Piece eatPiece0 = movePiece(fseat, tseat, getBlankPiece()); // 对方走一着
                Piece eatPiece1 = movePiece(fseats[i], tseat, getBlankPiece()); // 己方走一着

                bool willKill = false;
                //bool willKill = isWillKill(board, getColor(getPiece_s(tseat))); // 是否被杀

                movePiece(tseat, fseats[i], eatPiece1);
                movePiece(tseat, fseat, eatPiece0);
                if (!willKill)
                    return true;
            }
        }
    }

    return false;
}

// 构成“捉子”，应符合下列条件：
//　　29.1　捉子的形式可以有：能够直接吃子；能够通过连续照将吃子；能够通过完整的交换过程得子。
//　　29.2　“捉”产生于刚走的这着棋，上一着尚不存在。
//　　29.3　直接或间接被捉的，是有子力价值的无根子(含：假根子、少根子)。
//　　29.4　下一着吃子或得子后，不致被将死。
bool isCatch(Board board, Seat fseat, Seat tseat)
{
    if (isProtectedForce__(board, fseat, tseat))
        return false;

    return false;
}

bool isClout(Board board, Seat fseat, Seat tseat)
{
    return isKilling(board, fseat, tseat) || isWillKill(board, fseat, tseat) || isCatch(board, fseat, tseat);
}

// 28.9　闲
//　　凡走子性质不属于将、杀、捉，统称为“闲”。兑、献、拦、跟，均属“闲”的范畴。
bool isIdle(Board board, Seat fseat, Seat tseat)
{
    return !isClout(board, fseat, tseat);
}

bool isCanMove(Board board, Seat fseat, Seat tseat)
{
    Seat mseats[BOARDROW + BOARDCOL] = { NULL };
    int count = canMoveSeats(mseats, board, fseat, true);
    for (int i = 0; i < count; ++i)
        if (mseats[i] == tseat)
            return true;

    /*
    wchar_t wstr[WIDEWCHARSIZE] = { 0 }, seatStr[WCHARSIZE];
    wcscat(wstr, L"[");
    for (int i = 0; i < count; ++i) {
        wcscat(wstr, getSeatString(seatStr, mseats[i]));
    }
    wcscat(wstr, L"]\n\n");
    printBoard(board, count, -250, wstr);
    //*/

    return false;
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
    setSeat(piece, NULL);
    setSeat(othPiece, NULL);
    if (seat)
        setPiece_s__(seat, othPiece);
    if (othSeat)
        setPiece_s__(othSeat, piece);
}

void changeBoard(Board board, ChangeType ct)
{
    if (ct == EXCHANGE) {
        // 复制结构, 以保存和使用最初的位置棋子布局
        struct Board boardBak = *board;
        piecesMap(boardBak.pieces, exchangePiece__, boardBak.pieces);
    } else { // ROTATE, SYMMETRY_H, SYMMETRY_V
        int rowLimit = ct == SYMMETRY_H ? BOARDROW : BOARDROW / 2,
            colLimit = ct == SYMMETRY_H ? BOARDCOL / 2 : BOARDCOL;
        for (int row = 0; row < rowLimit; ++row)
            for (int col = 0; col < colLimit; ++col) {
                Seat seat = getSeat_rc(board, row, col),
                     othSeat = getOtherSeat(board, seat, ct);
                Piece piece = getPiece_s(seat);
                setPiece_s__(seat, getPiece_s(othSeat));
                setPiece_s__(othSeat, piece);
            }
    }
    if (ct != SYMMETRY_H)
        board->bottomColor = !(board->bottomColor);
}

bool getSeats_zh(Seat* pfseat, Seat* ptseat, Board board, const wchar_t* zhStr)
{
    assert(wcslen(zhStr) == 4);
    //wprintf(L"%d %ls\n", __LINE__, zhStr);
    //fflush(stdout);
    // 根据最后一个字符判断该着法属于哪一方
    PieceColor color = getColor_zh(zhStr);
    bool isBottom = isBottomSide(board, color);
    int index = 0, count = 0,
        movDir = getMovDir__(isBottom, zhStr[2]);
    wchar_t name = zhStr[0];
    Seat seats[SIDEPIECENUM] = { NULL };

    if (isPieceName(name)) { // 棋子名
        count = getLiveSeats_bcnc__(seats,
            board, color, name, getCol__(isBottom, getNum__(color, zhStr[1])));
        /*
        if (count == 0) {
            printBoard(board, __LINE__, 0, zhStr);
            fflush(stdout);
        }
        //*/
        if (count == 0)
            return false;
        assert(count > 0);
        // 排除：士、象同列时不分前后，以进、退区分棋子。移动方向为底退、顶进时，修正index
        index = (count == 2 && movDir == -1) ? 1 : 0; // movDir == -1：表示底退、顶进
    } else { // 非棋子名
        name = zhStr[1];
        count = isPawnPieceName(name) ? getSortPawnLiveSeats__(seats, board, color, name)
                                      : getLiveSeats_bcn__(seats, board, color, name);
        index = getIndex_ch__(isBottom, count, zhStr[0]);
    }

    /* 显示着法字符串
    wchar_t wstr[WCHARSIZE];
    for (int i = 0; i < count; ++i)
        fwprintf(test_out, L"%ls ", getSeatString(wstr, seats[i]));
    fwprintf(test_out, L"%ls index: %d count: %d\n", zhStr, index, count);
    //*/

    if (index >= count) {
        wchar_t wstr[WIDEWCHARSIZE];
        wprintf(L"%d:\n%ls%ls index:%d count:%d\n", __LINE__, getBoardString(wstr, board), zhStr, index, count);
        fflush(stdout);
    }
    assert(index < count);

    *pfseat = seats[index];
    int num = getNum__(color, zhStr[3]), tcol = getCol__(isBottom, num),
        frow = getRow_s(*pfseat), fcol = getCol_s(*pfseat), colAway = abs(tcol - fcol); //  相距1或2列
    //wprintf(L"%d %lc %d %d %d %d %d\n", __LINE__, name, frow, fcol, movDir, colAway, tcol);
    //fflush(stdout);

    *ptseat = isLinePieceName(name) ? (movDir == 0 ? getSeat_rc(board, frow, tcol)
                                                   : getSeat_rc(board, frow + movDir * num, fcol))
                                    // 斜线走子：仕、相、马
                                    : getSeat_rc(board, frow + movDir * (isKnightPieceName(name) ? (colAway == 1 ? 2 : 1) : colAway), tcol);

    //* 相互验证
    wchar_t tmpZhStr[6];
    getZhStr_seats(tmpZhStr, board, *pfseat, *ptseat);
    if (wcscmp(zhStr, tmpZhStr) != 0) {
        wchar_t wstr[WIDEWCHARSIZE];
        fwprintf(test_out, L"\n%ls %ls\n%ls\n", zhStr, tmpZhStr, getBoardString(wstr, board));
    }
    assert(wcscmp(zhStr, tmpZhStr) == 0);
    //*/
    return true;
}

void getZhStr_seats(wchar_t* zhStr, Board board, Seat fseat, Seat tseat)
{
    Piece fpiece = getPiece_s(fseat);
    assert(!isBlankPiece(fpiece));

    PieceColor color = getColor(fpiece);
    wchar_t name = getPieName(fpiece);
    int frow = getRow_s(fseat), fcol = getCol_s(fseat),
        trow = getRow_s(tseat), tcol = getCol_s(tseat);
    bool isBottom = isBottomSide(board, color);
    Seat seats[SIDEPIECENUM] = { 0 };
    int count = getLiveSeats_bcnc__(seats, board, color, name, fcol);

    if (count > 1 && isStronge(fpiece)) { // 马车炮兵
        if (getKind(fpiece) == PAWN)
            count = getSortPawnLiveSeats__(seats, board, color, name);
        int index = 0;
        while (fseat != seats[index])
            ++index;
        zhStr[0] = getPreChar__(isBottom, count, index);
        zhStr[1] = name;
    } else { //将帅, 仕(士),相(象): 不用“前”和“后”区别，因为能退的一定在前，能进的一定在后
        zhStr[0] = name;
        zhStr[1] = getNumChar__(color, getNum_Col__(isBottom, fcol));
    }
    zhStr[2] = MOVCHAR[frow == trow ? 1 : (isBottom == (trow > frow) ? 2 : 0)];
    zhStr[3] = getNumChar__(color, (isLinePieceName(name) && frow != trow) ? abs(trow - frow) : getNum_Col__(isBottom, tcol));
    zhStr[4] = L'\x0';

    /*
    Seat tmpfseat, tmptseat;
    getSeats_zh(&tmpfseat, &tmptseat, board, zhStr);
    assert(fseat == tmpfseat && tseat == tmptseat);
    //*/
}

const wchar_t* getICCS_s(wchar_t* iccs, Seat fseat, Seat tseat, ChangeType ct)
{
    int frow = getRow_s(fseat), fcol = getCol_s(fseat),
        trow = getRow_s(tseat), tcol = getCol_s(tseat);
    // 水平对称或旋转
    if (ct == SYMMETRY_H || ct == ROTATE) {
        fcol = getOtherCol(fcol);
        tcol = getOtherCol(tcol);
    }
    // 垂直对称或旋转
    if (ct == SYMMETRY_V || ct == ROTATE) {
        frow = getOtherRow(frow);
        trow = getOtherRow(trow);
    }
    // 交换颜色不需变换

    swprintf(iccs, 6, L"%lc%d%lc%d", fcol + L'a', frow, tcol + L'a', trow);
    return iccs;
}

wchar_t* getSeatString(wchar_t* seatStr, CSeat seat)
{
    wchar_t str[WCHARSIZE];
    swprintf(seatStr, WCHARSIZE, L"%02x%ls", getRowCol_s(seat), getPieString(str, getPiece_s(seat)));
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

void printBoard(Board board, int int_arg1, int int_arg2, const wchar_t* wcs_arg3)
{
    wchar_t wstr[2 * WIDEWCHARSIZE];
    getBoardString(wstr, board);
    int strLen = wcstombs(NULL, wstr, 0) + 1, arg3Len = wcstombs(NULL, wcs_arg3, 0) + 1;
    char str[strLen], wcs_args[arg3Len];
    wcstombs(str, wstr, strLen);
    wcstombs(wcs_args, wcs_arg3, arg3Len);
    printf("\n%s\narg1:%d arg2:%d\nargs:%s\n", str, int_arg1, int_arg2, wcs_args);
    fflush(stdout);
}

bool seat_equal(CSeat seat0, CSeat seat1)
{
    return ((seat0 == NULL && seat1 == NULL)
        || (seat0 && seat1
            && seat0->row == seat1->row && seat0->col == seat1->col
            && piece_equal(seat0->piece, seat1->piece)
            && ((seat0->piece == getBlankPiece() && seat1->piece == getBlankPiece())
                || (getSeat_p(seat0->piece) == seat0 && getSeat_p(seat1->piece) == seat1))));
}

bool board_equal(Board board0, Board board1)
{
    if (board0 == NULL && board1 == NULL)
        return true;
    // 其中有一个为空指针
    if (!(board0 && board1))
        return false;

    if (board0->bottomColor != board1->bottomColor)
        return false;

    for (int r = 0; r < BOARDROW; ++r)
        for (int c = 0; c < BOARDCOL; ++c)
            if (!seat_equal(getSeat_rc(board0, r, c), getSeat_rc(board1, r, c)))
                return false;

    return true;
}
