#include "head/aspect.h"
#include "head/board.h"
#include "head/md5.h"
#include "head/move.h"
#include "head/piece.h"
#include "head/tools.h"

struct MoveRec {
    CMove move; // 着法指针(主要用途：正在对局时，判断是否有违反行棋规则的棋例出现)
    int rowcols; // "rcrc"(主要用途：确定某局面下着法，根据count，weight分析着法优劣)
    int number; // 历史棋谱中某局面下该着法已发生的次数(0:表示后续有相同着法);本局面的重复次数
    int weight; // 对应某局面的本着价值权重(通过局面评价函数计算)
    MoveRec preMoveRec;
};

struct Aspect {
    wchar_t* FEN;
    unsigned char md5[MD5LEN];
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

static int getRowCols__(unsigned int mrValue) { return mrValue >> MD5LEN; }
static int getCount__(unsigned int mrValue) { return (mrValue >> 8) & 0xFF; }
static int getWeight__(unsigned int mrValue) { return mrValue & 0xFF; }

static MoveRec newMoveRec__(MoveRec pmr, const void* mrSource, SourceType st)
{
    assert(mrSource);
    MoveRec mr = malloc(sizeof(struct MoveRec));
    //assert(mr);
    mr->preMoveRec = pmr;
    switch (st) {
    case FEN_MovePtr: {
        mr->move = (CMove)mrSource;
        mr->rowcols = getRowCols_m(mr->move);
        mr->number = 1;
        pmr = mr;
        while ((pmr = pmr->preMoveRec))
            if (pmr->number && (mr->rowcols == pmr->rowcols)) {
                mr->number += pmr->number;
                pmr->number = 0; // 标记重复
                break; // 再往前推，如有重复在此之前也已被标记
            }
        mr->weight = 0; //待完善
    } break;
    case FEN_MRStr:
    case MD5_MRValue: {
        mr->move = NULL;
        unsigned int mrValue = *(unsigned int*)mrSource;
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
    MoveRec pmr = mr->preMoveRec;
    free(mr);
    delMoveRec__(pmr);
}

static Aspect newAspect__(Aspect pasp, const void* aspSource, const void* mrSource, SourceType st)
{
    assert(aspSource);
    assert(mrSource);
    Aspect asp = malloc(sizeof(struct Aspect));
    assert(asp);
    switch (st) {
    case FEN_MovePtr:
    case FEN_MRStr: {
        wchar_t* FEN = (wchar_t*)aspSource;
        asp->FEN = malloc((wcslen(FEN) + 1) * sizeof(wchar_t));
        wcscpy(asp->FEN, FEN);
        char fen[SEATNUM];
        wcstombs(fen, FEN, SEATNUM * 2);
        getMD5(asp->md5, fen);
    } break;
    case MD5_MRValue: {
        asp->FEN = NULL;
        unsigned char* md5 = (unsigned char*)aspSource;
        for (int i = 0; i < MD5LEN; ++i)
            asp->md5[i] = md5[i];
    } break;
    default:
        break;
    }
    asp->lastMoveRec = newMoveRec__(NULL, mrSource, st);
    asp->preAspect = pasp;
    return asp;
}

static void delAspect__(Aspect asp)
{
    if (asp == NULL)
        return;
    Aspect pasp = asp->preAspect;
    free(asp->FEN);
    delMoveRec__(asp->lastMoveRec);
    free(asp);
    delAspect__(pasp);
}

static Aspects newAspects__(int size)
{
    size = getPrime(size);
    Aspects aspects = malloc(sizeof(struct Aspects));
    aspects->size = size;
    aspects->aspCount = aspects->movCount = 0;
    aspects->loadfactor = 0.85;
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

static void putAspect__(Aspects aspects, const void* aspSource, const void* mrSource, SourceType st);

void setAspects_m(Move move, void* aspects, void* board)
{
    wchar_t FEN[SEATNUM + 1];
    putAspect__((Aspects)aspects, getFEN_board(FEN, (Board)board), move, FEN_MovePtr);
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
            putAspect__(aspects, FEN, &mrValue, FEN_MRStr);
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
    unsigned char md5[MD5LEN], count = 0;
    unsigned int mrValue = 0;
    while (fread(md5, MD5LEN, 1, fin) == 1) {
        if (fread(&count, 1, 1, fin) != 1)
            break;
        for (int i = 0; i < count; ++i) {
            if (fread(&mrValue, 4, 1, fin) != 1)
                break;
            putAspect__(aspects, md5, &mrValue, MD5_MRValue);
        }
    }
    fclose(fin);
    return aspects;
}

int getAspects_length(Aspects aspects) { return aspects->aspCount; }

static unsigned int BKDRHash__(const void* aspSource, SourceType st)
{
    return st == MD5_MRValue ? BKDRHash_c((unsigned char*)aspSource, MD5LEN) : BKDRHash_s((const wchar_t*)aspSource);
}

// 取得最后的局面记录指针
inline static Aspect* getLastAspect__(CAspects aspects, const void* aspSource, SourceType st)
{
    return &aspects->lastAspects[BKDRHash__(aspSource, st) % aspects->size];
}

// 取得相同哈希值下某个局面的记录
static Aspect getAspect__(Aspect asp, const void* aspSource, SourceType st)
{
    while (asp && !(st <= FEN_MRStr ? wcscmp(asp->FEN, (wchar_t*)aspSource) == 0 : MD5IsSame(asp->md5, (unsigned char*)aspSource)))
        asp = asp->preAspect;
    return asp;
}

/*
static MoveRec getMoveRec__(CAspects aspects, const void* aspSource, SourceType st)
{
    Aspect asp = getAspect__(*getLastAspect__(aspects, aspSource, st), aspSource, st);
    return asp ? asp->lastMoveRec : NULL;
}
//*/

// 检查容量，如果超出装载因子则扩容, 原局面(指针数组下内容)全面迁移至新表
static void checkAspectsCapacity__(Aspects aspects, SourceType st)
{
    if (aspects->aspCount < aspects->size * aspects->loadfactor || aspects->size == INT_MAX)
        return;

    Aspect *oldLastAspects = aspects->lastAspects,
           asp, preAsp, *pasp;
    int oldSize = aspects->size;
    aspects->size = getPrime(oldSize);
    aspects->lastAspects = malloc(aspects->size * sizeof(Aspect*));
    for (int i = 0; i < aspects->size; ++i)
        aspects->lastAspects[i] = NULL;
    for (int i = 0; i < oldSize; ++i) {
        asp = oldLastAspects[i];
        while (asp) {
            preAsp = asp->preAspect;
            const void* source = (st == MD5_MRValue ? (const void*)asp->md5 : (const void*)asp->FEN);
            pasp = getLastAspect__(aspects, source, st);
            asp->preAspect = *pasp;
            *pasp = asp;
            asp = preAsp;
        };
    }
    free(oldLastAspects);
}

static void putAspect__(Aspects aspects, const void* aspSource, const void* mrSource, SourceType st)
{
    checkAspectsCapacity__(aspects, st);
    Aspect *pasp = getLastAspect__(aspects, aspSource, st), asp = getAspect__(*pasp, aspSource, st);
    // 排除重复局面
    if (asp)
        asp->lastMoveRec = newMoveRec__(asp->lastMoveRec, mrSource, st);
    else {
        *pasp = newAspect__(*pasp, aspSource, mrSource, st);
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

int getLoopBoutCount(CAspects aspects, const wchar_t* FEN)
{
    int boutCount = 0;
    MoveRec lmr = getMoveRec__(aspects, FEN, FEN_MovePtr), mr = lmr;
    if (lmr && lmr->move)
        while ((mr = mr->preMoveRec))
            if (isSameMove(lmr->move, mr->move) && isConnected(lmr->move, mr->move)) {
                boutCount = (getNextNo(lmr->move) - getNextNo(lmr->move)) / 2;
                break;
            }
    return boutCount;
}
//*/

static void moveRecLink__(MoveRec mr, void applyMr(MoveRec, void*), void* ptr)
{
    //if (mr == NULL)
    //    return;
    //applyMr(mr, ptr);
    //moveRecLink__(mr->preMoveRec, applyMr, ptr);//递归方式, 不能按顺序还原
    int index = 0;
    MoveRec mrs[FILENAME_MAX] = { mr };
    while ((mr = mr->preMoveRec))
        mrs[++index] = mr;
    for (int i = index; i >= 0; --i)
        applyMr(mrs[i], ptr);
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
        unsigned int mrValue = (mr->rowcols << MD5LEN) | (mr->number << 8) | mr->weight;
        fwrite(&mrValue, 4, 1, (FILE*)ptr); // 4个字节
    }
}

static void printfAspectMD5__(Aspect asp, void* ptr)
{
    fwrite(asp->md5, MD5LEN, 1, (FILE*)ptr); // 16个字节
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