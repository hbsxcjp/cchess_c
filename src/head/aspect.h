#ifndef ASPECT_H
#define ASPECT_H

#include "base.h"

// �ŷ���¼����
typedef struct MoveRec* MoveRec;
// �����¼����
typedef struct Aspect* Aspect;

// �½���ɾ�������ϣ��
Aspects newAspects(void);
void delAspects(Aspects aspects);

int getAspects_length(Aspects aspects);
// ȡ�þ����������ŷ���¼
MoveRec getAspect(CAspects aspects, const wchar_t* FEN);
// ����һ������(���Ѵ�����ͬ������������)��������������ŷ���¼
MoveRec putAspect(Aspects aspects, const wchar_t* FEN, CMove move);
// ɾ��ĳ�����µ�ĳ�ŷ��������������û���ŷ���ɾ������
bool removeAspect(Aspects aspects, const wchar_t* FEN, CMove move);

// ȡ�þ��������ѭ������
int getLoopCount(CAspects aspects, const wchar_t* FEN);

// ��������ַ���
void writeAspectsStr(wchar_t** pstr, int* size, CAspects aspects);

#endif