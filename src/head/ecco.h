#ifndef ECCO_H
#define ECCO_H

#include "base.h"
#include "mylinkedlist.h"
#include "sqlite3.h"

// 获取开局对象链表
MyLinkedList getEccoMyLinkedList_file(const char* eccoWebFileName);
MyLinkedList getEccoMyLinkedList_db(const char* dbName, const char* lib_tblName);

// 设置棋谱对象的开局名称
bool setECCO_cm(ChessManual cm, MyLinkedList eccoList);

// 获取棋谱对象链表
MyLinkedList getCmMyLinkedList_dir(const char* dirName, RecFormat fromfmt, MyLinkedList eccoList);
MyLinkedList getCmMyLinkedList_webfile(const char* cmWebFileName);
MyLinkedList getCmMyLinkedList_db(const char* dbName, const char* man_tblName);

// 存储开局初始化数据至数据库（返回对象个数）
int storeEccolib(const char* dbName, const char* lib_tblName, MyLinkedList eccoList);
// 存储文件棋谱至数据库
int storeChessManual(const char* dbName, const char* man_tblName, MyLinkedList cmList);

// 下载开局网页的纯净字符串
bool downEccoLibWeb(const char* eccoWebFileName);
// 下载棋谱网页的纯净字符串(根据id顺序)
bool downChessManualWeb(const char* cmWebFileName, int first, int last, int step);

#endif