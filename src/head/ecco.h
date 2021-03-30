#ifndef ECCO_H
#define ECCO_H

#include "base.h"
#include "linkedlist.h"
#include "sqlite3.h"

typedef struct Ecco* Ecco;

LinkedList getInfoLinkedList_ecco(Ecco ecco);

// 下载开局网页的纯净字符串
bool downEccoLibWeb(const char* eccoWebFileName);
// 下载棋谱网页的纯净字符串(根据id顺序)
bool downChessManualWeb(const char* cmWebFileName, int first, int last, int step);

// 获取开局对象链表
LinkedList getEccoLinkedList_file(const char* eccoWebFileName);
LinkedList getEccoLinkedList_db(const char* dbName, const char* lib_tblName);

// 设置棋谱对象的开局名称
bool setECCO_cm(ChessManual cm, LinkedList eccoList);

// 获取棋谱对象链表
LinkedList getCmLinkedList_dir(const char* dirName, RecFormat fromfmt, LinkedList eccoList);
LinkedList getCmLinkedList_webfile(const char* cmWebFileName);
LinkedList getCmLinkedList_db(const char* dbName, const char* man_tblName);

// 存储对象链表的info链表数据至数据库（返回对象个数）
int storeLinkedList(const char* dbName, const char* tblName, LinkedList linkedList,
    LinkedList (*getInfoLinkedList)(void*), bool delOldTbl);
    
#endif