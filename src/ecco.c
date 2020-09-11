#include "head/ecco.h"
#include "head/chessManual.h"
#include "head/tools.h"

#define BOUT_MAX 80
#define MOVEFIELD_MAX 4
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
    if (pcrewch_exec(reg_p0, NULL, moveStr, wcslen(moveStr), 0, 0, ovector, 30) > 0) {
        // 处理三级局面的 C20 C30 C61 C72局面 有40项
        copySubStr(sn, moveStr, ovector[2], ovector[3]);
        if (wcscmp(sn, L"C73局面") == 0)
            sn[2] = L'2'; // C73 => C72
        for (int ir = 0; ir < record[3]; ++ir)
            if (wcscmp(tables[3][ir][0], sn) == 0)
                return tables[3][ir][1];
    }

    if (pcrewch_exec(reg_p1, NULL, moveStr, wcslen(moveStr), 0, 0, ovector, 30) > 0) {
        // 处理 前置省略的着法 有74项
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
static int getMoves__(wchar_t* move, wchar_t* moveStr, void* reg_m)
{
    int count = 0, first = 0, last = wcslen(moveStr), ovec[30];
    wchar_t mv[WCHARSIZE];
    move[0] = L'\x0';
    while (first < last) {
        wchar_t* tempWstr = moveStr + first;
        if (pcrewch_exec(reg_m, NULL, tempWstr, last - first, 0, 0, ovec, 30) <= 0)
            break;

        copySubStr(mv, tempWstr, ovec[2], ovec[3]);
        wcscat(move, mv);
        wcscat(move, L"／");
        ++count;
        first += ovec[1];
    }
    if (wcslen(move) > 0)
        move[wcslen(move) - 1] = L'\x0'; // 消除最后的“／”

    return count;
}

// 获取回合着法字符串数组, other:其他排列着法的标志
static void getBoutMove__(wchar_t boutMove[][MOVEFIELD_MAX][WCHARSIZE], wchar_t* moveStr, void* reg_bm, void* reg_m, int other)
{
    int first = 0, last = wcslen(moveStr), ovec[30];
    while (first < last) {
        wchar_t* tempWstr = moveStr + first;
        int count = pcrewch_exec(reg_bm, NULL, tempWstr, last - first, 0, 0, ovec, 30);
        if (count < 3)
            break;

        // 回合数组的序号, 首着或应着的序号
        int boutIndex = tempWstr[ovec[2]] - L'1', moveIndex = 0;
        copySubStr(boutMove[boutIndex][moveIndex + other], tempWstr, ovec[4], ovec[5]); // 回合的首着

        fwprintf(fout, L"bI:%d mv:%ls\n", boutIndex, boutMove[boutIndex][moveIndex + other]);
        for (int i = 3; i < count; ++i) {
            int start = ovec[2 * i], end = ovec[2 * i + 1];
            if (start == end)
                continue;
            wchar_t wstr[WCHARSIZE];
            copySubStr(wstr, tempWstr, start, end);
            fwprintf(fout, L"\ti:%d wstr:%ls\n", i, wstr);

            if (wcsstr(wstr, L"此前") != NULL) { // 非直接着法，"此前..."

                //    printf("count:%d i:%d\n", count, i);

                wchar_t insBoutIndex = L'a' - L'1', move[WCHARSIZE];
                getMoves__(move, wstr, reg_m);
                while (wcslen(boutMove[insBoutIndex][moveIndex + other]) != 0)
                    ++insBoutIndex; // 拟插入的空位
                if (wcsstr(wstr, L"除") != NULL)
                    wcscpy(boutMove[insBoutIndex][moveIndex + other], L"-");
                wcscat(boutMove[insBoutIndex][moveIndex + other], move);
            } else if (wcsstr(wstr, L"／\\n") != NULL) { // 非直接着法，"／\\n...$"
                getBoutMove__(boutMove, wstr, reg_bm, reg_m, 1); // 递归调用，存储至数组的旁边一列
            } else { // 回合的应着
                moveIndex = 2;
                wcscpy(boutMove[boutIndex][moveIndex + other], wstr);
            }
        }
        first += ovec[1];
    }
}

// 获取不分先后的着法字符串数组
static void getNoOrderMove__(wchar_t boutMove[][MOVEFIELD_MAX][WCHARSIZE], int moveIndex, wchar_t* moveStr, void* reg_m)
{
    wchar_t move[WCHARSIZE];
    int count = getMoves__(move, moveStr, reg_m);
    // 删除首着方的第一步着法
    if (moveIndex == 0 && count > 1) {
        int len = wcslen(move);
        for (int i = 5; i <= len; ++i)
            move[i - 5] = move[i];
    }

    // 将组合着法串写入着法数组
    int boutIndex = 0;
    while (wcslen(boutMove[boutIndex][moveIndex]) != 0)
        ++boutIndex;
    // 第一着不计数
    for (int i = 0; i < count - 1; ++i)
        wcscpy(boutMove[boutIndex + i][moveIndex], move);
}

// 调试输出
static void showBoutMove__(int no, wchar_t**** tables, int r, wchar_t boutMove[][MOVEFIELD_MAX][WCHARSIZE])
{
    fwprintf(fout, L"No.%d %ls\t%ls\t%ls\nFormatMoveStr: ", no, tables[2][r][0], tables[2][r][1], tables[2][r][2]);
    for (int i = 0; i < BOUT_MAX; ++i)
        if (wcslen(boutMove[i][0]) > 0)
            fwprintf(fout, L"\n%d. %ls|%ls\t%ls|%ls", i, boutMove[i][0], boutMove[i][1], boutMove[i][2], boutMove[i][3]);
    fwprintf(fout, L"\n\n");
}

// 格式化处理tables[2][r][2]=>moveStr字段
static void formatMoveStrs__(wchar_t**** tables, int* record)
{
    const char* error;
    int errorffset, no = 0;
    wchar_t ZhWChars[WCHARSIZE], mStr[WCHARSIZE], msStr[WIDEWCHARSIZE],
        bmsStr[WIDEWCHARSIZE], *split = L"[，、；\\s　和\\(／以]|$";
    // 着法字符组
    getZhWChars(ZhWChars);
    // 着法，含多选的复式着法
    swprintf(mStr, WCHARSIZE, L"(?:[%ls]{4}(?:／[%ls]{4})*)", ZhWChars, ZhWChars);
    // 捕捉一步着法: 1.着法，2.可能的“此前...”着法，3. 可能的“／\\n...$”着法
    swprintf(msStr, WIDEWCHARSIZE, L"(%ls|……)(?:%ls)(?:[\\S]*?(此前[^\\)]+?)\\))?(?:／\\n([\\s\\S]+)$)?", mStr, split);
    // 捕捉一个回合着法：1.序号，2.一步着法的1，3-5.着法或“此前”，4-6.着法或“此前”或“／\\n...$”
    swprintf(bmsStr, WIDEWCHARSIZE, L"([\\da-z]). %ls(?:%ls)?", msStr, msStr);
    void *reg_m = pcrewch_compile(msStr, 0, &error, &errorffset, NULL),
         *reg_bm = pcrewch_compile(bmsStr, 0, &error, &errorffset, NULL),
         *reg_p0 = pcrewch_compile(L"从([A-E]\\d\\d局面)开始：", 0, &error, &errorffset, NULL),
         *reg_p1 = pcrewch_compile(L"^[2-9a-z].", 0, &error, &errorffset, NULL),
         *reg_sp = pcrewch_compile(L"(.+)\\n红方：(.+)\\n黑方：(.+)", 0, &error, &errorffset, NULL);
    fwprintf(fout, L"%ls\n%ls\n%ls\n%ls\n%ls\n\n", split, ZhWChars, mStr, msStr, bmsStr);

    for (int r = 0; r < record[2]; ++r) {
        wchar_t* moveStr = tables[2][r][2];
        if (moveStr == NULL)
            continue;

        int ovector[30];
        // 着法记录数组，以序号字符ASCII值为数组索引存入(例如：L'a'-L'1')
        wchar_t boutMove[BOUT_MAX][MOVEFIELD_MAX][WCHARSIZE] = { 0 };
        // 获取前置着法描述
        wchar_t* preMoveStr = getPreMoveStr__(tables, record, tables[2][r][0], moveStr, reg_p0, reg_p1);
        if (preMoveStr != NULL) {

            //fwprintf(fout, L"%ls\t%ls\n", tables[2][r][0], preMoveStr);
            if (pcrewch_exec(reg_sp, NULL, preMoveStr, wcslen(preMoveStr), 0, 0, ovector, 30) == 4) {
                // 处理着法描述字符串————"红方：黑方："
                //fwprintf(fout, L"%ls\t%ls\n", tables[2][r][0], preMoveStr);
                for (int s = 0; s < 3; ++s) {
                    wchar_t splitStr[WCHARSIZE];
                    copySubStr(splitStr, preMoveStr, ovector[2 * s + 2], ovector[2 * s + 3]);
                    if (s == 0)
                        getBoutMove__(boutMove, splitStr, reg_bm, reg_m, 0);
                    else
                        getNoOrderMove__(boutMove, (s - 1) * 2, splitStr, reg_m);
                }
                //showBoutMove__(no++, tables, r, boutMove);
            } else {
                // 处理着法描述字符串————"1. 2. ..... k. l. m. n. "
                fwprintf(fout, L"%ls\t%ls\n", tables[2][r][0], preMoveStr);
                getBoutMove__(boutMove, preMoveStr, reg_bm, reg_m, 0);
                showBoutMove__(no++, tables, r, boutMove);
            }
            //getBoutMove__(boutMove, moveStr, reg_bm, reg_m, 0);// 覆盖前置已填充着法
            //showBoutMove__(no++, tables, r, boutMove);
        }
        //getBoutMove__(boutMove, moveStr, reg_bm, reg_m, 0);
        //if (preMoveStr != NULL)
        //showBoutMove__(no++, tables, r, boutMove);
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
            wchar_t move[WCHARSIZE], *tempWstr = moveStr + first;
            if (pcrewch_exec(reg1, NULL, tempWstr, last - first, 0, 0, ovector, 30) <= 0)
                break;

            //copySubStr(move, tempWstr, ovector[2], ovector[3]);

            //fwprintf(fout, L"%ls\n", move);
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
    wchar_t tsn[10], fmoveStr[WIDEWCHARSIZE], move[WCHARSIZE]; //tmoveStr[WIDEWCHARSIZE],
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

            copySubStr(move, tempWstr, ovector[2], ovector[3]);

            //fwprintf(fout, L"%ls\n", move);
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