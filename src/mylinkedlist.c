#include "head/mylinkedlist.h"

// 内部节点类
typedef struct Node* Node;
struct Node {
    void* data;

    Node prev;
    Node next;
};

static Node newNode__(void* data, Node prev, Node next)
{
    Node node = malloc(sizeof(struct Node));
    node->data = data;

    node->prev = prev;
    node->next = next;
    return node;
}

static void delNode__(Node node, void (*delData)(void*))
{
    if (node == NULL)
        return;

    Node next = node->next;
    if (node->data)
        delData(node->data);
    free(node);

    delNode__(next, delData);
}

struct MyLinkedList {
    Node beginMarker;
    Node endMarker;

    int theSize;
    int modCount;

    // 节点内数据对象内存释放函数
    void (*delData)(void*);
};

MyLinkedList newMyLinkedList(void (*delData)(void*))
{
    MyLinkedList myLinkedList = malloc(sizeof(struct MyLinkedList));
    myLinkedList->beginMarker = newNode__(NULL, NULL, NULL);
    myLinkedList->endMarker = newNode__(NULL, myLinkedList->beginMarker, NULL);
    myLinkedList->beginMarker->next = myLinkedList->endMarker;

    myLinkedList->theSize = 0;
    myLinkedList->modCount = 0;

    myLinkedList->delData = delData;
    return myLinkedList;
}

void delMyLinkedList(MyLinkedList myLinkedList)
{
    clearMyLinkedList(myLinkedList);
    free(myLinkedList);
}

void clearMyLinkedList(MyLinkedList myLinkedList)
{
    delNode__(myLinkedList->beginMarker, myLinkedList->delData);
    myLinkedList->beginMarker = NULL;
    myLinkedList->endMarker = NULL;

    myLinkedList->theSize = 0;
    myLinkedList->modCount++;
}

int myLinkedList_size(MyLinkedList myLinkedList)
{
    return myLinkedList->theSize;
}

bool myLinkedList_isempty(MyLinkedList myLinkedList)
{
    return myLinkedList_size(myLinkedList) == 0;
}

static void addBeforeMyLinkedList__(MyLinkedList myLinkedList, Node node, void* data)
{
    node->prev = node->prev->next = newNode__(data, node->prev, node); // 见数据结构与算法分析 P59.
    myLinkedList->theSize++;
    myLinkedList->modCount++;
}

static void removeMyLinkedList__(MyLinkedList myLinkedList, Node node)
{
    node->next->prev = node->prev;
    node->prev->next = node->next;
    myLinkedList->theSize--;
    myLinkedList->modCount++;
}

static Node getNodeMyLinkedList_idx__(MyLinkedList myLinkedList, int idx)
{
    int size = myLinkedList_size(myLinkedList);
    if (idx < 0 && idx > size)
        return myLinkedList->endMarker;

    Node node = NULL;
    if (idx < size / 2) {
        node = myLinkedList->beginMarker->next; // 默认idx==0
        for (int i = 0; i < idx; ++i)
            node = node->next;
    } else {
        node = myLinkedList->endMarker;
        for (int i = size; i > idx; --i)
            node = node->prev;
    }

    return node;
}

void addMyLinkedList(MyLinkedList myLinkedList, void* data)
{
    addMyLinkedList_idx(myLinkedList, myLinkedList_size(myLinkedList), data);
}

void addMyLinkedList_idx(MyLinkedList myLinkedList, int idx, void* data)
{
    addBeforeMyLinkedList__(myLinkedList, getNodeMyLinkedList_idx__(myLinkedList, idx), data);
}

void* getDataMyLinkedList_idx(MyLinkedList myLinkedList, int idx)
{
    return getNodeMyLinkedList_idx__(myLinkedList, idx)->data;
}

static void setNodeMyLinkedList__(MyLinkedList myLinkedList, Node node, void* newData)
{
    if (node == myLinkedList->endMarker)
        addMyLinkedList(myLinkedList, newData);
    else {
        myLinkedList->delData(node->data);
        node->data = newData;
    }
}

void setMyLinkedList_idx(MyLinkedList myLinkedList, int idx, void* newData)
{
    Node node = getNodeMyLinkedList_idx__(myLinkedList, idx);
    setNodeMyLinkedList__(myLinkedList, node, newData);
}

void removeMyLinkedList_idx(MyLinkedList myLinkedList, int idx)
{
    removeMyLinkedList__(myLinkedList, getNodeMyLinkedList_idx__(myLinkedList, idx));
}

static Node getNodeMyLinkedList_cond__(MyLinkedList myLinkedList, int (*compareCond)(void*, void*),
    void* condition)
{
    Node node = myLinkedList->beginMarker;
    while ((node = node->next) != myLinkedList->endMarker
        && compareCond(node->data, condition) != 0)
        ;
    return node; // myLinkedList->endMarker 或 符合条件的node
}

void* getDataMyLinkedList_cond(MyLinkedList myLinkedList, int (*compareCond)(void*, void*), void* condition)
{
    // myLinkedList->endMarker->data(NULL) 或 符合条件的node->data
    return getNodeMyLinkedList_cond__(myLinkedList, compareCond, condition)->data;
}

void setMyLinkedList_cond(MyLinkedList myLinkedList, int (*compareCond)(void*, void*),
    void* condition, void* newData)
{
    Node node = getNodeMyLinkedList_cond__(myLinkedList, compareCond, condition);
    setNodeMyLinkedList__(myLinkedList, node, newData);
}

void removeMyLinkedList_cond(MyLinkedList myLinkedList, int (*compareCond)(void*, void*),
    void* condition)
{
    removeMyLinkedList__(myLinkedList, getNodeMyLinkedList_cond__(myLinkedList, compareCond, condition));
}

void traverseMyLinkedList(MyLinkedList myLinkedList, void (*operatorData)(void*, void*, void*, void*),
    void* arg1, void* arg2, void* arg3)
{
    Node node = myLinkedList->beginMarker;
    while ((node = node->next) != myLinkedList->endMarker)
        operatorData(node->data, arg1, arg2, arg3);
}

static void operator_compare__(void* data, Node* pnode, int (*data_cmp)(void*, void*), bool* isSame)
{
    *pnode = (*pnode)->next;
    if (data_cmp(data, (*pnode)->data) != 0)
        *isSame = false;
}

bool myLinkedList_equal(MyLinkedList myLinkedList0, MyLinkedList myLinkedList1,
    int (*data_cmp)(void*, void*))
{
    if (myLinkedList_size(myLinkedList0) != myLinkedList_size(myLinkedList1)) 
        return false;

    bool isSame = true;
    Node node = myLinkedList1->beginMarker;
    traverseMyLinkedList(myLinkedList0, (void (*)(void*, void*, void*, void*))operator_compare__,
        &node, data_cmp, &isSame);
    return isSame;
}