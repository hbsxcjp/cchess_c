#include "head/move.h"
#include "head/board.h"
#include "head/piece.h"
#include "head/tools.h"

struct Move {
    Seat fseat, tseat; // 起止位置0x00
    Piece tpiece; // 目标位置棋子
    Move prev, next, other; // 前着、下着、变着
    wchar_t* remark; // 注解
    wchar_t zhStr[6]; // 着法名称

    int nextNo_, otherNo_, CC_ColNo_; // 走着、变着序号，文本图列号

    //wchar_t* fen; // 局面字符串
    bool kill, willKill, catch; // 将、杀、捉
};

Move newMove(void)
{
    Move move = malloc(sizeof(struct Move));
    assert(move);
    move->fseat = move->tseat = NULL;
    move->tpiece = getBlankPiece();
    move->prev = move->next = move->other = NULL;

    move->remark = NULL;
    wcscpy(move->zhStr, L"0000");

    move->nextNo_ = move->otherNo_ = move->CC_ColNo_ = 0;

    //move->fen = NULL;
    move->kill = move->willKill = move->catch = false;
    return move;
}

void delMove(Move move)
{
    if (move == NULL)
        return;

    Move other = move->other,
         next = move->next;
    //free(move->fen);
    free(move->remark);
    free(move);

    delMove(other);
    delMove(next);
}

inline Move getSimplePre(CMove move) { return move->prev; }

Move getPre(CMove move)
{
    //assert(getSimplePre(move));
    while (isPreOther(move)) // 推进到非变着
        move = move->prev;
    return move->prev;
}
inline Move getNext(CMove move) { return move->next; }
inline Move getOther(CMove move) { return move->other; }

void cutMove(Move move)
{
    if (isPreOther(move)) {
        move->prev->other = move->other;
        move->other = NULL;
    } else if (getSimplePre(move))
        move->prev->next = NULL;

    delMove(move);
}

int getPreMoves(Move* moves, Move move)
{
    int index = 0, nextNo = getNextNo(move);
    if (move) {
        while (getSimplePre(move)) {
            moves[index++] = move;
            move = getPre(move);
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
        while (getNext(move)) {
            move = getNext(move);
            moves[index++] = move;
        }
    return index;
}

int getAllMoves(Move* moves, Move move)
{
    int preCount = getPreMoves(moves, move);
    int sufCount = getSufMoves(moves + preCount, move);
    return preCount + sufCount;
}

inline int getNextNo(CMove move) { return move->nextNo_; }
inline int getOtherNo(CMove move) { return move->otherNo_; }
inline int getCC_ColNo(CMove move) { return move->CC_ColNo_; }

//inline PieceColor getFirstColor(CMove move) { return move->next == NULL ? RED : getColor(getPiece_s(move->fseat)); }

inline bool isRootMove(CMove move) { return move->prev == NULL; }
inline bool isPreOther(CMove move) { return move->prev != NULL && move == move->prev->other; }

void setRemark(Move move, wchar_t* remark)
{
    free(move->remark);
    move->remark = remark;
}
inline void setNextNo(Move move, int nextNo) { move->nextNo_ = nextNo; }
inline void setOtherNo(Move move, int otherNo) { move->otherNo_ = otherNo; }
inline void setCC_ColNo(Move move, int CC_ColNo) { move->CC_ColNo_ = CC_ColNo; }

void setMoveZhStr(Move move, Board board)
{
    getZhStr_seats(move->zhStr, board, move->fseat, move->tseat);
}

inline const wchar_t* getRemark(CMove move) { return move->remark; }

bool isXQFStoreError(CMove move, int frow, int fcol, int trow, int tcol)
{
    return (move->fseat && move->tseat && getRow_s(move->fseat) == frow && getCol_s(move->fseat) == fcol
        && getRow_s(move->tseat) == trow && getCol_s(move->tseat) == tcol);
}

int getFromRowCol_m(CMove move) { return getRowCol_s(move->fseat); }

int getToRowCol_m(CMove move) { return getRowCol_s(move->tseat); }

int getRowCols_m(CMove move) { return (getFromRowCol_m(move) << 8) | getToRowCol_m(move); }

const wchar_t* getZhStr(CMove move) { return move->zhStr; }

const wchar_t* getICCS_m(wchar_t* iccs, CMove move)
{
    if (isRootMove(move))
        iccs[0] = '\x0';
    else
        getICCS_s(iccs, move->fseat, move->tseat);
    return iccs;
}

const wchar_t* getICCS_mt(wchar_t* iccs, CMove move, Board board, ChangeType ct)
{
    return getICCS_s(iccs, getOtherSeat(board, move->fseat, ct), getOtherSeat(board, move->tseat, ct));
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

static bool setMoveSeat_zh__(Move move, Board board, const wchar_t* wstr)
{
    return getSeats_zh(&move->fseat, &move->tseat, board, wstr);
}

Move addMove(Move preMove, Board board, const wchar_t* wstr, RecFormat fmt, wchar_t* remark, bool isOther)
{
    bool success = true;
    Move move = newMove();
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
        success = setMoveSeat_zh__(move, board, wstr);
        break;
    }
    //*
    if (!success || !isCanMove(board, move->fseat, move->tseat)) {
        delMove(move);
        return NULL; // 添加着法失败
    }
    //*/

    // 以下在fmt==pgn_iccs时未成功？
    //doMove(move);
    //PieceColor color = getColor(getPiece_s(move->fseat));
    //move->kill = isKill(board, getOtherColor(color));
    //move->willKill = isWillKill(board, color);
    //move->catch = isCatch(board, color);
    //undoMove(move);

    setRemark(move, remark);
    if (isOther)
        preMove->other = move;
    else
        preMove->next = move;
    move->prev = preMove;

    move->nextNo_ = preMove->nextNo_ + (isOther ? 0 : 1);
    move->otherNo_ = preMove->otherNo_ + (isOther ? 1 : 0);

    return move;
}

void changeMove(Move move, Board board, ChangeType ct)
{
    if (move == NULL)
        return;

    move->fseat = getOtherSeat(board, move->fseat, ct);
    move->tseat = getOtherSeat(board, move->tseat, ct);

    changeMove(move->other, board, ct);
    changeMove(move->next, board, ct);
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
            getRowCol_s(move->fseat), getRowCol_s(move->tseat), getICCS_m(iccs, move), move->zhStr,
            (!isBlankPiece(move->tpiece) ? getPieName(move->tpiece) : getBlankChar()));
    return wstr;
}

wchar_t* getMoveStr(wchar_t* wstr, CMove move)
{
    wchar_t preWstr[WCHARSIZE] = { 0 }, thisWstr[WCHARSIZE] = { 0 }, nextWstr[WCHARSIZE] = { 0 }, otherWstr[WCHARSIZE] = { 0 };
    swprintf(wstr, WIDEWCHARSIZE, L"%s：%ls\n现在：%ls\n下着：%ls\n下变：%ls\n注解：               导航区%3d行%2d列\n%ls",
        ((move->prev == NULL || move->prev->next == move) ? L"前着" : L"前变"),
        getSimpleMoveStr__(preWstr, move->prev),
        getSimpleMoveStr__(thisWstr, move),
        getSimpleMoveStr__(nextWstr, move->next),
        getSimpleMoveStr__(otherWstr, move->other),
        move->nextNo_, move->CC_ColNo_ + 1, move->remark);
    return wstr;
}

wchar_t* getMoveString(wchar_t* wstr, CMove move)
{
    if (getSimplePre(move)) {
        wchar_t fstr[WCHARSIZE], tstr[WCHARSIZE];
        swprintf(wstr, WCHARSIZE, L"fseat->tseat: %ls %ls->%ls remark:%ls\n nextNo:%d otherNo:%d CC_ColNo:%d\n",
            getZhStr(move), getSeatString(fstr, move->fseat), getSeatString(tstr, move->tseat),
            getRemark(move) ? getRemark(move) : L"", getNextNo(move), getOtherNo(move), getCC_ColNo(move));
    }
    return wstr;
}

//inline bool isSameRowCol(CMove pmove, CMove prev) { return getRowCols_m(pmove) == getRowCols_m(prev); }

bool isConnected(CMove pmove, CMove nmove)
{
    while (pmove && pmove != nmove)
        pmove = getPre(pmove);
    return pmove;
}
/*
static void setFEN__(Move move, Board board)
{
    wchar_t FEN[SEATNUM];
    getFEN_board(FEN, board);

    move->fen = malloc((wcslen(FEN) + 1) * sizeof(wchar_t));
    wcscpy(move->fen, FEN);
}
//*/

// 满足某个函数条件返回真，否则返回假
static bool isStatus__(Move move, int boutCount, bool (*func)(Move))
{
    int count = 2 * boutCount, preCount = getNextNo(move) - 1;
    if (preCount < count - 1)
        return false;

    Move preMoves[preCount];
    preCount = getPreMoves(preMoves, move);
    while (preCount-- > 0 && count-- > 0)
        if (func(preMoves[preCount]))
            return true;

    return false;
}

static bool isEatPiece__(Move move) { return !isBlankPiece(move->tpiece); }
bool isNotEat(Move move, int boutCount) { return !isStatus__(move, boutCount, isEatPiece__); }

static bool isKill__(Move move) { return move->kill; }
bool isContinuousKill(Move move, int boutCount) { return isStatus__(move, boutCount, isKill__); }

static bool isWillKill__(Move move) { return move->willKill; }
bool isContinuousWillKill(Move move, int boutCount) { return isStatus__(move, boutCount, isWillKill__); }

static bool isCatch__(Move move) { return move->catch; }
bool isContinuousCatch(Move move, int boutCount) { return isStatus__(move, boutCount, isCatch__); }

static bool move_equal__(CMove move0, CMove move1)
{
    return (move0 && move1
        && seat_equal(move0->fseat, move1->fseat)
        && seat_equal(move0->tseat, move1->tseat)
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
        && move0->CC_ColNo_ == move1->CC_ColNo_);
}

static bool move_equalMap__(CMove move0, CMove move1)
{
    if (move0 == NULL && move1 == NULL)
        return true;
    else if (!move_equal__(move0, move1))
        return false;

    if (!move_equalMap__(getNext(move0), getNext(move1)))
        return false;

    if (!move_equalMap__(getOther(move0), getOther(move1)))
        return false;

    return true;
}

bool rootmove_equal(CMove rootmove0, CMove rootmove1)
{
    if (!((rootmove0->remark == NULL && rootmove1->remark == NULL)
            || (rootmove0->remark && rootmove1->remark
                && wcscmp(rootmove0->remark, rootmove1->remark) == 0)))
        return false;

    return move_equalMap__(rootmove0, rootmove1);
}
