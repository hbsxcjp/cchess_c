#ifndef ECCO_H
#define ECCO_H

#include "base.h"
#include "mylinkedlist.h"
#include "tools.h"

// 获取局面对象链表
MyLinkedList getEccoMyLinkedList(sqlite3* db, const char* lib_tblName);

// 根据棋谱iccs着法字符串获取开局编号
const wchar_t* getEccoSN_iccsStr(MyLinkedList eccoMyLinkedList, wchar_t* iccsStr);

void getEccoName(wchar_t* ecco_name, sqlite3* db, const char* lib_tblName, const wchar_t* ecco_sn);

// 打印输出局面库链表
void printEccoMyLinkedList(FILE* fout, MyLinkedList eccoMyLinkedList);

// 存储开局初始化数据至数据库
void storeEccolib_db(sqlite3* db, const char* lib_tblName, const char* fileName);

void initEcco(char* dbName);
#endif