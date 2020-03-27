#ifndef MOVE_H
#define MOVE_H

#include "base.h"

// 新建一个着法
Move newMove();

// 删除move的所有下着move、变着move及自身
void freeMove(Move move);

// 添加着法
Move addMove_rc(Move preMove, Board board, int frow, int fcol, int trow, int tcol, wchar_t* remark, bool isOther);
Move addMove_rowcol(Move preMove, Board board, int frowcol, int trowcol, wchar_t* remark, bool isOther);
Move addMove_iccs(Move preMove, Board board, const wchar_t* iccsStr, wchar_t* remark, bool isOther);
Move addMove_zh(Move preMove, Board board, const wchar_t* zhStr, wchar_t* remark, bool isOther);

int getRowCol_m(Move move, bool isFirst);

Move getPre(Move move);
Move getNext(Move move);
Move getOther(Move move);

int getNextNo(Move move);
int getOtherNo(Move move);
int getCC_ColNo(Move move);

const wchar_t* getRemark(Move move);
const wchar_t* getZhStr(Move move);
wchar_t* getICCS(wchar_t* ICCSStr, Move move);

PieceColor getFirstColor(Move move);

bool isStart(Move move);

bool hasNext(Move move);

bool hasPre(Move move);

bool hasOther(Move move);

bool hasPreOther(Move move);

void setNextNo(Move move, int nextNo);
void setOtherNo(Move move, int otherNo);
void setCC_ColNo(Move move, int CC_ColNo);

// 设置着法的中文字符串
void setMoveZhStr(Move move, Board board);

void setTPiece(Move move, Piece tpiece);

// 设置remark
void setRemark(Move move, wchar_t* remark);

// 切除move
void cutMove(Move move);

// 按某种变换类型变换着法记录
void changeMove(Move move, Board board, ChangeType ct);

// 执行或回退
void doMove(Board board, Move move);
void undoMove(Board board, Move move);

wchar_t* getMoveStr(wchar_t* wstr, const Move move);

// XQF格式
void readMove_XQF(Move* rootMove, Board board, FILE* fin, bool isOther);

// bin格式
wchar_t* readWstring_BIN(FILE* fin);
void readMove_BIN(Move rootMove, Board board, FILE* fin, bool isOther);
void writeWstring_BIN(FILE* fout, const wchar_t* wstr);
void writeMove_BIN(FILE* fout, Move rootMove);

// json格式
void readMove_JSON(Move rootMove, Board board, const cJSON* rootMoveJSON, bool isOther);
void writeMove_JSON(cJSON* rootmoveJSON, Move rootMove);

// pgn_iccs/pgn_zh格式
void readMove_PGN_ICCSZH(Move rootMove, FILE* fin, RecFormat fmt, Board board);
void writeMove_PGN_ICCSZH(FILE* fout, Move rootMove, RecFormat fmt);

// pgn_cc格式
void readMove_PGN_CC(Move rootMove, FILE* fin, Board board);
void writeMove_PGN_CC(wchar_t* moveStr, int colNum, Move rootMove);
void writeRemark_PGN_CC(wchar_t* remarkStr, int* premSize, Move rootMove);

#endif