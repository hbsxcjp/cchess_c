#include "head/move.h"
#include "head/board.h"
#include "head/piece.h"
#include "head/tools.h"

struct Move {
    Seat fseat, tseat; // 起止位置0x00
    Piece tpiece; // 目标位置棋子
    wchar_t* remark; // 注解
    wchar_t zhStr[6]; // 着法名称
    struct Move *pmove, *nmove, *omove; // 前着、下着、变着
    int nextNo_, otherNo_, CC_ColNo_; // 走着、变着序号，文本图列号
};

// 着法相关的字符数组静态全局变量
const wchar_t ICCSCOLCHAR[] = L"abcdefghi";
const wchar_t ICCSROWCHAR[] = L"0123456789";
const wchar_t PRECHAR[] = L"前中后";
const wchar_t MOVCHAR[] = L"退平进";
const wchar_t NUMCHAR[PIECECOLORNUM][BOARDCOL + 1] = {
    L"一二三四五六七八九", L"１２３４５６７８９"
};

extern const int ALLCOL;

static wchar_t getPreChar(bool isBottom, int count, int index)
{
    if (isBottom)
        index = count - 1 - index;
    if (count == 2 && index == 1)
        index = 2;
    if (count <= 3)
        return PRECHAR[index];
    else
        return NUMCHAR[RED][index];
}

static int getIndex_ch(bool isBottom, int count, wchar_t wch)
{
    int index = 0;
    wchar_t* pwch = wcschr(PRECHAR, wch);
    if (pwch) {
        index = pwch - PRECHAR;
        if (count == 2 && index == 2)
            index = 1;
    } else {
        pwch = wcschr(NUMCHAR[RED], wch);
        assert(pwch);
        index = pwch - NUMCHAR[RED];
    }
    if (isBottom)
        index = count - 1 - index;
    return index;
}

inline static int __getIndex(const wchar_t* str, wchar_t ch) { return wcschr(str, ch) - str; }

inline static int __getNum(PieceColor color, wchar_t numChar) { return __getIndex(NUMCHAR[color], numChar) + 1; }

inline static wchar_t __getNumChar(PieceColor color, int num) { return NUMCHAR[color][num - 1]; }

inline static int __getCol(bool isBottom, int num) { return isBottom ? BOARDCOL - num : num - 1; }

inline static int __getNum_col(bool isBottom, int col) { return (isBottom ? getOtherCol_c(col) : col) + 1; }

inline static int __getMovDir(bool isBottom, wchar_t mch) { return (__getIndex(MOVCHAR, mch) - 1) * (isBottom ? 1 : -1); }

inline static PieceColor __getColor_zh(const wchar_t* zhStr) { return wcschr(NUMCHAR[RED], zhStr[3]) == NULL ? BLACK : RED; }

static void __setMoveSeat_rc(Move move, Board board, int frow, int fcol, int trow, int tcol)
{
    move->fseat = getSeat_rc(board, frow, fcol);
    move->tseat = getSeat_rc(board, trow, tcol);
}

static void __setMoveSeat_iccs(Move move, Board board, const wchar_t* iccsStr)
{
    __setMoveSeat_rc(move, board,
        __getIndex(ICCSROWCHAR, iccsStr[1]), __getIndex(ICCSCOLCHAR, iccsStr[0]),
        __getIndex(ICCSROWCHAR, iccsStr[3]), __getIndex(ICCSCOLCHAR, iccsStr[2]));
}

inline Move getPre(Move move) { return move->pmove; }
inline Move getNext(Move move) { return move->nmove; }
inline Move getOther(Move move) { return move->omove; }

inline int getNextNo(Move move) { return move->nextNo_; }
inline int getOtherNo(Move move) { return move->otherNo_; }
inline int getCC_ColNo(Move move) { return move->CC_ColNo_; }

inline int getRowCol_m(Move move, bool isFirst) { return getRowCol_s(isFirst ? move->fseat : move->tseat); }

inline const wchar_t* getRemark(Move move) { return move->remark; }
inline const wchar_t* getZhStr(Move move) { return move->zhStr; }
wchar_t* getICCS(wchar_t* ICCSStr, const Move move)
{
    swprintf(ICCSStr, 5, L"%c%d%c%d",
        ICCSCOLCHAR[getCol_s(move->fseat)], getRow_s(move->fseat),
        ICCSCOLCHAR[getCol_s(move->tseat)], getRow_s(move->tseat));
    return ICCSStr;
}

inline PieceColor getFirstColor(Move move) { return move->nmove == NULL ? RED : getColor(getPiece_s(move->fseat)); }

inline bool isStart(Move move) { return !hasPre(move); }

inline bool hasNext(Move move) { return move->nmove != NULL; }

inline bool hasPre(Move move) { return move->pmove != NULL; }

inline bool hasOther(Move move) { return move->omove != NULL; }

inline bool hasPreOther(Move move) { return hasPre(move) && move == move->pmove->omove; }

inline void setNextNo(Move move, int nextNo) { move->nextNo_ = nextNo; }
inline void setOtherNo(Move move, int otherNo) { move->otherNo_ = otherNo; }
inline void setCC_ColNo(Move move, int CC_ColNo) { move->CC_ColNo_ = CC_ColNo; }

static void __setMoveSeat_zh(Move move, Board board, const wchar_t* zhStr)
{
    assert(wcslen(zhStr) == 4);
    //wprintf(L"%d %s\n", __LINE__, zhStr);
    //fflush(stdout);
    // 根据最后一个字符判断该着法属于哪一方
    PieceColor color = __getColor_zh(zhStr);
    bool isBottom = isBottomSide(board, color);
    int index = 0, count = 0,
        movDir = __getMovDir(isBottom, zhStr[2]);
    wchar_t name = zhStr[0];
    Seat seats[SIDEPIECENUM] = { NULL };

    if (isPieceName(name)) { // 棋子名
        //wchar_t wstr[WIDEWCHARSIZE];
        //wprintf(L"%d:%s\n%s\n", __LINE__, zhStr, getBoardString(wstr, board));
        count = getLiveSeats(seats,
            board, color, name, __getCol(isBottom, __getNum(color, zhStr[1])));
        assert(count > 0);
        // 排除：士、象同列时不分前后，以进、退区分棋子。移动方向为底退、顶进时，修正index
        index = (count == 2 && movDir == -1) ? 1 : 0; // movDir == -1：表示底退、顶进
    } else { // 非棋子名
        name = zhStr[1];
        count = isPawnPieceName(name) ? getSortPawnLiveSeats(seats, board, color, name)
                                      : getLiveSeats(seats, board, color, name, ALLCOL);
        index = getIndex_ch(isBottom, count, zhStr[0]);
    }

    //wchar_t wstr[30];
    //for (int i = 0; i < count; ++i)
    //    wprintf(L"%s ", getSeatString(wstr, seats[i]));
    //wprintf(L"\n%s index: %d count: %d\n", zhStr, index, count);

    assert(index < count);
    move->fseat = seats[index];
    int num = __getNum(color, zhStr[3]), toCol = __getCol(isBottom, num),
        frow = getRow_s(move->fseat), fcol = getCol_s(move->fseat), colAway = abs(toCol - fcol); //  相距1或2列
    //wprintf(L"%d %c %d %d %d %d %d\n", __LINE__, name, frow, fcol, movDir, colAway, toCol);
    //fflush(stdout);

    move->tseat = isLinePieceName(name) ? (movDir == 0 ? getSeat_rc(board, frow, toCol)
                                                       : getSeat_rc(board, frow + movDir * num, fcol))
                                        // 斜线走子：仕、相、马
                                        : getSeat_rc(board, frow + movDir * (isKnightPieceName(name) ? (colAway == 1 ? 2 : 1) : colAway), toCol);
    setMoveZhStr(move, board);
    assert(wcscmp(zhStr, move->zhStr) == 0);
    //
}

void setMoveZhStr(Move move, Board board)
{
    Piece fpiece = getPiece_s(move->fseat);
    if (fpiece == BLANKPIECE) {
    }
    assert(fpiece != BLANKPIECE);
    PieceColor color = getColor(fpiece);
    wchar_t name = getPieName(fpiece);
    int frow = getRow_s(move->fseat), fcol = getCol_s(move->fseat),
        trow = getRow_s(move->tseat), tcol = getCol_s(move->tseat);
    bool isBottom = isBottomSide(board, color);
    Seat seats[SIDEPIECENUM] = { 0 };
    int count = getLiveSeats(seats, board, color, name, fcol);

    if (count > 1 && isStronge(fpiece)) { // 马车炮兵
        if (getKind(fpiece) == PAWN)
            count = getSortPawnLiveSeats(seats, board, color, name);
        int index = 0;
        while (move->fseat != seats[index])
            ++index;
        move->zhStr[0] = getPreChar(isBottom, count, index);
        move->zhStr[1] = name;
    } else { //将帅, 仕(士),相(象): 不用“前”和“后”区别，因为能退的一定在前，能进的一定在后
        move->zhStr[0] = name;
        move->zhStr[1] = __getNumChar(color, __getNum_col(isBottom, fcol));
    }
    move->zhStr[2] = MOVCHAR[frow == trow ? 1 : (isBottom == (trow > frow) ? 2 : 0)];
    move->zhStr[3] = __getNumChar(color, (isLinePieceName(name) && frow != trow) ? abs(trow - frow) : __getNum_col(isBottom, tcol));
    move->zhStr[4] = L'\x0';

    //
    //Move amove = newMove();
    //__setMoveSeat_zh(amove, board, move->zhStr);
    //assert(move->fseat == amove->fseat && move->tseat == amove->tseat);
    //__freeMove(amove);
    //
    //return move->zhStr;
}

static Move __setMoveNext(Move preMove, Move move)
{
    move->nextNo_ = preMove->nextNo_ + 1;
    move->otherNo_ = preMove->otherNo_;
    move->pmove = preMove;
    return preMove->nmove = move;
}

static Move __setMoveOther(Move preMove, Move move)
{
    move->nextNo_ = preMove->nextNo_;
    move->otherNo_ = preMove->otherNo_ + 1;
    move->pmove = preMove;
    return preMove->omove = move;
}

static Move __SetRemark_addMove(Move preMove, Move move, wchar_t* remark, bool isOther)
{
    setRemark(move, remark);
    return isOther ? __setMoveOther(preMove, move) : __setMoveNext(preMove, move);
}

Move newMove()
{
    Move move = malloc(sizeof(struct Move));
    move->fseat = move->tseat = NULL;
    move->tpiece = NULL;
    move->remark = NULL;
    move->zhStr[0] = L'\x0';
    move->pmove = move->nmove = move->omove = NULL;
    move->nextNo_ = move->otherNo_ = move->CC_ColNo_ = 0;
    return move;
}

void freeMove(Move move)
{
    if (move == NULL)
        return;
    Move omove = move->omove,
         nmove = move->nmove;
    free(move->remark);
    free(move);
    freeMove(omove);
    freeMove(nmove);
}

Move addMove_rc(Move preMove, Board board, int frow, int fcol, int trow, int tcol, wchar_t* remark, bool isOther)
{
    Move move = newMove();
    __setMoveSeat_rc(move, board, frow, fcol, trow, tcol);
    return __SetRemark_addMove(preMove, move, remark, isOther);
}

Move addMove_rowcol(Move preMove, Board board, int frowcol, int trowcol, wchar_t* remark, bool isOther)
{
    return addMove_rc(preMove, board,
        getRow_rowcol(frowcol), getCol_rowcol(frowcol), getRow_rowcol(trowcol), getCol_rowcol(trowcol), remark, isOther);
}

Move addMove_iccs(Move preMove, Board board, const wchar_t* iccsStr, wchar_t* remark, bool isOther)
{
    Move move = newMove();
    __setMoveSeat_iccs(move, board, iccsStr);
    return __SetRemark_addMove(preMove, move, remark, isOther);
}

Move addMove_zh(Move preMove, Board board, const wchar_t* zhStr, wchar_t* remark, bool isOther)
{
    Move move = newMove();
    __setMoveSeat_zh(move, board, zhStr);
    return __SetRemark_addMove(preMove, move, remark, isOther);
}

void setRemark(Move move, wchar_t* remark)
{
    free(move->remark);
    move->remark = remark;
}

void changeMove(Move move, Board board, ChangeType ct)
{
    if (move == NULL)
        return;
    Seat fseat = move->fseat, tseat = move->tseat;
    //如ct==EXCHANGE, 则交换棋子，位置不需要更改
    if (ct == ROTATE) {
        move->fseat = getSeat_rc(board, getOtherRow_s(fseat), getOtherCol_s(fseat));
        move->tseat = getSeat_rc(board, getOtherRow_s(tseat), getOtherCol_s(tseat));
    } else if (ct == SYMMETRY) {
        move->fseat = getSeat_rc(board, getRow_s(fseat), getOtherCol_s(fseat));
        move->tseat = getSeat_rc(board, getRow_s(tseat), getOtherCol_s(tseat));
    }

    changeMove(move->omove, board, ct);
    changeMove(move->nmove, board, ct);
}

void doMove(Board board, Move move)
{
    move->tpiece = movePiece(board, move->fseat, move->tseat, BLANKPIECE);
}

void undoMove(Board board, Move move)
{
    movePiece(board, move->tseat, move->fseat, move->tpiece);
}

static wchar_t* __getSimpleMoveStr(wchar_t* wstr, const Move move)
{
    wchar_t iccs[6];
    if (move && move->fseat != NULL) // 排除未赋值fseat
        swprintf(wstr, WCHARSIZE, L"%02x->%02x %s %s@%c",
            getRowCol_s(move->fseat), getRowCol_s(move->tseat), getICCS(iccs, move), move->zhStr,
            (move->tpiece != BLANKPIECE ? getPieName(move->tpiece) : BLANKCHAR));
    return wstr;
}

int getAllMove(Move* moves, const Move move)
{
    int mindex = 0;
    // 提取所有前着，棋局回退至起始状态
    Move tmove = move;
    while (hasPre(tmove)) {
        moves[mindex++] = tmove;
        while (hasPreOther(tmove)) // 是变着
            tmove = getPre(tmove);
        tmove = getPre(tmove);
    }
    // 前着反序排列
    int mid = mindex / 2;
    for (int i = 0; i < mid; ++i) {
        int othi = mindex - 1 - i;
        Move amove = moves[i];
        moves[i] = moves[othi];
        moves[othi] = amove;
    }
    // 提取所有后着
    tmove = move;
    while (hasNext(tmove)) {
        tmove = getNext(tmove);
        moves[mindex++] = tmove;
    }
    return mindex;
}

wchar_t* getMoveStr(wchar_t* wstr, const Move move)
{
    wchar_t preWstr[WCHARSIZE] = { 0 }, thisWstr[WCHARSIZE] = { 0 }, nextWstr[WCHARSIZE] = { 0 }, otherWstr[WCHARSIZE] = { 0 };
    swprintf(wstr, WIDEWCHARSIZE, L"%s：%s\n现在：%s\n下着：%s\n下变：%s\n注解：               导航区%3d行%2d列\n%s",
        ((move->pmove == NULL || move->pmove->nmove == move) ? L"前着" : L"前变"),
        __getSimpleMoveStr(preWstr, move->pmove),
        __getSimpleMoveStr(thisWstr, move),
        __getSimpleMoveStr(nextWstr, move->nmove),
        __getSimpleMoveStr(otherWstr, move->omove),
        move->nextNo_, move->CC_ColNo_ + 1, move->remark);
    return wstr;
}

wchar_t* getMoveString(wchar_t* wstr, const Move move)
{
    if (hasPre(move)) {
        wchar_t fstr[WCHARSIZE], tstr[WCHARSIZE];
        swprintf(wstr, WCHARSIZE, L"fseat->tseat: %s->%s remark:%s\n nextNo:%d otherNo:%d CC_ColNo:%d\n",
            getSeatString(fstr, move->fseat), getSeatString(tstr, move->tseat),
            getRemark(move) ? getRemark(move) : L"", getNextNo(move), getOtherNo(move), getCC_ColNo(move));
    }
    return wstr;
}

// 供readXQF使用的有关解密钥匙
int Version = 0, KeyRMKSize = 0;
char KeyXYf = 0, KeyXYt = 0, F32Keys[PIECENUM] = { 0 };

unsigned char __calkey(unsigned char bKey, unsigned char cKey)
{
    return (((((bKey * bKey) * 3 + 9) * 3 + 8) * 2 + 1) * 3 + 8) * cKey; // % 256; // 保持为<256
}

static unsigned char __sub(unsigned char a, unsigned char b) { return a - b; } // 保持为<256

static void __readBytes(char* bytes, int size, FILE* fin)
{
    long pos = ftell(fin);
    fread(bytes, sizeof(char), size, fin);
    if (Version > 10) // '字节解密'
        for (int i = 0; i != size; ++i)
            bytes[i] = __sub(bytes[i], F32Keys[(pos + i) % 32]);
}

static void __readTagRowcolRemark_XQF(char* tag, int* fcolrow, int* tcolrow, wchar_t** remark, FILE* fin)
{
    char data[4] = { 0 };
    __readBytes(data, 4, fin);
    if (Version <= 10)
        data[2] = (data[2] & 0xF0 ? 0x80 : 0) | (data[2] & 0x0F ? 0x40 : 0);
    else
        data[2] &= 0xE0;
    *tag = data[2];
    //# 一步棋的起点和终点有简单的加密计算，读入时需要还原
    *fcolrow = __sub(data[0], 0X18 + KeyXYf);
    *tcolrow = __sub(data[1], 0X20 + KeyXYt);

    if (Version <= 10 || (data[2] & 0x20)) {
        char clen[4] = { 0 };
        __readBytes(clen, 4, fin);
        int RemarkSize = *(__int32*)clen - KeyRMKSize;
        if (RemarkSize > 0) {
            int len = RemarkSize + 1;
            char rem[len];
            __readBytes(rem, RemarkSize, fin);
            rem[RemarkSize] = '\x0';
            *remark = malloc(len * sizeof(wchar_t));
            mbstowcs(*remark, rem, len);
        }
    }
}

static void __readMove_XQF(Move preMove, Board board, FILE* fin, bool isOther)
{
    char tag = 0;
    int fcolrow = 0, tcolrow = 0;
    wchar_t* remark = NULL;
    __readTagRowcolRemark_XQF(&tag, &fcolrow, &tcolrow, &remark, fin);
    Move move = addMove_rc(preMove, board, fcolrow % 10, fcolrow / 10, tcolrow % 10, tcolrow / 10, remark, isOther);

    if (tag & 0x80) //# 有左子树
        __readMove_XQF(move, board, fin, false);
    if (tag & 0x40) // # 有右子树
        __readMove_XQF(move, board, fin, true);
}

void readMove_XQF(Move* rootMove, Board board, FILE* fin, bool isOther)
{
    char tag = 0;
    int fcolrow = 0, tcolrow = 0;
    wchar_t* remark = NULL;
    __readTagRowcolRemark_XQF(&tag, &fcolrow, &tcolrow, &remark, fin);
    setRemark(*rootMove, remark);

    if (tag & 0x80)
        __readMove_XQF(*rootMove, board, fin, false);
}

wchar_t* readWstring_BIN(FILE* fin)
{
    int len = 0;
    fread(&len, sizeof(int), 1, fin);
    wchar_t* wstr = malloc(len * sizeof(wchar_t));
    fread(wstr, sizeof(wchar_t), len, fin);
    return wstr;
}

static char __readMoveTagRemark_BIN(wchar_t** premark, FILE* fin)
{
    char tag = 0;
    fread(&tag, sizeof(char), 1, fin);
    if (tag & 0x20)
        *premark = readWstring_BIN(fin);
    return tag;
}

static void __readMove_BIN(Move preMove, Board board, FILE* fin, bool isOther)
{
    unsigned char frowcol, trowcol;
    fread(&frowcol, sizeof(unsigned char), 1, fin);
    fread(&trowcol, sizeof(unsigned char), 1, fin);
    wchar_t* remark = NULL;
    char tag = __readMoveTagRemark_BIN(&remark, fin);
    Move move = addMove_rowcol(preMove, board, frowcol, trowcol, remark, isOther);

    if (tag & 0x80)
        __readMove_BIN(move, board, fin, false);
    if (tag & 0x40)
        __readMove_BIN(move, board, fin, true);
}

void readMove_BIN(Move rootMove, Board board, FILE* fin, bool isOther)
{
    wchar_t* remark = NULL;
    char tag = __readMoveTagRemark_BIN(&remark, fin);
    if (remark != NULL)
        setRemark(rootMove, remark);
    if (tag & 0x80)
        __readMove_BIN(rootMove, board, fin, false);
}

void writeWstring_BIN(FILE* fout, const wchar_t* wstr)
{
    int len = wcslen(wstr) + 1;
    fwrite(&len, sizeof(int), 1, fout);
    fwrite(wstr, sizeof(wchar_t), len, fout);
}

static void __writeMoveTagRemark_BIN(Move move, FILE* fout)
{
    char tag = ((hasNext(move) ? 0x80 : 0x00)
        | (hasOther(move) ? 0x40 : 0x00)
        | (move->remark != NULL ? 0x20 : 0x00));
    fwrite(&tag, sizeof(char), 1, fout);
    if (tag & 0x20)
        writeWstring_BIN(fout, move->remark);
}

static void __writeMove_BIN(Move move, FILE* fout)
{
    if (move == NULL)
        return;
    unsigned char frowcol = getRowCol_m(move, true), trowcol = getRowCol_m(move, false);
    fwrite(&frowcol, sizeof(unsigned char), 1, fout);
    fwrite(&trowcol, sizeof(unsigned char), 1, fout);
    __writeMoveTagRemark_BIN(move, fout);

    if (hasNext(move))
        __writeMove_BIN(move->nmove, fout);
    if (hasOther(move))
        __writeMove_BIN(move->omove, fout);
}

void writeMove_BIN(FILE* fout, Move rootMove)
{
    __writeMoveTagRemark_BIN(rootMove, fout);
    if (hasNext(rootMove))
        __writeMove_BIN(rootMove->nmove, fout);
}

static wchar_t* __readMoveRemark_JSON(const cJSON* moveJSON)
{
    wchar_t* remark = NULL;
    cJSON* remarkJSON = cJSON_GetObjectItem(moveJSON, "r");
    if (remarkJSON != NULL) {
        int len = strlen(remarkJSON->valuestring) + 1;
        remark = malloc(len * sizeof(wchar_t));
        mbstowcs(remark, remarkJSON->valuestring, len);
    }
    return remark;
}

static void __readMove_JSON(Move preMove, Board board, const cJSON* moveJSON, bool isOther)
{
    int frowcol = cJSON_GetObjectItem(moveJSON, "f")->valueint;
    int trowcol = cJSON_GetObjectItem(moveJSON, "t")->valueint;
    Move move = addMove_rowcol(preMove, board, frowcol, trowcol, __readMoveRemark_JSON(moveJSON), isOther);

    cJSON* nmoveJSON = cJSON_GetObjectItem(moveJSON, "n");
    if (nmoveJSON != NULL)
        __readMove_JSON(move, board, nmoveJSON, false);

    cJSON* omoveJSON = cJSON_GetObjectItem(moveJSON, "o");
    if (omoveJSON != NULL)
        __readMove_JSON(move, board, omoveJSON, true);
}

void readMove_JSON(Move rootMove, Board board, const cJSON* rootMoveJSON, bool isOther)
{
    setRemark(rootMove, __readMoveRemark_JSON(rootMoveJSON));
    cJSON* nmoveJSON = cJSON_GetObjectItem(rootMoveJSON, "n");
    if (nmoveJSON != NULL)
        __readMove_JSON(rootMove, board, nmoveJSON, false);
}

static void __writeMoveRemark_JSON(cJSON* moveJSON, Move move)
{
    if (move->remark != NULL) {
        char remark[WIDEWCHARSIZE];
        wcstombs(remark, move->remark, WIDEWCHARSIZE);
        cJSON_AddStringToObject(moveJSON, "r", remark);
    }
}

static void __writeMove_JSON(cJSON* moveJSON, Move move)
{
    cJSON_AddNumberToObject(moveJSON, "f", getRowCol_m(move, true));
    cJSON_AddNumberToObject(moveJSON, "t", getRowCol_m(move, false));
    __writeMoveRemark_JSON(moveJSON, move);

    if (hasOther(move)) {
        cJSON* omoveJSON = cJSON_CreateObject();
        __writeMove_JSON(omoveJSON, getOther(move));
        cJSON_AddItemToObject(moveJSON, "o", omoveJSON);
    }
    if (hasNext(move)) {
        cJSON* nmoveJSON = cJSON_CreateObject();
        __writeMove_JSON(nmoveJSON, getNext(move));
        cJSON_AddItemToObject(moveJSON, "n", nmoveJSON);
    }
}

void writeMove_JSON(cJSON* rootmoveJSON, Move rootMove)
{
    __writeMoveRemark_JSON(rootmoveJSON, rootMove);
    if (hasNext(rootMove)) {
        cJSON* nmoveJSON = cJSON_CreateObject();
        __writeMove_JSON(nmoveJSON, getNext(rootMove));
        cJSON_AddItemToObject(rootmoveJSON, "n", nmoveJSON);
    }
}

void readMove_PGN_ICCSZH(Move rootMove, FILE* fin, RecFormat fmt, Board board)
{
    bool isPGN_ZH = fmt == PGN_ZH;
    wchar_t ICCSZHStr[WCHARSIZE];
    wcscpy(ICCSZHStr, L"([");
    if (isPGN_ZH) {
        wchar_t ZhChars[WCHARSIZE];
        wcscpy(ZhChars, PRECHAR);
        wcscat(ZhChars, PieceNames[RED]);
        wcscat(ZhChars, PieceNames[BLACK]);
        wcscat(ZhChars, MOVCHAR);
        wcscat(ZhChars, NUMCHAR[RED]);
        wcscat(ZhChars, NUMCHAR[BLACK]);
        wcscat(ICCSZHStr, ZhChars);
    } else {
        wchar_t ICCSChars[WCHARSIZE];
        wcscpy(ICCSChars, ICCSCOLCHAR);
        wcscat(ICCSChars, ICCSROWCHAR);
        wcscat(ICCSZHStr, ICCSChars);
    }
    wcscat(ICCSZHStr, L"]{4})");

    const wchar_t remStr[] = L"(?:[\\s\\n]*\\{([\\s\\S]*?)\\})?";
    wchar_t movePat[WCHARSIZE];
    wcscpy(movePat, L"(\\()?(?:[\\d\\.\\s]+)");
    wcscat(movePat, ICCSZHStr);
    wcscat(movePat, remStr);
    wcscat(movePat, L"(?:[\\s\\n]*(\\)+))?"); // 可能存在多个右括号
    wchar_t remPat[WCHARSIZE];
    wcscpy(remPat, remStr);
    wcscat(remPat, L"1\\.");

    const char* error;
    int erroffset = 0;
    pcre16* moveReg = pcre16_compile(movePat, 0, &error, &erroffset, NULL);
    assert(moveReg);
    pcre16* remReg = pcre16_compile(remPat, 0, &error, &erroffset, NULL);
    assert(remReg);

    wchar_t* moveStr = getWString(fin); // 读取文件内容到字符串
    int ovector[WCHARSIZE] = { 0 },
        regCount = pcre16_exec(remReg, NULL, moveStr, wcslen(moveStr), 0, 0, ovector, WCHARSIZE);
    if (regCount <= 0)
        return; // 没有move
    if (regCount == 2) { // rootMove有remark
        int len = ovector[3] - ovector[2];
        wchar_t* remark = malloc((len + 1) * sizeof(wchar_t));
        wcsncpy(remark, moveStr + ovector[2], len);
        remark[len] = L'\x0';
        setRemark(rootMove, remark); // 赋值一个动态分配内存的指针
    }

    Move move = NULL,
         preMove = rootMove,
         preOtherMoves[WIDEWCHARSIZE] = { NULL };
    int preOthIndex = 0, length = 0;
    wchar_t* pmoveStr = moveStr;
    while ((pmoveStr += ovector[1]) && (length = wcslen(pmoveStr)) > 0) {
        regCount = pcre16_exec(moveReg, NULL, pmoveStr, length, 0, 0, ovector, WCHARSIZE);
        if (regCount <= 0)
            break;
        // 是否有"("
        bool isOther = ovector[3] > ovector[2];
        if (isOther) {
            preOtherMoves[preOthIndex++] = preMove;
            if (isPGN_ZH)
                undoMove(board, preMove); // 回退前变着，以准备执行本变着
        }
        // 提取着法字符串
        int iccs_zhSize = ovector[5] - ovector[4];
        assert(iccs_zhSize > 0);
        wchar_t iccs_zhStr[6] = { 0 };
        wcsncpy(iccs_zhStr, pmoveStr + ovector[4], iccs_zhSize);
        // 提取注解
        int remarkSize = ovector[7] - ovector[6];
        wchar_t* remark = NULL;
        if (remarkSize > 0) {
            remark = malloc((remarkSize + 1) * sizeof(wchar_t));
            wcsncpy(remark, pmoveStr + ovector[6], remarkSize);
            remark[remarkSize] = L'\x0';
        }
        // 添加生成着法
        move = isPGN_ZH ? addMove_zh(preMove, board, iccs_zhStr, remark, isOther)
                        : addMove_iccs(preMove, board, iccs_zhStr, remark, isOther);
        if (isPGN_ZH)
            doMove(board, move); // 执行本着或本变着

        // 是否有一个以上的")"
        int num = ovector[9] - ovector[8];
        if (num > 0) {
            for (int i = 0; i < num; ++i) {
                preMove = preOtherMoves[--preOthIndex];
                if (isPGN_ZH) {
                    do {
                        undoMove(board, move); // 一直回退至前变着
                        move = getPre(move);
                    } while (move != preMove);
                    doMove(board, preMove); // 执行前变着，为后续执行做好准备
                }
            }
        } else
            preMove = move;
    }
    if (isPGN_ZH)
        while (move != rootMove) {
            undoMove(board, move);
            move = getPre(move);
        }
    free(moveStr);
    pcre16_free(remReg);
    pcre16_free(moveReg);
}

static void __writeRemark_PGN_ICCSZH(FILE* fout, Move move)
{
    if (getRemark(move) != NULL)
        fwprintf(fout, L" \n{%s}\n ", getRemark(move));
}

static void __writeMove_PGN_ICCSZH(FILE* fout, Move move, bool isPGN_ZH, bool isOther)
{
    wchar_t boutStr[6] = { 0 }, iccs_zhStr[6] = { 0 };
    swprintf(boutStr, 6, L"%d. ", (getNextNo(move) + 1) / 2);
    bool isEven = getNextNo(move) % 2 == 0;
    if (isOther) {
        fwprintf(fout, L"(%s", boutStr);
        if (isEven)
            fwprintf(fout, L"... ");
    } else
        fwprintf(fout, isEven ? L" " : boutStr);
    fwprintf(fout, isPGN_ZH ? getZhStr(move) : getICCS(iccs_zhStr, move));
    fwprintf(fout, L" ");
    __writeRemark_PGN_ICCSZH(fout, move);

    if (hasOther(move)) {
        __writeMove_PGN_ICCSZH(fout, getOther(move), isPGN_ZH, true);
        fwprintf(fout, L")");
    }
    if (hasNext(move))
        __writeMove_PGN_ICCSZH(fout, getNext(move), isPGN_ZH, false);
}

void writeMove_PGN_ICCSZH(FILE* fout, Move rootMove, RecFormat fmt)
{
    __writeRemark_PGN_ICCSZH(fout, rootMove);
    if (hasNext(rootMove))
        __writeMove_PGN_ICCSZH(fout, getNext(rootMove), fmt == PGN_ZH, false);
    fwprintf(fout, L"\n");
}

static wchar_t* __getRemark_PGN_CC(wchar_t* remLines[], int remCount, int row, int col)
{
    wchar_t* remark = NULL;
    wchar_t name[12] = { 0 };
    swprintf(name, 12, L"(%d,%d)", row, col);
    for (int index = 0; index < remCount; ++index)
        if (wcscmp(name, remLines[index * 2]) == 0)
            return remLines[index * 2 + 1];
    return remark;
}

static void __addMove_PGN_CC(Move preMove, Board board, pcre16* moveReg,
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
    Move move = addMove_zh(preMove, board, zhStr, __getRemark_PGN_CC(remLines, remCount, row, col), isOther);

    if (lastwc == L'…')
        __addMove_PGN_CC(move, board, moveReg,
            moveLines, rowNum, colNum, row, col + 1, remLines, remCount, true);

    if (getNextNo(move) < rowNum - 1
        && moveLines[(row + 1) * colNum + col][0] != L'　') {
        doMove(board, move);
        __addMove_PGN_CC(move, board, moveReg,
            moveLines, rowNum, colNum, row + 1, col, remLines, remCount, false);
        undoMove(board, move);
    }
}

void readMove_PGN_CC(Move rootMove, FILE* fin, Board board)
{
    const wchar_t movePat[] = L"([^…　]{4}[…　])",
                  remPat[] = L"(\\(\\d+,\\d+\\)): \\{([\\s\\S]*?)\\}";
    const char* error;
    int erroffset = 0;
    pcre16* moveReg = pcre16_compile(movePat, 0, &error, &erroffset, NULL);
    pcre16* remReg = pcre16_compile(remPat, 0, &error, &erroffset, NULL);
    assert(moveReg);
    assert(remReg);

    // 读取着法字符串
    wchar_t *moveLines[WIDEWCHARSIZE * 2], lineStr[WIDEWCHARSIZE];
    int rowNum = 0, colNum = -1;
    while (fgetws(lineStr, WIDEWCHARSIZE, fin) && lineStr[0] != L'\n') { // 空行截止
        //wprintf(L"%d: %s", __LINE__, lineStr);
        if (colNum < 0) // 只计算一次
            colNum = wcslen(lineStr) / 5;
        for (int col = 0; col < colNum; ++col) {
            wchar_t* zhStr = malloc(6 * sizeof(wchar_t));
            wcsncpy(zhStr, lineStr + col * 5, 5);
            zhStr[5] = L'\x0';
            moveLines[rowNum * colNum + col] = zhStr;
            //wprintf(L"%d: %s\n", __LINE__, moveLines[rowNum * colNum + col]);
        }
        ++rowNum;
        fgetws(lineStr, WIDEWCHARSIZE, fin); // 弃掉间隔行
    }

    // 读取注解字符串
    int remCount = 0, regCount = 0, ovector[WCHARSIZE];
    wchar_t *allremarksStr = getWString(fin),
            *remLines[WIDEWCHARSIZE],
            *remStr = NULL;
    ovector[1] = 0;
    remStr = allremarksStr;
    while (wcslen(remStr) > 0) {
        regCount = pcre16_exec(remReg, NULL, remStr, wcslen(remStr),
            0, 0, ovector, WCHARSIZE);
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

    setRemark(rootMove, __getRemark_PGN_CC(remLines, remCount, 0, 0));
    if (rowNum > 0)
        __addMove_PGN_CC(rootMove, board, moveReg, moveLines, rowNum, colNum, 1, 0, remLines, remCount, false);

    if (allremarksStr)
        free(allremarksStr);
    for (int i = rowNum * colNum - 1; i >= 0; --i)
        free(moveLines[i]);
    for (int i = 0; i < remCount; ++i) {
        free(remLines[i * 2]);
        //free(remLines[i * 2 + 1]); // 已赋值给move->remark
    }
    pcre16_free(remReg);
    pcre16_free(moveReg);
}

static void __writeMove_PGN_CC(wchar_t* moveStr, int colNum, Move move)
{
    int row = getNextNo(move) * 2, firstCol = getCC_ColNo(move) * 5;
    wcsncpy(&moveStr[row * colNum + firstCol], getZhStr(move), 4);

    if (hasOther(move)) {
        int fcol = firstCol + 4, tnum = getCC_ColNo(getOther(move)) * 5 - fcol;
        wmemset(&moveStr[row * colNum + fcol], L'…', tnum);
        __writeMove_PGN_CC(moveStr, colNum, getOther(move));
    }

    if (hasNext(move)) {
        moveStr[(row + 1) * colNum + firstCol + 2] = L'↓';
        __writeMove_PGN_CC(moveStr, colNum, getNext(move));
    }
}

void writeMove_PGN_CC(wchar_t* moveStr, int colNum, Move rootMove)
{
    if (hasNext(rootMove))
        __writeMove_PGN_CC(moveStr, colNum, getNext(rootMove));
}

static void __writeRemark_PGN_CC(wchar_t** premarkStr, int* premSize, Move move)
{
    if (getRemark(move) != NULL) {
        int len = wcslen(getRemark(move)) + 20;
        wchar_t remStr[len + 1];
        swprintf(remStr, len, L"(%d,%d): {%s}\n", getNextNo(move), getCC_ColNo(move), getRemark(move));
        // 如字符串分配的长度不够，则增加长度
        if (wcslen(*premarkStr) + len > *premSize - 1) {
            *premSize += WIDEWCHARSIZE + len;
            *premarkStr = realloc(*premarkStr, *premSize * sizeof(wchar_t));
        }
        wcscat(*premarkStr, remStr);
    }

    if (hasOther(move))
        __writeRemark_PGN_CC(premarkStr, premSize, getOther(move));
    if (hasNext(move))
        __writeRemark_PGN_CC(premarkStr, premSize, getNext(move));
}

void writeRemark_PGN_CC(wchar_t** premarkStr, int* premSize, Move rootMove)
{
    __writeRemark_PGN_CC(premarkStr, premSize, rootMove);
}
