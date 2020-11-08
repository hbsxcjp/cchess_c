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
    int klen; // 字符长度
    int mrCount; // 本局面下着法数
    MoveRec rootMR;
    Aspect forward;
};

struct Aspects {
    int size;
    double loadfactor;
    int aspCount, mrCount;
    Aspect* rootAsps;
    AspFormat aspFormat; // 数据源类型，决定Aspect的express字段解释（fen or hash）
};

typedef struct NumArray {
    int *num, size, count;
} * NumArray;

struct AspectAnalysis {
    NumArray move, mr, asp; // 棋谱着法、局面下着法、局面
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

Aspects newAspects(AspFormat aspFormat, int size)
{
    size = getPrime(size);
    Aspects asps = malloc(sizeof(struct Aspects));
    asps->size = size;
    asps->aspCount = asps->mrCount = 0;
    asps->loadfactor = 0.85;
    asps->rootAsps = malloc(size * sizeof(Aspect*));
    for (int i = 0; i < size; ++i)
        asps->rootAsps[i] = NULL;
    asps->aspFormat = aspFormat;
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
    do {
        applyMr(mr, mrArg);
    } while ((mr = mr->forward));
}

static void aspectsMap__(CAspects asps, void applyAsp(Aspect, void*), void* aspArg, void applyMr(MoveRec, void*), void* mrArg)
{
    assert(asps);
    assert(applyAsp && aspArg);
    Aspect asp, preAsp;
    for (int i = 0; i < asps->size; ++i) {
        if ((asp = asps->rootAsps[i]) == NULL)
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
    while (asp && !chars_equal(asp->key, key, klen))
        asp = asp->forward;
    return asp;
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
    Aspects tmpAsps = newAspects(asps->aspFormat, minSize);
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
    if (asp == NULL)
        asp = putAspect__(asps, fen, strlen(fen) + 1);

    int rowcols = getRowCols_m(move);
    MoveRec mr = asp->rootMR;
    while (mr) {
        // 未找到相同着法，调增number后退出
        if (mr->rowcols == rowcols) {
            mr->number++;
            return;
        }
        mr = mr->forward;
    }
    // 未找到相同着法，插入新着法记录
    mr = newMoveRec__(rowcols, 1, 0);
    mr->forward = asp->rootMR;
    asp->rootMR = mr;
    asp->mrCount++;

    ((Aspects)asps)->mrCount++;
}

void appendAspects_file(Aspects asps, const char* fileName)
{
    ChessManual cm = getChessManual_file(fileName);
    moveMap(cm, appendAspects_mb__, asps);
    delChessManual(cm);
}

static void appendAspects_fileInfo__(FileInfo fileInfo, Aspects asps)
{
    appendAspects_file(asps, fileInfo->name);
    //printf("%d: %s\n", __LINE__, fileInfo->name);
}

void appendAspects_dir(Aspects asps, const char* dirName)
{
    operateDir(dirName, (void (*)(void*, void*))appendAspects_fileInfo__, asps, true);
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
            if (fscanf(fin, "%x%d%d", &rowcols, &number, &weight) != 3) {
                //printf("\n%d %s", __LINE__, __FILE__);
                break;
            }
            putMoveRec_MR__(asp, rowcols, number, weight);
            asps->mrCount++;
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
    int mrCount = 0, mrValue[3];
    while ((fread(hash, HashSize, 1, fin) == 1)
        && (fread(&mrCount, sizeof(int), 1, fin) == 1)) {
        Aspect asp = putAspect__(asps, hash, HashSize);
        for (int i = 0; i < mrCount; ++i) {
            if (fread(&mrValue[0], sizeof(int), 3, fin) != 3) {
                //printf("\n%d %s", __LINE__, __FILE__);
                break;
            }
            putMoveRec_MR__(asp, mrValue[0], mrValue[1], mrValue[2]);
            asps->mrCount++;
        }
    }
    fclose(fin);
    return asps;
}

int getAspects_length(Aspects asps) { return asps->aspCount; }

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
    assert(asps->aspFormat == FEN_MRValue);
    FILE* fout = fopen(fileName, "w");
    aspectsMap__(asps, printfAspectShow__, fout, printfMoveRecShow__, fout);

    fprintf(fout, "\n【数组 大小:%d 局面数(使用):%d 着法数:%d 填充因子:%5.4f AspFormat:%d】\n",
        asps->size, asps->aspCount, asps->mrCount, (double)asps->aspCount / asps->size, asps->aspFormat);
    fclose(fout);
}

static void printfMoveRecFEN__(MoveRec mr, void* fout)
{
    fprintf(fout, "0x%04x %d %d ", mr->rowcols, mr->number, mr->weight);
}

static void printfAspectFEN__(Aspect asp, void* fout)
{
    fprintf(fout, "\n%s %d ", asp->key, asp->mrCount);
}

void storeAspectFEN(char* fileName, CAspects asps)
{
    assert(asps->aspFormat <= FEN_MRValue);
    FILE* fout = fopen(fileName, "w");
    fprintf(fout, "%s\n", ASPLIB_MARK);
    aspectsMap__(asps, printfAspectFEN__, fout, printfMoveRecFEN__, fout);
    fprintf(fout, "\n");

    fclose(fout);
}

static void writeMoveRecHash__(MoveRec mr, void* fout)
{
    fwrite(&mr->rowcols, sizeof(int), 1, fout);
    fwrite(&mr->number, sizeof(int), 1, fout);
    fwrite(&mr->weight, sizeof(int), 1, fout);
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
    getHashFun(hash, (unsigned char*)asp->key);
    Aspect hasp = putAspect__(hasps, (char*)hash, HashSize);

    // 参数hasp不能在aspectsMap__函数调用时提供，因此只能在这里调用moveRecMap__
    assert(asp->rootMR);
    moveRecMap__(asp->rootMR, copyMoveRec__, hasp);
}

void storeAspectHash(char* fileName, CAspects asps)
{
    assert(asps->aspFormat <= FEN_MRValue);

    FILE* fout = fopen(fileName, "wb");
    Aspects hasps = newAspects(Hash_MRValue, asps->size);
    aspectsMap__(asps, copyAspectHash__, hasps, NULL, NULL); // 转换存储格式
    hasps->mrCount = asps->mrCount;

    aspectsMap__(hasps, writeAspectHash__, fout, writeMoveRecHash__, fout);
    delAspects(hasps);
    fclose(fout);
}

static AspectAnalysis newAspectAnalysis__(void)
{
    AspectAnalysis aa = malloc(sizeof(struct AspectAnalysis));
    aa->move = malloc(sizeof(struct NumArray));
    aa->mr = malloc(sizeof(struct NumArray));
    aa->asp = malloc(sizeof(struct NumArray));
    aa->move->size = aa->mr->size = aa->asp->size = 2 << 9;
    aa->move->count = aa->mr->count = aa->asp->count = 0;
    aa->move->num = malloc(aa->move->size * sizeof(int));
    aa->mr->num = malloc(aa->mr->size * sizeof(int));
    aa->asp->num = malloc(aa->asp->size * sizeof(int));
    return aa;
}

static void delAspectAnalysis__(AspectAnalysis aa)
{
    free(aa->move->num);
    free(aa->mr->num);
    free(aa->asp->num);
    free(aa->move);
    free(aa->mr);
    free(aa->asp);
    free(aa);
}

static void appendNumArray__(NumArray na, int value)
{
    if (na->count >= na->size) {
        na->size *= 2;
        na->num = realloc(na->num, na->size * sizeof(int));
    }
    na->num[na->count++] = value;
}

static void calMoveNumber__(MoveRec mr, AspectAnalysis aa)
{
    appendNumArray__(aa->move, mr->number);
}

static void calMRNumber__(Aspect asp, AspectAnalysis aa)
{
    appendNumArray__(aa->mr, asp->mrCount);
}

static void calWriteOut__(FILE* fout, const char* entry, NumArray na)
{
    if (na->count <= 0 || na->size <= 0)
        return;
    int imax = 0, total = 0;
    for (int i = 0; i < na->count; ++i) {
        imax = fmax(imax, (na->num)[i]);
        total += (na->num)[i];
    }
    double ave = (double)total / na->count,
           scale = 1.0 * na->count / na->size,
           varinace = 0, difference = 0;
    // 计算方差
    for (int i = 0; i < na->count; ++i) {
        difference = (na->num)[i] - ave;
        varinace += difference * difference;
    }
    varinace /= na->count - 1;

    // 计算均方差
    double stdDiff = sqrt(varinace);
    fprintf(fout, "分析%8s => total:%d max:%d ave:%.4f 方差:%.4f 标准差:%.4f 【数组 %d/%d=%.4f】\n",
        entry, total, imax, ave, varinace, stdDiff, na->count, na->size, scale);
}

void analyzeAspects(char* fileName, CAspects asps)
{
    assert(asps);
    FILE* fout = fopen(fileName, "a");
    AspectAnalysis aa = newAspectAnalysis__();

    Aspect asp;
    for (int i = 0; i < asps->size; ++i) {
        if ((asp = asps->rootAsps[i])) {
            int count = 1;
            while ((asp = asp->forward))
                count++;
            // 加入单链表的局面个数
            appendNumArray__(aa->asp, count);
        }
    }
    // 加入单链表的着法记录个数、每个着法重复次数
    aspectsMap__(asps, (void (*)(Aspect, void*))calMRNumber__, aa, (void (*)(MoveRec, void*))calMoveNumber__, aa);

    fprintf(fout, "分析 aspects => 局面:%d 着法:%d  AspFormat:%d 【数组 %d/%d=%.4f】\n",
        asps->aspCount, asps->mrCount, asps->aspFormat, asps->aspCount, asps->size, (double)asps->aspCount / asps->size);
    calWriteOut__(fout, "moveNum", aa->move);
    calWriteOut__(fout, "mrNum", aa->mr);
    calWriteOut__(fout, "aspNum", aa->asp);
    fprintf(fout, "\n");

    delAspectAnalysis__(aa);
    fclose(fout);
}

static bool moveRec_equal__(MoveRec mr0, MoveRec mr1)
{
    return ((mr0 == NULL && mr1 == NULL)
        || (mr0 && mr1
            && mr0->rowcols == mr1->rowcols
            && mr0->number == mr1->number
            && mr0->weight == mr1->weight));
}

static bool aspect_equal__(Aspect asp0, Aspect asp1, bool isSame)
{
    if (asp0 == NULL && asp1 == NULL)
        return true;
    // 其中有一个为空指针
    if (!(asp0 && asp1))
        return false;

    if (!(asp0->mrCount == asp1->mrCount
            && (!isSame
                // 以下为isSame为真时进行比较
                || (asp0->klen == asp1->klen
                    && asp0->key && asp1->key
                    && chars_equal(asp0->key, asp1->key, asp0->klen))))) {
        //printf("\n%d %s", __LINE__, __FILE__);
        return false;
    }

    MoveRec mr0 = asp0->rootMR, mr1 = asp1->rootMR;
    assert(mr0 && mr1);
    while (mr0 && mr1) {
        if (!(moveRec_equal__(mr0, mr1))) {
            //printf("\n%d %s", __LINE__, __FILE__);
            return false;
        }
        mr0 = mr0->forward;
        mr1 = mr1->forward;
    }
    // mr0、mr1未同时抵达终点（空指针）
    if (mr0 != NULL || mr1 != NULL) {
        //printf("\n%d %s", __LINE__, __FILE__);
        return false;
    }

    return true;
}

bool aspects_equal(CAspects asps0, CAspects asps1)
{
    if (asps0 == NULL && asps1 == NULL)
        return true;
    // 其中有一个为空指针
    if (!(asps0 && asps1))
        return false;

    if (!(asps0->size == asps1->size
            && asps0->aspCount == asps1->aspCount
            && asps0->mrCount && asps1->mrCount)) {
        //printf("\n%d %s", __LINE__, __FILE__);
        return false;
    }

    int aspCount = 0;
    bool isSame = asps0->aspFormat == asps1->aspFormat;
    // 格式不相同，且前一个为Hash格式则需更换
    if (!isSame && (asps0->aspFormat == Hash_MRValue)) {
        CAspects tmpAsps = asps0;
        asps0 = asps1;
        asps1 = tmpAsps;
    }
    for (int i = 0; i < asps0->size; ++i) {
        Aspect asp0 = asps0->rootAsps[i], asp1 = NULL;
        if (asp0 == NULL)
            continue;

        do {
            if (isSame)
                asp1 = getAspect__(asps1, asp0->key, asp0->klen);
            else {
                unsigned char hash[HashSize];
                getHashFun(hash, (const unsigned char*)asp0->key);
                asp1 = getAspect__(asps1, (char*)hash, HashSize);
            }
            if (!(aspect_equal__(asp0, asp1, isSame))) {
                //printf("\n%d %s", __LINE__, __FILE__);
                return false;
            }
            aspCount++;
        } while ((asp0 = asp0->forward));
    }
    if (aspCount != asps0->aspCount) {
        //printf("\n%d %s", __LINE__, __FILE__);
        return false;
    }

    return true;
}
