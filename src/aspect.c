#include "head/aspect.h"
#include "head/board.h"
#include "head/move.h"
#include "head/piece.h"
#include "head/tools.h"

struct MoveRec {
    CMove move; // 着法指针
    MoveRec preMoveRec;
};

struct Aspect {
    wchar_t* FEN;
    MoveRec lastMoveRec;
    Aspect preAspect;
};

struct Aspects {
    int size, length, movCount;
    Aspect* lastAspects;
};

static MoveRec newMoveRec__(MoveRec preMoveRec, CMove move)
{
    assert(move);
    MoveRec mrc = malloc(sizeof(struct MoveRec));
    assert(mrc);
    mrc->move = move;
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

static Aspect newAspect__(Aspect preAspect, const wchar_t* FEN, CMove move)
{
    assert(FEN);
    Aspect aspect = malloc(sizeof(struct Aspect));
    assert(aspect);
    aspect->FEN = malloc((wcslen(FEN) + 1) * sizeof(wchar_t));
    wcscpy(aspect->FEN, FEN);
    aspect->lastMoveRec = newMoveRec__(NULL, move);
    aspect->preAspect = preAspect;
    return aspect;
}

static void delAspect__(Aspect aspect)
{
    if (aspect == NULL)
        return;
    Aspect preAspect = aspect->preAspect;
    free(aspect->FEN);
    delMoveRec__(aspect->lastMoveRec);
    free(aspect);
    delAspect__(preAspect);
}

Aspects newAspects(void)
{
    int size = 509; //509,1021,2053,4093,8191,16381,32771,65521,INT_MAX
    Aspects aspects = malloc(sizeof(struct Aspects) + size * sizeof(Aspect*));
    aspects->size = size;
    aspects->length = aspects->movCount = 0;
    aspects->lastAspects = (Aspect*)(aspects + 1);
    for (int i = 0; i < size; ++i)
        aspects->lastAspects[i] = NULL;
    return aspects;
}

void delAspects(Aspects aspects)
{
    assert(aspects);
    for (int i = 0; i < aspects->size; ++i)
        delAspect__(aspects->lastAspects[i]);
    free(aspects);
}

int getAspects_length(Aspects aspects) { return aspects->length; }

// 取得最后的局面记录
static Aspect* getLastAspect__(CAspects aspects, const wchar_t* FEN)
{
    return &aspects->lastAspects[BKDRHash(FEN) % aspects->size];
}

// 取得相同哈希值下相同局面的记录
static Aspect getAspect__(Aspect arc, const wchar_t* FEN)
{
    while (arc && wcscmp(arc->FEN, FEN) != 0)
        arc = arc->preAspect;
    return arc;
}

MoveRec getAspect(CAspects aspects, const wchar_t* FEN)
{
    Aspect arc = getAspect__(*getLastAspect__(aspects, FEN), FEN);
    return arc ? arc->lastMoveRec : NULL;
}

MoveRec putAspect(Aspects aspects, const wchar_t* FEN, CMove move)
{
    Aspect *plarc = getLastAspect__(aspects, FEN), arc = getAspect__(*plarc, FEN);
    if (arc == NULL) { // 表中不存在该局面，则添加aspect、move
        arc = newAspect__(*plarc, FEN, move);
        assert(arc);
        *plarc = arc;
        aspects->length++;
    } else // 表中已存在该局面，则添加move
        arc->lastMoveRec = newMoveRec__(arc->lastMoveRec, move);
    aspects->movCount++;
    assert(arc->lastMoveRec);
    return arc->lastMoveRec;
}

MoveRec putAspect_b(Aspects aspects, Board board, CMove move)
{
    wchar_t FEN[SEATNUM + 1];
    return putAspect(aspects, getFEN_board(FEN, board), move);
}

bool removeAspect(Aspects aspects, const wchar_t* FEN, CMove move)
{
    bool finish = false;
    Aspect *plarc = getLastAspect__(aspects, FEN), arc = getAspect__(*plarc, FEN);
    if (arc) {
        MoveRec lmrc = arc->lastMoveRec, mrc = lmrc;
        while (mrc) {
            if (mrc->move == move) {
                // 系最近着法
                if (arc->lastMoveRec == mrc) {
                    // 有前着法
                    if (mrc->preMoveRec)
                        arc->lastMoveRec = mrc->preMoveRec;
                    // 本局面已无着法，则删除局面
                    else {
                        if (*plarc == arc)
                            *plarc = NULL;
                        else {
                            Aspect larc = *plarc;
                            while (larc->preAspect != arc)
                                larc = larc->preAspect;
                            larc->preAspect = arc->preAspect;
                        }
                        delAspect__(arc);
                    }
                } else
                    lmrc->preMoveRec = mrc->preMoveRec;
                delMoveRec__(mrc);
                finish = true;
                break;
            } else {
                lmrc = mrc;
                mrc = mrc->preMoveRec;
            }
        }
    }
    return finish;
}

int getLoopBoutCount(CAspects aspects, const wchar_t* FEN)
{
    int boutCount = 0;
    MoveRec lmrc = getAspect(aspects, FEN), mrc = lmrc;
    if (lmrc)
        while ((mrc = mrc->preMoveRec))
            if (isSameMove(lmrc->move, mrc->move) && isConnected(lmrc->move, mrc->move)) {
                boutCount = (getNextNo(lmrc->move) - getNextNo(lmrc->move)) / 2;
                break;
            }
    return boutCount;
}

static void writeMoveRecStr__(FILE* fout, MoveRec mrc)
{
    if (mrc == NULL)
        return;
    wchar_t iccs[6];
    fwprintf(fout, L"\t\tmove@:%p iccs:%s\n", mrc->move, getICCS(iccs, mrc->move));
    writeMoveRecStr__(fout, mrc->preMoveRec);
}

static void writeAspectStr__(FILE* fout, Aspect asp)
{
    if (asp == NULL)
        return;
    fwprintf(fout, L"\tFEN:%s\n", asp->FEN);
    writeMoveRecStr__(fout, asp->lastMoveRec);
    writeAspectStr__(fout, asp->preAspect);
}

void writeAspectsStr(FILE* fout, CAspects aspects)
{
    assert(aspects);
    int a = 0;
    for (int i = 0; i < aspects->size; ++i) {
        Aspect lasp = aspects->lastAspects[i];
        if (lasp) {
            fwprintf(fout, L"\n%3d. HASH:%4d\n", ++a, i);
            writeAspectStr__(fout, lasp);
        }
    }
    fwprintf(fout, L"\naspect_count:%d aspect_movCount:%d ", aspects->length, aspects->movCount);
}
