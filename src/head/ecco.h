#ifndef ECCO_H
#define ECCO_H

#include "base.h"
#include "tools.h"

typedef struct RegObj* RegObj;

// 局面正则根对象（单链表根对象）
LinkedItem getRootRegObj_LinkedItem(sqlite3* db, const char* lib_tblName);

void delRootRegObj_LinkedItem(LinkedItem rootRegObj_item);

// 取得开局编号
const wchar_t* getEccoSn(wchar_t* ecco_sn, LinkedItem rootRegObj_item, ChessManual cm);

void storeManual(sqlite3* db, LinkedItem rootRegObj_item, const char* dirName, RecFormat fromfmt);

void initEcco(char* dbName);
#endif