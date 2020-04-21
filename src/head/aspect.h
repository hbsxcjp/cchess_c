#ifndef ASPECT_H
#define ASPECT_H

#include "base.h"

// �ŷ���¼����
typedef struct MoveRec* MoveRec;
// �����¼����
typedef struct Aspect* Aspect;

// ��ϣ����
unsigned int BKDRHash(const wchar_t* wstr);
// �½���ɾ�������ϣ��
AspectTable newAspectTable(void);
void delAspectTable(AspectTable table);

int aspectTable_length(AspectTable table);
// ȡ�þ����������ŷ���¼
MoveRec aspectTable_get(AspectTable table, const wchar_t* FEN);
// ����һ������(���Ѵ�����ͬ������������)��������������ŷ���¼
MoveRec aspectTable_put(AspectTable table, const wchar_t* FEN, CMove move);
// ɾ��ĳ�����µ�ĳ�ŷ��������������û���ŷ���ɾ������
bool aspectTable_remove(AspectTable table, const wchar_t* FEN, CMove move);

// ȡ�þ��������ѭ������
int getLoopCount(AspectTable table, const wchar_t* FEN);

// ����ŷ���¼���ַ���
wchar_t* getMoveRecStr(wchar_t* wstr, MoveRec mrc);
// ��������¼���ַ���
wchar_t* getAspectStr(wchar_t* wstr, Aspect asp);
// ��������ϣ����ַ���
wchar_t* getAspectTableStr(wchar_t* wstr, AspectTable table);

#endif