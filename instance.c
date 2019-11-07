#include "instance.h"

static const wchar_t ICCSCHAR[] = L"abcdefghi";

Move* addNext(Move* move)
{
    Move* nextMove = malloc(sizeof(Move));
    nextMove->nextNo_ = move->nextNo_ + 1;
    nextMove->otherNo_ = move->otherNo_;
    nextMove->pmove = move;
    return nextMove;
}

Move* addOther(Move* move)
{
    Move* otherMove = malloc(sizeof(Move));
    otherMove->nextNo_ = move->nextNo_;
    otherMove->otherNo_ = move->otherNo_ + 1;
    otherMove->pmove = move;
    return otherMove;
}

void delMove(Move* move)
{
    Move* cmove = move->nmove;
    while (cmove=cmove->nmove)
    {
        /* code */
    }
    
}

void cutMove(Move* move, bool isNext)
{
    Move* cmove = isNext ? move->nmove : move->omove;
    if (cmove) {
        if (cmove->omove) {
            if (isNext)
                move->nmove = cmove->omove;
            else
                move->omove = cmove->omove;
            cmove->omove = NULL;
        }
    }
    delMove(cmove);
}

wchar_t* getICCS(wchar_t* str, size_t n, const Move* move)
{
    swprintf(str, n, L"%c%d%c%d",
        ICCSCHAR[move->fseat.col], move->fseat.row,
        ICCSCHAR[move->tseat.col], move->tseat.row);
    return str;
}

wchar_t* getZH(wchar_t* str, size_t n, const Move* move)
{
    return str;
}

void moveDo(const Move* move) {}

void moveUndo(const Move* move) {}

void setMoveFromSeats(Move* move, Seat fseat, Seat tseat, wchar_t* remark) {}

void setMoveFromRowcol(Move* move,
    int frowcol, int trowcol, const wchar_t* remark) {}

void setMoveFromStr(Move* move,
    const wchar_t* str, RecFormat fmt, const wchar_t* remark) {}

wchar_t* getMovString(wchar_t* str, size_t n, const Move* move)
{
    return str;
}

void read(Instance* ins, const char* filename) {}

void write(const Instance* ins, const char* filename) {}

void go(Instance* ins) {}

void back(Instance* ins) {}

void backTo(Instance* ins, const Move* move) {}

void goOther(Instance* ins) {}

void goInc(Instance* ins, int inc) {}

void reset(Instance* ins) {}

const wchar_t* moveInfo(wchar_t* str, size_t n, const Instance* ins)
{
    return str;
}

void changeSide(Instance* ins, ChangeType ct) {}

inline void readXQF(Instance* ins, const char* filename) {}

inline void readBIN(Instance* ins, const char* filename) {}

inline void writeBIN(const Instance* ins, const char* filename) {}

inline void readJSON(Instance* ins, const char* filename) {}

inline void writeJSON(const Instance* ins, const char* filename) {}

inline void readInfo_PGN(Instance* ins, const char* filename) {}

inline void writeInfo_PGN(const Instance* ins, const char* filename) {}

inline void readMove_PGN_ICCSZH(Instance* ins, const char* filename) {}

inline void writeMove_PGN_ICCSZH(const Instance* ins, const char* filename) {}

inline void readMove_PGN_CC(Instance* ins, const char* filename) {}

inline void writeMove_PGN_CC(const Instance* ins, const char* filename) {}

inline void setMoveNums(const Instance* ins) {}

// 测试本翻译单元各种对象、函数
void testInstance(FILE* fout)
{
}
