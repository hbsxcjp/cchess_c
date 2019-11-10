#include "head/instance.h"
#include "head/board.h"
#include "head/move.h"
#include "head/piece.h"
#include "head/tools.h"

// 根据存储记录类型取得文件扩展名
static char* EXTNAMES[] = { ".xqf", ".pgn_iccs", ".pgn_zh", ".pgn_cc", ".bin", ".json" };

inline static void moveDone(Instance* ins, const Move* move)
{
    *move->tpiece = moveTo(ins->board, *move->fseat, *move->tseat, NULL);
}

inline static void moveUndo(Instance* ins, const Move* move)
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
static unsigned char KeyXYf = 0, KeyXYt = 0, F32Keys[PIECENUM * 2] = {};
static char data[4] = {}, *frc = &data[0], *trc = &data[1], *tag = &data[2];
static wchar_t remark[REMARKSIZE] = {};

static unsigned char __calkey(unsigned char bKey, unsigned char cKey)
{
    return (((((bKey * bKey) * 3 + 9) * 3 + 8) * 2 + 1) * 3 + 8) * cKey; // % 256; // 保持为<256
}

static unsigned char __sub(unsigned char a, unsigned char b)
{
    return (unsigned char)(a - b); // 保持为<256
}

static void __readBytes(char* bytes, int size, FILE* fin)
{
    long pos = ftell(fin);
    fread(bytes, sizeof(char), size, fin);
    if (Version > 10) // '字节解密'
        for (int i = 0; i != size; ++i)
            bytes[i] = __sub(bytes[i], F32Keys[(pos + i) % 32]);
}

static wchar_t* __readMoveData(FILE* fin)
{
    remark[0] = L'\x0';
    __readBytes(data, 4, fin);
    if (Version <= 10)
        *tag = (*tag & 0xF0 ? 0x80 : 0) | (*tag & 0x0F ? 0x40 : 0);
    else
        *tag = *tag & 0xE0;
    int RemarkSize = 0;
    if (Version <= 10 || *tag & 0x20) {
        char clen[4] = {};
        __readBytes(clen, 4, fin);
        RemarkSize = *(__int32*)clen - KeyRMKSize;
    }
    if (RemarkSize > 0) {
        char rem[REMARKSIZE] = { 0 };
        __readBytes(rem, RemarkSize, fin);
        mbstowcs(remark, rem, RemarkSize);
    }
    return remark;
}

static void __readMove(Move* move, FILE* fin)
{
    __readMoveData(fin);
    //# 一步棋的起点和终点有简单的加密计算，读入时需要还原
    int fcolrow = __sub(*frc, 0X18 + KeyXYf), tcolrow = __sub(*trc, 0X20 + KeyXYt);
    assert(fcolrow <= 89 && tcolrow <= 89);
    int frow = fcolrow % 10, fcol = fcolrow / 10, trow = tcolrow % 10, tcol = tcolrow / 10;
    *move->fseat = getSeat_rc(frow, fcol);
    *move->tseat = getSeat_rc(trow, tcol);
    setRemark(move, remark);

    char ntag = *tag; // 保存一个标志副本，在递归调用中记住tag
    if (ntag & 0x80) //# 有左子树
        __readMove(addNext(move), fin);
    if (ntag & 0x40) // # 有右子树
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

    wprintf(L"%d%d->%d%d ", (*move->fseat)->row, (*move->fseat)->col,
        (*move->tseat)->row, (*move->tseat)->col);
    moveDone(ins, move);
    __setMoveNums(ins, move->nmove); // 先深度搜索
    moveUndo(ins, move);

    if (move->omove != NULL)
        ++ins->maxCol_;
    __setMoveNums(ins, move->omove); // 后广度搜索
}

static void readXQF(Instance* ins, FILE* fin)
{
    const int pieceNum = 2 * PIECENUM;
    char Signature[3], headKeyMask, ProductId[4], //Version, 文件标记'XQ'=$5158/版本/加密掩码/ProductId[4], 产品(厂商的产品号)
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
    swprintf(infoTempStr, REMARKSIZE, L"%d", (int)Version);
    addInfoItem(ins, L"Version", infoTempStr); // 整数存储

    getFEN(infoTempStr, pieChars);
    addInfoItem(ins, L"FEN", wcscat(infoTempStr, headWhoPlay ? L" -r" : L" -b")); // 转换FEN存储

    //wprintf(L"=>%s\n", getInfoString(infoTempStr, ins));

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
    for (int i = 0; i != sizeof(names) / sizeof(names[0]); ++i) {
        wmemset(infoTempStr, 0, sizeof(infoTempStr));
        mbstowcs(infoTempStr, values[i], REMARKSIZE);
        addInfoItem(ins, names[i], infoTempStr); // 多字节字符串存储
    }
    printf("=> %s line:%d\n", TitleA, 245);

    fseek(fin, 1024, SEEK_SET);
    setRemark(ins->rootMove, __readMoveData(fin));
    if (*tag & 0x80) //# 有左子树
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
        moveDone(ins, ins->currentMove);
    }
}

void back(Instance* ins)
{
    if (ins->currentMove->pmove != NULL) {
        moveUndo(ins, ins->currentMove);
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
        moveUndo(ins, ins->currentMove);
        ins->currentMove = ins->currentMove->omove;
        moveDone(ins, ins->currentMove);
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
    fwprintf(fout, L"testInstance:\n%s", getInfoString(tempStr, ins));
    fwprintf(fout, L"%s", getBoardString(tempStr, ins->board));
    fwprintf(fout, L"getMoveInfo:%s", getMoveInfo(tempStr, TEMPSTR_SIZE, ins));
    fwprintf(fout, L"   rootMove:%s",
        getMovString_iccszh(tempStr, MOVES_SIZE, ins->rootMove, PGN_ICCS));
    /*
    //fwprintf(fout, L"currentMove:%s",
    //    getMovString_iccszh(tempStr, MOVES_SIZE, ins->currentMove, PGN_ICCS));
    //*/
    free(tempStr);
    delInstance(ins);
}
