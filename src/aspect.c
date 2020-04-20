#include "head/aspect.h"
#include "head/board.h"
#include "head/move.h"
#include "head/piece.h"
#include "head/tools.h"

struct MoveRec {
    CMove move; // 着法指针
    wchar_t cmpStr[8]; // 比较着法是否相等字符串
    MoveRec preMoveRec;
};

struct Aspect {
    wchar_t pieceStr[SEATNUM + 1];
    MoveRec lastMoveRec;
    Aspect preAspect;
};

struct AspectTable {
    int size, length;
    Aspect* lastAspects;
};

static MoveRec newMoveRec__(MoveRec preMoveRec, CMove move)
{
    assert(move);
    MoveRec mrc = malloc(sizeof(struct MoveRec));
    assert(mrc);
    mrc->move = move;
    getCmpMoveStr(mrc->cmpStr, move);
    mrc->preMoveRec = preMoveRec;
    return mrc;
}

static void delMoveRec__(MoveRec mrc)
{
    if (mrc == NULL)
        return;
    MoveRec preMoveRec = mrc->preMoveRec;
    free(mrc);
    delMoveRec__(preMoveRec);
}

static Aspect newAspect__(Aspect preAspect, const wchar_t* pieceStr, CMove move)
{
    assert(pieceStr && wcslen(pieceStr) == SEATNUM);
    Aspect aspect = malloc(sizeof(struct Aspect));
    assert(aspect);
    wcscpy(aspect->pieceStr, pieceStr);
    aspect->lastMoveRec = newMoveRec__(NULL, move);
    aspect->preAspect = preAspect;
    return aspect;
}

static void delAspect__(Aspect aspect)
{
    if (aspect == NULL)
        return;
    Aspect preAspect = aspect->preAspect;
    delMoveRec__(aspect->lastMoveRec);
    free(aspect);
    delAspect__(preAspect);
}

// BKDR Hash Function
unsigned int BKDRHash(wchar_t* wstr)
{
    unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
    unsigned int hash = 0;

    while (*wstr) {
        hash = hash * seed + (*wstr++);
    }

    return (hash & 0x7FFFFFFF);
}

AspectTable newAspectTable(void)
{
    int size = 509;
    AspectTable table = malloc(sizeof(struct AspectTable) + size * sizeof(Aspect*));
    table->size = size;
    table->length = 0;
    table->lastAspects = (Aspect*)(table + 1);
    for (int i = 0; i < size; ++i)
        table->lastAspects[i] = NULL;
    return table;
}

void delAspectTable(AspectTable table)
{
    assert(table);
    if (table->length > 0) {
        Aspect arc, pre = NULL;
        for (int i = 0; i < table->size; ++i)
            for (arc = table->lastAspects[i]; arc; arc = pre) {
                pre = arc->preAspect;
                delAspect__(arc);
            }
    }
    free(table);
}

int aspectTable_length(AspectTable table) { return table->length; }

// 取得最后的局面记录
static Aspect* getLastAspect__(AspectTable table, const wchar_t* pieceStr)
{
    return &table->lastAspects[BKDRHash((wchar_t*)pieceStr)];
}

// 取得相同哈希值下相同局面的记录
static Aspect getAspect__(Aspect arc, const wchar_t* pieceStr)
{
    while (arc && wcscmp(arc->pieceStr, pieceStr) != 0)
        arc = arc->preAspect;
    return arc;
}

MoveRec aspectTable_get(AspectTable table, const wchar_t* pieceStr)
{
    Aspect arc = getAspect__(*getLastAspect__(table, pieceStr), pieceStr);
    return arc ? arc->lastMoveRec : NULL;
}

MoveRec aspectTable_put(AspectTable table, const wchar_t* pieceStr, CMove move)
{
    Aspect *parc = getLastAspect__(table, pieceStr), arc = getAspect__(*parc, pieceStr);
    if (arc == NULL) {
        arc = newAspect__(*parc, pieceStr, move);
        *parc = arc;
    }
    return arc->lastMoveRec;
}

bool aspectTable_remove(AspectTable table, const wchar_t* pieceStr, CMove move)
{
    bool finish = false;
    Aspect *parc = getLastAspect__(table, pieceStr), arc = getAspect__(*parc, pieceStr);
    if (arc) {
        MoveRec lmrc = arc->lastMoveRec, mrc = lmrc;
        while (mrc) {
            if (mrc->move == move) {
                if (arc->lastMoveRec == mrc) {
                    if (*parc == arc)
                        *parc = NULL;
                    else {
                        Aspect larc = *parc;
                        while (larc->preAspect != arc)
                            larc = larc->preAspect;
                        larc->preAspect = arc->preAspect;
                    }
                    delAspect__(arc);
                } else
                    lmrc->preMoveRec = mrc->preMoveRec;
                delMoveRec__(mrc);
                finish = true;
                break;
            }
            lmrc = mrc;
            mrc = mrc->preMoveRec;
        }
    }
    return finish;
}

int getLoopCount(AspectTable table, const wchar_t* pieceStr)
{
    int count = 0;
    MoveRec lmrc = aspectTable_get(table, pieceStr);
    if (lmrc) {
        CMove move = lmrc->move;
        MoveRec mrc = lmrc->preMoveRec;
        while (mrc) {
            // 着法相等，且着法连通
            if (wcscmp(lmrc->cmpStr, mrc->cmpStr) == 0) {
                while (move) {
                    if (move == mrc->move) {
                        count++;
                        break;
                    }
                    move = getPre(move);
                }
            }
            mrc = mrc->preMoveRec;
        }
    }
    return count;
}
