#ifndef MOVE_AIDER_H
#define MOVE_AIDER_H

#include "base.h"

// �ŷ���¼����
typedef struct MoveRec* MoveRec;
// �����¼����
typedef struct AspectRec* AspectRec;

AspectRecTable newAspectRecTable(void);
void delAspectRecTable(AspectRecTable table);

int aspectRec_length(AspectRecTable table);
MoveRec aspectRec_get(AspectRecTable table, const wchar_t* pieceStr);
MoveRec aspectRec_put(AspectRecTable table, const wchar_t* pieceStr, CMove move);
MoveRec aspectRec_remove(AspectRecTable table, const wchar_t* pieceStr);

// ȡ�þ�������غ���ѭ������
int getLoopCount(Move move); //(δ����)

#endif