#ifndef MOVE_H
#define MOVE_H

#include "base.h"
#include "cJSON.h"

// 新建根着法
Move getRootMove();
// 删除rootMove的所有下着move、变着move及自身
void delRootMove(Move rootMove);

// 着法系列
Move getSimplePre(CMove move); // 取得简单前着-数据内部表示的前个链接
Move getPre(CMove move);
Move getNext(CMove move);
Move getOther(CMove move);
void cutMove(Move move); //(未测试)
int getPreMoves(Move* moves, Move move);
int getSufMoves(Move* moves, Move move);
int getAllMoves(Move* moves, Move move);

int getNextNo(CMove move);
int getOtherNo(CMove move);
int getCC_ColNo(CMove move);

bool isStart(CMove move);
bool hasNext(CMove move);
bool hasSimplePre(CMove move);
bool hasOther(CMove move);
bool hasPreOther(CMove move);
bool isRootMove(CMove move);
// 判断着法是否相同
bool isSameMove(CMove lmove, CMove pmove);
// 判断着法是否连通（是直接前后着关系，而不是平行的变着关系）
bool isConnected(CMove lmove, CMove pmove);

void setNextNo(Move move, int nextNo);
void setOtherNo(Move move, int otherNo);
void setCC_ColNo(Move move, int CC_ColNo);

const wchar_t* getRemark(CMove move);
// 获取行列整数值 "rcrc"
int getRowCols_m(CMove move);
//const wchar_t* getZhStr(CMove move);
//const wchar_t* getICCS(wchar_t* ICCSStr, CMove move);

// 添加着法
Move addMove(Move preMove, Board board, const wchar_t* wstr, RecFormat fmt, wchar_t* remark, bool isOther);

// 设置着法的中文字符串
void setMoveZhStr(Move move, Board board);
// 设置remark
void setRemark(Move move, wchar_t* remark);

// 按某种变换类型变换着法记录
void changeMove(Move move, Board board, ChangeType ct);

// 执行或回退
void doMove(Move move);
void undoMove(CMove move);

// 输出表示着法的字符串
wchar_t* getMoveStr(wchar_t* wstr, CMove move);
wchar_t* getMoveString(wchar_t* wstr, CMove move);

// XQF格式
void readMove_XQF(Move* rootMove, Board board, FILE* fin);
// bin格式
wchar_t* readWstring_BIN(FILE* fin);
void readMove_BIN(Move rootMove, Board board, FILE* fin);
void writeWstring_BIN(FILE* fout, const wchar_t* wstr);
void writeMove_BIN(FILE* fout, CMove rootMove);
// json格式
void readMove_JSON(Move rootMove, Board board, const cJSON* rootMoveJSON);
void writeMove_JSON(cJSON* rootmoveJSON, CMove rootMove);
// pgn_iccs/pgn_zh格式
void readMove_PGN_ICCSZH(Move rootMove, FILE* fin, RecFormat fmt, Board board);
void writeMove_PGN_ICCSZH(FILE* fout, CMove rootMove, RecFormat fmt);
// pgn_cc格式
void readMove_PGN_CC(Move rootMove, FILE* fin, Board board);
void writeMove_PGN_CC(wchar_t* moveStr, int colNum, CMove rootMove);
void writeRemark_PGN_CC(wchar_t** premarkStr, int* premSize, CMove rootMove);

// 判断给定着法及回合数内，是否未吃一子
bool isNotEat(Move move, int boutCount); //(未测试)
// 凡走子连续不停照将，而形成循环者，称为“长将”
bool isContinuousKill(Move move, int boutCount); //(未测试)
// 凡走子连续不停杀着，而形成循环者，称为“长杀”
bool isContinuousWillKill(Move move, int boutCount); //(未测试)
// 凡走子连续追捉一子或数子，而形成循环者，称为“长捉”
bool isContinuousCatch(Move move, int boutCount); //(未测试)

#endif