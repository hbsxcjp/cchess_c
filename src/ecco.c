#include "head/ecco.h"
#include "head/tools.h"

// 获取查询记录结果的数量
static int callCount__(void* count, int argc, char** argv, char** azColName)
{
    *(int*)count = (argv[0] ? atoi(argv[0]) : 0);
    return 0;
}

static int getRecCount__(sqlite3* db, char* tblName, char* where)
{
    // 查找表
    char tempSql[WCHARSIZE], *zErrMsg = 0;
    int count = 0;
    sprintf(tempSql, "SELECT count(*) FROM %s %s;", tblName, where);
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
    size_t size = SUPERWIDEWCHARSIZE;
    wchar_t *wInsertValuesSql = malloc(size * sizeof(wchar_t)),
            *tempWstr,
            lineStr[WIDEWCHARSIZE],
            outWstring[SUPERWIDEWCHARSIZE] = { 0 },
            wtblName[WCHARSIZE], sn[5], name[WCHARSIZE], nums[WCHARSIZE], moveStr[WIDEWCHARSIZE];
    assert(wInsertValuesSql);
    wInsertValuesSql[0] = L'\x0';
    mbstowcs(wtblName, tblName, WCHARSIZE);
    int snlen, namelen, numslen, moveStrlen, ovector[30], endOffset = wcslen(fileWstring);

    const char* error;
    int errorffset = 0;
    void* snReg[] = {
        pcrewch_compile(L"([A-E])．(\\S+)\\((共[\\s\\S]+?局)\\)", 0, &error, &errorffset, NULL),
        // \\s不包含"　"(全角空格)
        pcrewch_compile(L"([A-E]\\d)．(空|\\S+(?=\\(共))(?:(?![\\s　]+[A-E]\\d．)\\n|\\((共[\\s\\S]+?局)\\)"
                        "([\\s\\S]*?)(?=\\s+[A-E]\\d{2}．))",
            0, &error, &errorffset, NULL),

        //pcrewch_compile(L"([A-E][\\d]{2})．(\\S+(?=\\n))(?:\\n?((?![A-E]\\d)[\\s\\S]*?(?=\\n+上|\\n+[A-E]\\d{1,2}．)))",
        //    0, &error, &errorffset, NULL),
        pcrewch_compile(L"([A-E]\\d{2})．(\\S+)\\s+"
                        "(?:(?![A-E]\\d)([\\s\\S]*?)\\s*(无|共[\\s\\S]+?局)[\\s\\S]*?(?=\\s+上|\\s+[A-E]\\d{0,2}．))?",
            0, &error, &errorffset, NULL)
    };
    // B2. C0. D1. D2. D3. D4. moveStr
    // C20 C30 C61 C72局面 =
    void* equalEccoReg = pcrewch_compile(L"\\s*([A-E]\\d{2})局面 =([\\s\\S]*?)(?=\\s+[A-E]\\d{2}．))",
        0, &error, &errorffset, NULL);

    for (int i = 0; i < sizeof(snReg) / sizeof(snReg[0]); ++i) {
        assert(snReg[i]);

        int firstOffset = 0;
        while (firstOffset < endOffset) {
            tempWstr = fileWstring + firstOffset;
            if (pcrewch_exec(snReg[i], NULL, tempWstr, endOffset - firstOffset, 0, 0, ovector, 30) <= 0)
                break;

            snlen = ovector[3] - ovector[2];
            assert(snlen < 4);
            wcsncpy(sn, tempWstr + ovector[2], snlen);
            sn[snlen] = L'\x0';

            namelen = ovector[5] - ovector[4];
            wcsncpy(name, tempWstr + ovector[4], namelen);
            name[namelen] = L'\x0';
            if (wcscmp(name, L"空") == 0) {
                name[0] = L'\x0';
                nums[0] = L'\x0';
                moveStr[0] = L'\x0';
            } else {
                numslen = ovector[i < 2 ? 7 : 9] - ovector[i < 2 ? 6 : 8];
                wcsncpy(nums, tempWstr + ovector[i < 2 ? 6 : 8], numslen);
                nums[numslen] = L'\x0';

                if (i > 0) {
                    moveStrlen = ovector[i < 2 ? 9 : 7] - ovector[i < 2 ? 8 : 6];
                    wcsncpy(moveStr, tempWstr + ovector[i < 2 ? 8 : 6], moveStrlen);
                }
                moveStr[i == 0 ? 0 : moveStrlen] = L'\x0';
            }

            swprintf(lineStr, WIDEWCHARSIZE, L"INSERT INTO %ls (SN, NAME, NUMS, MOVESTR) "
                                             "VALUES ('%ls', '%ls', '%ls', '%ls' );\n",
                wtblName, sn, name, nums, moveStr);
            if (i == 1 && wcslen(moveStr) > 0)
                wcscat(outWstring, lineStr);

            supper_wcscat(&wInsertValuesSql, &size, lineStr);
            firstOffset += ovector[1];
        }
        pcrewch_free(snReg[i]);
    }

    size = wcslen(wInsertValuesSql) * sizeof(wchar_t) + 1;
    *insertValuesSql = malloc(size);
    wcstombs(*insertValuesSql, wInsertValuesSql, size);
    //printf("wsize:%ld size:%ld\n", wcslen(wInsertValuesSql), strlen(*insertValuesSql));

    //* 输出字符串，检查用
    FILE* fout = fopen("chessManual/eccolib", "w");
    //fwprintf(fout, outWstring);
    fwprintf(fout, wInsertValuesSql);
    fclose(fout);
    //*/

    free(wInsertValuesSql);
    free(fileWstring);
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
                     "NUMS TEXT NOT NULL,"
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