#include "head/move.h"
#include "head/board.h"
#include "head/piece.h"
#include "head/tools.h"

struct Move {
    Seat fseat, tseat; // 起止位置0x00
    Piece tpiece; // 目标位置棋子
    wchar_t* remark; // 注解
    wchar_t zhStr[6]; // 着法名称
    bool kill, willKill, catch; // 将、杀、捉
    Move pmove, nmove, omove; // 前着、下着、变着
    int nextNo_, otherNo_, CC_ColNo_; // 走着、变着序号，文本图列号
};

// 着法相关的字符数组静态全局变量
static const wchar_t PRECHAR[] = L"前中后";
static const wchar_t MOVCHAR[] = L"退平进";
static const wchar_t NUMWCHAR[PIECECOLORNUM][BOARDCOL + 1] = { L"一二三四五六七八九", L"１２３４５６７８９" };

static Move newMove__()
{
    Move move = malloc(sizeof(struct Move));
    assert(move);
    move->fseat = move->tseat = NULL;
    move->tpiece = NULL;
    move->remark = NULL;
    wcscpy(move->zhStr, L"0000");
    move->kill = move->willKill = move->catch = false;
    move->pmove = move->nmove = move->omove = NULL;
    move->nextNo_ = move->otherNo_ = move->CC_ColNo_ = 0;
    return move;
}

static void delMove__(Move move)
{
    if (move == NULL)
        return;
    Move omove = move->omove,
         nmove = move->nmove;
    free(move->remark);
    free(move);
    delMove__(omove);
    delMove__(nmove);
}

Move getRootMove() { return newMove__(); }
void delRootMove(Move rootMove) { delMove__(rootMove); }

static wchar_t getPreChar__(bool isBottom, int count, int index)
{
    if (isBottom)
        index = count - 1 - index;
    if (count == 2 && index == 1)
        index = 2;
    if (count <= 3)
        return PRECHAR[index];
    else
        return NUMWCHAR[RED][index];
}

static int getIndex_ch__(bool isBottom, int count, wchar_t wch)
{
    int index = 0;
    wchar_t* pwch = wcschr(PRECHAR, wch);
    if (pwch) {
        index = pwch - PRECHAR;
        if (count == 2 && index == 2)
            index = 1;
    } else {
        pwch = wcschr(NUMWCHAR[RED], wch);
        assert(pwch);
        index = pwch - NUMWCHAR[RED];
    }
    if (isBottom)
        index = count - 1 - index;
    return index;
}

inline static int getIndex__(const wchar_t* str, const wchar_t ch) { return wcschr(str, ch) - str; }
inline static int getNum__(PieceColor color, wchar_t numChar) { return getIndex__(NUMWCHAR[color], numChar) + 1; }
inline static wchar_t getNumChar__(PieceColor color, int num) { return NUMWCHAR[color][num - 1]; }
inline static int getCol__(bool isBottom, int num) { return isBottom ? BOARDCOL - num : num - 1; }
inline static int getNum_Col__(bool isBottom, int col) { return (isBottom ? getOtherCol_c(col) : col) + 1; }
inline static int getMovDir__(bool isBottom, wchar_t mch) { return (getIndex__(MOVCHAR, mch) - 1) * (isBottom ? 1 : -1); }
inline static PieceColor getColor_zh__(const wchar_t* zhStr) { return wcschr(NUMWCHAR[RED], zhStr[3]) == NULL ? BLACK : RED; }

inline Move getSimplePre(CMove move) { return move->pmove; }

Move getPre(CMove move)
{
    //assert(hasSimplePre(move));
    while (hasPreOther(move)) // 推进到非变着
        move = move->pmove;
    return move->pmove;
}
inline Move getNext(CMove move) { return move->nmove; }
inline Move getOther(CMove move) { return move->omove; }

void cutMove(Move move)
{
    if (hasPreOther(move))
        move->pmove->omove = NULL;
    else if (hasSimplePre(move))
        move->pmove->nmove = NULL;
    delMove__(move);
}

int getPreMoves(Move* moves, Move move)
{
    int index = 0, nextNo = getNextNo(move);
    if (move) {
        while (hasSimplePre(move)) {
            moves[index++] = move;
            move = getPre(move);
            /*
            while (hasPreOther(move)) // 推进到非变着
                move = move->pmove;
            move = move->pmove;
            //*/
        }
        int mid = index / 2;
        for (int i = 0; i < mid; ++i) {
            int othi = index - 1 - i;
            Move amove = moves[i];
            moves[i] = moves[othi];
            moves[othi] = amove;
        }
    }
    assert(index == nextNo);
    return index;
}

int getSufMoves(Move* moves, Move move)
{
    int index = 0;
    if (move)
        while (hasNext(move)) {
            move = getNext(move);
            moves[index++] = move;
        }
    return index;
}

int getAllMoves(Move* moves, Move move)
{
    int count = getPreMoves(moves, move);
    Move sufMoves[WIDEWCHARSIZE];
    int sufCount = getSufMoves(sufMoves, move);
    for (int i = 0; i < sufCount; ++i)
        moves[count++] = sufMoves[i];
    return count;
}

inline int getNextNo(CMove move) { return move->nextNo_; }
inline int getOtherNo(CMove move) { return move->otherNo_; }
inline int getCC_ColNo(CMove move) { return move->CC_ColNo_; }

//inline PieceColor getFirstColor(CMove move) { return move->nmove == NULL ? RED : getColor(getPiece_s(move->fseat)); }

inline bool isStart(CMove move) { return !hasSimplePre(move); }
inline bool hasNext(CMove move) { return move->nmove; }
inline bool hasSimplePre(CMove move) { return move->pmove; }
inline bool hasOther(CMove move) { return move->omove; }
inline bool hasPreOther(CMove move) { return hasSimplePre(move) && move == move->pmove->omove; }
inline bool isRootMove(CMove move) { return move->pmove == NULL; }
inline bool isSameRowCol(CMove lmove, CMove pmove) { return getRowCols_m(lmove) == getRowCols_m(pmove); }
bool isConnected(CMove lmove, CMove pmove)
{
    assert(pmove);
    while (lmove && lmove != pmove)
        lmove = getPre(lmove);
    return lmove;
}

inline void setNextNo(Move move, int nextNo) { move->nextNo_ = nextNo; }
inline void setOtherNo(Move move, int otherNo) { move->otherNo_ = otherNo; }
inline void setCC_ColNo(Move move, int CC_ColNo) { move->CC_ColNo_ = CC_ColNo; }

static void setMoveSeat_zh__(Move move, Board board, const wchar_t* zhStr)
{
    assert(wcslen(zhStr) == 4);
    //wprintf(L"%d %ls\n", __LINE__, zhStr);
    //fflush(stdout);
    // 根据最后一个字符判断该着法属于哪一方
    PieceColor color = getColor_zh__(zhStr);
    bool isBottom = isBottomSide(board, color);
    int index = 0, count = 0,
        movDir = getMovDir__(isBottom, zhStr[2]);
    wchar_t name = zhStr[0];
    Seat seats[SIDEPIECENUM] = { NULL };

    if (isPieceName(name)) { // 棋子名
        count = getLiveSeats_cnc(seats,
            board, color, name, getCol__(isBottom, getNum__(color, zhStr[1])));
        if (count == 0) {
            wchar_t wstr[WIDEWCHARSIZE];
            wprintf(L"%d:\n%ls%ls\n", __LINE__, getBoardString(wstr, board), zhStr);
            fflush(stdout);
        }
        assert(count > 0);
        // 排除：士、象同列时不分前后，以进、退区分棋子。移动方向为底退、顶进时，修正index
        index = (count == 2 && movDir == -1) ? 1 : 0; // movDir == -1：表示底退、顶进
    } else { // 非棋子名
        name = zhStr[1];
        count = isPawnPieceName(name) ? getSortPawnLiveSeats(seats, board, color, name)
                                      : getLiveSeats_cn(seats, board, color, name);
        index = getIndex_ch__(isBottom, count, zhStr[0]);
    }

    //wchar_t wstr[30];
    //for (int i = 0; i < count; ++i)
    //    wprintf(L"%ls ", getSeatString(wstr, seats[i]));
    //wprintf(L"\n%ls index: %d count: %d\n", zhStr, index, count);

    if (index >= count) {
        wchar_t wstr[WIDEWCHARSIZE];
        wprintf(L"%d:\n%ls%ls index:%d count:%d\n", __LINE__, getBoardString(wstr, board), zhStr, index, count);
        fflush(stdout);
    }
    assert(index < count);
    move->fseat = seats[index];
    int num = getNum__(color, zhStr[3]), toCol = getCol__(isBottom, num),
        frow = getRow_s(move->fseat), fcol = getCol_s(move->fseat), colAway = abs(toCol - fcol); //  相距1或2列
    //wprintf(L"%d %lc %d %d %d %d %d\n", __LINE__, name, frow, fcol, movDir, colAway, toCol);
    //fflush(stdout);

    move->tseat = isLinePieceName(name) ? (movDir == 0 ? getSeat_rc(board, frow, toCol)
                                                       : getSeat_rc(board, frow + movDir * num, fcol))
                                        // 斜线走子：仕、相、马
                                        : getSeat_rc(board, frow + movDir * (isKnightPieceName(name) ? (colAway == 1 ? 2 : 1) : colAway), toCol);
    //*
    setMoveZhStr(move, board);
    if (wcscmp(zhStr, move->zhStr) != 0) {
        wchar_t wstr[WIDEWCHARSIZE];
        wprintf(L"%d:\n%szhStr:%ls move->zhStr:%ls\n", __LINE__, getBoardString(wstr, board), zhStr, move->zhStr);
        fflush(stdout);
    }
    assert(wcscmp(zhStr, move->zhStr) == 0);
    //*/
}

void setMoveZhStr(Move move, Board board)
{
    Piece fpiece = getPiece_s(move->fseat);
    if (isBlankPiece(fpiece)) {
        printf("n:%d o:%d c:%d\n", getNextNo(move), getOtherNo(move), getCC_ColNo(move));
        fflush(stdout);
        //return;
    }
    assert(!isBlankPiece(fpiece));
    PieceColor color = getColor(fpiece);
    wchar_t name = getPieName(fpiece);
    int frow = getRow_s(move->fseat), fcol = getCol_s(move->fseat),
        trow = getRow_s(move->tseat), tcol = getCol_s(move->tseat);
    bool isBottom = isBottomSide(board, color);
    Seat seats[SIDEPIECENUM] = { 0 };
    int count = getLiveSeats_cnc(seats, board, color, name, fcol);

    if (count > 1 && isStronge(fpiece)) { // 马车炮兵
        if (getKind(fpiece) == PAWN)
            count = getSortPawnLiveSeats(seats, board, color, name);
        int index = 0;
        while (move->fseat != seats[index])
            ++index;
        move->zhStr[0] = getPreChar__(isBottom, count, index);
        move->zhStr[1] = name;
    } else { //将帅, 仕(士),相(象): 不用“前”和“后”区别，因为能退的一定在前，能进的一定在后
        move->zhStr[0] = name;
        move->zhStr[1] = getNumChar__(color, getNum_Col__(isBottom, fcol));
    }
    move->zhStr[2] = MOVCHAR[frow == trow ? 1 : (isBottom == (trow > frow) ? 2 : 0)];
    move->zhStr[3] = getNumChar__(color, (isLinePieceName(name) && frow != trow) ? abs(trow - frow) : getNum_Col__(isBottom, tcol));
    move->zhStr[4] = L'\x0';

    /*
    Move amove = newMove__();
    setMoveSeat_zh__(amove, board, move->zhStr);
    if (!(move->fseat == amove->fseat && move->tseat == amove->tseat)) {
        wchar_t wstr[WIDEWCHARSIZE], fstr[WCHARSIZE], tstr[WCHARSIZE], afstr[WCHARSIZE], atstr[WCHARSIZE];
        wprintf(L"%d:\n%smove:%ls->%ls amove:%ls->%ls move-zhStr:%ls\n", __LINE__, getBoardString(wstr, board),
            getSeatString(fstr, move->fseat), getSeatString(tstr, move->tseat),
            getSeatString(afstr, move->fseat), getSeatString(atstr, move->tseat), move->zhStr);
        fflush(stdout);
    }
    assert(move->fseat == amove->fseat && move->tseat == amove->tseat);
    delMove__(amove);
    //
    //*/
}

static Move setMoveNext__(Move preMove, Move move)
{
    move->nextNo_ = preMove->nextNo_ + 1;
    move->otherNo_ = preMove->otherNo_;
    move->pmove = preMove;
    return preMove->nmove = move;
}

static Move setMoveOther__(Move preMove, Move move)
{
    move->nextNo_ = preMove->nextNo_;
    move->otherNo_ = preMove->otherNo_ + 1;
    move->pmove = preMove;
    return preMove->omove = move;
}

static Move setRemark_addMove__(Move preMove, Move move, wchar_t* remark, bool isOther)
{
    setRemark(move, remark);
    return isOther ? setMoveOther__(preMove, move) : setMoveNext__(preMove, move);
}

inline const wchar_t* getRemark(CMove move) { return move->remark; }

static const wchar_t* getRcStr_rowcol__(wchar_t* rcStr, int frowcol, int trowcol)
{
    swprintf(rcStr, 5, L"%02x%02x", frowcol, trowcol);
    return rcStr;
}

int getRowCols_m(CMove move) { return (getRowCol_s(move->fseat) << 8) | getRowCol_s(move->tseat); }

inline static const wchar_t* getZhStr(CMove move) { return move->zhStr; }

static const wchar_t* getICCS(wchar_t* ICCSStr, CMove move)
{
    if (isRootMove(move))
        wcscpy(ICCSStr, L"0000");
    else {
        //*
        swprintf(ICCSStr, 6, L"%lc%d%lc%d",
            getCol_s(move->fseat) + L'a',
            getRow_s(move->fseat),
            getCol_s(move->tseat) + L'a',
            getRow_s(move->tseat));
        //*/
        /*
        ICCSStr[0] = getCol_s(move->fseat) + L'a';
        ICCSStr[1] = getRow_s(move->fseat) + L'0';
        ICCSStr[2] = getCol_s(move->tseat) + L'a';
        ICCSStr[3] = getRow_s(move->tseat) + L'0';
        //*/
    }
    return ICCSStr;
}

static void setMoveSeat_rc__(Move move, Board board, const wchar_t* rcStr)
{
    move->fseat = getSeat_rc(board, rcStr[0] - L'0', rcStr[1] - L'0');
    move->tseat = getSeat_rc(board, rcStr[2] - L'0', rcStr[3] - L'0');
}

static void setMoveSeat_iccs__(Move move, Board board, const wchar_t* iccsStr)
{
    wchar_t rcStr[5] = { iccsStr[1], iccsStr[0] - L'a' + L'0', iccsStr[3], iccsStr[2] - L'a' + L'0' };
    setMoveSeat_rc__(move, board, rcStr);
}

Move addMove(Move preMove, Board board, const wchar_t* wstr, RecFormat fmt, wchar_t* remark, bool isOther)
{
    Move move = newMove__();
    switch (fmt) {
    case XQF:
    case BIN:
    case JSON:
        setMoveSeat_rc__(move, board, wstr);
        break;
    case PGN_ICCS:
        setMoveSeat_iccs__(move, board, wstr);
        break;
    default: // PGN_ZH PGN_CC
        setMoveSeat_zh__(move, board, wstr);
        break;
    }
    setRemark_addMove__(preMove, move, remark, isOther);

    // 以下在fmt==pgn_iccs时未成功？
    //doMove(move);
    //PieceColor color = getColor(getPiece_s(move->fseat));
    //move->kill = isKill(board, getOtherColor(color));
    //move->willKill = isWillKill(board, color);
    //move->catch = isCatch(board, color);
    //undoMove(move);

    return move;
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

void doMove(Move move)
{
    move->tpiece = movePiece(move->fseat, move->tseat, getBlankPiece());
}

void undoMove(CMove move)
{
    movePiece(move->tseat, move->fseat, move->tpiece);
}

static wchar_t* getSimpleMoveStr__(wchar_t* wstr, CMove move)
{
    wstr[0] = L'\x0';
    wchar_t iccs[6] = { 0 };
    if (move)
        swprintf(wstr, WCHARSIZE, L"%02x->%02x %ls %ls@%lc",
            getRowCol_s(move->fseat), getRowCol_s(move->tseat), getICCS(iccs, move), move->zhStr,
            (!isBlankPiece(move->tpiece) ? getPieName(move->tpiece) : getBlankChar()));
    return wstr;
}

wchar_t* getMoveStr(wchar_t* wstr, CMove move)
{
    wchar_t preWstr[WCHARSIZE] = { 0 }, thisWstr[WCHARSIZE] = { 0 }, nextWstr[WCHARSIZE] = { 0 }, otherWstr[WCHARSIZE] = { 0 };
    swprintf(wstr, WIDEWCHARSIZE, L"%s：%ls\n现在：%ls\n下着：%ls\n下变：%ls\n注解：               导航区%3d行%2d列\n%ls",
        ((move->pmove == NULL || move->pmove->nmove == move) ? L"前着" : L"前变"),
        getSimpleMoveStr__(preWstr, move->pmove),
        getSimpleMoveStr__(thisWstr, move),
        getSimpleMoveStr__(nextWstr, move->nmove),
        getSimpleMoveStr__(otherWstr, move->omove),
        move->nextNo_, move->CC_ColNo_ + 1, move->remark);
    return wstr;
}

wchar_t* getMoveString(wchar_t* wstr, CMove move)
{
    if (hasSimplePre(move)) {
        wchar_t fstr[WCHARSIZE], tstr[WCHARSIZE];
        swprintf(wstr, WCHARSIZE, L"fseat->tseat: %ls %ls->%ls remark:%ls\n nextNo:%d otherNo:%d CC_ColNo:%d\n",
            getZhStr(move), getSeatString(fstr, move->fseat), getSeatString(tstr, move->tseat),
            getRemark(move) ? getRemark(move) : L"", getNextNo(move), getOtherNo(move), getCC_ColNo(move));
    }
    return wstr;
}

extern int Version, KeyRMKSize;
extern unsigned char KeyXYf, KeyXYt, F32Keys[PIECENUM];

static unsigned char sub__(unsigned char a, unsigned char b) { return a - b; } // 保持为<256

static void readBytes__(unsigned char* bytes, int size, FILE* fin)
{
    long pos = ftell(fin);
    fread(bytes, sizeof(unsigned char), size, fin);
    if (Version > 10) // '字节解密'
        for (int i = 0; i != size; ++i)
            bytes[i] = sub__(bytes[i], F32Keys[(pos + i) % 32]);
}

static void readTagRowcolRemark_XQF__(unsigned char* tag, int* fcolrow, int* tcolrow, wchar_t** remark, FILE* fin)
{
    unsigned char data[4] = { 0 };
    readBytes__(data, 4, fin);
    if (Version <= 10)
        data[2] = (data[2] & 0xF0 ? 0x80 : 0) | (data[2] & 0x0F ? 0x40 : 0);
    else
        data[2] &= 0xE0;
    *tag = data[2];
    //# 一步棋的起点和终点有简单的加密计算，读入时需要还原
    *fcolrow = sub__(data[0], 0X18 + KeyXYf);
    *tcolrow = sub__(data[1], 0X20 + KeyXYt);

    if (Version <= 10 || (data[2] & 0x20)) {
        unsigned char clen[4] = { 0 };
        readBytes__(clen, 4, fin);
        int RemarkSize = *(int*)clen - KeyRMKSize;
        if (RemarkSize > 0) {
            size_t len = RemarkSize + 1;
            unsigned char rem[len];
            readBytes__(rem, RemarkSize, fin);
            rem[RemarkSize] = '\x0';

            *remark = calloc(len, sizeof(wchar_t));
            assert(*remark);

#ifdef __linux
            size_t outlen = len * 4;
            char remc[outlen];
            code_convert("gbk", "utf-8", (char*)rem, remc, &outlen);
            mbstowcs(*remark, remc, len);
#else
            mbstowcs(*remark, rem, len);
#endif
            //wcstombs(remc, *remark, WIDEWCHARSIZE - 1);
            //printf("\nsize:%ld %s", strlen(remc), remc);
        }
    }
}

static void readMove_XQF__(Move preMove, Board board, FILE* fin, bool isOther)
{
    unsigned char tag = 0;
    int fcolrow = 0, tcolrow = 0;
    wchar_t* remark = NULL;
    readTagRowcolRemark_XQF__(&tag, &fcolrow, &tcolrow, &remark, fin);
    int frow = fcolrow % 10, fcol = fcolrow / 10, trow = tcolrow % 10, tcol = tcolrow / 10;
    Move move;
    // 纠正某些XQF文件错误，过滤掉其中存在的重复着法
    if (preMove->fseat && preMove->tseat && getRow_s(preMove->fseat) == frow && getCol_s(preMove->fseat) == fcol
        && getRow_s(preMove->tseat) == trow && getCol_s(preMove->tseat) == tcol) {
        move = preMove;
        if (remark)
            setRemark(move, remark);
    } else {
        wchar_t rcStr[5];
        move = addMove(preMove, board, getRcStr_rowcol__(rcStr, getRowCol_rc(frow, fcol), getRowCol_rc(trow, tcol)),
            XQF, remark, isOther);
    }

    if (tag & 0x80) //# 有左子树
        readMove_XQF__(move, board, fin, false);
    if (tag & 0x40) // # 有右子树
        readMove_XQF__(move, board, fin, true);
}

void readMove_XQF(Move* rootMove, Board board, FILE* fin)
{
    unsigned char tag = 0;
    int fcolrow = 0, tcolrow = 0;
    wchar_t* remark = NULL;
    readTagRowcolRemark_XQF__(&tag, &fcolrow, &tcolrow, &remark, fin);
    setRemark(*rootMove, remark);

    if (tag & 0x80)
        readMove_XQF__(*rootMove, board, fin, false);
}

wchar_t* readWstring_BIN(FILE* fin)
{
    int len = 0;
    fread(&len, sizeof(int), 1, fin);
    wchar_t* wstr = calloc(len, sizeof(wchar_t));
    assert(wstr);
    fread(wstr, sizeof(wchar_t), len, fin);
    return wstr;
}

static char readMoveTagRemark_BIN__(wchar_t** premark, FILE* fin)
{
    char tag = 0;
    fread(&tag, sizeof(char), 1, fin);
    if (tag & 0x20)
        *premark = readWstring_BIN(fin);
    return tag;
}

static void readMove_BIN__(Move preMove, Board board, FILE* fin, bool isOther)
{
    unsigned char frowcol, trowcol;
    fread(&frowcol, sizeof(unsigned char), 1, fin);
    fread(&trowcol, sizeof(unsigned char), 1, fin);
    wchar_t* remark = NULL;
    char tag = readMoveTagRemark_BIN__(&remark, fin);
    wchar_t rcStr[5];
    Move move = addMove(preMove, board, getRcStr_rowcol__(rcStr, frowcol, trowcol), BIN, remark, isOther);

    if (tag & 0x80)
        readMove_BIN__(move, board, fin, false);
    if (tag & 0x40)
        readMove_BIN__(move, board, fin, true);
}

void readMove_BIN(Move rootMove, Board board, FILE* fin)
{
    wchar_t* remark = NULL;
    char tag = readMoveTagRemark_BIN__(&remark, fin);
    if (remark)
        setRemark(rootMove, remark);
    if (tag & 0x80)
        readMove_BIN__(rootMove, board, fin, false);
}

void writeWstring_BIN(FILE* fout, const wchar_t* wstr)
{
    int len = wcslen(wstr) + 1;
    fwrite(&len, sizeof(int), 1, fout);
    fwrite(wstr, sizeof(wchar_t), len, fout);
}

static void writeMoveTagRemark_BIN__(FILE* fout, CMove move)
{
    char tag = ((hasNext(move) ? 0x80 : 0x00)
        | (hasOther(move) ? 0x40 : 0x00)
        | (move->remark ? 0x20 : 0x00));
    fwrite(&tag, sizeof(char), 1, fout);
    if (tag & 0x20)
        writeWstring_BIN(fout, move->remark);
}

static void writeMove_BIN__(FILE* fout, CMove move)
{
    if (move == NULL)
        return;
    unsigned char frowcol = getRowCol_s(move->fseat), trowcol = getRowCol_s(move->tseat);
    fwrite(&frowcol, sizeof(unsigned char), 1, fout);
    fwrite(&trowcol, sizeof(unsigned char), 1, fout);
    writeMoveTagRemark_BIN__(fout, move);

    if (hasNext(move))
        writeMove_BIN__(fout, move->nmove);
    if (hasOther(move))
        writeMove_BIN__(fout, move->omove);
}

void writeMove_BIN(FILE* fout, CMove rootMove)
{
    writeMoveTagRemark_BIN__(fout, rootMove);
    if (hasNext(rootMove))
        writeMove_BIN__(fout, rootMove->nmove);
}

static wchar_t* readMoveRemark_JSON__(const cJSON* moveJSON)
{
    wchar_t* remark = NULL;
    cJSON* remarkJSON = cJSON_GetObjectItem(moveJSON, "r");
    if (remarkJSON) {
        int len = strlen(remarkJSON->valuestring) + 1;
        remark = calloc(len, sizeof(wchar_t));
        assert(remark);
        mbstowcs(remark, remarkJSON->valuestring, len);
    }
    return remark;
}

static void readMove_JSON__(Move preMove, Board board, const cJSON* moveJSON, bool isOther)
{
    int frowcol = cJSON_GetObjectItem(moveJSON, "f")->valueint;
    int trowcol = cJSON_GetObjectItem(moveJSON, "t")->valueint;
    wchar_t rcStr[5];
    Move move = addMove(preMove, board, getRcStr_rowcol__(rcStr, frowcol, trowcol), JSON,
        readMoveRemark_JSON__(moveJSON), isOther);

    cJSON* nmoveJSON = cJSON_GetObjectItem(moveJSON, "n");
    if (nmoveJSON)
        readMove_JSON__(move, board, nmoveJSON, false);

    cJSON* omoveJSON = cJSON_GetObjectItem(moveJSON, "o");
    if (omoveJSON)
        readMove_JSON__(move, board, omoveJSON, true);
}

void readMove_JSON(Move rootMove, Board board, const cJSON* rootMoveJSON)
{
    setRemark(rootMove, readMoveRemark_JSON__(rootMoveJSON));
    cJSON* moveJSON = cJSON_GetObjectItem(rootMoveJSON, "n");
    if (moveJSON)
        readMove_JSON__(rootMove, board, moveJSON, false);
}

static void writeMoveRemark_JSON__(cJSON* moveJSON, CMove move)
{
    if (move->remark) {
        size_t len = wcslen(move->remark) * sizeof(wchar_t) + 1;
        char remark[len];
        wcstombs(remark, move->remark, len);
        cJSON_AddStringToObject(moveJSON, "r", remark);
    }
}

static void writeMove_JSON__(cJSON* moveJSON, CMove move)
{
    cJSON_AddNumberToObject(moveJSON, "f", getRowCol_s(move->fseat));
    cJSON_AddNumberToObject(moveJSON, "t", getRowCol_s(move->tseat));
    writeMoveRemark_JSON__(moveJSON, move);

    if (hasOther(move)) {
        cJSON* omoveJSON = cJSON_CreateObject();
        writeMove_JSON__(omoveJSON, getOther(move));
        cJSON_AddItemToObject(moveJSON, "o", omoveJSON);
    }

    if (hasNext(move)) {
        cJSON* nmoveJSON = cJSON_CreateObject();
        writeMove_JSON__(nmoveJSON, getNext(move));
        cJSON_AddItemToObject(moveJSON, "n", nmoveJSON);
    }
}

void writeMove_JSON(cJSON* rootmoveJSON, CMove rootMove)
{
    writeMoveRemark_JSON__(rootmoveJSON, rootMove);

    if (hasNext(rootMove)) {
        cJSON* moveJSON = cJSON_CreateObject();
        writeMove_JSON__(moveJSON, getNext(rootMove));
        cJSON_AddItemToObject(rootmoveJSON, "n", moveJSON);
    }
}

static wchar_t* getRemark_PGN_ICCSZH__(const wchar_t* tempMoveStr, int remarkSize)
{
    wchar_t* remark = NULL;
    if (remarkSize > 0) {
        remark = calloc((remarkSize + 1), sizeof(wchar_t));
        assert(remark);
        wcsncpy(remark, tempMoveStr, remarkSize);
    }
    return remark;
}

void readMove_PGN_ICCSZH(Move rootMove, FILE* fin, RecFormat fmt, Board board)
{
    //printf("\n读取文件内容到字符串... ");
    wchar_t *moveStr = getWString(fin), *tempMoveStr = moveStr;
    if (moveStr == NULL)
        return;

    bool isPGN_ZH = fmt == PGN_ZH;
    const wchar_t* remStr = L"(?:[\\s\\n]*\\{([\\s\\S]*?)\\})?";
    wchar_t ICCSZHStr[WCHARSIZE], movePat[WCHARSIZE], remPat[WCHARSIZE];
    if (isPGN_ZH)
        swprintf(ICCSZHStr, WCHARSIZE, L"%ls%ls%ls%ls%ls",
            PRECHAR, getPieceNames(), MOVCHAR, NUMWCHAR[RED], NUMWCHAR[BLACK]);
    else
        swprintf(ICCSZHStr, WCHARSIZE, L"abcdefghi\\d");
    swprintf(movePat, WCHARSIZE, L"(\\()?(?:[\\d\\.\\s]+)([%ls]{4})%ls(?:[\\s\\n]*(\\)+))?", ICCSZHStr, remStr); // 可能存在多个右括号
    swprintf(remPat, WCHARSIZE, L"%ls1\\.", remStr);

    const char* error;
    int erroffset = 0;
    void *moveReg = pcrewch_compile(movePat, 0, &error, &erroffset, NULL),
         *remReg = pcrewch_compile(remPat, 0, &error, &erroffset, NULL);
    assert(moveReg);
    assert(remReg);

    int ovector[30] = { 0 },
        regCount = pcrewch_exec(remReg, NULL, moveStr, wcslen(moveStr), 0, 0, ovector, 30);
    if (regCount <= 0)
        return;
    setRemark(rootMove, getRemark_PGN_ICCSZH__(moveStr + ovector[2], ovector[3] - ovector[2])); // 赋值一个动态分配内存的指针

    Move move = NULL,
         preMove = rootMove,
         preOtherMoves[WIDEWCHARSIZE] = { NULL };
    int preOthIndex = 0, length = 0;
    //printf("读取moveStr... \n");
    while ((tempMoveStr += ovector[1]) && (length = wcslen(tempMoveStr)) > 0) {
        regCount = pcrewch_exec(moveReg, NULL, tempMoveStr, length, 0, 0, ovector, 30);
        if (regCount <= 0)
            break;
        // 是否有"("
        bool isOther = ovector[3] > ovector[2];
        if (isOther) {
            preOtherMoves[preOthIndex++] = preMove;
            if (isPGN_ZH)
                undoMove(preMove); // 回退前变着，以准备执行本变着
        }
        // 提取字符串
        int iccs_zhSize = ovector[5] - ovector[4];
        assert(iccs_zhSize == 4);
        wchar_t iccs_zhStr[6] = { 0 };
        wcsncpy(iccs_zhStr, tempMoveStr + ovector[4], iccs_zhSize);
        // 添加生成着法
        move = addMove(preMove, board, iccs_zhStr, fmt,
            getRemark_PGN_ICCSZH__(tempMoveStr + ovector[6], ovector[7] - ovector[6]), isOther);

        if (isPGN_ZH)
            doMove(move); // 执行本着或本变着

        // 是否有一个以上的")"
        int num = ovector[9] - ovector[8];
        if (num > 0) {
            for (int i = 0; i < num; ++i) {
                preMove = preOtherMoves[--preOthIndex];
                if (isPGN_ZH) {
                    do {
                        undoMove(move); // 一直回退至前变着
                        move = move->pmove;
                    } while (move != preMove);
                    doMove(preMove); // 执行前变着，为后续执行做好准备
                }
            }
        } else
            preMove = move;
    }
    if (isPGN_ZH)
        while (move != rootMove) {
            undoMove(move);
            move = move->pmove;
        }
    pcrewch_free(remReg);
    pcrewch_free(moveReg);
    free(moveStr);
}

static void writeRemark_PGN_ICCSZH__(FILE* fout, CMove move)
{
    if (getRemark(move))
        fwprintf(fout, L" \n{%ls}\n ", getRemark(move));
}

static void writeMove_PGN_ICCSZH__(FILE* fout, Move move, bool isPGN_ZH, bool isOther)
{
    wchar_t boutStr[WCHARSIZE], iccs[6] = { 0 };
    swprintf(boutStr, WCHARSIZE, L"%d. ", (getNextNo(move) + 1) / 2);
    bool isEven = getNextNo(move) % 2 == 0;
    fwprintf(fout, L"%ls%ls%ls%ls ",
        (isOther ? L"(" : L""),
        (isOther || !isEven ? boutStr : L" "),
        (isOther && isEven ? L"... " : L""),
        (isPGN_ZH ? getZhStr(move) : getICCS(iccs, move)));
    writeRemark_PGN_ICCSZH__(fout, move);

    if (hasOther(move)) {
        writeMove_PGN_ICCSZH__(fout, getOther(move), isPGN_ZH, true);
        fwprintf(fout, L")");
    }
    if (hasNext(move))
        writeMove_PGN_ICCSZH__(fout, getNext(move), isPGN_ZH, false);
}

void writeMove_PGN_ICCSZH(FILE* fout, CMove rootMove, RecFormat fmt)
{
    writeRemark_PGN_ICCSZH__(fout, rootMove);
    if (hasNext(rootMove))
        writeMove_PGN_ICCSZH__(fout, getNext(rootMove), fmt == PGN_ZH, false);
    fwprintf(fout, L"\n");
}

static wchar_t* getRemark_PGN_CC__(wchar_t* remLines[], int remCount, int row, int col)
{
    wchar_t name[12] = { 0 };
    swprintf(name, 12, L"(%d,%d)", row, col);
    for (int index = 0; index < remCount; ++index)
        if (wcscmp(name, remLines[index * 2]) == 0)
            return remLines[index * 2 + 1];
    return NULL;
}

static void addMove_PGN_CC__(Move preMove, Board board, void* moveReg,
    wchar_t* moveLines[], int rowNum, int colNum, int row, int col,
    wchar_t* remLines[], int remCount, bool isOther)
{
    wchar_t* zhStr = moveLines[row * colNum + col];
    while (zhStr[0] == L'…')
        zhStr = moveLines[row * colNum + (++col)];
    int ovector[9],
        regCount = pcrewch_exec(moveReg, NULL, zhStr, wcslen(zhStr), 0, 0, ovector, 9);
    /*调试用
    if (regCount <= 0) {
        wchar_t wstr[WIDEWCHARSIZE], fstr[WCHARSIZE]; //, tstr[WCHARSIZE];
        wprintf(L"%d:%d\n%spreMove:%ls zhStr:%ls\n", __LINE__, regCount, getBoardString(wstr, board),
            getMoveString(fstr, preMove), zhStr);
        for (int r = 0; r < rowNum; ++r) {
            for (int c = 0; c < colNum; ++c)
                wprintf(L"%ls", moveLines[r * colNum + c]);
            wprintf(L"\n");
        }
        for (int r = 0; r < remCount; ++r) {
            wprintf(L"%ls\n", remLines[r]);
        }
        fflush(stdout);
    }
    //*/
    assert(regCount > 0);

    wchar_t lastwc = zhStr[4];
    zhStr[4] = L'\x0';
    Move move = addMove(preMove, board, zhStr, PGN_CC, getRemark_PGN_CC__(remLines, remCount, row, col), isOther);

    if (lastwc == L'…')
        addMove_PGN_CC__(move, board, moveReg,
            moveLines, rowNum, colNum, row, col + 1, remLines, remCount, true);

    if (getNextNo(move) < rowNum - 1
        && moveLines[(row + 1) * colNum + col][0] != L'　') {
        doMove(move);
        addMove_PGN_CC__(move, board, moveReg,
            moveLines, rowNum, colNum, row + 1, col, remLines, remCount, false);
        undoMove(move);
    }
}

void readMove_PGN_CC(Move rootMove, FILE* fin, Board board)
{
    // 设置字符串容量
    wchar_t wch;
    long start = ftell(fin);
    if (start < 0)
        return;
    int rowNum = 0, rowIndex = 0, remArrayLen = 0, lineSize = 3; // lineSize 加回车和空字符位置
    while ((wch = fgetwc(fin)) && wch != L'\n')
        ++lineSize;
    int colNum = lineSize / 5;
    wchar_t lineStr[lineSize];
    fseek(fin, start, SEEK_SET); // 回到开始
    while (fgetws(lineStr, lineSize, fin) && lineStr[0] != L'\n') { // 空行截止
        ++rowNum;
        fgetws(lineStr, lineSize, fin); // 间隔行则弃掉
    }
    if (colNum == 0 || rowNum == 0)
        return;
    while (fgetws(lineStr, lineSize, fin))
        ++remArrayLen;
    wchar_t **moveLines = calloc((rowNum * colNum), sizeof(wchar_t*)),
            **remLines = calloc(remArrayLen, sizeof(wchar_t*));
    fseek(fin, start, SEEK_SET); // 回到开始

    // 读取着法字符串
    while (fgetws(lineStr, lineSize, fin) && lineStr[0] != L'\n') { // 空行截止
        //wprintf(L"%d: %ls", __LINE__, lineStr);
        for (int col = 0; col < colNum; ++col) {
            wchar_t* zhStr = calloc(6, sizeof(wchar_t));
            assert(zhStr);
            wcsncpy(zhStr, lineStr + col * 5, 5);
            moveLines[rowIndex * colNum + col] = zhStr;
            //wprintf(L"%d: %ls\n", __LINE__, moveLines[rowNum * colNum + col]);
        }
        ++rowIndex;
        fgetws(lineStr, lineSize, fin);
    }
    assert(rowNum == rowIndex);

    // 读取注解字符串
    int remCount = 0, regCount = 0, ovector[30] = { 0 };
    const wchar_t movePat[] = L"([^…　]{4}[…　])",
                  remPat[] = L"(\\(\\d+,\\d+\\)): \\{([\\s\\S]*?)\\}";
    const char* error;
    int erroffset = 0;
    void *moveReg = pcrewch_compile(movePat, 0, &error, &erroffset, NULL),
         *remReg = pcrewch_compile(remPat, 0, &error, &erroffset, NULL);
    assert(moveReg);
    assert(remReg);

    wchar_t *remarkStr = getWString(fin), *tempRemStr = remarkStr;
    while (tempRemStr != NULL && wcslen(tempRemStr) > 0) {
        regCount = pcrewch_exec(remReg, NULL, tempRemStr, wcslen(tempRemStr), 0, 0, ovector, 30);
        if (regCount <= 0)
            break;
        int rclen = ovector[3] - ovector[2], remlen = ovector[5] - ovector[4];
        wchar_t *rcKey = calloc((rclen + 1), sizeof(wchar_t)),
                *remark = calloc((remlen + 1), sizeof(wchar_t));
        assert(rcKey);
        assert(remark);
        wcsncpy(rcKey, tempRemStr + ovector[2], rclen);
        wcsncpy(remark, tempRemStr + ovector[4], remlen);
        rcKey[rclen] = L'\x0';
        remark[remlen] = L'\x0';
        remLines[remCount * 2] = rcKey;
        remLines[remCount * 2 + 1] = remark;
        //wprintf(L"%d: %ls: %ls\n", __LINE__, remLines[remCount * 2], remLines[remCount * 2 + 1]);
        ++remCount;
        tempRemStr += ovector[1];
    }

    setRemark(rootMove, getRemark_PGN_CC__(remLines, remCount, 0, 0));
    if (rowNum > 0)
        addMove_PGN_CC__(rootMove, board, moveReg, moveLines, rowNum, colNum, 1, 0, remLines, remCount, false);

    free(remarkStr);
    for (int i = 0; i < remCount; ++i) {
        free(remLines[i * 2]);
        //free(remLines[i * 2 + 1]); // 已赋值给move->remark
    }
    for (int i = rowNum * colNum - 1; i >= 0; --i)
        free(moveLines[i]);
    free(remLines);
    free(moveLines);
    pcrewch_free(remReg);
    pcrewch_free(moveReg);
}

static void writeMove_PGN_CC__(wchar_t* moveStr, int colNum, CMove move)
{
    int row = getNextNo(move) * 2, firstCol = getCC_ColNo(move) * 5;
    wcsncpy(&moveStr[row * colNum + firstCol], getZhStr(move), 4);

    if (hasOther(move)) {
        int fcol = firstCol + 4, tnum = getCC_ColNo(getOther(move)) * 5 - fcol;
        wmemset(&moveStr[row * colNum + fcol], L'…', tnum);
        writeMove_PGN_CC__(moveStr, colNum, getOther(move));
    }

    if (hasNext(move)) {
        moveStr[(row + 1) * colNum + firstCol + 2] = L'↓';
        writeMove_PGN_CC__(moveStr, colNum, getNext(move));
    }
}

void writeMove_PGN_CC(wchar_t* moveStr, int colNum, CMove rootMove)
{
    if (hasNext(rootMove))
        writeMove_PGN_CC__(moveStr, colNum, getNext(rootMove));
}

static void writeRemark_PGN_CC__(wchar_t** pstr, size_t* psize, CMove move)
{
    const wchar_t* remark = getRemark(move);
    if (remark != NULL) {
        size_t len = wcslen(remark) + 32;
        wchar_t remarkStr[len];
        swprintf(remarkStr, len, L"(%d,%d): {%ls}\n", getNextNo(move), getCC_ColNo(move), remark);
        supper_wcscat(pstr, psize, remarkStr);
    }

    if (hasOther(move))
        writeRemark_PGN_CC__(pstr, psize, getOther(move));
    if (hasNext(move))
        writeRemark_PGN_CC__(pstr, psize, getNext(move));
}

void writeRemark_PGN_CC(wchar_t** pstr, size_t* psize, CMove rootMove)
{
    writeRemark_PGN_CC__(pstr, psize, rootMove);
}

static bool isStatus__(Move move, int boutCount, bool (*func)(Move))
{
    int count = 2 * boutCount, preCount = getNextNo(move);
    if (preCount < count)
        return false;
    Move preMoves[getNextNo(move)];
    preCount = getPreMoves(preMoves, move);
    while (preCount-- > 0 && count-- > 0)
        if (!func(preMoves[preCount]))
            return false;
    return true;
}

static bool isEatPiece__(Move move) { return !isBlankPiece(move->tpiece); }
bool isNotEat(Move move, int boutCount) { return isStatus__(move, boutCount, isEatPiece__); }

static bool isKill__(Move move) { return move->kill; }
bool isContinuousKill(Move move, int boutCount) { return isStatus__(move, boutCount, isKill__); }

static bool isWillKill__(Move move) { return move->willKill; }
bool isContinuousWillKill(Move move, int boutCount) { return isStatus__(move, boutCount, isWillKill__); }

static bool isCatch__(Move move) { return move->catch; }
bool isContinuousCatch(Move move, int boutCount) { return isStatus__(move, boutCount, isCatch__); }

static bool move_equal__(CMove move0, CMove move1)
{
    return ((move0 == NULL && move1 == NULL)
        || (move0 && move1
            && isSameRowCol(move0, move1)
            && piece_equal(move0->tpiece, move1->tpiece)
            && ((move0->remark == NULL && move1->remark == NULL)
                || (move0->remark && move1->remark
                    && wcscmp(move0->remark, move1->remark) == 0))
            && wcscmp(move0->zhStr, move1->zhStr) == 0
            && move0->kill == move1->kill
            && move0->willKill == move1->willKill
            && move0->catch == move1->catch
            && move0->nextNo_ == move1->nextNo_
            && move0->otherNo_ == move1->otherNo_
            && move0->CC_ColNo_ == move1->CC_ColNo_));
}

static bool move_equalMap__(CMove move0, CMove move1)
{
    if (!move_equal__(move0, move1))
        return false;

    if (!move_equalMap__(getNext(move0), getNext(move1)))
        return false;
        
    if (!move_equalMap__(getOther(move0), getOther(move1)))
        return false;

    return true;
}

bool rootmove_equal(CMove rootmove0, CMove rootmove1)
{
    return move_equalMap__(rootmove0, rootmove1);
}
