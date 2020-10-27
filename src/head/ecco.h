#ifndef ECCO_H
#define ECCO_H

#include "base.h"
#include "mylinkedlist.h"
#include "tools.h"

// 存储文件棋谱至数据库
void storeManual_db(sqlite3* db, const char* tblName,
    const char* dirName, RecFormat fromfmt, MyLinkedList regObjMyLinkedList);

// 获取局面正则对象链表
MyLinkedList getRegObjMyLinkedList(sqlite3* db, const char* tblName);

// 设置开局编号
bool setECCO_cm(ChessManual cm, MyLinkedList regObjMyLinkedList);


void initEcco(char* dbName);
#endif