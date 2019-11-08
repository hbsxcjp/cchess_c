#include "head/instance.h"
#include "head/board.h"
#include "head/move.h"
#include "head/piece.h"

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
    Instance* ins = newInstance();
    //read(ins, "a.pgn");
    wchar_t tempStr[TEMPSTR_SIZE];
    fwprintf(fout, L"testInstance:\n%sinfo:%s\n",
        getBoardString(tempStr, ins->board), ins->info);
    fwprintf(fout, L"   rootMove:%s ",
        getMovString(tempStr, TEMPSTR_SIZE, ins->rootMove));
    fwprintf(fout, L"currentMove:%s ",
        getMovString(tempStr, TEMPSTR_SIZE, ins->currentMove));
    fwprintf(fout, L"movCount:%d remCount:%d remLenMax:%d maxRow:%d maxCol:%d\n",
        ins->movCount_, ins->remCount_, ins->remLenMax_,
        ins->maxRow_, ins->maxCol_);
    delInstance(ins);
}
