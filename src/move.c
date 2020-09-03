#include "head/move.h"
#include "head/board.h"
#include "head/piece.h"
#include "head/tools.h"

struct Move {
    Seat fseat, tseat; // 起止位置0x00
    Piece tpiece; // 目标位置棋子
    //wchar_t* fen; // 局面字符串
    wchar_t* remark; // 注解
    wchar_t zhStr[6]; // 着法名称
    bool kill, willKill, catch; // 将、杀、捉
    Move pmove, nmove, omove; // 前着、下着、变着
    int nextNo_, otherNo_, CC_ColNo_; // 走着、变着序号，文本图列号
};

struct MoveRec {
    char text[WCHARSIZE];
};

Move newMove()
{
    Move move = malloc(sizeof(struct Move));
    assert(move);
    move->fseat = move->tseat = NULL;
    move->tpiece = getBlankPiece();
    //move->fen = NULL;
    move->remark = NULL;
    wcscpy(move->zhStr, L"0000");
    move->kill = move->willKill = move->catch = false;
    move->pmove = move->nmove = move->omove = NULL;
    move->nextNo_ = move->otherNo_ = move->CC_ColNo_ = 0;
    return move;
}

void delMove(Move move)
{
    if (move == NULL)
        return;
    Move omove = move->omove,
         nmove = move->nmove;
    //free(move->fen);
    free(move->remark);
    free(move);
    delMove(omove);
    delMove(nmove);
}

inline Move getSimplePre(CMove move) { return move->pmove; }

Move getPre(CMove move)
{
    //assert(getSimplePre(move));
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
    else if (getSimplePre(move))
        move->pmove->nmove = NULL;
    delMove(move);
}

int getPreMoves(Move* moves, Move move)
{
    int index = 0, nextNo = getNextNo(move);
    if (move) {
        while (getSimplePre(move)) {
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
        while (getNext(move)) {
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

inline bool isStart(CMove move) { return !getSimplePre(move); }
inline bool hasPreOther(CMove move) { return getSimplePre(move) && move == move->pmove->omove; }
inline bool isRootMove(CMove move) { return move->pmove == NULL; }
//inline bool isSameRowCol(CMove lmove, CMove pmove) { return getRowCols_m(lmove) == getRowCols_m(pmove); }
bool isConnected(CMove lmove, CMove pmove)
{
    assert(pmove);
    while (lmove && lmove != pmove)
        lmove = getPre(lmove);
    return lmove;
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

const char* getIccs_m(char* iccs, CMove move)
{
    if (isRootMove(move))
        iccs[0] = '\x0';
    else
        getIccs_s(iccs, move->fseat, move->tseat);
    return iccs;
}

const char* getOtherIccs_m(char* iccs, CMove move, Board board, ChangeType ct)
{
    return getIccs_s(iccs, getOtherSeat(board, move->fseat, ct), getOtherSeat(board, move->tseat, ct));
}

const wchar_t* getICCS(wchar_t* iccs, CMove move)
{
    char str[6];
    mbstowcs(iccs, getIccs_m(str, move), 6);
    return iccs;
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
        //setMoveSeat_zh__(move, board, wstr);
        getSeats_zh(&move->fseat, &move->tseat, board, wstr);
        break;
    }
    //setFEN__(move, board);
    //setRemark_addMove__(preMove, move, remark, isOther);
    setRemark(move, remark);
    isOther ? setMoveOther__(preMove, move) : setMoveNext__(preMove, move);

    // 以下在fmt==pgn_iccs时未成功？
    //doMove(move);
    //PieceColor color = getColor(getPiece_s(move->fseat));
    //move->kill = isKill(board, getOtherColor(color));
    //move->willKill = isWillKill(board, color);
    //move->catch = isCatch(board, color);
    //undoMove(move);

    return move;
}

void changeMove(Move move, Board board, ChangeType ct)
{
    if (move == NULL)
        return;
    move->fseat = getOtherSeat(board, move->fseat, ct);
    move->tseat = getOtherSeat(board, move->tseat, ct);

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
    if (getSimplePre(move)) {
        wchar_t fstr[WCHARSIZE], tstr[WCHARSIZE];
        swprintf(wstr, WCHARSIZE, L"fseat->tseat: %ls %ls->%ls remark:%ls\n nextNo:%d otherNo:%d CC_ColNo:%d\n",
            getZhStr(move), getSeatString(fstr, move->fseat), getSeatString(tstr, move->tseat),
            getRemark(move) ? getRemark(move) : L"", getNextNo(move), getOtherNo(move), getCC_ColNo(move));
    }
    return wstr;
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
