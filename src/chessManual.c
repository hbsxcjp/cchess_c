#define PCRE_STATIC
#include "head/chessManual.h"
#include "head/board.h"
#include "head/move.h"
#include "head/piece.h"
//#include <regex.h>
//#include <sys/types.h>

// 棋局信息数量
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
    ChessManual cm = calloc(sizeof(struct ChessManual), 1);
    cm->board = newBoard();
    cm->currentMove = cm->rootMove = newMove();
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
    int count = cm->infoCount, nameLen = wcsnlen_s(name, WCHARSIZE);
    if (count == 32 || nameLen == 0)
        return;
    cm->info[count][0] = (wchar_t*)calloc(nameLen + 1, sizeof(name[0]));
    wcscpy(cm->info[count][0], name);
    cm->info[count][1] = (wchar_t*)calloc(wcsnlen_s(value, WCHARSIZE) + 1, sizeof(value[0]));
    wcscpy(cm->info[count][1], value);
    ++(cm->infoCount);
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
{ /*
    ++cm->movCount_;
    if (move->otherNo_ > cm->maxCol_)
        cm->maxCol_ = move->otherNo_;
    if (move->nextNo_ > cm->maxRow_)
        cm->maxRow_ = move->nextNo_;

    move->CC_ColNo_ = cm->maxCol_; // # 本着在视图中的列数
    if (move->remark != NULL) {
        ++cm->remCount_;
        int maxRemLen_ = wcslen(move->remark);
        if (maxRemLen_ > cm->maxRemLen_)
            cm->maxRemLen_ = maxRemLen_;
    }

    //assert(move->fseat != NULL);
    //wprintf(L"%3d=> %02x->%02x\n", __LINE__, move->fseat, move->tseat);

    // 先深度搜索
    setMoveZhStr(move, cm->board);
    //wprintf(L"%s ", __LINE__, move->zhStr);

    doMove(cm->board, move);
    if (hasNext(move))
        __setMoveNumZhStr(cm, move->nmove);
    undoMove(cm->board, move);

    // 后广度搜索
    if (move->omove != NULL) {
        ++cm->maxCol_;
        __setMoveNumZhStr(cm, move->omove);
    }
    //*/
}

extern int Version, KeyRMKSize;
extern char KeyXYf, KeyXYt, F32Keys[PIECENUM];
extern unsigned char __calkey(unsigned char bKey, unsigned char cKey);

static void readXQF(ChessManual cm, FILE* fin)
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

static void readBIN(ChessManual cm, FILE* fin)
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
        setRemark(cm->rootMove, __readWstring_BIN(fin));
    if (tag & 0x80)
        readMove_BIN(cm->rootMove, cm->board, fin, false);
}

static void writeBIN(ChessManual cm, FILE* fout)
{
    fwrite(FILETAG, sizeof(char), sizeof(FILETAG), fout);
    char infoCount = cm->infoCount;
    char tag = ((infoCount > 0 ? 0x10 : 0x00)
        | (cm->rootMove->remark != NULL ? 0x20 : 0x00)
        | (hasNext(cm->rootMove) ? 0x80 : 0x00));
    fwrite(&tag, sizeof(char), 1, fout);
    if (tag & 0x10) {
        fwrite(&infoCount, sizeof(char), 1, fout);
        for (int i = 0; i < infoCount; ++i) {
            writeWstring_BIN(cm->info[i][0], fout);
            writeWstring_BIN(cm->info[i][1], fout);
        }
    }

    if (tag & 0x20)
        writeWstring_BIN(cm->rootMove->remark, fout);
    if (tag & 0x80)
        writeMove_BIN(cm->rootMove, fout);
}

static void readJSON(ChessManual cm, FILE* fin)
{
    fseek(fin, 0L, SEEK_END); // 定位到文件末尾
    long last = ftell(fin);
    char* insJSONString = (char*)calloc(last + 1, sizeof(char));
    fseek(fin, 0L, SEEK_SET); // 定位到文件开始
    fread(insJSONString, sizeof(char), last, fin);
    cJSON* insJSON = cJSON_Parse(insJSONString);
    free(insJSONString);

    cJSON* infoJSON = cJSON_GetObjectItem(insJSON, "info");
    int infoCount = cJSON_GetArraySize(infoJSON);
    for (int i = 0; i < infoCount; ++i) {
        cJSON* keyValueJSON = cJSON_GetArrayItem(infoJSON, i);
        wchar_t nameValue[2][WCHARSIZE] = { 0 };
        for (int j = 0; j < 2; ++j)
            mbstowcs(nameValue[j],
                cJSON_GetStringValue(cJSON_GetArrayItem(keyValueJSON, j)), WCHARSIZE);
        addInfoItem(cm, nameValue[0], nameValue[1]);
    }

    cJSON* rootMoveJSON = cJSON_GetObjectItem(insJSON, "rootmove");
    if (rootMoveJSON != NULL)
        readMove_JSON(cm->rootMove, cm->board, rootMoveJSON, false);
    cJSON_Delete(insJSON);
}

static void writeJSON(ChessManual cm, FILE* fout)
{
    cJSON *insJSON = cJSON_CreateObject(),
          *infoJSON = cJSON_CreateArray(),
          *rootmoveJSON = cJSON_CreateObject();
    for (int i = 0; i < cm->infoCount; ++i) {
        //int nameLen = wcslen(cm->info[i][0]) + 1,
        //    valueLen = wcslen(cm->info[i][1]) + 1;
        char name[WCHARSIZE], value[WCHARSIZE];
        wcstombs(name, cm->info[i][0], WCHARSIZE);
        wcstombs(value, cm->info[i][1], WCHARSIZE);
        cJSON_AddItemToArray(infoJSON,
            cJSON_CreateStringArray((const char* const[]){ name, value }, 2));
    }
    cJSON_AddItemToObject(insJSON, "info", infoJSON);

    writeMove_JSON(rootmoveJSON, cm->rootMove);
    cJSON_AddItemToObject(insJSON, "rootmove", rootmoveJSON);

    char* insJSONString = cJSON_Print(insJSON);
    fwrite(insJSONString, sizeof(char), strlen(insJSONString) + 1, fout);
    cJSON_Delete(insJSON);
}

static void readInfo_PGN(ChessManual cm, FILE* fin)
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

extern const wchar_t PRECHAR[4];
extern const wchar_t MOVCHAR[4];
extern const wchar_t NUMCHAR[PIECECOLORNUM][BOARDCOL + 1];
extern const wchar_t* PieceNames[PIECECOLORNUM];
extern const wchar_t ICCSCOLCHAR[BOARDCOL + 1];
extern const wchar_t ICCSROWCHAR[BOARDROW + 1];

static void readMove_PGN_ICCSZH(ChessManual cm, FILE* fin, RecFormat fmt)
{
    bool isPGN_ZH = fmt == PGN_ZH;
    wchar_t ICCSZHStr[WIDEWCHARSIZE] = { 0 };
    wcscat(ICCSZHStr, L"([");
    if (isPGN_ZH) {
        wchar_t ZhChars[WCHARSIZE] = { 0 };
        wcscat(ZhChars, PRECHAR);
        wcscat(ZhChars, PieceNames[RED]);
        wcscat(ZhChars, PieceNames[BLACK]);
        wcscat(ZhChars, MOVCHAR);
        wcscat(ZhChars, NUMCHAR[RED]);
        wcscat(ZhChars, NUMCHAR[BLACK]);
        wcscat(ICCSZHStr, ZhChars);
    } else {
        wchar_t ICCSChars[WCHARSIZE] = { 0 };
        wcscat(ICCSChars, ICCSCOLCHAR);
        wcscat(ICCSChars, ICCSROWCHAR);
        wcscat(ICCSZHStr, ICCSChars);
    }
    wcscat(ICCSZHStr, L"]{4})");

    const wchar_t remStr[] = L"(?:[\\s\\n]*\\{([\\s\\S]*?)\\})?";
    wchar_t movePat[WCHARSIZE] = { 0 };
    wcscat(movePat, L"(\\()?(?:[\\d\\.\\s]+)");
    wcscat(movePat, ICCSZHStr);
    wcscat(movePat, remStr);
    wcscat(movePat, L"(?:[\\s\\n]*(\\)+))?"); // 可能存在多个右括号
    wchar_t remPat[WCHARSIZE] = { 0 };
    wcscat(remPat, remStr);
    wcscat(remPat, L"1\\.");

    const char* error;
    int erroffset = 0, ovector[INFOSIZE];
    pcre16* moveReg = pcre16_compile(movePat, 0, &error, &erroffset, NULL);
    assert(moveReg);
    pcre16* remReg = pcre16_compile(remPat, 0, &error, &erroffset, NULL);
    assert(remReg);

    wchar_t* moveStr = getWString(fin);
    int regCount = pcre16_exec(remReg, NULL, moveStr, wcslen(moveStr),
        0, 0, ovector, INFOSIZE);
    if (regCount <= 0)
        return; // 没有move
    if (regCount == 2) {
        int len = ovector[3] - ovector[2] + 1;
        wchar_t* remark = malloc(len * sizeof(wchar_t));
        wcsncpy(remark, moveStr + ovector[2], len);
        remark[len - 1] = L'\x0';
        setRemark(cm->rootMove, remark); // 赋值一个动态分配内存的指针
    }

    Move move = NULL,
         preMove = cm->rootMove,
         preOtherMoves[WIDEWCHARSIZE] = { NULL };
    int preOthIndex = 0, length = 0;
    wchar_t* pmoveStr = moveStr;
    while ((pmoveStr += ovector[1]) && (length = wcslen(pmoveStr)) > 0) {
        regCount = pcre16_exec(moveReg, NULL, pmoveStr,
            length, 0, 0, ovector, INFOSIZE);
        if (regCount <= 0)
            break;

        // 第1个子匹配成功，有"("
        /*
        if (ovector[3] > ovector[2]) {
            move = addOther(preMove);
            preOtherMoves[preOthIndex++] = preMove;
            if (isPGN_ZH)
                undoMove(cm->board, preMove);
        } else
            move = addNext(preMove);

        // 提取第2个匹配zhStr，设置move
        int num = ovector[5] - ovector[4];
        assert(num > 0);
        wchar_t iccs_zhStr[6] = { 0 };
        wcsncpy(iccs_zhStr, pmoveStr + ovector[4], num);

        if (isPGN_ZH)
            __setMoveSeat_zh(move, cm->board, iccs_zhStr);
        else
            __setMoveSeat_iccs(move, cm->board, iccs_zhStr);

        // 提取第3个匹配remark，设置move
        num = ovector[7] - ovector[6];
        if (num > 0) {
            wchar_t* remark = malloc((num + 1) * sizeof(wchar_t));
            wcsncpy(remark, pmoveStr + ovector[6], num);
            remark[num] = L'\x0';
            setRemark(move, remark);
        }
            //*/

        bool isOther = ovector[3] > ovector[2];
        int iccs_zhSize = ovector[5] - ovector[4];
        assert(iccs_zhSize > 0);
        wchar_t iccs_zhStr[6] = { 0 };
        wcsncpy(iccs_zhStr, pmoveStr + ovector[4], iccs_zhSize);

        int remarkSize = ovector[7] - ovector[6];
        wchar_t* remark = NULL;
        if (remarkSize > 0) {
            remark = malloc((remarkSize + 1) * sizeof(wchar_t));
            wcsncpy(remark, pmoveStr + ovector[6], remarkSize);
            remark[remarkSize] = L'\x0';
        }
        move = isPGN_ZH ? addMove_zh(preMove, cm->board, iccs_zhStr, remark, isOther)
                        : addMove_iccs(preMove, cm->board, iccs_zhStr, remark, isOther);
        if (isOther) {
            preOtherMoves[preOthIndex++] = preMove;
            if (isPGN_ZH)
                undoMove(cm->board, preMove);
        }

        if (isPGN_ZH)
            doMove(cm->board, move);

        // 第4个子匹配成功，有")+"
        int num = ovector[9] - ovector[8];
        if (num > 0) {
            for (int i = 0; i < num; ++i) {
                preMove = preOtherMoves[--preOthIndex];
                if (isPGN_ZH) {
                    do {
                        undoMove(cm->board, move);
                        move = move->pmove;
                    } while (move != preMove);
                    doMove(cm->board, preMove);
                }
            }
        } else
            preMove = move;
    }
    if (isPGN_ZH)
        while (move != cm->rootMove) {
            undoMove(cm->board, move);
            move = move->pmove;
        }
    free(moveStr);
    pcre16_free(remReg);
    pcre16_free(moveReg);
}

static void __writeMove_PGN_ICCSZH(ChessManual cm, FILE* fout, Move move,
    bool isPGN_ZH, bool isOther)
{
    wchar_t boutStr[6] = { 0 }, iccs_zhStr[6] = { 0 };
    swprintf(boutStr, 6, L"%d. ", (move->nextNo_ + 1) / 2);
    bool isEven = move->nextNo_ % 2 == 0;
    if (isOther) {
        fwprintf(fout, L"(%s", boutStr);
        if (isEven)
            fwprintf(fout, L"... ");
    } else
        fwprintf(fout, isEven ? L" " : boutStr);

    fwprintf(fout, isPGN_ZH ? move->zhStr : getICCS(iccs_zhStr, move));
    fwprintf(fout, L" ");
    if (move->remark != NULL)
        fwprintf(fout, L" \n{%s}\n ", move->remark);

    if (move->omove != NULL) {
        __writeMove_PGN_ICCSZH(cm, fout, move->omove, isPGN_ZH, true);
        fwprintf(fout, L")");
    }

    //if (isPGN_ZH)
    //    doMove(cm->board, move); // 执行本着
    if (hasNext(move))
        __writeMove_PGN_ICCSZH(cm, fout, move->nmove, isPGN_ZH, false);
    //if (isPGN_ZH)
    //    undoMove(cm->board, move); // 退回本着
}

static void writeMove_PGN_ICCSZH(ChessManual cm, FILE* fout, RecFormat fmt)
{
    if (cm->rootMove->remark != NULL)
        fwprintf(fout, L" \n{%s}\n ", cm->rootMove->remark);
    //backFirst(cm);
    if (hasNext(cm->rootMove))
        __writeMove_PGN_ICCSZH(cm, fout, cm->rootMove->nmove, fmt == PGN_ZH, false);
    fwprintf(fout, L"\n");
}

static wchar_t* __getRemark_PGN_CC(wchar_t* remLines[], int remCount, int row, int col)
{
    wchar_t* remark = NULL;
    wchar_t name[12] = { 0 };
    swprintf(name, 12, L"(%d,%d)", row, col);
    for (int index = 0; index < remCount; ++index)
        if (wcscmp(name, remLines[index * 2]) == 0) {
            remark = malloc((wcslen(remLines[index * 2 + 1]) + 1) * sizeof(wchar_t));
            wcscpy(remark, remLines[index * 2 + 1]);
            //wprintf(L"%d: %s: %s\n", __LINE__, name, remLines[index * 2 + 1]);
            break;
        }
    return remark;
}

static void __setMove_PGN_CC(ChessManual cm, Move preMove, pcre16* moveReg,
    wchar_t* moveLines[], int rowNum, int colNum, int row, int col,
    wchar_t* remLines[], int remCount, bool isOther)
{
    wchar_t* zhStr = moveLines[row * colNum + col];
    while (zhStr[0] == L'…')
        zhStr = moveLines[row * colNum + (++col)];
    int regCount = 0, ovector[8]; //OVECCOUNT = 8,
    regCount = pcre16_exec(moveReg, NULL, zhStr, wcslen(zhStr), 0, 0, ovector, 8);
    assert(regCount > 0);

    wchar_t lastwc = zhStr[4];
    zhStr[4] = L'\x0';
    //__setMoveSeat_zh(move, cm->board, zhStr);
    //__setRemark_PGN_CC(move, row, col, remLines, remCount);
    Move move = addMove_zh(preMove, cm->board, zhStr,
        __getRemark_PGN_CC(remLines, remCount, row, col), isOther);

    if (lastwc == L'…')
        __setMove_PGN_CC(cm, move, moveReg,
            moveLines, rowNum, colNum, row, col + 1, remLines, remCount, true);

    if (move->nextNo_ < rowNum - 1
        && moveLines[(row + 1) * colNum + col][0] != L'　') {
        doMove(cm->board, move);
        __setMove_PGN_CC(cm, move, moveReg,
            moveLines, rowNum, colNum, row + 1, col, remLines, remCount, false);
        undoMove(cm->board, move);
    }
}

static void readMove_PGN_CC(ChessManual cm, FILE* fin)
{
    const wchar_t movePat[] = L"([^…　]{4}[…　])",
                  remPat[] = L"(\\(\\d+,\\d+\\)): \\{([\\s\\S]*?)\\}";
    const char* error;
    int erroffset = 0;
    pcre16* moveReg = pcre16_compile(movePat, 0, &error, &erroffset, NULL);
    pcre16* remReg = pcre16_compile(remPat, 0, &error, &erroffset, NULL);
    assert(moveReg);
    assert(remReg);

    wchar_t *moveLines[WIDEWCHARSIZE * 2], lineStr[WIDEWCHARSIZE];
    int rowNum = 0, colNum = -1;
    while (fgetws(lineStr, WIDEWCHARSIZE, fin) && lineStr[0] != L'\n') { // 空行截止
        //wprintf(L"%d: %s", __LINE__, lineStr);
        if (colNum < 0) // 只计算一次
            colNum = wcslen(lineStr) / 5;
        for (int col = 0; col < colNum; ++col) {
            wchar_t* moveStr = calloc(6, sizeof(wchar_t));
            wcsncpy(moveStr, lineStr + col * 5, 5);
            moveLines[rowNum * colNum + col] = moveStr;
            //wprintf(L"%d: %s\n", __LINE__, moveLines[rowNum * colNum + col]);
        }
        ++rowNum;
        fgetws(lineStr, WIDEWCHARSIZE, fin); // 弃掉间隔行
    }

    int remCount = 0, regCount = 0, ovector[INFOSIZE];
    wchar_t *allremarksStr = getWString(fin),
            *remLines[WIDEWCHARSIZE],
            *remStr = NULL;
    ovector[1] = 0;
    remStr = allremarksStr;
    while (wcslen(remStr) > 0) {
        regCount = pcre16_exec(remReg, NULL, remStr, wcslen(remStr),
            0, 0, ovector, INFOSIZE);
        if (regCount <= 0)
            break;
        int rclen = ovector[3] - ovector[2], remlen = ovector[5] - ovector[4];
        wchar_t *rowcolName = malloc((rclen + 1) * sizeof(wchar_t)),
                *reamrkStr = malloc((remlen + 1) * sizeof(wchar_t));
        wcsncpy(rowcolName, remStr + ovector[2], rclen);
        wcsncpy(reamrkStr, remStr + ovector[4], remlen);
        rowcolName[rclen] = L'\x0';
        reamrkStr[remlen] = L'\x0';
        remLines[remCount * 2] = rowcolName;
        remLines[remCount * 2 + 1] = reamrkStr;
        //wprintf(L"%d: %s: %s\n", __LINE__, remLines[remCount * 2], remLines[remCount * 2 + 1]);
        ++remCount;
        remStr += ovector[1];
    }

    setRemark(cm->rootMove, __getRemark_PGN_CC(remLines, remCount, 0, 0));
    if (rowNum > 0)
        //__setMove_PGN_CC(cm, addNext(cm->rootMove), moveReg, moveLines, rowNum, colNum, 1, 0, remLines, remCount);
        __setMove_PGN_CC(cm, cm->rootMove, moveReg, moveLines, rowNum, colNum, 1, 0, remLines, remCount, false);

    if (allremarksStr)
        free(allremarksStr);
    for (int i = rowNum * colNum - 1; i >= 0; --i)
        free(moveLines[i]);
    for (int i = 0; i < remCount; ++i) {
        free(remLines[i * 2]);
        free(remLines[i * 2 + 1]);
    }
    pcre16_free(remReg);
    pcre16_free(moveReg);
}

static void __writeMove_PGN_CC(wchar_t* moveStr, int colNum, ChessManual cm, Move move)
{
    int row = move->nextNo_ * 2, firstCol = move->CC_ColNo_ * 5;
    wcsncpy(&moveStr[row * colNum + firstCol], move->zhStr, 4);

    if (move->omove != NULL) {
        int fcol = firstCol + 4, tnum = move->omove->CC_ColNo_ * 5 - fcol;
        wmemset(&moveStr[row * colNum + fcol], L'…', tnum);
        __writeMove_PGN_CC(moveStr, colNum, cm, move->omove);
    }

    if (hasNext(move)) {
        moveStr[(row + 1) * colNum + firstCol + 2] = L'↓';
        __writeMove_PGN_CC(moveStr, colNum, cm, move->nmove);
    }
}

void writeMove_PGN_CCtoWstr(ChessManual cm, wchar_t** pmoveStr)
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

    //backFirst(cm);
    if (hasNext(cm->rootMove))
        __writeMove_PGN_CC(moveStr, colNum, cm, cm->rootMove->nmove);

    *pmoveStr = moveStr;
}

static void __writeRemark_PGN_CC(wchar_t* remarkStr, long* premSize, Move move)
{
    if (move->remark != NULL) {
        int len = wcslen(move->remark) + 16;
        wchar_t* remStr = calloc(len, sizeof(wchar_t));
        swprintf(remStr, len, L"(%d,%d): {%s}\n", move->nextNo_, move->CC_ColNo_, move->remark);
        // 如字符串分配的长度不够，则增加长度
        if (wcslen(remarkStr) + len > *premSize - 1) {
            *premSize += WIDEWCHARSIZE;
            remarkStr = realloc(remarkStr, (*premSize) * sizeof(wchar_t));
        }
        wcscat(remarkStr, remStr);
        free(remStr);
    }

    if (move->omove != NULL)
        __writeRemark_PGN_CC(remarkStr, premSize, move->omove);
    if (hasNext(move))
        __writeRemark_PGN_CC(remarkStr, premSize, move->nmove);
}

void writeRemark_PGN_CCtoWstr(ChessManual cm, wchar_t** premarkStr)
{
    long remSize = SUPERWIDEWCHARSIZE;
    wchar_t* remarkStr = calloc(remSize, sizeof(wchar_t));
    __writeRemark_PGN_CC(remarkStr, &remSize, cm->rootMove);
    *premarkStr = remarkStr;
}

static void writeMove_PGN_CC(ChessManual cm, FILE* fout)
{
    wchar_t *moveStr = NULL, *remarkStr = NULL;
    writeMove_PGN_CCtoWstr(cm, &moveStr);
    writeRemark_PGN_CCtoWstr(cm, &remarkStr);

    fwprintf(fout, L"%s\n%s", moveStr, remarkStr);
    free(remarkStr);
    free(moveStr);
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
        //printf("%d:%s %d\n", __LINE__, ext, fmt);
        readXQF(cm, fin);
        //
        //if (cm->rootMove->remark != NULL) {
        //    wchar_t wfname[FILENAME_MAX];
        //    mbstowcs(wfname, filename, FILENAME_MAX);
        //    wprintf(L"%3d filename: %s\nrootMove_remark: %s\n",
        //        __LINE__, wfname, cm->rootMove->remark);
        //}
        //
        break;
    case BIN:
        readBIN(cm, fin);
        break;
    case JSON:
        readJSON(cm, fin);
        break;
    default:
        readInfo_PGN(cm, fin);
        // PGN_ZH, PGN_CC在读取move之前需要先设置board
        __getFENToSetBoard(cm);
        switch (fmt) {
        case PGN_CC:
            readMove_PGN_CC(cm, fin);
            break;
        default: //PGN_ZH PGN_ICCS
            readMove_PGN_ICCSZH(cm, fin, fmt);
            break;
        }
        break;
    }
    if (fmt == XQF || fmt == BIN || fmt == JSON)
        __getFENToSetBoard(cm);

    if (hasNext(cm->rootMove))
        __setMoveNumZhStr(cm, cm->rootMove->nmove); // 驱动函数
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
        writeBIN(cm, fout);
        break;
    case JSON:
        writeJSON(cm, fout);
        break;
    default:
        for (int i = 0; i < cm->infoCount; ++i)
            fwprintf(fout, L"[%s \"%s\"]\n", cm->info[i][0], cm->info[i][1]);
        fwprintf(fout, L"\n");
        switch (fmt) {
        case PGN_CC:
            writeMove_PGN_CC(cm, fout);
            break;
        default: //PGN_ZH PGN_ICCS
            writeMove_PGN_ICCSZH(cm, fout, fmt);
            break;
        }
        //
        //wchar_t tempStr[SUPERWIDEWCHARSIZE] = { 0 };
        //fwprintf(fout, L"\n%s", getBoardString(tempStr, cm->board));
        //fwprintf(fout, L"MoveInfo: movCount:%d remCount:%d remLenMax:%d maxRow:%d maxCol:%d\n\n",
        //    cm->movCount_, cm->remCount_, cm->maxRemLen_, cm->maxRow_, cm->maxCol_);
        //
        break;
    }
    fclose(fout);
}

void go(ChessManual cm)
{
    if (hasNext(cm->currentMove)) {
        cm->currentMove = cm->currentMove->nmove;
        doMove(cm->board, cm->currentMove);
    }
}

void goOther(ChessManual cm)
{
    if (hasOther(cm->currentMove)) {
        undoMove(cm->board, cm->currentMove);
        cm->currentMove = cm->currentMove->omove;
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
        if (move->pmove->omove == move) // 本着为变着
            move = move->pmove;
        move = move->pmove;
    }
    while (--index >= 0)
        doMove(cm->board, preMoves[index]);
}

static void __doBack(ChessManual cm)
{
    undoMove(cm->board, cm->currentMove);
    cm->currentMove = cm->currentMove->pmove;
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
    Move firstMove = cm->rootMove->nmove;
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
                ChessManual cm = newChessManual(dir_fileName);
                //printf("%s %d: %s\n", __FILE__, __LINE__, dir_fileName);
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
    writeChessManual(cm, "01.bin");

    resetChessManual(&cm, "01.bin");
    /*
    writeChessManual(cm, "01.json");
    resetChessManual(&cm, "01.json");
    writeChessManual(cm, "01.pgn_iccs");

    resetChessManual(&cm, "01.pgn_iccs");
    writeChessManual(cm, "01.pgn_zh");

    resetChessManual(&cm, "01.pgn_zh");
    writeChessManual(cm, "01.pgn_cc");

    resetChessManual(&cm, "01.pgn_cc");
    writeChessManual(cm, "01.pgn_cc");
    //

    fprintf(fout, "%s: movCount:%d remCount:%d remLenMax:%d maxRow:%d maxCol:%d\n",
        __func__, cm->movCount_, cm->remCount_, cm->maxRemLen_, cm->maxRow_, cm->maxCol_);

    //
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
