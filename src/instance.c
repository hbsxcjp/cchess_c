#include "head/instance.h"
#include "head/board.h"
#include "head/move.h"
#include "head/piece.h"

inline static const char* getExt(const char* filename)
{
    return strrchr(filename, '.');
}

// 根据存储记录类型取得文件扩展名
inline static const char* getExtName(const RecFormat fmt)
{
    switch (fmt) {
    case XQF:
        return ".xqf";
    case BIN:
        return ".bin";
    case JSON:
        return ".json";
    case PGN_ICCS:
        return ".pgn_iccs";
    case PGN_ZH:
        return ".pgn_zh";
    case PGN_CC:
        return ".pgn_cc";
    default:
        return ".pgn_cc";
    }
}

// 根据文件扩展名取得存储记录类型
inline static RecFormat getRecFormat(const char* ext)
{
    if (strcmp(ext, ".xqf"))
        return XQF;
    else if (strcmp(ext, ".bin"))
        return BIN;
    else if (strcmp(ext, ".json"))
        return JSON;
    else if (strcmp(ext, ".pgn_iccs"))
        return PGN_ICCS;
    else if (strcmp(ext, ".pgn_zh"))
        return PGN_ZH;
    else if (strcmp(ext, ".pgn_cc"))
        return PGN_CC;
    else
        return PGN_CC;
}

inline static unsigned char __calkey(unsigned char bKey, unsigned char cKey)
{
    return (((((bKey * bKey) * 3 + 9) * 3 + 8) * 2 + 1) * 3 + 8) * cKey; // % 256; // 保持为<256
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
    memset(pieChars, BLANKCHAR, sizeof(pieChars));
    wchar_t QiziChars[] = L"RNBAKABNRCCPPPPPrnbakabnrccppppp"; // QiziXY设定的棋子顺序
    for (int i = 0; i != pieceNum; ++i) {
        int xy = head_QiziXY[i];
        if (xy <= 89) // 用单字节坐标表示, 将字节变为十进制,  十位数为X(0-8),个位数为Y(0-9),棋盘的左下角为原点(0, 0)
            pieChars[xy % 10 * 9 + xy / 10] = QiziChars[i];
    }
    setBoard(ins->board, pieChars);
    
    /*   
    swprintf(ins->info, TEMPSTR_SIZE, 
    L"", );
        { L"Version", std::to_wstring(Version) },
        { L"Result", (std::map<unsigned char, std::wstring>{ { 0, L"未知" }, { 1, L"红胜" }, { 2, L"黑胜" }, { 3, L"和棋" } })[headPlayResult] },
        { L"PlayType", (std::map<unsigned char, std::wstring>{ { 0, L"全局" }, { 1, L"开局" }, { 2, L"中局" }, { 3, L"残局" } })[headCodeA_H[0]] },
        { L"TitleA", Tools::s2ws(TitleA) },
        { L"Event", Tools::s2ws(Event) },
        { L"Date", Tools::s2ws(Date) },
        { L"Site", Tools::s2ws(Site) },
        { L"Red", Tools::s2ws(Red) },
        { L"Black", Tools::s2ws(Black) },
        { L"Opening", Tools::s2ws(Opening) },
        { L"RMKWriter", Tools::s2ws(RMKWriter) },
        { L"Author", Tools::s2ws(Author) },
        { L"FEN", pieCharsToFEN(pieChars) } // 可能存在不是红棋先走的情况？
    };
 
    std::function<unsigned char(unsigned char, unsigned char)>
        __sub = [](unsigned char a, unsigned char b) {
            return a - b;
        }; // 保持为<256
    auto __readBytes = [&](char* bytes, int size) {
        int pos = is.tellg();
        isfread(bytes, size);
        if (Version > 10) // '字节解密'
            for (int i = 0; i != size; ++i)
                bytes[i] = __sub(bytes[i], F32Keys[(pos + i) % 32]);
    };
    char data[4]{}, &frc{ data[0] }, &trc{ data[1] }, &tag{ data[2] };
    auto __getRemarksize = [&]() {
        char clen[4]{};
        __readBytes(clen, 4);
        return *(int*)clen - KeyRMKSize;
    };
    std::function<std::wstring()>
        __readDataAndGetRemark = [&]() {
            __readBytes(data, 4);
            int RemarkSize{};
            if (Version <= 10) {
                tag = ((tag & 0xF0) ? 0x80 : 0) | ((tag & 0x0F) ? 0x40 : 0);
                RemarkSize = __getRemarksize();
            } else {
                tag &= 0xE0;
                if (tag & 0x20)
                    RemarkSize = __getRemarksize();
            }
            if (RemarkSize > 0) { // # 如果有注解
                char rem[RemarkSize + 1]{};
                __readBytes(rem, RemarkSize);
                return Tools::s2ws(rem);
            } else
                return std::wstring{};
        };
    std::function<void(const std::shared_ptr<Move>&)>
        __readMove = [&](const std::shared_ptr<Move>& move) {
            auto remark = __readDataAndGetRemark();
            //# 一步棋的起点和终点有简单的加密计算，读入时需要还原
            int fcolrow = __sub(frc, 0X18 + KeyXYf), tcolrow = __sub(trc, 0X20 + KeyXYt);
            assert(fcolrow <= 89 && tcolrow <= 89);
            __setMoveFromRowcol(move, (fcolrow % 10) * 10 + fcolrow / 10,
                (tcolrow % 10) * 10 + tcolrow / 10, remark);

            char ntag{ tag };
            if (ntag & 0x80) //# 有左子树
                __readMove(move->addNext());
            if (ntag & 0x40) // # 有右子树
                __readMove(move->addOther());
        };

    is.seekg(1024);
    rootMove_->setRemark(__readDataAndGetRemark());
    char rtag{ tag };
    if (rtag & 0x80) //# 有左子树
        __readMove(rootMove_->addNext());
    */
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
    memset(ins->info, 0, sizeof(ins->info));
    ins->rootMove = ins->currentMove = newMove();
    ins->movCount_ = ins->remCount_ = ins->remLenMax_ = ins->maxRow_ = ins->maxCol_ = 0;
    return ins;
}

void delInstance(Instance* ins)
{
    delMove(ins->rootMove);
    delBoard(ins->board);
    free(ins);
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
    //read(ins, "a.pgn");
    wchar_t tempStr[TEMPSTR_SIZE];
    fwprintf(fout, L"testInstance:\n%sinfo:%s\n",
        getBoardString(tempStr, ins->board), ins->info);
    fwprintf(fout, L"   rootMove:%s",
        getMovString(tempStr, TEMPSTR_SIZE, ins->rootMove));
    fwprintf(fout, L"currentMove:%s",
        getMovString(tempStr, TEMPSTR_SIZE, ins->currentMove));
    fwprintf(fout, L"moveInfo:%s", moveInfo(tempStr, TEMPSTR_SIZE, ins));
    delInstance(ins);
}
