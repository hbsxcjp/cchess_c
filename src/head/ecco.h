#ifndef ECCO_H
#define ECCO_H

#include "base.h"

typedef struct RegObj* RegObj;

void initEcco(char* dbName);

// 取得正则对象（单链表根对象）
RegObj getRootRegObj(sqlite3* db, const char* lib_tblName);

// 取得开局编号
const wchar_t* getEccoSn(wchar_t* ecco_sn, RegObj rootRegObj, ChessManual cm);

void storeManual(sqlite3* db, RegObj rootRegObj, const char* dirName, RecFormat fromfmt);

#endif