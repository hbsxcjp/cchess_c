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
typedef struct MyLinkedList* MyLinkedList;

//=================================================================
// 双链表类函数
//=================================================================

MyLinkedList newMyLinkedList(void (*delData)(void*));

void delMyLinkedList(MyLinkedList myLinkedList);

int myLinkedList_size(MyLinkedList myLinkedList);

bool myLinkedList_isempty(MyLinkedList myLinkedList);

void addMyLinkedList(MyLinkedList myLinkedList, void* data);

// 以索引号操作数据对象函数(返回：符合索引的data 或 myLinkedList->endMarker->data, 即NULL)
void* getDataMyLinkedList_idx(MyLinkedList myLinkedList, int idx);

void addMyLinkedList_idx(MyLinkedList myLinkedList, int idx, void* data);

void removeMyLinkedList_idx(MyLinkedList myLinkedList, int idx);

void setMyLinkedList_idx(MyLinkedList myLinkedList, int idx, void* newData);

// 以某个条件操作数据对象函数(返回：符合条件的data 或 myLinkedList->endMarker->data, 即NULL)
void* getDataMyLinkedList_cond(MyLinkedList myLinkedList, int (*compareCond)(void*, void*),
    void* condition);

void removeMyLinkedList_cond(MyLinkedList myLinkedList, int (*compareCond)(void*, void*),
    void* condition);

void setMyLinkedList_cond(MyLinkedList myLinkedList, int (*compareCond)(void*, void*),
    void* condition, void* newData);

// 遍历全部对象，执行指定操作（返回执行对象的个数）
int traverseMyLinkedList(MyLinkedList myLinkedList, void (*operatorData)(void*, void*, void*, void*),
    void* arg1, void* arg2, void* arg3);

//=================================================================
// 名值双链表类函数
//=================================================================

MyLinkedList newInfoMyLinkedList(void);

const wchar_t* getInfoValue_name(MyLinkedList infoMyLinkedList, const wchar_t* name);

void setInfoItem(MyLinkedList infoMyLinkedList, const wchar_t* name, const wchar_t* value);

void delInfoItem(MyLinkedList infoMyLinkedList, const wchar_t* name);

void printInfoMyLinkedList(FILE* fout, MyLinkedList infoMyLinkedList);
#endif