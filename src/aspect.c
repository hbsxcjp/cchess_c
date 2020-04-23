#include "head/aspect.h"
#include "head/board.h"
#include "head/move.h"
#include "head/piece.h"
#include "head/tools.h"

struct MoveRec {
    CMove move; // �ŷ�ָ��
    wchar_t iccs[6];
    MoveRec preMoveRec;
};

struct Aspect {
    wchar_t* FEN;
    MoveRec lastMoveRec;
    Aspect preAspect;
};

struct Aspects {
    int size, length;
    Aspect* lastAspects;
};

static MoveRec newMoveRec__(MoveRec preMoveRec, CMove move)
{
    assert(move);
    MoveRec mrc = malloc(sizeof(struct MoveRec));
    assert(mrc);
    mrc->move = move;
    getICCS(mrc->iccs, move);
    mrc->preMoveRec = preMoveRec;
    return mrc;
}

static void delMoveRec__(MoveRec mrc)
{
    if (mrc == NULL)
        return;
    MoveRec preMoveRec = mrc->preMoveRec;
    free(mrc);
    delMoveRec__(preMoveRec);
}

static Aspect newAspect__(Aspect preAspect, const wchar_t* FEN, CMove move)
{
    assert(FEN);
    Aspect aspect = malloc(sizeof(struct Aspect));
    assert(aspect);
    aspect->FEN = malloc((wcslen(FEN) + 1) * sizeof(wchar_t));
    wcscpy(aspect->FEN, FEN);
    aspect->lastMoveRec = newMoveRec__(NULL, move);
    aspect->preAspect = preAspect;
    return aspect;
}

static void delAspect__(Aspect aspect)
{
    if (aspect == NULL)
        return;
    Aspect preAspect = aspect->preAspect;
    delMoveRec__(aspect->lastMoveRec);
    free(aspect->FEN);
    free(aspect);
    delAspect__(preAspect);
}

Aspects newAspects(void)
{
    int size = 509;
    Aspects aspects = malloc(sizeof(struct Aspects) + size * sizeof(Aspect*));
    aspects->size = size;
    aspects->length = 0;
    aspects->lastAspects = (Aspect*)(aspects + 1);
    for (int i = 0; i < size; ++i)
        aspects->lastAspects[i] = NULL;
    return aspects;
}

void delAspects(Aspects aspects)
{
    assert(aspects);
    if (aspects->length > 0) {
        Aspect arc, pre = NULL;
        for (int i = 0; i < aspects->size; ++i)
            for (arc = aspects->lastAspects[i]; arc; arc = pre) {
                pre = arc->preAspect;
                delAspect__(arc);
            }
    }
    free(aspects);
}

int getAspects_length(Aspects aspects) { return aspects->length; }

// ȡ�����ľ����¼
static Aspect* getLastAspect__(CAspects aspects, const wchar_t* FEN)
{
    return &aspects->lastAspects[BKDRHash(FEN) % aspects->size];
}

// ȡ����ͬ��ϣֵ����ͬ����ļ�¼
static Aspect getAspect__(Aspect arc, const wchar_t* FEN)
{
    while (arc && wcscmp(arc->FEN, FEN) != 0)
        arc = arc->preAspect;
    return arc;
}

MoveRec getAspect(CAspects aspects, const wchar_t* FEN)
{
    Aspect arc = getAspect__(*getLastAspect__(aspects, FEN), FEN);
    return arc ? arc->lastMoveRec : NULL;
}

MoveRec putAspect(Aspects aspects, const wchar_t* FEN, CMove move)
{
    Aspect *parc = getLastAspect__(aspects, FEN), arc = getAspect__(*parc, FEN);
    if (arc == NULL) { // ���в����ڣ������
        arc = newAspect__(*parc, FEN, move);
        *parc = arc;
        aspects->length++;
    }
    return arc->lastMoveRec;
}

bool removeAspect(Aspects aspects, const wchar_t* FEN, CMove move)
{
    bool finish = false;
    Aspect *parc = getLastAspect__(aspects, FEN), arc = getAspect__(*parc, FEN);
    if (arc) {
        MoveRec lmrc = arc->lastMoveRec, mrc = lmrc;
        while (mrc) {
            if (mrc->move == move) {
                // ϵ����ŷ�
                if (arc->lastMoveRec == mrc) {
                    // ��ǰ�ŷ�
                    if (mrc->preMoveRec)
                        arc->lastMoveRec = mrc->preMoveRec;
                    // �����������ŷ�����ɾ������
                    else {
                        if (*parc == arc)
                            *parc = NULL;
                        else {
                            Aspect larc = *parc;
                            while (larc->preAspect != arc)
                                larc = larc->preAspect;
                            larc->preAspect = arc->preAspect;
                        }
                        delAspect__(arc);
                    }
                } else
                    lmrc->preMoveRec = mrc->preMoveRec;
                delMoveRec__(mrc);
                finish = true;
                break;
            } else {
                lmrc = mrc;
                mrc = mrc->preMoveRec;
            }
        }
    }
    return finish;
}

int getLoopCount(CAspects aspects, const wchar_t* FEN)
{
    int count = 0;
    MoveRec lmrc = getAspect(aspects, FEN);
    if (lmrc) {
        MoveRec mrc = lmrc->preMoveRec;
        while (mrc) {
            // �ŷ����
            if (wcscmp(lmrc->iccs, mrc->iccs) == 0) {
                // �ж��ŷ��Ƿ���ͨ����ֱ��ǰ���Ź�ϵ��������ƽ�еı��Ź�ϵ��
                CMove move = lmrc->move;
                while ((move = getPre(move))) {
                    if (move == mrc->move) {
                        count++;
                        break;
                    }
                }
            }
            mrc = mrc->preMoveRec;
        }
    }
    return count;
}

static void writeMoveRecStr__(wchar_t** pstr, int* size, MoveRec mrc)
{
    if (mrc == NULL)
        return;
    wchar_t wstr[WCHARSIZE];
    swprintf(wstr, WCHARSIZE, L"\t\tmove@:%p iccs:%s\n", mrc->move, mrc->iccs);
    writeWString(pstr, size, wstr);
    writeMoveRecStr__(pstr, size, mrc->preMoveRec);
}

static void writeMoveRecStr(wchar_t** pstr, int* size, MoveRec mrc)
{
    if (mrc)
        writeMoveRecStr__(pstr, size, mrc);
}

static void writeAspectStr__(wchar_t** pstr, int* size, Aspect asp)
{
    if (asp == NULL)
        return;
    wchar_t wstr[WCHARSIZE];
    swprintf(wstr, WCHARSIZE, L"\tFEN:%s\n", asp->FEN);
    writeWString(pstr, size, wstr);
    writeMoveRecStr(pstr, size, asp->lastMoveRec);
    writeAspectStr__(pstr, size, asp->preAspect);
}

static void writeAspectStr(wchar_t** pstr, int* size, Aspect asp)
{
    if (asp)
        writeAspectStr__(pstr, size, asp);
}

void writeAspectsStr(wchar_t** pstr, int* size, CAspects aspects)
{
    assert(aspects);
    wchar_t wstr[WCHARSIZE];
    for (int i = 0; i < aspects->size; ++i) {
        Aspect lasp = aspects->lastAspects[i];
        if (lasp) {
            swprintf(wstr, WCHARSIZE, L"\n%3d.\n", i);
            writeWString(pstr, size, wstr);
            writeAspectStr(pstr, size, lasp);
        }
    }
    swprintf(wstr, WCHARSIZE, L"\naspect count:%3d\n", aspects->length);
    writeWString(pstr, size, wstr);
}
