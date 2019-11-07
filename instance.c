#include "instance.h"
#include "board.h"
#include "move.h"
#include "piece.h"

void read(Instance* ins, const char* filename) {}

void write(const Instance* ins, const char* filename) {}

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
    while (!isSame(ins->currentMove, ins->rootMove) && !isSame(ins->currentMove, move)) {
        back(ins);
    }
}

void goOther(Instance* ins)
{
    if (!isSame(ins->currentMove, ins->rootMove) && ins->currentMove->omove != NULL) {
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
