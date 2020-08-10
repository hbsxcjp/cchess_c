#include "head/ecco.h"
#include "head/tools.h"

// 获取查询记录结果的数量
static int callCount__(void* count, int argc, char** argv, char** azColName)
{
    *(int*)count = (argv[0] ? atoi(argv[0]) : 0);
    return 0;
}

static int getRecCount__(sqlite3* db, char* tblName, char* whereSql)
{
    // 查找表
    char tempSql[WCHARSIZE], *zErrMsg = 0;
    int count = 0;
    sprintf(tempSql, "SELECT count(*) FROM %s %s;", tblName, whereSql);
    int rc = sqlite3_exec(db, tempSql, callCount__, &count, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Get RecCount error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return -1;
    }
    //printf("Table name: %s, record count: %d\n", tblName, count);

    return count;
}

static void getInsertValuesSql__(char** insertValuesSql, char* tblName, char* fileName)
{
    // 读取开局库源文件内容
    FILE* fin = fopen(fileName, "r");
    wchar_t* fileWstring = getWString(fin);
    fclose(fin);
    if (fileWstring == NULL)
        return;

    // 提取插入记录的字符串
    const char* error;
    int erroffset = 0;
    void* snReg[] = {
        pcrewch_compile(L"([A-E])．(\\S+)(?=\\(共)(\\S+(?=\\n))", 0, &error, &erroffset, NULL),
        //pcrewch_compile(L"([A-E]\\d)．(\\S+)(?=[\\n　]*[A-E]\\d．)", 0, &error, &erroffset, NULL),
        pcrewch_compile(L"([A-E]\\d)．(\\S+)(?=\\n|\\(共)([\\s\\S]+?(?=[A-E]\\d{2}．))", 0, &error, &erroffset, NULL),

        //pcrewch_compile(L"([A-E]\\d{2})．(\\S+)(?=\\n)", 0, &error, &erroffset, NULL),
        //pcrewch_compile(L"([A-E][\\d]{2})．([\\S]+)\n([\\s\\S]+?(?=[A-Z][\\d]{2}．))?", 0, &error, &erroffset, NULL),
        pcrewch_compile(L"([A-E]\\d{2})．(\\S+)\\n([\\s\\S]*?(?=[A-E]\\d+．))?", 0, &error, &erroffset, NULL),
    };

    size_t size = SUPERWIDEWCHARSIZE;
    wchar_t *wInsertValuesSql = malloc(size * sizeof(wchar_t)),
            //snStr[WIDEWCHARSIZE] = { 0 },
            lineStr[WIDEWCHARSIZE] = { 0 },
            *tempWstr,
            wtblName[WCHARSIZE], sn[5], name[WCHARSIZE], moveStr[WIDEWCHARSIZE];
    assert(wInsertValuesSql);
    wInsertValuesSql[0] = L'\x0';
    mbstowcs(wtblName, tblName, WCHARSIZE);
    int snlen, namelen, moveStrlen, ovector[30], endOffset = wcslen(fileWstring);
    for (int i = 0; i < sizeof(snReg) / sizeof(snReg[0]); ++i) {
        assert(snReg[i]);
        printf("%d: %d\n", __LINE__, i);

        int firstOffset = 0;
        while (firstOffset < endOffset) {
            tempWstr = fileWstring + firstOffset;
            if (pcrewch_exec(snReg[i], NULL, tempWstr, endOffset - firstOffset, 0, 0, ovector, 30) <= 0)
                break;

            snlen = ovector[3] - ovector[2];
            assert(snlen < 4);
            wcsncpy(sn, tempWstr + ovector[2], snlen);
            sn[snlen] = L'\x0';

            // 如果sn还没有出现
            //if (wcsstr(snStr, sn) == NULL) {
            //    wcscat(snStr, sn);

                namelen = ovector[5] - ovector[4];
                wcsncpy(name, tempWstr + ovector[4], namelen);
                name[namelen] = L'\x0';
                if (wcscmp(name, L"空") == 0)
                    name[0] = L'\x0';

                //if (i <2) {
                    moveStrlen = ovector[7] - ovector[6];
                    wcsncpy(moveStr, tempWstr + ovector[6], moveStrlen);
                    moveStr[moveStrlen] = L'\x0';
                //} else
                //    moveStr[0] = L'\x0';

                swprintf(lineStr, WIDEWCHARSIZE, L"INSERT INTO %ls (SN, NAME, MOVESTR) "
                                                 "VALUES ('%ls', '%ls', '%ls' );\n",
                    wtblName, sn, name, moveStr);
                supper_wcscat(&wInsertValuesSql, &size, lineStr);
            //}
            firstOffset += ovector[1];
        }
        printf("%d: %d\n", __LINE__, i);
        pcrewch_free(snReg[i]);
    }

    size = wcslen(wInsertValuesSql) * sizeof(wchar_t) + 1;
    *insertValuesSql = malloc(size);
    wcstombs(*insertValuesSql, wInsertValuesSql, size);
    //printf("wsize:%ld size:%ld\n%s\n", wcslen(wInsertValuesSql), strlen(*insertValuesSql), *insertValuesSql);
    printf("wsize:%ld size:%ld\n", wcslen(wInsertValuesSql), strlen(*insertValuesSql));

    free(wInsertValuesSql);
    free(fileWstring);

    //* 输出字符串，检查用
    FILE* fout = fopen("chessManual/eccolib", "w");
    fprintf(fout, *insertValuesSql);
    fclose(fout);
    //*/
}

// 初始化开局类型编码名称
static void initLib__(sqlite3* db)
{
    char *tblName = "eccolib", tempSql[WCHARSIZE], *zErrMsg = 0;
    int rc;
    sprintf(tempSql, "WHERE type = 'table' AND name = '%s'", tblName);
    if (getRecCount__(db, "sqlite_master", tempSql) > 0) {
        // 查找表记录
        //if (getRecCount__(db, tblName, "") == 555)
        //    return;

        // 删除表
        sprintf(tempSql, "DROP TABLE %s;", tblName);
        rc = sqlite3_exec(db, tempSql, NULL, NULL, &zErrMsg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Delete table error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
            return;
        } else
            fprintf(stdout, "Delete table successfully.\n");
    }

    // 创建表
    sprintf(tempSql, "CREATE TABLE %s("
                     "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                     "SN TEXT NOT NULL,"
                     "NAME TEXT NOT NULL,"
                     "MOVESTR TEXT);",
        tblName);
    rc = sqlite3_exec(db, tempSql, NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Create table error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return;
    } else
        fprintf(stdout, "Table created successfully.\n");

    // 获取插入记录字符串，并插入
    char* insertValuesSql = NULL;
    getInsertValuesSql__(&insertValuesSql, tblName, "chessManual/eccolib_src");
    rc = sqlite3_exec(db, insertValuesSql, NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Insert values error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        fprintf(stdout, "Values inserted successfully.\n");

        fprintf(stdout, "Inserted values count: %d\n", getRecCount__(db, tblName, ""));
    }
    free(insertValuesSql);
}

void eccoInit(char* dbName)
{
    sqlite3* db = NULL;
    int rc = sqlite3_open(dbName, &db);
    if (rc) {
        fprintf(stderr, "\nCan't open database: %s\n", sqlite3_errmsg(db));
    } else {
        //fprintf(stderr, "\nOpened database successfully. \n");

        initLib__(db);
    }

    sqlite3_close(db);
}