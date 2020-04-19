#include "head/move_aider.h"
#include "head/board.h"
#include "head/move.h"
#include "head/piece.h"
#include "head/tools.h"

struct MoveRec {
    CMove move; // 着法指针
    wchar_t cmpStr[8]; // 比较着法是否相等字符串
    MoveRec preMoveRec;
};

struct AspectRec {
    wchar_t pieceStr[SEATNUM + 1];
    MoveRec moveRec;
};

struct AspectRecTable {
    int size, length;
    AspectRec* aspectRecs;
};

static int hash__(AspectRecTable table, const wchar_t* pieceStr)
{
    //table->hash(pieceStr) % table->size;
    return 0;
}

static MoveRec newMoveRec__(MoveRec preMoveRec, CMove move)
{
    assert(move);
    MoveRec moveRec = malloc(sizeof(struct MoveRec));
    assert(moveRec);
    moveRec->move = move;
    getCmpMoveStr(moveRec->cmpStr, move);
    moveRec->preMoveRec = preMoveRec;
    return moveRec;
}

static void delMoveRec__(MoveRec moveRec)
{
    if (moveRec == NULL)
        return;
    MoveRec preMoveRec = moveRec->preMoveRec;
    free(moveRec);
    delMoveRec__(preMoveRec);
}

AspectRecTable newAspectRecTable(void)
{
    int firstSize = 509;
    AspectRecTable table;
    table = malloc(sizeof(struct AspectRecTable) + firstSize * sizeof(AspectRec*));
    table->size = firstSize;
    table->length = 0;
    table->aspectRecs = (AspectRec*)(table + 1);
    for (int i = 0; i < firstSize; ++i)
        table->aspectRecs[i] = NULL;
    return table;
}

void delAspectRecTable(AspectRecTable table)
{
    assert(table);
    if (table->length > 0) {
        AspectRec a, p = NULL;
        for (int i = 0; i < table->size; ++i)
            for (a = table->aspectRecs[i]; a; a = p) {
                delMoveRec__(a->moveRec);
                free(a);
            }
    }
    free(table);
}

int aspectRec_length(AspectRecTable table) { return table->length; }

static AspectRec getAspectRec__(AspectRecTable table, const wchar_t* pieceStr)
{
    return table->aspectRecs[hash__(table, pieceStr)];
}

MoveRec aspectRec_get(AspectRecTable table, const wchar_t* pieceStr)
{
    AspectRec p = getAspectRec__(table, pieceStr);
    return p ? p->moveRec : NULL;
}

MoveRec aspectRec_put(AspectRecTable table, const wchar_t* pieceStr, CMove move)
{
    assert(table);
    assert(pieceStr && wcslen(pieceStr) == SEATNUM);
    assert(move);
    AspectRec p = getAspectRec__(table, pieceStr);
    if (p == NULL) {
        p = malloc(sizeof(*p));
        wcscpy(p->pieceStr, pieceStr);
        p->moveRec = newMoveRec__(NULL, move);
        table->aspectRecs[hash__(table, pieceStr)] = p;
    } else
        p->moveRec = newMoveRec__(p->moveRec, move);
    return p->moveRec;
}

MoveRec aspectRec_remove(AspectRecTable table, const wchar_t* pieceStr)
{
    return NULL;
}

/*
static bool isSameMove__(Move move, Move othMove)
{
    return move->fseat == othMove->fseat && move->tseat == othMove->tseat;
}

static bool isSameBout__(Move* bout, Move* preBout)
{
    for (int i = 0; i < 2; ++i)
        if (!isSameMove__(preBout[i], bout[i]))
            return false;
    return true;
}//*/

int getLoopCount(Move move)
{
    int count = 0;
    /* 可能存在走子位置相同但吃子的情况，并不是重复！
    if (!hasSimplePre(move))
        return count;
    Move bout[2] = { move, move = getPre(move) };
    while (hasSimplePre(move)) {
        Move preBout[2] = { move = getPre(move) };
        if (!hasSimplePre(move))
            break;
        preBout[1] = getPre(move);
        if (!isSameBout__(bout, preBout))
            break;
        count++;
    }//*/
    return count;
}
