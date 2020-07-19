#include "head/play.h"

void go(ChessManual cm)
{
    if (hasNext(cm->currentMove)) {
        cm->currentMove = getNext(cm->currentMove);
        doMove(cm->currentMove);
    }
}

void goOther(ChessManual cm)
{
    if (hasOther(cm->currentMove)) {
        undoMove(cm->currentMove);
        cm->currentMove = getOther(cm->currentMove);
        doMove(cm->currentMove);
    }
}

void goEnd(ChessManual cm)
{
    while (hasNext(cm->currentMove))
        go(cm);
}

void goTo(ChessManual cm, Move move)
{
    cm->currentMove = move;
    Move preMoves[getNextNo(move)];
    int count = getPreMoves(preMoves, move);
    for (int i = 0; i < count; ++i)
        doMove(preMoves[i]);
}

static void doBack__(ChessManual cm)
{
    undoMove(cm->currentMove);
    cm->currentMove = getSimplePre(cm->currentMove);
}

void back(ChessManual cm)
{
    if (hasPreOther(cm->currentMove))
        backOther(cm);
    else if (hasSimplePre(cm->currentMove))
        doBack__(cm);
}

void backNext(ChessManual cm)
{
    if (hasSimplePre(cm->currentMove) && !hasPreOther(cm->currentMove))
        doBack__(cm);
}

void backOther(ChessManual cm)
{
    if (hasPreOther(cm->currentMove)) {
        doBack__(cm); // 变着回退
        doMove(cm->currentMove); // 前变执行
    }
}

void backFirst(ChessManual cm)
{
    while (hasSimplePre(cm->currentMove))
        back(cm);
}

void backTo(ChessManual cm, Move move)
{
    while (hasSimplePre(cm->currentMove) && cm->currentMove != move)
        back(cm);
}

void goInc(ChessManual cm, int inc)
{
    int count = abs(inc);
    void (*func)(ChessManual) = inc > 0 ? go : back;
    while (count-- > 0)
        func(cm);
}

void changeChessManual(ChessManual cm, ChangeType ct)
{
    Move curMove = cm->currentMove;
    backFirst(cm);

    // info未更改
    changeBoard(cm->board, ct);
    Move firstMove = getNext(cm->rootMove);
    if (firstMove) {
        if (ct != EXCHANGE)
            changeMove(firstMove, cm->board, ct);
        if (ct == EXCHANGE || ct == SYMMETRY) {
            cm->movCount_ = cm->remCount_ = cm->maxRemLen_ = cm->maxRow_ = cm->maxCol_ = 0;
            setMoveNumZhStr__(cm, firstMove);
        }
    }

    goTo(cm, curMove);
}
