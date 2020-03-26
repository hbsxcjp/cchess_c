#include "head/move.h"
#include "head/board.h"
#include "head/piece.h"

struct Move {
    Seat fseat, tseat; // 起止位置0x00
    Piece tpiece; // 目标位置棋子
    wchar_t* remark; // 注解
    wchar_t zhStr[6]; // 着法名称
    struct Move *pmove, *nmove, *omove; // 前着、下着、变着
    int nextNo_, otherNo_, CC_ColNo_; // 走着、变着序号，文本图列号
};

// 着法相关的字符数组静态全局变量
static const wchar_t ICCSCOLCHAR[] = L"abcdefghi";
static const wchar_t ICCSROWCHAR[] = L"0123456789";
static const wchar_t PRECHAR[] = L"前中后";
static const wchar_t MOVCHAR[] = L"退平进";
static const wchar_t NUMCHAR[PIECECOLORNUM][BOARDCOL + 1] = {
    L"一二三四五六七八九", L"１２３４５６７８９"
};

static wchar_t* __getPreCHars(wchar_t* preChars, int count)
{
    if (count == 2) {
        preChars[0] = PRECHAR[0];
        preChars[1] = PRECHAR[2];
        preChars[2] = L'\x0';
    } else if (count == 3)
        wcscpy(preChars, PRECHAR);
    else // count == 4,5
        wcsncpy(preChars, NUMCHAR[RED], 5);
    return preChars;
}

static int __getIndex(const wchar_t* str, wchar_t ch) { return wcschr(str, ch) - str; }

static int __getNum(PieceColor color, wchar_t numChar) { return __getIndex(NUMCHAR[color], numChar) + 1; }

static int __getCol(bool isBottom, int num) { return isBottom ? BOARDCOL - num : num - 1; }

static int __getMovDir(bool isBottom, wchar_t mch) { return (__getIndex(MOVCHAR, mch) - 1) * (isBottom ? 1 : -1); }

static PieceColor __getColor_zh(const wchar_t* zhStr) { return wcschr(NUMCHAR[RED], zhStr[3]) == NULL ? BLACK : RED; }

static void __setMoveSeat_rc(Move move, Board board, int frow, int fcol, int trow, int tcol)
{
    move->fseat = getSeat_rc(board, frow, fcol);
    move->tseat = getSeat_rc(board, trow, tcol);
    move->tpiece = getPiece_s(move->tseat);
}

static void __setMoveSeat_iccs(Move move, Board board, const wchar_t* iccsStr)
{
    __setMoveSeat_rc(move, board,
        __getIndex(ICCSROWCHAR, iccsStr[1]), __getIndex(ICCSCOLCHAR, iccsStr[0]),
        __getIndex(ICCSROWCHAR, iccsStr[3]), __getIndex(ICCSCOLCHAR, iccsStr[2]));
}

int getRowCol_m(Move move, bool isFirst) { return getRowCol_s(isFirst ? move->fseat : move->tseat); }

wchar_t* getRemark(Move move) { return move->remark; }

wchar_t* getICCS(wchar_t* ICCSStr, const Move move)
{
    swprintf(ICCSStr, 5, L"%c%d%c%d",
        ICCSCOLCHAR[getCol_s(move->fseat)], getRow_s(move->fseat),
        ICCSCOLCHAR[getCol_s(move->tseat)], getRow_s(move->tseat));
    return ICCSStr;
}

PieceColor getFirstColor(Move move) { return move->nmove == NULL ? RED : getColor(getPiece_s(move->fseat)); }

bool isStart(Move move) { return !hasPre(move); }

bool hasNext(Move move) { return move->nmove != NULL; }

bool hasPre(Move move) { return move->pmove != NULL; }

bool hasOther(Move move) { return move->omove != NULL; }

bool hasPreOther(Move move) { return hasPre(move) && move == move->pmove->omove; }

static void __setMoveSeat_zh(Move move, Board board, const wchar_t* zhStr)
{
    assert(wcslen(zhStr) == 4);
    // 根据最后一个字符判断该着法属于哪一方
    PieceColor color = __getColor_zh(zhStr);
    bool isBottom = isBottomSide(board, color);
    int index = 0, count = 0,
        movDir = __getMovDir(isBottom, zhStr[2]);
    wchar_t name = zhStr[0];
    Seat seats[PIECENUM / 2] = { NULL };

    if (isPieceName(name)) { // 棋子名
        count = getLiveSeats(seats,
            board, color, name, __getCol(isBottom, __getNum(color, zhStr[1])), false);
        assert(count > 0);
        // 排除：士、象同列时不分前后，以进、退区分棋子。移动方向为退时，修正index
        index = (count == 2 && movDir == -1) ? 1 : 0; // 如为士象且退时
    } else { // 非棋子名
        name = zhStr[1];
        count = isPawnPieceName(name) ? getSortPawnLiveSeats(seats, board, color, name)
                                      : getLiveSeats(seats, board, color, name, -1, false);
        wchar_t preChars[6] = { 0 };
        __getPreCHars(preChars, count);
        assert(wcschr(preChars, zhStr[0]) != NULL);
        index = __getIndex(preChars, zhStr[0]);
        if (isBottom)
            index = count - 1 - index;
    }

    //wprintf(L"%s index: %d count: %d\n", zhStr, index, count);
    assert(index < count);
    move->fseat = seats[index];
    int num = __getNum(color, zhStr[3]), toCol = __getCol(isBottom, num),
        frow = getRow_s(move->fseat), fcol = getCol_s(move->fseat), colAway = abs(toCol - fcol); //  相距1或2列
    move->tseat = isLinePiece(name) ? (movDir == 0 ? getSeat_rc(board, frow, toCol)
                                                   : getSeat_rc(board, frow + movDir * num, fcol))
                                    // 斜线走子：仕、相、马
                                    : getSeat_rc(board,
                                          frow + movDir * (isKnightPieceName(name) ? colAway : (colAway == 1 ? 2 : 1)), toCol);

    //__setMoveZhStr(move, board);
    //assert(wcscmp(zhStr, move->zhStr) == 0);
    //
}

void setMoveZhStr(Move move, Board board)
{
    Piece fpiece = getPiece_s(move->fseat);
    assert(fpiece != BLANKPIECE);
    PieceColor color = getColor(fpiece);
    wchar_t name = getPieName(fpiece);
    int frow = getRow_s(move->fseat), fcol = getCol_s(move->fseat),
        trow = getRow_s(move->tseat), tcol = getCol_s(move->tseat);
    bool isBottom = isBottomSide(board, color);
    Seat seats[PIECENUM] = { 0 };
    int count = getLiveSeats(seats, board, color, name, fcol, false);

    if (count > 1 && getKind(fpiece) > KNIGHT) { // 马车炮兵
        if (getKind(fpiece) == PAWN)
            count = getSortPawnLiveSeats(seats, board, color, name);
        wchar_t preChars[6] = { 0 };
        __getPreCHars(preChars, count);
        int index = 0;
        while (move->fseat != seats[index])
            ++index;
        assert(index < count);
        move->zhStr[0] = preChars[isBottom ? count - 1 - index : index];
        move->zhStr[1] = name;
    } else { //将帅, 仕(士),相(象): 不用“前”和“后”区别，因为能退的一定在前，能进的一定在后
        move->zhStr[0] = name;
        move->zhStr[1] = NUMCHAR[color][isBottom ? getOtherCol_c(fcol) : fcol];
    }
    move->zhStr[2] = MOVCHAR[frow == trow ? 1 : (isBottom == (trow > frow) ? 2 : 0)];
    move->zhStr[3] = NUMCHAR[color][(isLinePiece(name) && frow != trow) ? abs(trow - frow) - 1
                                                                        : (isBottom ? getOtherCol_c(tcol) : tcol)];
    move->zhStr[4] = L'\x0';

    //
    //wchar_t iccsStr[12], boardStr[WIDEWCHARSIZE];
    //wprintf(L"iccs: %s zh:%s\n%s\n",
    //    __getICCS(iccsStr, move), zhStr, getBoardString(boardStr, board));
    //

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

Move newMove() { return calloc(sizeof(struct Move), 1); }

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

void setTPiece(Move move, Piece tpiece) { move->tpiece = tpiece; }

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
    setTPiece(move, movePiece(board, move->fseat, move->tseat, BLANKPIECE));
}

void undoMove(Board board, Move move)
{
    movePiece(board, move->tseat, move->fseat, move->tpiece);
}

static wchar_t* __getSimpleMoveStr(wchar_t* wstr, const Move move)
{
    wchar_t iccs[6];
    if (move && move->fseat != NULL) // 排除未赋值fseat
        wsprintfW(wstr, L"%02x->%02x %s %s@%c",
            getRowCol_s(move->fseat), getRowCol_s(move->tseat), getICCS(iccs, move), move->zhStr,
            (move->tpiece != BLANKPIECE ? getPieName(move->tpiece) : BLANKCHAR));
    return wstr;
}

wchar_t* getMoveStr(wchar_t* wstr, const Move move)
{
    wchar_t preWstr[WCHARSIZE] = { 0 }, thisWstr[WCHARSIZE] = { 0 }, nextWstr[WCHARSIZE] = { 0 }, otherWstr[WCHARSIZE] = { 0 };
    wsprintfW(wstr, L"%s：%s\n现在：%s\n下着：%s\n下变：%s\n注解：               导航区%3d行%2d列\n%s",
        ((move->pmove == NULL || move->pmove->nmove == move) ? L"前着" : L"前变"),
        __getSimpleMoveStr(preWstr, move->pmove),
        __getSimpleMoveStr(thisWstr, move),
        __getSimpleMoveStr(nextWstr, move->nmove),
        __getSimpleMoveStr(otherWstr, move->omove),
        move->nextNo_, move->CC_ColNo_ + 1, move->remark);
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

static void __readMove_XQF(Move preMove, Board board, FILE* fin, bool isOther)
{
    char data[4] = { 0 };
    __readBytes(data, 4, fin);
    if (Version <= 10)
        data[2] = (data[2] & 0xF0 ? 0x80 : 0) | (data[2] & 0x0F ? 0x40 : 0);
    else
        data[2] &= 0xE0;
    char* rem = NULL;
    if (Version <= 10 || (data[2] & 0x20)) {
        char clen[4] = { 0 };
        __readBytes(clen, 4, fin);
        int RemarkSize = *(__int32*)clen - KeyRMKSize;
        if (RemarkSize > 0) {
            rem = malloc(RemarkSize + 1);
            __readBytes(rem, RemarkSize, fin);
            rem[RemarkSize] = '\0';
        }
    }

    //# 一步棋的起点和终点有简单的加密计算，读入时需要还原
    int fcolrow = __sub(data[0], 0X18 + KeyXYf),
        tcolrow = __sub(data[1], 0X20 + KeyXYt);
    //wprintf(L"%3d=> %d->%d\n", __LINE__, fcolrow, tcolrow);
    // assert(fcolrow <= 89 && tcolrow <= 89); 根节点不能断言！
    wchar_t* remark = NULL;
    if (rem != NULL) {
        int length = strlen(rem) + 1;
        remark = malloc(length * sizeof(wchar_t));
        mbstowcs(remark, rem, length);
        free(rem);
    }
    Move move = addMove_rc(preMove, board, fcolrow % 10, fcolrow / 10, tcolrow % 10, tcolrow / 10, remark, isOther);

    if (data[2] & 0x80) //# 有左子树
        __readMove_XQF(move, board, fin, false);
    if (data[2] & 0x40) // # 有右子树
        __readMove_XQF(move, board, fin, true);
}

void readMove_XQF(Move* rootMove, Board board, FILE* fin, bool isOther)
{
    Move tempPreMove = newMove();
    __readMove_XQF(tempPreMove, board, fin, false);
    *rootMove = &tempPreMove->nmove; // XQF文件存储了rootMove的无用数据，需要读取出来
    tempPreMove->nmove = NULL; //切断关联
    freeMove(tempPreMove);
}

wchar_t* readWstring_BIN(FILE* fin)
{
    int len = 0;
    fread(&len, sizeof(int), 1, fin);
    wchar_t* wstr = malloc(len * sizeof(wchar_t));
    fread(wstr, sizeof(wchar_t), len, fin);
    return wstr;
}

static void __readMove_BIN(Move preMove, Board board, FILE* fin, bool isOther)
{
    unsigned char frowcol, trowcol;
    fread(&frowcol, sizeof(unsigned char), 1, fin);
    fread(&trowcol, sizeof(unsigned char), 1, fin);
    char tag = 0;
    fread(&tag, sizeof(char), 1, fin);
    wchar_t* remark = NULL;
    if (tag & 0x20)
        remark = __readWstring_BIN(fin);
    Move move = addMove_rowcol(preMove, board, frowcol, trowcol, remark, isOther);

    if (tag & 0x80)
        __readMove_BIN(move, board, fin, false);
    if (tag & 0x40)
        __readMove_BIN(move, board, fin, true);
}

void readMove_BIN(Move rootMove, Board board, FILE* fin, bool isOther)
{
    __readMove_BIN(rootMove, board, fin, false);
}

void writeWstring_BIN(const wchar_t* wstr, FILE* fout)
{
    int len = wcslen(wstr) + 1;
    fwrite(&len, sizeof(int), 1, fout);
    fwrite(wstr, sizeof(wchar_t), len, fout);
}

static void __writeMove_BIN(Move move, FILE* fout)
{
    if (move == NULL)
        return;
    unsigned char frowcol = getRowCol_m(move, true), trowcol = getRowCol_m(move, false);
    fwrite(&frowcol, sizeof(unsigned char), 1, fout);
    fwrite(&trowcol, sizeof(unsigned char), 1, fout);
    char tag = ((hasNext(move) ? 0x80 : 0x00)
        | (move->omove != NULL ? 0x40 : 0x00)
        | (move->remark != NULL ? 0x20 : 0x00));
    fwrite(&tag, sizeof(char), 1, fout);
    if (tag & 0x20)
        writeWstring_BIN(move->remark, fout);

    if (tag & 0x80)
        __writeMove_BIN(move->nmove, fout);
    if (tag & 0x40)
        __writeMove_BIN(move->omove, fout);
}

void writeMove_BIN(Move rootMove, FILE* fout)
{
    if (hasNext(rootMove))
        __writeMove_BIN(rootMove->nmove, fout);
}

static void __readMove_JSON(Move preMove, Board board, const cJSON* moveJSON, bool isOther)
{
    int frowcol = cJSON_GetObjectItem(moveJSON, "f")->valueint;
    int trowcol = cJSON_GetObjectItem(moveJSON, "t")->valueint;
    cJSON* remarkJSON = cJSON_GetObjectItem(moveJSON, "r");
    wchar_t* remark = NULL;
    if (remarkJSON != NULL) {
        int len = strlen(remarkJSON->valuestring) + 1;
        remark = malloc(len * sizeof(wchar_t));
        mbstowcs(remark, remarkJSON->valuestring, len);
    }
    Move move = addMove_rowcol(preMove, board, frowcol, trowcol, remark, isOther);

    cJSON* nmoveJSON = cJSON_GetObjectItem(moveJSON, "n");
    if (nmoveJSON != NULL)
        __readMove_JSON(move, board, nmoveJSON, false);

    cJSON* omoveJSON = cJSON_GetObjectItem(moveJSON, "o");
    if (omoveJSON != NULL)
        __readMove_JSON(move, board, omoveJSON, true);
}

void readMove_JSON(Move rootMove, Board board, const cJSON* rootMoveJSON, bool isOther)
{
    __readMove_JSON(rootMove, board, rootMoveJSON, false);
}

static void __writeMove_JSON(cJSON* moveJSON, Move move)
{
    cJSON_AddNumberToObject(moveJSON, "f", getRowCol_m(move, true));
    cJSON_AddNumberToObject(moveJSON, "t", getRowCol_m(move, false));
    if (move->remark != NULL) {
        //int len = wcslen(move->remark) * sizeof(wchar_t) + 1;
        char remark[WCHARSIZE];
        wcstombs(remark, move->remark, WCHARSIZE);
        cJSON_AddStringToObject(moveJSON, "r", remark);
    }

    if (hasNext(move)) {
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

void writeMove_JSON(cJSON* rootmoveJSON, Move rootMove)
{
    __writeMove_JSON(rootmoveJSON, rootMove);
}