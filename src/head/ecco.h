#ifndef ECCO_H
#define ECCO_H

#include "base.h"
#include "mylinkedlist.h"
#include "tools.h"

// 存储文件棋谱至数据库
void storeManual_db(sqlite3* db, const char* tblName,
    const char* dirName, RecFormat fromfmt, MyLinkedList regObj_MyLinkedList);

// 获取局面正则对象链表
MyLinkedList getRegObj_MyLinkedList(sqlite3* db, const char* tblName);

// 设置开局编号
bool setECCO_cm(ChessManual cm, MyLinkedList regObj_MyLinkedList);


void initEcco(char* dbName);
#endif