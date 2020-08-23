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

/*// 获取单一查询记录(字符串)
static int getFieldStr__(void* str, int argc, char** argv, char** azColName)
{
    strcpy(str, argv[0]);
    return 0;
}
//*/

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

// 提取插入记录的字符串
static void getEccoSql__(char** initEccoSql, char* tblName, wchar_t* fileWstring)
{
    const char* error;
    int errorffset, ovector[30],
        first, last = wcslen(fileWstring), index = 0,
               num0 = 555, num1 = 4, num = num0 + num1;
    wchar_t *tempWstr, sn[num][10], name[num][WCHARSIZE], nums[num][WCHARSIZE], moveStr[num][WIDEWCHARSIZE];
    void* regs[] = {
        pcrewch_compile(L"([A-E])．(\\S+)\\((共[\\s\\S]+?局)\\)", 0, &error, &errorffset, NULL),
        // B2. C0. D1. D2. D3. D4. \\s不包含"　"(全角空格)
        pcrewch_compile(L"([A-E]\\d)．(空|\\S+(?=\\(共))(?:(?![\\s　]+[A-E]\\d．)\\n|\\((共[\\s\\S]+?局)\\)"
                        "[\\s　]*([\\s\\S]*?)(?=[\\s　]*[A-E]\\d{2}．))",
            0, &error, &errorffset, NULL),
        pcrewch_compile(L"([A-E]\\d{2})．(\\S+)[\\s　]+"
                        "(?:(?![A-E]\\d)([\\s\\S]*?)[\\s　]*(无|共[\\s\\S]+?局)[\\s\\S]*?(?=\\s+上|\\s+[A-E]\\d{0,2}．))?",
            0, &error, &errorffset, NULL),

        // C20 C30 C61 C72局面
        pcrewch_compile(L"([A-E]\\d)\\d局面 =([\\s\\S\n]*?)(?=[\\s　]*[A-E]\\d{2}．)",
            0, &error, &errorffset, NULL),
    };
    for (int i = 0; i < sizeof(regs) / sizeof(regs[0]); ++i) {
        assert(regs[i]);

        first = 0;
        while (first < last) {
            tempWstr = fileWstring + first;
            if (pcrewch_exec(regs[i], NULL, tempWstr, last - first, 0, 0, ovector, 30) <= 0)
                break;

            assert(index < num);
            copySubStr(sn[index], tempWstr, ovector[2], ovector[3]);
            name[index][0] = L'\x0';
            nums[index][0] = L'\x0';
            moveStr[index][0] = L'\x0';
            copySubStr(i < 3 ? name[index] : moveStr[index], tempWstr, ovector[4], ovector[5]);

            if (i < 3) {
                if (wcscmp(name[index], L"空") != 0) { // i=0,1,2
                    copySubStr(nums[index], tempWstr, ovector[i < 2 ? 6 : 8], ovector[i < 2 ? 7 : 9]);

                    if (i > 0) // i=1,2
                        copySubStr(moveStr[index], tempWstr, ovector[i < 2 ? 8 : 6], ovector[i < 2 ? 9 : 7]);
                } else
                    name[index][0] = L'\x0';
            }
            first += ovector[1];
            index++;
        }
        pcrewch_free(regs[i]);
    }
    assert(index == num);

    // 修正C20 C30 C61 C72局面
    for (int i = num0; i < num; ++i) // 555-558
        for (int j = 0; j < num0; ++j)
            if (wcscmp(sn[j], sn[i]) == 0) {
                wcscpy(moveStr[j], moveStr[i]);
                break;
            }

    // 修正moveStr
    int No = 0;
    void *reg0 = pcrewch_compile(L"^[2-9a-z].", 0, &error, &errorffset, NULL),
         *reg1 = pcrewch_compile(L"([2-9a-z]. [\\n\\S]+(?:[，　]+[\\n\\S]*?(?=[2-9a-z].))?)", 0, &error, &errorffset, NULL);
    // ([2-9a-z]. )([^，\s]+)(，[\n\S]+)?

    wchar_t tsn[10], fmoveStr[WIDEWCHARSIZE], move[WCHARSIZE]; //tmoveStr[WIDEWCHARSIZE],
    for (int i = 55; i < num0; ++i) { // wcslen(sn[i]) == 3  有74项
        if (pcrewch_exec(reg0, NULL, moveStr[i], wcslen(moveStr[i]), 0, 0, ovector, 30) <= 0)
            continue;

        // 取得替换模板字符串
        wcscpy(tsn, sn[i]);
        if (tsn[0] == L'C')
            tsn[1] = L'0'; // C0-C9 => C0
        tsn[2] = L'\x0';
        if (wcscmp(tsn, L"D5") == 0)
            wcscpy(tsn, L"D51"); // D5_ => D51
        for (int j = 0; j < num0; ++j)
            if (wcscmp(sn[j], tsn) == 0) {
                wcscpy(fmoveStr, moveStr[j]);
                break;
            }

        fwprintf(fout, L"No:%d\n%ls\t%ls\n\n", No++, sn[i], moveStr[i]);
        first = 0;
        last = wcslen(moveStr[i]);
        while (first < last) {
            tempWstr = moveStr[i] + first;
            if (pcrewch_exec(reg1, NULL, tempWstr, last - first, 0, 0, ovector, 30) <= 0)
                break;

            copySubStr(move, tempWstr, ovector[2], ovector[3]);

            //fwprintf(fout, L"%ls\n", move);
            first += ovector[1];
        }
        fwprintf(fout, L"\nfmoveStr:%ls\n", fmoveStr);
    }
    pcrewch_free(reg0);
    pcrewch_free(reg1);

    size_t size = SUPERWIDEWCHARSIZE;
    wchar_t wtblName[WCHARSIZE], lineStr[WIDEWCHARSIZE],
        *wInitEccoSql = malloc(size * sizeof(wchar_t));
    assert(wInitEccoSql);
    wInitEccoSql[0] = L'\x0';
    mbstowcs(wtblName, tblName, WCHARSIZE);
    for (int i = 0; i < num0; ++i) {
        swprintf(lineStr, WIDEWCHARSIZE, L"INSERT INTO %ls (SN, NAME, NUMS, MOVESTR) "
                                         "VALUES ('%ls', '%ls', '%ls', '%ls' );\n",
            wtblName, sn[i], name[i], nums[i], moveStr[i]);
        supper_wcscat(&wInitEccoSql, &size, lineStr);
    }
    //fwprintf(fout, wInitEccoSql);

    size = wcslen(wInitEccoSql) * sizeof(wchar_t) + 1;
    *initEccoSql = malloc(size);
    wcstombs(*initEccoSql, wInitEccoSql, size);
    free(wInitEccoSql);
    free(fileWstring);
}

// 初始化开局类型编码名称
static void initEccoTable__(sqlite3* db, char* tblName, char* colNames, wchar_t* fileWstring)
{
    if (createTable__(db, tblName, colNames) < 0)
        return;

    // 获取插入记录字符串，并插入
    char *initEccoSql = NULL, *zErrMsg;
    getEccoSql__(&initEccoSql, tblName, fileWstring);

    int rc = sqlite3_exec(db, initEccoSql, NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "\nTable %s operate error: %s", tblName, zErrMsg);
        sqlite3_free(zErrMsg);
    } else
        fprintf(stdout, "\nTable %s have records: %d\n", tblName, getRecCount__(db, tblName, ""));

    free(initEccoSql);
}

static bool isMatch__(const char* str, const void* reg)
{
    int len = strlen(str) * sizeof(wchar_t) + 1, ovector[30];
    wchar_t wstr[len];
    mbstowcs(wstr, str, len);
    return pcrewch_exec(reg, NULL, wstr, wcslen(wstr), 0, 0, ovector, 30) > 0;
}

// 处理查询记录
static int updateMoveStr__(void* arg, int argc, char** argv, char** azColName)
{

    const char* error;
    int errorffset = 0;
    void* reg = pcrewch_compile(L"^[2-9a-z].", 0, &error, &errorffset, NULL);
    if (!isMatch__(argv[1], reg))
        return 0;

    //fprintf(fout, "%s\t%s\n", argv[0], argv[1]);
    return 0;
}

static void updateEccoField__(sqlite3* db, char* tblName)
{
    char sql[WCHARSIZE];
    //int count = 0;
    sprintf(sql, "SELECT SN, MOVESTR FROM %s "
                 //"WHERE LENGTH(SN) = 2 AND LENGTH(MOVESTR) > 0;",
                 "WHERE LENGTH(SN) = 3;",
        tblName);

    sqlite3_exec(db, sql, updateMoveStr__, NULL, NULL);
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

        char* tblName = "ecco";
        char colNames[] = "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                          "SN TEXT NOT NULL,"
                          "NAME TEXT,"
                          "NUMS TEXT,"
                          "MOVESTR TEXT,"
                          "MOVEROWCOL TEXT";
        initEccoTable__(db, tblName, colNames, fileWstring);

        updateEccoField__(db, tblName);
    }

    sqlite3_close(db);

    fclose(fin);
    fclose(fout);
}