#include "head/linkedlist.h"

// 内部节点类
typedef struct Node* Node;
struct Node {
    void* data;

    Node prev;
    Node next;
};

struct LinkedList {
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

LinkedList newLinkedList(void (*delData)(void*))
{
    LinkedList linkedList = malloc(sizeof(struct LinkedList));
    linkedList->beginMarker = newNode__(NULL, NULL, NULL);
    linkedList->endMarker = newNode__(NULL, linkedList->beginMarker, NULL);
    linkedList->beginMarker->next = linkedList->endMarker;

    linkedList->theSize = 0;
    linkedList->modCount = 0;

    linkedList->delData = delData;
    return linkedList;
}

void delLinkedList(LinkedList linkedList)
{
    delNode__(linkedList->beginMarker, linkedList->delData);
    free(linkedList);
}

int linkedList_size(LinkedList linkedList)
{
    return linkedList->theSize;
}

bool linkedList_isempty(LinkedList linkedList)
{
    return linkedList_size(linkedList) == 0;
}

static void addBeforeLinkedList__(LinkedList linkedList, Node node, void* data)
{
    node->prev = node->prev->next = newNode__(data, node->prev, node); // 见数据结构与算法分析(Java版) P59.
    linkedList->theSize++;
    linkedList->modCount++;
}

// 参数node：静态函数返回的符合索引或条件的node 或 linkedList->endMarker
static void removeLinkedList__(LinkedList linkedList, Node node)
{
    if (node == linkedList->endMarker)
        return;

    node->next->prev = node->prev;
    node->prev->next = node->next;
    eraseNode__(node, linkedList->delData);

    linkedList->theSize--;
    linkedList->modCount++;
}

// 返回：符合索引的node 或 linkedList->endMarker
static Node getNodeLinkedList_idx__(LinkedList linkedList, int idx)
{
    int size = linkedList_size(linkedList);
    if (idx < 0 && idx >= size)
        return linkedList->endMarker;

    Node node = linkedList->beginMarker->next; // 起始索引idx==0
    if (idx < size / 2) {
        for (int i = 0; i < idx; ++i)
            node = node->next;
    } else {
        node = linkedList->endMarker;
        for (int i = size; i > idx; --i)
            node = node->prev;
    }

    return node;
}

void addLinkedList(LinkedList linkedList, void* data)
{
    addBeforeLinkedList__(linkedList, linkedList->endMarker, data);
}

void* getDataLinkedList_idx(LinkedList linkedList, int idx)
{
    return getNodeLinkedList_idx__(linkedList, idx)->data;
}

void addLinkedList_idx(LinkedList linkedList, int idx, void* data)
{
    addBeforeLinkedList__(linkedList, getNodeLinkedList_idx__(linkedList, idx), data);
}

void removeLinkedList_idx(LinkedList linkedList, int idx)
{
    removeLinkedList__(linkedList, getNodeLinkedList_idx__(linkedList, idx));
}

static void setNodeLinkedList__(LinkedList linkedList, Node node, void* newData)
{
    if (node == linkedList->endMarker || node == linkedList->beginMarker)
        addLinkedList(linkedList, newData);
    else {
        linkedList->delData(node->data);
        node->data = newData;
    }
}

void setLinkedList_idx(LinkedList linkedList, int idx, void* newData)
{
    setNodeLinkedList__(linkedList, getNodeLinkedList_idx__(linkedList, idx), newData);
}

// 返回：符合条件的node 或 linkedList->endMarker
static Node getNodeLinkedList_cond__(LinkedList linkedList, int (*compareCond)(void*, void*),
    void* condition)
{
    Node node = linkedList->beginMarker;
    while ((node = node->next) != linkedList->endMarker && compareCond(node->data, condition) != 0)
        ;
    return node;
}

void* getDataLinkedList_cond(LinkedList linkedList, int (*compareCond)(void*, void*), void* condition)
{    
    return getNodeLinkedList_cond__(linkedList, compareCond, condition)->data;
}

void removeLinkedList_cond(LinkedList linkedList, int (*compareCond)(void*, void*),
    void* condition)
{
    removeLinkedList__(linkedList, getNodeLinkedList_cond__(linkedList, compareCond, condition));
}

void setLinkedList_cond(LinkedList linkedList, int (*compareCond)(void*, void*),
    void* condition, void* newData)
{
    setNodeLinkedList__(linkedList, getNodeLinkedList_cond__(linkedList, compareCond, condition), newData);
}

int traverseLinkedList(LinkedList linkedList, void (*operatorData)(void*, void*, void*, void*),
    void* arg1, void* arg2, void* arg3)
{
    int result = 0;
    Node node = linkedList->beginMarker;
    while ((node = node->next) != linkedList->endMarker) {
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

LinkedList newInfoLinkedList(void)
{
    return newLinkedList((void (*)(void*))delInfo__);
}

static int infoName_cmp__(Info info, const wchar_t* name)
{
    return wcscmp(info->name, name);
}

const wchar_t* getInfoValue_name(LinkedList infoLinkedList, const wchar_t* name)
{
    Info info = getDataLinkedList_cond(infoLinkedList, (int (*)(void*, void*))infoName_cmp__, (void*)name);
    return info ? info->value : L"";
}

void setInfoItem(LinkedList infoLinkedList, const wchar_t* name, const wchar_t* value)
{
    setLinkedList_cond(infoLinkedList, (int (*)(void*, void*))infoName_cmp__,
        (void*)name, newInfo__(name, value));
}

void delInfoItem(LinkedList infoLinkedList, const wchar_t* name)
{
    removeLinkedList_cond(infoLinkedList, (int (*)(void*, void*))infoName_cmp__, (void*)name);
}

static void printInfo__(Info info, FILE* fout, void* _0, void* _1)
{
    fwprintf(fout, L"\t%ls: %ls\n", info->name, info->value);
}

void printInfoLinkedList(FILE* fout, LinkedList infoLinkedList)
{
    traverseLinkedList(infoLinkedList, (void (*)(void*, void*, void*, void*))printInfo__,
        fout, NULL, NULL);
}