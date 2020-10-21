#define PCRE_STATIC
#include "head/chessManual.h"
#include "head/aspect.h"
#include "head/board.h"
#include "head/cJSON.h"
#include "head/move.h"
#include "head/piece.h"
//#include <regex.h>
//#include <sys/types.h>

#define INFOSIZE 32

struct ChessManual {
    char* fileName;
    Board board;
    Move rootMove, curMove;
    wchar_t* info[INFOSIZE][2];
    int infoCount, movCount_, remCount_, maxRemLen_, maxRow_, maxCol_;
};

struct ChessManualRec {
    ChessManual cm;
    ChessManualRec ncmr;
};

typedef struct OperateDirData {
    int fcount, dcount, movCount, remCount, remLenMax;
    const char *fromDir, *toDir;
    RecFormat fromfmt, tofmt;
} * OperateDirData;

const char* EXTNAMES[] = {
    ".xqf", ".bin", ".json", ".pgn_iccs", ".pgn_zh", ".pgn_cc"
};
static const char FILETAG[] = "learnchess";

// 着法相关的字符数组静态全局变量
static const wchar_t* PRECHAR = L"前中后";
static const wchar_t* MOVCHAR = L"退平进";
static const wchar_t* NUMWCHAR[PIECECOLORNUM] = { L"一二三四五六七八九", L"１２３４５６７８９" };

// 根据文件扩展名取得存储记录类型
static RecFormat getRecFormat__(const char* ext)
{
    if (ext) {
        char lowExt[WCHARSIZE] = { 0 };
        int len = fmin(WCHARSIZE, strlen(ext));
        for (int i = 0; i < len; ++i)
            lowExt[i] = tolower(ext[i]);
        for (int f = XQF; f < NOTFMT; ++f)
            if (strcmp(lowExt, EXTNAMES[f]) == 0)
                return f;
    }
    return NOTFMT;
}

static bool fileIsRight__(const char* fileName)
{
    return fileName && getRecFormat__(getExtName(fileName)) != NOTFMT;
}

// 从文件读取到chessManual
static void readChessManual__(ChessManual cm, const char* fileName);

ChessManual newChessManual(const char* fileName)
{
    ChessManual cm = malloc(sizeof(struct ChessManual));
    assert(cm);
    if (fileName == NULL)
        cm->fileName = NULL;
    else {
        cm->fileName = malloc(strlen(fileName) + 1);
        strcpy(cm->fileName, fileName);
    }
    cm->board = newBoard();
    cm->rootMove = newMove();
    cm->curMove = cm->rootMove;
    for (int i = 0; i < INFOSIZE; ++i)
        for (int j = 0; j < 2; ++j)
            cm->info[i][j] = NULL;
    cm->infoCount = cm->movCount_ = cm->remCount_ = cm->maxRemLen_ = cm->maxRow_ = cm->maxCol_ = 0;
    readChessManual__(cm, fileName);
    return cm;
}

ChessManual resetChessManual(ChessManual* cm, const char* fileName)
{
    delChessManual(*cm);
    return *cm = newChessManual(fileName);
}

void delChessManual(ChessManual cm)
{
    if (cm == NULL)
        return;
    free(cm->fileName);
    for (int i = 0; i < cm->infoCount; ++i)
        for (int j = 0; j < 2; ++j)
            free(cm->info[i][j]);
    delMove(cm->rootMove);
    delBoard(cm->board);
    free(cm);
}

Move getRootMove(ChessManual cm) { return cm->rootMove; }

const char* getFileName_cm(ChessManual cm) { return cm->fileName; }

static bool go__(ChessManual cm, Move curMove)
{
    doMove(curMove);
    cm->curMove = curMove;
    return true;
}

bool go(ChessManual cm)
{
    if (getNext(cm->curMove))
        return go__(cm, getNext(cm->curMove));
    return false;
}

bool goOther(ChessManual cm)
{
    if (getOther(cm->curMove)) {
        undoMove(cm->curMove);
        return go__(cm, getOther(cm->curMove));
    }
    return false;
}

int goEnd(ChessManual cm)
{
    int count = 0;
    while (getNext(cm->curMove)) {
        go__(cm, getNext(cm->curMove));
        ++count;
    }
    return count;
}

int goTo(ChessManual cm, Move move)
{
    assert(move);
    cm->curMove = move;
    Move preMoves[getNextNo(move)];
    int count = getPreMoves(preMoves, move);
    for (int i = 0; i < count; ++i)
        doMove(preMoves[i]);
    return count;
}

static bool back__(ChessManual cm)
{
    undoMove(cm->curMove);
    cm->curMove = getSimplePre(cm->curMove);
    return true;
}

bool back(ChessManual cm)
{
    if (hasPreOther(cm->curMove))
        return backOther(cm);
    else if (getSimplePre(cm->curMove))
        return back__(cm);
    return false;
}

bool backNext(ChessManual cm)
{
    if (getSimplePre(cm->curMove) && !hasPreOther(cm->curMove)) {
        back__(cm);
        return true;
    }
    return false;
}

bool backOther(ChessManual cm)
{
    if (hasPreOther(cm->curMove)) {
        back__(cm); // 变着回退
        doMove(cm->curMove); // 前变执行
        return true;
    }
    return false;
}

int backToPre(ChessManual cm)
{
    int count = 0;
    while (hasPreOther(cm->curMove)) {
        backOther(cm);
        ++count;
    }
    backNext(cm);
    return ++count;
}

int backFirst(ChessManual cm)
{
    int count = 0;
    while (getSimplePre(cm->curMove)) {
        back(cm);
        ++count;
    }
    return count;
}

int backTo(ChessManual cm, Move move)
{
    int count = 0;
    while (getSimplePre(cm->curMove) && cm->curMove != move) {
        back(cm);
        ++count;
    }
    return count;
}

int goInc(ChessManual cm, int inc)
{
    int incCount = abs(inc), count = 0;
    bool (*func)(ChessManual) = inc > 0 ? go : back;
    while (incCount-- > 0)
        if (func(cm))
            ++count;
    return count;
}

Move appendMove(ChessManual cm, const wchar_t* wstr, RecFormat fmt, wchar_t* remark, bool isOther)
{
    if (isOther)
        undoMove(cm->curMove);
    Move move = addMove(cm->curMove, cm->board, wstr, fmt, remark, isOther);
    if (move == NULL) {
        if (isOther)
            doMove(cm->curMove);
        //printBoard(cm->board, fmt, isOther, wstr);
        return NULL;
    }
    assert(move != NULL);
    go__(cm, move);
    return move;
}

const wchar_t* getICCS_cm(wchar_t* iccs, ChessManual cm, ChangeType ct)
{
    return getICCS_mt(iccs, cm->curMove, cm->board, ct);
}

void addInfoItem(ChessManual cm, const wchar_t* name, const wchar_t* value)
{
    int count = cm->infoCount, nameLen = wcslen(name) + 1;
    if (count == INFOSIZE || nameLen == 1)
        return;
    cm->info[count][0] = malloc(nameLen * sizeof(wchar_t));
    assert(cm->info[count][0]);
    wcscpy(cm->info[count][0], name);
    cm->info[count][1] = malloc((wcslen(value) + 1) * sizeof(wchar_t));
    assert(cm->info[count][1]);
    wcscpy(cm->info[count][1], value);
    ++(cm->infoCount);
}

const wchar_t* getInfoValue(ChessManual cm, const wchar_t* name)
{
    for (int i = 0; i < cm->infoCount; ++i)
        if (wcscmp(cm->info[i][0], name) == 0)
            return cm->info[i][1];
    return L"";
}

void delInfoItem(ChessManual cm, const wchar_t* name)
{
    for (int i = 0; i < cm->infoCount; ++i)
        if (wcscmp(cm->info[i][0], name) == 0) {
            for (int j = 0; j < 2; ++j)
                free(cm->info[i][j]);
            for (int k = i + 1; k < cm->infoCount; ++k)
                for (int j = 0; j < 2; ++j)
                    cm->info[k - 1][j] = cm->info[k][j];
            --(cm->infoCount);
            break;
        }
}

static wchar_t* getFENFromCM__(ChessManual cm)
{
    wchar_t fen[] = L"FEN";
    for (int i = 0; i < cm->infoCount; ++i)
        if (wcscmp(cm->info[i][0], fen) == 0)
            return cm->info[i][1];
    addInfoItem(cm, fen, L"rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR");
    return cm->info[cm->infoCount - 1][1];
}

static void setMoveNumZhStr__(ChessManual cm, Move move)
{ //*
    ++cm->movCount_;
    if (getOtherNo(move) > cm->maxCol_)
        setOtherNo(move, cm->maxCol_);
    if (getNextNo(move) > cm->maxRow_)
        cm->maxRow_ = getNextNo(move);

    setCC_ColNo(move, cm->maxCol_); // # 本着在视图中的列数
    if (getRemark(move)) {
        ++cm->remCount_;
        int maxRemLen_ = wcslen(getRemark(move));
        if (maxRemLen_ > cm->maxRemLen_)
            cm->maxRemLen_ = maxRemLen_;
    }

    //assert(move->fseat);
    //wprintf(L"%3d=> %02x->%02x\n", __LINE__, move->fseat, move->tseat);
    // 先深度搜索
    setMoveZhStr(move, cm->board);
    //wprintf(L"%ls ", __LINE__, getZhStr(move));

    doMove(move);
    if (getNext(move))
        setMoveNumZhStr__(cm, getNext(move));
    undoMove(move);

    // 后广度搜索
    if (getOther(move)) {
        ++cm->maxCol_;
        setMoveNumZhStr__(cm, getOther(move));
    }
}

void setMoveNumZhStr(ChessManual cm)
{
    cm->movCount_ = cm->remCount_ = cm->maxRemLen_ = cm->maxRow_ = cm->maxCol_ = 0;
    setMoveNumZhStr__(cm, getNext(cm->rootMove));
}

static const wchar_t* getRcStr_rowcol__(wchar_t* rcStr, int frowcol, int trowcol)
{
    swprintf(rcStr, 5, L"%02x%02x", frowcol, trowcol);
    return rcStr;
}

// 供readXQF使用的有关解密钥匙
static int Version = 0, KeyRMKSize = 0;
static unsigned char KeyXYf = 0, KeyXYt = 0, F32Keys[PIECENUM] = { 0 };

static unsigned char calkey__(unsigned char bKey, unsigned char cKey)
{
    return (((((bKey * bKey) * 3 + 9) * 3 + 8) * 2 + 1) * 3 + 8) * cKey; // % 256; // 保持为<256
}

static unsigned char sub__(unsigned char a, unsigned char b) { return a - b; } // 保持为<256

static void readBytes__(unsigned char* bytes, int size, FILE* fin)
{
    long pos = ftell(fin);
    fread(bytes, sizeof(unsigned char), size, fin);
    if (Version > 10) // '字节解密'
        for (int i = 0; i != size; ++i)
            bytes[i] = sub__(bytes[i], F32Keys[(pos + i) % 32]);
}

static void readTagRowcolRemark_XQF__(unsigned char* tag, int* fcolrow, int* tcolrow, wchar_t** remark, FILE* fin)
{
    unsigned char data[4] = { 0 };
    readBytes__(data, 4, fin);
    if (Version <= 10)
        data[2] = (data[2] & 0xF0 ? 0x80 : 0) | (data[2] & 0x0F ? 0x40 : 0);
    else
        data[2] &= 0xE0;
    *tag = data[2];
    //# 一步棋的起点和终点有简单的加密计算，读入时需要还原
    *fcolrow = sub__(data[0], 0X18 + KeyXYf);
    *tcolrow = sub__(data[1], 0X20 + KeyXYt);

    if (Version <= 10 || (data[2] & 0x20)) {
        unsigned char clen[4] = { 0 };
        readBytes__(clen, 4, fin);
        int RemarkSize = *(int*)clen - KeyRMKSize;
        if (RemarkSize > 0) {
            size_t len = RemarkSize + 1;
            unsigned char rem[len];
            readBytes__(rem, RemarkSize, fin);
            rem[RemarkSize] = '\x0';

            *remark = calloc(len, sizeof(wchar_t));
            assert(*remark);

#ifdef __linux
            size_t outlen = WIDEWCHARSIZE;
            char remc[outlen];
            code_convert("gbk", "utf-8", (char*)rem, remc, &outlen);
            mbstowcs(*remark, remc, len);
#else
            mbstowcs(*remark, (char*)rem, len);
#endif
            //wcstombs(remc, *remark, WIDEWCHARSIZE - 1);
            //printf("\nsize:%ld %s", strlen(remc), remc);
        }
    }
}

static void readMove_XQF__(ChessManual cm, FILE* fin, bool isOther)
{
    unsigned char tag = 0;
    int fcolrow = 0, tcolrow = 0;
    wchar_t* remark = NULL;
    readTagRowcolRemark_XQF__(&tag, &fcolrow, &tcolrow, &remark, fin);
    int frow = fcolrow % 10, fcol = fcolrow / 10, trow = tcolrow % 10, tcol = tcolrow / 10;
    Move move;
    if (isXQFStoreError(cm->curMove, frow, fcol, trow, tcol)) {
        move = cm->curMove;
        if (remark)
            setRemark(move, remark);
    } else {
        wchar_t rcStr[5];
        getRcStr_rowcol__(rcStr, getRowCol_rc(frow, fcol), getRowCol_rc(trow, tcol));
        move = appendMove(cm, rcStr, XQF, remark, isOther);
    }

    if (tag & 0x80) //# 有左子树
        readMove_XQF__(cm, fin, false);

    backTo(cm, move);
    if (tag & 0x40) // # 有右子树
        readMove_XQF__(cm, fin, true);
}

static void readXQF__(ChessManual cm, FILE* fin)
{
    unsigned char xqfData[1024] = { 0 };
    fread(xqfData, sizeof(unsigned char), 1024, fin);
    unsigned char Signature[3] = { 0 }, headKeyMask, ProductId[4] = { 0 }, //Version, 文件标记'XQ'=$5158/版本/加密掩码/ProductId[4], 产品(厂商的产品号)
        headKeyOrA, headKeyOrB, headKeyOrC, headKeyOrD,
                  headKeysSum, headKeyXY, headKeyXYf, headKeyXYt, // 加密的钥匙和/棋子布局位置钥匙/棋谱起点钥匙/棋谱终点钥匙
        headQiziXY[PIECENUM] = { 0 }, // 32个棋子的原始位置
        // 用单字节坐标表示, 将字节变为十进制, 十位数为X(0-8)个位数为Y(0-9),
        // 棋盘的左下角为原点(0, 0). 32个棋子的位置从1到32依次为:
        // 红: 车马相士帅士相马车炮炮兵兵兵兵兵 (位置从右到左, 从下到上)
        // 黑: 车马象士将士象马车炮炮卒卒卒卒卒 (位置从右到左, 从下到上)PlayStepNo[2],
        PlayStepNo[2] = { 0 }, headWhoPlay, headPlayResult, PlayNodes[4] = { 0 }, PTreePos[4] = { 0 }, Reserved1[4] = { 0 },
                  // 该谁下 0-红先, 1-黑先/最终结果 0-未知, 1-红胜 2-黑胜, 3-和棋
        headCodeA_H[16] = { 0 };
    char TitleA[65] = { 0 }, TitleB[65] = { 0 }, //对局类型(开,中,残等)
        Event[65] = { 0 }, Date[17] = { 0 }, Site[17] = { 0 }, Red[17] = { 0 }, Black[17] = { 0 },
         Opening[65] = { 0 }, Redtime[17] = { 0 }, Blktime[17] = { 0 }, Reservedh[33] = { 0 },
         RMKWriter[17] = { 0 }, Author[17] = { 0 }; //, Other[528]; // 棋谱评论员/文件的作者
    memcpy(Signature, xqfData, 2);
    Version = xqfData[2];
    headKeyMask = xqfData[3];
    memcpy(ProductId, xqfData + 4, 4);
    headKeyOrA = xqfData[8];
    headKeyOrB = xqfData[9];
    headKeyOrC = xqfData[10];
    headKeyOrD = xqfData[11];
    headKeysSum = xqfData[12];
    headKeyXY = xqfData[13];
    headKeyXYf = xqfData[14];
    headKeyXYt = xqfData[15]; // = 16 bytes
    memcpy(headQiziXY, xqfData + 16, 32); // = 48 bytes
    memcpy(PlayStepNo, xqfData + 48, 2);
    headWhoPlay = xqfData[50];
    headPlayResult = xqfData[51];
    memcpy(PlayNodes, xqfData + 52, 4);
    memcpy(PTreePos, xqfData + 56, 4);
    memcpy(Reserved1, xqfData + 60, 4); // = 64 bytes
    memcpy(headCodeA_H, xqfData + 64, 16);

    // 以下为棋局信息
    memcpy(TitleA, xqfData + 80, 64);
    memcpy(TitleB, xqfData + 144, 64);
    memcpy(Event, xqfData + 208, 64);
    memcpy(Date, xqfData + 272, 16);
    memcpy(Site, xqfData + 288, 16);
    memcpy(Red, xqfData + 304, 16);
    memcpy(Black, xqfData + 320, 16);
    memcpy(Opening, xqfData + 336, 64);
    memcpy(Redtime, xqfData + 400, 16);
    memcpy(Blktime, xqfData + 416, 16);
    memcpy(Reservedh, xqfData + 432, 32);
    memcpy(RMKWriter, xqfData + 464, 16);
    memcpy(Author, xqfData + 480, 16); // = 496 bytes

    assert(Signature[0] == 0x58 || Signature[1] == 0x51);
    assert((headKeysSum + headKeyXY + headKeyXYf + headKeyXYt) % 256 == 0); // L" 检查密码校验和不对，不等于0。\n";
    assert(Version <= 18); // L" 这是一个高版本的XQF文件，您需要更高版本的XQStudio来读取这个文件。\n";

    // 计算解密数据
    unsigned char KeyXY; //KeyXYf, KeyXYt, F32Keys[PIECENUM],//int KeyRMKSize = 0;
    if (Version <= 10) { // version <= 10 兼容1.0以前的版本
        KeyXY = KeyRMKSize = KeyXYf = KeyXYt = 0;
    } else {
        KeyXY = calkey__(headKeyXY, headKeyXY);
        KeyXYf = calkey__(headKeyXYf, KeyXY);
        KeyXYt = calkey__(headKeyXYt, KeyXYf);
        KeyRMKSize = (headKeysSum * 256 + headKeyXY) % 32000 + 767; // % 65536
        if (Version >= 12) { // 棋子位置循环移动
            unsigned char Qixy[PIECENUM] = { 0 };
            memcpy(Qixy, headQiziXY, PIECENUM);
            for (int i = 0; i != PIECENUM; ++i)
                headQiziXY[(i + KeyXY + 1) % PIECENUM] = Qixy[i];
        }
        for (int i = 0; i != PIECENUM; ++i)
            headQiziXY[i] -= KeyXY; // 保持为8位无符号整数，<256
    }
    int KeyBytes[4] = {
        (headKeysSum & headKeyMask) | headKeyOrA,
        (headKeyXY & headKeyMask) | headKeyOrB,
        (headKeyXYf & headKeyMask) | headKeyOrC,
        (headKeyXYt & headKeyMask) | headKeyOrD
    };
    const char copyright[] = "[(C) Copyright Mr. Dong Shiwei.]";
    for (int i = 0; i != PIECENUM; ++i)
        F32Keys[i] = copyright[i] & KeyBytes[i % 4]; // ord(c)

    //wprintf(L"%3d=> %d %d %d %d\n", __LINE__, Version, KeyRMKSize, KeyXYf, KeyXYt);

    // 取得棋子字符串
    wchar_t pieChars[SEATNUM + 1] = { 0 };
    wmemset(pieChars, getBlankChar(), SEATNUM);
    const wchar_t QiziChars[] = L"RNBAKABNRCCPPPPPrnbakabnrccppppp"; // QiziXY设定的棋子顺序
    for (int i = 0; i != PIECENUM; ++i) {
        if (headQiziXY[i] <= 89)
            // 用字节坐标表示, 将字节变为十进制,  十位数为X(0-8),个位数为Y(0-9)
            // 棋盘的左下角为原点(0, 0)，需转换成FEN格式：以左上角为原点(0, 0)
            pieChars[(9 - headQiziXY[i] % 10) * 9 + headQiziXY[i] / 10] = QiziChars[i];
    }

    wchar_t tempStr[WIDEWCHARSIZE] = { 0 };
    char* values[] = {
        TitleA, Event, Date, Site, Red, Black,
        Opening, RMKWriter, Author
    };
    wchar_t* names[] = {
        L"TitleA", L"Event", L"Date", L"Site", L"Red", L"Black",
        L"Opening", L"RMKWriter", L"Author"
    };
    for (int i = 0; i != sizeof(names) / sizeof(names[0]); ++i) {
#ifdef __linux
        size_t len = strlen(values[i]) + 1;
        size_t outlen = len * 3;
        char tstr[outlen];
        code_convert("gbk", "utf-8", values[i], tstr, &outlen);
        mbstowcs(tempStr, tstr, WIDEWCHARSIZE - 1);
#else
        mbstowcs(tempStr, values[i], WIDEWCHARSIZE - 1);
#endif
        //wcstombs(tstr, tempStr, WIDEWCHARSIZE - 1);
        //printf("\nsize:%ld %s", strlen(tstr), tstr);

        addInfoItem(cm, names[i], tempStr);
    }
    wchar_t* PlayType[] = { L"全局", L"开局", L"中局", L"残局" };
    addInfoItem(cm, L"PlayType", PlayType[headCodeA_H[0]]); // 编码定义存储
    getFEN_pieChars(tempStr, pieChars);
    addInfoItem(cm, L"FEN", wcscat(tempStr, headWhoPlay ? L" -r" : L" -b")); // 转换FEN存储
    wchar_t* Result[] = { L"未知", L"红胜", L"黑胜", L"和棋" };
    addInfoItem(cm, L"Result", Result[headPlayResult]); // 编码定义存储
    swprintf(tempStr, WIDEWCHARSIZE, L"%d", Version);
    addInfoItem(cm, L"Version", tempStr); // 整数存储
    // "标题: 赛事: 日期: 地点: 红方: 黑方: 结果: 评论: 作者: "

    fseek(fin, 1024, SEEK_SET);
    unsigned char tag = 0;
    int fcolrow = 0, tcolrow = 0;
    wchar_t* remark = NULL;
    readTagRowcolRemark_XQF__(&tag, &fcolrow, &tcolrow, &remark, fin);
    setRemark(cm->rootMove, remark);

    setBoard_FEN(cm->board, getFENFromCM__(cm));
    if (tag & 0x80)
        readMove_XQF__(cm, fin, false);

    backFirst(cm);
}

static wchar_t* readWstring_BIN__(FILE* fin)
{
    int len = 0;
    fread(&len, sizeof(int), 1, fin);
    wchar_t* wstr = calloc(len, sizeof(wchar_t));
    assert(wstr);
    fread(wstr, sizeof(wchar_t), len, fin);
    return wstr;
}

static char readMoveTagRemark_BIN__(wchar_t** premark, FILE* fin)
{
    char tag = 0;
    fread(&tag, sizeof(char), 1, fin);
    if (tag & 0x20)
        *premark = readWstring_BIN__(fin);
    return tag;
}

static void readMove_BIN__(ChessManual cm, FILE* fin, bool isOther)
{
    unsigned char frowcol, trowcol;
    fread(&frowcol, sizeof(unsigned char), 1, fin);
    fread(&trowcol, sizeof(unsigned char), 1, fin);
    wchar_t* remark = NULL;
    char tag = readMoveTagRemark_BIN__(&remark, fin);
    wchar_t rcStr[5];
    Move move = appendMove(cm, getRcStr_rowcol__(rcStr, frowcol, trowcol), BIN, remark, isOther);

    if (tag & 0x80)
        readMove_BIN__(cm, fin, false);

    backTo(cm, move);
    if (tag & 0x40)
        readMove_BIN__(cm, fin, true);
}

static void readBin__(ChessManual cm, FILE* fin)
{
    char fileTag[sizeof(FILETAG)];
    fread(&fileTag, sizeof(char), sizeof(FILETAG), fin);
    if (strcmp(fileTag, FILETAG) != 0) //文件标志不对
        return;
    char tag = 0, infoCount = 0;
    fread(&tag, sizeof(char), 1, fin);
    if (tag & 0x10) {
        fread(&infoCount, sizeof(char), 1, fin);
        for (int i = 0; i < infoCount; ++i) {
            wchar_t* name = readWstring_BIN__(fin);
            wchar_t* value = readWstring_BIN__(fin);
            addInfoItem(cm, name, value);
            free(name);
            free(value);
        }
    }
    setBoard_FEN(cm->board, getFENFromCM__(cm));

    if (tag & 0x20)
        setRemark(cm->rootMove, readWstring_BIN__(fin));
    if (tag & 0x80) {
        wchar_t* remark = NULL;
        char tag = readMoveTagRemark_BIN__(&remark, fin);
        if (remark)
            setRemark(cm->rootMove, remark);
        if (tag & 0x80)
            readMove_BIN__(cm, fin, false);
    }
    backFirst(cm);
}

static void writeWstring_BIN__(FILE* fout, const wchar_t* wstr)
{
    int len = wcslen(wstr) + 1;
    fwrite(&len, sizeof(int), 1, fout);
    fwrite(wstr, sizeof(wchar_t), len, fout);
}

static void writeMoveTagRemark_BIN__(FILE* fout, CMove move)
{
    char tag = ((getNext(move) ? 0x80 : 0x00)
        | (getOther(move) ? 0x40 : 0x00)
        | (getRemark(move) ? 0x20 : 0x00));
    fwrite(&tag, sizeof(char), 1, fout);
    if (tag & 0x20)
        writeWstring_BIN__(fout, getRemark(move));
}

static void writeMove_BIN__(FILE* fout, CMove move)
{
    if (move == NULL)
        return;
    //unsigned char frowcol = getRowCol_s(move->fseat), trowcol = getRowCol_s(move->tseat);
    unsigned char frowcol = getFromRowCol_m(move), trowcol = getToRowCol_m(move);
    fwrite(&frowcol, sizeof(unsigned char), 1, fout);
    fwrite(&trowcol, sizeof(unsigned char), 1, fout);
    writeMoveTagRemark_BIN__(fout, move);

    if (getNext(move))
        writeMove_BIN__(fout, getNext(move));
    if (getOther(move))
        writeMove_BIN__(fout, getOther(move));
}

static void writeBIN__(FILE* fout, ChessManual cm)
{
    fwrite(FILETAG, sizeof(char), sizeof(FILETAG), fout);
    char infoCount = cm->infoCount;
    char tag = ((infoCount > 0 ? 0x10 : 0x00) | (getRemark(cm->rootMove) ? 0x20 : 0x00) | (getNext(cm->rootMove) ? 0x80 : 0x00));
    fwrite(&tag, sizeof(char), 1, fout);
    if (tag & 0x10) {
        fwrite(&infoCount, sizeof(char), 1, fout);
        for (int i = 0; i < infoCount; ++i) {
            writeWstring_BIN__(fout, cm->info[i][0]);
            writeWstring_BIN__(fout, cm->info[i][1]);
        }
    }

    if (tag & 0x20)
        writeWstring_BIN__(fout, getRemark(cm->rootMove));
    if (tag & 0x80) {
        writeMoveTagRemark_BIN__(fout, cm->rootMove);
        if (getNext(cm->rootMove))
            writeMove_BIN__(fout, getNext(cm->rootMove));
    }
}

static wchar_t* readMoveRemark_JSON__(const cJSON* moveJSON)
{
    wchar_t* remark = NULL;
    cJSON* remarkJSON = cJSON_GetObjectItem(moveJSON, "r");
    if (remarkJSON) {
        int len = mbstowcs(NULL, remarkJSON->valuestring, 0) + 1;
        remark = calloc(len, sizeof(wchar_t));
        assert(remark);
        mbstowcs(remark, remarkJSON->valuestring, len);
    }
    return remark;
}

static void readMove_JSON__(ChessManual cm, const cJSON* moveJSON, bool isOther)
{
    int frowcol = cJSON_GetObjectItem(moveJSON, "f")->valueint;
    int trowcol = cJSON_GetObjectItem(moveJSON, "t")->valueint;
    wchar_t rcStr[5];
    Move move = appendMove(cm, getRcStr_rowcol__(rcStr, frowcol, trowcol), JSON, readMoveRemark_JSON__(moveJSON), isOther);

    cJSON* nmoveJSON = cJSON_GetObjectItem(moveJSON, "n");
    if (nmoveJSON)
        readMove_JSON__(cm, nmoveJSON, false);

    backTo(cm, move);
    cJSON* omoveJSON = cJSON_GetObjectItem(moveJSON, "o");
    if (omoveJSON)
        readMove_JSON__(cm, omoveJSON, true);
}

static void readJSON__(ChessManual cm, FILE* fin)
{
    fseek(fin, 0L, SEEK_END); // 定位到文件末尾
    long last = ftell(fin);
    char* manualString = malloc(last + 1);
    assert(manualString);
    fseek(fin, 0L, SEEK_SET); // 定位到文件开始
    fread(manualString, sizeof(char), last, fin);
    manualString[last] = '\x0';
    cJSON* manualJSON = cJSON_Parse(manualString);
    free(manualString);

    cJSON* infoJSON = cJSON_GetObjectItem(manualJSON, "info");
    int infoCount = cJSON_GetArraySize(infoJSON);
    for (int i = 0; i < infoCount; ++i) {
        cJSON* keyValueJSON = cJSON_GetArrayItem(infoJSON, i);
        wchar_t nameValue[2][WIDEWCHARSIZE] = { 0 };
        for (int j = 0; j < 2; ++j) {
            cJSON* kvItem = cJSON_GetArrayItem(keyValueJSON, j);
            char* str = cJSON_GetStringValue(kvItem);
            mbstowcs(nameValue[j], str, WIDEWCHARSIZE);
        }
        addInfoItem(cm, nameValue[0], nameValue[1]);
    }
    setBoard_FEN(cm->board, getFENFromCM__(cm));

    cJSON* rootMoveJSON = cJSON_GetObjectItem(manualJSON, "rootmove");
    if (rootMoveJSON) {
        setRemark(cm->rootMove, readMoveRemark_JSON__(rootMoveJSON));
        cJSON* moveJSON = cJSON_GetObjectItem(rootMoveJSON, "n");
        if (moveJSON)
            readMove_JSON__(cm, moveJSON, false);
    }
    backFirst(cm);
    cJSON_Delete(manualJSON);
}

static void writeMoveRemark_JSON__(cJSON* moveJSON, CMove move)
{
    const wchar_t* wremark = getRemark(move);
    if (wremark) {
        //size_t len = wcslen(wremark) * sizeof(wchar_t) + 1;
        size_t len = wcstombs(NULL, wremark, 0) + 1;
        char remark[len];
        wcstombs(remark, wremark, len);
        cJSON_AddStringToObject(moveJSON, "r", remark);
    }
}

static void writeMove_JSON__(cJSON* moveJSON, CMove move)
{
    cJSON_AddNumberToObject(moveJSON, "f", getFromRowCol_m(move));
    cJSON_AddNumberToObject(moveJSON, "t", getToRowCol_m(move));
    writeMoveRemark_JSON__(moveJSON, move);

    if (getOther(move)) {
        cJSON* omoveJSON = cJSON_CreateObject();
        writeMove_JSON__(omoveJSON, getOther(move));
        cJSON_AddItemToObject(moveJSON, "o", omoveJSON);
    }

    if (getNext(move)) {
        cJSON* nmoveJSON = cJSON_CreateObject();
        writeMove_JSON__(nmoveJSON, getNext(move));
        cJSON_AddItemToObject(moveJSON, "n", nmoveJSON);
    }
}

static void writeJSON__(FILE* fout, ChessManual cm)
{
    cJSON *manualJSON = cJSON_CreateObject(),
          *infoJSON = cJSON_CreateArray(),
          *rootmoveJSON = cJSON_CreateObject();

    for (int i = 0; i < cm->infoCount; ++i) {
        size_t namelen = wcstombs(NULL, cm->info[i][0], 0) + 1,
               valuelen = wcstombs(NULL, cm->info[i][1], 0) + 1;
        char name[namelen], value[valuelen];
        wcstombs(name, cm->info[i][0], namelen);
        wcstombs(value, cm->info[i][1], valuelen);
        cJSON_AddItemToArray(infoJSON,
            cJSON_CreateStringArray((const char* const[]) { name, value }, 2));
    }
    cJSON_AddItemToObject(manualJSON, "info", infoJSON);

    //writeMove_JSON(rootmoveJSON, cm->rootMove);
    writeMoveRemark_JSON__(rootmoveJSON, cm->rootMove);
    if (getNext(cm->rootMove)) {
        cJSON* moveJSON = cJSON_CreateObject();
        writeMove_JSON__(moveJSON, getNext(cm->rootMove));
        cJSON_AddItemToObject(rootmoveJSON, "n", moveJSON);
    }

    cJSON_AddItemToObject(manualJSON, "rootmove", rootmoveJSON);

    char* manualString = cJSON_Print(manualJSON);
    fwrite(manualString, sizeof(char), strlen(manualString), fout);
    free(manualString);
    cJSON_Delete(manualJSON); // 释放自身及所有子对象
}

static void readInfo_PGN__(ChessManual cm, FILE* fin)
{
    const char* error;
    int erroffset = 0, infoCount = 0, offsetArray[30]; //OVECCOUNT = 10,
    const wchar_t* infoPat = L"\\[(\\w+)\\s+\"([\\s\\S]*?)\"\\]";
    void* infoReg = pcrewch_compile(infoPat, 0, &error, &erroffset, NULL);
    assert(infoReg);
    wchar_t infoStr[WIDEWCHARSIZE] = { 0 };

    //printf("\nline:%d %s", __LINE__, "read info...\n");
    while (fgetws(infoStr, WIDEWCHARSIZE, fin) && infoStr[0] != L'\n') { // 以空行为终止特征
        infoCount = pcrewch_exec(infoReg, NULL, infoStr, wcslen(infoStr), 0, 0, offsetArray, 30);
        if (infoCount < 0)
            continue;

        wchar_t name[WCHARSIZE], value[WIDEWCHARSIZE];
        copySubStr(name, infoStr, offsetArray[2], offsetArray[3]);
        copySubStr(value, infoStr, offsetArray[4], offsetArray[5]);
        addInfoItem(cm, name, value);
    }
    pcrewch_free(infoReg);
}

wchar_t* getZhWChars(wchar_t* ZhWChars)
{
    swprintf(ZhWChars, WCHARSIZE, L"%ls%ls%ls%ls%ls",
        PRECHAR, getPieceNames(), MOVCHAR, NUMWCHAR[RED], NUMWCHAR[BLACK]);
    return ZhWChars;
}

static void readMove_PGN_ICCSZH__(ChessManual cm, FILE* fin, RecFormat fmt)
{
    //printf("\n读取文件内容到字符串... ");
    wchar_t *moveStr = getWString(fin), *tempMoveStr = moveStr;
    if (moveStr == NULL)
        return;

    bool isPGN_ZH = fmt == PGN_ZH;
    const wchar_t* remStr = L"(?:[\\s\\n]*\\{([\\s\\S]*?)\\})?";
    wchar_t ZhWChars[WCHARSIZE], movePat[WCHARSIZE], remPat[WCHARSIZE];
    swprintf(movePat, WCHARSIZE, L"(\\()?(?:[\\d\\.\\s]+)([%ls]{4})%ls(?:[\\s\\n]*(\\)+))?",
        isPGN_ZH ? getZhWChars(ZhWChars) : L"a-i\\d", remStr); // 可能存在多个右括号
    swprintf(remPat, WCHARSIZE, L"%ls1\\.", remStr);

    const char* error;
    int erroffset = 0;
    void *moveReg = pcrewch_compile(movePat, 0, &error, &erroffset, NULL),
         *remReg = pcrewch_compile(remPat, 0, &error, &erroffset, NULL);
    assert(moveReg);
    assert(remReg);

    int ovector[30] = { 0 },
        regCount = pcrewch_exec(remReg, NULL, moveStr, wcslen(moveStr), 0, 0, ovector, 30);
    if (regCount <= 0)
        return;

    wchar_t* remark = getSubStr(moveStr, ovector[2], ovector[3]);
    setRemark(cm->rootMove, remark); // 赋值一个动态分配内存的指针

    Move preOtherMoves[WIDEWCHARSIZE] = { NULL };
    int preOthIndex = 0, length = 0;
    //printf("读取moveStr... \n");
    while ((tempMoveStr += ovector[1]) && (length = wcslen(tempMoveStr)) > 0) {
        regCount = pcrewch_exec(moveReg, NULL, tempMoveStr, length, 0, 0, ovector, 30);
        if (regCount <= 0)
            break;
        // 是否有"("
        bool isOther = ovector[3] > ovector[2];
        if (isOther)
            preOtherMoves[preOthIndex++] = cm->curMove;

        // 提取字符串
        wchar_t iccs_zhStr[6];
        copySubStr(iccs_zhStr, tempMoveStr, ovector[4], ovector[5]);
        remark = getSubStr(tempMoveStr, ovector[6], ovector[7]);
        appendMove(cm, iccs_zhStr, fmt, remark, isOther);

        // 是否有一个以上的")"
        int num = ovector[9] - ovector[8];
        while (num--)
            backTo(cm, preOtherMoves[--preOthIndex]);
    }
    backFirst(cm);

    pcrewch_free(remReg);
    pcrewch_free(moveReg);
    free(moveStr);
}

static wchar_t* getRemark_PGN_CC__(wchar_t* remLines[], int remCount, int row, int col)
{
    wchar_t name[12] = { 0 };
    swprintf(name, 12, L"(%d,%d)", row, col);
    for (int index = 0; index < remCount; ++index)
        if (wcscmp(name, remLines[index * 2]) == 0)
            return remLines[index * 2 + 1];
    return NULL;
}

static void addMove_PGN_CC__(ChessManual cm, void* moveReg,
    wchar_t* moveLines[], int rowNum, int colNum, int row, int col,
    wchar_t* remLines[], int remCount, bool isOther)
{
    wchar_t* zhStr = moveLines[row * colNum + col];
    while (zhStr[0] == L'…')
        zhStr = moveLines[row * colNum + (++col)];
    int ovector[9],
        regCount = pcrewch_exec(moveReg, NULL, zhStr, wcslen(zhStr), 0, 0, ovector, 9);
    assert(regCount > 0);

    wchar_t lastwc = zhStr[4];
    zhStr[4] = L'\x0';
    Move move = appendMove(cm, zhStr, PGN_CC, getRemark_PGN_CC__(remLines, remCount, row, col), isOther);

    if (lastwc == L'…')
        addMove_PGN_CC__(cm, moveReg, moveLines, rowNum, colNum, row, col + 1, remLines, remCount, true);

    backTo(cm, move);
    if (getNextNo(move) < rowNum - 1
        && moveLines[(row + 1) * colNum + col][0] != L'　') {
        addMove_PGN_CC__(cm, moveReg, moveLines, rowNum, colNum, row + 1, col, remLines, remCount, false);
    }
}

static void readMove_PGN_CC__(ChessManual cm, FILE* fin)
{
    // 设置字符串容量
    wchar_t wch;
    long start = ftell(fin);
    if (start < 0)
        return;
    int rowNum = 0, rowIndex = 0, remArrayLen = 0, lineSize = 3; // lineSize 加回车和空字符位置
    while ((wch = fgetwc(fin)) && wch != L'\n')
        ++lineSize;
    int colNum = lineSize / 5;
    wchar_t lineStr[lineSize];
    fseek(fin, start, SEEK_SET); // 回到开始
    while (fgetws(lineStr, lineSize, fin) && lineStr[0] != L'\n') { // 空行截止
        ++rowNum;
        fgetws(lineStr, lineSize, fin); // 间隔行则弃掉
    }
    if (colNum == 0 || rowNum == 0)
        return;
    while (fgetws(lineStr, lineSize, fin))
        ++remArrayLen;
    wchar_t **moveLines = calloc((rowNum * colNum), sizeof(wchar_t*)),
            **remLines = calloc(remArrayLen, sizeof(wchar_t*));
    fseek(fin, start, SEEK_SET); // 回到开始

    // 读取着法字符串
    while (fgetws(lineStr, lineSize, fin) && lineStr[0] != L'\n') { // 空行截止
        //wprintf(L"%d: %ls", __LINE__, lineStr);
        for (int col = 0; col < colNum; ++col) {
            moveLines[rowIndex * colNum + col] = getSubStr(lineStr, col * 5, col * 5 + 5);
            //wprintf(L"%d: %ls\n", __LINE__, moveLines[rowNum * colNum + col]);
        }
        ++rowIndex;
        fgetws(lineStr, lineSize, fin);
    }
    assert(rowNum == rowIndex);

    // 读取注解字符串
    int remCount = 0, regCount = 0, ovector[30] = { 0 };
    const wchar_t movePat[] = L"([^…　]{4}[…　])",
                  remPat[] = L"(\\(\\d+,\\d+\\)): \\{([\\s\\S]*?)\\}";
    const char* error;
    int erroffset = 0;
    void *moveReg = pcrewch_compile(movePat, 0, &error, &erroffset, NULL),
         *remReg = pcrewch_compile(remPat, 0, &error, &erroffset, NULL);
    assert(moveReg);
    assert(remReg);

    wchar_t *remarkStr = getWString(fin), *tempRemStr = remarkStr;
    while (tempRemStr != NULL && wcslen(tempRemStr) > 0) {
        regCount = pcrewch_exec(remReg, NULL, tempRemStr, wcslen(tempRemStr), 0, 0, ovector, 30);
        if (regCount <= 0)
            break;
        //int rclen = ovector[3] - ovector[2], remlen = ovector[5] - ovector[4];
        wchar_t *rcKey = malloc((ovector[3] - ovector[2] + 1) * sizeof(wchar_t)),
                *remark = malloc((ovector[5] - ovector[4] + 1) * sizeof(wchar_t));
        assert(rcKey);
        assert(remark);
        copySubStr(rcKey, tempRemStr, ovector[2], ovector[3]);
        copySubStr(remark, tempRemStr, ovector[4], ovector[5]);

        remLines[remCount * 2] = rcKey;
        remLines[remCount * 2 + 1] = remark;
        //wprintf(L"%d: %ls: %ls\n", __LINE__, remLines[remCount * 2], remLines[remCount * 2 + 1]);
        ++remCount;
        tempRemStr += ovector[1];
    }

    setRemark(cm->rootMove, getRemark_PGN_CC__(remLines, remCount, 0, 0));
    if (rowNum > 0)
        addMove_PGN_CC__(cm, moveReg, moveLines, rowNum, colNum, 1, 0, remLines, remCount, false);
    backFirst(cm);

    free(remarkStr);
    for (int i = 0; i < remCount; ++i) {
        free(remLines[i * 2]);
        //free(remLines[i * 2 + 1]); // 已赋值给move->remark
    }
    for (int i = rowNum * colNum - 1; i >= 0; --i)
        free(moveLines[i]);
    free(remLines);
    free(moveLines);
    pcrewch_free(remReg);
    pcrewch_free(moveReg);
}

static void readPGN__(ChessManual cm, FILE* fin, RecFormat fmt)
{
    //printf("\n准备读取info... ");
    readInfo_PGN__(cm, fin);
    // PGN_ZH, PGN_CC在读取move之前需要先设置board
    setBoard_FEN(cm->board, getFENFromCM__(cm));

    //printf("\n准备读取move... ");
    if (fmt == PGN_CC)
        readMove_PGN_CC__(cm, fin);
    else
        readMove_PGN_ICCSZH__(cm, fin, fmt);
}

static void writeRemark_PGN_ICCSZH__s(wchar_t** pmoveStr, size_t* psize, CMove move)
{
    if (getRemark(move)) {
        wchar_t remarkstr[WIDEWCHARSIZE];
        swprintf(remarkstr, WIDEWCHARSIZE, L" \n{%ls}\n ", getRemark(move));
        supper_wcscat(pmoveStr, psize, remarkstr);
    }
}

static void writeMove_PGN_ICCSZH__s(wchar_t** pmoveStr, size_t* psize, Move move, bool isPGN_ZH, bool isOther)
{
    wchar_t boutNum[WCHARSIZE], boutStr[WCHARSIZE], iccs[6] = { 0 };
    swprintf(boutNum, WCHARSIZE, L"%d. ", (getNextNo(move) + 1) / 2);
    bool isEven = getNextNo(move) % 2 == 0;
    swprintf(boutStr, WCHARSIZE, L"%ls%ls%ls%ls ",
        (isOther ? L"(" : L""),
        (isOther || !isEven ? boutNum : L" "),
        (isOther && isEven ? L"... " : L""),
        (isPGN_ZH ? getZhStr(move) : getICCS_m(iccs, move)));
    supper_wcscat(pmoveStr, psize, boutStr);
    writeRemark_PGN_ICCSZH__s(pmoveStr, psize, move);

    if (getOther(move)) {
        writeMove_PGN_ICCSZH__s(pmoveStr, psize, getOther(move), isPGN_ZH, true);
        supper_wcscat(pmoveStr, psize, L")");
    }
    if (getNext(move))
        writeMove_PGN_ICCSZH__s(pmoveStr, psize, getNext(move), isPGN_ZH, false);
}

static void writeMove_PGN_CC__(wchar_t* moveStr, int colNum, CMove move)
{
    int row = getNextNo(move) * 2, firstCol = getCC_ColNo(move) * 5;
    wcsncpy(&moveStr[row * colNum + firstCol], getZhStr(move), 4);

    if (getOther(move)) {
        int fcol = firstCol + 4, tnum = getCC_ColNo(getOther(move)) * 5 - fcol;
        wmemset(&moveStr[row * colNum + fcol], L'…', tnum);
        writeMove_PGN_CC__(moveStr, colNum, getOther(move));
    }

    if (getNext(move)) {
        moveStr[(row + 1) * colNum + firstCol + 2] = L'↓';
        writeMove_PGN_CC__(moveStr, colNum, getNext(move));
    }
}

static void writeRemark_PGN_CC__(wchar_t** pstr, size_t* psize, CMove move)
{
    const wchar_t* remark = getRemark(move);
    if (remark != NULL) {
        size_t len = wcslen(remark) + 32;
        wchar_t remarkStr[len];
        swprintf(remarkStr, len, L"(%d,%d): {%ls}\n", getNextNo(move), getCC_ColNo(move), remark);
        supper_wcscat(pstr, psize, remarkStr);
    }

    if (getOther(move))
        writeRemark_PGN_CC__(pstr, psize, getOther(move));
    if (getNext(move))
        writeRemark_PGN_CC__(pstr, psize, getNext(move));
}

void writeInfo_PGNtoWstr(wchar_t** pinfoStr, ChessManual cm)
{
    size_t size = WIDEWCHARSIZE;
    *pinfoStr = calloc(size, sizeof(wchar_t));
    assert(*pinfoStr);

    wchar_t tmpWstr[WIDEWCHARSIZE];
    for (int i = 0; i < cm->infoCount; ++i) {
        swprintf(tmpWstr, WIDEWCHARSIZE, L"[%ls \"%ls\"]\n", cm->info[i][0], cm->info[i][1]);
        supper_wcscat(pinfoStr, &size, tmpWstr);
    }
}

void writeMove_PGN_CCtoWstr(wchar_t** pmoveStr, ChessManual cm)
{
    int rowNum = (cm->maxRow_ + 1) * 2,
        colNum = (cm->maxCol_ + 1) * 5 + 1,
        size = rowNum * colNum;
    wchar_t* moveStr = malloc((size + 1) * sizeof(wchar_t));
    assert(moveStr);
    wmemset(moveStr, L'　', size);
    moveStr[size] = L'\x0';
    for (int row = 0; row < rowNum; ++row) {
        moveStr[(row + 1) * colNum - 1] = L'\n';
        //if (row % 2 == 1)
        //  moveStr[row * colNum] = L' '; // 为显示美观, 奇数行改为半角空格
    }
    moveStr[1] = L'开';
    moveStr[2] = L'始';
    moveStr[colNum + 2] = L'↓';
    if (getNext(cm->rootMove))
        writeMove_PGN_CC__(moveStr, colNum, getNext(cm->rootMove));

    *pmoveStr = moveStr;
}

void writeRemark_PGN_CCtoWstr(wchar_t** premStr, ChessManual cm)
{
    size_t size = WIDEWCHARSIZE;
    *premStr = malloc(size * sizeof(wchar_t));
    assert(*premStr);
    (*premStr)[0] = L'\x0';
    writeRemark_PGN_CC__(premStr, &size, cm->rootMove);
}

void writeMoveRemark_PGN_ICCSZHtoWstr(wchar_t** pmoveStr, ChessManual cm, RecFormat fmt)
{
    size_t size = WIDEWCHARSIZE;
    *pmoveStr = malloc(size * sizeof(wchar_t));
    assert(*pmoveStr);
    (*pmoveStr)[0] = L'\x0';
    writeRemark_PGN_ICCSZH__s(pmoveStr, &size, cm->rootMove);
    if (getNext(cm->rootMove))
        writeMove_PGN_ICCSZH__s(pmoveStr, &size, getNext(cm->rootMove), fmt == PGN_ZH, false);
    supper_wcscat(pmoveStr, &size, L"\n");
}

void writePGNtoWstr(wchar_t** pstr, ChessManual cm, RecFormat fmt)
{
    wchar_t *infoStr = NULL, *moveStr = NULL, *remarkStr = NULL;
    writeInfo_PGNtoWstr(&infoStr, cm);
    if (fmt == PGN_CC) {
        writeMove_PGN_CCtoWstr(&moveStr, cm);
        writeRemark_PGN_CCtoWstr(&remarkStr, cm);
    } else {
        writeMoveRemark_PGN_ICCSZHtoWstr(&moveStr, cm, fmt);
        remarkStr = L"";
    }

    int len = wcslen(infoStr) + wcslen(moveStr) + wcslen(remarkStr) + 3; // 两个回车，一个结束符号
    *pstr = malloc(len * sizeof(wchar_t));
    assert(*pstr);
    swprintf(*pstr, len, L"%ls\n%ls\n%ls", infoStr, moveStr, remarkStr);

    free(infoStr);
    free(moveStr);
    if (fmt == PGN_CC)
        free(remarkStr);
}

static void writePGN__(FILE* fout, ChessManual cm, RecFormat fmt)
{
    wchar_t* pstr = NULL;
    writePGNtoWstr(&pstr, cm, fmt);
    fwprintf(fout, L"%ls", pstr);
    free(pstr);
}

void readChessManual__(ChessManual cm, const char* fileName)
{
    if (!fileIsRight__(fileName)) {
        setBoard_FEN(cm->board, getFENFromCM__(cm));
        return;
    }

    RecFormat fmt = getRecFormat__(getExtName(fileName));
    FILE* fin = fopen(fileName, (fmt == XQF || fmt == BIN || fmt == JSON) ? "rb" : "r");
    if (fin == NULL)
        return;
    switch (fmt) {
    case XQF:
        readXQF__(cm, fin);
        break;
    case BIN:
        readBin__(cm, fin);
        break;
    case JSON:
        readJSON__(cm, fin);
        break;
    default:
        readPGN__(cm, fin, fmt);
        break;
    }
    if (getNext(cm->rootMove))
        setMoveNumZhStr(cm); // 驱动函数
    fclose(fin);
}

void writeChessManual(ChessManual cm, const char* fileName)
{
    if (!fileIsRight__(fileName))
        return;

    RecFormat fmt = getRecFormat__(getExtName(fileName));
    FILE* fout = fopen(fileName,
        (fmt == XQF || fmt == BIN || fmt == JSON) ? "wb" : "w");
    if (fout == NULL)
        return;
    switch (fmt) {
    case XQF:
        wprintf(L"未实现的写入文件扩展名！");
        break;
    case BIN:
        writeBIN__(fout, cm);
        break;
    case JSON:
        writeJSON__(fout, cm);
        break;
    default:
        writePGN__(fout, cm, fmt);
        break;
    }
    fclose(fout);
}

static void moveMap__(Move move, Board board, void apply(Move, Board, void*), void* ptr)
{
    if (move == NULL)
        return;
    apply(move, board, ptr);

    doMove(move);
    moveMap__(getNext(move), board, apply, ptr);
    undoMove(move);

    moveMap__(getOther(move), board, apply, ptr);
}

void moveMap(ChessManual cm, void apply(Move, Board, void*), void* ptr)
{
    moveMap__(getNext(cm->rootMove), cm->board, apply, ptr);
}

void changeChessManual(ChessManual cm, ChangeType ct)
{
    // info未更改
    changeBoard(cm->board, ct);
    if (getNext(cm->rootMove)) {
        if (ct != EXCHANGE)
            changeMove(getNext(cm->rootMove), cm->board, ct);
        if (ct == EXCHANGE || ct == SYMMETRY_H || ct == SYMMETRY_V)
            setMoveNumZhStr(cm);
    }
}

static void transFile__(FileInfo fileInfo, OperateDirData odata)
{
    const char* fileName = fileInfo->name;
    if (!fileIsRight__(fileName))
        return;
    //printf("line:%d %s\n", __LINE__, fileName);

    //OperateDirData odata = (OperateDirData)ptr;
    ChessManual cm = newChessManual(fileName);
    //printf("\nline:%d %s", __LINE__, fileName);

    char toDirName[FILENAME_MAX], toFileName[FILENAME_MAX];
    //替换源目录名
    snprintf(toFileName, FILENAME_MAX, "%s/%s", odata->toDir, fileName + strlen(odata->fromDir));
    getDirName(toDirName, toFileName);
    transFileExtName(toFileName, EXTNAMES[odata->tofmt]);
    //printf("%d: %s\n", __LINE__, toFileName);

    // 检查并创建(多级)目录
    char tmpDirName[FILENAME_MAX] = { 0 }, *dname, tokseps[] = "\\/";
    dname = strtok(toDirName, tokseps);
    while (dname) {
        strcat(tmpDirName, dname);
        if (access(tmpDirName, 0) != 0) {
            makeDir(tmpDirName);
            odata->dcount++;
            //printf("%d: makeDir-> %s\n", __LINE__, toDirName);
        }
        strcat(tmpDirName, "/");
        dname = strtok(NULL, tokseps);
    }

    writeChessManual(cm, toFileName);
    //printf("\nline:%d %s\n", __LINE__, toFileName);

    ++odata->fcount;
    odata->movCount += cm->movCount_;
    odata->remCount += cm->remCount_;
    if (odata->remLenMax < cm->maxRemLen_)
        odata->remLenMax = cm->maxRemLen_;
    delChessManual(cm);
}

void transDir(const char* dirName, RecFormat fromfmt, RecFormat tofmt, bool isPrint)
{
    char fromDir[FILENAME_MAX], toDir[FILENAME_MAX];
    sprintf(fromDir, "%s%s", dirName, EXTNAMES[fromfmt]);
    sprintf(toDir, "%s%s", dirName, EXTNAMES[tofmt]);

    OperateDirData odata = malloc(sizeof(struct OperateDirData));
    odata->fcount = odata->dcount = odata->movCount = odata->remCount = odata->remLenMax = 0;
    odata->fromDir = fromDir;
    odata->toDir = toDir;
    odata->fromfmt = fromfmt;
    odata->tofmt = tofmt;
    if (strlen(toDir) > 0 && access(toDir, 0) != 0) {
        makeDir(toDir);
        //printf("\nline:%d %s\n", __LINE__, toDir);
    }
    //printf("\nline:%d %s -> %s", __LINE__, fromDir, toDir);

    operateDir(fromDir, (void (*)(void*, void*))transFile__, odata, true);

    if (isPrint)
        printf("\n%s =>%s: %d files, %d dirs.\n   movCount: %d, remCount: %d, remLenMax: %d",
            fromDir, toDir, odata->fcount, odata->dcount, odata->movCount, odata->remCount, odata->remLenMax);

    free(odata);
}

void getChessManualNumStr(char* str, ChessManual cm)
{
    snprintf(str, WIDEWCHARSIZE, "%s: movCount:%d remCount:%d remLenMax:%d maxRow:%d maxCol:%d\n",
        __func__, cm->movCount_, cm->remCount_, cm->maxRemLen_, cm->maxRow_, cm->maxCol_);
}

const wchar_t* getIccsStr(wchar_t* wIccsStr, ChessManual cm)
{
    wchar_t iccs[6], redStr[WCHARSIZE] = { 0 }, blackStr[WCHARSIZE] = { 0 };
    backFirst(cm);
    while (go(cm)) {
        wcscat(redStr, getICCS_cm(iccs, cm, EXCHANGE));
        if (go(cm))
            wcscat(blackStr, getICCS_cm(iccs, cm, EXCHANGE));
    }
    backFirst(cm);
    swprintf(wIccsStr, WIDEWCHARSIZE, L"%ls&%ls", redStr, blackStr);
    return wIccsStr;
}

const char* getIccsStr_c(char* iccsStr, ChessManual cm)
{
    wchar_t wIccsStr[WIDEWCHARSIZE] = { 0 };
    getIccsStr(wIccsStr, cm);
    wcstombs(iccsStr, wIccsStr, wcstombs(NULL, wIccsStr, 0) + 1);
    return iccsStr;
}

bool chessManual_equal(ChessManual cm0, ChessManual cm1)
{
    if (cm0 == NULL && cm1 == NULL)
        return true;
    // 其中有一个为空指针
    if (!(cm0 && cm1)) {
        //printf("\n!(cm0 && cm1)\n%d %s", __LINE__, __FILE__);
        return false;
    }

    if (!(cm0->infoCount == cm1->infoCount
            && cm0->movCount_ == cm1->movCount_
            && cm0->remCount_ == cm1->remCount_
            && cm0->maxRemLen_ == cm1->maxRemLen_
            && cm0->maxRow_ == cm1->maxRow_
            && cm0->maxCol_ == cm1->maxCol_)) {
        //printf("\n!(cm0 && cm1)\n%d %s", __LINE__, __FILE__);
        return false;
    }

    for (int i = 0; i < cm0->infoCount; ++i) {
        for (int j = 0; j < 2; ++j)
            if (wcscmp(cm0->info[i][j], cm1->info[i][j]) != 0) {
                //printf("\n!(cm0 && cm1)\n%d %s", __LINE__, __FILE__);
                return false;
            }
    }

    if (!board_equal(cm0->board, cm1->board)) {
        //printf("\n!(cm0 && cm1)\n%d %s", __LINE__, __FILE__);
        return false;
    }

    if (!rootmove_equal(cm0->rootMove, cm1->rootMove)) {
        //printf("\n!(cm0 && cm1)\n%d %s", __LINE__, __FILE__);
        return false;
    }

    return true;
}

ChessManualRec newChessManualRec(ChessManualRec preChessManualRec, const char* fileName)
{
    ChessManualRec cmr = malloc(sizeof(struct ChessManualRec));
    cmr->cm = newChessManual(fileName);
    cmr->ncmr = NULL;
    if (preChessManualRec)
        preChessManualRec->ncmr = cmr;
    return cmr;
}

void delChessManualRec(ChessManualRec cmr)
{
    if (cmr == NULL)
        return;

    ChessManualRec ncmr = cmr->ncmr;
    delChessManual(cmr->cm);
    free(cmr);

    delChessManualRec(ncmr);
}

ChessManualRec getNextCMR(ChessManualRec cmr) { return cmr->ncmr; }

ChessManual getNextCM(ChessManualRec cmr) { return cmr->cm; }

static void readFileToChessManualRec__(FileInfo fileInfo, ChessManualRec* cpcmr)
{
    const char* fileName = fileInfo->name;
    if (!fileIsRight__(fileName))
        return;

    *cpcmr = newChessManualRec(*cpcmr, fileName);
}

ChessManualRec readDirToChessManualRec(const char* dirName, RecFormat fromfmt)
{
    char fromDir[FILENAME_MAX];
    sprintf(fromDir, "%s%s", dirName, EXTNAMES[fromfmt]);

    ChessManualRec cmr = newChessManualRec(NULL, NULL),
                   rcmr = cmr;
    operateDir(fromDir, (void (*)(void*, void*))readFileToChessManualRec__, &cmr, true);
    return rcmr;
}

void printChessManualRec(FILE* fout, ChessManualRec rcmr)
{
    int no = 0;
    ChessManualRec cmr = rcmr;
    fwprintf(fout, L"rcmr:\n");
    while ((cmr = cmr->ncmr)) {

        wchar_t ecco_sn[4] = { 0 }, fileName[WIDEWCHARSIZE], wIccsStr[WIDEWCHARSIZE];
        mbstowcs(fileName, cmr->cm->fileName, WIDEWCHARSIZE - 1);
        getIccsStr(wIccsStr, cmr->cm);
        fwprintf(fout, L"\nNo.%d sn:%ls file:%ls\niccses:%ls\n",
            no++, ecco_sn, fileName, wIccsStr);

        wchar_t* wstr = NULL;
        writePGNtoWstr(&wstr, cmr->cm, PGN_ZH);
        //fwprintf(fout, L"PGN_CC: %ls\n", wstr);
        free(wstr);
    }
}

static int addCM_Item__(FileInfo fileInfo, LinkedItem* ppreItem)
{
    const char* fileName = fileInfo->name;
    if (!fileIsRight__(fileName))
        return -1;

    ChessManual cm = newChessManual(fileName);
    *(LinkedItem*)ppreItem = newLinkedItem(*(LinkedItem*)ppreItem, cm);
    return 0;
}

LinkedItem getRootCM_LinkedItem(const char* dirName, RecFormat fromfmt)
{
    char fromDir[FILENAME_MAX];
    sprintf(fromDir, "%s%s", dirName, EXTNAMES[fromfmt]);
    LinkedItem rootCM_LinkedItem = newLinkedItem(NULL, NULL),
               preCM_LinkedItem = rootCM_LinkedItem;
    operateDir(fromDir, (void (*)(void*, void*))addCM_Item__, &preCM_LinkedItem, true);
    return rootCM_LinkedItem;
}

void delRootCM_LinkedItem(LinkedItem rootCM_item)
{
    delLinkedItem(rootCM_item, (void (*)(void*))delChessManual);
}

static void printCM_Str__(ChessManual cm, FILE* fout, int* no, bool* onward)
{
    wchar_t ecco_sn[4] = { 0 }, fileName[WIDEWCHARSIZE], wIccsStr[WIDEWCHARSIZE];
    mbstowcs(fileName, cm->fileName, WIDEWCHARSIZE - 1);
    getIccsStr(wIccsStr, cm);
    fwprintf(fout, L"\nNo.%d sn:%ls file:%ls\niccses:%ls\n",
        (*no)++, ecco_sn, fileName, wIccsStr);
}

void printCM_LinkedItem(FILE* fout, LinkedItem rootCM_item)
{
    bool onward = true;
    int no = 0;
    traverseLinkedItem(rootCM_item, (void (*)(void*, void*, void*, bool*))printCM_Str__, fout, &no, &onward);
}