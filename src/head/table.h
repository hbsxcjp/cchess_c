#ifndef TABLE_H
#define TABLE_H

#include <stdbool.h>

typedef struct Table* Table;

// 创建一个哈希表（key为const char*, value为void*)
Table newTable(int hint, void delValue(void*));

// 释放一个表（但不释放表中的键或值）
void delTable(Table table);

// 返回表中桶的数目
int getTableSize(Table table);

// 返回表中键的数目
int getTableLength(Table table);

// 添加一个新的键值对,或改变一个现存键值对中的值
void putTable(Table* table, char* key, void* value);

// 取得与某个键关联的值
void* getTable(Table table, const char* key);

// 删除一个键值对，返回值指针
void removeTable(Table table, const char* key);

// 对每个键值对调用apply指向的函数
void mapTable(Table table, void apply(char* key, void* value, void* cl), void* cl);

// 对每个桶调用apply指向的函数
void mapTable_Buckets(Table table, void apply(int count, void* cl), void* cl);

#endif
