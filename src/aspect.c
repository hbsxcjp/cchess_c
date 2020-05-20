#include "head/aspect.h"
#include "head/board.h"
#include "head/move.h"
#include "head/tools.h"

struct MoveRec {
    CMove move; // 着法指针(主要用途：正在对局时，判断是否有违反行棋规则的棋例出现)
    int rowcols; // "rcrc"(主要用途：确定某局面下着法，根据count，weight分析着法优劣)
    int number; // 历史棋谱中某局面下该着法已发生的次数(0:重复标记，表示后续有相同着法);本局面的重复次数
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
    SourceType st; // 数据源类型，决定Aspect的express字段解释（fen or hash）
};

struct AspectAnalysis {
    int *mNumber, mSize, mCount; // 着法发生次数
    int *lmNumber, lmSize, lmCount; // 同一局面下着法个数
    int *laNumber, laSize, laCount; // 同一哈希值下局面个数
};

static const wchar_t ASPLIB_MARK[] = L"AspectLib\n";

static int getRowCols__(unsigned int mrValue) { return mrValue >> 16; }
static int getNumber__(unsigned int mrValue) { return (mrValue >> 8) & 0xFF; }
static int getWeight__(unsigned int mrValue) { return mrValue & 0xFF; }
static unsigned int getMRValue__(MoveRec mr) { return ((unsigned int)mr->rowcols << 16) | (mr->number << 8) | mr->weight; }

static MoveRec newMoveRec_MP__(CMove move)
{
    MoveRec mr = malloc(sizeof(struct MoveRec));
    mr->move = move;
    mr->rowcols = getRowCols_m(mr->move);
    mr->number = 1;
    mr->weight = 0; //待完善
    mr->preMoveRec = NULL;
    return mr;
}

static MoveRec newMoveRec_MR__(unsigned int mrValue)
{
    MoveRec mr = malloc(sizeof(struct MoveRec));
    mr->move = NULL;
    mr->rowcols = getRowCols__(mrValue);
    mr->number = getNumber__(mrValue);
    mr->weight = getWeight__(mrValue);
    mr->preMoveRec = NULL;
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

static Aspect newAspect__(char* aspSource)
{
    assert(aspSource);
    Aspect asp = malloc(sizeof(struct Aspect));
    asp->express = aspSource;
    asp->lastMoveRec = NULL;
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

Aspects newAspects(SourceType st, int size)
{
    size = getPrime(size);
    Aspects asps = malloc(sizeof(struct Aspects));
    asps->size = size;
    asps->aspCount = asps->movCount = 0;
    asps->loadfactor = 0.85;
    asps->lastAspects = malloc(size * sizeof(Aspect*));
    for (int i = 0; i < size; ++i)
        asps->lastAspects[i] = NULL;
    asps->st = st;
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

static void aspectsMap__(CAspects asps, void applyAsp(Aspect, void*), void* aspArg, void applyMr(MoveRec, void*), void* mrArg)
{
    assert(asps);
    assert(applyAsp && aspArg);
    for (int i = 0; i < asps->size; ++i) {
        Aspect asp = asps->lastAspects[i], preAsp;
        if (!asp)
            continue;
        while (asp) {
            preAsp = asp->preAspect;
            applyAsp(asp, aspArg);

            // 处理applyMr
            if (applyMr) {
                MoveRec mr = asp->lastMoveRec, pmr;
                while (mr) {
                    pmr = mr->preMoveRec;
                    applyMr(mr, mrArg);
                    mr = pmr;
                }
            }
            asp = preAsp;
        }
    }
}

// 依据aspSource取得最后的局面记录指针
inline static Aspect* getLastAspect__(CAspects asps, char* aspSource)
{
    return &asps->lastAspects[BKDRHash_c(aspSource, asps->st == Hash_MRValue ? HashSize : strlen(aspSource)) % asps->size];
}

// 取得某个局面
static Aspect getAspect__(CAspects asps, char* aspSource)
{
    Aspect asp = *getLastAspect__(asps, aspSource);
    int (*cmpfun)(const char*, const char*) = asps->st == Hash_MRValue ? HashCMP : strcmp;
    while (asp && cmpfun(asp->express, aspSource) != 0)
        asp = asp->preAspect;
    return asp;
}

static void loadAspect__(Aspect asp, void* asps)
{
    Aspect* pasp = getLastAspect__(asps, asp->express);
    asp->preAspect = *pasp;
    *pasp = asp;
}

// 源局面(指针数组下内容)全面迁移至新局面
static void reloadLastAspects__(Aspects asps, int minSize)
{
    Aspects tmpAsps = newAspects(asps->st, minSize);
    aspectsMap__(asps, loadAspect__, tmpAsps, NULL, NULL);

    asps->size = tmpAsps->size;
    free(asps->lastAspects);
    asps->lastAspects = tmpAsps->lastAspects;
    free(tmpAsps);
}

static void putMoveRec_MP__(Aspect asp, CMove move)
{
    MoveRec pmr = asp->lastMoveRec, mr = newMoveRec_MP__(move);
    // 允许存在相同着法，需对number作出标记
    while (pmr) {
        if (mr->rowcols == pmr->rowcols) {
            mr->number += pmr->number;
            pmr->number = 0; // 标记重复，存储为FEN_MRValue, MD_MRValue格式时该mr被过滤
            break; // 再往前推，如有重复在此之前也已被标记
        }
        pmr = pmr->preMoveRec;
    }
    // 插入方式
    mr->preMoveRec = asp->lastMoveRec;
    asp->lastMoveRec = mr;
}

static void putMoveRec_MR__(Aspect asp, unsigned int mrValue)
{
    MoveRec pmr = asp->lastMoveRec, mr = newMoveRec_MR__(mrValue);
    // 追加方式
    if (pmr) {
        // 递进到最前着
        while (pmr->preMoveRec)
            pmr = pmr->preMoveRec;
        pmr->preMoveRec = mr;
    } else
        asp->lastMoveRec = mr;
}

static Aspect putAspect__(Aspects asps, char* aspSource)
{
    // 检查容量，如果超出装载因子则扩容
    if (asps->aspCount >= asps->size * asps->loadfactor && asps->size < INT_MAX)
        reloadLastAspects__(asps, asps->size + 1);

    Aspect asp = newAspect__(aspSource);
    loadAspect__(asp, asps);
    asps->aspCount++;
    return asp;
}

static char* getfen_board__(wchar_t* FEN)
{
    char* fen = malloc(wcslen(FEN) * 2 + 1);
    wcstombs(fen, FEN, wcslen(FEN) * 2);
    return fen;
}

void appendAspects_mb(Move move, void* asps, void* board)
{
    wchar_t FEN[SEATNUM + 1];
    getFEN_board(FEN, board);
    char* fen = getfen_board__(FEN);
    Aspect asp = getAspect__(asps, fen);
    if (asp)
        free(fen);
    else
        asp = putAspect__(asps, fen);
    putMoveRec_MP__(asp, move);
    ((Aspects)asps)->movCount++;
}

Aspects getAspects_fs(const char* fileName)
{
    Aspects asps = newAspects(FEN_MRValue, 0);
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
        Aspect asp = putAspect__(asps, getfen_board__(FEN));
        while (wcslen(sourceStr) > 10 && swscanf(sourceStr, L"%x", &mrValue) == 1) {
            putMoveRec_MR__(asp, mrValue);
            asps->movCount++;
            sourceStr += 11; //source存储长度为11个字符:0x+8位数字+1个空格
        }
    }
    fclose(fin);
    return asps;
}

Aspects getAspects_fb(const char* fileName)
{
    Aspects asps = newAspects(Hash_MRValue, 0);
    FILE* fin = fopen(fileName, "rb");
    char* hash = malloc(HashSize);
    int count = 0;
    while (fread(hash, HashSize, 1, fin) == 1) {
        if (fread(&count, sizeof(int), 1, fin) != 1)
            break;
        unsigned int mrValue[count];
        if (fread(mrValue, sizeof(unsigned int), count, fin) != count)
            break;
        Aspect asp = putAspect__(asps, hash);
        for (int i = 0; i < count; ++i) {
            putMoveRec_MR__(asp, mrValue[i]);
            asps->movCount++;
        }
        hash = malloc(HashSize);
    }
    free(hash);
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

static void printfMoveRecShow__(MoveRec mr, void* fout)
{
    wchar_t iccs[6];
    fwprintf(fout, L"\tmove@:%p iccs:%s\n", mr->move, getICCS(iccs, mr->move));
}

static void printfAspectShow__(Aspect asp, void* fout)
{
    fprintf(fout, "FEN:%s\n", asp->express);
}

void writeAspectShow(char* fileName, CAspects asps)
{
    assert(asps->st == FEN_MovePtr);
    FILE* fout = fopen(fileName, "w");
    aspectsMap__(asps, printfAspectShow__, fout, printfMoveRecShow__, fout);

    fprintf(fout, "\n【数组 大小:%d 局面数(使用):%d 着法数:%d 填充因子:%5.2f SourceType:%d】\n",
        asps->size, asps->aspCount, asps->movCount, (double)asps->aspCount / asps->size, asps->st);
    fclose(fout);
}

static void printfMoveRecFEN__(MoveRec mr, void* fout)
{
    if (mr->number)
        fprintf(fout, "0x%08x ", getMRValue__(mr));
}

static void printfAspectFEN__(Aspect asp, void* fout)
{
    fprintf(fout, "\n%s ", asp->express);
}

void storeAspectFEN(char* fileName, CAspects asps)
{
    FILE* fout = fopen(fileName, "w");
    assert(asps->st <= FEN_MRValue);
    fwprintf(fout, ASPLIB_MARK);
    aspectsMap__(asps, printfAspectFEN__, fout, printfMoveRecFEN__, fout);

    fclose(fout);
}

static void putAspectHash__(Aspect asp, void* masps)
{
    Aspect masp = putAspect__(masps, (char*)HashGetFun(asp->express));
    MoveRec mr = asp->lastMoveRec;
    do
        if (mr->number) {
            putMoveRec_MR__(masp, getMRValue__(mr));
            ((Aspects)masps)->movCount++;
        }
    while ((mr = mr->preMoveRec));
}

static void writeMoveRecHash__(MoveRec mr, void* fout)
{
    // 排除重复标记的着法
    if (mr->number) {
        unsigned int mrValue = getMRValue__(mr);
        fwrite(&mrValue, sizeof(unsigned int), 1, fout); // 4个字节
    }
}

static void writeAspectHash__(Aspect asp, void* fout)
{
    fwrite(asp->express, HashSize, 1, fout); // 16个字节
    int count = 1;
    MoveRec mr = asp->lastMoveRec;
    while ((mr = mr->preMoveRec) && mr->number)
        count++;
    fwrite(&count, sizeof(int), 1, fout); // 4个字节
}

void storeAspectHash(char* fileName, CAspects asps)
{
    FILE* fout = fopen(fileName, "wb");
    Aspects masps = newAspects(Hash_MRValue, asps->size);
    aspectsMap__(asps, putAspectHash__, masps, NULL, NULL); // 转换存储格式
    aspectsMap__(masps, writeAspectHash__, fout, writeMoveRecHash__, fout);
    delAspects(masps);
    fclose(fout);
}

static AspectAnalysis newAspectAnalysis__(void)
{
    AspectAnalysis aa = malloc(sizeof(struct AspectAnalysis));
    aa->mSize = aa->lmSize = aa->laSize = 2 << 9;
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

static void calMoveNumber__(MoveRec mr, void* ana)
{
    AspectAnalysis aa = (AspectAnalysis)ana;
    assert(mr->number);
    checkApendArray__(&aa->mNumber, &aa->mSize, &aa->mCount, mr->number);
}

static void calMoveRecCount__(Aspect asp, void* ana)
{
    AspectAnalysis aa = (AspectAnalysis)ana;
    int mcount = 1;
    MoveRec mr = asp->lastMoveRec;
    while ((mr = mr->preMoveRec))
        mcount++;
    checkApendArray__(&aa->lmNumber, &aa->lmSize, &aa->lmCount, mcount);
}

static void calWriteOut__(FILE* fout, const char* entry, int* number, int size, int count)
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
    fprintf(fout, "分析%8s => total:%d max:%d ave:%.4f 方差:%.4f 标准差:%.4f 【数组 %d/%d=%.4f】\n",
        entry, total, imax, ave, varinace, stdDiff, count, size, scale);
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
    aspectsMap__(asps, calMoveRecCount__, aa, calMoveNumber__, aa);

    fprintf(fout, "分析 aspects => 局面:%d 着法:%d  SourceType:%d 【数组 %d/%d=%.4f】\n",
        asps->aspCount, asps->movCount, asps->st, asps->aspCount, asps->size, (double)asps->aspCount / asps->size);
    calWriteOut__(fout, "moveNum", aa->mNumber, aa->mSize, aa->mCount);
    calWriteOut__(fout, "mrNum", aa->lmNumber, aa->lmSize, aa->lmCount);
    calWriteOut__(fout, "aspNum", aa->laNumber, aa->laSize, aa->laCount);
    fprintf(fout, "\n");

    delAspectAnalysis__(aa);
    fclose(fout);
}

static void aspectCmp__(Aspect asp, void* oasps)
{
    //printf("check:%s ", asp->express);
    Aspect oasp = getAspect__(oasps, (char*)HashGetFun(asp->express));
    assert(oasp);
    MoveRec mr = asp->lastMoveRec, omr = oasp->lastMoveRec;
    while (mr) {
        assert(omr);
        unsigned int mrV = getMRValue__(mr), omrV = getMRValue__(omr);
        if (mrV != omrV) {
            printf("0x%08x - 0x%08x ", (int)mrV, (int)omrV);
            fflush(stdout);
        }
        assert(mr->number);
        assert(mrV == omrV);

        mr = mr->preMoveRec;
        omr = omr->preMoveRec;
    }
    //printf("ok!\n");
}

// 检查局面Hash数据文件与局面文本数据文件是否完全一致
static void checkAspectHash__(char* libFileName, char* md5FileName)
{
    Aspects asps = getAspects_fs(libFileName),
            oasps = getAspects_fb(md5FileName);
    aspectsMap__(asps, aspectCmp__, oasps, NULL, NULL);
    delAspects(asps);
    delAspects(oasps);
}

void testAspects(Aspects asps)
{
    char log[] = "log", libs[] = "libs", hash[] = "hash";
    //analyzeAspects(log, asps);
    storeAspectFEN(libs, asps);
    printf("storeAspectFEN OK!\n");
    fflush(stdout);
    delAspects(asps);

    asps = getAspects_fs(libs);
    analyzeAspects(log, asps);
    //*
    printf("getAspects_fs OK!\n");
    fflush(stdout);

    storeAspectHash(hash, asps);
    printf("storeAspectHash OK!\n");
    fflush(stdout);
    delAspects(asps);

    asps = getAspects_fb(hash);
    analyzeAspects(log, asps);
    printf("getAspects_fb OK!\n");
    fflush(stdout);

    checkAspectHash__(libs, hash);
    printf("checkAspectHash__ OK!\n");
    fflush(stdout);
    //*/
    delAspects(asps);
}