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

static MoveRec newMoveRec__(const void* mrSource, SourceType st)
{
    assert(mrSource);
    MoveRec mr = malloc(sizeof(struct MoveRec));
    mr->preMoveRec = NULL;
    switch (st) {
    case FEN_MovePtr: {
        mr->move = (CMove)mrSource;
        mr->rowcols = getRowCols_m(mr->move);
        mr->number = 1;
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

static Aspect newAspect__(char* aspSource, const void* mrSource, SourceType st)
{
    assert(aspSource);
    assert(mrSource);
    Aspect asp = malloc(sizeof(struct Aspect));
    asp->express = aspSource;
    asp->lastMoveRec = newMoveRec__(mrSource, st);
    asp->preAspect = NULL;
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
    Aspect preAsp = asp->preAspect;
    applyAsp(asp, ptr);
    if (moveRecLink)
        moveRecLink(asp->lastMoveRec, applyMr, ptr);
    aspectLink__(preAsp, applyAsp, moveRecLink, applyMr, ptr);
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

inline static int getLastIndex__(SourceType st, int size, char* aspSource)
{
    /*
    int index = BKDRHash_c(aspSource, st == MD5_MRValue ? MD5LEN : strlen(aspSource)) % size;
    if (st == MD5_MRValue) {
        unsigned char* md5 = (unsigned char*)aspSource;
        printf("MD5:");
        for (int i = 0; i < MD5LEN; i++)
            printf("%02x", md5[i]);
    } else
        printf("FEN:%s", aspSource);
    printf(" index:%d\n", index);
    //*/
    return BKDRHash_c(aspSource, st == MD5_MRValue ? MD5LEN : strlen(aspSource)) % size;
}

// 依据aspSource取得最后的局面记录指针
inline static Aspect* getLastAspect__(CAspects asps, char* aspSource)
{
    return &asps->lastAspects[getLastIndex__(asps->hst, asps->size, aspSource)];
}

// 取得某个局面
static Aspect getAspect__(CAspects asps, char* aspSource)
{
    Aspect asp = *getLastAspect__(asps, aspSource);
    int (*cmpfun)(const char*, const char*) = asps->hst == MD5_MRValue ? md5cmp : strcmp;
    while (asp && cmpfun(asp->express, aspSource) != 0)
        asp = asp->preAspect;
    return asp;
}

static void loadAspect__(Aspect asp, void* newAsps)
{
    Aspect* pasp = getLastAspect__((Aspects)newAsps, asp->express);
    asp->preAspect = *pasp;
    *pasp = asp;
}

static void startLoadAspect__(Aspect lasp, void* newAsps)
{
    aspectLink__(lasp, loadAspect__, NULL, NULL, newAsps);
}

// 源局面(指针数组下内容)全面迁移至新局面
static void reloadLastAspects__(Aspects asps, int newSize)
{
    Aspects newAsps = newAspects(asps->hst, newSize);
    aspectsMap(asps, startLoadAspect__, newAsps);

    asps->size = newAsps->size;
    free(asps->lastAspects);
    asps->lastAspects = newAsps->lastAspects;
    free(newAsps);
}

static void putAspect__(Aspects asps, char* aspSource, const void* mrSource, SourceType st)
{
    assert(asps->hst == st);
    // 检查容量，如果超出装载因子则扩容
    if (asps->aspCount >= asps->size * asps->loadfactor && asps->size < INT_MAX)
        reloadLastAspects__(asps, asps->size + 1);

    Aspect asp = getAspect__(asps, aspSource);

    if (asp) { // 已有相同局面
        MoveRec pmr = asp->lastMoveRec, mr = newMoveRec__(mrSource, st);
        if (st == FEN_MovePtr) { 
            // 寻找相同着法
            while (pmr) {
                if (pmr->number && (mr->rowcols == pmr->rowcols)) {
                    mr->number += pmr->number;
                    pmr->number = 0; // 标记重复
                    break; // 再往前推，如有重复在此之前也已被标记
                }
                pmr = pmr->preMoveRec;
            }
            // 插入方式
            mr->preMoveRec = asp->lastMoveRec;
            asp->lastMoveRec = mr;
        } else { 
            // 追加方式
            while (pmr->preMoveRec)
                pmr = pmr->preMoveRec;
            pmr->preMoveRec = mr;
        }
    } else {
        loadAspect__(newAspect__(aspSource, mrSource, st), asps);
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

void setAspects_mb(Move move, void* asps, void* board)
{
    wchar_t FEN[SEATNUM + 1];
    getFEN_board(FEN, board);
    putAspect__((Aspects)asps, getfen_board__(FEN), move, FEN_MovePtr);
}

Aspects getAspects_fs(const char* fileName)
{
    Aspects asps = newAspects(FEN_MRValue, 0);
    FILE* fin = fopen(fileName, "r");
    wchar_t lineStr[FILENAME_MAX], FEN[SEATNUM], *sourceStr;
    fgetws(lineStr, FILENAME_MAX, fin);
    assert(wcscmp(lineStr, ASPLIB_MARK) == 0); // 检验文件标志
    fgetws(lineStr, FILENAME_MAX, fin); // 去掉一空行
    int mrValue = 0;
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
    return asps;
}

Aspects getAspects_fb(const char* fileName)
{
    Aspects asps = newAspects(MD5_MRValue, 0);
    FILE* fin = fopen(fileName, "rb");
    char* md5 = malloc(MD5LEN);
    int count = 0;
    while (fread(md5, MD5LEN, 1, fin) == 1) {
        if (fread(&count, sizeof(int), 1, fin) != 1)
            break;
        int mrValue[count];
        if (fread(mrValue, sizeof(int), count, fin) != count)
            break;
        for (int i = 0; i < count; ++i)
            putAspect__(asps, md5, &mrValue[i], MD5_MRValue);

        md5 = malloc(MD5LEN);
    }
    fclose(fin);
    return asps;
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

static void startWriteAspect__(Aspect lasp, void* ptr)
{
    aspectLink__(lasp, printfAspect__, moveRecLink__, printfMoveRec__, ptr);
}

void writeAspectStr(char* fileName, CAspects asps)
{
    assert(asps->hst == FEN_MovePtr);
    FILE* fout = fopen(fileName, "w");
    aspectsMap(asps, startWriteAspect__, fout);
    fprintf(fout, "\n【数组 局面数(使用):%d 着法数:%d 大小:%d 填充因子:%5.2f SourceType:%d】\n",
        asps->aspCount, asps->movCount, asps->size, (double)asps->aspCount / asps->size, asps->hst);
    fclose(fout);
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

static void startStoreAspectLib__(Aspect lasp, void* ptr)
{
    aspectLink__(lasp, printfAspectLib__, moveRecLink__, printfMoveRecLib__, ptr);
}

void storeAspectLib(char* fileName, CAspects asps)
{
    FILE* fout = fopen(fileName, "w");
    assert(asps->hst <= FEN_MRValue);
    fwprintf(fout, ASPLIB_MARK);
    aspectsMap(asps, startStoreAspectLib__, fout);
    fclose(fout);
}

static void transToMD5__(Aspect asp, void* ptr)
{
    char* oldExpress = asp->express;
    asp->express = (char*)getMD5(oldExpress);
    free(oldExpress);
}

static void startTransAspectMD5__(Aspect lasp, void* ptr)
{
    aspectLink__(lasp, transToMD5__, NULL, NULL, ptr);
}

// 非MD5格式局面数据转换成MD5格式局面数据
static void transToMD5Aspects__(Aspects asps)
{
    assert(asps);
    aspectsMap(asps, startTransAspectMD5__, NULL);
    asps->hst = MD5_MRValue; // 修改源格式
    reloadLastAspects__(asps, asps->size); // 尺寸保持不变
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

static void startStoreAspectMD5__(Aspect lasp, void* ptr)
{
    aspectLink__(lasp, writeAspectMD5__, moveRecLink__, writeMoveRecMD5__, ptr);
}

void storeAspectMD5(char* fileName, Aspects asps)
{
    FILE* fout = fopen(fileName, "wb");
    transToMD5Aspects__(asps);
    assert(asps->hst == MD5_MRValue);
    aspectsMap(asps, startStoreAspectMD5__, fout);
    fclose(fout);
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

static void startAnalyzeAspect__(Aspect lasp, void* ptr)
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

void analyzeAspects(char* fileName, CAspects asps)
{
    assert(asps);
    FILE* fout = fopen(fileName, "a");
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
    aspectsMap(asps, startAnalyzeAspect__, aa);
    fprintf(fout, "【数组 局面数(使用):%d 着法数:%d 大小:%d 填充因子:%5.2f SourceType:%d】\n",
        asps->aspCount, asps->movCount, asps->size, (double)asps->aspCount / asps->size, asps->hst);
    calWriteCount__(fout, "move", aa->mNumber, aa->mSize, aa->mCount);
    calWriteCount__(fout, "moveRec", aa->lmNumber, aa->lmSize, aa->lmCount);
    calWriteCount__(fout, "aspect", aa->laNumber, aa->laSize, aa->laCount);
    fprintf(fout, "\n");

    delAspectAnalysis__(aa);
    fclose(fout);
}

static void aspectCmp__(Aspect asp, void* ptr)
{
    printf("check:%s ", asp->express);
    Aspect oasp = getAspect__((Aspects)ptr, (char*)getMD5(asp->express));
    assert(oasp);
    MoveRec mr = asp->lastMoveRec, omr = oasp->lastMoveRec;
    while (mr) {
        assert(omr);
        int mrV = getMRValue__(mr->rowcols, mr->number, mr->weight), omrV = getMRValue__(omr->rowcols, omr->number, omr->weight);
        assert(mrV == omrV);
        assert(mr->number > 0);
        printf("0x%08x=0x%08x ", mrV, omrV);
        mr = mr->preMoveRec;
        omr = omr->preMoveRec;
    }
    printf("ok!\n");
}

static void startAspectCmp__(Aspect lasp, void* ptr)
{
    aspectLink__(lasp, aspectCmp__, NULL, NULL, ptr);
}

// 检查局面MD5数据文件与局面文本数据文件是否完全一致
static void checkAspectMD5__(char* libFileName, char* md5FileName)
{
    Aspects asps = getAspects_fs(libFileName),
            oasps = getAspects_fb(md5FileName);

    aspectsMap(asps, startAspectCmp__, oasps);
    printf("checkAspectMD5 OK!\n");

    delAspects(asps);
    delAspects(oasps);
}

void testAspects(Aspects asps)
{
    char log[] = "ana", libs[] = "libs", md5[] = "md5";
    analyzeAspects(log, asps);
    storeAspectLib(libs, asps); 
    delAspects(asps);

    asps = getAspects_fs(libs);
    analyzeAspects(log, asps);

    storeAspectMD5(md5, asps);
    analyzeAspects(log, asps);
    delAspects(asps);

    asps = getAspects_fb(md5);
    analyzeAspects(log, asps);
    delAspects(asps);

    //*
    checkAspectMD5__(libs, md5);
    //*/
}