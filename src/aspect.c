#include "head/aspect.h"
#include "head/board.h"
#include "head/md5.h"
#include "head/move.h"
#include "head/tools.h"

struct MoveRec {
    CMove move; // 着法指针(主要用途：正在对局时，判断是否有违反行棋规则的棋例出现)
    int rowcols; // "rcrc"(主要用途：确定某局面下着法，根据count，weight分析着法优劣)
    int number; // 历史棋谱中某局面下该着法已发生的次数(0:表示后续有相同着法);本局面的重复次数
    int weight; // 对应某局面的本着价值权重(通过局面评价函数计算)
    MoveRec preMoveRec;
};

struct Aspect {
    char* express; // 局面表示的指针
    MoveRec lastMoveRec;
    Aspect preAspect;
};

struct Aspects {
    int size, aspCount, movCount;
    double loadfactor;
    Aspect* lastAspects;
    SourceType hst; // 数据源类型，决定Aspect的express字段解释（fen or md5）
};

struct AspectAnalysis {
    int *mNumber, mSize, mCount; // 着法发生次数
    int *lmNumber, lmSize, lmCount; // 同一局面下着法个数
    int *laNumber, laSize, laCount; // 同一哈希值下局面个数
};

static const wchar_t ASPLIB_MARK[] = L"AspectLib\n";

static int getRowCols__(int mrValue) { return mrValue >> 16; }
static int getCount__(int mrValue) { return (mrValue >> 8) & 0xFF; }
static int getWeight__(int mrValue) { return mrValue & 0xFF; }
static int getMRValue__(int rowcols, int number, int weight) { return (rowcols << 16) | (number << 8) | weight; }

static MoveRec newMoveRec__(MoveRec pmr, const void* mrSource, SourceType st)
{
    assert(mrSource);
    MoveRec mr = malloc(sizeof(struct MoveRec));
    mr->preMoveRec = pmr;
    switch (st) {
    case FEN_MovePtr: {
        mr->move = (CMove)mrSource;
        mr->rowcols = getRowCols_m(mr->move);
        mr->number = 1;
        while (pmr) {
            if (pmr->number && (mr->rowcols == pmr->rowcols)) {
                mr->number += pmr->number;
                pmr->number = 0; // 标记重复
                break; // 再往前推，如有重复在此之前也已被标记
            }
            pmr = pmr->preMoveRec;
        }
        mr->weight = 0; //待完善
    } break;
    case FEN_MRValue:
    case MD5_MRValue: {
        mr->move = NULL;
        int mrValue = *(int*)mrSource;
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

static Aspect newAspect__(Aspect pasp, char* aspSource, const void* mrSource, SourceType st)
{
    assert(aspSource);
    assert(mrSource);
    Aspect asp = malloc(sizeof(struct Aspect));
    asp->express = aspSource;
    asp->lastMoveRec = newMoveRec__(NULL, mrSource, st);
    asp->preAspect = pasp;
    return asp;
}

static void delAspect__(Aspect asp)
{
    if (asp == NULL)
        return;
    Aspect pasp = asp->preAspect;
    free(asp->express);
    delMoveRec__(asp->lastMoveRec);
    free(asp);
    delAspect__(pasp);
}

Aspects newAspects(SourceType hst, int size)
{
    size = getPrime(size);
    Aspects asps = malloc(sizeof(struct Aspects));
    asps->size = size;
    asps->aspCount = asps->movCount = 0;
    asps->loadfactor = 0.85;
    asps->lastAspects = malloc(size * sizeof(Aspect*));
    for (int i = 0; i < size; ++i)
        asps->lastAspects[i] = NULL;
    asps->hst = hst;
    return asps;
}

void delAspects(Aspects asps)
{
    assert(asps);
    for (int i = 0; i < asps->size; ++i)
        delAspect__(asps->lastAspects[i]);
    free(asps->lastAspects);
    free(asps);
}

inline static int getLastIndex__(SourceType st, int size, char* aspSource)
{
    return BKDRHash_c(aspSource, st == MD5_MRValue ? MD5LEN : strlen(aspSource)) % size;
}

// 依据asps->hst, asps->size, aspSource 取得最后的局面记录指针
inline static Aspect* getLastAspect__(CAspects asps, char* aspSource)
{
    return &asps->lastAspects[getLastIndex__(asps->hst, asps->size, aspSource)];
}

// 取得相同哈希值下某个局面的记录
static Aspect getAspect__(Aspect asp, char* aspSource, SourceType hst)
{
    int (*cmpfun)(const char*, const char*) = hst == MD5_MRValue ? md5cmp : strcmp;
    while (asp && cmpfun(asp->express, aspSource) != 0)
        asp = asp->preAspect;
    return asp;
}

static void moveRecLink__(MoveRec mr, void applyMr(MoveRec, void*), void* ptr)
{
    if (mr == NULL)
        return;
    applyMr(mr, ptr);
    moveRecLink__(mr->preMoveRec, applyMr, ptr); //递归方式
}

static void aspectLink__(Aspect asp, void applyAsp(Aspect, void*),
    void moveRecLink(MoveRec, void(MoveRec, void*), void*), void applyMr(MoveRec, void*), void* ptr)
{
    if (asp == NULL)
        return;
    applyAsp(asp, ptr);
    if (moveRecLink)
        moveRecLink(asp->lastMoveRec, applyMr, ptr);
    aspectLink__(asp->preAspect, applyAsp, moveRecLink, applyMr, ptr);
}

void aspectsMap(CAspects asps, void startAspectLink(Aspect, void*), void* ptr)
{
    assert(asps);
    for (int i = 0; i < asps->size; ++i) {
        Aspect lasp = asps->lastAspects[i];
        if (lasp)
            startAspectLink(lasp, ptr);
    }
}

static void reloadAspect__(Aspect asp, void* newAsps)
{
    Aspect preAsp, *pasp;
    while (asp) {
        preAsp = asp->preAspect;
        pasp = getLastAspect__((Aspects)newAsps, asp->express);
        asp->preAspect = *pasp;
        *pasp = asp;
        asp = preAsp;
    };
}

static void startReloadAspect__(Aspect lasp, void* newAsps)
{
    aspectLink__(lasp, reloadAspect__, NULL, NULL, newAsps);
}

static void transToMD5__(Aspect asp, void* ptr)
{
    char* oldExpress = asp->express;
    asp->express = (char*)getMD5(oldExpress);
    free(oldExpress);
}

static void transAspectMD5__(Aspect lasp, void* ptr)
{
    aspectLink__(lasp, transToMD5__, NULL, NULL, ptr);
}

// 源局面(指针数组下内容)全面迁移至新局面
static void reloadLastAspects__(Aspects asps, SourceType newHst, int newSize)
{
    if (newHst == asps->hst && newSize == asps->size)
        return;

    asps->hst = newHst;
    int oldSize = asps->size;
    asps->size = newSize;
    assert(newSize >= oldSize);
    Aspect* oldLastAspects = asps->lastAspects;
    Aspects newAsps = newAspects(newHst, newSize);
    if (newHst == MD5_MRValue)
        aspectsMap(asps, transAspectMD5__, NULL);
    aspectsMap(asps, startReloadAspect__, newAsps);
    asps->lastAspects = newAsps->lastAspects;
    free(newAsps);
    /*
    Aspect *oldLastAspects = asps->lastAspects, asp, preAsp, *pasp;
    asps->lastAspects = malloc(newSize * sizeof(Aspect*));
    for (int i = 0; i < newSize; ++i)
        asps->lastAspects[i] = NULL;
    for (int i = 0; i < oldSize; ++i) {
        asp = oldLastAspects[i];
        while (asp) {
            preAsp = asp->preAspect;
            pasp = getLastAspect__(asps, asp->express);
            asp->preAspect = *pasp;
            *pasp = asp;
            asp = preAsp;
        };
    }
    //*/
    free(oldLastAspects);
}

// 检查容量，如果超出装载因子则扩容
static void checkAspectsCapacity__(Aspects asps)
{
    if (asps->aspCount < asps->size * asps->loadfactor || asps->size == INT_MAX)
        return;

    reloadLastAspects__(asps, asps->hst, getPrime(asps->size << 2));
}

static void putAspect__(Aspects asps, char* aspSource, const void* mrSource, SourceType st)
{
    assert(asps->hst == st);
    checkAspectsCapacity__(asps);
    Aspect *pasp = getLastAspect__(asps, aspSource),
           asp = getAspect__(*pasp, aspSource, asps->hst);
    // 排除重复局面
    if (asp)
        asp->lastMoveRec = newMoveRec__(asp->lastMoveRec, mrSource, st);
    else {
        *pasp = newAspect__(*pasp, aspSource, mrSource, st);
        asps->aspCount++;
    }
    asps->movCount++;
}

static char* getfen_board__(wchar_t* FEN)
{
    char* fen = malloc(wcslen(FEN) * 2 + 1);
    wcstombs(fen, FEN, wcslen(FEN) * 2);
    return fen;
}

void transToMD5Aspects(Aspects asps)
{
    reloadLastAspects__(asps, MD5_MRValue, asps->size);
}

void setAspects_mb(Move move, void* asps, void* board)
{
    wchar_t FEN[SEATNUM + 1];
    getFEN_board(FEN, board);
    putAspect__((Aspects)asps, getfen_board__(FEN), move, FEN_MovePtr);
}

void setAspects_fs(Aspects asps, const char* fileName)
{
    FILE* fin = fopen(fileName, "r");
    wchar_t lineStr[FILENAME_MAX], FEN[SEATNUM], *sourceStr;
    fgetws(lineStr, FILENAME_MAX, fin);
    assert(wcscmp(lineStr, ASPLIB_MARK) == 0); // 检验文件标志
    fgetws(lineStr, FILENAME_MAX, fin); // 去掉一空行
    unsigned int mrValue = 0;
    while (fgetws(lineStr, FILENAME_MAX, fin) && lineStr[0] != L'\n') { // 遇到空行(只有字符'\n')则结束
        if (swscanf(lineStr, L"%s", FEN) != 1)
            continue;
        sourceStr = lineStr + wcslen(FEN);
        while (wcslen(sourceStr) > 10 && swscanf(sourceStr, L"%x", &mrValue) == 1) {
            putAspect__(asps, getfen_board__(FEN), &mrValue, FEN_MRValue);
            sourceStr += 11; //source存储长度为11个字符:0x+8位数字+1个空格
        }
    }
    fclose(fin);
}

void setAspects_fb(Aspects asps, const char* fileName)
{
    assert(asps->hst == MD5_MRValue);
    FILE* fin = fopen(fileName, "rb");
    char* md5 = malloc(MD5LEN);
    int count = 0;
    while (fread(md5, MD5LEN, 1, fin) == 1) {
        if (fread(&count, sizeof(int), 1, fin) != 1) {
            printf("%d: %s\n", __LINE__, fileName);
            fflush(stdout);
            break;
        }
        unsigned int mrValue[count];
        if (fread(mrValue, sizeof(unsigned int), count, fin) != count) {
            printf("%d: %s\n", __LINE__, fileName);
            fflush(stdout);
            break;
        } else {
            //printf("%d: %08x %d\n", __LINE__, mrValue[0], count);
            //fflush(stdout);
        }
        for (int i = 0; i < count; ++i)
            putAspect__(asps, md5, &mrValue[i], MD5_MRValue);

        md5 = malloc(MD5LEN);
    }
    fclose(fin);
}

int getAspects_length(Aspects asps) { return asps->aspCount; }

/*
int getLoopBoutCount(CAspects asps, const wchar_t* FEN)
{
    int boutCount = 0;
    MoveRec lmr = getMoveRec__(asps, FEN, FEN_MovePtr), mr = lmr;
    if (lmr && lmr->move)
        while ((mr = mr->preMoveRec))
            if (isSameMove(lmr->move, mr->move) && isConnected(lmr->move, mr->move)) {
                boutCount = (getNextNo(lmr->move) - getNextNo(lmr->move)) / 2;
                break;
            }
    return boutCount;
}
//*/

static void printfMoveRec__(MoveRec mr, void* ptr)
{
    wchar_t iccs[6];
    fwprintf((FILE*)ptr, L"\tmove@:%p iccs:%s\n", mr->move, getICCS(iccs, mr->move));
}

static void printfAspect__(Aspect asp, void* ptr)
{
    fprintf((FILE*)ptr, "FEN:%s\n", asp->express);
}

static void writeAspect__(Aspect lasp, void* ptr)
{
    aspectLink__(lasp, printfAspect__, moveRecLink__, printfMoveRec__, ptr);
}

void writeAspectStr(FILE* fout, CAspects asps)
{
    assert(asps->hst <= FEN_MRValue);
    aspectsMap(asps, writeAspect__, fout);
    fprintf(fout, "\n【数组 局面数(使用):%d 着法数:%d 大小:%d 填充因子:%5.2f SourceType:%d】\n",
        asps->aspCount, asps->movCount, asps->size, (double)asps->aspCount / asps->size, asps->hst);
}

static void printfMoveRecLib__(MoveRec mr, void* ptr)
{
    if (mr->number > 0)
        fprintf((FILE*)ptr, "0x%08x ", getMRValue__(mr->rowcols, mr->number, mr->weight));
}

static void printfAspectLib__(Aspect asp, void* ptr)
{
    fprintf((FILE*)ptr, "\n%s ", asp->express);
}

static void storeAspectLib__(Aspect lasp, void* ptr)
{
    aspectLink__(lasp, printfAspectLib__, moveRecLink__, printfMoveRecLib__, ptr);
}

void storeAspectLib(FILE* fout, CAspects asps)
{
    assert(asps->hst <= FEN_MRValue);
    fwprintf(fout, ASPLIB_MARK);
    aspectsMap(asps, storeAspectLib__, fout);
}

static void writeMoveRecMD5__(MoveRec mr, void* ptr)
{
    // 排除重复标记的着法
    if (mr->number > 0) {
        int mrValue = getMRValue__(mr->rowcols, mr->number, mr->weight);
        fwrite(&mrValue, sizeof(int), 1, (FILE*)ptr); // 4个字节
    }
}

static void writeAspectMD5__(Aspect asp, void* ptr)
{
    fwrite(asp->express, MD5LEN, 1, (FILE*)ptr); // 16个字节
    int count = 1;
    MoveRec mr = asp->lastMoveRec;
    while ((mr = mr->preMoveRec) && mr->number > 0)
        count++;
    fwrite(&count, sizeof(int), 1, (FILE*)ptr); // 4个字节
}

static void storeAspectMD5__(Aspect lasp, void* ptr)
{
    aspectLink__(lasp, writeAspectMD5__, moveRecLink__, writeMoveRecMD5__, ptr);
}

void storeAspectMD5(FILE* fout, CAspects asps)
{
    assert(asps->hst == MD5_MRValue);
    aspectsMap(asps, storeAspectMD5__, fout);
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

static void calWriteCount__(FILE* fout, const char* entry, int* number, int size, int count)
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
    fprintf(fout, "分析%8s => 平均:%.2f 最大:%2d 总数:%d 方差:%.2f 标准差:%.2f 【数组 使用:%d 大小:%d 填充因子:%5.2f】\n",
        entry, ave, imax, total, varinace, stdDiff, count, size, scale);
}

void analyzeAspects(FILE* fout, CAspects asps)
{
    assert(asps);
    AspectAnalysis aa = newAspectAnalysis__();
    for (int i = 0; i < asps->size; ++i) {
        Aspect asp = asps->lastAspects[i];
        if (asp == NULL)
            continue;
        int count = 1;
        while ((asp = asp->preAspect))
            count++;
        checkApendArray__(&aa->laNumber, &aa->laSize, &aa->laCount, count);
    }
    aspectsMap(asps, analyzeAspect__, aa);
    fprintf(fout, "\n\n【数组 局面数(使用):%d 着法数:%d 大小:%d 填充因子:%5.2f SourceType:%d】\n",
        asps->aspCount, asps->movCount, asps->size, (double)asps->aspCount / asps->size, asps->hst);
    calWriteCount__(fout, "move", aa->mNumber, aa->mSize, aa->mCount);
    calWriteCount__(fout, "moveRec", aa->lmNumber, aa->lmSize, aa->lmCount);
    calWriteCount__(fout, "aspect", aa->laNumber, aa->laSize, aa->laCount);

    delAspectAnalysis__(aa);
}
