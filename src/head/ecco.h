#ifndef ECCO_H
#define ECCO_H

#include "base.h"
#include "mylinkedlist.h"
#include "tools.h"

// 获取局面正则对象链表
MyLinkedList getRegMyLinkedList(sqlite3* db, const char* lib_tblName);

// 设置开局编号
bool setECCO_cm(ChessManual cm, MyLinkedList regMyLinkedList);

// 打印输出局面库链表
void printEccoMyLinkedList(FILE* fout, MyLinkedList eccoMyLinkedList);

// 存储开局初始化数据至数据库
void storeEccolib_db(sqlite3* db, const char* lib_tblName, const char* fileName);

void initEcco(char* dbName);
#endif