#include "head/instance.h"
#include "head/board.h"
#include "head/move.h"
#include "head/piece.h"
#include "head/tools.h"

// 根据存储记录类型取得文件扩展名
static const char* EXTNAMES[] = { ".xqf", ".pgn_iccs", ".pgn_zh", ".pgn_cc", ".bin", ".json" };

inline static void doMove(Instance* ins, const Move* move)
{
    *move->tpiece = moveTo(ins->board, *move->fseat, *move->tseat, NULL);
}

inline static void undoMove(Instance* ins, const Move* move)
{
    moveTo(ins->board, *move->tseat, *move->fseat, *move->tpiece);
}

// 根据文件扩展名取得存储记录类型
static RecFormat getRecFormat(const char* ext)
{
    for (int i = XQF; i <= JSON; ++i)
        if (strcmp(ext, EXTNAMES[i]) == 0)
            return i;
    return XQF;
}

// 供readXQF使用的有关解密钥匙
static int Version = 0, KeyRMKSize = 0;
static unsigned char KeyXYf = 0, KeyXYt = 0, F32Keys[PIECENUM * 2] = { 0 };

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
    int RemarkSize = 0;
    if (Version <= 10 || (data[2] & 0x20)) {
        char clen[4] = { 0 };
        __readBytes(clen, 4, fin);
        RemarkSize = *(__int32*)clen - KeyRMKSize;
    }
    if (RemarkSize > 0) {
        char rem[REMARKSIZE] = { 0 };
        __readBytes(rem, RemarkSize, fin);
        mbstowcs(remark, rem, REMARKSIZE - 1);
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
    assert(fcolrow <= 89 && tcolrow <= 89);
    wprintf(L"%3d: %d->%d\n", __LINE__, fcolrow, tcolrow);
    *move->fseat = getSeat_rc(fcolrow % 10, fcolrow / 10);
    *move->tseat = getSeat_rc(tcolrow % 10, tcolrow / 10);
    setRemark(move, remark);

    char tag = data[2]; // 保存一个标志副本，在递归调用中记住tag
    if (tag & 0x80) //# 有左子树
        __readMove(addNext(move), fin);
    if (tag & 0x40) // # 有右子树
        __readMove(addOther(move), fin);
}

static void __setMoveNums(Instance* ins, Move* move)
{
    if (move == NULL)
        return;
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
    //move->setZhStr(board_->getZhStr(move->fseat(), move->tseat()));

    assert(*move->fseat);
    wprintf(L"%3d: %d%d->%d%d\n", __LINE__,
        (*move->fseat)->row, (*move->fseat)->col,
        (*move->tseat)->row, (*move->tseat)->col);
    doMove(ins, move);
    __setMoveNums(ins, move->nmove); // 先深度搜索
    undoMove(ins, move);

    if (move->omove != NULL)
        ++ins->maxCol_;
    __setMoveNums(ins, move->omove); // 后广度搜索
}

static void readXQF(Instance* ins, FILE* fin)
{
    const int pieceNum = 2 * PIECENUM;
    //*
    char xqfData[1024] = { 0 };
    fread(xqfData, sizeof(char), 1024, fin);
    char Signature[3] = { 0 }, headKeyMask, ProductId[4] = { 0 }, //Version, 文件标记'XQ'=$5158/版本/加密掩码/ProductId[4], 产品(厂商的产品号)
        headKeyOrA, headKeyOrB, headKeyOrC, headKeyOrD,
         headKeysSum, headKeyXY, headKeyXYf, headKeyXYt, // 加密的钥匙和/棋子布局位置钥匙/棋谱起点钥匙/棋谱终点钥匙
        headQiziXY[2 * PIECENUM] = { 0 }, // 32个棋子的原始位置
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

    //*/
    /*
    char Signature[3] = { 0 }, headKeyMask, ProductId[4] = { 0 }, //Version, 文件标记'XQ'=$5158/版本/加密掩码/ProductId[4], 产品(厂商的产品号)
        headKeyOrA, headKeyOrB, headKeyOrC, headKeyOrD,
         headKeysSum, headKeyXY, headKeyXYf, headKeyXYt, // 加密的钥匙和/棋子布局位置钥匙/棋谱起点钥匙/棋谱终点钥匙
        headQiziXY[2 * PIECENUM] = { 0 }, // 32个棋子的原始位置
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

    fread(Signature, 1, 2, fin);
    Version = getc(fin);
    headKeyMask = getc(fin);
    fread(ProductId, 1, 4, fin); // = 8 bytes
    headKeyOrA = getc(fin);
    headKeyOrB = getc(fin);
    headKeyOrC = getc(fin);
    headKeyOrD = getc(fin);
    headKeysSum = getc(fin);
    headKeyXY = getc(fin);
    headKeyXYf = getc(fin);
    headKeyXYt = getc(fin); // = 16 bytes
    fread(headQiziXY, 1, pieceNum, fin); // = 48 bytes
    fread(PlayStepNo, 1, 2, fin);
    headWhoPlay = getc(fin);
    headPlayResult = getc(fin);
    fread(PlayNodes, 1, 4, fin);
    fread(PTreePos, 1, 4, fin);
    fread(Reserved1, 1, 4, fin); // = 64 bytes
    fread(headCodeA_H, 1, 16, fin);
    fread(TitleA, 1, 64, fin);
    fread(TitleB, 1, 64, fin);
    fread(Event, 1, 64, fin);
    fread(Date, 1, 16, fin);
    fread(Site, 1, 16, fin);
    fread(Red, 1, 16, fin);
    fread(Black, 1, 16, fin);
    fread(Opening, 1, 64, fin);
    fread(Redtime, 1, 16, fin);
    fread(Blktime, 1, 16, fin);
    fread(Reservedh, 1, 32, fin);
    fread(RMKWriter, 1, 16, fin);
    fread(Author, 1, 16, fin);
    //*/

    assert(Signature[0] == 0x58 || Signature[1] == 0x51);
    assert((headKeysSum + headKeyXY + headKeyXYf + headKeyXYt) % 256 == 0); // L" 检查密码校验和不对，不等于0。\n";
    assert(Version <= 18); // L" 这是一个高版本的XQF文件，您需要更高版本的XQStudio来读取这个文件。\n";

    // 计算解密数据
    unsigned char KeyXY, *head_QiziXY = (unsigned char*)headQiziXY; //KeyXYf, KeyXYt, F32Keys[pieceNum],//int KeyRMKSize = 0;
    if (Version <= 10) { // version <= 10 兼容1.0以前的版本
        KeyRMKSize = KeyXYf = KeyXYt = 0;
    } else {
        KeyXY = __calkey(headKeyXY, headKeyXY);
        KeyXYf = __calkey(headKeyXYf, KeyXY);
        KeyXYt = __calkey(headKeyXYt, KeyXYf);
        KeyRMKSize = (unsigned char)headKeysSum * 256 + (unsigned char)headKeyXY % 32000 + 767; // % 65536
        if (Version >= 12) { // 棋子位置循环移动
            unsigned char Qixy[pieceNum];
            memcpy(Qixy, headQiziXY, pieceNum);
            for (int i = 0; i != pieceNum; ++i)
                head_QiziXY[(i + KeyXY + 1) % pieceNum] = Qixy[i];
        }
        for (int i = 0; i != pieceNum; ++i)
            head_QiziXY[i] -= KeyXY; // 保持为8位无符号整数，<256
    }
    int KeyBytes[4] = {
        (headKeysSum & headKeyMask) | headKeyOrA,
        (headKeyXY & headKeyMask) | headKeyOrB,
        (headKeyXYf & headKeyMask) | headKeyOrC,
        (headKeyXYt & headKeyMask) | headKeyOrD
    };
    const char copyright[] = "[(C) Copyright Mr. Dong Shiwei.]";
    for (int i = 0; i != pieceNum; ++i)
        F32Keys[i] = copyright[i] & KeyBytes[i % 4]; // ord(c)

    // 取得棋子字符串
    wchar_t pieChars[SEATNUM + 1] = { 0 };
    wmemset(pieChars, BLANKCHAR, SEATNUM);
    const wchar_t QiziChars[] = L"RNBAKABNRCCPPPPPrnbakabnrccppppp"; // QiziXY设定的棋子顺序
    for (int i = 0; i != pieceNum; ++i) {
        int xy = head_QiziXY[i];
        if (xy <= 89) // 用单字节坐标表示, 将字节变为十进制,  十位数为X(0-8),个位数为Y(0-9),棋盘的左下角为原点(0, 0)
            pieChars[xy % 10 * 9 + xy / 10] = QiziChars[i];
    }
    setBoard(ins->board, pieChars);

    wchar_t tempStr[REMARKSIZE];
    swprintf(tempStr, REMARKSIZE, L"%d", (int)Version);
    addInfoItem(ins, L"Version", tempStr); // 整数存储

    getFEN(tempStr, pieChars);
    addInfoItem(ins, L"FEN", wcscat(tempStr, headWhoPlay ? L" -r" : L" -b")); // 转换FEN存储

    wchar_t* PlayType[] = { L"全局", L"开局", L"中局", L"残局" };
    addInfoItem(ins, L"PlayType", PlayType[(int)(headCodeA_H[0])]); // 编码定义存储
    wchar_t* Result[] = { L"未知", L"红胜", L"黑胜", L"和棋" };
    addInfoItem(ins, L"Result", Result[(int)headPlayResult]); // 编码定义存储

    char* values[] = {
        TitleA, Event, Date, Site, Red, Black,
        Opening, RMKWriter, Author
    };
    wchar_t* names[] = {
        L"TitleA", L"Event", L"Date", L"Site", L"Red", L"Black",
        L"Opening", L"RMKWriter", L"Author"
    };
    //*
    for (int i = 0; i != sizeof(names) / sizeof(names[0]); ++i) {
        mbstowcs(tempStr, values[i], REMARKSIZE - 1);
        addInfoItem(ins, names[i], tempStr); // 多字节字符串存储
    }
    //*/
    //wprintf(L"=> %s ", names[i], __LINE__);
    //wprintf(L"=> %s ", tempStr, __LINE__);
    //printf("%s line:%d\n", values[i], __LINE__);
    //wprintf(L"testInstance:\n%s", getInfoString(tempStr, ins));

    fseek(fin, 1024, SEEK_SET);
    char data[4] = { 0 };
    __readMoveData(data, tempStr, fin);
    setRemark(ins->rootMove, tempStr);
    wprintf(L"%3d: %s\n", __LINE__, tempStr);
    if (data[2] & 0x80) //# 有左子树
        __readMove(addNext(ins->rootMove), fin);
}

inline static void readBIN(Instance* ins, FILE* fin) {}

inline static void writeBIN(const Instance* ins, FILE* fout) {}

inline static void readJSON(Instance* ins, FILE* fin) {}

inline static void writeJSON(const Instance* ins, FILE* fout) {}

inline static void readInfo_PGN(Instance* ins, FILE* fin) {}

inline static void writeInfo_PGN(const Instance* ins, FILE* fout) {}

inline static void readMove_PGN_ICCSZH(Instance* ins, FILE* fin, RecFormat fmt) {}

inline static void writeMove_PGN_ICCSZH(const Instance* ins, FILE* fout, RecFormat fmt) {}

inline static void readMove_PGN_CC(Instance* ins, FILE* fin) {}

inline static void writeMove_PGN_CC(const Instance* ins, FILE* fout) {}

inline static void setMoveNums(const Instance* ins) {}

inline static const wchar_t* getMoveInfo(wchar_t* str, size_t n, const Instance* ins)
{
    swprintf(str, n, L"movCount:%d remCount:%d remLenMax:%d maxRow:%d maxCol:%d\n",
        ins->movCount_, ins->remCount_, ins->maxRemLen_,
        ins->maxRow_, ins->maxCol_);
    return str;
}

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
    int count = ins->infoCount;
    if (count == 32)
        return;
    ins->info[count][0] = (wchar_t*)calloc(wcslen(name) + 1, sizeof(name[0]));
    wcscpy(ins->info[count][0], name);
    ins->info[count][1] = (wchar_t*)calloc(wcslen(value) + 1, sizeof(value[0]));
    wcscpy(ins->info[count][1], value);
    ++(ins->infoCount);
}

wchar_t* getInfoString(wchar_t* infoString, Instance* ins)
{
    infoString[0] = L'\x0';
    wchar_t tempStr[REMARKSIZE];
    for (int i = 0; i < ins->infoCount; ++i) {
        swprintf(tempStr, REMARKSIZE, L"[%s \"%s\"]\n",
            ins->info[i][0], ins->info[i][1]);
        wcscat(infoString, tempStr);
    }
    return infoString;
}

Instance* read(const char* filename)
{
    Instance* ins = newInstance();
    RecFormat fmt = getRecFormat(getExt(filename));
    FILE* fin = fopen(filename, (fmt == XQF || fmt == BIN || fmt == JSON) ? "r" : "r");
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
        default: // PGN_CC:
            readMove_PGN_CC(ins, fin);
            break;
        }
        break;
    }
    __setMoveNums(ins, ins->rootMove->nmove); // 驱动函数
    return ins;
}

void write(const Instance* ins, const char* filename)
{
    RecFormat fmt = getRecFormat(getExt(filename));
    FILE* fout = fopen(filename, "w");
    switch (fmt) {
    case XQF:
        break;
    case BIN:
        writeBIN(ins, fout);
        break;
    case JSON:
        writeJSON(ins, fout);
        break;
    default:
        writeInfo_PGN(ins, fout);
        switch (fmt) {
        case PGN_ICCS:
            writeMove_PGN_ICCSZH(ins, fout, PGN_ICCS);
            break;
        case PGN_ZH:
            writeMove_PGN_ICCSZH(ins, fout, PGN_ZH);
            break;
        default: // PGN_CC:
            writeMove_PGN_CC(ins, fout);
            break;
        }
        break;
    }
}

void go(Instance* ins)
{
    if (ins->currentMove->nmove != NULL) {
        ins->currentMove = ins->currentMove->nmove;
        doMove(ins, ins->currentMove);
    }
}

void back(Instance* ins)
{
    if (ins->currentMove->pmove != NULL) {
        undoMove(ins, ins->currentMove);
        ins->currentMove = ins->currentMove->pmove;
    }
}

void backTo(Instance* ins, const Move* move)
{
    while (ins->currentMove->pmove != NULL
        && !isSame(ins->currentMove, move))
        back(ins);
}

void goOther(Instance* ins)
{
    if (ins->currentMove->pmove != NULL
        && ins->currentMove->omove != NULL) {
        undoMove(ins, ins->currentMove);
        ins->currentMove = ins->currentMove->omove;
        doMove(ins, ins->currentMove);
    }
}

void goInc(Instance* ins, int inc)
{
    for (int i = abs(inc); i != 0; --i)
        if (inc > 0)
            go(ins);
        else
            back(ins);
}

void changeSide(Instance* ins, ChangeType ct) {}

// 测试本翻译单元各种对象、函数
void testInstance(FILE* fout)
{
    const char* filename = "01.xqf";
    const char* ext = getExt(filename);
    printf("%s Ext:%s RecFormat:%d\n", filename, ext, getRecFormat(ext));
    Instance* ins = read(filename);

    wchar_t* tempStr = (wchar_t*)calloc(MOVES_SIZE, sizeof(wchar_t));
    //*
    fwprintf(fout, L"testInstance:\n%s", getInfoString(tempStr, ins));
    fwprintf(fout, L"%s", getBoardString(tempStr, ins->board));
    fwprintf(fout, L"getMoveInfo:%s", getMoveInfo(tempStr, TEMPSTR_SIZE, ins));
    fwprintf(fout, L"   rootMove:%s",
        getMovString_iccszh(tempStr, MOVES_SIZE, ins->rootMove, PGN_ICCS));
    //fwprintf(fout, L"currentMove:%s",
    //    getMovString_iccszh(tempStr, MOVES_SIZE, ins->currentMove, PGN_ICCS));
    //*/
    free(tempStr);
    delInstance(ins);
}
