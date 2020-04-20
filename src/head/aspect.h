#ifndef ASPECT_H
#define ASPECT_H

#include "base.h"

// �ŷ���¼����
typedef struct MoveRec* MoveRec;
// �����¼����
typedef struct Aspect* Aspect;

// ��ϣ����
unsigned int BKDRHash(wchar_t* wstr);
// �½���ɾ�������ϣ��
AspectTable newAspectTable(void);
void delAspectTable(AspectTable table);

int aspectTable_length(AspectTable table);
// ȡ�þ����������ŷ���¼
MoveRec aspectTable_get(AspectTable table, const wchar_t* pieceStr);
// ����һ������(���Ѵ�����ͬ������������)��������������ŷ���¼
MoveRec aspectTable_put(AspectTable table, const wchar_t* pieceStr, CMove move);
// ɾ��ĳ�����µ�ĳ�ŷ��������������û���ŷ���ɾ������
bool aspectTable_remove(AspectTable table, const wchar_t* pieceStr, CMove move);

// ȡ�þ��������ѭ������
int getLoopCount(AspectTable table, const wchar_t* pieceStr);

#endif