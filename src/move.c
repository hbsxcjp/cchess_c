#include "head/move.h"
#include "head/board.h"
#include "head/piece.h"
#include "head/tools.h"

// 着法相关的字符数组静态全局变量
const wchar_t ICCSCOLCHAR[] = L"abcdefghi";
const wchar_t ICCSROWCHAR[] = L"0123456789";
const wchar_t PRECHAR[] = L"前中后";
const wchar_t MOVCHAR[] = L"退平进";
const wchar_t NUMCHAR[PIECECOLORNUM][BOARDCOL + 1] = {
    L"一二三四五六七八九", L"１２３４５６７８９"
};

PMove newMove(void)
{
    PMove move = calloc(sizeof(Move), 1);
    move->fseat = move->tseat = -1;
    move->tpiece = BLANKPIECE;
    //move->remark = NULL;
    //move->pmove = move->nmove = move->omove = NULL;
    //move->nextNo_ = move->otherNo_ = move->CC_ColNo_ = 0;
    return move;
}

bool isSameMove(const PMove amove, const PMove bmove)
{
    //return (amove->nextNo_ == bmove->nextNo_) && (amove->otherNo_ == bmove->otherNo_);
    return amove == bmove;
}

PMove addNext(PMove move)
{
    PMove nextMove = newMove();
    nextMove->nextNo_ = move->nextNo_ + 1;
    nextMove->otherNo_ = move->otherNo_;
    nextMove->pmove = move;
    return move->nmove = nextMove;
}

PMove addOther(PMove move)
{
    PMove otherMove = newMove();
    otherMove->nextNo_ = move->nextNo_;
    otherMove->otherNo_ = move->otherNo_ + 1;
    otherMove->pmove = move;
    return move->omove = otherMove;
}

void delMove(PMove move)
{
    if (move == NULL)
        return;
    delMove(move->omove);
    delMove(move->nmove);
    free(move->remark);
    free(move);
}

void cutNextMove(PMove move)
{
    PMove cmove = move->nmove;
    if (cmove != NULL && cmove->omove != NULL) {
        move->nmove = cmove->omove;
        cmove->omove = NULL;
    }
    delMove(cmove);
}

void cutOhterMove(PMove move)
{

    PMove cmove = move->omove;
    if (cmove != NULL && cmove->omove != NULL) {
        move->omove = cmove->omove;
        cmove->omove = NULL;
    }
    delMove(cmove);
}

void setMove_iccs(PMove move, const wchar_t* iccsStr)
{
    move->fseat = getSeat_rc(
        wcschr(ICCSROWCHAR, iccsStr[1]) - ICCSROWCHAR,
        wcschr(ICCSCOLCHAR, iccsStr[0]) - ICCSCOLCHAR);
    move->tseat = getSeat_rc(
        wcschr(ICCSROWCHAR, iccsStr[3]) - ICCSROWCHAR,
        wcschr(ICCSCOLCHAR, iccsStr[2]) - ICCSCOLCHAR);
}

wchar_t* getICCS(wchar_t* ICCSStr, const PMove move)
{
    swprintf(ICCSStr, 5, L"%c%d%c%d",
        ICCSCOLCHAR[getCol_s(move->fseat)], getRow_s(move->fseat),
        ICCSCOLCHAR[getCol_s(move->tseat)], getRow_s(move->tseat));
    return ICCSStr;
}

inline static int __getNum(PieceColor color, wchar_t numChar)
{
    return wcschr(NUMCHAR[color], numChar) - NUMCHAR[color] + 1;
}

inline static int __getCol(bool isBottom, int num)
{
    return isBottom ? BOARDCOL - num : num - 1;
}

inline static bool __isLinePiece(wchar_t name)
{
    return wcschr(L"将帅车炮兵卒", name) != NULL;
}

static wchar_t* __getPreCHars(wchar_t* preChars, int count)
{
    if (count == 2) {
        preChars[0] = PRECHAR[0];
        preChars[1] = PRECHAR[2];
        preChars[2] = L'\x0';
    } else if (count == 3)
        wcscpy(preChars, PRECHAR);
    else // count == 4,5
        wcsncpy(preChars, NUMCHAR[RED], 5);
    return preChars;
}

extern const wchar_t* PieceNames[PIECECOLORNUM];

PieceColor getColor_zh(const wchar_t* zhStr)
{
    return wcschr(NUMCHAR[RED], zhStr[3]) == NULL ? BLACK : RED;
}

static wchar_t* __getSimpleMoveStr(wchar_t* wstr, const PMove move)
{
    wchar_t iccs[6];
    if (move && move->fseat >= 0)
        wsprintfW(wstr, L"%02x->%02x %s %s@%c",
            move->fseat, move->tseat, getICCS(iccs, move), move->zhStr,
            (move->tpiece != BLANKPIECE ? getPieName(move->tpiece) : BLANKCHAR));
    return wstr;
}

wchar_t* getMoveStr(wchar_t* wstr, const PMove move)
{
    wchar_t preWstr[WCHARSIZE] = { 0 }, thisWstr[WCHARSIZE] = { 0 }, nextWstr[WCHARSIZE] = { 0 }, otherWstr[WCHARSIZE] = { 0 };
    wsprintfW(wstr, L"%s：%s\n现在：%s\n下着：%s\n下变：%s\n注解：               导航区%3d行%2d列\n%s",
        ((!move->pmove || move->pmove->nmove == move) ? L"前着" : L"前变"),
        __getSimpleMoveStr(preWstr, move->pmove),
        __getSimpleMoveStr(thisWstr, move),
        __getSimpleMoveStr(nextWstr, move->nmove),
        __getSimpleMoveStr(otherWstr, move->omove),
        move->nextNo_, move->CC_ColNo_ + 1, move->remark);
    return wstr;
}

void setMove_zh(PMove move, const PBoard board, const wchar_t* zhStr)
{
    assert(wcslen(zhStr) == 4);
    // 根据最后一个字符判断该着法属于哪一方
    PieceColor color = getColor_zh(zhStr);
    bool isBottom = isBottomSide(board, color);
    int index = 0, count = 0,
        movDir = (wcschr(MOVCHAR, zhStr[2]) - MOVCHAR - 1) * (isBottom ? 1 : -1);
    wchar_t name = zhStr[0];
    Seat seats[PIECENUM] = {};

    if (wcschr(PieceNames[RED], name) != NULL || wcschr(PieceNames[BLACK], name) != NULL) {
        count = getLiveSeats(seats, PIECENUM,
            board, color, name, __getCol(isBottom, __getNum(color, zhStr[1])), false);
        assert(count > 0);
        // 排除：士、象同列时不分前后，以进、退区分棋子。移动方向为退时，修正index
        index = (count == 2 && movDir == -1) ? 1 : 0; // 如为士象且退时
    } else {
        name = zhStr[1];
        if (name == PieceNames[RED][PAWN] || name == PieceNames[BLACK][PAWN])
            count = getSortPawnLiveSeats(seats, board, color, name);
        else
            count = getLiveSeats(seats, PIECENUM, board, color, name, -1, false);
        wchar_t preChars[6] = { 0 };
        __getPreCHars(preChars, count);
        assert(wcschr(preChars, zhStr[0]) != NULL);
        index = wcschr(preChars, zhStr[0]) - preChars;
        if (isBottom)
            index = count - 1 - index;
    }

    //wprintf(L"%s index: %d count: %d\n", zhStr, index, count);
    assert(index < count);
    move->fseat = seats[index];
    int num = __getNum(color, zhStr[3]), toCol = __getCol(isBottom, num),
        frow = getRow_s(move->fseat);
    if (__isLinePiece(name)) {
        move->tseat = (movDir == 0
                ? getSeat_rc(frow, toCol)
                : getSeat_rc(frow + movDir * num, getCol_s(move->fseat)));
    } else { // 斜线走子：仕、相、马
        int colAway = abs(toCol - getCol_s(move->fseat)), //  相距1或2列
            trow = frow + movDir * (name != L'马' ? colAway : (colAway == 1 ? 2 : 1));
        move->tseat = getSeat_rc(trow, toCol);
    }

    /*
    setZhStr(move, board);
    assert(wcscmp(zhStr, move->zhStr) == 0);
    //*/
}

void setZhStr(PMove move, const PBoard board)
{
    Piece fpiece = getPiece_s(board, move->fseat);
    assert(fpiece != BLANKPIECE);
    PieceColor color = getColor(fpiece);
    wchar_t name = getPieName(fpiece);
    int frow = getRow_s(move->fseat), fcol = getCol_s(move->fseat),
        trow = getRow_s(move->tseat), tcol = getCol_s(move->tseat);
    bool isBottom = isBottomSide(board, color);
    Seat seats[PIECENUM] = { 0 };
    int count = getLiveSeats(seats, PIECENUM, board, color, name, fcol, false);

    if (count > 1 && wcschr(PieceNames[color] + 3, name) != NULL) {
        if (getKind(fpiece) == PAWN)
            count = getSortPawnLiveSeats(seats, board, color, name);
        wchar_t preChars[6] = { 0 };
        __getPreCHars(preChars, count);
        int index = 0;
        for (int i = 0; i < count; ++i)
            if (move->fseat == seats[i]) {
                index = i;
                break;
            }
        move->zhStr[0] = preChars[isBottom ? count - 1 - index : index];
        move->zhStr[1] = name;
    } else { //将帅, 仕(士),相(象): 不用“前”和“后”区别，因为能退的一定在前，能进的一定在后
        move->zhStr[0] = name;
        move->zhStr[1] = NUMCHAR[color][isBottom ? BOARDCOL - 1 - fcol : fcol];
    }
    move->zhStr[2] = MOVCHAR[frow == trow ? 1 : (isBottom == (trow > frow) ? 2 : 0)];
    move->zhStr[3] = NUMCHAR[color][(__isLinePiece(name) && frow != trow)
            ? abs(trow - frow) - 1
            : (isBottom ? BOARDCOL - 1 - tcol : tcol)];
    move->zhStr[4] = L'\x0';

    /*
    wchar_t iccsStr[12], boardStr[WIDEWCHARSIZE];
    wprintf(L"iccs: %s zh:%s\n%s\n",
        getICCS(iccsStr, move), zhStr, getBoardString(boardStr, board));
    //*/

    //*
    PMove amove = newMove();
    setMove_zh(amove, board, move->zhStr);
    assert(move->fseat == amove->fseat && move->tseat == amove->tseat);
    delMove(amove);
    //*/
    //return move->zhStr;
}

void setRemark(PMove move, const wchar_t* remark)
{
    //remark = wtrim(remark);
    int len = wcslen(remark) + 1;
    if (len > 1) {
        move->remark = malloc(len * sizeof(wchar_t));
        wcscpy(move->remark, remark);
    }
}

void changeMove(PMove move, ChangeType ct)
{
    Seat fseat = move->fseat, tseat = move->tseat;
    if (ct == ROTATE) {
        move->fseat = getSeat_rc(getOtherRow_s(fseat), getOtherCol_s(fseat));
        move->tseat = getSeat_rc(getOtherRow_s(tseat), getOtherCol_s(tseat));
    } else if (ct == SYMMETRY) {
        move->fseat = getSeat_rc(getRow_s(fseat), getOtherCol_s(fseat));
        move->tseat = getSeat_rc(getRow_s(tseat), getOtherCol_s(tseat));
    }

    if (move->nmove != NULL)
        changeMove(move->nmove, ct);

    if (move->omove != NULL)
        changeMove(move->omove, ct);
}
