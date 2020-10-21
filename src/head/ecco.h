#ifndef ECCO_H
#define ECCO_H

#include "base.h"
#include "tools.h"

typedef struct RegObj* RegObj;

void initEcco(char* dbName);

// 局面正则根对象（单链表根对象）
SingleList getRegObj_SingleList(sqlite3* db, const char* lib_tblName);

void delRegObj_SingleList(SingleList rootRegObj_slist);

// 取得开局编号
const wchar_t* getEccoSn(wchar_t* ecco_sn, SingleList rootRegObj_slist, ChessManual cm);

void storeManual(sqlite3* db, SingleList rootRegObj_slist, const char* dirName, RecFormat fromfmt);

#endif