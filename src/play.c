#include "head/play.h"

struct Play {
    ChessManual cm;
    Move curMove;
};

Play newPlay(const char* fileName)
{
    Play play = malloc(sizeof(struct Play));
    assert(play);
    play->cm = newChessManual(fileName);
    play->curMove = getRootMove(play->cm);
    return play;
}

void delPlay(Play play)
{
    delChessManual(play->cm);
    free(play);
}

void go(Play play)
{
    if (getNext(play->curMove)) {
        play->curMove = getNext(play->curMove);
        doMove(play->curMove);
    }
}

void goOther(Play play)
{
    if (getOther(play->curMove)) {
        undoMove(play->curMove);
        play->curMove = getOther(play->curMove);
        doMove(play->curMove);
    }
}

void goEnd(Play play)
{
    while (getNext(play->curMove))
        go(play);
}

void goTo(Play play, Move move)
{
    play->curMove = move;
    Move preMoves[getNextNo(move)];
    int count = getPreMoves(preMoves, move);
    for (int i = 0; i < count; ++i)
        doMove(preMoves[i]);
}

static void doBack__(Play play)
{
    undoMove(play->curMove);
    play->curMove = getSimplePre(play->curMove);
}

void back(Play play)
{
    if (hasPreOther(play->curMove))
        backOther(play);
    else if (getSimplePre(play->curMove))
        doBack__(play);
}

void backNext(Play play)
{
    if (getSimplePre(play->curMove) && !hasPreOther(play->curMove))
        doBack__(play);
}

void backOther(Play play)
{
    if (hasPreOther(play->curMove)) {
        doBack__(play); // 变着回退
        doMove(play->curMove); // 前变执行
    }
}

void backFirst(Play play)
{
    while (getSimplePre(play->curMove))
        back(play);
}

void backTo(Play play, Move move)
{
    while (getSimplePre(play->curMove) && play->curMove != move)
        back(play);
}

void goInc(Play play, int inc)
{
    int count = abs(inc);
    void (*func)(Play) = inc > 0 ? go : back;
    while (count-- > 0)
        func(play);
}
