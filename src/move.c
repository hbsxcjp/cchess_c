#include "head/move.h"
#include "head/board.h"
#include "head/piece.h"
#include "head/tools.h"

const wchar_t ICCSCHAR[BOARDCOL + 1] = L"abcdefghi";
// 着法相关的字符数组静态全局变量
const wchar_t PRECHAR[] = L"前中后";
const wchar_t MOVCHAR[] = L"退平进";
const wchar_t NUMCHAR[PIECECOLORNUM][BOARDCOL + 1] = {
    L"一二三四五六七八九", L"１２３４５６７８９"
};

Move* newMove(void)
{
    Move* move = (Move*)malloc(sizeof(Move));
    move->fseat = move->tseat = -1;
    move->tpiece = BLANKPIECE;
    move->remark = L'\x0';
    move->pmove = move->nmove = move->omove = NULL;
    move->nextNo_ = move->otherNo_ = move->CC_ColNo_ = 0;
    return move;
}

bool isSame(const Move* amove, const Move* bmove)
{
    return amove->nextNo_ == bmove->nextNo_ && amove->otherNo_ == bmove->otherNo_;
}

Move* addNext(Move* move)
{
    Move* nextMove = newMove();
    nextMove->nextNo_ = move->nextNo_ + 1;
    nextMove->otherNo_ = move->otherNo_;
    nextMove->pmove = move;
    return move->nmove = nextMove;
}

Move* addOther(Move* move)
{
    Move* otherMove = newMove();
    otherMove->nextNo_ = move->nextNo_;
    otherMove->otherNo_ = move->otherNo_ + 1;
    otherMove->pmove = move;
    return move->omove = otherMove;
}

void delMove(Move* move)
{
    if (move == NULL)
        return;
    delMove(move->omove);
    delMove(move->nmove);
    if (move->remark != NULL)
        free(move->remark);
    free(move);
}

void cutNextMove(Move* move)
{
    Move* cmove = move->nmove;
    if (cmove != NULL && cmove->omove != NULL) {
        move->nmove = cmove->omove;
        cmove->omove = NULL;
    }
    delMove(cmove);
}

void cutOhterMove(Move* move)
{

    Move* cmove = move->omove;
    if (cmove != NULL && cmove->omove != NULL) {
        move->omove = cmove->omove;
        cmove->omove = NULL;
    }
    delMove(cmove);
}

wchar_t* getICCS(wchar_t* ICCSStr, size_t n, const Move* move)
{
    swprintf(ICCSStr, n, L"%c%d%c%d",
        ICCSCHAR[getCol_s(move->fseat)], getRow_s(move->fseat),
        ICCSCHAR[getCol_s(move->tseat)], getRow_s(move->tseat));
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
    if (count == 2)
        wcsncpy(wcsncpy(preChars, PRECHAR, 1), PRECHAR + 2, 1);
    else if (count == 3)
        wcscpy(preChars, PRECHAR);
    else // count == 4,5
        wcsncpy(preChars, NUMCHAR[RED], 5);
    return preChars;
}

extern const wchar_t* PieceNames[PIECECOLORNUM];

void setMove(Move* move, const Board* board, const wchar_t* zhStr, size_t n)
{
    assert(wcslen(zhStr) == 4);
    // 根据最后一个字符判断该着法属于哪一方
    PieceColor color = wcschr(NUMCHAR[RED], zhStr[3]) == NULL ? BLACK : RED;
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

    //wchar_t azhStr[5];
    //assert(wcscmp(zhStr, getZhStr(azhStr, 5, board, move)) == 0);
}

wchar_t* getZhStr(wchar_t* zhStr, size_t n, const Board* board, const Move* move)
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
        zhStr[0] = preChars[isBottom ? count - 1 - index : index];
        zhStr[1] = name;
    } else { //将帅, 仕(士),相(象): 不用“前”和“后”区别，因为能退的一定在前，能进的一定在后
        zhStr[0] = name;
        zhStr[1] = NUMCHAR[color][isBottom ? BOARDCOL - 1 - fcol : fcol];
    }
    zhStr[2] = MOVCHAR[frow == trow ? 1 : (isBottom == (trow > frow) ? 2 : 0)];
    zhStr[3] = NUMCHAR[color][(__isLinePiece(name) && frow != trow)
            ? abs(trow - frow) - 1
            : (isBottom ? BOARDCOL - 1 - tcol : tcol)];
    zhStr[4] = L'\x0';

    //*
    Move* amove = newMove();
    setMove(amove, board, zhStr, 5);
    assert(move->fseat == amove->fseat && move->tseat == amove->tseat);
    //*/
    return zhStr;
}

void setRemark(Move* move, wchar_t* remark)
{
    remark = wtrim(remark);
    int len = wcslen(remark);
    if (len > 0) {
        move->remark = (wchar_t*)calloc(len + 1, sizeof(remark[0]));
        wcscpy(move->remark, remark);
    }
}
