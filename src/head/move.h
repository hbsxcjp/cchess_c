#ifndef MOVE_H
#define MOVE_H

#include "base.h"

// 新建一个move
PMove newMove(void);

// 判断两个move是否为同一个
bool isSameMove(const PMove amove, const PMove bmove);

// 加入下着
PMove addNext(PMove move);

// 加入变着
PMove addOther(PMove move);

// 删除move的所有下着move、变着move及自身
void delMove(PMove move);

// 切除move的下着
void cutNextMove(PMove move);

// 切除move的变着
void cutOhterMove(PMove move);

// 根据iccs着法设置内部着法
void setMove_iccs(PMove move, const wchar_t* iccsStr);

// 取得ICCS字符串
wchar_t* getICCS(wchar_t* ICCSStr, const PMove move);

// 辅助函数：根据中文着法取得走棋方颜色
PieceColor getColor_zh(const wchar_t* zhStr);

wchar_t* getMoveStr(wchar_t* wstr, const PMove move);

// 根据中文着法设置内部着法
void setMove_zh(PMove move, const PBoard board, const wchar_t* zhStr);

// 根据内部着法表示设置中文着法
void setZhStr(PMove move, const PBoard board);
//wchar_t* getZhStr(wchar_t* zhStr, const PBoard board, const PMove  move);

// 设置remark
void setRemark(PMove move, const wchar_t* remark);

// 按某种变换类型变换着法记录
void changeMove(PMove move, ChangeType ct);


#endif