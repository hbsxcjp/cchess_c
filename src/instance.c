#define PCRE_STATIC
#include "head/instance.h"
#include "head/board.h"
#include "head/cJSON.h"
#include "head/move.h"
#include "head/piece.h"
#include "head/tools.h"
//#include <regex.h>
//#include <sys/types.h>
#include "pcre.h"

static wchar_t FEN_0[] = L"rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR";

Instance* newInstance(void)
{
    Instance* ins = malloc(sizeof(Instance));
    memset(ins, 0, sizeof(Instance));
    ins->board = newBoard();
    ins->rootMove = newMove();
    ins->currentMove = ins->rootMove;
    ins->infoCount = ins->movCount_ = ins->remCount_ = ins->maxRemLen_ = ins->maxRow_ = ins->maxCol_ = 0;
    return ins;
}

void delInstance(Instance* ins)
{
    ins->currentMove = NULL;
    for (int i = ins->infoCount - 1; i >= 0; --i)
        for (int j = 0; j < 2; ++j)
            free(ins->info[i][j]);
    delMove(ins->rootMove);
    free(ins->board);
    free(ins);
}

void addInfoItem(Instance* ins, const wchar_t* name, const wchar_t* value)
{
    int count = ins->infoCount, nameLen = wcslen(name);
    if (count == 32 || nameLen == 0)
        return;
    ins->info[count][0] = (wchar_t*)calloc(nameLen + 1, sizeof(name[0]));
    wcscpy(ins->info[count][0], name);
    ins->info[count][1] = (wchar_t*)calloc(wcslen(value) + 1, sizeof(value[0]));
    wcscpy(ins->info[count][1], value);
    ++(ins->infoCount);
}

// 根据文件扩展名取得存储记录类型
static RecFormat __getRecFormat(const char* ext)
{
    static const char* EXTNAMES[] = {
        ".xqf", ".pgn_iccs", ".pgn_zh", ".pgn_cc", ".bin", ".json"
    };
    for (int fmt = XQF; fmt <= JSON; ++fmt)
        if (strcmp(ext, EXTNAMES[fmt]) == 0)
            return fmt;
    return XQF;
}

static wchar_t* getFEN_ins(Instance* ins)
{
    wchar_t* FEN = FEN_0;
    bool hasFEN = false;
    for (int i = 0; i < ins->infoCount; ++i)
        if (wcscmp(ins->info[i][0], L"FEN") == 0) {
            FEN = ins->info[i][1];
            hasFEN = true;
            break;
        }
    if (!hasFEN)
        addInfoItem(ins, L"FEN", FEN_0);
    return FEN;
}

inline static void __doMove(Instance* ins, Move* move)
{
    move->tpiece = moveTo(ins->board, move->fseat, move->tseat, BLANKPIECE);
}

inline static void __undoMove(Instance* ins, Move* move)
{
    moveTo(ins->board, move->tseat, move->fseat, move->tpiece);
}

// 供readXQF使用的有关解密钥匙
static int Version = 0, KeyRMKSize = 0;
static unsigned char KeyXYf = 0, KeyXYt = 0, F32Keys[PIECENUM] = { 0 };

inline static unsigned char __calkey(unsigned char bKey, unsigned char cKey)
{
    return (((((bKey * bKey) * 3 + 9) * 3 + 8) * 2 + 1) * 3 + 8) * cKey; // % 256; // 保持为<256
}

inline static unsigned char __sub(unsigned char a, unsigned char b)
{
    return a - b; // 保持为<256
}

static void __readBytes(char* bytes, int size, FILE* fin)
{
    long pos = ftell(fin);
    fread(bytes, sizeof(char), size, fin);
    if (Version > 10) // '字节解密'
        for (int i = 0; i != size; ++i)
            bytes[i] = __sub(bytes[i], F32Keys[(pos + i) % 32]);
}

static void __readMoveData(char* data, wchar_t* remark, FILE* fin)
{
    remark[0] = L'\x0';
    __readBytes(data, 4, fin);
    if (Version <= 10)
        data[2] = (data[2] & 0xF0 ? 0x80 : 0) | (data[2] & 0x0F ? 0x40 : 0);
    else
        data[2] &= 0xE0;
    if (Version <= 10 || (data[2] & 0x20)) {
        char clen[4] = { 0 };
        __readBytes(clen, 4, fin);
        int RemarkSize = *(__int32*)clen - KeyRMKSize;
        if (RemarkSize > 0) {
            char rem[REMARKSIZE] = { 0 };
            __readBytes(rem, RemarkSize, fin);
            mbstowcs(remark, rem, REMARKSIZE - 1);
        }
    }
}

static void __readMove(Move* move, FILE* fin)
{
    char data[4] = { 0 };
    wchar_t remark[REMARKSIZE] = { 0 };
    __readMoveData(data, remark, fin);
    //# 一步棋的起点和终点有简单的加密计算，读入时需要还原
    int fcolrow = __sub(data[0], 0X18 + KeyXYf),
        tcolrow = __sub(data[1], 0X20 + KeyXYt);
    //wprintf(L"%3d=> %d->%d\n", __LINE__, fcolrow, tcolrow);
    // assert(fcolrow <= 89 && tcolrow <= 89); 根节点不能断言！
    move->fseat = getSeat_rc(fcolrow % 10, fcolrow / 10);
    move->tseat = getSeat_rc(tcolrow % 10, tcolrow / 10);
    setRemark(move, remark);

    if (data[2] & 0x80) //# 有左子树
        __readMove(addNext(move), fin);
    if (data[2] & 0x40) // # 有右子树
        __readMove(addOther(move), fin);
}

void setMoveNums(Instance* ins, Move* move)
{
    ++ins->movCount_;
    if (move->otherNo_ > ins->maxCol_)
        ins->maxCol_ = move->otherNo_;
    if (move->nextNo_ > ins->maxRow_)
        ins->maxRow_ = move->nextNo_;

    move->CC_ColNo_ = ins->maxCol_; // # 本着在视图中的列数
    if (move->remark != NULL) {
        ++ins->remCount_;
        int maxRemLen_ = wcslen(move->remark);
        if (maxRemLen_ > ins->maxRemLen_)
            ins->maxRemLen_ = maxRemLen_;
    }

    assert(move->fseat >= 0);
    //wprintf(L"%3d=> %02x->%02x\n", __LINE__, move->fseat, move->tseat);
    //__doMove(ins, move);
    if (move->nmove != NULL)
        setMoveNums(ins, move->nmove); // 先深度搜索
    //__undoMove(ins, move);

    if (move->omove != NULL) {
        ++ins->maxCol_;
        setMoveNums(ins, move->omove); // 后广度搜索
    }
}

static void readXQF(Instance* ins, FILE* fin)
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
    //setBoard(ins->board, pieChars);

    wchar_t tempStr[REMARKSIZE] = { 0 };
    char* values[] = {
        TitleA, Event, Date, Site, Red, Black,
        Opening, RMKWriter, Author
    };
    wchar_t* names[] = {
        L"TitleA", L"Event", L"Date", L"Site", L"Red", L"Black",
        L"Opening", L"RMKWriter", L"Author"
    };
    for (int i = 0; i != sizeof(names) / sizeof(names[0]); ++i) {
        mbstowcs(tempStr, values[i], REMARKSIZE - 1);
        addInfoItem(ins, names[i], tempStr); // 多字节字符串存储
    }
    wchar_t* PlayType[] = { L"全局", L"开局", L"中局", L"残局" };
    addInfoItem(ins, L"PlayType", PlayType[(int)(headCodeA_H[0])]); // 编码定义存储
    getFEN(tempStr, pieChars);
    addInfoItem(ins, L"FEN", wcscat(tempStr, headWhoPlay ? L" -r" : L" -b")); // 转换FEN存储
    wchar_t* Result[] = { L"未知", L"红胜", L"黑胜", L"和棋" };
    addInfoItem(ins, L"Result", Result[(int)headPlayResult]); // 编码定义存储
    swprintf(tempStr, REMARKSIZE, L"%d", (int)Version);
    addInfoItem(ins, L"Version", tempStr); // 整数存储

    fseek(fin, 1024, SEEK_SET);
    __readMove(ins->rootMove, fin);
}

static void __readWstring_BIN(wchar_t* wstr, FILE* fin)
{
    int len = 0;
    fread(&len, sizeof(int), 1, fin);
    fread(wstr, sizeof(wchar_t), len, fin);
}

static void __readRemark_BIN(Move* move, FILE* fin)
{
    wchar_t remark[REMARKSIZE] = { 0 };
    __readWstring_BIN(remark, fin);
    setRemark(move, remark);
}

static void __readMove_BIN(Move* move, FILE* fin)
{
    fread(&move->fseat, sizeof(int), 1, fin);
    fread(&move->tseat, sizeof(int), 1, fin);
    int tag = 0;
    fread(&tag, sizeof(int), 1, fin);
    if (tag & 0x20)
        __readRemark_BIN(move, fin);

    if (tag & 0x80)
        __readMove_BIN(addNext(move), fin);
    if (tag & 0x40)
        __readMove_BIN(addOther(move), fin);
}

static void __getFENToSetBoard(Instance* ins)
{
    wchar_t* FEN = getFEN_ins(ins);
    wchar_t pieChars[SEATNUM + 1] = { 0 };
    setBoard(ins->board, getPieChars_F(pieChars, FEN, wcslen(FEN)));
}

static void readBIN(Instance* ins, FILE* fin)
{
    int tag = 0, infoCount = 0;
    fread(&tag, sizeof(int), 1, fin);
    if (tag & 0x10) {
        fread(&infoCount, sizeof(int), 1, fin);
        for (int i = 0; i < infoCount; ++i) {
            wchar_t name[REMARKSIZE] = { 0 }, value[REMARKSIZE] = { 0 };
            __readWstring_BIN(name, fin);
            __readWstring_BIN(value, fin);
            addInfoItem(ins, name, value);
        }
    }
    /*
    wchar_t* FEN = getFEN_ins(ins);
    wchar_t pieChars[SEATNUM + 1] = { 0 };
    setBoard(ins->board, getPieChars_F(pieChars, FEN, wcslen(FEN)));
    //*/

    __readMove_BIN(ins->rootMove, fin);
}

static void __writeWstring_BIN(const wchar_t* wstr, FILE* fout)
{
    int len = wcslen(wstr) + 1;
    fwrite(&len, sizeof(int), 1, fout);
    fwrite(wstr, sizeof(wchar_t), len, fout);
}

static void __writeMove_BIN(const Move* move, FILE* fout)
{
    fwrite(&move->fseat, sizeof(int), 1, fout);
    fwrite(&move->tseat, sizeof(int), 1, fout);
    int tag = ((move->nmove != NULL ? 0x80 : 0x00)
        | (move->omove != NULL ? 0x40 : 0x00)
        | (move->remark != NULL ? 0x20 : 0x00));
    fwrite(&tag, sizeof(int), 1, fout);
    if (tag & 0x20)
        __writeWstring_BIN(move->remark, fout);
    if (tag & 0x80)
        __writeMove_BIN(move->nmove, fout);
    if (tag & 0x40)
        __writeMove_BIN(move->omove, fout);
}

static void writeBIN(const Instance* ins, FILE* fout)
{
    int tag = ((ins->infoCount > 0 ? 0x10 : 0x00)
        | (ins->rootMove->remark != NULL ? 0x20 : 0x00)
        | (ins->rootMove->nmove != NULL ? 0x80 : 0x00));
    fwrite(&tag, sizeof(int), 1, fout);
    if (tag & 0x10) {
        fwrite(&ins->infoCount, sizeof(int), 1, fout);
        for (int i = 0; i < ins->infoCount; ++i) {
            __writeWstring_BIN(ins->info[i][0], fout);
            __writeWstring_BIN(ins->info[i][1], fout);
        }
    }

    __writeMove_BIN(ins->rootMove, fout);
}

static void __readMove_JSON(const cJSON* moveJSON, Move* move)
{
    move->fseat = cJSON_GetObjectItem(moveJSON, "f")->valueint;
    move->tseat = cJSON_GetObjectItem(moveJSON, "t")->valueint;
    cJSON* remarkJSON = cJSON_GetObjectItem(moveJSON, "r");
    if (remarkJSON != NULL) {
        wchar_t remark[REMARKSIZE] = { 0 };
        mbstowcs(remark, remarkJSON->valuestring, REMARKSIZE - 1);
        setRemark(move, remark);
    }

    cJSON* nmoveJSON = cJSON_GetObjectItem(moveJSON, "n");
    if (nmoveJSON != NULL)
        __readMove_JSON(nmoveJSON, addNext(move));

    cJSON* omoveJSON = cJSON_GetObjectItem(moveJSON, "o");
    if (omoveJSON != NULL)
        __readMove_JSON(omoveJSON, addOther(move));
}

static void readJSON(Instance* ins, FILE* fin)
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
        wchar_t nameValue[2][REMARKSIZE] = { 0 };
        for (int j = 0; j < 2; ++j)
            mbstowcs(nameValue[j],
                cJSON_GetStringValue(cJSON_GetArrayItem(keyValueJSON, j)), REMARKSIZE - 1);
        addInfoItem(ins, nameValue[0], nameValue[1]);
    }

    cJSON* rootMoveJSON = cJSON_GetObjectItem(insJSON, "rootmove");
    if (rootMoveJSON != NULL)
        __readMove_JSON(rootMoveJSON, ins->rootMove);
    cJSON_Delete(insJSON);
}

static void __writeMove_JSON(cJSON* moveJSON, const Move* move)
{
    cJSON_AddNumberToObject(moveJSON, "f", move->fseat);
    cJSON_AddNumberToObject(moveJSON, "t", move->tseat);
    if (move->remark != NULL) {
        char remark[REMARKSIZE] = { 0 };
        wcstombs(remark, move->remark, REMARKSIZE - 1);
        cJSON_AddStringToObject(moveJSON, "r", remark);
    }

    if (move->nmove != NULL) {
        cJSON* nmoveJSON = cJSON_CreateObject();
        __writeMove_JSON(nmoveJSON, move->nmove);
        cJSON_AddItemToObject(moveJSON, "n", nmoveJSON);
    }
    if (move->omove != NULL) {
        cJSON* omoveJSON = cJSON_CreateObject();
        __writeMove_JSON(omoveJSON, move->omove);
        cJSON_AddItemToObject(moveJSON, "o", omoveJSON);
    }
}

static void writeJSON(const Instance* ins, FILE* fout)
{
    cJSON *insJSON = cJSON_CreateObject(),
          *infoJSON = cJSON_CreateArray(),
          *rootmoveJSON = cJSON_CreateObject();
    for (int i = 0; i < ins->infoCount; ++i) {
        char name[REMARKSIZE] = { 0 }, value[REMARKSIZE] = { 0 };
        wcstombs(name, ins->info[i][0], REMARKSIZE - 1);
        wcstombs(value, ins->info[i][1], REMARKSIZE - 1);
        cJSON_AddItemToArray(infoJSON,
            cJSON_CreateStringArray((const char* const[]){ name, value }, 2));
    }
    cJSON_AddItemToObject(insJSON, "info", infoJSON);

    __writeMove_JSON(rootmoveJSON, ins->rootMove);
    cJSON_AddItemToObject(insJSON, "rootmove", rootmoveJSON);

    char* insJSONString = cJSON_Print(insJSON);
    fwrite(insJSONString, sizeof(char), strlen(insJSONString) + 1, fout);
    cJSON_Delete(insJSON);
}

static void readInfo_PGN(Instance* ins, FILE* fin)
{
    const char* error;
    int erroffset = 0, infoCount = 0, OVECCOUNT = 10, ovector[OVECCOUNT];
    const wchar_t* infoPat = L"\\[(\\w+)\\s+\"([\\s\\S]*?)\"\\]";
    pcre16* infoReg = pcre16_compile(infoPat, 0, &error, &erroffset, NULL);
    assert(infoReg);
    wchar_t lineStr[REMARKSIZE] = { 0 };
    while (fgetws(lineStr, REMARKSIZE, fin) && lineStr[0] != L'\n') { // 以空行为终止特征
        infoCount = pcre16_exec(infoReg, NULL, lineStr, wcslen(lineStr),
            0, 0, ovector, OVECCOUNT);
        if (infoCount < 0)
            continue;
        //wprintf(L"%d: lineStr: %sinfoCount: %d\n", __LINE__, lineStr, infoCount);
        wchar_t name[REMARKSIZE] = { 0 }, value[REMARKSIZE] = { 0 };
        wcsncpy(name, lineStr + ovector[2], ovector[3] - ovector[2]);
        wcsncpy(value, lineStr + ovector[4], ovector[5] - ovector[4]);
        addInfoItem(ins, name, value);
        //wprintf(L"name:%s value: %s\n", name, value);

        //for (int i = 0; i < OVECCOUNT; ++i)
        //    wprintf(L"ovector[%d]:%d \n", i, ovector[i]);
    }
    pcre16_free(infoReg);
}

static void readMove_PGN_ICCSZH(Instance* ins, FILE* fin, RecFormat fmt)
{
    extern const wchar_t PRECHAR[4];
    extern const wchar_t MOVCHAR[4];
    extern const wchar_t NUMCHAR[PIECECOLORNUM][BOARDCOL + 1];
    extern const wchar_t* PieceNames[PIECECOLORNUM];
    extern const wchar_t ICCSCHAR[BOARDCOL + 1];
    wchar_t ZhChars[REMARKSIZE] = { 0 }, ICCSChars[REMARKSIZE] = { 0 },
            ICCSZHStr[REMARKSIZE] = { 0 },
            movePat[REMARKSIZE] = { 0 }, remPat[REMARKSIZE] = { 0 };
    wcscat(ZhChars, PRECHAR);
    wcscat(ZhChars, PieceNames[RED]);
    wcscat(ZhChars, PieceNames[BLACK]);
    wcscat(ZhChars, MOVCHAR);
    wcscat(ZhChars, NUMCHAR[RED]);
    wcscat(ZhChars, NUMCHAR[BLACK]);

    wcscat(ICCSChars, L"1234567890");
    wcscat(ICCSChars, ICCSCHAR);
    wprintf(L"%d: ZhChars: %s\n", __LINE__, ZhChars);
    wprintf(L"%d: ICCSChars: %s\n", __LINE__, ICCSChars);

    wcscat(ICCSZHStr, L"([");
    //wcscat(ICCSZHStr, fmt != PGN_ZH ? ZhChars : ICCSChars);
    wcscat(ICCSZHStr, fmt == PGN_ZH ? ZhChars : ICCSChars);
    wcscat(ICCSZHStr, L"]{4})\b");
    wprintf(L"%d: ICCSZHStr: %s\n", __LINE__, ICCSZHStr);

    const wchar_t remarkStr[] = L"(?:\\s*\\{([\\s\\S]*?)\\})?",
                  otherEndStr[] = L"\\s*(\\)+)?"; // 可能存在多个右括号;
    wcscat(movePat, L"(\\()?(\\d+\\.)?[\\s...]*\b");
    wcscat(movePat, ICCSZHStr);
    wcscat(movePat, remarkStr);
    wcscat(movePat, otherEndStr);

    wcscat(remPat, remarkStr);
    wcscat(remPat, L"1\\.");
    wprintf(L"%d: movePat: %s\n", __LINE__, movePat);
    wprintf(L"%d: remPat: %s\n", __LINE__, remPat);

    const char* error;
    int erroffset = 0, infoCount = 0, OVECCOUNT = 32, ovector[OVECCOUNT];
    pcre16* moveReg = pcre16_compile(movePat, 0, &error, &erroffset, NULL);
    assert(moveReg);
    pcre16* remReg = pcre16_compile(remPat, 0, &error, &erroffset, NULL);
    assert(remReg);

    /*
    int offsetEnd = TEMPSTR_SIZE, length = 1;
    wchar_t* moveStr = calloc(offsetEnd, sizeof(wchar_t));
    wchar_t lineStr[TEMPSTR_SIZE] = { 0 };
    while (fgetws(lineStr, TEMPSTR_SIZE, fin) != NULL) {
        length += wcslen(lineStr);
        if (offsetEnd < length) {
            offsetEnd += TEMPSTR_SIZE;
            realloc(moveStr, offsetEnd);
        }
        wcscat(moveStr, lineStr);
    }
    //*/
    long start = ftell(fin);
    fseek(fin, 0, SEEK_END);
    long end = ftell(fin), length = end - start;
    fseek(fin, start, SEEK_SET);
    char* cmoveStr = malloc(length + 1);
    cmoveStr[length] = '\0';
    fread(cmoveStr, sizeof(char), length, fin);
    wchar_t* moveStr = malloc((length + 1) * sizeof(wchar_t));
    mbstowcs(moveStr, cmoveStr, length);
    free(cmoveStr);

    wchar_t tempStr[REMARKSIZE] = { 0 };
    length = wcslen(moveStr);
    infoCount = pcre16_exec(remReg, NULL, moveStr, length,
        0, 0, ovector, OVECCOUNT);
    if (infoCount <= 0)
        return; // 没有move
    if (infoCount == 2) {
        wcsncpy(tempStr, moveStr + ovector[2], ovector[3] - ovector[2]);
        setRemark(ins->rootMove, tempStr);
    }

    Move *preMove = ins->rootMove,
         *move = preMove,
         *preOtherMoves[REMARKSIZE] = { NULL };
    int curIndex = 0, movRegCount = 0, preOthIndex = 0;
    wprintf(L"%d: moveStr:\n%s\n", __LINE__, moveStr + curIndex);
    //*
    while ((curIndex += ovector[1]) < length) {
        movRegCount = pcre16_exec(moveReg, NULL, moveStr + curIndex,
            wcslen(moveStr + curIndex), 0, 0, ovector, OVECCOUNT);
        wprintf(L"%d: \ncurIndex: %d\nmoveStr: %s\nmovRegCount: %d\n",
            __LINE__, curIndex, moveStr + curIndex, movRegCount);
        if (movRegCount <= 0)
            break;
        for (int i = 0; i < OVECCOUNT; ++i)
            wprintf(L"ovector[%d]:%d \n", i, ovector[i]);
        if (ovector[3] > ovector[2]) { // 第1个子匹配成功，有"("
            move = addOther(preMove);
            preOtherMoves[preOthIndex++] = preMove;
            if (fmt == PGN_ZH)
                __undoMove(ins, preMove);
        } else
            move = addNext(preMove);
        wchar_t zhStr[6] = { 0 }, remarkStr[REMARKSIZE] = { 0 };
        wcsncpy(zhStr, moveStr + curIndex + ovector[6], ovector[7] - ovector[6]);
        wcsncpy(remarkStr, moveStr + curIndex + ovector[8], ovector[9] - ovector[8]);
        setMove(move, ins->board, zhStr, 6);
        setRemark(move, remarkStr);
        if (fmt == PGN_ZH)
            __doMove(ins, move);

        if (ovector[11] > ovector[10]) { // 第5个子匹配成功，有")+"
            int num = ovector[11] - ovector[10];
            for (int i = 0; i < num; ++i) {
                preMove = preOtherMoves[--preOthIndex];
                if (fmt == PGN_ZH) {
                    do
                        __undoMove(ins, move);
                    while ((move = move->pmove) != preMove);
                    __doMove(ins, preMove);
                }
            }
        } else
            preMove = move;

                //moveStr += ovector[1];
    }

    if (fmt == PGN_ZH)
        while (move != ins->rootMove) {
            __undoMove(ins, move);
            move = move->pmove;
        }
            //*/
    /*
    

    //*/
    free(moveStr);
    pcre16_free(remReg);
    pcre16_free(moveReg);
}

static void __writeMove_PGN_ICCSZH(Instance* ins, FILE* fout, Move* move,
    bool isPGN_ZH, bool isOther)
{
    wchar_t boutStr[6] = { 0 }, tempStr[REMARKSIZE] = { 0 };
    swprintf(boutStr, 6, L"%d. ", (move->nextNo_ + 1) / 2);
    bool isEven = move->nextNo_ % 2 == 0;
    if (isOther)
        fwprintf(fout, L"(%s%s", boutStr, isEven ? L"... " : L"");
    else
        fwprintf(fout, isEven ? L" " : boutStr);
    fwprintf(fout, isPGN_ZH ? getZhStr(tempStr, REMARKSIZE, ins->board, move) : getICCS(tempStr, REMARKSIZE, move));
    fwprintf(fout, L" ");
    if (move->remark != NULL)
        fwprintf(fout, L" \n{%s}\n ", move->remark);

    if (move->omove != NULL) {
        __writeMove_PGN_ICCSZH(ins, fout, move->omove, isPGN_ZH, true);
        fwprintf(fout, L")");
    }

    if (isPGN_ZH)
        __doMove(ins, move); // 执行本着
    if (move->nmove != NULL)
        __writeMove_PGN_ICCSZH(ins, fout, move->nmove, isPGN_ZH, false);
    if (isPGN_ZH)
        __undoMove(ins, move); // 退回本着
}

static void writeMove_PGN_ICCSZH(Instance* ins, FILE* fout, RecFormat fmt)
{
    if (ins->rootMove->remark != NULL)
        fwprintf(fout, L" \n{%s}\n ", ins->rootMove->remark);
    if (ins->rootMove->nmove != NULL)
        __writeMove_PGN_ICCSZH(ins, fout, ins->rootMove->nmove, fmt == PGN_ZH, false);
}

static void readMove_PGN_CC(Instance* ins, FILE* fin) {}

wchar_t* __getRemarkStr_PGN(wchar_t* remStr, Move* move)
{
    swprintf(remStr, REMARKSIZE, L"(%d,%d): {%s}\n",
        move->nextNo_, move->CC_ColNo_, move->remark);
    return remStr;
}

static void __writeMove_PGN_CC(int rowNum, int colNum, wchar_t lineStr[rowNum][colNum],
    wchar_t* remarkStr, Instance* ins, Move* move)
{
    int firstCol = move->CC_ColNo_ * 5, row = move->nextNo_ * 2;
    wchar_t zhStr[6] = { 0 }, remStr[REMARKSIZE] = { 0 };
    getZhStr(zhStr, 6, ins->board, move);
    wprintf(L"line:%3d=> (%d,%d) %s\n", __LINE__, move->nextNo_, move->CC_ColNo_, zhStr);
    assert(wcslen(zhStr) == 4);
    for (int i = 0; i < 4; ++i)
        lineStr[row][firstCol + i] = zhStr[i];

    if (move->remark != NULL)
        wcscat(remarkStr, __getRemarkStr_PGN(remStr, move));

    if (move->omove != NULL) {
        int fcol = firstCol + 4, tnum = move->omove->CC_ColNo_ * 5 - fcol;
        for (int i = 0; i < tnum; ++i)
            lineStr[row][fcol + i] = L'…';
        __writeMove_PGN_CC(rowNum, colNum, lineStr, remarkStr, ins, move->omove);
    }

    //*
    if (move->nmove != NULL) {
        lineStr[row + 1][firstCol + 2] = L'↓';
        __doMove(ins, move);
        __writeMove_PGN_CC(rowNum, colNum, lineStr, remarkStr, ins, move->nmove);
        __undoMove(ins, move);
    }
    //*/
}

static void writeMove_PGN_CC(Instance* ins, FILE* fout)
{
    int rowNum = (ins->maxRow_ + 1) * 2, colNum = (ins->maxCol_ + 1) * 5 + 1;
    wchar_t lineStr[rowNum][colNum];
    wmemset(lineStr[0], L'　', rowNum * colNum);
    for (int row = 0; row < rowNum; ++row)
        lineStr[row][colNum - 1] = L'\x0'; // 加尾0字符后，每行数组可直接转换成字符串
    lineStr[0][0] = L'　';
    lineStr[0][1] = L'开';
    lineStr[0][2] = L'始';
    lineStr[1][2] = L'↓';
    wchar_t remarkStr[MOVES_SIZE] = { 0 }, remStr[REMARKSIZE] = { 0 };
    if (ins->rootMove->remark != NULL)
        wcscat(remarkStr, __getRemarkStr_PGN(remStr, ins->rootMove));

    if (ins->rootMove->nmove != NULL)
        __writeMove_PGN_CC(rowNum, colNum, lineStr, remarkStr, ins, ins->rootMove->nmove);
    for (int i = 0; i < rowNum; ++i)
        fwprintf(fout, L"%s\n", lineStr[i]);
    fwprintf(fout, L"%s", remarkStr);
}

Instance* read(Instance* ins, const char* filename)
{
    RecFormat fmt = __getRecFormat(getExt(filename));
    FILE* fin = fopen(filename,
        (fmt == XQF || fmt == BIN || fmt == JSON) ? "rb" : "r");
    switch (fmt) {
    case XQF:
        readXQF(ins, fin);
        break;
    case BIN:
        readBIN(ins, fin);
        break;
    case JSON:
        readJSON(ins, fin);
        break;
    default:
        readInfo_PGN(ins, fin);
        switch (fmt) {
        case PGN_ICCS:
            readMove_PGN_ICCSZH(ins, fin, PGN_ICCS);
            break;
        case PGN_ZH:
            readMove_PGN_ICCSZH(ins, fin, PGN_ZH);
            break;
        case PGN_CC:
            readMove_PGN_CC(ins, fin);
            break;
        default:
            wprintf(L"未实现的打开文件扩展名！");
            break;
        }
        break;
    }
    fclose(fin);
    __getFENToSetBoard(ins);
    if (ins->rootMove->nmove != NULL)
        setMoveNums(ins, ins->rootMove->nmove); // 驱动函数
    return ins;
}

void write(Instance* ins, const char* filename)
{
    RecFormat fmt = __getRecFormat(getExt(filename));
    FILE* fout = fopen(filename,
        (fmt == XQF || fmt == BIN || fmt == JSON) ? "wb" : "w");
    switch (fmt) {
    case XQF:
        wprintf(L"未实现的写入文件扩展名！");
        break;
    case BIN:
        writeBIN(ins, fout);
        break;
    case JSON:
        writeJSON(ins, fout);
        break;
    default:
        for (int i = 0; i < ins->infoCount; ++i)
            fwprintf(fout, L"[%s \"%s\"]\n", ins->info[i][0], ins->info[i][1]);
        fwprintf(fout, L"\n");

        wchar_t tempStr[REMARKSIZE] = { 0 };
        fwprintf(fout, L"%s", getBoardString(tempStr, ins->board));
        fwprintf(fout, L"MoveInfo: movCount:%d remCount:%d remLenMax:%d maxRow:%d maxCol:%d\n\n",
            ins->movCount_, ins->remCount_, ins->maxRemLen_, ins->maxRow_, ins->maxCol_);
        switch (fmt) {
        case PGN_ICCS:
        case PGN_ZH:
            writeMove_PGN_ICCSZH(ins, fout, fmt);
            break;
        case PGN_CC:
            writeMove_PGN_CC(ins, fout);
            break;
        default:
            wprintf(L"未实现的写入文件扩展名！");
            break;
        }
        break;
    }
    fclose(fout);
}

void go(Instance* ins)
{
    if (ins->currentMove->nmove != NULL) {
        ins->currentMove = ins->currentMove->nmove;
        __doMove(ins, ins->currentMove);
    }
}

void back(Instance* ins)
{
    if (ins->currentMove->pmove != NULL) {
        __undoMove(ins, ins->currentMove);
        ins->currentMove = ins->currentMove->pmove;
    }
}

void backTo(Instance* ins, Move* move)
{
    while (ins->currentMove->pmove != NULL
        && !isSame(ins->currentMove, move))
        back(ins);
}

void goOther(Instance* ins)
{
    if (ins->currentMove->pmove != NULL
        && ins->currentMove->omove != NULL) {
        __undoMove(ins, ins->currentMove);
        ins->currentMove = ins->currentMove->omove;
        __doMove(ins, ins->currentMove);
    }
}

void goInc(Instance* ins, int inc)
{
    for (int i = abs(inc); i != 0; --i)
        (inc > 0) ? go(ins) : back(ins);
}

void changeSide(Instance* ins, ChangeType ct) {}

// 测试本翻译单元各种对象、函数
void testInstance(FILE* fout)
{
    Instance* ins = newInstance();
    read(ins, "01.xqf");
    write(ins, "01.bin");
    delInstance(ins);

    ins = newInstance();
    read(ins, "01.bin");
    write(ins, "01.json");
    delInstance(ins);

    ins = newInstance();
    read(ins, "01.json");
    /*
    //*/

    write(ins, "01.pgn_iccs");
    delInstance(ins);

    ins = newInstance();
    read(ins, "01.pgn_iccs");

    write(ins, "01.pgn_zh");
    //write(ins, "01.pgn_cc");
    delInstance(ins);
}
