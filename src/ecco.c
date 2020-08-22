#include "head/ecco.h"
#include "head/tools.h"

//* 输出字符串，检查用
static FILE* fout;

// 获取查询记录结果的数量
static int callCount__(void* count, int argc, char** argv, char** azColName)
{
    *(int*)count = (argv[0] ? atoi(argv[0]) : 0);
    return 0;
}

static int getRecCount__(sqlite3* db, char* tblName, char* where)
{
    // 查找表
    char sql[WCHARSIZE], *zErrMsg = 0;
    int count = 0;
    sprintf(sql, "SELECT count(*) FROM %s %s;", tblName, where);
    int rc = sqlite3_exec(db, sql, callCount__, &count, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "\nTable %s get records error: %s", tblName, zErrMsg);
        sqlite3_free(zErrMsg);
        return -1;
    }

    return count;
}

// 初始化表
static int createTable__(sqlite3* db, char* tblName, char* colNames)
{
    char sql[WCHARSIZE], *zErrMsg = 0;
    int rc;
    sprintf(sql, "WHERE type = 'table' AND name = '%s'", tblName);
    if (getRecCount__(db, "sqlite_master", sql) > 0) {
        // 删除表
        sprintf(sql, "DROP TABLE %s;", tblName);
        rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "\nTable %s deleted error: %s", tblName, zErrMsg);
            sqlite3_free(zErrMsg);
            return -1;
        } else
            fprintf(stdout, "\nTable %s deleted successfully.", tblName);
    }

    // 创建表
    sprintf(sql, "CREATE TABLE %s(%s);", tblName, colNames);
    rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "\nTable %s created error: %s", tblName, zErrMsg);
        sqlite3_free(zErrMsg);
        return -1;
    } else
        fprintf(stdout, "\nTable %s created successfully.", tblName);

    return 0;
}

static void operateTable__(sqlite3* db, char* tblName, char* operateSql)
{
    char* zErrMsg;
    int rc = sqlite3_exec(db, operateSql, NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "\nTable %s operate error: %s", tblName, zErrMsg);
        sqlite3_free(zErrMsg);
    } else
        fprintf(stdout, "\nTable %s have records: %d\n", tblName, getRecCount__(db, tblName, ""));
}

// 获取操作字符串
static void getOperateSql__(char** operateSql, char* tblName, wchar_t* optWsql,
    wchar_t* fileWstring, wchar_t* regWstr, int argc)
{
    size_t size = SUPERWIDEWCHARSIZE;
    wchar_t *wOperateSql = malloc(size * sizeof(wchar_t)),
            *tempWstr,
            lineStr[WIDEWCHARSIZE],
            wtblName[WCHARSIZE],
            subWstr[argc][WIDEWCHARSIZE];
    assert(wOperateSql);
    wOperateSql[0] = L'\x0';
    mbstowcs(wtblName, tblName, WCHARSIZE);

    const char* error;
    int errorffset = 0;
    void* reg = pcrewch_compile(regWstr, 0, &error, &errorffset, NULL);
    assert(reg);

    int ovector[30], firstOffset = 0, endOffset = wcslen(fileWstring);
    while (firstOffset < endOffset) {
        tempWstr = fileWstring + firstOffset;
        if (pcrewch_exec(reg, NULL, tempWstr, endOffset - firstOffset, 0, 0, ovector, 30) <= 0)
            break;

        for (int i = 0; i < argc; ++i)
            copySubStr(subWstr[i], tempWstr, ovector[2 * i + 2], ovector[2 * i + 3]);
        switch (argc) {
        case 1:
            swprintf(lineStr, WIDEWCHARSIZE, optWsql, wtblName, subWstr[0]);
            break;
        case 2:
            swprintf(lineStr, WIDEWCHARSIZE, optWsql, wtblName, subWstr[0], subWstr[1]);
            break;
        case 3:
            swprintf(lineStr, WIDEWCHARSIZE, optWsql, wtblName, subWstr[0], subWstr[1], subWstr[2]);
            break;
        case 4:
            swprintf(lineStr, WIDEWCHARSIZE, optWsql, wtblName, subWstr[0], subWstr[1], subWstr[2], subWstr[3]);
            break;
        default:
            lineStr[0] = L'\x0';
            break;
        }

        supper_wcscat(&wOperateSql, &size, lineStr);
        firstOffset += ovector[1];
    }

    size = wcslen(wOperateSql) * sizeof(wchar_t) + 1;
    *operateSql = malloc(size);
    wcstombs(*operateSql, wOperateSql, size);
    //printf("\nwsize:%ld size:%ld\n", wcslen(wOperateSql), strlen(*operateSql));

    fprintf(fout, *operateSql);
    free(wOperateSql);
}

static void getInsertValuesSql__(char** operateSql, char* tblName, wchar_t* fileWstring)
{
    // 提取插入记录的字符串
    size_t size = 6 * SUPERWIDEWCHARSIZE;
    wchar_t *wInsertValuesSql = malloc(size * sizeof(wchar_t)),
            *tempWstr,
            lineStr[WIDEWCHARSIZE],
            wtblName[WCHARSIZE], sn[10], name[WCHARSIZE], nums[WCHARSIZE], moveStr[WIDEWCHARSIZE];
    assert(wInsertValuesSql);
    wInsertValuesSql[0] = L'\x0';
    mbstowcs(wtblName, tblName, WCHARSIZE);
    int ovector[30], endOffset = wcslen(fileWstring);

    const char* error;
    int errorffset = 0;
    void* reg[] = {
        pcrewch_compile(L"([A-E])．(\\S+)\\((共[\\s\\S]+?局)\\)", 0, &error, &errorffset, NULL),
        // B2. C0. D1. D2. D3. D4. \\s不包含"　"(全角空格)
        pcrewch_compile(L"([A-E]\\d)．(空|\\S+(?=\\(共))(?:(?![\\s　]+[A-E]\\d．)\\n|\\((共[\\s\\S]+?局)\\)"
                        "([\\s\\S]*?)(?=\\s+[A-E]\\d{2}．))",
            0, &error, &errorffset, NULL),
        pcrewch_compile(L"([A-E]\\d{2})．(\\S+)\\s+"
                        "(?:(?![A-E]\\d)([\\s\\S]*?)\\s*(无|共[\\s\\S]+?局)[\\s\\S]*?(?=\\s+上|\\s+[A-E]\\d{0,2}．))?",
            0, &error, &errorffset, NULL),

        // C20 C30 C61 C72局面
        pcrewch_compile(L"([A-E]\\d)\\d局面 =([\\s\\S\n]*?(?=\\s*[A-E]\\d{2}．))?",
            0, &error, &errorffset, NULL),
    };

    for (int i = 0; i < sizeof(reg) / sizeof(reg[0]); ++i) {
        assert(reg[i]);

        int firstOffset = 0;
        while (firstOffset < endOffset) {
            tempWstr = fileWstring + firstOffset;
            if (pcrewch_exec(reg[i], NULL, tempWstr, endOffset - firstOffset, 0, 0, ovector, 30) <= 0)
                break;

            copySubStr(sn, tempWstr, ovector[2], ovector[3]);
            name[0] = L'\x0';
            nums[0] = L'\x0';
            moveStr[0] = L'\x0';
            copySubStr(i < 3 ? name : moveStr, tempWstr, ovector[4], ovector[5]);

            if (i < 3) {
                if (wcscmp(name, L"空") != 0) { // i=0,1,2
                    copySubStr(nums, tempWstr, ovector[i < 2 ? 6 : 8], ovector[i < 2 ? 7 : 9]);

                    if (i > 0) // i=1,2
                        copySubStr(moveStr, tempWstr, ovector[i < 2 ? 8 : 6], ovector[i < 2 ? 9 : 7]);
                    // 筛除 C20 C30局面字符串
                    int ovec[9];
                    if (i == 1 && pcrewch_exec(reg[3], NULL, moveStr, wcslen(moveStr), 0, 0, ovec, 9) > 0)
                        moveStr[0] = L'\x0';
                } else
                    name[0] = L'\x0';
                swprintf(lineStr, WIDEWCHARSIZE, L"INSERT INTO %ls (SN, NAME, NUMS, MOVESTR) "
                                                 "VALUES ('%ls', '%ls', '%ls', '%ls' );\n",
                    wtblName, sn, name, nums, moveStr);
            } else
                swprintf(lineStr, WIDEWCHARSIZE, L"UPDATE %ls SET MOVESTR = '%ls' WHERE SN = '%ls';\n",
                    wtblName, moveStr, sn);

            supper_wcscat(&wInsertValuesSql, &size, lineStr);
            firstOffset += ovector[1];
        }
        pcrewch_free(reg[i]);
    }

    size = wcslen(wInsertValuesSql) * sizeof(wchar_t) + 1;
    *operateSql = malloc(size);
    wcstombs(*operateSql, wInsertValuesSql, size);
    //printf("wsize:%ld size:%ld\n", wcslen(wInsertValuesSql), strlen(*operateSql));

    //fprintf(fout, *operateSql);
    free(wInsertValuesSql);
    free(fileWstring);
}

// 初始化一级编码名称
static void initLevelOneTable__(sqlite3* db, wchar_t* fileWstring)
{
    char tblName[] = "levelOne";
    char colNames[] = "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "SN TEXT NOT NULL,"
                      "NAME TEXT,"
                      "NUMS TEXT";
    if (createTable__(db, tblName, colNames) < 0)
        return;

    // 获取插入记录字符串，并插入
    char* operateSql = NULL;
    wchar_t optWsql[] = L"INSERT INTO %ls (SN, NAME, NUMS) "
                        "VALUES ('%ls', '%ls', '%ls');\n",
            regWstr[] = L"([A-E])．(\\S+)\\((共[\\s\\S]+?局)\\)";
    getOperateSql__(&operateSql, tblName, optWsql, fileWstring, regWstr, 3);
    operateTable__(db, tblName, operateSql);

    free(operateSql);
}

// 初始化二级编码名称
static void initLevelTwoTable__(sqlite3* db, wchar_t* fileWstring)
{
    char tblName[] = "levelTwo";
    char colNames[] = "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "SN TEXT NOT NULL,"
                      "NAME TEXT,"
                      "NUMS TEXT,"
                      "MOVESTR TEXT";
    if (createTable__(db, tblName, colNames) < 0)
        return;

    // 获取插入记录字符串，并插入
    char* operateSql = NULL;
    wchar_t optWsql[] = L"INSERT INTO %ls (SN, NAME, NUMS, MOVESTR) "
                        "VALUES ('%ls', '%ls', '%ls', '%ls');\n",
            regWstr[] = L"([A-E]\\d)．(空|\\S+(?=\\(共))(?:(?![\\s　]+[A-E]\\d．)\\n|\\((共[\\s\\S]+?局)\\)"
                        "([\\s\\S]*?)(?=\\s+[A-E]\\d{2}．))";
    getOperateSql__(&operateSql, tblName, optWsql, fileWstring, regWstr, 4);
    operateTable__(db, tblName, operateSql);

    free(operateSql);
}

// 初始化开局类型编码名称
static void initTable__(sqlite3* db, char* tblName, char* colNames, wchar_t* fileWstring)
{
    if (createTable__(db, tblName, colNames) < 0)
        return;

    // 获取插入记录字符串，并插入
    char *zErrMsg, *operateSql = NULL;
    getInsertValuesSql__(&operateSql, tblName, fileWstring);
    int rc = sqlite3_exec(db, operateSql, NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "\nInsert values error: %s", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        fprintf(stdout, "\nInserted values count: %d", getRecCount__(db, tblName, ""));
    }
    free(operateSql);
}

static bool isMatch__(const char* str, const wchar_t* regWstr)
{
    const char* error;
    int errorffset = 0, ovector[30], len = strlen(str) * sizeof(wchar_t) + 1;
    void* reg = pcrewch_compile(regWstr, 0, &error, &errorffset, NULL);
    wchar_t wstr[len];
    mbstowcs(wstr, str, len);
    return pcrewch_exec(reg, NULL, wstr, wcslen(wstr), 0, 0, ovector, 30) > 0;
}

// 处理查询记录
static int updateMoveStr__(void* count, int argc, char** argv, char** azColName)
{

    /*
    for (int i = 0; i < argc; ++i) {
        printf("%s\t", argv[i]);
    }
    printf("\n");
    //*/
    if (!isMatch__(argv[1], L"^[2-9a-z]."))
        return 0;

    //fprintf(fout, "%s\t%s\n", argv[0], argv[1]);
    ++*(int*)count;
    return 0;
}

static void updateStrLib__(sqlite3* db, char* tblName)
{
    char sql[WCHARSIZE], *zErrMsg = 0;
    int count = 0;
    sprintf(sql, "SELECT SN, MOVESTR FROM %s "
                 //"WHERE LENGTH(SN) = 2 AND LENGTH(MOVESTR) > 0;",
                 "WHERE LENGTH(SN) = 3;",
        tblName);
    int rc = sqlite3_exec(db, sql, updateMoveStr__, &count, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "\nUpdateStrLib__ error: %s", zErrMsg);
        sqlite3_free(zErrMsg);
    } else
        printf("\nUpdateStrLib__ record count:%s %d", tblName, count);
}

void eccoInit(char* dbName)
{
    // 读取开局库源文件内容
    FILE* fin = fopen("chessManual/eccolib_src", "r");
    fout = fopen("chessManual/eccolib", "w");

    sqlite3* db = NULL;
    int rc = sqlite3_open(dbName, &db);
    if (rc) {
        fprintf(stderr, "\nCan't open database: %s", sqlite3_errmsg(db));
    } else {
        wchar_t* fileWstring = getWString(fin);
        assert(fileWstring);

        /*
        char* tblName = "eccolib";
        char colNames[] = "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                          "SN TEXT NOT NULL,"
                          "NUMS TEXT,"
                          "NAME TEXT,"
                          "MOVESTR TEXT,"
                          "MOVEROWCOL TEXT";
        //*/
        initLevelOneTable__(db, fileWstring);
        initLevelTwoTable__(db, fileWstring);

        //initTable__(db, tblName, colNames, fileWstring);
        //updateStrLib__(db, tblName);
    }

    sqlite3_close(db);

    fclose(fin);
    fclose(fout);
}