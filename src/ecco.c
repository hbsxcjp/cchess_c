#include "head/ecco.h"
#include "head/chessManual.h"
#include "head/tools.h"

#define BOUT_MAX 80
#define FIELD_MAX 4
//* 输出字符串，检查用
static FILE* fout;

// 读取开局着法内容至数据表
static void getSplitFields__(wchar_t**** tables, int* record, int* field, const wchar_t* wstring)
{
    // 根据正则表达式分解字符串，获取字段内容
    const char* error;
    int errorffset, ovector[30], fieldCount[] = { 3, 4, 4, 2 };
    void* regs[] = {
        // field: sn name nums
        pcrewch_compile(L"([A-E])．(\\S+)\\((共[\\s\\S]+?局)\\)",
            0, &error, &errorffset, NULL),
        // field: sn name nums moveStr
        // B2. C0. D1. D2. D3. D4. \\s不包含"　"(全角空格)
        pcrewch_compile(L"([A-E]\\d)．(空|\\S+(?=\\(共))(?:(?![\\s　]+[A-E]\\d．)\\n|\\((共[\\s\\S]+?局)\\)"
                        "[\\s　]*([\\s\\S]*?)(?=[\\s　]*[A-E]\\d{2}．))",
            0, &error, &errorffset, NULL),
        // field: sn name moveStr nums
        pcrewch_compile(L"([A-E]\\d{2})．(\\S+)[\\s　]+"
                        "(?:(?![A-E]\\d|上一)([\\s\\S]*?)[\\s　]*(无|共[\\s\\S]+?局)[\\s\\S]*?(?=上|[A-E]\\d{0,2}．))?",
            0, &error, &errorffset, NULL),
        // field: sn moveStr C20 C30 C61 C72局面字符串
        pcrewch_compile(L"([A-E]\\d\\d局面) =([\\s\\S\n]*?)(?=[\\s　]*[A-E]\\d{2}．)",
            0, &error, &errorffset, NULL)
    };
    for (int g = 0; g < sizeof(regs) / sizeof(regs[0]); ++g) {
        tables[g] = malloc(THOUSAND * sizeof(wchar_t**));
        field[g] = fieldCount[g];
        int first = 0, last = wcslen(wstring), recIndex = 0;
        while (first < last) {
            const wchar_t* tempWstr = wstring + first;
            int count = pcrewch_exec(regs[g], NULL, tempWstr, last - first, 0, 0, ovector, 30);
            if (count <= 0)
                break;

            assert(recIndex < THOUSAND);
            assert(count - 1 <= field[g]);
            tables[g][recIndex] = calloc(field[g], sizeof(wchar_t*));

            //fwprintf(fout, L"%d:\t", recIndex);
            for (int f = 0; f < count - 1; ++f) {
                int start = ovector[2 * f + 2], end = ovector[2 * f + 3];
                wchar_t* mStr = malloc((end - start + 1) * sizeof(wchar_t));
                copySubStr(mStr, tempWstr, start, end);
                tables[g][recIndex][f] = mStr;

                //fwprintf(fout, L"%ls\t", mStr);
            }
            first += ovector[1];

            //fwprintf(fout, L"\n");
            recIndex++;
        }
        record[g] = recIndex;
        pcrewch_free(regs[g]);
    }
}

// 获取moveStr字段前置省略内容 有40+74=114项
static wchar_t* getPreMoveStr__(wchar_t**** tables, int* record, const wchar_t* snStr,
    const wchar_t* moveStr, void* reg_p0, void* reg_p1)
{
    int ovector[30];
    wchar_t sn[10];
    // 三级局面的 C20 C30 C61 C72局面 有40项
    if (pcrewch_exec(reg_p0, NULL, moveStr, wcslen(moveStr), 0, 0, ovector, 30) > 0) {
        copySubStr(sn, moveStr, ovector[2], ovector[3]);
        if (wcscmp(sn, L"C73局面") == 0)
            sn[2] = L'2'; // C73 => C72
        for (int ir = 0; ir < record[3]; ++ir)
            if (wcscmp(tables[3][ir][0], sn) == 0)
                return tables[3][ir][1];
    }

    // 前置省略的着法 有74项
    if (pcrewch_exec(reg_p1, NULL, moveStr, wcslen(moveStr), 0, 0, ovector, 30) > 0) {
        wcscpy(sn, snStr);
        if (sn[0] == L'C')
            sn[1] = L'0'; // C0/C1/C5/C6/C7/C8/C9 => C0
        sn[2] = L'\x0';
        if (wcscmp(sn, L"D5") == 0)
            wcscpy(sn, L"D51"); // D5 => D51

        int level = wcslen(sn) - 1;
        for (int ir = 0; ir < record[level]; ++ir)
            if (wcscmp(tables[level][ir][0], sn) == 0)
                return tables[level][ir][level == 1 ? 3 : 2];
    }
    return NULL;
}

// 将不分前后的着法组合成着法串, 返回着法数量（“此前...”，“红方：...黑方：...”）
static void getMoves__(wchar_t* moves, wchar_t* moveStr, void* reg_m)
{
    int first = 0, last = wcslen(moveStr), ovector[30];
    wchar_t move[WCHARSIZE];
    moves[0] = L'\x0';
    while (first < last) {
        wchar_t* tempWstr = moveStr + first;
        if (pcrewch_exec(reg_m, NULL, tempWstr, last - first, 0, 0, ovector, 30) <= 0)
            break;

        copySubStr(move, tempWstr, ovector[2], ovector[3]);
        wcscat(moves, move);
        wcscat(moves, L"／");
        first += ovector[1];
    }
    if (wcslen(moves) > 0)
        moves[wcslen(moves) - 1] = L'\x0'; // 消除最后的“／”
}

// 调试输出
static void showBoutMove__(int no, wchar_t**** tables, int r, wchar_t boutMove[][FIELD_MAX][WCHARSIZE],
    int choiceIndex[][2], int (*repeatCount)[FIELD_MAX])
{
    fwprintf(fout, L"No.%d %ls\t%ls\t%ls\nFormatMoveStr: ", no, tables[2][r][0], tables[2][r][1], tables[2][r][2]);
    for (int i = 0; i < BOUT_MAX; ++i)
        if (boutMove[i][0][0] || boutMove[i][1][0] || boutMove[i][2][0] || boutMove[i][3][0])
            fwprintf(fout, L"\n%d. %ls|%ls\t%ls|%ls Repeat:%d",
                i, boutMove[i][0], boutMove[i][1], boutMove[i][2], boutMove[i][3], repeatCount[i][0]);
    for (int i = 0; i < 2; ++i)
        fwprintf(fout, L"\nPre_Next:%d start:%d end:%d", i, choiceIndex[i][0], choiceIndex[i][1]);
    fwprintf(fout, L"\n\n");
}

// 处理前置着法描述字符串————"红方：黑方："
static bool putRedBlackMove__(wchar_t boutMove[][FIELD_MAX][WCHARSIZE], int (*repeatCount)[FIELD_MAX], wchar_t* moveStr,
    void* reg_sp, void* reg_m)
{
    int ovector[30];
    if (pcrewch_exec(reg_sp, NULL, moveStr, wcslen(moveStr), 0, 0, ovector, 30) == 3) {
        wchar_t splitStr[WCHARSIZE], moves[WCHARSIZE];
        for (int s = 0; s < 2; ++s) {
            copySubStr(splitStr, moveStr, ovector[2 * s + 2], ovector[2 * s + 3]);
            getMoves__(moves, splitStr, reg_m);
            wcscpy(boutMove[1][2 * s], moves);
            //fwprintf(fout, L"s:%d %ls\n", s, moves);
        }
        repeatCount[1][0] = (wcslen(moves) + 1) / 5;
        return true;
    }
    return false;
}

// 处理"此前..."着法描述字符串
static void putPreMove__(wchar_t boutMove[][FIELD_MAX][WCHARSIZE], int choiceIndex[][2], wchar_t* moveStr,
    int fieldIndex, int other, int partIndex, void* reg_m)
{
    int insertBoutIndex = 10 + 30 * partIndex;
    wchar_t moves[WCHARSIZE];
    getMoves__(moves, moveStr, reg_m);
    swprintf(boutMove[insertBoutIndex][fieldIndex], WCHARSIZE, L"%ls%ls",
        wcschr(moveStr, L'除') == NULL ? L"" : L"-", moves);
    // (变着，且非变着为空)||(非变着，且属覆盖前置，且前置有变着)，复制“此前”着法
    if (other == 1 && wcslen(boutMove[insertBoutIndex][fieldIndex - 1]) == 0)
        wcscpy(boutMove[insertBoutIndex][fieldIndex - 1], boutMove[insertBoutIndex][fieldIndex]);
    // 变着，且非前置，起始序号设此序号
    if (other == 1 && partIndex == 1)
        choiceIndex[1][0] = insertBoutIndex;
}

// 获取回合着法字符串数组,
static void putBoutMove__(wchar_t boutMove[][FIELD_MAX][WCHARSIZE], int (*repeatCount)[FIELD_MAX], int choiceIndex[][2], wchar_t* moveStr,
    void* reg_sp, void* reg_bm, void* reg_m, int partIndex)
{
    int first = 0, last = wcslen(moveStr), sn = 0, psn = 0, boutIndex = 0, other = 0, ovector[30];
    while (first < last) {
        wchar_t* tempWstr = moveStr + first;
        int count = pcrewch_exec(reg_bm, NULL, tempWstr, last - first, 0, 0, ovector, 30);
        if (count < 3)
            break;

        if ((sn = tempWstr[ovector[2]]) <= psn)
            other = 1; // 此后着法皆为变着
        psn = sn;
        boutIndex = sn - L'0' + (partIndex == 0 && sn > L'a' ? -40 : 0);
        for (int i = 2; i < count; ++i) {
            wchar_t wstr[WCHARSIZE];
            copySubStr(wstr, tempWstr, ovector[2 * i], ovector[2 * i + 1]);
            if (wcslen(wstr) == 0)
                continue;

            //fwprintf(fout, L"\ti:%d %ls\n", i, wstr);
            int fieldIndex = (i / 5) * 2 + other;
            if (i == 2 || i == 5) // 回合的着法
                wcscpy(boutMove[boutIndex][fieldIndex], wstr);
            else if (i == 3 || i == 6) // "不分先后"
                ;
            else if (i == 4 || i == 7) // "此前..."
                putPreMove__(boutMove, choiceIndex, wstr, fieldIndex, other, partIndex, reg_m);

            if (other == 1 && choiceIndex[partIndex][0] == 0)
                choiceIndex[partIndex][0] = boutIndex;
        }
        first += ovector[1];
    }
    if (other == 1)
        choiceIndex[partIndex][1] = boutIndex;
}

// 格式化处理tables[2][r][2]=>moveStr字段
static void formatMoveStrs__(wchar_t**** tables, int* record)
{
    const char* error;
    int errorffset, no = 0;
    wchar_t ZhWChars[WCHARSIZE], mStr[WCHARSIZE], msStr[WIDEWCHARSIZE],
        bmsStr[WIDEWCHARSIZE], *split = L"[，、；\\s　和\\(\\)以／]|$";
    // 着法字符组
    getZhWChars(ZhWChars);
    // 着法，含多选的复式着法
    swprintf(mStr, WCHARSIZE, L"(?:[%ls]{4}(?:／[%ls]{4})*)", ZhWChars, ZhWChars);
    // 捕捉一步着法: 1.着法，2.可能的“此前...”着法，3. 可能的“／\\n...$”着法
    swprintf(msStr, WIDEWCHARSIZE, L"(%ls|……)(?:%ls)(\\(不.先后[\\)，])?([^%ls]*?此前[^\\)]+?\\))?", mStr, split, ZhWChars); //  (?:(／\\n[\\s\\S]+)$)?
    // 捕捉一个回合着法：1.序号，2.一步着法的1，3-5.着法或“此前”，4-6.着法或“此前”或“／\\n...$”
    swprintf(bmsStr, WIDEWCHARSIZE, L"([\\da-z]). %ls(?:%ls)?", msStr, msStr);
    void *reg_m = pcrewch_compile(msStr, 0, &error, &errorffset, NULL),
         *reg_bm = pcrewch_compile(bmsStr, 0, &error, &errorffset, NULL),
         *reg_p0 = pcrewch_compile(L"从([A-E]\\d\\d局面)开始：", 0, &error, &errorffset, NULL),
         *reg_p1 = pcrewch_compile(L"^[2-9a-z].", 0, &error, &errorffset, NULL),
         *reg_sp = pcrewch_compile(L"红方：(.+)\\n黑方：(.+)", 0, &error, &errorffset, NULL);
    fwprintf(fout, L"%ls\n%ls\n%ls\n%ls\n%ls\n\n", split, ZhWChars, mStr, msStr, bmsStr);

    for (int r = 0; r < record[2]; ++r) {
        wchar_t* moveStr = tables[2][r][2];
        if (moveStr == NULL)
            continue;

        // 着法可选变着的起止boutIndex [Pre-Next][Start-End]
        int choiceIndex[2][2] = { 0 }, repeatCount[BOUT_MAX][FIELD_MAX] = { 0 };
        // 着法记录数组，以序号字符ASCII值为数组索引存入(例如：L'a'-L'1')
        wchar_t boutMove[BOUT_MAX][FIELD_MAX][WCHARSIZE] = { 0 },
                *preMoveStr = getPreMoveStr__(tables, record, tables[2][r][0], moveStr, reg_p0, reg_p1);
        if (preMoveStr && !putRedBlackMove__(boutMove, repeatCount, preMoveStr, reg_sp, reg_m))
            putBoutMove__(boutMove, repeatCount, choiceIndex, preMoveStr, reg_sp, reg_bm, reg_m, 0);

        putBoutMove__(boutMove, repeatCount, choiceIndex, moveStr, reg_sp, reg_bm, reg_m, 1);
        if (preMoveStr) {
            fwprintf(fout, L"PreMoveStr: %ls\n", preMoveStr);
            showBoutMove__(no++, tables, r, boutMove, choiceIndex, repeatCount);
        }
    }

    pcrewch_free(reg_m);
    pcrewch_free(reg_bm);
    pcrewch_free(reg_p0);
    pcrewch_free(reg_p1);
    pcrewch_free(reg_sp);
}

static void testGetFields__(wchar_t* fileWstring)
{
    // 读取全部所需的字符串数组
    int group = 4, record[group], field[group];
    wchar_t*** tables[group];
    getSplitFields__(tables, record, field, fileWstring);

    formatMoveStrs__(tables, record);

    //*reg1 = pcrewch_compile(L"([2-9a-z]. ?)([^，a-z\\f\\r\\t\\v]+)(，[^　／a-z\\f\\r\\t\\v]+)?",
    //    0, &error, &errorffset, NULL);

    /*
        int first = 0, last = wcslen(moveStr);
        while (first < last) {
            wchar_t moves[WCHARSIZE], *tempWstr = moveStr + first;
            if (pcrewch_exec(reg1, NULL, tempWstr, last - first, 0, 0, ovector, 30) <= 0)
                break;

            //copySubStr(moves, tempWstr, ovector[2], ovector[3]);

            //fwprintf(fout, L"%ls\n", moves);
            first += ovector[1];
        }
        //*/

    //pcrewch_free(reg1);

    // 释放资源
    for (int g = 0; g < group; ++g) {
        for (int r = 0; r < record[g]; ++r) {
            for (int f = 0; f < field[g]; ++f)
                free(tables[g][r][f]);
            free(tables[g][r]);
        }
        free(tables[g]);
    }
}

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
        } // else
        // fprintf(stdout, "\nTable %s deleted successfully.", tblName);
    }

    // 创建表
    sprintf(sql, "CREATE TABLE %s(%s);", tblName, colNames);
    rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "\nTable %s created error: %s", tblName, zErrMsg);
        sqlite3_free(zErrMsg);
        return -1;
    } //else
    //fprintf(stdout, "\nTable %s created successfully.", tblName);

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
        pcrewch_compile(L"([A-E]\\d\\d?局面) =([\\s\\S\n]*?)(?=[\\s　]*[A-E]\\d{2}．)",
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

            //fwprintf(fout, L"%d\t%ls\t%ls\t%ls\t%ls\n", index, sn[index], name[index], nums[index], moveStr[index]);
            index++;
        }
        pcrewch_free(regs[i]);
    }
    if (index != num)
        printf("index:%d num:%d.\n", index, num);
    assert(index == num);

    /*/ 修正C20 C30 C61 C72局面
    for (int i = num0; i < num; ++i) // 555-558
        for (int j = 0; j < num0; ++j)
            if (wcscmp(sn[j], sn[i]) == 0) {
                wcscpy(moveStr[j], moveStr[i]);
                break;
            }
            //*/

    // 修正moveStr
    //int No = 0;
    void *reg_r = pcrewch_compile(L"^[2-9a-z].", 0, &error, &errorffset, NULL),
         *reg1 = pcrewch_compile(L"([2-9a-z]. ?)([^，a-z\\f\\r\\t\\v]+)(，[^　／a-z\\f\\r\\t\\v]+)?",
             0, &error, &errorffset, NULL);
    // ([2-9a-z]. ?)([^，a-z\f\r\t\v]+)(，[^　／a-z\f\r\t\v]+)?

    // wcslen(sn[i]) == 3  有74项
    wchar_t tsn[10], fmoveStr[WIDEWCHARSIZE], moves[WCHARSIZE]; //tmoveStr[WIDEWCHARSIZE],
    for (int i = 55; i < num0; ++i) {
        if (pcrewch_exec(reg_r, NULL, moveStr[i], wcslen(moveStr[i]), 0, 0, ovector, 30) <= 0)
            continue;

        // 取得替换模板字符串
        wcscpy(tsn, sn[i]);
        if (tsn[0] == L'C')
            tsn[1] = L'0'; // C0/C1/C5/C6/C7/C8/C9 => C0
        tsn[2] = L'\x0';
        if (wcscmp(tsn, L"D5") == 0)
            wcscpy(tsn, L"D51"); // D51 => D5
        for (int j = 0; j < num0; ++j)
            if (wcscmp(sn[j], tsn) == 0) {
                wcscpy(fmoveStr, moveStr[j]);
                break;
            }

        //fwprintf(fout, L"No:%d\n%ls\t%ls\n\n", No++, sn[i], moveStr[i]);
        first = 0;
        last = wcslen(moveStr[i]);
        while (first < last) {
            tempWstr = moveStr[i] + first;
            if (pcrewch_exec(reg1, NULL, tempWstr, last - first, 0, 0, ovector, 30) <= 0)
                break;

            copySubStr(moves, tempWstr, ovector[2], ovector[3]);

            //fwprintf(fout, L"%ls\n", moves);
            first += ovector[1];
        }
        //fwprintf(fout, L"\nfmoveStr:%ls\n", fmoveStr);
    }
    pcrewch_free(reg_r);
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
    fout = fopen("chessManual/eccolib", "w");
    // 读取开局库源文件内容
    FILE* fin = fopen("chessManual/eccolib_src", "r");
    wchar_t* fileWstring = getWString(fin);
    assert(fileWstring);

    sqlite3* db = NULL;
    int rc = sqlite3_open(dbName, &db);
    if (rc) {
        fprintf(stderr, "\nCan't open database: %s", sqlite3_errmsg(db));
    } else {
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

    free(fileWstring);
    fclose(fin);
    fclose(fout);
}

void testEcco(void)
{
    char *wm, *rm;
#ifdef __linux
    wm = "w";
    rm = "r";
#else
    wm = "w, ccs=UTF-8";
    rm = "r, ccs=UTF-8";
#endif

    fout = fopen("chessManual/eccolib", wm);
    FILE* fin = fopen("chessManual/eccolib_src", rm);
    wchar_t* fileWstring = getWString(fin);
    assert(fileWstring);

    testGetFields__(fileWstring);

    free(fileWstring);
    fclose(fin);
    fclose(fout);
}