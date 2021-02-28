#include "head/aspect.h"
#include "head/board.h"
#include "head/chessManual.h"
#include "head/move.h"
#include "head/operatefile.h"
#include "head/table.h"
#include "head/tools.h"
#include <math.h>

// 着法记录类型
typedef struct MoveRec* MoveRec;
typedef struct MoveReces* MoveReces;

struct MoveRec {
    int rowcols; // "rcrc"(主要用途：确定某局面下着法，根据count，weight分析着法优劣)
    int number; // 历史棋谱中某局面下该着法已发生的次数(0:重复标记，表示后续有相同着法);本局面的重复次数
    int weight; // 对应某局面的本着价值权重(通过局面评价函数计算)

    bool killing; // 将
    bool willKill; // 杀
    bool catch; // 捉

    MoveRec link; // 下一个链接
};

struct MoveReces {
    PieceColor moveColor; // 走棋方
    int mrCount; // 本局面下着法数
    bool fail; // 失败

    MoveRec rootMR; // 着法根链接
};

struct Aspects {
    int mrsCount; // 总局面数
    int mrCount; // 总着法数

    Table table; // 哈希表，存储局面及着法
};

typedef struct NumArray {
    int *num, size, count;
} * NumArray;

typedef struct AspectAnalysis {
    NumArray move, mr, mrs; // 棋谱着法、局面下着法、局面
} * AspectAnalysis;

static const char ASPLIB_MARK[] = "AspectLib";

static MoveRec newMoveRec__(int rowcols, int number, int weight)
{
    MoveRec mr = malloc(sizeof(*mr));
    mr->rowcols = rowcols;
    mr->number = number;
    mr->weight = weight;
    mr->killing = false;
    mr->willKill = false;
    mr->catch = false;

    mr->link = NULL;
    return mr;
}

static void delMoveRec__(MoveRec mr)
{
    while (mr) {
        MoveRec pmr = mr->link;
        free(mr);
        mr = pmr;
    }
}

static MoveReces newMoveReces__()
{
    MoveReces mrs = malloc(sizeof(*mrs));
    mrs->moveColor = RED;
    mrs->mrCount = 0;
    mrs->fail = false;

    mrs->rootMR = NULL;
    return mrs;
}

static void delMoveReces__(MoveReces mrs)
{
    delMoveRec__(mrs->rootMR);
    free(mrs);
}

Aspects newAspects(int size)
{
    Aspects asps = malloc(sizeof(*asps));
    asps->mrsCount = 0;
    asps->mrCount = 0;

    asps->table = newTable(size, (void (*)(void*))delMoveReces__);
    return asps;
}

void delAspects(Aspects asps)
{
    assert(asps);
    delTable(asps->table);
    free(asps);
}

int getAspectsLength(Aspects asps)
{
    return getTableLength(asps->table);
}

static void putMoveRec__(MoveReces mrs, int rowcols, int number, int weight)
{
    MoveRec mr = newMoveRec__(rowcols, number, weight);
    mr->link = mrs->rootMR;
    mrs->rootMR = mr;
    mrs->mrCount++;
}

static MoveReces putMoveReces__(Aspects asps, char* key)
{
    assert(asps);
    MoveReces mrs = getTable(asps->table, key);
    if (mrs == NULL) {
        mrs = newMoveReces__();
        putTable(&asps->table, key, mrs);
        asps->mrsCount++;
    }
    return mrs;
}

static void appendAspects__(Move move, Board board, Aspects asps)
{
    wchar_t FEN[SEATNUM];
    getFEN_board(FEN, board);
    char* fen = malloc(SEATNUM * sizeof(char));
    wcstombs(fen, FEN, SEATNUM);
    int rowcols = getRowCols_m(move);

    MoveReces mrs = putMoveReces__(asps, fen);
    //*
    MoveRec mr = mrs->rootMR;
    while (mr) {
        // 找到相同着法，调增number后退出
        if (mr->rowcols == rowcols) {
            mr->number++;
            // 还可增加一些其他操作

            return;
        }
        mr = mr->link;
    }

    // 未找到相同着法，插入新着法记录
    putMoveRec__(mrs, rowcols, 1, 0);
    //*/
    asps->mrCount++;
}

void appendAspectsFromCMfile(Aspects asps, const char* fileName)
{
    assert(asps);
    ChessManual cm = getChessManual_file(fileName);
    moveMap(cm, (void (*)(Move, Board, void*))appendAspects__, asps);
    delChessManual(cm);
}

static void appendAspects_fileInfo__(FileInfo fileInfo, Aspects asps)
{
    appendAspectsFromCMfile(asps, fileInfo->name);
    //printf("%d: %s\n", __LINE__, fileInfo->name);
}

void appendAspectsFromCMdir(Aspects asps, const char* dirName)
{
    operateDir(dirName, (void (*)(void*, void*))appendAspects_fileInfo__, asps, true);
}

Aspects getAspectsFromAspsfile(const char* fileName)
{

    FILE* fin = fopen(fileName, "r");
    char tag[FILENAME_MAX];
    fscanf(fin, "%s", tag);
    assert(strcmp(tag, ASPLIB_MARK) == 0); // 检验文件标志

    int mrCount = 0, rowcols = 0, number = 0, weight = 0;
    Aspects asps = newAspects(0);
    char* fen = malloc(SEATNUM * sizeof(char));
    while (fscanf(fin, "%s", fen) == 1) { // 遇到空行(只有字符'\n')则结束
        if (fscanf(fin, "%d", &mrCount) != 1)
            continue;

        MoveReces mrs = putMoveReces__(asps, fen);
        for (int i = 0; i < mrCount; ++i) {
            if (fscanf(fin, "%x%d%d", &rowcols, &number, &weight) != 3) {
                //printf("\n%d %s", __LINE__, __FILE__);
                break;
            }
            putMoveRec__(mrs, rowcols, number, weight);
            asps->mrCount++;
        }
        fen = malloc(SEATNUM * sizeof(char));
    }
    free(fen);

    fclose(fin);
    return asps;
}

static void writefMoveRecesShow__(char* key, MoveReces mrs, void* fout)
{
    fprintf(fout, "\nFEN:%s ", key);
    //fprintf(fout, "\nFEN:%s ", mrs->key);

    MoveRec mr = mrs->rootMR;
    while (mr) {
        fprintf(fout, "\t0x%04x %d %d ", mr->rowcols, mr->number, mr->weight);
        mr = mr->link;
    }
}

void writeAspectsShow(char* fileName, CAspects asps)
{
    FILE* fout = fopen(fileName, "w");
    mapTable(asps->table, (void (*)(char*, void*, void*))writefMoveRecesShow__, fout);

    int aspSize = getTableSize(asps->table),
        aspCount = getTableLength(asps->table);
    fprintf(fout, "\n【数组 大小:%d 局面数(使用):%d 着法数:%d 填充因子:%5.4f】\n",
        aspSize, aspCount, asps->mrCount, (double)aspCount / aspSize);

    fclose(fout);
}

static void storeMoveRecesFEN__(char* key, MoveReces mrs, void* fout)
{
    fprintf(fout, "\n%s %d ", key, mrs->mrCount);

    MoveRec mr = mrs->rootMR;
    while (mr) {
        fprintf(fout, "0x%04x %d %d ", mr->rowcols, mr->number, mr->weight);
        mr = mr->link;
    }
}

void storeAspectsFEN(char* fileName, CAspects asps)
{
    FILE* fout = fopen(fileName, "w");
    fprintf(fout, "%s\n", ASPLIB_MARK);
    mapTable(asps->table, (void (*)(char*, void*, void*))storeMoveRecesFEN__, fout);
    fprintf(fout, "\n");

    fclose(fout);
}

static AspectAnalysis newAspectAnalysis__(void)
{
    AspectAnalysis aa = malloc(sizeof(struct AspectAnalysis));
    aa->move = malloc(sizeof(struct NumArray));
    aa->mr = malloc(sizeof(struct NumArray));
    aa->mrs = malloc(sizeof(struct NumArray));
    aa->move->size = aa->mr->size = aa->mrs->size = 2 << 4;
    aa->move->count = aa->mr->count = aa->mrs->count = 0;
    aa->move->num = malloc(aa->move->size * sizeof(int));
    aa->mr->num = malloc(aa->mr->size * sizeof(int));
    aa->mrs->num = malloc(aa->mrs->size * sizeof(int));
    return aa;
}

static void delAspectAnalysis__(AspectAnalysis aa)
{
    free(aa->move->num);
    free(aa->mr->num);
    free(aa->mrs->num);
    free(aa->move);
    free(aa->mr);
    free(aa->mrs);
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

static void calMRNumber__(char* key, MoveReces mrs, AspectAnalysis aa)
{
    appendNumArray__(aa->mr, mrs->mrCount);

    MoveRec mr = mrs->rootMR;
    while (mr) {
        appendNumArray__(aa->move, mr->number);
        mr = mr->link;
    }
}

// 待完善
static void calMrsNumber__(char* key, MoveReces mrs, AspectAnalysis aa)
{
    static int count = 0;
    appendNumArray__(aa->mrs, count);
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

    /*
    MoveReces mrs;
    for (int i = 0; i < asps->size; ++i) {
        if ((mrs = asps->rootAsps[i])) {
            int count = 1;
            while ((mrs = mrs->link))
                count++;
            // 加入单链表的局面个数
            appendNumArray__(aa->mrs, count);
        }
    }
    //*/
    // 加入局面表每个桶的局面个数
    mapTable(asps->table, (void (*)(char*, void*, void*))calMrsNumber__, aa);

    // 加入单链表的着法记录个数、每个着法重复次数
    mapTable(asps->table, (void (*)(char*, void*, void*))calMRNumber__, aa);

    int aspSize = getTableSize(asps->table),
        aspCount = getTableLength(asps->table);
    fprintf(fout, "分析 aspects => 局面:%d 着法:%d 【数组 %d/%d=%.4f】\n",
        aspCount, asps->mrCount, aspCount, aspSize, (double)aspCount / aspSize);
    calWriteOut__(fout, "moveNum", aa->move);
    calWriteOut__(fout, "mrNum", aa->mr);
    calWriteOut__(fout, "aspNum", aa->mrs);
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

static bool aspect_equal__(MoveReces asp0, MoveReces asp1, bool isSame)
{
    if (asp0 == NULL && asp1 == NULL)
        return true;
    // 其中有一个为空指针
    if (!(asp0 && asp1))
        return false;

    /*
    if (!(asp0->mrCount == asp1->mrCount
            && (!isSame
                // 以下为isSame为真时进行比较
                || (asp0->key && asp1->key
                    && strcmp(asp0->key, asp1->key))))) {
        //printf("\n%d %s", __LINE__, __FILE__);
        return false;
    }
    //*/
    if (asp0->mrCount != asp1->mrCount) {
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
        mr0 = mr0->link;
        mr1 = mr1->link;
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

    if (!(getTableSize(asps0->table) == getTableSize(asps1->table)
            && getTableLength(asps0->table) == getTableLength(asps1->table)
            //if (!(asps0->size == asps1->size
            //&& asps0->aspCount == asps1->aspCount
            && asps0->mrCount && asps1->mrCount)) {
        //printf("\n%d %s", __LINE__, __FILE__);
        return false;
    }

    /*
    int aspCount = 0;
    bool isSame = asps0->aspFormat == asps1->aspFormat;
    // 格式不相同，且前一个为Hash格式则需更换
    if (!isSame && (asps0->aspFormat == Hash_MRValue)) {
        CAspects tmpAsps = asps0;
        asps0 = asps1;
        asps1 = tmpAsps;
    }

    for (int i = 0; i < asps0->size; ++i) {
        MoveReces asp0 = asps0->rootAsps[i], asp1 = NULL;
        if (asp0 == NULL)
            continue;

        do {
            if (isSame)
                asp1 = getMoveReces__(asps1, asp0->key, asp0->klen);
            else {
                unsigned char hash[HashSize];
                getHashFun(hash, (unsigned char*)asp0->key);

                //getHashFun(hash, (const unsigned char*)asp0->key);
                asp1 = getMoveReces__(asps1, (char*)hash, HashSize);
            }
            if (!(aspect_equal__(asp0, asp1, isSame))) {
                //printf("\n%d %s", __LINE__, __FILE__);
                return false;
            }
            aspCount++;
        } while ((asp0 = asp0->link));
    }
    if (aspCount != asps0->aspCount) {
        //printf("\n%d %s", __LINE__, __FILE__);
        return false;
    }
    //*/

    return true;
}
