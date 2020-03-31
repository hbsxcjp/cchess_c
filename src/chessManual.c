#define PCRE_STATIC
#include "head/chessManual.h"
#include "head/board.h"
#include "head/move.h"
#include "head/piece.h"
#include "head/tools.h"
//#include <regex.h>
//#include <sys/types.h>

#define INFOSIZE 32

struct ChessManual {
    Board board;
    Move rootMove, currentMove; // 根节点、当前节点
    wchar_t* info[INFOSIZE][2];
    int infoCount, movCount_, remCount_, maxRemLen_, maxRow_, maxCol_;
};

static const char* EXTNAMES[] = {
    ".xqf", ".bin", ".json", ".pgn_iccs", ".pgn_zh", ".pgn_cc"
};
static const char FILETAG[] = "learnchess";

// 从文件读取到chessManual
static void __readChessManual(ChessManual cm, const char* filename);

// 增删改move后，更新zhStr、行列数值
static void __setMoveNumZhStr(ChessManual cm, Move move);

ChessManual newChessManual(const char* filename)
{
    ChessManual cm = malloc(sizeof(struct ChessManual));
    cm->board = newBoard();
    cm->currentMove = cm->rootMove = newMove();
    for (int i = 0; i < INFOSIZE; ++i)
        for (int j = 0; j < 2; ++j)
            cm->info[i][j] = NULL;
    cm->infoCount = cm->movCount_ = cm->remCount_ = cm->maxRemLen_ = cm->maxRow_ = cm->maxCol_ = 0;
    __readChessManual(cm, filename);
    return cm;
}

ChessManual resetChessManual(ChessManual* cm, const char* filename)
{
    delChessManual(*cm);
    return *cm = newChessManual(filename);
}

void delChessManual(ChessManual cm)
{
    if (cm == NULL)
        return;
    for (int i = 0; i < cm->infoCount; ++i)
        for (int j = 0; j < 2; ++j)
            free(cm->info[i][j]);
    freeMove(cm->rootMove);
    freeBoard(cm->board);
    free(cm);
}

void addInfoItem(ChessManual cm, const wchar_t* name, const wchar_t* value)
{
    int count = cm->infoCount, nameLen = wcsnlen_s(name, WCHARSIZE) + 1;
    if (count == INFOSIZE || nameLen == 0)
        return;
    cm->info[count][0] = malloc(nameLen * sizeof(name[0]));
    wcscpy(cm->info[count][0], name);
    cm->info[count][1] = malloc((wcsnlen_s(value, WCHARSIZE) + 1) * sizeof(value[0]));
    wcscpy(cm->info[count][1], value);
    ++(cm->infoCount);
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

// 根据文件扩展名取得存储记录类型
static RecFormat __getRecFormat(const char* ext)
{
    char lowExt[WCHARSIZE] = { 0 };
    int len = min(WCHARSIZE, strlen(ext));
    for (int i = 0; i < len; ++i)
        lowExt[i] = tolower(ext[i]);
    for (int f = XQF; f < NOTFMT; ++f)
        if (strcmp(lowExt, EXTNAMES[f]) == 0)
            return f;
    return NOTFMT;
}

static wchar_t* __getFENFromCM(ChessManual cm)
{
    wchar_t fen[] = L"FEN";
    for (int i = 0; i < cm->infoCount; ++i)
        if (wcscmp(cm->info[i][0], fen) == 0)
            return cm->info[i][1];
    addInfoItem(cm, fen, L"rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR");
    return cm->info[cm->infoCount - 1][1];
}

static void __setMoveNumZhStr(ChessManual cm, Move move)
{ //*
    ++cm->movCount_;
    if (getOtherNo(move) > cm->maxCol_)
        setOtherNo(move, cm->maxCol_);
    if (getNextNo(move) > cm->maxRow_)
        cm->maxRow_ = getNextNo(move);

    setCC_ColNo(move, cm->maxCol_); // # 本着在视图中的列数
    if (getRemark(move) != NULL) {
        ++cm->remCount_;
        int maxRemLen_ = wcslen(getRemark(move));
        if (maxRemLen_ > cm->maxRemLen_)
            cm->maxRemLen_ = maxRemLen_;
    }

    //assert(move->fseat != NULL);
    //wprintf(L"%3d=> %02x->%02x\n", __LINE__, move->fseat, move->tseat);

    // 先深度搜索
    setMoveZhStr(move, cm->board);
    //wprintf(L"%s ", __LINE__, getZhStr(move));

    doMove(cm->board, move);
    if (hasNext(move))
        __setMoveNumZhStr(cm, getNext(move));
    undoMove(cm->board, move);

    // 后广度搜索
    if (hasOther(move)) {
        ++cm->maxCol_;
        __setMoveNumZhStr(cm, getOther(move));
    }
    //*/
}

extern int Version, KeyRMKSize;
extern char KeyXYf, KeyXYt, F32Keys[PIECENUM];
extern unsigned char __calkey(unsigned char bKey, unsigned char cKey);

static void __readXQF(ChessManual cm, FILE* fin)
{
    char xqfData[1024] = { 0 };
    fread(xqfData, sizeof(char), 1024, fin);
    char Signature[3] = { 0 }, headKeyMask, ProductId[4] = { 0 }, //Version, 文件标记'XQ'=$5158/版本/加密掩码/ProductId[4], 产品(厂商的产品号)
        headKeyOrA, headKeyOrB, headKeyOrC, headKeyOrD,
         headKeysSum, headKeyXY, headKeyXYf, headKeyXYt, // 加密的钥匙和/棋子布局位置钥匙/棋谱起点钥匙/棋谱终点钥匙
        headQiziXY[PIECENUM] = { 0 }, // 32个棋子的原始位置
        // 用单字节坐标表示, 将字节变为十进制, 十位数为X(0-8)个位数为Y(0-9),
        // 棋盘的左下角为原点(0, 0). 32个棋子的位置从1到32依次为:
        // 红: 车马相士帅士相马车炮炮兵兵兵兵兵 (位置从右到左, 从下到上)
        // 黑: 车马象士将士象马车炮炮卒卒卒卒卒 (位置从右到左, 从下到上)PlayStepNo[2],
        PlayStepNo[2] = { 0 },
         headWhoPlay, headPlayResult, PlayNodes[4] = { 0 }, PTreePos[4] = { 0 }, Reserved1[4] = { 0 },
         // 该谁下 0-红先, 1-黑先/最终结果 0-未知, 1-红胜 2-黑胜, 3-和棋
        headCodeA_H[16] = { 0 }, TitleA[65] = { 0 }, TitleB[65] = { 0 }, //对局类型(开,中,残等)
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
    unsigned char KeyXY, *head_QiziXY = (unsigned char*)headQiziXY; //KeyXYf, KeyXYt, F32Keys[PIECENUM],//int KeyRMKSize = 0;
    if (Version <= 10) { // version <= 10 兼容1.0以前的版本
        KeyXY = KeyRMKSize = KeyXYf = KeyXYt = 0;
    } else {
        KeyXY = __calkey(headKeyXY, headKeyXY);
        KeyXYf = __calkey(headKeyXYf, KeyXY);
        KeyXYt = __calkey(headKeyXYt, KeyXYf);
        KeyRMKSize = ((unsigned char)headKeysSum * 256 + (unsigned char)headKeyXY) % 32000 + 767; // % 65536
        if (Version >= 12) { // 棋子位置循环移动
            unsigned char Qixy[PIECENUM] = { 0 };
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
        (headKeyXYt & headKeyMask) | headKeyOrD
    };
    const char copyright[] = "[(C) Copyright Mr. Dong Shiwei.]";
    for (int i = 0; i != PIECENUM; ++i)
        F32Keys[i] = copyright[i] & KeyBytes[i % 4]; // ord(c)

    //wprintf(L"%3d=> %d %d %d %d\n", __LINE__, Version, KeyRMKSize, KeyXYf, KeyXYt);

    // 取得棋子字符串
    wchar_t pieChars[SEATNUM + 1] = { 0 };
    wmemset(pieChars, BLANKCHAR, SEATNUM);
    const wchar_t QiziChars[] = L"RNBAKABNRCCPPPPPrnbakabnrccppppp"; // QiziXY设定的棋子顺序
    for (int i = 0; i != PIECENUM; ++i) {
        int xy = head_QiziXY[i];
        if (xy <= 89)
            // 用字节坐标表示, 将字节变为十进制,  十位数为X(0-8),个位数为Y(0-9)
            // 棋盘的左下角为原点(0, 0)，需转换成FEN格式：以左上角为原点(0, 0)
            pieChars[(9 - xy % 10) * 9 + xy / 10] = QiziChars[i];
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
        mbstowcs(tempStr, values[i], WIDEWCHARSIZE - 1);
        addInfoItem(cm, names[i], tempStr);
    }
    wchar_t* PlayType[] = { L"全局", L"开局", L"中局", L"残局" };
    addInfoItem(cm, L"PlayType", PlayType[(int)(headCodeA_H[0])]); // 编码定义存储
    setFENFromPieChars(tempStr, pieChars);
    addInfoItem(cm, L"FEN", wcscat(tempStr, headWhoPlay ? L" -r" : L" -b")); // 转换FEN存储
    wchar_t* Result[] = { L"未知", L"红胜", L"黑胜", L"和棋" };
    addInfoItem(cm, L"Result", Result[(int)headPlayResult]); // 编码定义存储
    swprintf(tempStr, WIDEWCHARSIZE, L"%d", (int)Version);
    addInfoItem(cm, L"Version", tempStr); // 整数存储
    // "标题: 赛事: 日期: 地点: 红方: 黑方: 结果: 评论: 作者: "

    fseek(fin, 1024, SEEK_SET);
    readMove_XQF(&cm->rootMove, cm->board, fin, false);
}

static void __getFENToSetBoard(ChessManual cm)
{
    wchar_t* FEN = __getFENFromCM(cm);
    wchar_t pieChars[SEATNUM + 1];
    setBoard(cm->board, setPieCharsFromFEN(pieChars, FEN));
}

static void __readBin(ChessManual cm, FILE* fin)
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
            wchar_t* name = readWstring_BIN(fin);
            wchar_t* value = readWstring_BIN(fin);
            addInfoItem(cm, name, value);
            free(name);
            free(value);
        }
    }
    //
    //wchar_t* FEN = __getFENFromCM(cm);
    //wchar_t pieChars[SEATNUM + 1] = { 0 };
    //setBoard(cm->board, getPieChars_F(pieChars, FEN, wcslen(FEN)));
    //

    if (tag & 0x20)
        setRemark(cm->rootMove, readWstring_BIN(fin));
    if (tag & 0x80)
        readMove_BIN(cm->rootMove, cm->board, fin, false);
}

static void __writeBIN(ChessManual cm, FILE* fout)
{
    fwrite(FILETAG, sizeof(char), sizeof(FILETAG), fout);
    char infoCount = cm->infoCount;
    char tag = ((infoCount > 0 ? 0x10 : 0x00)
        | (getRemark(cm->rootMove) != NULL ? 0x20 : 0x00)
        | (hasNext(cm->rootMove) ? 0x80 : 0x00));
    fwrite(&tag, sizeof(char), 1, fout);
    if (tag & 0x10) {
        fwrite(&infoCount, sizeof(char), 1, fout);
        for (int i = 0; i < infoCount; ++i) {
            writeWstring_BIN(fout, cm->info[i][0]);
            writeWstring_BIN(fout, cm->info[i][1]);
        }
    }

    if (tag & 0x20)
        writeWstring_BIN(fout, getRemark(cm->rootMove));
    if (tag & 0x80)
        writeMove_BIN(fout, cm->rootMove);
}

static void __readJSON(ChessManual cm, FILE* fin)
{
    fseek(fin, 0L, SEEK_END); // 定位到文件末尾
    long last = ftell(fin);
    char* manualString = (char*)calloc(last + 1, sizeof(char));
    fseek(fin, 0L, SEEK_SET); // 定位到文件开始
    fread(manualString, sizeof(char), last, fin);
    cJSON* manualJSON = cJSON_Parse(manualString);
    free(manualString);

    cJSON* infoJSON = cJSON_GetObjectItem(manualJSON, "info");
    int infoCount = cJSON_GetArraySize(infoJSON);
    for (int i = 0; i < infoCount; ++i) {
        cJSON* keyValueJSON = cJSON_GetArrayItem(infoJSON, i);
        wchar_t nameValue[2][WCHARSIZE] = { 0 };
        for (int j = 0; j < 2; ++j)
            mbstowcs(nameValue[j],
                cJSON_GetStringValue(cJSON_GetArrayItem(keyValueJSON, j)), WCHARSIZE);
        addInfoItem(cm, nameValue[0], nameValue[1]);
    }

    cJSON* rootMoveJSON = cJSON_GetObjectItem(manualJSON, "rootmove");
    if (rootMoveJSON != NULL)
        readMove_JSON(cm->rootMove, cm->board, rootMoveJSON, false);
    cJSON_Delete(manualJSON);
}

static void __writeJSON(ChessManual cm, FILE* fout)
{
    cJSON *manualJSON = cJSON_CreateObject(),
          *infoJSON = cJSON_CreateArray(),
          *rootmoveJSON = cJSON_CreateObject();
    for (int i = 0; i < cm->infoCount; ++i) {
        char name[WCHARSIZE], value[WCHARSIZE];
        wcstombs(name, cm->info[i][0], WCHARSIZE);
        wcstombs(value, cm->info[i][1], WCHARSIZE);
        cJSON_AddItemToArray(infoJSON,
            cJSON_CreateStringArray((const char* const[]){ name, value }, 2));
    }
    cJSON_AddItemToObject(manualJSON, "info", infoJSON);

    writeMove_JSON(rootmoveJSON, cm->rootMove);
    cJSON_AddItemToObject(manualJSON, "rootmove", rootmoveJSON);

    char* manualString = cJSON_Print(manualJSON);
    fwrite(manualString, sizeof(char), strlen(manualString) + 1, fout);
    cJSON_Delete(manualJSON);
}

static void __readInfo_PGN(ChessManual cm, FILE* fin)
{
    const char* error;
    int erroffset = 0, infoCount = 0, ovector[10]; //OVECCOUNT = 10,
    const wchar_t* infoPat = L"\\[(\\w+)\\s+\"([\\s\\S]*?)\"\\]";
    pcre16* infoReg = pcre16_compile(infoPat, 0, &error, &erroffset, NULL);
    assert(infoReg);
    wchar_t infoStr[WIDEWCHARSIZE] = { 0 };
    while (fgetws(infoStr, WIDEWCHARSIZE, fin) && infoStr[0] != L'\n') { // 以空行为终止特征
        infoCount = pcre16_exec(infoReg, NULL, infoStr, wcslen(infoStr),
            0, 0, ovector, 10);
        if (infoCount < 0)
            continue;
        wchar_t name[WCHARSIZE] = { 0 }, value[WCHARSIZE] = { 0 };
        wcsncpy(name, infoStr + ovector[2], ovector[3] - ovector[2]);
        wcsncpy(value, infoStr + ovector[4], ovector[5] - ovector[4]);
        addInfoItem(cm, name, value);
    }
    pcre16_free(infoReg);
}

static void __readPGN(ChessManual cm, FILE* fin, RecFormat fmt)
{
    __readInfo_PGN(cm, fin);
    // PGN_ZH, PGN_CC在读取move之前需要先设置board
    __getFENToSetBoard(cm);
    (fmt == PGN_CC) ? readMove_PGN_CC(cm->rootMove, fin, cm->board) : readMove_PGN_ICCSZH(cm->rootMove, fin, fmt, cm->board);
}

void writeMove_PGN_CCtoWstr(wchar_t** pmoveStr, ChessManual cm)
{
    int rowNum = (cm->maxRow_ + 1) * 2,
        colNum = (cm->maxCol_ + 1) * 5 + 1;
    wchar_t* moveStr = malloc((rowNum * colNum + 1) * sizeof(wchar_t));
    wmemset(moveStr, L'　', rowNum * colNum);
    for (int row = 0; row < rowNum; ++row) {
        moveStr[(row + 1) * colNum - 1] = L'\n';
        //if (row % 2 == 1)
        //  moveStr[row * colNum] = L' '; // 为显示美观, 奇数行改为半角空格
    }
    moveStr[1] = L'开';
    moveStr[2] = L'始';
    moveStr[colNum + 2] = L'↓';
    moveStr[rowNum * colNum] = L'\x0';

    writeMove_PGN_CC(moveStr, colNum, cm->rootMove);
    *pmoveStr = moveStr;
}

void writeRemark_PGN_CCtoWstr(wchar_t** premStr, ChessManual cm)
{
    int size = WIDEWCHARSIZE;
    wchar_t* remarkStr = calloc(size, sizeof(wchar_t));
    writeRemark_PGN_CC(&remarkStr, &size, cm->rootMove);
    *premStr = remarkStr;
}

static void __writePGN(ChessManual cm, FILE* fout, RecFormat fmt)
{
    for (int i = 0; i < cm->infoCount; ++i)
        fwprintf(fout, L"[%s \"%s\"]\n", cm->info[i][0], cm->info[i][1]);
    fwprintf(fout, L"\n");
    if (fmt != PGN_CC)
        writeMove_PGN_ICCSZH(fout, cm->rootMove, fmt);
    else {
        wchar_t *moveStr = NULL, *remarkStr = NULL;
        writeMove_PGN_CCtoWstr(&moveStr, cm);
        writeRemark_PGN_CCtoWstr(&remarkStr, cm);
        fwprintf(fout, L"%s\n%s", moveStr, remarkStr);
        free(moveStr);
        free(remarkStr);
    }
}

void __readChessManual(ChessManual cm, const char* filename)
{
    if (filename == NULL || strlen(filename) == 0)
        return;
    const char* ext = getExt(filename);
    if (ext == NULL)
        return;
    RecFormat fmt = __getRecFormat(ext);
    if (fmt == NOTFMT) {
        wprintf(L"未实现的打开文件扩展名！");
        return;
    }
    FILE* fin = fopen(filename,
        (fmt == XQF || fmt == BIN || fmt == JSON) ? "rb" : "r");
    if (fin == NULL)
        return;
    switch (fmt) {
    case XQF:
        __readXQF(cm, fin);
        break;
    case BIN:
        __readBin(cm, fin);
        break;
    case JSON:
        __readJSON(cm, fin);
        break;
    default:
        __readPGN(cm, fin, fmt);
        break;
    }
    if (fmt == XQF || fmt == BIN || fmt == JSON)
        __getFENToSetBoard(cm);
    //*

    if (hasNext(cm->rootMove))
        __setMoveNumZhStr(cm, getNext(cm->rootMove)); // 驱动函数
        //*/
    fclose(fin);
}

void writeChessManual(ChessManual cm, const char* filename)
{
    RecFormat fmt = __getRecFormat(getExt(filename));
    if (fmt == NOTFMT) {
        wprintf(L"未实现的写入文件扩展名！");
        return;
    }
    FILE* fout = fopen(filename,
        (fmt == XQF || fmt == BIN || fmt == JSON) ? "wb" : "w");
    if (fout == NULL)
        return;
    switch (fmt) {
    case XQF:
        wprintf(L"未实现的写入文件扩展名！");
        break;
    case BIN:
        __writeBIN(cm, fout);
        break;
    case JSON:
        __writeJSON(cm, fout);
        break;
    default:
        __writePGN(cm, fout, fmt);
        break;
    }
    fclose(fout);
}

void go(ChessManual cm)
{
    if (hasNext(cm->currentMove)) {
        cm->currentMove = getNext(cm->currentMove);
        doMove(cm->board, cm->currentMove);
    }
}

void goOther(ChessManual cm)
{
    if (hasOther(cm->currentMove)) {
        undoMove(cm->board, cm->currentMove);
        cm->currentMove = getOther(cm->currentMove);
        doMove(cm->board, cm->currentMove);
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
    int index = 0;
    Move preMoves[100];
    while (move != cm->rootMove) {
        preMoves[index++] = move;
        if (hasPreOther(move)) // 本着为变着
            move = getPre(move);
        move = getPre(move);
    }
    while (--index >= 0)
        doMove(cm->board, preMoves[index]);
}

static void __doBack(ChessManual cm)
{
    undoMove(cm->board, cm->currentMove);
    cm->currentMove = getPre(cm->currentMove);
}

void back(ChessManual cm)
{
    if (hasPreOther(cm->currentMove))
        backOther(cm);
    else if (hasPre(cm->currentMove))
        __doBack(cm);
}

void backNext(ChessManual cm)
{
    if (hasPre(cm->currentMove) && !hasPreOther(cm->currentMove))
        __doBack(cm);
}

void backOther(ChessManual cm)
{
    if (hasPreOther(cm->currentMove)) {
        __doBack(cm); // 变着回退
        doMove(cm->board, cm->currentMove); // 前变执行
    }
}

void backFirst(ChessManual cm)
{
    while (hasPre(cm->currentMove))
        back(cm);
}

void backTo(ChessManual cm, Move move)
{
    while (hasPre(cm->currentMove) && cm->currentMove != move)
        back(cm);
}

void goInc(ChessManual cm, int inc)
{
    int count = abs(inc);
    if (inc > 0)
        while (count-- > 0)
            go(cm);
    else
        while (count-- > 0)
            back(cm);
}

void changeChessManual(ChessManual cm, ChangeType ct)
{
    Move curMove = cm->currentMove;
    backFirst(cm);

    // info未更改
    changeBoard(cm->board, ct);
    Move firstMove = getNext(cm->rootMove);
    if (firstMove != NULL) {
        if (ct != EXCHANGE)
            changeMove(firstMove, cm->board, ct);
        if (ct == EXCHANGE || ct == SYMMETRY) {
            cm->movCount_ = cm->remCount_ = cm->maxRemLen_ = cm->maxRow_ = cm->maxCol_ = 0;
            __setMoveNumZhStr(cm, firstMove);
        }
    }

    goTo(cm, curMove);
}

static void __transDir(const char* dirfrom, const char* dirto, RecFormat tofmt,
    int* pfcount, int* pdcount, int* pmovcount, int* premcount, int* premlenmax)
{
    long hFile = 0; //文件句柄
    struct _finddata_t fileinfo; //文件信息
    if (access(dirto, 0) != 0)
        mkdir(dirto);
    char dirName[FILENAME_MAX] = { 0 };
    strcpy(dirName, dirfrom);
    //printf("%d: %s\n", __LINE__, dirName);
    if ((hFile = _findfirst(strcat(dirName, "\\*"), &fileinfo)) == -1)
        return;

    do {
        char name[FILENAME_MAX];
        strcpy(name, fileinfo.name);
        char dir_fileName[FILENAME_MAX] = { 0 };
        strcpy(dir_fileName, dirfrom);
        strcat(strcat(dir_fileName, "\\"), name);
        //printf("%d: len:%d %s\n", __LINE__, (int)strlen(dir_fileName), dir_fileName);

        if (fileinfo.attrib & _A_SUBDIR) { //如果是目录,迭代之
            if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
                continue;
            ++*pdcount;
            char subdirto[FILENAME_MAX] = { 0 };
            strcpy(subdirto, dirto);
            strcat(strcat(subdirto, "\\"), name);
            __transDir(dir_fileName, subdirto, tofmt,
                pfcount, pdcount, pmovcount, premcount, premlenmax);
        } else { //如果是文件,执行转换
            char fromExt[FILENAME_MAX];
            strcpy_s(fromExt, FILENAME_MAX, getExt(name));
            //printf("%d: %s len:%d\n", __LINE__, name, (int)strlen(name));

            char tofilename[FILENAME_MAX] = { 0 };
            strcpy(tofilename, dirto);
            strcat(strcat(tofilename, "\\"), getFileName_cut(name));
            //printf("%d: dirto: %s fromExt: %s Recformat: %d\n",
            //    __LINE__, dirto, fromExt, __getRecFormat(fromExt));
            //
            if (__getRecFormat(fromExt) != NOTFMT) {
                printf("%s %d: %s\n", __FILE__, __LINE__, dir_fileName);
                ChessManual cm = newChessManual(dir_fileName);
                //if (__readChessManual(cm, dir_fileName) == NULL) {
                //  delChessManual(cm);
                //  return;
                //}
                strcat(tofilename, EXTNAMES[tofmt]);

                //printf("%s %d: %s\n", __FILE__, __LINE__, tofilename);
                writeChessManual(cm, tofilename);

                ++*pfcount;
                *pmovcount += cm->movCount_;
                *premcount += cm->remCount_;
                if (*premlenmax < cm->maxRemLen_)
                    *premlenmax = cm->maxRemLen_;
                delChessManual(cm);
            } else {
                //strcat(tofilename, fromExt);
                //copyFile(dir_fileName, tofilename);
                //printf("%d: %s\n", __LINE__, tofilename);
            }
            //
        }
    } while (_findnext(hFile, &fileinfo) == 0);
    _findclose(hFile);
}

void transDir(const char* dirfrom, RecFormat tofmt)
{
    int fcount = 0, dcount = 0, movcount = 0, remcount = 0, remlenmax = 0;
    char dirto[FILENAME_MAX];
    strcpy(dirto, dirfrom);
    strcat(getFileName_cut(dirto), EXTNAMES[tofmt]);
    //printf("%d: %s tofmt:%s\n", __LINE__, dirto, EXTNAMES[tofmt]);

    __transDir(dirfrom, dirto, tofmt, &fcount, &dcount, &movcount, &remcount, &remlenmax);
    wchar_t wformatStr[] = L"%s =>%s: 转换%d个文件, %d个目录成功！\n   着法数量: %d, 注释数量: %d, 最大注释长度: %d\n";
    char formatStr[WCHARSIZE];
    wcstombs(formatStr, wformatStr, WCHARSIZE);
    printf(formatStr, dirfrom, EXTNAMES[tofmt], fcount, dcount, movcount, remcount, remlenmax);
}

void testTransDir(int fromDir, int toDir,
    int fromFmtStart, int fromFmtEnd, int toFmtStart, int toFmtEnd)
{
    const wchar_t* wdirfroms[] = {
        L"c:\\棋谱\\示例文件",
        L"c:\\棋谱\\象棋杀着大全",
        L"c:\\棋谱\\疑难文件",
        L"c:\\棋谱\\中国象棋棋谱大全"
    };
    int dirCount = sizeof(wdirfroms) / sizeof(wdirfroms[0]);
    char dirfroms[dirCount][FILENAME_MAX];
    for (int i = 0; i < dirCount; ++i)
        wcstombs(dirfroms[i], wdirfroms[i], FILENAME_MAX);

    RecFormat fmts[] = { XQF, BIN, JSON, PGN_ICCS, PGN_ZH, PGN_CC };
    // 调节三个循环变量的初值、终值，控制转换目录
    for (int dir = fromDir; dir < dirCount && dir != toDir; ++dir) {
        for (int fromFmt = fromFmtStart; fromFmt != fromFmtEnd; ++fromFmt) {
            char fromDirName[FILENAME_MAX];
            strcpy(fromDirName, dirfroms[dir]);
            strcat(fromDirName, EXTNAMES[fmts[fromFmt]]);
            for (int toFmt = toFmtStart; toFmt != toFmtEnd; ++toFmt)
                if (toFmt > XQF && toFmt != fromFmt) {
                    transDir(fromDirName, fmts[toFmt]);
                }
        }
    }
}

// 测试本翻译单元各种对象、函数
void testChessManual(FILE* fout)
{
    ChessManual cm = newChessManual("01.xqf");
    //*
    writeChessManual(cm, "01.bin");

    resetChessManual(&cm, "01.bin");
    writeChessManual(cm, "01.json");

    //wchar_t wstr[WIDEWCHARSIZE];
    //wprintf(L"%s", getBoardString(wstr, cm->board));

    resetChessManual(&cm, "01.json");
    writeChessManual(cm, "01.pgn_iccs");

    resetChessManual(&cm, "01.pgn_iccs");
    writeChessManual(cm, "01.pgn_zh");

    resetChessManual(&cm, "01.pgn_zh");
    writeChessManual(cm, "01.pgn_cc");

    resetChessManual(&cm, "01.pgn_cc");
    writeChessManual(cm, "01.pgn_cc");

    //fprintf(fout, "%s: movCount:%d remCount:%d remLenMax:%d maxRow:%d maxCol:%d\n",
    //    __func__, cm->movCount_, cm->remCount_, cm->maxRemLen_, cm->maxRow_, cm->maxCol_);

    for (int ct = EXCHANGE; ct <= SYMMETRY; ++ct) {
        changeChessManual(cm, ct);
        char fname[32] = { 0 };
        sprintf(fname, "01_%d.pgn_cc", ct);
        //resetChessManual(&cm, fname); //未成功？
        writeChessManual(cm, fname);
    }
    //*/

    delChessManual(cm);
}