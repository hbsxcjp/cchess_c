#ifndef OPERATESQLITE3_H
#define OPERATESQLITE3_H

#include "mylinkedlist.h"
#include "sqlite3.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

int sqlite3_createTable(sqlite3* db, const char* tblName, const char* colNames);

bool sqlite3_existTable(sqlite3* db, const char* tblName);

int sqlite3_deleteTable(sqlite3* db, const char* tblName);

int sqlite3_getRecCount(sqlite3* db, const char* tblName, char* where);

int sqlite3_getValue(char* destColValue, sqlite3* db, const char* tblName,
    char* destColName, char* srcColName, char* srcColValue);

int sqlite3_exec_showErrMsg(sqlite3* db, const char* sql);

// 存储对象链表至数据库（返回对象个数）
int storeObjMyLinkedList(sqlite3* db, const char* tblName, MyLinkedList objMyLinkedList,
    MyLinkedList (*getInfoMyLinkedList)(void*));
    
#endif