#include "head/move.h"
#include "head/board.h"
#include "head/piece.h"

static const wchar_t ICCSCHAR[] = L"abcdefghi";

Move* newMove(void)
{
    Move* move = malloc(sizeof(Move));
    move->fseat = move->tseat = NULL;
    move->tpiece = NULL;
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
    if (move->omove)
        delMove(move->omove);
    else if (move->nmove)
        delMove(move->nmove);
    else
        free(move);
}

void cutNextMove(Move* move)
{
    Move* cmove = move->nmove;
    if (cmove && cmove->omove) {
        move->nmove = cmove->omove;
        cmove->omove = NULL;
    }
    delMove(cmove);
}

void cutOhterMove(Move* move)
{

    Move* cmove = move->omove;
    if (cmove && cmove->omove) {
        move->omove = cmove->omove;
        cmove->omove = NULL;
    }
    delMove(cmove);
}

wchar_t* getICCS(wchar_t* str, size_t n, const Move* move)
{
    swprintf(str, n, L"%c%d%c%d",
        ICCSCHAR[(*move->fseat)->col], (*move->fseat)->row,
        ICCSCHAR[(*move->tseat)->col], (*move->tseat)->row);
    return str;
}

wchar_t* getZH(wchar_t* str, size_t n, const Move* move)
{
    return str;
}

void moveDo(Instance* ins, const Move* move)
{
    *move->tpiece = seatMoveTo(ins->board, *move->fseat, *move->tseat, NULL);
}

void moveUndo(Instance* ins, const Move* move)
{
    seatMoveTo(ins->board, *move->tseat, *move->fseat, *move->tpiece);
}

void setMoveFromSeats(Move* move, const Seat fseat, const Seat tseat, const wchar_t* remark)
{
    *move->fseat = getSeat_rc(fseat.row, fseat.col);
    *move->tseat = getSeat_rc(tseat.row, tseat.col);
    wcscpy(move->remark, remark);
}

void setMoveFromStr(Move* move,
    const wchar_t* str, RecFormat fmt, const wchar_t* remark) {}

wchar_t* getMovString(wchar_t* str, size_t n, const Move* move)
{
    wchar_t pieStr[9] = {};
    swprintf(str, n, L"%d%d => %d%d %s remark: %s pmove:@%p nmove:@%p omove:@%p nextNo_:%d otherNo_:%d CC_ColNo_:%d\n",
        (*move->fseat) ? (*move->fseat)->row : -1,
        (*move->fseat) ? (*move->fseat)->col : -1,
        (*move->tseat) ? (*move->tseat)->row : -1,
        (*move->tseat) ? (*move->tseat)->col : -1,
        getPieString(pieStr, 8, *move->tpiece),
        move->pmove, move->nmove, move->omove,
        move->nextNo_, move->otherNo_, move->CC_ColNo_);
    return str;
}
