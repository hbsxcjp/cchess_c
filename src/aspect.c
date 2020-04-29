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
    MoveRec mr = malloc(sizeof(struct MoveRec));
    assert(mr);
    switch (mst) {
    case MovePtr:
        mr->move = (CMove)source;
        wchar_t tempRcStr[5];
        wcscpy(mr->rcStr, getRcStr_m(tempRcStr, mr->move));
        break;
    case MoveRcStrPtr:
        mr->move = NULL;
        wcscpy(mr->rcStr, (const wchar_t*)source);
        break;
    default:
        break;
    }
    mr->count = 1;
    mr->weight = 0;
    mr->preMoveRec = preMoveRec;
    return mr;
}

static void delMoveRec__(MoveRec mr)
{
    if (mr == NULL)
        return;
    MoveRec preMoveRec = mr->preMoveRec;
    free(mr);
    delMoveRec__(preMoveRec);
}

// 取得相同局面下的着法记录
static MoveRec getMoveRec__(MoveRec mr, const wchar_t* rcStr)
{
    while (mr && wcscmp(mr->rcStr, rcStr) != 0)
        mr = mr->preMoveRec;
    return mr;
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
    size = getPrime(size);
    Aspects aspects = malloc(sizeof(struct Aspects));
    aspects->size = size;
    aspects->length = aspects->movCount = 0;
    aspects->loadfactor = 0.8;
    aspects->lastAspects = malloc(size * sizeof(Aspect*));
    for (int i = 0; i < size; ++i)
        aspects->lastAspects[i] = NULL;
    return aspects;
}

Aspects newAspects(void)
{
    return newAspects__(0);
}

void delAspects(Aspects aspects)
{
    assert(aspects);
    for (int i = 0; i < aspects->size; ++i)
        delAspect__(aspects->lastAspects[i]);
    free(aspects);
}

int getAspects_length(Aspects aspects) { return aspects->length; }

// 取得局面哈希值取模后的数组序号值
inline static int getIndex_FEN__(const wchar_t* FEN, int size) { return BKDRHash(FEN) % size; }

// 取得最后的局面记录指针
inline static Aspect* getLastAspect__(CAspects aspects, const wchar_t* FEN) { return &aspects->lastAspects[getIndex_FEN__(FEN, aspects->size)]; }

// 取得相同哈希值下相同局面的记录
static Aspect getAspect__(Aspect asp, const wchar_t* FEN)
{
    while (asp && wcscmp(asp->FEN, FEN) != 0)
        asp = asp->preAspect;
    return asp;
}

MoveRec getAspect(CAspects aspects, const wchar_t* FEN)
{
    Aspect asp = getAspect__(*getLastAspect__(aspects, FEN), FEN);
    return asp ? asp->lastMoveRec : NULL;
}

inline static void insertAspect__(Aspect* plarc, Aspect lasp)
{
    lasp->preAspect = *plarc;
    *plarc = lasp;
}

// 检查容量，如果超出装载因子则扩容, 原局面(指针数组下内容)全面迁移至新表
static void checkCapacity__(Aspects aspects)
{
    if (aspects->length < aspects->size * aspects->loadfactor || aspects->size == INT_MAX)
        return;
    int size = getPrime(aspects->size);
    printf("%d: %d\n", __LINE__, size);
    fflush(stdout);
    Aspect *oldLastAspects = aspects->lastAspects,
           *newLastAspects = malloc(size * sizeof(Aspect*));
    for (int i = 0; i < size; ++i)
        newLastAspects[i] = NULL;
    for (int i = 0; i < aspects->size; ++i) {
        Aspect lasp = oldLastAspects[i];
        while (lasp) {
            Aspect preArc = lasp->preAspect,
                   *pnlarc = &newLastAspects[getIndex_FEN__(lasp->FEN, size)];
            insertAspect__(pnlarc, lasp);
            lasp = preArc;
        }
    }
    aspects->size = size;
    aspects->lastAspects = newLastAspects;
    free(oldLastAspects);
}

static MoveRec putAspect__(Aspects aspects, const wchar_t* FEN, const void* source, MoveSourceType mst)
{
    checkCapacity__(aspects);
    Aspect *plarc = getLastAspect__(aspects, FEN), asp = getAspect__(*plarc, FEN);
    if (asp == NULL) {
        asp = newAspect__(*plarc, FEN, source, mst);
        insertAspect__(plarc, asp);
        aspects->length++;
    } else {
        if (mst == MoveRcStrPtr) {
            MoveRec mr = getMoveRec__(asp->lastMoveRec, (const wchar_t*)source);
            if (mr) {
                mr->count++;
                return mr;
            }
        }
        asp->lastMoveRec = newMoveRec__(asp->lastMoveRec, source, mst);
    }
    aspects->movCount++;
    return asp->lastMoveRec;
}

MoveRec putAspect_bm(Aspects aspects, Board board, CMove move)
{
    wchar_t FEN[SEATNUM + 1];
    return putAspect__(aspects, getFEN_board(FEN, board), move, MovePtr);
}

MoveRec putAspect_fs(Aspects aspects, const wchar_t* FEN, const wchar_t* rcStr)
{
    return putAspect__(aspects, FEN, rcStr, MoveRcStrPtr);
}

bool removeAspect(Aspects aspects, const wchar_t* FEN, CMove move)
{
    bool finish = false;
    Aspect *plarc = getLastAspect__(aspects, FEN), asp = getAspect__(*plarc, FEN);
    if (asp) {
        MoveRec lmr = asp->lastMoveRec, mr = lmr;
        while (mr) {
            if (mr->move == move) {
                // 系最近着法
                if (asp->lastMoveRec == mr) {
                    // 有前着法
                    if (mr->preMoveRec)
                        asp->lastMoveRec = mr->preMoveRec;
                    // 本局面已无着法，则删除局面
                    else {
                        if (*plarc == asp)
                            *plarc = NULL;
                        else {
                            Aspect lasp = *plarc;
                            while (lasp->preAspect != asp)
                                lasp = lasp->preAspect;
                            lasp->preAspect = asp->preAspect;
                        }
                        delAspect__(asp);
                    }
                } else
                    lmr->preMoveRec = mr->preMoveRec;
                delMoveRec__(mr);
                finish = true;
                break;
            } else {
                lmr = mr;
                mr = mr->preMoveRec;
            }
        }
    }
    return finish;
}

int getLoopBoutCount(CAspects aspects, const wchar_t* FEN)
{
    int boutCount = 0;
    MoveRec lmr = getAspect(aspects, FEN), mr = lmr;
    if (lmr)
        while ((mr = mr->preMoveRec))
            if (isSameMove(lmr->move, mr->move) && isConnected(lmr->move, mr->move)) {
                boutCount = (getNextNo(lmr->move) - getNextNo(lmr->move)) / 2;
                break;
            }
    return boutCount;
}

static void moveRecLink__(MoveRec mr, void applyMr(MoveRec, void*), void* ptr)
{
    if (mr == NULL)
        return;
    applyMr(mr, ptr);
    moveRecLink__(mr->preMoveRec, applyMr, ptr);
}

static void aspectLink__(Aspect asp, void applyAsp(Aspect, void*),
    void moveRecLink(MoveRec, void(MoveRec, void*), void*), void applyMr(MoveRec, void*), void* ptr)
{
    if (asp == NULL)
        return;
    applyAsp(asp, ptr);
    moveRecLink(asp->lastMoveRec, applyMr, ptr);
    aspectLink__(asp->preAspect, applyAsp, moveRecLink__, applyMr, ptr);
}

void aspectsMap(CAspects aspects, void aspectLink(Aspect, void*), void* ptr)
{
    assert(aspects);
    for (int i = 0; i < aspects->size; ++i) {
        Aspect lasp = aspects->lastAspects[i];
        if (lasp)
            aspectLink(lasp, ptr);
    }
}

static void printfAspect__(Aspect asp, void* ptr)
{
    fwprintf((FILE*)ptr, L"FEN:%s\n", asp->FEN);
}

static void printfMoveRec__(MoveRec mr, void* ptr)
{
    wchar_t iccs[6];
    fwprintf((FILE*)ptr, L"\tmove@:%p iccs:%s\n", mr->move, getICCS(iccs, mr->move));
}

static void writeAspectStr__(Aspect lasp, void* ptr)
{
    aspectLink__(lasp, printfAspect__, moveRecLink__, printfMoveRec__, ptr);
}

void writeAspectsStr(FILE* fout, CAspects aspects)
{
    assert(aspects);
    aspectsMap(aspects, writeAspectStr__, fout);
    fwprintf(fout, L"\n【aspect size:%d length:%d movCount:%d】 ", aspects->size, aspects->length, aspects->movCount);
}

static void printfAspectLib__(Aspect asp, void* ptr)
{
    fwprintf((FILE*)ptr, L"\n%s\n", asp->FEN);
}

static void printfMoveRecLib__(MoveRec mr, void* ptr)
{
    fwprintf((FILE*)ptr, L"%s ", mr->rcStr);
}

static void writeAspectLib__(Aspect lasp, void* ptr)
{
    aspectLink__(lasp, printfAspectLib__, moveRecLink__, printfMoveRecLib__, ptr);
}

void storeAspects(FILE* fout, CAspects aspects)
{
    assert(aspects);
    aspectsMap(aspects, writeAspectLib__, fout);
}