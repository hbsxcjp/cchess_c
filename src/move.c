#include "head/move.h"
#include "head/board.h"
#include "head/piece.h"

static const wchar_t ICCSCHAR[] = L"abcdefghi";

Move* newMove(void)
{
    Move* move = (Move*)malloc(sizeof(Move));
    move->fseat = calloc(1, sizeof(move->fseat));
    move->tseat = calloc(1, sizeof(move->tseat));
    *move->fseat = *move->tseat = NULL;
    move->tpiece = calloc(1, sizeof(move->tpiece));
    *move->tpiece = NULL;
    move->remark = NULL;
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
        ICCSCHAR[(*move->fseat)->col], (*move->fseat)->row,
        ICCSCHAR[(*move->tseat)->col], (*move->tseat)->row);
    return ICCSStr;
}

wchar_t* getZH(wchar_t* ZHStr, size_t n, const Move* move)
{
    return ZHStr;
}

void setRemark(Move* move, const wchar_t* remark)
{
    move->remark = (wchar_t*)calloc(wcslen(remark) + 1, sizeof(remark[0]));
    wcscpy(move->remark, remark);
}

static wchar_t* __getRemarkStr(wchar_t* remark, size_t n, const Move* move)
{
    if (move->remark == NULL)
        remark[0] = L'\x0';
    else
        swprintf(remark, n, L" \n{%s}\n ", move->remark);
    return remark;
}

static wchar_t* __getMovString(wchar_t* movStr, size_t n, const Move* move,
    bool isPGN_ZH, bool isOther)
{
    wchar_t boutStr[6] = {}, tempStr[REMARKSIZE] = {};
    swprintf(boutStr, 6, L"%d. ", (move->nextNo_ + 1) / 2);
    bool isEven = move->nextNo_ % 2 == 0;
    if (isOther) {
        swprintf(tempStr, REMARKSIZE, L"(%s%s", boutStr, isEven ? L"... " : L"");
        wcscat(movStr, tempStr);
    } else
        wcscat(movStr, isEven ? L" " : boutStr);
    wcscat(movStr, isPGN_ZH ? getZH(tempStr, REMARKSIZE, move) : getICCS(tempStr, REMARKSIZE, move));
    wcscat(movStr, L" ");
    wcscat(movStr, __getRemarkStr(tempStr, REMARKSIZE, move));

    if (move->omove != NULL) {
        __getMovString(movStr, n, move->omove, isPGN_ZH, true);
        wcscat(movStr, L")");
    }
    if (move->nmove != NULL)
        __getMovString(movStr, n, move->nmove, isPGN_ZH, false);
    return movStr;
}

wchar_t* getMovString_iccszh(wchar_t* movStr, size_t n, const Move* move, RecFormat fmt)
{
    movStr[0] = L'\x0';
    if (move->pmove == NULL) // 为根目录
        __getRemarkStr(movStr, n, move);
    __getMovString(movStr, n, move->pmove == NULL ? move->nmove : move, fmt == PGN_ZH, false);
    /*
    wchar_t pieStr[9] = {};
    swprintf(movStr, n, L"%d%d => %d%d remark: %s nextNo_:%d otherNo_:%d CC_ColNo_:%d\npmove:@%p nmove:@%p omove:@%p\n",
        (*move->fseat) ? (*move->fseat)->row : -1,
        (*move->fseat) ? (*move->fseat)->col : -1,
        (*move->tseat) ? (*move->tseat)->row : -1,
        (*move->tseat) ? (*move->tseat)->col : -1,
        getPieString(pieStr, 8, *move->tpiece ? *move->tpiece : NULL),
        move->nextNo_, move->otherNo_, move->CC_ColNo_,
        move->pmove, move->nmove, move->omove);
        */
    return movStr;
}
