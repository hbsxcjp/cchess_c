#include "head/operatesqlite3.h"
#include "head/base.h"
#include "head/tools.h"

// 初始化表
int sqlite3_createTable(sqlite3* db, const char* tblName, const char* colNames)
{
    char sql[WIDEWCHARSIZE];
    sprintf(sql, "CREATE TABLE %s(%s);", tblName, colNames);
    return sqlite3_exec_showErrMsg(db, sql);
}

bool sqlite3_existTable(sqlite3* db, const char* tblName)
{
    char sql[WIDEWCHARSIZE];
    sprintf(sql, "type = 'table' AND name = '%s'", tblName);
    int count = sqlite3_getRecCount(db, "sqlite_master", sql);
    return count > 0;
}

int sqlite3_deleteTable(sqlite3* db, const char* tblName)
{
    char sql[WIDEWCHARSIZE];
    sprintf(sql, "DROP TABLE %s;", tblName);
    return sqlite3_exec_showErrMsg(db, sql);
}

// 获取查询记录结果的数量
static int sqlite3_callCount__(void* count, int argc, char** argv, char** colNames)
{
    if (argv)
        *(int*)count = atoi(argv[0]);
    return 0;
}

int sqlite3_getRecCount(sqlite3* db, const char* tblName, char* condition)
{
    int count = 0;
    char sql[WIDEWCHARSIZE], *zErrMsg = 0;
    sprintf(sql, "SELECT count(*) FROM %s WHERE %s;", tblName, condition);
    int rc = sqlite3_exec(db, sql, sqlite3_callCount__, &count, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "\nTable %s get records error: %s", tblName, zErrMsg);
        sqlite3_free(zErrMsg);
        return -1;
    }

    return count;
}

// 获取查询记录结果
static int sqlite3_setValue__(void* destColValue, int argc, char** argv, char** colNames)
{
    strcpy((char*)destColValue, argv ? argv[0] : "");
    //printf("\n%s", argv[0]);
    return 0;
}

int sqlite3_getValue(char* destColValue, sqlite3* db, const char* tblName,
    char* destColName, char* srcColName, char* srcColValue)
{
    char sql[WIDEWCHARSIZE], *zErrMsg = 0;
    sprintf(sql, "SELECT %s FROM %s WHERE %s = '%s';", destColName, tblName, srcColName, srcColValue);

    //printf("\n%s", sql);
    int rc = sqlite3_exec(db, sql, sqlite3_setValue__, destColValue, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "\nTable %s get records error: %s", tblName, zErrMsg);
        sqlite3_free(zErrMsg);
        return -1;
    }

    return 0;
}

int sqlite3_exec_showErrMsg(sqlite3* db, const char* sql)
{
    char* zErrMsg = NULL;
    int rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "\nErrMsg: %s", zErrMsg);
        sqlite3_free(zErrMsg);
        return -1;
    }

    return 0;
}

static void wcscatColName__(Info info, wchar_t* wcolNames, const wchar_t* sufStr, void* _)
{
    wcscat(wcolNames, info->name);
    wcscat(wcolNames, sufStr);
}

static void wcscatColNames__(wchar_t* wcolNames, const wchar_t* sufStr, LinkedList infoLinkedList)
{
    traverseLinkedList(infoLinkedList, (void (*)(void*, void*, void*, void*))wcscatColName__,
        wcolNames, (void*)sufStr, NULL);
    if (wcslen(wcolNames) > 0)
        wcolNames[wcslen(wcolNames) - 1] = L'\x0'; //去掉后缀最后的分隔逗号
}

static void wcscatColValue__(Info info, wchar_t** pvalues, size_t* psize, void* _)
{
    supper_wcscat(pvalues, psize, L"\'");
    supper_wcscat(pvalues, psize, info->value);
    supper_wcscat(pvalues, psize, L"\' ,");
}

static void wcscatInsertLineStr__(wchar_t** pwInsertSql, size_t* psize, wchar_t* insertFormat,
    LinkedList infoLinkedList)
{
    size_t size = SUPERWIDEWCHARSIZE;
    wchar_t* values = malloc(size * sizeof(wchar_t));
    wcscpy(values, L"");
    traverseLinkedList(infoLinkedList, (void (*)(void*, void*, void*, void*))wcscatColValue__,
        &values, &size, NULL);
    values[wcslen(values) - 1] = L'\x0';

    size = wcslen(values) + wcslen(insertFormat) + 1;
    wchar_t lineStr[size];
    swprintf(lineStr, size, insertFormat, values);
    supper_wcscat(pwInsertSql, psize, lineStr);
    free(values);
}

// 创建表
static int checkTable__(sqlite3* db, const char* tblName, LinkedList infoLinkedList)
{
    if (!sqlite3_existTable(db, tblName)) {
        char colNames[WIDEWCHARSIZE];
        wchar_t wcolNames[WIDEWCHARSIZE];
        wcscpy(wcolNames, L"ID INTEGER PRIMARY KEY AUTOINCREMENT,");
        wcscatColNames__(wcolNames, L" TEXT,", infoLinkedList);
        wcstombs(colNames, wcolNames, WIDEWCHARSIZE);
        if (sqlite3_createTable(db, tblName, colNames) != SQLITE_OK)
            return -1;
    }

    return 0;
}

// 存储信息链表至数据库（name对应数据表列名，value对应该列的记录值）clearTable:是否清除原有记录
static void storeInfoLinkedList__(void* obj, sqlite3* db, wchar_t* wInsertFormat,
    LinkedList (*getInfoLinkedList)(void*))
{
    LinkedList infoLinkedList = getInfoLinkedList(obj);
    if (linkedList_size(infoLinkedList) < 1)
        return;

    // 获取存储写入数据库语句
    size_t size = SUPERWIDEWCHARSIZE;
    wchar_t* wInsertSql = malloc(size * sizeof(wchar_t));
    wcscpy(wInsertSql, L"");
    wcscatInsertLineStr__(&wInsertSql, &size, wInsertFormat, infoLinkedList);
    //fwprintf(fout, L"\n%ls\nwInsertSql len:%d\n\n", wInsertSql, wcslen(wInsertSql));

    // 存储写入数据库
    size = wcstombs(NULL, wInsertSql, 0) + 1;
    char insertSql[size];
    wcstombs(insertSql, wInsertSql, size);
    free(wInsertSql);

    sqlite3_exec_showErrMsg(db, insertSql);
}

int storeObjLinkedList(sqlite3* db, const char* tblName, LinkedList objLinkedList, LinkedList (*getInfoLinkedList)(void*))
{
    if (linkedList_size(objLinkedList) < 1)
        return 0;

    LinkedList infoLinkedList = getInfoLinkedList(getDataLinkedList_idx(objLinkedList, 0));
    if (infoLinkedList == NULL
        || linkedList_size(infoLinkedList) < 1
        || checkTable__(db, tblName, infoLinkedList) != 0)
        return 0;

    // 获取存储写入数据库语句
    wchar_t wtblName[WCHARSIZE], wcolNames[WIDEWCHARSIZE], wInsertFormat[WIDEWCHARSIZE];
    mbstowcs(wtblName, tblName, WCHARSIZE);
    wcscpy(wcolNames, L"");
    wcscatColNames__(wcolNames, L" ,", infoLinkedList);
    swprintf(wInsertFormat, WIDEWCHARSIZE, L"INSERT INTO %ls (%ls) VALUES (%%ls);\n", wtblName, wcolNames);

    return traverseLinkedList(objLinkedList, (void (*)(void*, void*, void*, void*))storeInfoLinkedList__,
        db, wInsertFormat, getInfoLinkedList);
}