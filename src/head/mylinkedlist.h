#ifndef MYLINKEDLIST_H
#define MYLINKEDLIST_H

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

// 链表类
typedef struct MyLinkedList* MyLinkedList;

MyLinkedList newMyLinkedList(void (*delData)(void*));

void delMyLinkedList(MyLinkedList myLinkedList);

void clearMyLinkedList(MyLinkedList myLinkedList);

int myLinkedList_size(MyLinkedList myLinkedList);

bool myLinkedList_isempty(MyLinkedList myLinkedList);

void addMyLinkedList(MyLinkedList myLinkedList, void* data);

// 以索引号操作数据对象函数
void addMyLinkedList_idx(MyLinkedList myLinkedList, int idx, void* data);

void* getDataMyLinkedList_idx(MyLinkedList myLinkedList, int idx);

void setMyLinkedList_idx(MyLinkedList myLinkedList, int idx, void* newData);

void removeMyLinkedList_idx(MyLinkedList myLinkedList, int idx);

// 以某个条件操作数据对象函数
void* getDataMyLinkedList_cond(MyLinkedList myLinkedList, int (*compareCond)(void*, void*),
    void* condition);

void setMyLinkedList_cond(MyLinkedList myLinkedList, int (*compareCond)(void*, void*),
    void* condition, void* newData);

void removeMyLinkedList_cond(MyLinkedList myLinkedList, int (*compareCond)(void*, void*),
    void* condition);

// 遍历全部对象，执行指定操作
void traverseMyLinkedList(MyLinkedList myLinkedList, void (*operatorData)(void*, void*, void*, void*),
    void* arg1, void* arg2, void* arg3);

// 比较两个链表对象是否相等
bool myLinkedList_equal(MyLinkedList myLinkedList0, MyLinkedList myLinkedList1,
    int (*data_cmp)(void*, void*));
#endif