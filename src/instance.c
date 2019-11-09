#include "head/instance.h"
#include "head/board.h"
#include "head/move.h"
#include "head/piece.h"
#include "head/tools.h"

// 根据存储记录类型取得文件扩展名
static char* EXTNAMES[] = { ".xqf", ".pgn_iccs", ".pgn_zh", ".pgn_cc", ".bin", ".json" };

// 根据文件扩展名取得存储记录类型
static RecFormat getRecFormat(const char* ext)
{
    for (int i = XQF; i <= JSON; ++i)
        if (strcmp(ext, EXTNAMES[i]) == 0)
            return i;
    return XQF;
}

static unsigned char __calkey(unsigned char bKey, unsigned char cKey)
{
    return (((((bKey * bKey) * 3 + 9) * 3 + 8) * 2 + 1) * 3 + 8) * cKey; // % 256; // 保持为<256
}

static unsigned char __sub(unsigned char a, unsigned char b)
{
    return (unsigned char)(a - b); // 保持为<256
}

static void __readBytes(char* bytes, int size, FILE* fin, int Version, unsigned char F32Keys[])
{
    long pos = ftell(fin);
    fread(bytes, sizeof(char), size, fin);
    if (Version > 10) // '字节解密'
        for (int i = 0; i != size; ++i)
            bytes[i] = __sub(bytes[i], F32Keys[(pos + i) % 32]);
}

static int __getRemarksize(FILE* fin, int Version, unsigned char F32Keys[], int KeyRMKSize)
{
    char clen[4] = {};
    __readBytes(clen, 4, fin, Version, F32Keys);
    return *(int*)clen - KeyRMKSize;
}

static wchar_t* __readDataAndGetRemark(wchar_t* wstr, char* data, char* tag,
    FILE* fin, int Version, unsigned char F32Keys[], int KeyRMKSize)
{
    __readBytes(data, 4, fin, Version, F32Keys);
    int RemarkSize = 0;
    if (Version <= 10) {
        *tag = ((*tag & 0xF0) ? 0x80 : 0) | ((*tag & 0x0F) ? 0x40 : 0);
        RemarkSize = __getRemarksize(fin, Version, F32Keys, KeyRMKSize);
    } else {
        *tag &= 0xE0;
        if (*tag & 0x20)
            RemarkSize = __getRemarksize(fin, Version, F32Keys, KeyRMKSize);
    }
    char rem[REMARKSIZE] = {};
    if (RemarkSize > 0) { // # 如果有注解
        __readBytes(rem, RemarkSize, fin, Version, F32Keys);
        mbstowcs(wstr, rem, RemarkSize);
    }
    return wstr;
}

static void __readMove(Move* move, wchar_t* remark, char* data, char* frc, char* trc, char* tag,
    unsigned char KeyXYf, unsigned char KeyXYt,
    FILE* fin, int Version, unsigned char F32Keys[], int KeyRMKSize)
{
    __readDataAndGetRemark(remark, data, tag,
        fin, Version, F32Keys, KeyRMKSize);
    //# 一步棋的起点和终点有简单的加密计算，读入时需要还原
    int fcolrow = __sub(*frc, 0X18 + KeyXYf), tcolrow = __sub(*trc, 0X20 + KeyXYt);
    assert(fcolrow <= 89 && tcolrow <= 89);
    setMoveFromSeats(move, (Seat){ fcolrow % 10, fcolrow / 10 },
        (Seat){ tcolrow % 10, tcolrow / 10 });
    setRemark(move, remark);

    if (*tag & 0x80) //# 有左子树
        __readMove(addNext(move), remark, data, frc, trc, tag, KeyXYf, KeyXYt,
            fin, Version, F32Keys, KeyRMKSize);
    if (*tag & 0x40) // # 有右子树
        __readMove(addOther(move), remark, data, frc, trc, tag, KeyXYf, KeyXYt,
            fin, Version, F32Keys, KeyRMKSize);
}

inline static void readXQF(Instance* ins, FILE* fin)
{
    const int pieceNum = 2 * PIECENUM;
    char Signature[3], Version, headKeyMask, ProductId[4], //文件标记'XQ'=$5158/版本/加密掩码/ProductId[4], 产品(厂商的产品号)
        headKeyOrA, headKeyOrB, headKeyOrC, headKeyOrD,
        headKeysSum, headKeyXY, headKeyXYf, headKeyXYt, // 加密的钥匙和/棋子布局位置钥匙/棋谱起点钥匙/棋谱终点钥匙
        headQiziXY[pieceNum], // 32个棋子的原始位置
        // 用单字节坐标表示, 将字节变为十进制, 十位数为X(0-8)个位数为Y(0-9),
        // 棋盘的左下角为原点(0, 0). 32个棋子的位置从1到32依次为:
        // 红: 车马相士帅士相马车炮炮兵兵兵兵兵 (位置从右到左, 从下到上)
        // 黑: 车马象士将士象马车炮炮卒卒卒卒卒 (位置从右到左, 从下到上)PlayStepNo[2],
        PlayStepNo[2],
        headWhoPlay, headPlayResult, PlayNodes[4], PTreePos[4], Reserved1[4],
        // 该谁下 0-红先, 1-黑先/最终结果 0-未知, 1-红胜 2-黑胜, 3-和棋
        headCodeA_H[16], TitleA[65], TitleB[65], //对局类型(开,中,残等)
        Event[65], Date[17], Site[17], Red[17], Black[17],
        Opening[65], Redtime[17], Blktime[17], Reservedh[33],
        RMKWriter[17], Author[17]; //, Other[528]; // 棋谱评论员/文件的作者

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

    assert(Signature[0] == 0x58 || Signature[1] == 0x51);
    assert((headKeysSum + headKeyXY + headKeyXYf + headKeyXYt) % 256 == 0); // L" 检查密码校验和不对，不等于0。\n";
    assert(Version <= 18); // L" 这是一个高版本的XQF文件，您需要更高版本的XQStudio来读取这个文件。\n";

    unsigned char KeyXY, KeyXYf, KeyXYt, F32Keys[pieceNum], *head_QiziXY = (unsigned char*)headQiziXY;
    int KeyRMKSize = 0;
    if (Version <= 10) { // version <= 10 兼容1.0以前的版本
        KeyRMKSize = KeyXYf = KeyXYt = 0;
    } else {
        KeyXY = __calkey(headKeyXY, headKeyXY);
        KeyXYf = __calkey(headKeyXYf, KeyXY);
        KeyXYt = __calkey(headKeyXYt, KeyXYf);
        KeyRMKSize = (unsigned char)headKeysSum * 256 + (unsigned char)headKeyXY % 32000 + 767; // % 65536
        if (Version >= 12) { // 棋子位置循环移动
            unsigned char Qixy[pieceNum];
            for (int i = 0; i != pieceNum; ++i)
                Qixy[i] = headQiziXY[i]; // 数组不能直接拷贝
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
    const wchar_t copyright[] = L"[(C) Copyright Mr. Dong Shiwei.]";
    for (int i = 0; i != pieceNum; ++i)
        F32Keys[i] = copyright[i] & KeyBytes[i % 4]; // ord(c)

    // 取得棋子字符串
    wchar_t infoTempStr[REMARKSIZE] = {};
    wchar_t pieChars[SEATNUM + 1];
    wmemset(pieChars, BLANKCHAR, sizeof(pieChars));
    pieChars[SEATNUM] = L'\x0';
    wchar_t QiziChars[] = L"RNBAKABNRCCPPPPPrnbakabnrccppppp"; // QiziXY设定的棋子顺序
    for (int i = 0; i != pieceNum; ++i) {
        int xy = head_QiziXY[i];
        if (xy <= 89) // 用单字节坐标表示, 将字节变为十进制,  十位数为X(0-8),个位数为Y(0-9),棋盘的左下角为原点(0, 0)
            pieChars[xy % 10 * 9 + xy / 10] = QiziChars[i];
    }
    setBoard(ins->board, pieChars);
    //wchar_t boardStr[TEMPSTR_SIZE];
    //wprintf(L"%s ins->board@:%p\n", getBoardString(boardStr, ins->board), ins->board);

    swprintf(infoTempStr, REMARKSIZE, L"%d", (int)Version);
    addInfoItem(ins, L"Version", infoTempStr);
    wchar_t* FEN = getFEN(infoTempStr, pieChars);
    addInfoItem(ins, L"FEN", wcscat(FEN, headWhoPlay ? L" -r" : L" -b")); // 存在不是红棋先走的情况？

    wchar_t* PlayType[] = { L"全局", L"开局", L"中局", L"残局" };
    addInfoItem(ins, L"PlayType", PlayType[(int)(headCodeA_H[0])]);
    wchar_t* Result[] = { L"未知", L"红胜", L"黑胜", L"和棋" };
    addInfoItem(ins, L"Result", Result[(int)headPlayResult]);

    char* values[] = {
        TitleA, Event, Date, Site, Red, Black,
        Opening, RMKWriter, Author
    };
    wchar_t* names[] = {
        L"TitleA", L"Event", L"Date", L"Site", L"Red", L"Black",
        L"Opening", L"RMKWriter", L"Author"
    };
    for (int i = 0; i != sizeof(names) / sizeof(names[0]); ++i) {
        mbstowcs(infoTempStr, values[i], strlen(values[i]) + 1);
        addInfoItem(ins, names[i], infoTempStr);
    }
    wprintf(L"%s", getInfoString(infoTempStr, ins));

    char data[4] = {}, *tag = &data[2], *frc = 0, *trc = 0;
    fseek(fin, 1024, SEEK_SET);
    wchar_t remark[REMARKSIZE] = {};
    //*
    setRemark(ins->rootMove,
        __readDataAndGetRemark(remark, data, tag, fin, Version, F32Keys, KeyRMKSize));
    if (*tag & 0x80) //# 有左子树
        ;//__readMove(addNext(ins->rootMove), remark, data, frc, trc, tag, KeyXYf, KeyXYt,
          //  fin, Version, F32Keys, KeyRMKSize);
    //*/
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

inline static const wchar_t* moveInfo(wchar_t* str, size_t n, const Instance* ins)
{
    swprintf(str, n, L"movCount:%d remCount:%d remLenMax:%d maxRow:%d maxCol:%d\n",
        ins->movCount_, ins->remCount_, ins->remLenMax_,
        ins->maxRow_, ins->maxCol_);
    return str;
}

Instance* newInstance(void)
{
    Instance* ins = malloc(sizeof(Instance));
    ins->board = newBoard();
    ins->rootMove = ins->currentMove = newMove();
    memset(ins->info, 0, sizeof(ins->info));
    ins->infoCount = ins->movCount_ = ins->remCount_ = ins->remLenMax_ = ins->maxRow_ = ins->maxCol_ = 0;
    return ins;
}

void delInstance(Instance* ins)
{
    ins->currentMove = NULL;
    for (int i = ins->infoCount; i > 0; --i)
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
    ins->info[count][0] = (wchar_t*)malloc((wcslen(name) + 1) * sizeof(name[0]));
    wcscpy(ins->info[count][0], name);
    ins->info[count][1] = (wchar_t*)malloc((wcslen(value) + 1) * sizeof(name[0]));
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

void read(Instance* ins, const char* filename)
{
    RecFormat fmt = getRecFormat(getExt(filename));
    FILE* fin = fopen(filename, "r");
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
    ins->currentMove = ins->rootMove;
    //__setMoveZhStrAndNums();
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
    if (ins->currentMove->nmove) {
        ins->currentMove = ins->currentMove->nmove;
        moveDo(ins, ins->currentMove);
    }
}

void back(Instance* ins)
{
    if (ins->currentMove->pmove) {
        moveUndo(ins, ins->currentMove);
        ins->currentMove = ins->currentMove->pmove;
    }
}

void backTo(Instance* ins, const Move* move)
{
    while (ins->currentMove->pmove
        && !isSame(ins->currentMove, move))
        back(ins);
}

void goOther(Instance* ins)
{
    if (ins->currentMove->pmove
        && ins->currentMove->omove != NULL) {
        moveUndo(ins, ins->currentMove);
        ins->currentMove = ins->currentMove->omove;
        moveDo(ins, ins->currentMove);
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
    Instance* ins = newInstance();
    const char* filename = "01.xqf";
    const char* ext = getExt(filename);
    printf("%s Ext:%s RecFormat:%d\n", filename, ext, getRecFormat(ext));
    read(ins, filename);

    //*
    wchar_t tempStr[TEMPSTR_SIZE];
    fwprintf(fout, L"testInstance:\n%s\n", getBoardString(tempStr, ins->board));
    fwprintf(fout, L"%s", getInfoString(tempStr, ins));
    if (ins->rootMove->nmove)
        fwprintf(fout, L"   rootMove:%s",
            getMovString(tempStr, TEMPSTR_SIZE, ins->rootMove->nmove));
    //fwprintf(fout, L"currentMove:%s",
    //    getMovString(tempStr, TEMPSTR_SIZE, ins->currentMove));
    fwprintf(fout, L"moveInfo:%s", moveInfo(tempStr, TEMPSTR_SIZE, ins));
    //*/
    delInstance(ins);
}
