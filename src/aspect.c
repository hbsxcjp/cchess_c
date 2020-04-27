#include "head/aspect.h"
#include "head/board.h"
#include "head/move.h"
#include "head/piece.h"
#include "head/tools.h"

typedef enum {
    MovePtr,
    MoveRcStrPtr
} MoveSourceType;

struct MoveRec {
    CMove move; // 着法指针
    wchar_t rcStr[8]; // "rcrc"
    int count; // 发生次数
    int weight; // 对应某局面的本着价值权重(通过局面评价函数计算)
    MoveRec preMoveRec;
};

struct Aspect {
    wchar_t* FEN;
    MoveRec lastMoveRec;
    Aspect preAspect;
};

struct Aspects {
    int size, length, movCount;
    double loadfactor;
    Aspect* lastAspects;
};

static MoveRec newMoveRec__(MoveRec preMoveRec, const void* source, MoveSourceType mst)
{
    assert(source);
    MoveRec mrc = malloc(sizeof(struct MoveRec));
    assert(mrc);
    switch (mst) {
    case MovePtr:
        mrc->move = (CMove)source;
        wchar_t tempRcStr[5];
        wcscpy(mrc->rcStr, getRcStr_m(tempRcStr, mrc->move));
        break;
    case MoveRcStrPtr:
        mrc->move = NULL;
        wcscpy(mrc->rcStr, (const wchar_t*)source);
        break;
    default:
        break;
    }
    mrc->count = 1;
    mrc->weight = 0;
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

// 取得相同局面下的着法记录
static MoveRec getMoveRec__(MoveRec mrc, const wchar_t* rcStr)
{
    while (mrc && wcscmp(mrc->rcStr, rcStr) != 0)
        mrc = mrc->preMoveRec;
    return mrc;
}

static Aspect newAspect__(Aspect preAspect, const wchar_t* FEN, const void* source, MoveSourceType mst)
{
    assert(FEN);
    Aspect aspect = malloc(sizeof(struct Aspect));
    assert(aspect);
    aspect->FEN = malloc((wcslen(FEN) + 1) * sizeof(wchar_t));
    wcscpy(aspect->FEN, FEN);
    aspect->lastMoveRec = newMoveRec__(NULL, source, mst);
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

static Aspects newAspects__(int size)
{
    //static int primes[] = { 509, 509, 1021, 2053, 4093, 8191, 16381, 32771, 65521, INT_MAX };
    //int size = 1 << 10; // 如果哈希函数的散列效果较好，容量长度可以不用素数。使用2的幂，容易取模、翻番
    //int i = 1;
    //for (; primes[i] < size; i++)
    //    ;

    Aspects aspects = malloc(sizeof(struct Aspects) + size * sizeof(Aspect*));
    aspects->size = size;
    aspects->length = aspects->movCount = 0;
    aspects->loadfactor = 0.8;
    aspects->lastAspects = (Aspect*)(aspects + 1);
    for (int i = 0; i < size; ++i)
        aspects->lastAspects[i] = NULL;
    return aspects;
}

Aspects newAspects(void)
{
    return newAspects__(509);
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
    return &aspects->lastAspects[BKDRHash(FEN) % aspects->size]; // 等效于：& (aspects->size - 1)
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

// 原局面全面迁移至新表
static Aspects expandCapacity__(Aspects aspects)
{
    int size = aspects->size > INT_MAX / 2 ? INT_MAX : aspects->size * 2;
    Aspects newAspects = newAspects__(size);
    newAspects->length = aspects->length;
    newAspects->movCount = aspects->movCount;
    for (int i = 0; i < aspects->size; ++i) {
        Aspect larc = aspects->lastAspects[i];
        if (larc)
            *getLastAspect__(newAspects, larc->FEN) = larc;
    }
    free(aspects);
    return newAspects;
}

static MoveRec putAspect__(Aspects aspects, const wchar_t* FEN, const void* source, MoveSourceType mst)
{
    Aspect *plarc = getLastAspect__(aspects, FEN), arc = getAspect__(*plarc, FEN);
    if (arc == NULL) { // 表中不存在该局面，则添加aspect、move
        // 检查容量，如果超出装载因子则扩容
        if (aspects->length > aspects->size * aspects->loadfactor || aspects->size == INT_MAX)
            ; //aspects = expandCapacity__(aspects);
        arc = newAspect__(*plarc, FEN, source, mst);
        *plarc = arc;
        aspects->length++;
    } else { // 表中已存在该局面
        if (mst == MoveRcStrPtr) {
            MoveRec mrc = getMoveRec__(arc->lastMoveRec, (const wchar_t*)source);
            if (mrc) {
                mrc->count++;
                return mrc;
            }
        }
        arc->lastMoveRec = newMoveRec__(arc->lastMoveRec, source, mst);
    }
    aspects->movCount++;
    return arc->lastMoveRec;
}

MoveRec putAspect_fm(Aspects aspects, const wchar_t* FEN, CMove move)
{
    return putAspect__(aspects, FEN, move, MovePtr);
}

MoveRec putAspect_bm(Aspects aspects, Board board, CMove move)
{
    wchar_t FEN[SEATNUM + 1];
    return putAspect__(aspects, getFEN_board(FEN, board), move, MovePtr);
}

MoveRec putAspect_txt(Aspects aspects, const wchar_t* FEN, const wchar_t* rcStr)
{
    return putAspect__(aspects, FEN, rcStr, MoveRcStrPtr);
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
    fwprintf(fout, L"\n【aspect size:%d length:%d movCount:%d】 ", aspects->size, aspects->length, aspects->movCount);
}
