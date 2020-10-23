#ifndef ECCO_H
#define ECCO_H

#include "base.h"
#include "tools.h"

typedef struct RegObj* RegObj;

// 局面正则根对象（单链表根对象）
LinkedItem getRootRegObj_LinkedItem(sqlite3* db, const char* lib_tblName);

void delRootRegObj_LinkedItem(LinkedItem rootRegObj_item);

// 取得开局编号
const wchar_t* getEcco_sn(RegObj regObj, const wchar_t* iccsStr);

void storeManual(sqlite3* db, const char* man_tblName, const char** col_names, int col_len,
    const char* dirName, RecFormat fromfmt, LinkedItem rootRegObj_item);

void initEcco(char* dbName);
#endif