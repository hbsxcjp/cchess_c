#include "head/table.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define InitTableSize 1021
#define TableLoadFactor 0.85

static int getPrime(int size)
{
    //static int primes[] = { 509, 509, 1021, 2053, 4093, 8191, 16381, 32771, 65521, INT_MAX };
    //3, 7, 13,
    int primes[] = { 31, 61, 127, 251, 509, 1021, 2039, 4093, 8191, 16381, 32749, 65521,
        131071, 262139, 524287, 1048573, 2097143, 4194301, 8388593, 16777213, 33554393,
        67108859, 134217689, 268435399, 536870909, 1073741789, INT32_MAX };
    int i = 0;
    while (primes[i] < size)
        i++;
    return primes[i];
}

static unsigned int BKDRHash(const char* src)
{
    // 31 131 1313 13131 131313 etc..
    unsigned int seed = 131;
    unsigned int hash = 0;

    while (*src)
        hash = hash * seed + (*src++);

    return (hash & 0x7FFFFFFF);
}

typedef struct binding* Binding;
struct binding {
    char* key;
    void* value;

    Binding link;
};

struct Table {
    size_t size;
    size_t length;
    void (*delValue)(void*);

    Binding* buckets;
};

// 创建一个哈希表
Table newTable(int hint, void delValue(void*))
{
    int size = getPrime(hint);
    Table table = malloc(sizeof(*table));
    table->size = size;
    table->length = 0;
    table->delValue = delValue;
    table->buckets = malloc(size * sizeof(table->buckets[0])); // 只存链表首指针，不存储内部结构字段
    for (int i = 0; i < size; ++i)
        table->buckets[i] = NULL;

    return table;
}

// 释放桶外的一个binding结构
static void cutBinding__(Table table, Binding p)
{
    free(p->key);
    table->delValue(p->value);
    free(p);
}

// 释放桶外的一个binding结构
static void delBinding__(Table table, Binding p)
{
    while (p) {
        Binding q = p->link;
        cutBinding__(table, p);
        p = q;
    }
}

// 释放一个表
void delTable(Table table)
{
    assert(table);
    for (size_t i = 0; i < table->size; i++)
        delBinding__(table, table->buckets[i]);
    free(table->buckets);
    free(table);
}

int getTableSize(Table table)
{
    assert(table);
    return table->size;
}

int getTableLength(Table table)
{
    assert(table);
    return table->length;
}

// 取得某个键的桶首指针
inline static Binding* getHeadBinding__(Table table, const char* key)
{
    return table->buckets + (BKDRHash(key) % table->size);
}

// 取得某个桶中与某个键相等的内部结构指针
static Binding getBinding__(Binding* pp, const char* key)
{
    Binding p = *pp;
    while (p && strcmp(key, p->key) != 0)
        p = p->link;
    return p; // 空指针 或 目标对象指针
}

static void insertBinding__(char* key, void* value, Binding* pp)
{
    Binding p = malloc(sizeof(*p));
    p->key = key;
    p->value = value;
    p->link = *pp;
    *pp = p;
}

static void loadBinding__(char* key, void* value, Table table)
{
    Binding* pp = getHeadBinding__(table, key);
    insertBinding__(key, value, pp);
}

// 源表内容迁移至新表
static void reloadTable__(Table* ptable)
{
    Table oldTable = *ptable,
          nTable = newTable(oldTable->size + 1, oldTable->delValue);
    mapTable(oldTable, (void (*)(char*, void*, void*))loadBinding__, nTable);
    nTable->length = oldTable->length;

    for (size_t i = 0; i < oldTable->size; i++) {
        Binding p = oldTable->buckets[i];
        while (p) {
            Binding q = p->link;
            free(p); // 不释放原有的binding结构内字段(key, value)
            p = q;
        }
    }
    free(oldTable->buckets);
    free(oldTable);

    *ptable = nTable;
}

void putTable(Table* ptable, char* key, void* value)
{
    assert(ptable && *ptable);
    assert(key);
    Table table = *ptable;
    Binding *pp = getHeadBinding__(table, key),
            p = getBinding__(pp, key);
    if (p == NULL) {
        // 检查容量，如果超出装载因子则扩容
        if (table->length >= table->size * TableLoadFactor
            && table->size < INT32_MAX) {
            reloadTable__(ptable);
            table = *ptable;
            pp = getHeadBinding__(table, key);
        }

        insertBinding__(key, value, pp);
        table->length++;
    } else {
        table->delValue(p->value);
        p->value = value;
    }
}

void* getTable(Table table, const char* key)
{
    assert(table);
    assert(key);
    Binding *pp = getHeadBinding__(table, key),
            p = getBinding__(pp, key);
    return p ? p->value : NULL;
}

void removeTable(Table table, const char* key)
{
    assert(table);
    assert(key);
    //table->timestamp++;
    Binding* pp = getHeadBinding__(table, key);
    for (Binding p = *pp; p != NULL; pp = &p, p = p->link)
        if (strcmp(key, p->key) == 0) {
            *pp = p->link;
            cutBinding__(table, p);
            table->length--;
            return;
        }
}

// 对每个键值对调用apply指向的函数
void mapTable(Table table, void apply(char* key, void* value, void* cl), void* cl)
{
    assert(table);
    assert(apply);
    // unsigned int stamp = table->timestamp; // 运行apply之前的时间戳
    for (size_t i = 0; i < table->size; i++) {
        for (Binding p = table->buckets[i]; p != NULL; p = p->link) {
            apply(p->key, p->value, cl);
            // assert(table->timestamp == stamp); // 检查apply是否修改了table（使用put或remove）
        }
    }
}