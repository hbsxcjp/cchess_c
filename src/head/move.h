#ifndef MOVE_H
#define MOVE_H

#include "base.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

// 新建根着法
Move newMove(void);
// 删除move的所有下着move、变着move及自身
void delMove(Move move);

// 取得简单前着（数据内部表示的前个链接），可能是前着，也可能是前变着
Move getSimplePre(CMove move);
// 取得前着，非前变着
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
bool hasPreOther(CMove move);
bool isRootMove(CMove move);

// 判断着法是否相同
//bool isSameRowCol(CMove lmove, CMove pmove);
// 判断着法是否连通（是直接前后着关系，而不是平行的变着关系）
bool isConnected(CMove lmove, CMove pmove);

// 设置remark
void setRemark(Move move, wchar_t* remark);
void setNextNo(Move move, int nextNo);
void setOtherNo(Move move, int otherNo);
void setCC_ColNo(Move move, int CC_ColNo);

const wchar_t* getRemark(CMove move);

int getFromRowCol_m(CMove move);
int getToRowCol_m(CMove move);
// 获取行列整数值 "rcrc"
int getRowCols_m(CMove move);
const wchar_t* getZhStr(CMove move);
// 获取iccs着法描述
const wchar_t* getICCS_m(wchar_t* iccs, CMove move);
// 获取棋盘转换后的iccs着法描述
const wchar_t* getICCS_mt(wchar_t* iccs, CMove move, Board board, ChangeType ct);

// 检测是否存在一个XQF文件存储时的错误
bool isXQFStoreError(CMove move, int frow, int fcol, int trow, int tcol);

// 添加着法
Move addMove(Move preMove, Board board, const wchar_t* wstr, RecFormat fmt, wchar_t* remark, bool isOther);

// 设置着法的中文字符串
void setMoveZhStr(Move move, Board board);

// 按某种变换类型变换着法记录
void changeMove(Move move, Board board, ChangeType ct);

// 执行或回退
void doMove(Move move);
void undoMove(CMove move);

// 输出表示着法的字符串
wchar_t* getMoveStr(wchar_t* wstr, CMove move);
wchar_t* getMoveString(wchar_t* wstr, CMove move);

// 判断给定着法及回合数内，是否未吃一子
bool isNotEat(Move move, int boutCount); //(未测试)
// 凡走子连续不停照将，而形成循环者，称为“长将”
bool isContinuousKill(Move move, int boutCount); //(未测试)
// 凡走子连续不停杀着，而形成循环者，称为“长杀”
bool isContinuousWillKill(Move move, int boutCount); //(未测试)
// 凡走子连续追捉一子或数子，而形成循环者，称为“长捉”
bool isContinuousCatch(Move move, int boutCount); //(未测试)

bool rootmove_equal(CMove rootmove0, CMove rootmove1);

#endif