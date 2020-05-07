#include "head/aspect.h"
#include "head/board.h"
#include "head/md5.h"
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
    int number; // 历史棋谱中某局面下该着法已发生的次数(0:表示后续有相同着法)
    int weight; // 对应某局面的本着价值权重(通过局面评价函数计算)
    MoveRec preMoveRec;
};

struct Aspect {
    wchar_t* FEN;
    unsigned char md5[16];
    MoveRec lastMoveRec;
    Aspect preAspect;
};

struct Aspects {
    int size, aspCount, movCount;
    double loadfactor;
    Aspect* lastAspects;
};

struct AspectAnalysis {
    int *mNumber, mSize, mCount; // 着法发生次数
    int *lmNumber, lmSize, lmCount; // 同一局面下着法个数
    int *laNumber, laSize, laCount; // 同一哈希值下局面个数
};

static int getRowCols__(unsigned int mrValue) { return mrValue >> 16; }
static int getCount__(unsigned int mrValue) { return (mrValue >> 8) & 0xFF; }
static int getWeight__(unsigned int mrValue) { return mrValue & 0xFF; }

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
        mr->number = 1;
        MoveRec pmr = mr;
        while ((pmr = pmr->preMoveRec))
            if (mr->rowcols == pmr->rowcols) {
                mr->number += pmr->number;
                pmr->number = 0; // 标记重复
                break; // 再往前推，如有重复在此之前也已被标记
            }
        mr->weight = 0;
    } break;
    case MoveRecSource: {
        mr->move = NULL;
        unsigned int mrValue = *(unsigned int*)source;
        mr->rowcols = getRowCols__(mrValue);
        mr->number = getCount__(mrValue);
        mr->weight = getWeight__(mrValue);
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
    char fen[SEATNUM];
    wcstombs(fen, FEN, SEATNUM);
    getMD5(asp->md5, fen);

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

void delAspects(Aspects aspects)
{
    assert(aspects);
    for (int i = 0; i < aspects->size; ++i)
        delAspect__(aspects->lastAspects[i]);
    free(aspects);
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

Aspects getAspects_fs(const char* fileName)
{
    Aspects aspects = newAspects();
    FILE* fin = fopen(fileName, "r");
    wchar_t lineStr[FILENAME_MAX], FEN[SEATNUM], *sourceStr;
    unsigned int mrValue = 0;
    while (fgetws(lineStr, FILENAME_MAX, fin)) {
        if (swscanf(lineStr, L"%s", FEN) != 1)
            continue;
        sourceStr = lineStr + wcslen(FEN);
        while (swscanf(sourceStr, L"%x", &mrValue) == 1) { //wcslen(sourceStr) > 10 &&
            putAspect__(aspects, FEN, &mrValue, MoveRecSource);
            sourceStr += 11; //source存储长度为11个字符:0x+8位数字+1个空格
        }
    }
    fclose(fin);
    return aspects;
}

Aspects getAspects_fb(const char* fileName)
{
    Aspects aspects = newAspects();
    FILE* fin = fopen(fileName, "rb");
    char md5[16], count = 0;
    unsigned int mrValue;
    while (fread(md5, 16, 1, fin) == 16) {
        fread(&count, 1, 1, fin);
        for (int i = 0; i < count; ++i) {
            fread(&mrValue, 4, 1, fin);
            putAspect__(aspects, L"", &mrValue, MoveRecSource);
        }
    }
    fclose(fin);
    return aspects;
}

int getAspects_length(Aspects aspects) { return aspects->aspCount; }

// 取得局面哈希值取模后的数组序号值
inline static int getIndex_FEN__(const wchar_t* FEN, int size) { return BKDRHash(FEN) % size; } //BKDRHash DJBHash SDBMHash

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
static void checkAspectsCapacity__(Aspects aspects)
{
    if (aspects->aspCount < aspects->size * aspects->loadfactor || aspects->size == INT_MAX)
        return;
    int size = getPrime(aspects->size);
    //printf("%d: %d\n", __LINE__, size);
    //fflush(stdout);

    Aspect *oldLastAspects = aspects->lastAspects,
           *newLastAspects = malloc(size * sizeof(Aspect*)),
           asp, preAsp, *pasp;
    for (int i = 0; i < size; ++i)
        newLastAspects[i] = NULL;
    for (int i = 0; i < aspects->size; ++i) {
        asp = oldLastAspects[i];
        if (asp == NULL)
            continue;
        do {
            preAsp = asp->preAspect;
            pasp = &newLastAspects[getIndex_FEN__(asp->FEN, size)];
            asp->preAspect = *pasp;
            *pasp = asp;
        } while ((asp = preAsp));
    }
    aspects->size = size;
    aspects->lastAspects = newLastAspects;
    free(oldLastAspects);
}

static void putAspect__(Aspects aspects, const wchar_t* FEN, const void* source, MoveSourceType mst)
{
    checkAspectsCapacity__(aspects);
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
    //aspectLink__(asp->preAspect, applyAsp, moveRecLink__, applyMr, ptr);//递归方式, 不能按顺序还原
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

static void printfMoveRec__(MoveRec mr, void* ptr)
{
    wchar_t iccs[6];
    fwprintf((FILE*)ptr, L"\tmove@:%p iccs:%s\n", mr->move, getICCS(iccs, mr->move));
}

static void printfAspect__(Aspect asp, void* ptr)
{
    fwprintf((FILE*)ptr, L"FEN:%s\n", asp->FEN);
}

static void writeAspect__(Aspect lasp, void* ptr)
{
    aspectLink__(lasp, printfAspect__, moveRecLink__, printfMoveRec__, ptr);
}

void writeAspects(FILE* fout, CAspects aspects)
{
    assert(aspects);
    aspectsMap(aspects, writeAspect__, fout);
    fwprintf(fout, L"\n\n【数组 局面数(使用):%d 着法数:%d 大小:%d 填充因子:%5.2f】\n",
        aspects->aspCount, aspects->movCount, aspects->size, (double)aspects->aspCount / aspects->size);
}

static void printfMoveRecLib__(MoveRec mr, void* ptr)
{
    // 排除重复标记的着法
    if (mr->number > 0)
        fwprintf((FILE*)ptr, L"0x%04x%02x%02x ", mr->rowcols, mr->number, mr->weight);
}

static void printfAspectLib__(Aspect asp, void* ptr)
{
    fwprintf((FILE*)ptr, L"\n%s ", asp->FEN);
}

static void storeAspect__(Aspect lasp, void* ptr)
{
    aspectLink__(lasp, printfAspectLib__, moveRecLink__, printfMoveRecLib__, ptr);
}

void storeAspects(FILE* fout, CAspects aspects)
{
    assert(aspects);
    aspectsMap(aspects, storeAspect__, fout);
}

static void printfMoveRecMD5__(MoveRec mr, void* ptr)
{
    // 排除重复标记的着法
    if (mr->number > 0) {
        unsigned int mrValue = (mr->rowcols << 16) | (mr->number << 8) | mr->weight;
        fwrite(&mrValue, 4, 1, (FILE*)ptr); // 4个字节
    }
}

static void printfAspectMD5__(Aspect asp, void* ptr)
{
    fwrite(asp->md5, 16, 1, (FILE*)ptr); // 16个字节
    char count = 1;
    MoveRec mr = asp->lastMoveRec;
    while ((mr = mr->preMoveRec))
        count++;
    fwrite(&count, 1, 1, (FILE*)ptr); // 1个字节
}

static void storeAspectMD5__(Aspect lasp, void* ptr)
{
    aspectLink__(lasp, printfAspectMD5__, moveRecLink__, printfMoveRecMD5__, ptr);
}

void storeAspectMD5(FILE* fout, CAspects aspects)
{
    assert(aspects);
    aspectsMap(aspects, storeAspectMD5__, fout);
}

static AspectAnalysis newAspectAnalysis__(void)
{
    AspectAnalysis aa = malloc(sizeof(struct AspectAnalysis));
    aa->mSize = aa->lmSize = aa->laSize = 256;
    aa->mCount = aa->lmCount = aa->laCount = 0;
    aa->mNumber = malloc(aa->mSize * sizeof(int));
    aa->lmNumber = malloc(aa->lmSize * sizeof(int));
    aa->laNumber = malloc(aa->laSize * sizeof(int));
    return aa;
}

static void delAspectAnalysis__(AspectAnalysis aa)
{
    free(aa->mNumber);
    free(aa->lmNumber);
    free(aa->laNumber);
    free(aa);
}

static void checkApendArray__(int** array, int* size, int* count, int value)
{
    if (*count >= *size) {
        *size += *size;
        *array = realloc(*array, *size * sizeof(int));
    }
    (*array)[(*count)++] = value;
}

static void calculateMoveRecLib__(MoveRec mr, void* ptr)
{
    AspectAnalysis aa = (AspectAnalysis)ptr;
    checkApendArray__(&aa->mNumber, &aa->mSize, &aa->mCount, mr->number);
}

static void calculateAspectLib__(Aspect asp, void* ptr)
{
    AspectAnalysis aa = (AspectAnalysis)ptr;
    int mcount = 1;
    MoveRec mr = asp->lastMoveRec;
    while ((mr = mr->preMoveRec))
        mcount++;
    checkApendArray__(&aa->lmNumber, &aa->lmSize, &aa->lmCount, mcount);
}

static void analyzeAspect__(Aspect lasp, void* ptr)
{
    aspectLink__(lasp, calculateAspectLib__, moveRecLink__, calculateMoveRecLib__, ptr);
}

static void calWriteCount__(FILE* fout, const wchar_t* entry, int* number, int size, int count)
{
    if (count <= 0 || size <= 0)
        return;
    int imax = 0, total = 0;
    for (int i = 0; i < count; ++i) {
        imax = max(imax, number[i]);
        total += number[i];
    }
    double ave = (double)total / count, scale = 1.0 * count / size, varinace = 0, difference = 0, stdDiff = 0;
    for (int i = 0; i < count; ++i) {
        difference = number[i] - ave;
        varinace += difference * difference;
    }
    varinace /= count - 1;
    stdDiff = sqrt(varinace);
    fwprintf(fout, L"分析%8s => 平均:%.2f 最大:%2d 总数:%d 方差:%.2f 标准差:%.2f 【数组 使用:%d 大小:%d 填充因子:%5.2f】\n",
        entry, ave, imax, total, varinace, stdDiff, count, size, scale);
}

void analyzeAspects(FILE* fout, CAspects aspects)
{
    assert(aspects);
    AspectAnalysis aa = newAspectAnalysis__();
    for (int i = 0; i < aspects->size; ++i) {
        Aspect asp = aspects->lastAspects[i];
        if (asp == NULL)
            continue;
        int count = 1;
        while ((asp = asp->preAspect))
            count++;
        checkApendArray__(&aa->laNumber, &aa->laSize, &aa->laCount, count);
    }
    aspectsMap(aspects, analyzeAspect__, aa);
    fwprintf(fout, L"\n\n【数组 局面数(使用):%d 着法数:%d 大小:%d 填充因子:%5.2f】\n",
        aspects->aspCount, aspects->movCount, aspects->size, (double)aspects->aspCount / aspects->size);
    calWriteCount__(fout, L"move", aa->mNumber, aa->mSize, aa->mCount);
    calWriteCount__(fout, L"moveRec", aa->lmNumber, aa->lmSize, aa->lmCount);
    calWriteCount__(fout, L"aspect", aa->laNumber, aa->laSize, aa->laCount);

    delAspectAnalysis__(aa);
}