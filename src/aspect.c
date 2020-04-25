#include "head/aspect.h"
#include "head/board.h"
#include "head/move.h"
#include "head/piece.h"
#include "head/tools.h"

struct MoveRec {
    CMove move; // 着法指针
    MoveRec preMoveRec;
};

struct Aspect {
    wchar_t* FEN;
    MoveRec lastMoveRec;
    Aspect preAspect;
};

struct Aspects {
    int size, length, movCount;
    Aspect* lastAspects;
};

static MoveRec newMoveRec__(MoveRec preMoveRec, CMove move)
{
    assert(move);
    MoveRec mrc = malloc(sizeof(struct MoveRec));
    assert(mrc);
    mrc->move = move;
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
    free(aspect->FEN);
    delMoveRec__(aspect->lastMoveRec);
    free(aspect);
    delAspect__(preAspect);
}

Aspects newAspects(void)
{
    int size = 509; //509,1021,2053,4093,8191,16381,32771,65521,INT_MAX
    Aspects aspects = malloc(sizeof(struct Aspects) + size * sizeof(Aspect*));
    aspects->size = size;
    aspects->length = aspects->movCount = 0;
    aspects->lastAspects = (Aspect*)(aspects + 1);
    for (int i = 0; i < size; ++i)
        aspects->lastAspects[i] = NULL;
    return aspects;
}

void delAspects(Aspects aspects)
{
    assert(aspects);
    for (int i = 0; i < aspects->size; ++i)
        delAspect__(aspects->lastAspects[i]);
    free(aspects);
}

int getAspects_length(Aspects aspects) { return aspects->length; }

// 取得最后的局面记录
static Aspect* getLastAspect__(CAspects aspects, const wchar_t* FEN)
{
    return &aspects->lastAspects[BKDRHash(FEN) % aspects->size];
}

// 取得相同哈希值下相同局面的记录
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
    Aspect *plarc = getLastAspect__(aspects, FEN), arc = getAspect__(*plarc, FEN);
    if (arc == NULL) { // 表中不存在该局面，则添加aspect、move
        arc = newAspect__(*plarc, FEN, move);
        assert(arc);
        *plarc = arc;
        aspects->length++;
    } else // 表中已存在该局面，则添加move
        arc->lastMoveRec = newMoveRec__(arc->lastMoveRec, move);
    aspects->movCount++;
    assert(arc->lastMoveRec);
    return arc->lastMoveRec;
}

MoveRec putAspect_b(Aspects aspects, Board board, CMove move)
{
    wchar_t FEN[SEATNUM + 1];
    return putAspect(aspects, getFEN_board(FEN, board), move);
}

bool removeAspect(Aspects aspects, const wchar_t* FEN, CMove move)
{
    bool finish = false;
    Aspect *plarc = getLastAspect__(aspects, FEN), arc = getAspect__(*plarc, FEN);
    if (arc) {
        MoveRec lmrc = arc->lastMoveRec, mrc = lmrc;
        while (mrc) {
            if (mrc->move == move) {
                // 系最近着法
                if (arc->lastMoveRec == mrc) {
                    // 有前着法
                    if (mrc->preMoveRec)
                        arc->lastMoveRec = mrc->preMoveRec;
                    // 本局面已无着法，则删除局面
                    else {
                        if (*plarc == arc)
                            *plarc = NULL;
                        else {
                            Aspect larc = *plarc;
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

int getLoopBoutCount(CAspects aspects, const wchar_t* FEN)
{
    int boutCount = 0;
    MoveRec lmrc = getAspect(aspects, FEN), mrc = lmrc;
    if (lmrc)
        while ((mrc = mrc->preMoveRec))
            if (isSameMove(lmrc->move, mrc->move) && isConnected(lmrc->move, mrc->move)) {
                boutCount = (getNextNo(lmrc->move) - getNextNo(lmrc->move)) / 2;
                break;
            }
    return boutCount;
}

static void writeMoveRecStr__(FILE* fout, MoveRec mrc)
{
    if (mrc == NULL)
        return;
    wchar_t iccs[6];
    fwprintf(fout, L"\t\tmove@:%p iccs:%s\n", mrc->move, getICCS(iccs, mrc->move));
    writeMoveRecStr__(fout, mrc->preMoveRec);
}

static void writeAspectStr__(FILE* fout, Aspect asp)
{
    if (asp == NULL)
        return;
    fwprintf(fout, L"\tFEN:%s\n", asp->FEN);
    writeMoveRecStr__(fout, asp->lastMoveRec);
    writeAspectStr__(fout, asp->preAspect);
}

void writeAspectsStr(FILE* fout, CAspects aspects)
{
    assert(aspects);
    int a = 0;
    for (int i = 0; i < aspects->size; ++i) {
        Aspect lasp = aspects->lastAspects[i];
        if (lasp) {
            fwprintf(fout, L"\n%3d. HASH:%4d\n", ++a, i);
            writeAspectStr__(fout, lasp);
        }
    }
    fwprintf(fout, L"\naspect_count:%d aspect_movCount:%d ", aspects->length, aspects->movCount);
}

//
const double P = 1.0; //平均每个桶装的元素个数的上限 ,实测貌似1.0效果比较好
int E; //目前使用了哈希值的前 E 位来分组
int R; //实际装入本哈希表的元素总数
int N; //目前使用的桶的个数
/*
操作过程中，始终维护两个性质   
1. R/N <= P          可以推出  max(N) = max(R/P) = maxn/P   所以，所需链表的个数为 maxn/P 
2. 2^(E-1) <=  N  < 2^E
*/
int p2[33]; //记录2的各个次方  p2[i]=2^i
int mask[33]; //记录掩码 mask[i]=p2[i]-1
bool ERROR; //错误信息
//
int hash(int x)
{ //32位哈希函数
    return x * 2654435769;
}
bool hashEq(int x, int y)
{ //判断x与y在当前条件下属不属于一个桶
    return (x & mask[E]) == (y & mask[E]);
}
//
int currentHash(int Hash)
{ //当前哈希值
    Hash = Hash & mask[E];
    return Hash < N ? Hash : Hash & mask[E - 1];
}

typedef struct ListNode* ListNode;
struct ListNode { //链表节点定义
    int Hash; //32位哈希值，根据Key计算，通常为 hash(Key)
    int Key; //键值，唯一
    int Value; //键值Key对应的值
    ListNode* next; //指向链表中的下一节点，或者为空

    //构造函数
    ListNode() {}
    ListNode(int H, int K, int V)
        : Hash(H)
        , Key(K)
        , Value(V)
    {
    }
};
struct List { //链表定义
    ListNode* Head; //头指针

    //构造函数 析构函数
    List()
        : Head(NULL)
    {
    }
    ~List() { clear(); }

    //插入函数
    void Insert(int H, int K, int V)
    {
        Insert(new ListNode(H, K, V));
    }
    void Insert(ListNode* temp)
    {
        temp->next = Head;
        Head = temp;
    }

    //转移函数
    void Transfer(int H, List* T)
    { //将本链表中，Hash值掩码之后为H的元素加入到链表T中去。
        ListNode *temp, *p;
        while (Head && hashEq(Head->Hash, H)) {
            temp = Head;
            Head = Head->next;
            T->Insert(temp);
        }
        p = Head;
        while (p && p->next) {
            if (hashEq(p->next->Hash, H)) {
                temp = p->next;
                p->next = p->next->next;
                T->Insert(temp);
            } else
                p = p->next;
        }
    }

    //寻找函数
    int Find(int Key)
    {
        ERROR = false;
        ListNode* temp = Head;
        while (temp) {
            if (temp->Key == Key)
                return temp->Value;
            temp = temp->next;
        }
        return ERROR = true;
    }

    //显示函数
    void Show()
    {
        ListNode* temp = Head;
        while (temp) {
            printf("(%d,%d) ", temp->Key, temp->Value);
            temp = temp->next;
        }
    }

    //释放申请空间
    void clear()
    {
        while (Head) {
            ListNode* temp = Head;
            Head = Head->next;
            delete temp;
        }
    }
} L[100000];

//初始化
void Init()
{
    p2[0] = 1;
    for (int i = 1; i <= 32; ++i)
        p2[i] = p2[i - 1] << 1;
    for (int i = 0; i <= 32; ++i)
        mask[i] = p2[i] - 1;
    E = 1;
    N = 1;
    R = 0;
    L[0] = List();
}

//调整
void Adjust()
{
    while ((double)R / N > P) {
        //将属于N的信息加入List[N]
        L[N & mask[E - 1]].Transfer(N, &L[N]);
        //更正 N 和 E
        if (++N >= p2[E])
            ++E;
        L[N] = List();
    }
}

//插入
void Insert(int Hash, int Key, int Value)
{
    //插入元素
    L[currentHash(Hash)].Insert(Hash, Key, Value);
    ++R;
    //调整 N 和 E
    Adjust();
}

//寻找
int Find(int Hash, int Key)
{
    return L[currentHash(Hash)].Find(Key);
}
//释放所有
void FreeAll()
{
    for (int i = 0; i < N; ++i)
        L[i].clear();
}
//显示
void ShowList()
{
    OUT3(E, R, N);
    for (int i = 0; i < N; ++i) {
        printf("%d:", i);
        L[i].Show();
        printf("\n");
    }
}
/*
使用上述模板需要知道的外部函数：
void Init() :初始化  在所有操作之前运行 
void FreeAll():全部释放  在所有操作之后运行 
void Insert(Hash,Key,Value):加入元素，这里的Hash是32位Hash ，一般取Hash=hash(Key)
int Find(Hash,Key):找到键值Key对应的Value 
调用Find之后，若全局变量ERROR为true 则表示没有找到，此时返回值无效，否则返回值为Value 
void ShowList():显示所有桶的元素 
*/
