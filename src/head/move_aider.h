#ifndef MOVE_AIDER_H
#define MOVE_AIDER_H

#include "base.h"

// 着法记录类型
typedef struct MoveRec* MoveRec;
// 局面记录类型
typedef struct AspectRec* AspectRec;

AspectRecTable newAspectRecTable(void);
void delAspectRecTable(AspectRecTable table);

int aspectRec_length(AspectRecTable table);
MoveRec aspectRec_get(AspectRecTable table, const wchar_t* pieceStr);
MoveRec aspectRec_put(AspectRecTable table, const wchar_t* pieceStr, CMove move);
MoveRec aspectRec_remove(AspectRecTable table, const wchar_t* pieceStr);

// 取得局面最近回合已循环次数
int getLoopCount(Move move); //(未测试)

#endif