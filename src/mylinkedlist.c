#include "head/mylinkedlist.h"

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
    delNode__(myLinkedList->beginMarker, myLinkedList->delData);
    free(myLinkedList);
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
    node->prev = node->prev->next = newNode__(data, node->prev, node); // 见数据结构与算法分析(Java版) P59.
    myLinkedList->theSize++;
    myLinkedList->modCount++;
}

// 参数node：静态函数返回的符合索引或条件的node 或 myLinkedList->endMarker
static void removeMyLinkedList__(MyLinkedList myLinkedList, Node node)
{
    if (node == myLinkedList->endMarker)
        return;

    node->next->prev = node->prev;
    node->prev->next = node->next;
    eraseNode__(node, myLinkedList->delData);

    myLinkedList->theSize--;
    myLinkedList->modCount++;
}

// 返回：符合索引的node 或 myLinkedList->endMarker
static Node getNodeMyLinkedList_idx__(MyLinkedList myLinkedList, int idx)
{
    int size = myLinkedList_size(myLinkedList);
    if (idx < 0 && idx >= size)
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
    addBeforeMyLinkedList__(myLinkedList, myLinkedList->endMarker, data);
}

void* getDataMyLinkedList_idx(MyLinkedList myLinkedList, int idx)
{
    return getNodeMyLinkedList_idx__(myLinkedList, idx)->data;
}

void addMyLinkedList_idx(MyLinkedList myLinkedList, int idx, void* data)
{
    addBeforeMyLinkedList__(myLinkedList, getNodeMyLinkedList_idx__(myLinkedList, idx), data);
}

void removeMyLinkedList_idx(MyLinkedList myLinkedList, int idx)
{
    removeMyLinkedList__(myLinkedList, getNodeMyLinkedList_idx__(myLinkedList, idx));
}

static void setNodeMyLinkedList__(MyLinkedList myLinkedList, Node node, void* newData)
{
    if (node == myLinkedList->endMarker || node == myLinkedList->beginMarker)
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

// 返回：符合条件的node 或 myLinkedList->endMarker
static Node getNodeMyLinkedList_cond__(MyLinkedList myLinkedList, int (*compareCond)(void*, void*),
    void* condition)
{
    Node node = myLinkedList->beginMarker;
    while ((node = node->next) != myLinkedList->endMarker && compareCond(node->data, condition) != 0)
        ;
    return node;
}

void* getDataMyLinkedList_cond(MyLinkedList myLinkedList, int (*compareCond)(void*, void*), void* condition)
{    
    return getNodeMyLinkedList_cond__(myLinkedList, compareCond, condition)->data;
}

void removeMyLinkedList_cond(MyLinkedList myLinkedList, int (*compareCond)(void*, void*),
    void* condition)
{
    removeMyLinkedList__(myLinkedList, getNodeMyLinkedList_cond__(myLinkedList, compareCond, condition));
}

void setMyLinkedList_cond(MyLinkedList myLinkedList, int (*compareCond)(void*, void*),
    void* condition, void* newData)
{
    setNodeMyLinkedList__(myLinkedList, getNodeMyLinkedList_cond__(myLinkedList, compareCond, condition), newData);
}

int traverseMyLinkedList(MyLinkedList myLinkedList, void (*operatorData)(void*, void*, void*, void*),
    void* arg1, void* arg2, void* arg3)
{
    int result = 0;
    Node node = myLinkedList->beginMarker;
    while ((node = node->next) != myLinkedList->endMarker) {
        operatorData(node->data, arg1, arg2, arg3);
        ++result;
    }
    return result;
}

static Info newInfo__(const wchar_t* name, const wchar_t* value)
{
    Info info = malloc(sizeof(struct Info));
    info->name = NULL;
    info->value = NULL;
    if (name && value) {
        info->name = malloc((wcslen(name) + 1) * sizeof(wchar_t));
        info->value = malloc((wcslen(value) + 1) * sizeof(wchar_t));
        wcscpy(info->name, name);
        wcscpy(info->value, value);
    }
    return info;
}

static void delInfo__(Info info)
{
    free(info->name);
    free(info->value);
    free(info);
}

MyLinkedList newInfoMyLinkedList(void)
{
    return newMyLinkedList((void (*)(void*))delInfo__);
}

static int infoName_cmp__(Info info, const wchar_t* name)
{
    return wcscmp(info->name, name);
}

const wchar_t* getInfoValue_name(MyLinkedList infoMyLinkedList, const wchar_t* name)
{
    Info info = getDataMyLinkedList_cond(infoMyLinkedList, (int (*)(void*, void*))infoName_cmp__, (void*)name);
    return info ? info->value : L"";
}

void setInfoItem(MyLinkedList infoMyLinkedList, const wchar_t* name, const wchar_t* value)
{
    setMyLinkedList_cond(infoMyLinkedList, (int (*)(void*, void*))infoName_cmp__,
        (void*)name, newInfo__(name, value));
}

void delInfoItem(MyLinkedList infoMyLinkedList, const wchar_t* name)
{
    removeMyLinkedList_cond(infoMyLinkedList, (int (*)(void*, void*))infoName_cmp__, (void*)name);
}

static void printInfo__(Info info, FILE* fout, void* _0, void* _1)
{
    fwprintf(fout, L"\t%ls: %ls\n", info->name, info->value);
}

void printInfoMyLinkedList(FILE* fout, MyLinkedList infoMyLinkedList)
{
    traverseMyLinkedList(infoMyLinkedList, (void (*)(void*, void*, void*, void*))printInfo__,
        fout, NULL, NULL);
}