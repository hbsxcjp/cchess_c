#define PCRE_STATIC
#include "head/chessManual.h"
#include "head/aspect.h"
#include "head/board.h"
#include "head/cJSON.h"
#include "head/move.h"
#include "head/piece.h"
//#include <regex.h>
//#include <sys/types.h>

struct ChessManual {
    Board board;
    Move rootMove, curMove;
    MyLinkedList infoMyLinkedList;
    int movCount_, remCount_, maxRemLen_, maxRow_, maxCol_;
};

typedef struct OperateDirData {
    int fcount, dcount, movCount, remCount, remLenMax;
    const char *fromDir, *toDir;
    RecFormat fromfmt, tofmt;
} * OperateDirData;

const char* EXTNAMES[] = {
    ".xqf", ".bin", ".json", ".pgn_iccs", ".pgn_zh", ".pgn_cc"
};

static const wchar_t* CMINFO_NAMES[] = {
    L"TITLE", L"EVENT", L"DATE", L"SITE", L"BLACK", L"RED",
    L"OPENING", L"WRITER", L"AUTHOR", L"TYPE", L"RESULT", L"VERSION",
    L"SOURCE", L"FEN", L"ICCSSTR", L"ECCO_SN", L"ECCO_NAME", L"MOVESTR"
}; // 所有字段构成创建表的SQL语句长度不能超过256字符

typedef enum {
    TITLE_INDEX,
    EVENT_INDEX,
    DATE_INDEX,
    SITE_INDEX,
    BLACK_INDEX,
    RED_INDEX,
    OPENING_INDEX,
    WRITER_INDEX,
    AUTHOR_INDEX,
    TYPE_INDEX,
    RESULT_INDEX,
    VERSION_INDEX,
    SOURCE_INDEX,
    FEN_INDEX,
    ICCSSTR_INDEX,
    ECCOSN_INDEX,
    ECCONAME_INDEX,
    MOVESTR_INDEX
} CMINFO_INDEX;
static const int CMINFO_LEN = sizeof(CMINFO_NAMES) / sizeof(CMINFO_NAMES[0]);

static const char FILETAG[] = "learnchess";

// 着法相关的字符数组静态全局变量
static const wchar_t* PRECHAR = L"前中后";
static const wchar_t* MOVCHAR = L"退平进";
static const wchar_t* NUMWCHAR[PIECECOLORNUM] = { L"一二三四五六七八九", L"１２３４５６７８９" };
static const wchar_t* FEN_INITVALUE = L"rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR";

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

static const wchar_t* getFENFromCM__(ChessManual cm)
{
    const wchar_t* fen = CMINFO_NAMES[FEN_INDEX];
    const wchar_t* value = getInfoValue_name_cm(cm, fen);
    if (wcslen(value) > 0)
        return value;

    setInfoItem_cm(cm, fen, FEN_INITVALUE);
    return FEN_INITVALUE;
}

ChessManual newChessManual(void)
{
    ChessManual cm = malloc(sizeof(struct ChessManual));
    assert(cm);
    cm->board = newBoard();
    cm->rootMove = newMove();
    cm->curMove = cm->rootMove;
    cm->infoMyLinkedList = newMyLinkedList((void (*)(void*))delInfo);
    for (int i = 0; i < CMINFO_LEN; ++i)
        setInfoItem_cm(cm, CMINFO_NAMES[i], L"");
    cm->movCount_ = cm->remCount_ = cm->maxRemLen_ = cm->maxRow_ = cm->maxCol_ = 0;
    setBoard_FEN(cm->board, getFENFromCM__(cm));

    return cm;
}

ChessManual getChessManual_file(const char* fileName)
{
    ChessManual cm = newChessManual();
    readChessManual__(cm, fileName);
    return cm;
}

ChessManual resetChessManual(ChessManual* cm, const char* fileName)
{
    delChessManual(*cm);
    return *cm = getChessManual_file(fileName);
}

void delChessManual(ChessManual cm)
{
    if (cm == NULL)
        return;

    delMyLinkedList(cm->infoMyLinkedList);
    delMove(cm->rootMove);
    delBoard(cm->board);
    free(cm);
}

Move getRootMove(ChessManual cm) { return cm->rootMove; }

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
    if (isOther) {
        undoMove(cm->curMove);
    }
    Move move = addMove(cm->curMove, cm->board, wstr, fmt, remark, isOther);
    if (move == NULL) {
        if (isOther) {
            doMove(cm->curMove);
        }
        //printBoard(cm->board, fmt, isOther, wstr);
        return NULL;
    }
    assert(move != NULL);
    go__(cm, move);
    return move;
}

const wchar_t* getCurMoveICCS_cm(wchar_t* iccs, ChessManual cm, ChangeType ct)
{
    return getICCS_mt(iccs, cm->curMove, cm->board, ct);
}

void setInfoItem_cm(ChessManual cm, const wchar_t* name, const wchar_t* value)
{
    setInfoItem(cm->infoMyLinkedList, name, value);
}

const wchar_t* getInfoValue_name_cm(ChessManual cm, const wchar_t* name)
{
    return getInfoValue_name(cm->infoMyLinkedList, name);
}

void delInfoItem_cm(ChessManual cm, const wchar_t* name)
{
    delInfoItem(cm->infoMyLinkedList, name);
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
            mbstowcs_gbk(*remark, (char*)rem);
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
        PlayStepNo[2] = { 0 },
                  //headWhoPlay,
        headPlayResult, PlayNodes[4] = { 0 }, PTreePos[4] = { 0 }, Reserved1[4] = { 0 },
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
    //headWhoPlay = xqfData[50];
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
    char* values[] = { TitleA, Event, Date, Site, Red, Black, Opening, RMKWriter, Author };
    for (int i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
        // "TitleA", "Event", "Date", "Site", "Red", "Black", "Opening", "RMKWriter", "Author"
        // "标题: 赛事: 日期: 地点: 红方: 黑方: 结果: 评论: 作者: "
        mbstowcs_gbk(tempStr, values[i]);
        setInfoItem_cm(cm, CMINFO_NAMES[i], tempStr);
    }
    wchar_t* PlayType[] = { L"全局", L"开局", L"中局", L"残局" };
    setInfoItem_cm(cm, CMINFO_NAMES[TYPE_INDEX], PlayType[headCodeA_H[0]]); // 编码定义存储

    getFEN_pieChars(tempStr, pieChars);
    //wcscat(tempStr, headWhoPlay ? L" -r" : L" -b");
    setInfoItem_cm(cm, CMINFO_NAMES[FEN_INDEX], tempStr); // 转换FEN存储

    wchar_t* Result[] = { L"未知", L"红胜", L"黑胜", L"和棋" };
    setInfoItem_cm(cm, CMINFO_NAMES[RESULT_INDEX], Result[headPlayResult]); // 编码定义存储

    swprintf(tempStr, WIDEWCHARSIZE, L"%d", Version);
    setInfoItem_cm(cm, CMINFO_NAMES[VERSION_INDEX], tempStr); // 整数存储

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
            setInfoItem_cm(cm, name, value);
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

static void writeInfo_BIN__(Info info, FILE* fout, void* _0, void* _1)
{
    writeWstring_BIN__(fout, getInfoName(info));
    writeWstring_BIN__(fout, getInfoValue(info));
}

static void writeBIN__(FILE* fout, ChessManual cm)
{
    fwrite(FILETAG, sizeof(char), sizeof(FILETAG), fout);
    char infoCount = myLinkedList_size(cm->infoMyLinkedList);

    char tag = ((infoCount > 0 ? 0x10 : 0x00)
        | (getRemark(cm->rootMove) ? 0x20 : 0x00)
        | (getNext(cm->rootMove) ? 0x80 : 0x00));
    fwrite(&tag, sizeof(char), 1, fout);
    if (tag & 0x10) {
        fwrite(&infoCount, sizeof(char), 1, fout);
        traverseMyLinkedList(cm->infoMyLinkedList, (void (*)(void*, void*, void*, void*))writeInfo_BIN__,
            fout, NULL, NULL);
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
        setInfoItem_cm(cm, nameValue[0], nameValue[1]);
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

static void writeInfo_JSON__(Info info, cJSON* infoJSON, void* _0, void* _1)
{
    size_t namelen = wcstombs(NULL, getInfoName(info), 0) + 1,
           valuelen = wcstombs(NULL, getInfoValue(info), 0) + 1;
    char name[namelen], value[valuelen];
    wcstombs(name, getInfoName(info), namelen);
    wcstombs(value, getInfoValue(info), valuelen);
    cJSON_AddItemToArray(infoJSON,
        cJSON_CreateStringArray((const char* const[]) { name, value }, 2));
}

static void writeJSON__(FILE* fout, ChessManual cm)
{
    cJSON *manualJSON = cJSON_CreateObject(),
          *infoJSON = cJSON_CreateArray(),
          *rootmoveJSON = cJSON_CreateObject();

    traverseMyLinkedList(cm->infoMyLinkedList, (void (*)(void*, void*, void*, void*))writeInfo_JSON__,
        infoJSON, NULL, NULL);
    cJSON_AddItemToObject(manualJSON, "info", infoJSON);

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

static wchar_t* readInfo_PGN__(ChessManual cm, wchar_t* fileWstring)
{
    const char* error;
    int erroffset = 0, infoCount = 0, ovector[PCREARRAY_SIZE];
    const wchar_t* infoPat = L"\\[(\\w+)\\s+\"([\\s\\S]*?)\"\\]";
    void* reg = pcrewch_compile(infoPat, 0, &error, &erroffset, NULL);
    wchar_t* tempWstr = fileWstring;
    while ((infoCount = pcrewch_exec(reg, NULL, tempWstr, wcslen(tempWstr), 0, 0, ovector, PCREARRAY_SIZE)) > 0) {
        wchar_t name[WCHARSIZE], value[WIDEWCHARSIZE];
        pcrewch_copy_substring(tempWstr, ovector, infoCount, 1, name, WIDEWCHARSIZE);
        pcrewch_copy_substring(tempWstr, ovector, infoCount, 2, value, WIDEWCHARSIZE);
        setInfoItem_cm(cm, name, value);

        tempWstr += ovector[1];
    }
    pcrewch_free(reg);

    return tempWstr;
}

wchar_t* getZhWChars(wchar_t* ZhWChars)
{
    swprintf(ZhWChars, WCHARSIZE, L"%ls%ls%ls%ls%ls",
        PRECHAR, getPieceNames(), MOVCHAR, NUMWCHAR[RED], NUMWCHAR[BLACK]);
    return ZhWChars;
}

static void readMove_PGN_ICCSZH__(ChessManual cm, wchar_t* moveWstring, RecFormat fmt)
{
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

    int ovector[PCREARRAY_SIZE] = { 0 },
        regCount = pcrewch_exec(remReg, NULL, moveWstring, wcslen(moveWstring), 0, 0, ovector, PCREARRAY_SIZE);
    if (regCount > 0 && ovector[2] < ovector[3])
        setRemark(cm->rootMove, getSubStr(moveWstring, ovector[2], ovector[3])); // 赋值一个动态分配内存的指针

    Move preOtherMoves[WIDEWCHARSIZE] = { NULL };
    int preOthIndex = 0, length = 0;
    //printf("读取moveStr... \n");
    wchar_t* tempMoveStr = moveWstring;
    while ((tempMoveStr += ovector[1]) && (length = wcslen(tempMoveStr)) > 0) {
        regCount = pcrewch_exec(moveReg, NULL, tempMoveStr, length, 0, 0, ovector, PCREARRAY_SIZE);
        if (regCount <= 0)
            break;

        // 是否有"("
        bool isOther = ovector[3] > ovector[2];
        if (isOther)
            preOtherMoves[preOthIndex++] = cm->curMove;

        // 提取字符串
        wchar_t iccs_zhStr[6];
        pcrewch_copy_substring(tempMoveStr, ovector, regCount, 2, iccs_zhStr, 6);
        appendMove(cm, iccs_zhStr, fmt, getSubStr(tempMoveStr, ovector[6], ovector[7]), isOther);

        // 是否有一个以上的")"
        int num = ovector[9] - ovector[8];
        while (num--)
            backTo(cm, preOtherMoves[--preOthIndex]);
    }
    backFirst(cm);

    pcrewch_free(remReg);
    pcrewch_free(moveReg);
}

static wchar_t* getRemark_PGN_CC__(MyLinkedList mvstrMyLinkedList, int row, int col)
{
    wchar_t name[WCHARSIZE];
    swprintf(name, WCHARSIZE, L"(%d,%d)", row, col);
    const wchar_t* value = getInfoValue_name(mvstrMyLinkedList, name);
    if (value)
        return getSubStr(value, 0, wcslen(value));

    return NULL;
}

static void addMove_PGN_CC__(ChessManual cm, MyLinkedList mvstrMyLinkedList,
    int rowNum, int row, int col, bool isOther)
{
    wchar_t name[WCHARSIZE] = { 0 };
    swprintf(name, WCHARSIZE, L"%d,%d", row, col);
    const wchar_t* mvstr = getInfoValue_name(mvstrMyLinkedList, name);
    if (mvstr == NULL || mvstr[0] == L'　')
        return;

    while (mvstr[0] == L'…') {
        swprintf(name, WCHARSIZE, L"%d,%d", row, ++col);
        mvstr = getInfoValue_name(mvstrMyLinkedList, name);
    }
    assert(mvstr);

    wchar_t zhStr[MOVESTR_LEN + 1];
    copySubStr(zhStr, mvstr, 0, MOVESTR_LEN);
    Move move = appendMove(cm, zhStr, PGN_CC, getRemark_PGN_CC__(mvstrMyLinkedList, row, col), isOther);

    if (mvstr[MOVESTR_LEN] == L'…')
        addMove_PGN_CC__(cm, mvstrMyLinkedList, rowNum, row, col + 1, true);

    backTo(cm, move);
    if (getNextNo(move) < rowNum - 1)
        addMove_PGN_CC__(cm, mvstrMyLinkedList, rowNum, row + 1, col, false);
}

static void readMove_PGN_CC__(ChessManual cm, wchar_t* moveWstring)
{
    if (wcslen(moveWstring) < 5)
        return;

    // 读取着法字符串
    MyLinkedList mvstrMyLinkedList = newMyLinkedList((void (*)(void*))delInfo);
    wchar_t name[WCHARSIZE], value[WCHARSIZE],
        *moveWstrEnd = moveWstring + (wcslen(moveWstring) - 1),
        *lineStart = wcschr(moveWstring + 1, L'\n'),
        *lineEnd = wcschr(moveWstring, L'(') ? wcschr(moveWstring, L'(') : moveWstrEnd;
    int rowIndex = 0,
        wchNum = wcschr(lineStart + 1, L'\n') - lineStart - 1,
        colNum = wchNum / (MOVESTR_LEN + 1);
    //fwprintf(fout, L"\nline:%d r:%d w:%d c:%d\n", __LINE__, rowIndex, wchNum, colNum);
    while (lineStart && (++lineStart)[0] && lineStart + wchNum <= lineEnd) {
        for (int col = 0; col < colNum; ++col) {
            int subStart = col * (MOVESTR_LEN + 1);
            swprintf(name, WCHARSIZE, L"%d,%d", rowIndex, col);
            copySubStr(value, lineStart, subStart, subStart + MOVESTR_LEN + 1);
            addMyLinkedList(mvstrMyLinkedList, newInfo(name, value));
        }
        ++rowIndex;
        lineStart = wcschr(lineStart, L'\n'); // 弃掉间隔行
        lineStart = wcschr(lineStart + 1, L'\n');
    }

    // 读取注解字符串
    const char* error;
    int erroffset = 0, ovector[PCREARRAY_SIZE] = { 0 };
    void *moveReg = pcrewch_compile(L"([^…　]{4}[…　])",
             0, &error, &erroffset, NULL),
         *remReg = pcrewch_compile(L"(\\(\\d+,\\d+\\)): \\{([\\s\\S]*?)\\}",
             0, &error, &erroffset, NULL);
    while (lineEnd && wcslen(lineEnd) > 0) {
        int remCount = pcrewch_exec(remReg, NULL, lineEnd, wcslen(lineEnd), 0, 0, ovector, PCREARRAY_SIZE);
        if (remCount <= 0)
            break;

        pcrewch_copy_substring(lineEnd, ovector, remCount, 1, name, WIDEWCHARSIZE);
        pcrewch_copy_substring(lineEnd, ovector, remCount, 2, value, WIDEWCHARSIZE);
        addMyLinkedList(mvstrMyLinkedList, newInfo(name, value));
        lineEnd += ovector[1];
    }

    setRemark(cm->rootMove, getRemark_PGN_CC__(mvstrMyLinkedList, 0, 0));
    if (rowIndex > 0)
        addMove_PGN_CC__(cm, mvstrMyLinkedList, rowIndex, 1, 0, false);
    backFirst(cm);

    //traverseMyLinkedList(mvstrMyLinkedList, (void (*)(void*, void*, void*, void*))printInfo, fout, NULL, NULL);
    //printInfoMyLinkedList(fout, mvstrMyLinkedList);

    pcrewch_free(remReg);
    pcrewch_free(moveReg);
    delMyLinkedList(mvstrMyLinkedList);
}

static void readPGN__(ChessManual cm, FILE* fin, RecFormat fmt)
{
    wchar_t *fileWstring = getWString(fin),
            *moveWstring = readInfo_PGN__(cm, fileWstring);
    //fwprintf(fout, moveWstring);

    // PGN_ZH, PGN_CC在读取move之前需要先设置board
    setBoard_FEN(cm->board, getFENFromCM__(cm));

    if (fmt == PGN_CC)
        readMove_PGN_CC__(cm, moveWstring);
    else
        readMove_PGN_ICCSZH__(cm, moveWstring, fmt);

    free(fileWstring);
}

static void writeRemark_PGN_ICCSZH__(wchar_t** pmoveStr, size_t* psize, CMove move)
{
    const wchar_t* remark = getRemark(move);
    if (remark && wcslen(remark) > 0) {
        wchar_t remarkstr[WIDEWCHARSIZE];
        swprintf(remarkstr, WIDEWCHARSIZE, L" \n{%ls}\n ", remark);
        supper_wcscat(pmoveStr, psize, remarkstr);
    }
}

static void writeMove_PGN_ICCSZH__(wchar_t** pmoveStr, size_t* psize, Move move, bool isPGN_ZH, bool isOther)
{
    wchar_t boutNum[WCHARSIZE], boutStr[WCHARSIZE], iccs[6] = { 0 };
    swprintf(boutNum, WCHARSIZE, L" %d. ", (getNextNo(move) + 1) / 2);
    bool isEven = getNextNo(move) % 2 == 0;
    swprintf(boutStr, WCHARSIZE, L"%ls%ls%ls%ls",
        (isOther ? L"(" : L""),
        (isOther || !isEven ? boutNum : L" "),
        (isOther && isEven ? L"... " : L""),
        (isPGN_ZH ? getZhStr(move) : getICCS_m(iccs, move)));
    supper_wcscat(pmoveStr, psize, boutStr);
    writeRemark_PGN_ICCSZH__(pmoveStr, psize, move);

    if (getOther(move)) {
        writeMove_PGN_ICCSZH__(pmoveStr, psize, getOther(move), isPGN_ZH, true);
        supper_wcscat(pmoveStr, psize, L")");
    }
    if (getNext(move))
        writeMove_PGN_ICCSZH__(pmoveStr, psize, getNext(move), isPGN_ZH, false);
}

static void writeMove_PGN_CC__(wchar_t* moveStr, int colNum, CMove move)
{
    int row = getNextNo(move) * 2, firstCol = getCC_ColNo(move) * 5;
    wcsncpy(moveStr + row * colNum + firstCol, getZhStr(move), 4);

    if (getOther(move)) {
        int fcol = firstCol + 4, tnum = getCC_ColNo(getOther(move)) * 5 - fcol;
        wmemset(moveStr + row * colNum + fcol, L'…', tnum);
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

static void writeInfo_PGN__(Info info, wchar_t** pinfoStr, size_t* size, void* _)
{
    int len = wcslen(getInfoName(info)) + wcslen(getInfoValue(info)) + WCHARSIZE;
    wchar_t tmpWstr[len];
    swprintf(tmpWstr, len, L"[%ls \"%ls\"]\n", getInfoName(info), getInfoValue(info));
    supper_wcscat(pinfoStr, size, tmpWstr);
}

void writeInfo_PGN(wchar_t** pinfoStr, ChessManual cm)
{
    size_t size = WIDEWCHARSIZE;
    *pinfoStr = calloc(size, sizeof(wchar_t));
    traverseMyLinkedList(cm->infoMyLinkedList, (void (*)(void*, void*, void*, void*))writeInfo_PGN__,
        pinfoStr, &size, NULL);
}

void writeMove_PGN_CC(wchar_t** pmoveStr, ChessManual cm)
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

void writeRemark_PGN_CC(wchar_t** premStr, ChessManual cm)
{
    size_t size = WIDEWCHARSIZE;
    *premStr = calloc(size, sizeof(wchar_t));
    writeRemark_PGN_CC__(premStr, &size, cm->rootMove);
}

void writeMoveRemark_PGN_ICCSZH(wchar_t** pmoveStr, ChessManual cm, RecFormat fmt)
{
    size_t size = WIDEWCHARSIZE;
    *pmoveStr = malloc(size * sizeof(wchar_t));
    assert(*pmoveStr);
    (*pmoveStr)[0] = L'\x0';
    writeRemark_PGN_ICCSZH__(pmoveStr, &size, cm->rootMove);
    if (getNext(cm->rootMove))
        writeMove_PGN_ICCSZH__(pmoveStr, &size, getNext(cm->rootMove), fmt == PGN_ZH, false);
    else
        printf("line:%d file:%s\n", __LINE__, "fileName");

    supper_wcscat(pmoveStr, &size, L"\n");
}

void writePGNtoWstr(wchar_t** pstr, ChessManual cm, RecFormat fmt)
{
    wchar_t *infoStr = NULL, *moveStr = NULL, *remarkStr = NULL;
    writeInfo_PGN(&infoStr, cm);
    if (fmt == PGN_CC) {
        writeMove_PGN_CC(&moveStr, cm);
        writeRemark_PGN_CC(&remarkStr, cm);
    } else {
        writeMoveRemark_PGN_ICCSZH(&moveStr, cm, fmt);
        remarkStr = L"";
    }
    //setInfoItem_cm(cm, CMINFO_NAMES[MOVESTR_INDEX], moveStr);

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

static void readChessManual__(ChessManual cm, const char* fileName)
{
    if (!fileIsRight__(fileName)) {
        return;
    }

    RecFormat fmt = getRecFormat__(getExtName(fileName));
    FILE* fin = ((fmt == XQF || fmt == BIN || fmt == JSON)
            ? fopen(fileName, "rb")
            //: openFile_utf8(fileName, "r"));
            : fopen(fileName, "r"));

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
    else
        printf("line:%d file:%s\n", __LINE__, fileName);

    fclose(fin);

    // 文件名存入信息链表
    wchar_t value[WCHARSIZE];
    mbstowcs(value, fileName, WCHARSIZE);
    setInfoItem_cm(cm, CMINFO_NAMES[SOURCE_INDEX], value);

    // 着法存入信息链表
    if (getNext(cm->rootMove)) {
        wchar_t* movestr = NULL;
        writeMoveRemark_PGN_ICCSZH(&movestr, cm, PGN_ZH);
        setInfoItem_cm(cm, CMINFO_NAMES[MOVESTR_INDEX], movestr);
        free(movestr);
    }
    //*/
}

void writeChessManual(ChessManual cm, const char* fileName)
{
    if (!fileIsRight__(fileName))
        return;

    RecFormat fmt = getRecFormat__(getExtName(fileName));
    FILE* fout = ((fmt == XQF || fmt == BIN || fmt == JSON)
            ? fopen(fileName, "wb")
            //: openFile_utf8(fileName, "w"));
            : fopen(fileName, "w"));
    if (fout == NULL)
        return;

    if (getNext(cm->rootMove) == NULL)
        printf("line:%d file:%s\n", __LINE__, fileName);

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

    ChessManual cm = getChessManual_file(fileName);
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
        wcscat(redStr, getCurMoveICCS_cm(iccs, cm, EXCHANGE));
        if (go(cm))
            wcscat(blackStr, getCurMoveICCS_cm(iccs, cm, EXCHANGE));
    }
    backFirst(cm);
    swprintf(wIccsStr, WIDEWCHARSIZE, L"%ls&%ls", redStr, blackStr);
    return wIccsStr;
}

static int info_cmp__(Info info0, Info info1)
{
    if (info0 == NULL && info1 == NULL)
        return 0;

    // 其中有一个为空指针，不相等
    if (!(info0 && info1))
        return 1;

    // 文件名属性不做比较，视同相等
    if (wcscmp(getInfoName(info0), getInfoName(info1)) == 0
        && wcscmp(getInfoName(info0), CMINFO_NAMES[SOURCE_INDEX]) == 0)
        return 0;

    // 两个属性有一个不相等，则不相等
    if (wcscmp(getInfoName(info0), getInfoName(info1)) != 0 || wcscmp(getInfoValue(info0), getInfoValue(info1)) != 0) {
        int nameLen0 = wcslen(getInfoName(info0)),
            nameLen1 = wcslen(getInfoName(info1)),
            valueLen0 = wcslen(getInfoValue(info0)),
            valueLen1 = wcslen(getInfoValue(info1));
        printf("len:%d %d %d %d\n", nameLen0, nameLen1, valueLen0, valueLen1);
        return 1;
    }

    return 0;
}

bool chessManual_equal(ChessManual cm0, ChessManual cm1)
{
    if (cm0 == NULL && cm1 == NULL)
        return true;
    // 其中有一个为空指针
    if (!(cm0 && cm1)) {
        printf("\n!(cm0 && cm1)\n%d %s", __LINE__, __FILE__);
        return false;
    }

    if (!(cm0->movCount_ == cm1->movCount_
            && cm0->remCount_ == cm1->remCount_
            && cm0->maxRemLen_ == cm1->maxRemLen_
            && cm0->maxRow_ == cm1->maxRow_
            && cm0->maxCol_ == cm1->maxCol_)) {
        printf("\n!(cm0 && cm1)\n%d %s\nmov:%d %d\nrem:%d %d\nmaxRen:%d %d\nmaxRow:%d %d\nmaxCol:%d %d\n",
            __LINE__, __FILE__,
            cm0->movCount_, cm1->movCount_,
            cm0->remCount_, cm1->remCount_,
            cm0->maxRemLen_, cm1->maxRemLen_,
            cm0->maxRow_, cm1->maxRow_,
            cm0->maxCol_, cm1->maxCol_);
        return false;
    }

    //*
    if (!myLinkedList_equal(cm0->infoMyLinkedList, cm1->infoMyLinkedList,
            (int (*)(void*, void*))info_cmp__)) {
        printf("\n!(info0 && info1) %d %s", __LINE__, __FILE__);
        return false;
    }
    //*/

    if (!board_equal(cm0->board, cm1->board)) {
        printf("\n!(cm0 && cm1)\n%d %s", __LINE__, __FILE__);
        return false;
    }

    if (!rootmove_equal(cm0->rootMove, cm1->rootMove)) {
        printf("\n!(cm0 && cm1)\n%d %s", __LINE__, __FILE__);
        return false;
    }

    return true;
}

static void printCmStr__(ChessManual cm, FILE* fout, int* no, void* _)
{
    fwprintf(fout, L"No:%d\n", (*no)++);
    //traverseMyLinkedList(cm->infoMyLinkedList, (void (*)(void*, void*, void*, void*))printInfo,
    //    fout, NULL, NULL);
    printInfoMyLinkedList(fout, cm->infoMyLinkedList);
}

void printCmMyLinkedList(FILE* fout, MyLinkedList cmMyLinkedList)
{
    fwprintf(fout, L"cmMyLinkedList:\n");
    int no = 1;
    traverseMyLinkedList(cmMyLinkedList, (void (*)(void*, void*, void*, void*))printCmStr__,
        fout, &no, NULL);
}

bool setECCO_cm(ChessManual cm, MyLinkedList eccoMyLinkedList)
{
    // 非全局棋谱不设置
    if (wcscmp(getInfoValue_name_cm(cm, CMINFO_NAMES[FEN_INDEX]), FEN_INITVALUE) != 0)
        return false;

    wchar_t iccsStr[WIDEWCHARSIZE];
    getIccsStr(iccsStr, cm);
    setInfoItem_cm(cm, CMINFO_NAMES[ICCSSTR_INDEX], iccsStr);

    Ecco ecco = getEcco_iccsStr(eccoMyLinkedList, iccsStr);
    if (ecco == NULL)
        return false;
    else {
        setInfoItem_cm(cm, CMINFO_NAMES[ECCOSN_INDEX], getEccoSn(ecco));
        setInfoItem_cm(cm, CMINFO_NAMES[ECCONAME_INDEX], getEccoName(ecco));
        return true;
    }
}

static void setECCO_cm__(void* cm, void* eccoMyLinkedList, void* _0, void* _1)
{
    setECCO_cm(cm, eccoMyLinkedList);
}

static void addCM_LinkedList__(FileInfo fileInfo, MyLinkedList cmMyLinkedList)
{
    const char* fileName = fileInfo->name;
    if (!fileIsRight__(fileName))
        return;

    addMyLinkedList(cmMyLinkedList, getChessManual_file(fileName));
}

MyLinkedList getCmMyLinkedList_dir(const char* dirName, RecFormat fromfmt, MyLinkedList eccoMyLinkedList)
{
    MyLinkedList cmMyLinkedList = newMyLinkedList((void (*)(void*))delChessManual);

    char fromDir[FILENAME_MAX];
    sprintf(fromDir, "%s%s", dirName, EXTNAMES[fromfmt]);
    operateDir(fromDir, (void (*)(void*, void*))addCM_LinkedList__, cmMyLinkedList, true);

    traverseMyLinkedList(cmMyLinkedList, setECCO_cm__, eccoMyLinkedList, NULL, NULL);

    return cmMyLinkedList;
}

/*
static void setEccoNameStr__(ChessManual cm, sqlite3* db, const char* lib_tblName, void* _)
{
    const wchar_t* ecco_sn = getInfoValue_name_cm(cm, CMINFO_NAMES[ECCOSN_INDEX]);
    if (wcslen(ecco_sn) < 1)
        return;

    wchar_t ecco_name[WCHARSIZE];
    getEccoName(ecco_name, db, lib_tblName, ecco_sn);
    setInfoItem_cm(cm, CMINFO_NAMES[ECCONAME_INDEX], ecco_name);
}
//*/

static MyLinkedList getInfoMyLinkedList_cm__(ChessManual cm)
{
    return cm->infoMyLinkedList;
}

int storeChessManual_dir(const char* dbName, const char* lib_tblName, const char* man_tblName,
    const char* dirName, RecFormat fromfmt)
{
    sqlite3* db = NULL;
    int result = 0, rc = sqlite3_open(dbName, &db);
    if (rc) {
        fprintf(stderr, "\nCan't open database: %s", sqlite3_errmsg(db));
        sqlite3_close(db);
        return result;
    }

    MyLinkedList eccoMyLinkedList = getEccoMyLinkedList(db, lib_tblName);
    MyLinkedList cmMyLinkedList = getCmMyLinkedList_dir(dirName, fromfmt, eccoMyLinkedList);

    /*
        traverseMyLinkedList(cmMyLinkedList, (void (*)(void*, void*, void*, void*))setEccoNameStr__,
            db, (void*)lib_tblName, NULL);

        //*/
    result = storeObjMyLinkedList(db, man_tblName, cmMyLinkedList, (MyLinkedList(*)(void*))getInfoMyLinkedList_cm__);

    printCmMyLinkedList(fout, cmMyLinkedList);
    delMyLinkedList(cmMyLinkedList);
    delMyLinkedList(eccoMyLinkedList);

    sqlite3_close(db);
    return result;
}

static void addCm_xqbase_idUrl__(const wchar_t* idUrl, MyLinkedList cmMyLinkedList, void** regs, int* regNum)
{
    wchar_t* wstr = getWebWstr(idUrl);
    if (!wstr) {
        fwprintf(fout, L"\n页面没有找到：%ls\n", idUrl);
        return;
    }

    ChessManual cm = newChessManual();
    setInfoItem_cm(cm, CMINFO_NAMES[SOURCE_INDEX], idUrl);
    wchar_t value[SUPERWIDEWCHARSIZE];
    int ovector[PCREARRAY_SIZE], len = wcslen(wstr);
    for (int i = 0; i < *regNum; ++i) {
        int count = pcrewch_exec(regs[i], NULL, wstr, len, 0, 0, ovector, PCREARRAY_SIZE);
        for (int c = 1; c < count; ++c) {
            pcrewch_copy_substring(wstr, ovector, count, c, value, SUPERWIDEWCHARSIZE);
            int index = (c == 1 ? (i < 3 ? i : (i < 5 ? i + 1 : (i == 5 ? ECCOSN_INDEX : MOVESTR_INDEX)))
                                : (i == 2 ? SITE_INDEX : (i == 5 ? ECCONAME_INDEX : RESULT_INDEX)));
            if (c == 1 && i == 6) {
                int idx = wcslen(value);
                while (--idx >= 0)
                    if (value[idx] == L'\r' || value[idx] == L'\n')
                        value[idx] = L' ';
            }
            setInfoItem_cm(cm, CMINFO_NAMES[index], value);
        }
    }
    free(wstr);

    /* 验证读取棋谱的正确性
    static int no = 1;
    printCmStr__(cm, fout, &no, NULL);

    wcscpy(value, getInfoValue_name_cm(cm, CMINFO_NAMES[MOVESTR_INDEX]));
    readMove_PGN_ICCSZH__(cm, value, PGN_ZH);
    if (getNext(cm->rootMove))
        setMoveNumZhStr(cm); // 驱动函数

    writePGN__(fout, cm, PGN_ICCS);
    writePGN__(fout, cm, PGN_CC);
    //*/

    addMyLinkedList(cmMyLinkedList, cm);
}

int storeChessManual_xqbase(const char* dbName, const char* man_tblName)
{
    sqlite3* db = NULL;
    int result = 0, rc = sqlite3_open(dbName, &db);
    if (rc) {
        fprintf(stderr, "\nCan't open database: %s", sqlite3_errmsg(db));
        sqlite3_close(db);
        return result;
    }

    // 获取棋谱cm链表
    wchar_t* regStr[] = {
        L"\\<title\\>(.*)\\</title\\>",
        L"\\>([^\\>]+赛[^\\>]*)\\<",
        L"\\>(\\d+年\\d+月\\d+日) ([^\\<]*)\\<",
        L"\\>黑方 ([^\\<]*)\\<",
        L"\\>红方 ([^\\<]*)\\<",
        L"\\>([A-E]\\d{2})\\. ([^\\<]*)\\<",
        L"\\<pre\\>\\s*(1\\.[\\s\\S]*?)\\((.+)\\)\\</pre\\>"
    };
    const char* error;
    int errorffset, regNum = sizeof(regStr) / sizeof(regStr[0]);
    void* regs[regNum];
    for (int i = 0; i < regNum; ++i)
        regs[i] = pcrewch_compile(regStr[i], 0, &error, &errorffset, NULL);

    //
    //for (wchar_t sn_0 = L'a'; sn_0 <= L'e'; ++sn_0) {
    //for (wchar_t sn_0 = L'a'; sn_0 <= L'a'; ++sn_0) {
    //MyLinkedList cmMyLinkedList = getCmMyLinkedList_xqbase__(sn_0);
    //

    // 获取网页idUrl链表
    //MyLinkedList idUrlMyLinkedList = getIdUrlMyLinkedList_xqbase(sn_0);
    //traverseMyLinkedList(idUrlMyLinkedList, (void (*)(void*, void*, void*, void*))printWstr, fout, NULL, NULL);

    int step = 10;
    for (int id_start = 1, id_end = id_start + step; id_start <= ECCO_IDMAX;
         id_start = id_end, id_end = fmin(id_end + step, ECCO_IDMAX + 1)) {
        MyLinkedList idUrlMyLinkedList = getIdUrlMyLinkedList_xqbase_2(id_start, id_end);
        MyLinkedList cmMyLinkedList = newMyLinkedList((void (*)(void*))delChessManual);
        traverseMyLinkedList(idUrlMyLinkedList, (void (*)(void*, void*, void*, void*))addCm_xqbase_idUrl__,
            cmMyLinkedList, regs, &regNum);

        //*
        // 执行存储对象
        result = storeObjMyLinkedList(db, man_tblName, cmMyLinkedList,
            (MyLinkedList(*)(void*))getInfoMyLinkedList_cm__);
        //*/

        printCmMyLinkedList(fout, cmMyLinkedList);
        fwprintf(fout, L"\nGameid:%d-%d 下载棋谱完成！\n\n", id_start, id_end - 1);

        delMyLinkedList(cmMyLinkedList);
        delMyLinkedList(idUrlMyLinkedList);

        if (id_end > 10)
            break;
    }

    for (int i = 0; i < regNum; ++i)
        pcrewch_free(regs[i]);

    sqlite3_close(db);

    return result;
}
