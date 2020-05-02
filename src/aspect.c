#include "head/aspect.h"
#include "head/board.h"
#include "head/move.h"
#include "head/piece.h"
#include "head/tools.h"

typedef enum {
    MovePtrSource,
    MoveRecSource
} MoveSourceType;

struct MoveRec {
    CMove move; // 着法指针(主要用途：正在对局时，判断是否有违反行棋规则的棋例出现)
    int rowcols; // "rcrc"(主要用途：确定某局面下着法，根据count，weight分析着法优劣)
    int count; // 历史棋谱中某局面下该着法已发生的次数(0:表示后续有相同着法)
    int weight; // 对应某局面的本着价值权重(通过局面评价函数计算)
    MoveRec preMoveRec;
};

struct Aspect {
    wchar_t* FEN;
    MoveRec lastMoveRec;
    Aspect preAspect;
};

struct Aspects {
    int size, aspCount, movCount;
    double loadfactor;
    Aspect* lastAspects;
};

static int getRowCols__(unsigned int source) { return source >> 16; }
static int getCount__(unsigned int source) { return (source >> 8) & 0xFF; }
static int getWeight__(unsigned int source) { return source & 0xFF; }

static MoveRec newMoveRec__(MoveRec preMoveRec, const void* source, MoveSourceType mst)
{
    assert(source);
    MoveRec mr = malloc(sizeof(struct MoveRec));
    //assert(mr);
    mr->preMoveRec = preMoveRec;
    switch (mst) {
    case MovePtrSource: {
        mr->move = (CMove)source;
        mr->rowcols = getRowCols_m(mr->move);
        mr->count = 1;
        MoveRec pmr = mr;
        while ((pmr = pmr->preMoveRec))
            if (mr->rowcols == pmr->rowcols) {
                mr->count += pmr->count;
                pmr->count = 0; // 标记重复
                break; // 再往前推，如有重复在此之前也已被标记
            }
        mr->weight = 0;
    } break;
    case MoveRecSource: {
        mr->move = NULL;
        unsigned int src = *(unsigned int*)source;
        mr->rowcols = getRowCols__(src);
        mr->count = getCount__(src);
        mr->weight = getWeight__(src);
    } break;
    default:
        break;
    }
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

static Aspect newAspect__(Aspect preAspect, const wchar_t* FEN, const void* source, MoveSourceType mst)
{
    //assert(FEN);
    Aspect asp = malloc(sizeof(struct Aspect));
    //assert(asp);
    asp->FEN = malloc((wcslen(FEN) + 1) * sizeof(wchar_t));
    wcscpy(asp->FEN, FEN);
    asp->lastMoveRec = newMoveRec__(NULL, source, mst);
    asp->preAspect = preAspect;
    return asp;
}

static void delAspect__(Aspect asp)
{
    if (asp == NULL)
        return;
    Aspect preAspect = asp->preAspect;
    free(asp->FEN);
    delMoveRec__(asp->lastMoveRec);
    free(asp);
    delAspect__(preAspect);
}

static Aspects newAspects__(int size)
{
    size = getPrime(size);
    Aspects aspects = malloc(sizeof(struct Aspects));
    aspects->size = size;
    aspects->aspCount = aspects->movCount = 0;
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

static void putAspect__(Aspects aspects, const wchar_t* FEN, const void* source, MoveSourceType mst);

void setAspects_m(Move move, void* aspects, void* board)
{
    wchar_t FEN[SEATNUM + 1];
    putAspect__((Aspects)aspects, getFEN_board(FEN, (Board)board), move, MovePtrSource);
}

Aspects getAspects_bm(Board board, Move rootMove)
{
    Aspects aspects = newAspects();
    moveMap(rootMove, setAspects_m, aspects, board);
    return aspects;
}

Aspects getAspects_fin(const char* fileName)
{
    Aspects aspects = newAspects();
    FILE* fin = fopen(fileName, "r");
    wchar_t lineStr[FILENAME_MAX], FEN[SEATNUM], *sourceStr;
    unsigned int source = 0;
    while (fgetws(lineStr, FILENAME_MAX, fin)) {
        if (swscanf(lineStr, L"%s", FEN) != 1)
            continue;
        sourceStr = lineStr + wcslen(FEN);
        while (swscanf(sourceStr, L"%x", &source) == 1) { //wcslen(sourceStr) > 10 &&
            putAspect__(aspects, FEN, &source, MoveRecSource);
            sourceStr += 11; //source存储长度为11个字符:0x+8位数字+1个空格
        }
    }
    fclose(fin);
    return aspects;
}

void delAspects(Aspects aspects)
{
    assert(aspects);
    for (int i = 0; i < aspects->size; ++i)
        delAspect__(aspects->lastAspects[i]);
    free(aspects);
}

int getAspects_length(Aspects aspects) { return aspects->aspCount; }

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

// 检查容量，如果超出装载因子则扩容, 原局面(指针数组下内容)全面迁移至新表
static void checkCapacity__(Aspects aspects)
{
    if (aspects->aspCount < aspects->size * aspects->loadfactor || aspects->size == INT_MAX)
        return;
    int size = getPrime(aspects->size);
    //printf("%d: %d\n", __LINE__, size);
    //fflush(stdout);

    Aspect *oldLastAspects = aspects->lastAspects,
           *newLastAspects = malloc(size * sizeof(Aspect*)),
           asp, nasp, *pasp;
    for (int i = 0; i < size; ++i)
        newLastAspects[i] = NULL;
    for (int i = 0; i < aspects->size; ++i) {
        asp = oldLastAspects[i];
        while (asp) {
            Aspect preAsp = asp->preAspect;
            pasp = &newLastAspects[getIndex_FEN__(asp->FEN, size)];
            asp->preAspect = NULL; //切断原来的链接
            if (*pasp) {
                nasp = *pasp;
                while (nasp->preAspect)
                    nasp = nasp->preAspect;
                nasp->preAspect = asp;
            } else
                *pasp = asp;
            asp = preAsp;
        }
    }
    aspects->size = size;
    aspects->lastAspects = newLastAspects;
    free(oldLastAspects);
}

static void putAspect__(Aspects aspects, const wchar_t* FEN, const void* source, MoveSourceType mst)
{
    checkCapacity__(aspects);
    Aspect *pasp = getLastAspect__(aspects, FEN), asp = getAspect__(*pasp, FEN);
    // 排除重复局面
    if (asp)
        asp->lastMoveRec = newMoveRec__(asp->lastMoveRec, source, mst);
    else {
        *pasp = newAspect__(*pasp, FEN, source, mst);
        aspects->aspCount++;
    }
    aspects->movCount++;
}

/*
bool removeAspect(Aspects aspects, const wchar_t* FEN, CMove move)
{
    bool finish = false;
    Aspect *pasp = getLastAspect__(aspects, FEN), asp = getAspect__(*pasp, FEN);
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
                        if (*pasp == asp)
                            *pasp = NULL;
                        else {
                            Aspect lasp = *pasp;
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
//*/

int getLoopBoutCount(CAspects aspects, const wchar_t* FEN)
{
    int boutCount = 0;
    MoveRec lmr = getAspect(aspects, FEN), mr = lmr;
    if (lmr && lmr->move)
        while ((mr = mr->preMoveRec))
            if (isSameMove(lmr->move, mr->move) && isConnected(lmr->move, mr->move)) {
                boutCount = (getNextNo(lmr->move) - getNextNo(lmr->move)) / 2;
                break;
            }
    return boutCount;
}

static void moveRecLink__(MoveRec mr, void applyMr(MoveRec, void*), void* ptr)
{
    //if (mr == NULL)
    //    return;
    //applyMr(mr, ptr);
    //moveRecLink__(mr->preMoveRec, applyMr, ptr);//递归方式, 不能按顺序还原
    int count = 0;
    MoveRec mrs[FILENAME_MAX];
    do
        mrs[count++] = mr;
    while ((mr = mr->preMoveRec));
    for (int i = count - 1; i >= 0; --i)
        applyMr(mrs[i], ptr);
}

static void aspectLink__(Aspect asp, void applyAsp(Aspect, void*),
    void moveRecLink(MoveRec, void(MoveRec, void*), void*), void applyMr(MoveRec, void*), void* ptr)
{
    //if (asp == NULL)
    //    return;
    //applyAsp(asp, ptr);
    //moveRecLink(asp->lastMoveRec, applyMr, ptr);
    //aspectLink__(asp->preAspect, applyAsp, moveRecLink__, applyMr, ptr);
    //*
    int count = 0;
    Aspect asps[FILENAME_MAX];
    do
        asps[count++] = asp;
    while ((asp = asp->preAspect));
    for (int i = count - 1; i >= 0; --i) {
        applyAsp(asps[i], ptr);
        moveRecLink(asps[i]->lastMoveRec, applyMr, ptr);
    }
    //*/
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
    fwprintf(fout, L"\n【aspect size:%d aspCount:%d movCount:%d】 ", aspects->size, aspects->aspCount, aspects->movCount);
}

static void printfAspectLib__(Aspect asp, void* ptr)
{
    fwprintf((FILE*)ptr, L"\n%s ", asp->FEN);
}

static void printfMoveRecLib__(MoveRec mr, void* ptr)
{
    // 排除重复标记的着法
    if (mr->count > 0)
        fwprintf((FILE*)ptr, L"0x%04x%02x%02x ", mr->rowcols, mr->count, mr->weight);
}

static void storeAspectLib__(Aspect lasp, void* ptr)
{
    aspectLink__(lasp, printfAspectLib__, moveRecLink__, printfMoveRecLib__, ptr);
}

void storeAspects(FILE* fout, CAspects aspects)
{
    assert(aspects);
    aspectsMap(aspects, storeAspectLib__, fout);
}