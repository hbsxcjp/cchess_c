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
    *tag = data[2];
    /*
    if (Version <= 10) {
        tag = ((tag & 0xF0) ? 0x80 : 0) | ((tag & 0x0F) ? 0x40 : 0);
        RemarkSize = __getRemarksize(fin, Version, F32Keys, KeyRMKSize);
    } else {
        tag &= 0xE0;
        if (tag & 0x20)
            RemarkSize = __getRemarksize(fin, Version, F32Keys, KeyRMKSize);
    }*/
    if (Version <= 10 || (*tag &= 0xE0) & 0x20) {
        if (Version <= 10)
            *tag = ((*tag & 0xF0) ? 0x80 : 0) | ((*tag & 0x0F) ? 0x40 : 0);
        //RemarkSize = __getRemarksize(fin, Version, F32Keys, KeyRMKSize);
        char clen[4] = {};
        __readBytes(clen, 4, fin, Version, F32Keys);
        RemarkSize = *(int*)clen - KeyRMKSize;
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
    remark = __readDataAndGetRemark(remark, data, tag,
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

    wchar_t infoTempStr[REMARKSIZE];
    swprintf(infoTempStr, REMARKSIZE, L"%d", Version);
    addInfoItem(ins, L"Version", infoTempStr);

    wchar_t* Result[] = { L"未知", L"红胜", L"黑胜", L"和棋" };
    addInfoItem(ins, L"Result", Result[(int)headPlayResult]);

    wchar_t* PlayType[] = { L"全局", L"开局", L"中局", L"残局" };
    addInfoItem(ins, L"PlayType", PlayType[(int)(headCodeA_H[0])]);
    char* values[] = {
        TitleA, Event, Date, Site, Red, Black,
        Opening, RMKWriter, Author
    };
    wchar_t* names[] = {
        L"TitleA", L"Event", L"Date", L"Site", L"Red", L"Black",
        L"Opening", L"RMKWriter", L"Author"
    };
    for (int i = 3; i != 12; ++i) {
        mbstowcs(infoTempStr, values[i], strlen(values[i]));
        addInfoItem(ins, names[i], infoTempStr);
    }
    addInfoItem(ins, L"FEN", getFEN(infoTempStr, pieChars)); // 可能存在不是红棋先走的情况？

    char data[4] = {}, *tag = 0, *frc = 0, *trc = 0;
    wchar_t remark[REMARKSIZE] = {};
    fseek(fin, 1024, SEEK_SET);
    setRemark(ins->rootMove,
        __readDataAndGetRemark(remark, data, tag, fin, Version, F32Keys, KeyRMKSize));
    if (*tag & 0x80) //# 有左子树
        __readMove(addNext(ins->rootMove), remark, data, frc, trc, tag, KeyXYf, KeyXYt,
            fin, Version, F32Keys, KeyRMKSize);
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

inline static bool isRootMove(Move* move)
{
    return move->nextNo_ == 0 && move->otherNo_ == 0;
}

Instance* newInstance(void)
{
    Instance* ins = malloc(sizeof(Instance));
    ins->board = newBoard();
    ins->rootMove = ins->currentMove = newMove();
    ins->info_name = ins->info_value = NULL;
    ins->infoCount = ins->movCount_ = ins->remCount_ = ins->remLenMax_ = ins->maxRow_ = ins->maxCol_ = 0;
    return ins;
}

void delInstance(Instance* ins)
{
    ins->currentMove = NULL;
    for (int i = ins->infoCount - 1; i >= 0; --i) {
        free(ins->info_name[i]);
        free(ins->info_value[i]);
    }
    delMove(ins->rootMove);
    free(ins->board);
    free(ins);
}

void addInfoItem(Instance* ins, wchar_t* name, wchar_t* value)
{
    ins->info_name[ins->infoCount] = (wchar_t*)malloc(wcslen(name));
    wcscpy(ins->info_name[ins->infoCount], name);
    ins->info_value[ins->infoCount] = (wchar_t*)malloc(wcslen(value));
    wcscpy(ins->info_value[ins->infoCount], value);
    ++ins->infoCount;
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
    case PGN_ICCS:
        readInfo_PGN(ins, fin);
        readMove_PGN_ICCSZH(ins, fin, PGN_ICCS);
        break;
    case PGN_ZH:
        readInfo_PGN(ins, fin);
        readMove_PGN_ICCSZH(ins, fin, PGN_ZH);
        break;
    case PGN_CC:
        readInfo_PGN(ins, fin);
        readMove_PGN_CC(ins, fin);
        break;
    default:
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
    case PGN_ICCS:
        writeInfo_PGN(ins, fout);
        writeMove_PGN_ICCSZH(ins, fout, PGN_ICCS);
        break;
    case PGN_ZH:
        writeInfo_PGN(ins, fout);
        writeMove_PGN_ICCSZH(ins, fout, PGN_ZH);
        break;
    case PGN_CC:
        writeInfo_PGN(ins, fout);
        writeMove_PGN_CC(ins, fout);
        break;
    default:
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
    while (!isRootMove(ins->currentMove)
        && !isSame(ins->currentMove, move)) {
        back(ins);
    }
}

void goOther(Instance* ins)
{
    if (!isRootMove(ins->currentMove)
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

    /*
    wchar_t tempStr[TEMPSTR_SIZE];
    fwprintf(fout, L"testInstance:\n%s\n", getBoardString(tempStr, ins->board));
    for (int i = 0; i < ins->infoCount; ++i)
        fwprintf(fout, L"info[%d]%s==%s\n", i, ins->info_name[i], ins->info_value[i]);
    fwprintf(fout, L"   rootMove:%s",
        getMovString(tempStr, TEMPSTR_SIZE, ins->rootMove));
    fwprintf(fout, L"currentMove:%s",
        getMovString(tempStr, TEMPSTR_SIZE, ins->currentMove));
    fwprintf(fout, L"moveInfo:%s", moveInfo(tempStr, TEMPSTR_SIZE, ins));
    //*/
    delInstance(ins);
}
