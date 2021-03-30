/* 名值类与双链表类 */
#ifndef MYLINKEDLIST_H
#define MYLINKEDLIST_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

// 名值类
typedef struct Info* Info;
struct Info {
    wchar_t* name;
    wchar_t* value;
};

// 双链表类
typedef struct LinkedList* LinkedList;

//=================================================================
// 双链表类函数
//=================================================================

LinkedList newLinkedList(void (*delData)(void*));

void delLinkedList(LinkedList linkedList);

int linkedList_size(LinkedList linkedList);

bool linkedList_isempty(LinkedList linkedList);

void addLinkedList(LinkedList linkedList, void* data);

// 以索引号操作数据对象函数(返回：符合索引的data 或 linkedList->endMarker->data, 即NULL)
void* getDataLinkedList_idx(LinkedList linkedList, int idx);

void addLinkedList_idx(LinkedList linkedList, int idx, void* data);

void removeLinkedList_idx(LinkedList linkedList, int idx);

void setLinkedList_idx(LinkedList linkedList, int idx, void* newData);

// 以某个条件操作数据对象函数(返回：符合条件的data 或 linkedList->endMarker->data, 即NULL)
void* getDataLinkedList_cond(LinkedList linkedList, int (*compareCond)(void*, void*),
    void* condition);

void removeLinkedList_cond(LinkedList linkedList, int (*compareCond)(void*, void*),
    void* condition);

void setLinkedList_cond(LinkedList linkedList, int (*compareCond)(void*, void*),
    void* condition, void* newData);

// 遍历全部对象，执行指定操作（返回执行对象的个数）
int traverseLinkedList(LinkedList linkedList, void (*operatorData)(void*, void*, void*, void*),
    void* arg1, void* arg2, void* arg3);

//=================================================================
// 名值双链表类函数
//=================================================================

LinkedList newInfoLinkedList(void);

const wchar_t* getInfoValue_name(LinkedList infoLinkedList, const wchar_t* name);

void setInfoItem(LinkedList infoLinkedList, const wchar_t* name, const wchar_t* value);

void delInfoItem(LinkedList infoLinkedList, const wchar_t* name);

void printInfoLinkedList(FILE* fout, LinkedList infoLinkedList);
#endif