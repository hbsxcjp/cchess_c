#define PCRE_STATIC
#include "head/chessManual.h"
#include "head/aspect.h"
#include "head/board.h"
#include "head/move.h"
#include "head/piece.h"
#include "head/tools.h"
//#include "pcre.h"
//#include <regex.h>
//#include <sys/types.h>

#define INFOSIZE 32

struct ChessManual
{
    wchar_t *fileName;
    Board board;
    Move rootMove, currentMove; // 根节点、当前节点
    wchar_t *info[INFOSIZE][2];
    int infoCount, movCount_, remCount_, maxRemLen_, maxRow_, maxCol_;
};

typedef struct OperateDirData
{
    int fcount, dcount, movCount, remCount, remLenMax;
    const char *fromDir, *toDir;
    RecFormat fromfmt, tofmt;
} * OperateDirData;

static const char *EXTNAMES[] = {
    ".xqf", ".bin", ".json", ".pgn_iccs", ".pgn_zh", ".pgn_cc"};
static const char FILETAG[] = "learnchess";

// 根据文件扩展名取得存储记录类型
static RecFormat getRecFormat__(const char *ext)
{
    if (ext)
    {
        char lowExt[WCHARSIZE] = {0};
        int len = fmin(WCHARSIZE, strlen(ext));
        for (int i = 0; i < len; ++i)
            lowExt[i] = tolower(ext[i]);
        for (int f = XQF; f < NOTFMT; ++f)
            if (strcmp(lowExt, EXTNAMES[f]) == 0)
                return f;
    }
    return NOTFMT;
}

static bool fileIsRight__(const char *fileName)
{
    return fileName && getRecFormat__(getExtName(fileName)) != NOTFMT;
}

// 从文件读取到chessManual
static void readChessManual__(ChessManual cm, const char *fileName);

// 增删改move后，更新zhStr、行列数值
static void setMoveNumZhStr__(ChessManual cm, Move move);

ChessManual newChessManual(const char *fileName)
{
    ChessManual cm = malloc(sizeof(struct ChessManual));
    assert(cm);
    wchar_t *fname = malloc(strlen(fileName) * sizeof(wchar_t));
    mbstowcs(fname, fileName, FILENAME_MAX);
    cm->fileName = fname;
    cm->board = newBoard();
    cm->currentMove = cm->rootMove = getRootMove();
    for (int i = 0; i < INFOSIZE; ++i)
        for (int j = 0; j < 2; ++j)
            cm->info[i][j] = NULL;
    cm->infoCount = cm->movCount_ = cm->remCount_ = cm->maxRemLen_ = cm->maxRow_ = cm->maxCol_ = 0;
    readChessManual__(cm, fileName);
    return cm;
}

ChessManual resetChessManual(ChessManual *cm, const char *fileName)
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
    delRootMove(cm->rootMove);
    delBoard(cm->board);
    free(cm);
}

void addInfoItem(ChessManual cm, const wchar_t *name, const wchar_t *value)
{
    int count = cm->infoCount, nameLen = wcslen(name) + 1;
    if (count == INFOSIZE || nameLen == 0)
        return;
    cm->info[count][0] = malloc(nameLen * sizeof(wchar_t));
    assert(cm->info[count][0]);
    wcscpy(cm->info[count][0], name);
    cm->info[count][1] = malloc((wcslen(value) + 1) * sizeof(wchar_t));
    assert(cm->info[count][1]);
    wcscpy(cm->info[count][1], value);
    ++(cm->infoCount);
}

void delInfoItem(ChessManual cm, const wchar_t *name)
{
    for (int i = 0; i < cm->infoCount; ++i)
        if (wcscmp(cm->info[i][0], name) == 0)
        {
            for (int j = 0; j < 2; ++j)
                free(cm->info[i][j]);
            for (int k = i + 1; k < cm->infoCount; ++k)
                for (int j = 0; j < 2; ++j)
                    cm->info[k - 1][j] = cm->info[k][j];
            --(cm->infoCount);
            break;
        }
}

static void setMoveNumZhStr__(ChessManual cm, Move move)
{ //*
    ++cm->movCount_;
    if (getOtherNo(move) > cm->maxCol_)
        setOtherNo(move, cm->maxCol_);
    if (getNextNo(move) > cm->maxRow_)
        cm->maxRow_ = getNextNo(move);

    setCC_ColNo(move, cm->maxCol_); // # 本着在视图中的列数
    if (getRemark(move))
    {
        ++cm->remCount_;
        int maxRemLen_ = wcslen(getRemark(move));
        if (maxRemLen_ > cm->maxRemLen_)
            cm->maxRemLen_ = maxRemLen_;
    }

    //assert(move->fseat);
    //wprintf(L"%3d=> %02x->%02x\n", __LINE__, move->fseat, move->tseat);

    // 先深度搜索
    setMoveZhStr(move, cm->board);
    //wprintf(L"%s ", __LINE__, getZhStr(move));

    doMove(move);
    if (hasNext(move))
        setMoveNumZhStr__(cm, getNext(move));
    undoMove(move);

    // 后广度搜索
    if (hasOther(move))
    {
        ++cm->maxCol_;
        setMoveNumZhStr__(cm, getOther(move));
    }
    //*/
}

extern int Version, KeyRMKSize;
extern char KeyXYf, KeyXYt, F32Keys[PIECENUM];

static unsigned char calkey__(unsigned char bKey, unsigned char cKey)
{
    return (((((bKey * bKey) * 3 + 9) * 3 + 8) * 2 + 1) * 3 + 8) * cKey; // % 256; // 保持为<256
}

static void readXQF__(ChessManual cm, FILE *fin)
{
    char xqfData[1024] = {0};
    fread(xqfData, sizeof(char), 1024, fin);
    char Signature[3] = {0}, headKeyMask, ProductId[4] = {0}, //Version, 文件标记'XQ'=$5158/版本/加密掩码/ProductId[4], 产品(厂商的产品号)
        headKeyOrA, headKeyOrB, headKeyOrC, headKeyOrD,
         headKeysSum, headKeyXY, headKeyXYf, headKeyXYt, // 加密的钥匙和/棋子布局位置钥匙/棋谱起点钥匙/棋谱终点钥匙
        headQiziXY[PIECENUM] = {0},                      // 32个棋子的原始位置
        // 用单字节坐标表示, 将字节变为十进制, 十位数为X(0-8)个位数为Y(0-9),
        // 棋盘的左下角为原点(0, 0). 32个棋子的位置从1到32依次为:
        // 红: 车马相士帅士相马车炮炮兵兵兵兵兵 (位置从右到左, 从下到上)
        // 黑: 车马象士将士象马车炮炮卒卒卒卒卒 (位置从右到左, 从下到上)PlayStepNo[2],
        PlayStepNo[2] = {0},
         headWhoPlay, headPlayResult, PlayNodes[4] = {0}, PTreePos[4] = {0}, Reserved1[4] = {0},
         // 该谁下 0-红先, 1-黑先/最终结果 0-未知, 1-红胜 2-黑胜, 3-和棋
        headCodeA_H[16] = {0}, TitleA[65] = {0}, TitleB[65] = {0}, //对局类型(开,中,残等)
        Event[65] = {0}, Date[17] = {0}, Site[17] = {0}, Red[17] = {0}, Black[17] = {0},
         Opening[65] = {0}, Redtime[17] = {0}, Blktime[17] = {0}, Reservedh[33] = {0},
         RMKWriter[17] = {0}, Author[17] = {0}; //, Other[528]; // 棋谱评论员/文件的作者
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
    headKeyXYt = xqfData[15];             // = 16 bytes
    memcpy(headQiziXY, xqfData + 16, 32); // = 48 bytes
    memcpy(PlayStepNo, xqfData + 48, 2);
    headWhoPlay = xqfData[50];
    headPlayResult = xqfData[51];
    memcpy(PlayNodes, xqfData + 52, 4);
    memcpy(PTreePos, xqfData + 56, 4);
    memcpy(Reserved1, xqfData + 60, 4); // = 64 bytes
    memcpy(headCodeA_H, xqfData + 64, 16);
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
    assert(Version <= 18);                                                  // L" 这是一个高版本的XQF文件，您需要更高版本的XQStudio来读取这个文件。\n";

    // 计算解密数据
    unsigned char KeyXY, *head_QiziXY = (unsigned char *)headQiziXY; //KeyXYf, KeyXYt, F32Keys[PIECENUM],//int KeyRMKSize = 0;
    if (Version <= 10)
    { // version <= 10 兼容1.0以前的版本
        KeyXY = KeyRMKSize = KeyXYf = KeyXYt = 0;
    }
    else
    {
        KeyXY = calkey__(headKeyXY, headKeyXY);
        KeyXYf = calkey__(headKeyXYf, KeyXY);
        KeyXYt = calkey__(headKeyXYt, KeyXYf);
        KeyRMKSize = ((unsigned char)headKeysSum * 256 + (unsigned char)headKeyXY) % 32000 + 767; // % 65536
        if (Version >= 12)
        { // 棋子位置循环移动
            unsigned char Qixy[PIECENUM] = {0};
            memcpy(Qixy, head_QiziXY, PIECENUM);
            for (int i = 0; i != PIECENUM; ++i)
                head_QiziXY[(i + KeyXY + 1) % PIECENUM] = Qixy[i];
        }
        for (int i = 0; i != PIECENUM; ++i)
            head_QiziXY[i] -= KeyXY; // 保持为8位无符号整数，<256
    }
    int KeyBytes[4] = {
        (headKeysSum & headKeyMask) | headKeyOrA,
        (headKeyXY & headKeyMask) | headKeyOrB,
        (headKeyXYf & headKeyMask) | headKeyOrC,
        (headKeyXYt & headKeyMask) | headKeyOrD};
    const char copyright[] = "[(C) Copyright Mr. Dong Shiwei.]";
    for (int i = 0; i != PIECENUM; ++i)
        F32Keys[i] = copyright[i] & KeyBytes[i % 4]; // ord(c)

    //wprintf(L"%3d=> %d %d %d %d\n", __LINE__, Version, KeyRMKSize, KeyXYf, KeyXYt);

    // 取得棋子字符串
    wchar_t pieChars[SEATNUM + 1] = {0};
    wmemset(pieChars, getBlankChar(), SEATNUM);
    const wchar_t QiziChars[] = L"RNBAKABNRCCPPPPPrnbakabnrccppppp"; // QiziXY设定的棋子顺序
    for (int i = 0; i != PIECENUM; ++i)
    {
        int xy = head_QiziXY[i];
        if (xy <= 89)
            // 用字节坐标表示, 将字节变为十进制,  十位数为X(0-8),个位数为Y(0-9)
            // 棋盘的左下角为原点(0, 0)，需转换成FEN格式：以左上角为原点(0, 0)
            pieChars[(9 - xy % 10) * 9 + xy / 10] = QiziChars[i];
    }

    wchar_t tempStr[WIDEWCHARSIZE] = {0};
    char *values[] = {
        TitleA, Event, Date, Site, Red, Black,
        Opening, RMKWriter, Author};
    wchar_t *names[] = {
        L"TitleA", L"Event", L"Date", L"Site", L"Red", L"Black",
        L"Opening", L"RMKWriter", L"Author"};
    for (int i = 0; i != sizeof(names) / sizeof(names[0]); ++i)
    {
        mbstowcs(tempStr, values[i], WIDEWCHARSIZE - 1);
        addInfoItem(cm, names[i], tempStr);
    }
    wchar_t *PlayType[] = {L"全局", L"开局", L"中局", L"残局"};
    addInfoItem(cm, L"PlayType", PlayType[(int)(headCodeA_H[0])]); // 编码定义存储
    getFEN_pieChars(tempStr, pieChars);
    addInfoItem(cm, L"FEN", wcscat(tempStr, headWhoPlay ? L" -r" : L" -b")); // 转换FEN存储
    wchar_t *Result[] = {L"未知", L"红胜", L"黑胜", L"和棋"};
    addInfoItem(cm, L"Result", Result[(int)headPlayResult]); // 编码定义存储
    swprintf(tempStr, WIDEWCHARSIZE, L"%d", (int)Version);
    addInfoItem(cm, L"Version", tempStr); // 整数存储
    // "标题: 赛事: 日期: 地点: 红方: 黑方: 结果: 评论: 作者: "

    fseek(fin, 1024, SEEK_SET);
    readMove_XQF(&cm->rootMove, cm->board, fin, false);
}

static wchar_t *getFENFromCM__(ChessManual cm)
{
    wchar_t fen[] = L"FEN";
    for (int i = 0; i < cm->infoCount; ++i)
        if (wcscmp(cm->info[i][0], fen) == 0)
            return cm->info[i][1];
    addInfoItem(cm, fen, L"rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR");
    return cm->info[cm->infoCount - 1][1];
}

static void getFENToSetBoard__(ChessManual cm)
{
    setBoard_FEN(cm->board, getFENFromCM__(cm));
}

static void readBin__(ChessManual cm, FILE *fin)
{
    char fileTag[sizeof(FILETAG)];
    fread(&fileTag, sizeof(char), sizeof(FILETAG), fin);
    if (strcmp(fileTag, FILETAG) != 0) //文件标志不对
        return;
    char tag = 0, infoCount = 0;
    fread(&tag, sizeof(char), 1, fin);
    if (tag & 0x10)
    {
        fread(&infoCount, sizeof(char), 1, fin);
        for (int i = 0; i < infoCount; ++i)
        {
            wchar_t *name = readWstring_BIN(fin);
            wchar_t *value = readWstring_BIN(fin);
            addInfoItem(cm, name, value);
            free(name);
            free(value);
        }
    }

    if (tag & 0x20)
        setRemark(cm->rootMove, readWstring_BIN(fin));
    if (tag & 0x80)
        readMove_BIN(cm->rootMove, cm->board, fin, false);
}

static void writeBIN__(FILE *fout, ChessManual cm)
{
    fwrite(FILETAG, sizeof(char), sizeof(FILETAG), fout);
    char infoCount = cm->infoCount;
    char tag = ((infoCount > 0 ? 0x10 : 0x00) | (getRemark(cm->rootMove) ? 0x20 : 0x00) | (hasNext(cm->rootMove) ? 0x80 : 0x00));
    fwrite(&tag, sizeof(char), 1, fout);
    if (tag & 0x10)
    {
        fwrite(&infoCount, sizeof(char), 1, fout);
        for (int i = 0; i < infoCount; ++i)
        {
            writeWstring_BIN(fout, cm->info[i][0]);
            writeWstring_BIN(fout, cm->info[i][1]);
        }
    }

    if (tag & 0x20)
        writeWstring_BIN(fout, getRemark(cm->rootMove));
    if (tag & 0x80)
        writeMove_BIN(fout, cm->rootMove);
}

static void readJSON__(ChessManual cm, FILE *fin)
{
    fseek(fin, 0L, SEEK_END); // 定位到文件末尾
    long last = ftell(fin);
    char *manualString = (char *)calloc(last + 1, sizeof(char));
    assert(manualString);
    fseek(fin, 0L, SEEK_SET); // 定位到文件开始
    fread(manualString, sizeof(char), last, fin);
    cJSON *manualJSON = cJSON_Parse(manualString);
    free(manualString);

    cJSON *infoJSON = cJSON_GetObjectItem(manualJSON, "info");
    int infoCount = cJSON_GetArraySize(infoJSON);
    for (int i = 0; i < infoCount; ++i)
    {
        cJSON *keyValueJSON = cJSON_GetArrayItem(infoJSON, i);
        wchar_t nameValue[2][WCHARSIZE] = {0};
        for (int j = 0; j < 2; ++j)
            mbstowcs(nameValue[j],
                     cJSON_GetStringValue(cJSON_GetArrayItem(keyValueJSON, j)), WCHARSIZE);
        addInfoItem(cm, nameValue[0], nameValue[1]);
    }

    cJSON *rootMoveJSON = cJSON_GetObjectItem(manualJSON, "rootmove");
    if (rootMoveJSON)
        readMove_JSON(cm->rootMove, cm->board, rootMoveJSON, false);
    cJSON_Delete(manualJSON);
}

static void writeJSON__(FILE *fout, ChessManual cm)
{
    cJSON *manualJSON = cJSON_CreateObject(),
          *infoJSON = cJSON_CreateArray(),
          *rootmoveJSON = cJSON_CreateObject();
    for (int i = 0; i < cm->infoCount; ++i)
    {
        char name[WCHARSIZE], value[WCHARSIZE];
        wcstombs(name, cm->info[i][0], WCHARSIZE);
        wcstombs(value, cm->info[i][1], WCHARSIZE);
        cJSON_AddItemToArray(infoJSON,
                             cJSON_CreateStringArray((const char *const[]){name, value}, 2));
    }
    cJSON_AddItemToObject(manualJSON, "info", infoJSON);

    writeMove_JSON(rootmoveJSON, cm->rootMove);
    cJSON_AddItemToObject(manualJSON, "rootmove", rootmoveJSON);

    char *manualString = cJSON_Print(manualJSON);
    fwrite(manualString, sizeof(char), strlen(manualString) + 1, fout);
    cJSON_Delete(manualJSON);
}

static void readInfo_PGN__(ChessManual cm, FILE *fin)
{
    const char *error;
    int erroffset = 0, infoCount = 0, ovector[10]; //OVECCOUNT = 10,
    const wchar_t *infoPat = L"\\[(\\w+)\\s+\"([\\s\\S]*?)\"\\]";
    pcre32 *infoReg = pcre32_compile((const unsigned int *)infoPat, 0, &error, &erroffset, NULL);
    assert(infoReg);
    wchar_t infoStr[WIDEWCHARSIZE] = {0};
    while (fgetws(infoStr, WIDEWCHARSIZE, fin) && infoStr[0] != L'\n')
    { // 以空行为终止特征
        infoCount = pcre32_exec(infoReg, NULL, (const unsigned int *)infoStr, wcslen(infoStr),
                                0, 0, ovector, 10);
        if (infoCount < 0)
            continue;
        wchar_t name[WCHARSIZE] = {0}, value[WCHARSIZE] = {0};
        wcsncpy(name, infoStr + ovector[2], ovector[3] - ovector[2]);
        wcsncpy(value, infoStr + ovector[4], ovector[5] - ovector[4]);
        addInfoItem(cm, name, value);
    }
    pcre32_free(infoReg);
}

static void readPGN__(ChessManual cm, FILE *fin, RecFormat fmt)
{
    //printf("准备读取info... ");
    //
    readInfo_PGN__(cm, fin);
    // PGN_ZH, PGN_CC在读取move之前需要先设置board
    getFENToSetBoard__(cm);

    //printf("准备读取move... ");
    //
    (fmt == PGN_CC) ? readMove_PGN_CC(cm->rootMove, fin, cm->board) : readMove_PGN_ICCSZH(cm->rootMove, fin, fmt, cm->board);
}

void writeInfo_PGN_CCtoWstr(wchar_t **pinfoStr, ChessManual cm)
{
    int size = WIDEWCHARSIZE;
    wchar_t tmpWstr[WIDEWCHARSIZE], *infoStr = malloc(size * sizeof(wchar_t));
    assert(infoStr);
    infoStr[0] = L'\x0';
    for (int i = 0; i < cm->infoCount; ++i)
    {
        swprintf(tmpWstr, WIDEWCHARSIZE, L"[%s \"%s\"]\n", cm->info[i][0], cm->info[i][1]);
        writeWString(&infoStr, &size, tmpWstr);
    }
    *pinfoStr = infoStr;
}

void writeMove_PGN_CCtoWstr(wchar_t **pmoveStr, ChessManual cm)
{
    int rowNum = (cm->maxRow_ + 1) * 2,
        colNum = (cm->maxCol_ + 1) * 5 + 1,
        size = rowNum * colNum;
    wchar_t *moveStr = malloc((size + 1) * sizeof(wchar_t));
    assert(moveStr);
    wmemset(moveStr, L'　', size);
    for (int row = 0; row < rowNum; ++row)
    {
        moveStr[(row + 1) * colNum - 1] = L'\n';
        //if (row % 2 == 1)
        //  moveStr[row * colNum] = L' '; // 为显示美观, 奇数行改为半角空格
    }
    moveStr[1] = L'开';
    moveStr[2] = L'始';
    moveStr[colNum + 2] = L'↓';
    moveStr[size] = L'\x0';

    writeMove_PGN_CC(moveStr, colNum, cm->rootMove);
    *pmoveStr = moveStr;
}

void writeRemark_PGN_CCtoWstr(wchar_t **premStr, ChessManual cm)
{
    int size = WIDEWCHARSIZE;
    wchar_t *remarkStr = malloc(size * sizeof(wchar_t));
    assert(remarkStr);
    remarkStr[0] = L'\x0';
    writeRemark_PGN_CC(&remarkStr, &size, cm->rootMove);
    *premStr = remarkStr;
}

void writePGN_CCtoWstr(wchar_t **pstr, ChessManual cm)
{
    wchar_t *infoStr = NULL, *moveStr = NULL, *remarkStr = NULL;
    writeInfo_PGN_CCtoWstr(&infoStr, cm);
    writeMove_PGN_CCtoWstr(&moveStr, cm);
    writeRemark_PGN_CCtoWstr(&remarkStr, cm);

    wchar_t *wstr = malloc((wcslen(infoStr) + wcslen(moveStr) + wcslen(remarkStr) + 1) * sizeof(wchar_t));
    assert(wstr);
    wcscpy(wstr, infoStr);
    wcscat(wstr, L"\n");
    wcscat(wstr, moveStr);
    wcscat(wstr, L"\n");
    wcscat(wstr, remarkStr);
    *pstr = wstr;

    free(infoStr);
    free(moveStr);
    free(remarkStr);
}

static void writePGN__(FILE *fout, ChessManual cm, RecFormat fmt)
{
    if (fmt != PGN_CC)
    {
        for (int i = 0; i < cm->infoCount; ++i)
            fwprintf(fout, L"[%s \"%s\"]\n", cm->info[i][0], cm->info[i][1]);
        fwprintf(fout, L"\n");
        writeMove_PGN_ICCSZH(fout, cm->rootMove, fmt);
    }
    else
    {
        wchar_t *wstr = NULL;
        writePGN_CCtoWstr(&wstr, cm);
        fwprintf(fout, L"%s", wstr);
        free(wstr);

        /*
        wchar_t *moveStr = NULL, *remarkStr = NULL;
        writeMove_PGN_CCtoWstr(&moveStr, cm);
        writeRemark_PGN_CCtoWstr(&remarkStr, cm);
        fwprintf(fout, L"%s\n%s", moveStr, remarkStr);
        free(moveStr);
        free(remarkStr);
        //*/
    }
}

void readChessManual__(ChessManual cm, const char *fileName)
{
    if (!fileIsRight__(fileName))
        return;
    RecFormat fmt = getRecFormat__(getExtName(fileName));
    /*
    if (fmt == NOTFMT) {
        wprintf(L"未实现的打开文件扩展名！");
        return;
    }
    //*/
    FILE *fin = fopen(fileName, (fmt == XQF || fmt == BIN || fmt == JSON) ? "rb" : "r");
    if (fin == NULL)
        return;
    switch (fmt)
    {
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
        //printf("准备读取文件...%s ", fileName);
        //
        readPGN__(cm, fin, fmt);
        //printf("读取成功！ ");
        //
        break;
    }
    if (fmt == XQF || fmt == BIN || fmt == JSON)
        getFENToSetBoard__(cm);
    //*

    if (hasNext(cm->rootMove))
        setMoveNumZhStr__(cm, getNext(cm->rootMove)); // 驱动函数
    //*/
    //printf("设置成功！\n");

    fclose(fin);
}

void writeChessManual(ChessManual cm, const char *fileName)
{
    if (!fileIsRight__(fileName))
        return;
    RecFormat fmt = getRecFormat__(getExtName(fileName));
    /*
    if (fmt == NOTFMT) {
        wprintf(L"未实现的写入文件扩展名！");
        return;
    }
    //*/
    FILE *fout = fopen(fileName,
                       (fmt == XQF || fmt == BIN || fmt == JSON) ? "wb" : "w");
    if (fout == NULL)
        return;
    switch (fmt)
    {
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

void go(ChessManual cm)
{
    if (hasNext(cm->currentMove))
    {
        cm->currentMove = getNext(cm->currentMove);
        doMove(cm->currentMove);
    }
}

void goOther(ChessManual cm)
{
    if (hasOther(cm->currentMove))
    {
        undoMove(cm->currentMove);
        cm->currentMove = getOther(cm->currentMove);
        doMove(cm->currentMove);
    }
}

void goEnd(ChessManual cm)
{
    while (hasNext(cm->currentMove))
        go(cm);
}

void goTo(ChessManual cm, Move move)
{
    cm->currentMove = move;
    Move preMoves[getNextNo(move)];
    int count = getPreMoves(preMoves, move);
    for (int i = 0; i < count; ++i)
        doMove(preMoves[i]);
}

static void doBack__(ChessManual cm)
{
    undoMove(cm->currentMove);
    cm->currentMove = getSimplePre(cm->currentMove);
}

void back(ChessManual cm)
{
    if (hasPreOther(cm->currentMove))
        backOther(cm);
    else if (hasSimplePre(cm->currentMove))
        doBack__(cm);
}

void backNext(ChessManual cm)
{
    if (hasSimplePre(cm->currentMove) && !hasPreOther(cm->currentMove))
        doBack__(cm);
}

void backOther(ChessManual cm)
{
    if (hasPreOther(cm->currentMove))
    {
        doBack__(cm);            // 变着回退
        doMove(cm->currentMove); // 前变执行
    }
}

void backFirst(ChessManual cm)
{
    while (hasSimplePre(cm->currentMove))
        back(cm);
}

void backTo(ChessManual cm, Move move)
{
    while (hasSimplePre(cm->currentMove) && cm->currentMove != move)
        back(cm);
}

void goInc(ChessManual cm, int inc)
{
    int count = abs(inc);
    void (*func)(ChessManual) = inc > 0 ? go : back;
    while (count-- > 0)
        func(cm);
}

static void moveMap__(Move move, Board board, void apply(Move, Board, void *), void *ptr)
{
    if (move == NULL)
        return;
    apply(move, board, ptr);

    doMove(move);
    moveMap__(getNext(move), board, apply, ptr);
    undoMove(move);

    moveMap__(getOther(move), board, apply, ptr);
}

void moveMap(ChessManual cm, void apply(Move, Board, void *), void *ptr)
{
    moveMap__(getNext(cm->rootMove), cm->board, apply, ptr);
}

void changeChessManual(ChessManual cm, ChangeType ct)
{
    Move curMove = cm->currentMove;
    backFirst(cm);

    // info未更改
    changeBoard(cm->board, ct);
    Move firstMove = getNext(cm->rootMove);
    if (firstMove)
    {
        if (ct != EXCHANGE)
            changeMove(firstMove, cm->board, ct);
        if (ct == EXCHANGE || ct == SYMMETRY)
        {
            cm->movCount_ = cm->remCount_ = cm->maxRemLen_ = cm->maxRow_ = cm->maxCol_ = 0;
            setMoveNumZhStr__(cm, firstMove);
        }
    }

    goTo(cm, curMove);
}

void writeAllMoveStr(FILE *fout, ChessManual cm, const Move amove)
{
    Move cmove = cm->currentMove, moves[WIDEWCHARSIZE];
    backFirst(cm);
    int count = getAllMoves(moves, amove);
    // 输出棋局和着法
    for (int i = 0; i < count; ++i)
    {
        Move move = moves[i];
        wchar_t bstr[WIDEWCHARSIZE], tstr[WCHARSIZE];
        fwprintf(fout, L"%s%s\n", getBoardString(bstr, cm->board), getMoveString(tstr, move));
        doMove(move);
    }
    for (int i = count - 1; i >= 0; --i)
        undoMove(moves[i]);
    goTo(cm, cmove);
}

static void transFile__(FileInfo fileInfo, void *ptr)
{
    char fileName[FILENAME_MAX];
    getFileInfoName(fileName, fileInfo);
    if (!fileIsRight__(fileName))
        return;
    //printf("%d: %s\n", __LINE__, fileName);
    //
    OperateDirData odata = (OperateDirData)ptr;
    ChessManual cm = newChessManual(fileName);

    char toDirName[FILENAME_MAX], toFileName[FILENAME_MAX];
    strcpy(toFileName, odata->toDir);
    strcat(toFileName, fileName + strlen(odata->fromDir)); //替换源目录名
    getDirName(toDirName, toFileName);

    // 检查并创建(多级)目录
    char tmpDirName[FILENAME_MAX] = {0}, *dname, tokseps[] = "\\/";
    dname = strtok(toDirName, tokseps);
    while (dname)
    {
        strcat(tmpDirName, dname);
        if (access(tmpDirName, 0) != 0)
        {
            makeDir(tmpDirName);
            odata->dcount++;
            //printf("%d: makeDir-> %s\n", __LINE__, toDirName);
            //
        }
        strcat(tmpDirName, "/");
        dname = strtok(NULL, tokseps);
    }

    //char toFileName[FILENAME_MAX];
    transFileExtName(fileName, EXTNAMES[odata->tofmt]);
    //snprintf(toFileName, FILENAME_MAX, "%s/%s", toDirName, getFileName(fileName));
    strcpy(toFileName, toDirName);
    strcat(toFileName, "/");
    strcat(toFileName, getFileName(fileName));

    //printf("%d: %s\n", __LINE__, toFileName);
    //

    writeChessManual(cm, toFileName);
    ++odata->fcount;
    odata->movCount += cm->movCount_;
    odata->remCount += cm->remCount_;
    if (odata->remLenMax < cm->maxRemLen_)
        odata->remLenMax = cm->maxRemLen_;
    delChessManual(cm);
}

void transDir(const char *dirName, RecFormat fromfmt, RecFormat tofmt)
{
    char fromDir[FILENAME_MAX], toDir[FILENAME_MAX]; //, curDir[FILENAME_MAX];
    sprintf(fromDir, "%s%s", dirName, EXTNAMES[fromfmt]);
    sprintf(toDir, "%s%s", dirName, EXTNAMES[tofmt]);

    OperateDirData odata = malloc(sizeof(struct OperateDirData));
    odata->fcount = odata->dcount = odata->movCount = odata->remCount = odata->remLenMax = 0;
    odata->fromDir = fromDir;
    odata->toDir = toDir;
    odata->fromfmt = fromfmt;
    odata->tofmt = tofmt;
    if (strlen(toDir) > 0 && access(toDir, 0) != 0)
    {
        makeDir(toDir);
        //printf("%d: %s\n", __LINE__, toDir);
    }
    operateDir(fromDir, transFile__, odata, true);

    printf("\n%s =>%s: %d files, %d dirs.\n   movCount: %d, remCount: %d, remLenMax: %d\n",
           fromDir, toDir, odata->fcount, odata->dcount, odata->movCount, odata->remCount, odata->remLenMax);

    free(odata);
}

void getChessManualNumStr(char *str, ChessManual cm)
{
    snprintf(str, WIDEWCHARSIZE, "%s: movCount:%d remCount:%d remLenMax:%d maxRow:%d maxCol:%d\n",
             __func__, cm->movCount_, cm->remCount_, cm->maxRemLen_, cm->maxRow_, cm->maxCol_);
}
