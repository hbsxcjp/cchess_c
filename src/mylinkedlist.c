#include "head/mylinkedlist.h"

struct Info {
    wchar_t* name;
    wchar_t* value;
};

// 内部节点类
typedef struct Node* Node;
struct Node {
    void* data;

    Node prev;
    Node next;
};

struct MyLinkedList {
    Node beginMarker;
    Node endMarker;

    int theSize;
    int modCount;

    // 节点内数据对象内存释放函数
    void (*delData)(void*);
};

Info newInfo(const wchar_t* name, const wchar_t* value)
{
    Info info = malloc(sizeof(struct Info));
    info->name = NULL;
    info->value = NULL;
    if (name && value) {
        info->name = malloc((wcslen(name) + 1) * sizeof(wchar_t));
        wcscpy(info->name, name);
        info->value = malloc((wcslen(value) + 1) * sizeof(wchar_t));
        wcscpy(info->value, value);
    }
    return info;
}

void delInfo(Info info)
{
    free(info->name);
    free(info->value);
    free(info);
}

const wchar_t* getInfoName(Info info)
{
    return info->name;
}

const wchar_t* getInfoValue(Info info)
{
    return info->value;
}

static int infoName_cmp__(Info info, const wchar_t* name)
{
    return wcscmp(getInfoName(info), name);
}

const wchar_t* getInfoValue_name(MyLinkedList myLinkedList, const wchar_t* name)
{
    Info info = getDataMyLinkedList_cond(myLinkedList, (int (*)(void*, void*))infoName_cmp__, (void*)name);
    return info ? getInfoValue(info) : L"";
}

void setInfoItem(MyLinkedList myLinkedList, const wchar_t* name, const wchar_t* value)
{
    setMyLinkedList_cond(myLinkedList, (int (*)(void*, void*))infoName_cmp__,
        (void*)name, newInfo(name, value));
}

void delInfoItem(MyLinkedList myLinkedList, const wchar_t* name)
{
    removeMyLinkedList_cond(myLinkedList, (int (*)(void*, void*))infoName_cmp__, (void*)name);
}

void printInfo(Info info, FILE* fout, void* _0, void* _1)
{
    fwprintf(fout, L"\t%ls: %ls\n", getInfoName(info), getInfoValue(info));
}

static Node newNode__(void* data, Node prev, Node next)
{
    Node node = malloc(sizeof(struct Node));
    node->data = data;

    node->prev = prev;
    node->next = next;
    return node;
}

static void eraseNode__(Node node, void (*delData)(void*))
{
    if (node->data)
        delData(node->data);
    free(node);
}

static void delNode__(Node node, void (*delData)(void*))
{
    if (node == NULL)
        return;

    Node next = node->next;
    eraseNode__(node, delData);

    delNode__(next, delData);
}

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
    eraseNode__(node, myLinkedList->delData);

    myLinkedList->theSize--;
    myLinkedList->modCount++;
}

static Node getNodeMyLinkedList_idx__(MyLinkedList myLinkedList, int idx)
{
    int size = myLinkedList_size(myLinkedList);
    if (idx < 0 && idx > size)
        return myLinkedList->endMarker;

    Node node = myLinkedList->beginMarker->next; // 起始索引idx==0
    if (idx < size / 2) {
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

void* getEndDataMyLinkedList(MyLinkedList myLinkedList)
{
    return getDataMyLinkedList_idx(myLinkedList, myLinkedList_size(myLinkedList) - 1);
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
    setNodeMyLinkedList__(myLinkedList, getNodeMyLinkedList_idx__(myLinkedList, idx), newData);
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
    return node; // 返回：myLinkedList->endMarker 或 符合条件的node
}

void* getDataMyLinkedList_cond(MyLinkedList myLinkedList, int (*compareCond)(void*, void*), void* condition)
{
    // 返回：myLinkedList->endMarker->data(NULL) 或 符合条件的node->data
    return getNodeMyLinkedList_cond__(myLinkedList, compareCond, condition)->data;
}

void setMyLinkedList_cond(MyLinkedList myLinkedList, int (*compareCond)(void*, void*),
    void* condition, void* newData)
{
    setNodeMyLinkedList__(myLinkedList, getNodeMyLinkedList_cond__(myLinkedList, compareCond, condition), newData);
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
    if (*pnode == NULL)
        return;

    *pnode = (*pnode)->next;
    if (data_cmp(data, (*pnode)->data) != 0) {
        *isSame = false;
        *pnode = NULL;
    }
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