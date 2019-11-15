#include "head/move.h"
#include "head/board.h"
#include "head/piece.h"
#include "head/tools.h"

static const wchar_t ICCSCHAR[] = L"abcdefghi";

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

void setRemark(Move* move, wchar_t* remark)
{
    remark = wtrim(remark);
    int len = wcslen(remark);
    if (len > 0) {
        move->remark = (wchar_t*)calloc(len + 1, sizeof(remark[0]));
        wcscpy(move->remark, remark);
    }
}
