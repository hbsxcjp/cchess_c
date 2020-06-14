#include "head/aspect.h"
#include "head/board.h"
#include "head/chessManual.h"
#include "head/move.h"
#include "head/tools.h"

struct MoveRec {
    int rowcols; // "rcrc"(主要用途：确定某局面下着法，根据count，weight分析着法优劣)
    int number; // 历史棋谱中某局面下该着法已发生的次数(0:重复标记，表示后续有相同着法);本局面的重复次数
    int weight; // 对应某局面的本着价值权重(通过局面评价函数计算)
    MoveRec forward;
};

struct Aspect {
    char* key; // 局面表示的指针
    int klen;
    int mrCount;
    MoveRec rootMR;
    Aspect forward;
};

struct Aspects {
    int size;
    double loadfactor;
    int aspCount, movCount;
    Aspect* rootAsps;
    SourceType st; // 数据源类型，决定Aspect的express字段解释（fen or hash）
};

struct AspectAnalysis {
    int *mNumber, mSize, mCount; // 着法发生次数
    int *lmNumber, lmSize, lmCount; // 同一局面下着法个数
    int *laNumber, laSize, laCount; // 同一哈希值下局面个数
};

static const char ASPLIB_MARK[] = "AspectLib";

static MoveRec newMoveRec__(int rowcols, int number, int weight)
{
    MoveRec mr = malloc(sizeof(struct MoveRec));
    mr->rowcols = rowcols;
    mr->number = number;
    mr->weight = weight;
    mr->forward = NULL;
    return mr;
}

static void delMoveRec__(MoveRec mr)
{
    while (mr != NULL) {
        MoveRec pmr = mr->forward;
        free(mr);
        mr = pmr;
    }
}

static Aspect newAspect__(const char* key, int klen)
{
    Aspect asp = malloc(sizeof(struct Aspect));
    asp->key = malloc(klen);
    memcpy(asp->key, key, klen);
    asp->klen = klen;
    asp->mrCount = 0;
    asp->rootMR = NULL;
    asp->forward = NULL;
    return asp;
}

static void delAspect__(Aspect asp)
{
    while (asp != NULL) {
        Aspect pasp = asp->forward;
        free(asp->key);
        delMoveRec__(asp->rootMR);
        free(asp);
        asp = pasp;
    }
}

Aspects newAspects(SourceType st, int size)
{
    size = getPrime(size);
    Aspects asps = malloc(sizeof(struct Aspects));
    asps->size = size;
    asps->aspCount = asps->movCount = 0;
    asps->loadfactor = 0.85;
    asps->rootAsps = malloc(size * sizeof(Aspect*));
    for (int i = 0; i < size; ++i)
        asps->rootAsps[i] = NULL;
    asps->st = st;
    return asps;
}

void delAspects(Aspects asps)
{
    assert(asps);
    for (int i = 0; i < asps->size; ++i)
        delAspect__(asps->rootAsps[i]);
    free(asps->rootAsps);
    free(asps);
}

static void moveRecMap__(MoveRec mr, void applyMr(MoveRec, void*), void* mrArg)
{
    do
        applyMr(mr, mrArg);
    while ((mr = mr->forward));
}

static void aspectsMap__(CAspects asps, void applyAsp(Aspect, void*), void* aspArg, void applyMr(MoveRec, void*), void* mrArg)
{
    assert(asps);
    assert(applyAsp && aspArg);
    for (int i = 0; i < asps->size; ++i) {
        Aspect asp = asps->rootAsps[i], preAsp;
        if (!asp)
            continue;
        while (asp) {
            preAsp = asp->forward;
            applyAsp(asp, aspArg);

            assert(asp->rootMR);
            // 处理applyMr
            if (applyMr) {
                moveRecMap__(asp->rootMR, applyMr, mrArg);
            }
            asp = preAsp;
        }
    }
}

// hash值相等的最后局面记录指针
inline static Aspect* getLastAspect__(CAspects asps, const char* key, int klen)
{
    return &asps->rootAsps[BKDRHash_c(key, klen) % asps->size];
}

// 取得key完全相等的局面记录指针
static Aspect getAspect__(CAspects asps, const char* key, int klen)
{
    Aspect asp = *getLastAspect__(asps, key, klen);
    while (asp && !charIsSame(asp->key, key, klen))
        asp = asp->forward;
    return asp;
}

static void putMoveRec_MP__(Aspect asp, CMove move)
{
    MoveRec pmr = asp->rootMR, mr = newMoveRec__(getRowCols_m(move), 1, 0);
    // 允许存在相同着法，需对number作出标记
    while (pmr) {
        if (mr->rowcols == pmr->rowcols) {
            mr->number += pmr->number;
            pmr->number = 0; // 标记重复，存储为FEN_MRValue, Hash_MRValue格式时该mr被过滤
            asp->mrCount--; // 与函数尾部的递增抵消，即不增不减
            break; // 不需再往前推，因为如有重复在此之前也已标记重复
        }
        pmr = pmr->forward;
    }
    // 插入方式
    mr->forward = asp->rootMR;
    asp->rootMR = mr;
    asp->mrCount++;
}

static void putMoveRec_MR__(Aspect asp, int rowcols, int number, int weight)
{
    MoveRec pmr = asp->rootMR, mr = newMoveRec__(rowcols, number, weight);
    // 追加方式
    if (pmr) {
        // 递进到最前着
        while (pmr->forward)
            pmr = pmr->forward;
        pmr->forward = mr;
    } else
        asp->rootMR = mr;
    asp->mrCount++;
}

static void loadAspect__(Aspect asp, void* asps)
{
    Aspect* pasp = getLastAspect__(asps, asp->key, asp->klen);
    asp->forward = *pasp;
    *pasp = asp;
}

// 源局面(指针数组下内容)全面迁移至新局面
static void reloadLastAspects__(Aspects asps, int minSize)
{
    Aspects tmpAsps = newAspects(asps->st, minSize);
    aspectsMap__(asps, loadAspect__, tmpAsps, NULL, NULL);

    asps->size = tmpAsps->size;
    free(asps->rootAsps);
    asps->rootAsps = tmpAsps->rootAsps; // 替换内容
    free(tmpAsps); // tmpAsps->rootAsps 仍保留
}

static Aspect putAspect__(Aspects asps, const char* key, int klen)
{
    // 检查容量，如果超出装载因子则扩容
    if (asps->aspCount >= asps->size * asps->loadfactor && asps->size < INT32_MAX)
        reloadLastAspects__(asps, asps->size + 1);

    Aspect asp = newAspect__(key, klen);
    loadAspect__(asp, asps);

    asps->aspCount++;
    return asp;
}

// 使用void*参数，目的是使其可以作为moveMap调用的函数参数
static void appendAspects_mb__(Move move, Board board, void* asps)
{
    wchar_t FEN[SEATNUM];
    getFEN_board(FEN, board);
    char fen[SEATNUM];
    wcstombs(fen, FEN, SEATNUM);
    Aspect asp = getAspect__(asps, fen, strlen(fen) + 1);
    if (!asp)
        asp = putAspect__(asps, fen, strlen(fen) + 1);

    putMoveRec_MP__(asp, move);
    ((Aspects)asps)->movCount++;
}

Aspects getAspects_fs(const char* fileName)
{
    Aspects asps = newAspects(FEN_MRValue, 0);
    FILE* fin = fopen(fileName, "r");
    char tag[FILENAME_MAX];
    fscanf(fin, "%s", tag);
    assert(strcmp(tag, ASPLIB_MARK) == 0); // 检验文件标志
    int mrCount = 0, rowcols = 0, number = 0, weight = 0;
    char fen[SEATNUM];
    while (fscanf(fin, "%s", fen) == 1) { // 遇到空行(只有字符'\n')则结束
        if (fscanf(fin, "%d", &mrCount) != 1)
            continue;
        Aspect asp = putAspect__(asps, fen, strlen(fen) + 1);

        for (int i = 0; i < mrCount; ++i) {
            if (fscanf(fin, "%x%d%d", &rowcols, &number, &weight) != 3)
                break;
            putMoveRec_MR__(asp, rowcols, number, weight);
            asps->movCount++;
        }
    }
    fclose(fin);
    return asps;
}

Aspects getAspects_fb(const char* fileName)
{
    Aspects asps = newAspects(Hash_MRValue, 0);
    FILE* fin = fopen(fileName, "rb");
    char hash[HashSize];
    int mrCount = 0;
    while (true) {
        if (fread(hash, HashSize, 1, fin) != 1)
            break;
        if (fread(&mrCount, sizeof(int), 1, fin) != 1)
            break;
        Aspect asp = putAspect__(asps, hash, HashSize);

        int mrValue[3];
        for (int i = 0; i < mrCount; ++i) {
            if (fread(&mrValue, sizeof(int), 3, fin) != 3)
                break;
            putMoveRec_MR__(asp, mrValue[0], mrValue[1], mrValue[2]);
            asps->movCount++;
        }
    }
    fclose(fin);
    return asps;
}

void appendAspects_file(Aspects asps, const char* fileName)
{
    ChessManual cm = newChessManual(fileName);
    moveMap(cm, appendAspects_mb__, asps);
    delChessManual(cm);
}

// 使用void*参数，目的是使其可以作为operateDir调用的函数参数
static void appendAspects_fileInfo__(FileInfo fileInfo, void* asps)
{
    appendAspects_file(asps, fileInfo->name);
    //printf("%d: %s\n", __LINE__, fileInfo->name);
}

void appendAspects_dir(Aspects asps, const char* dirName)
{
    operateDir(dirName, appendAspects_fileInfo__, asps, true);
}

int getAspects_length(Aspects asps) { return asps->aspCount; }

/*
int getLoopBoutCount(CAspects asps, const wchar_t* FEN)
{
    int boutCount = 0;
    MoveRec lmr = getMoveRec__(asps, FEN, FEN_MovePtr), mr = lmr;
    if (lmr && lmr->move)
        while ((mr = mr->forward))
            if (isSameMove(lmr->move, mr->move) && isConnected(lmr->move, mr->move)) {
                boutCount = (getNextNo(lmr->move) - getNextNo(lmr->move)) / 2;
                break;
            }
    return boutCount;
}
//*/

static void printfMoveRecShow__(MoveRec mr, void* fout)
{
    fprintf(fout, "\t0x%04x %d %d ", mr->rowcols, mr->number, mr->weight);
}

static void printfAspectShow__(Aspect asp, void* fout)
{
    fprintf(fout, "\nFEN:%s ", asp->key);
}

void writeAspectShow(char* fileName, CAspects asps)
{
    assert(asps->st == FEN_MovePtr);
    FILE* fout = fopen(fileName, "w");
    aspectsMap__(asps, printfAspectShow__, fout, printfMoveRecShow__, fout);
    //printf("%d: %s\n", __LINE__, fileName);

    fprintf(fout, "\n【数组 大小:%d 局面数(使用):%d 着法数:%d 填充因子:%5.2f SourceType:%d】\n",
        asps->size, asps->aspCount, asps->movCount, (double)asps->aspCount / asps->size, asps->st);
    fclose(fout);
}

static void printfMoveRecFEN__(MoveRec mr, void* fout)
{
    if (mr->number)
        fprintf(fout, "0x%04x %d %d ", mr->rowcols, mr->number, mr->weight);
}

static void printfAspectFEN__(Aspect asp, void* fout)
{
    fprintf(fout, "\n%s %d ", asp->key, asp->mrCount);
}

void storeAspectFEN(char* fileName, CAspects asps)
{
    assert(asps->st <= FEN_MRValue);
    FILE* fout = fopen(fileName, "w");
    fprintf(fout, "%s\n", ASPLIB_MARK);
    aspectsMap__(asps, printfAspectFEN__, fout, printfMoveRecFEN__, fout);

    fclose(fout);
}

static void writeMoveRecHash__(MoveRec mr, void* fout)
{
    // 排除重复标记的着法
    if (mr->number) {
        fwrite(&mr->rowcols, sizeof(int), 1, fout);
        fwrite(&mr->number, sizeof(int), 1, fout);
        fwrite(&mr->weight, sizeof(int), 1, fout);
    }
}

static void writeAspectHash__(Aspect asp, void* fout)
{
    fwrite(asp->key, asp->klen, 1, fout);
    fwrite(&asp->mrCount, sizeof(int), 1, fout);
}

static void copyMoveRec__(MoveRec mr, void* asp)
{
    putMoveRec_MR__(asp, mr->rowcols, mr->number, mr->weight);
}

// 非hash类型的asps复制存入hash类型的asps
static void copyAspectHash__(Aspect asp, void* hasps)
{
    unsigned char hash[HashSize];
    getHashFun(hash, asp->key);
    Aspect hasp = putAspect__(hasps, (char*)hash, HashSize);

    moveRecMap__(asp->rootMR, copyMoveRec__, hasp); // 参数hasp不能在aspectsMap__函数调用时提供，因此只能在这里调用moveRecMap__
}

void storeAspectHash(char* fileName, CAspects asps)
{
    FILE* fout = fopen(fileName, "wb");
    Aspects hasps = newAspects(Hash_MRValue, asps->size);
    aspectsMap__(asps, copyAspectHash__, hasps, NULL, NULL); // 转换存储格式
    hasps->movCount = asps->movCount;

    aspectsMap__(hasps, writeAspectHash__, fout, writeMoveRecHash__, fout);
    delAspects(hasps);
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
    /*
    if (!mr->number) {
        printf("0x%04x %u %u ", mr->rowcols, mr->number, mr->weight);
    }
    //*/
    //assert(mr->number);
    checkApendArray__(&aa->mNumber, &aa->mSize, &aa->mCount, mr->number);
}

static void calMoveRecCount__(Aspect asp, void* ana)
{
    AspectAnalysis aa = (AspectAnalysis)ana;
    checkApendArray__(&aa->lmNumber, &aa->lmSize, &aa->lmCount, asp->mrCount);
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
        Aspect asp = asps->rootAsps[i];
        if (asp == NULL)
            continue;
        int count = 1;
        while ((asp = asp->forward))
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
    //printf("check:%s ", asp->key);
    unsigned char hash[HashSize];
    getHashFun(hash, asp->key);
    Aspect oasp = getAspect__(oasps, (char*)hash, HashSize);

    assert(oasp);
    MoveRec mr = asp->rootMR, omr = oasp->rootMR;
    while (mr) {
        assert(omr);
        /*
        if (!(mr->rowcols == omr->rowcols && mr->number == omr->number && mr->weight == omr->weight)) {
            printf("0x%04hx %d %d - 0x%04hx %d %d \n", mr->rowcols, mr->number, mr->weight, omr->rowcols, omr->number, omr->weight);
        }
        //*/
        assert(mr->number);
        assert(mr->rowcols == omr->rowcols && mr->number == omr->number && mr->weight == omr->weight);

        mr = mr->forward;
        omr = omr->forward;
    }
    //printf("ok!\n");
}

// 检查局面Hash数据文件与局面文本数据文件是否完全一致
void checkAspectHash(char* libFileName, char* md5FileName)
{
    Aspects asps = getAspects_fs(libFileName),
            oasps = getAspects_fb(md5FileName);
    aspectsMap__(asps, aspectCmp__, oasps, NULL, NULL);
    delAspects(asps);
    delAspects(oasps);
}

void testAspects(CAspects asps)
{
    assert(asps);
    char *show = "show", *log = "log", *libs = "libs", *hash = "hash";
    writeAspectShow(show, asps);
    //printf("\nwriteAspectShow OK!\n");

    analyzeAspects(log, asps);
    storeAspectFEN(libs, asps);
    //printf("storeAspectFEN OK!\n");

    Aspects aspsl = getAspects_fs(libs);
    //printf("getAspects_fs OK!\n");

    analyzeAspects(log, aspsl);
    storeAspectHash(hash, aspsl);
    //printf("storeAspectHash OK!\n");

    delAspects(aspsl);

    Aspects aspsh = getAspects_fb(hash);
    //printf("getAspects_fb OK!\n");

    analyzeAspects(log, aspsh);
    checkAspectHash(libs, hash);
    //printf("checkAspectHash OK!\n");

    delAspects(aspsh);
}