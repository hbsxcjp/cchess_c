#include "head/ecco.h"

sqlite3* db = NULL;

// 获取查询记录结果的数量
static int getCount__(void* count, int argc, char** argv, char** azColName)
{
    *(int*)count = (argv[0] ? atoi(argv[0]) : 0);
    return 0;
}

// 初始化表及原始数据
static void initTable__(char* tblName, char* creatTableSql, char* insertValuesSql)
{
    // 查找表
    char searchSql[WIDEWCHARSIZE], *zErrMsg = 0;
    int count = 0, rc = 0;
    sprintf(searchSql, "SELECT count(tbl_name) FROM sqlite_master "
                       "WHERE type = 'table' AND tbl_name = '%s';",
        tblName);
    rc = sqlite3_exec(db, searchSql, getCount__, &count, &zErrMsg);
    if (count == 0) {
        // 创建表
        rc = sqlite3_exec(db, creatTableSql, NULL, NULL, &zErrMsg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Create table error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
            return;
        } else
            fprintf(stdout, "Table created successfully.\n");
    }

    sprintf(searchSql, "SELECT count(TAG) FROM %s ", tblName);
    rc = sqlite3_exec(db, searchSql, getCount__, &count, &zErrMsg);
    if (count == 0) {
        // 插入记录
        rc = sqlite3_exec(db, insertValuesSql, NULL, NULL, &zErrMsg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Insert values error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
            return;
        } else {
            fprintf(stdout, "Values inserted successfully.\n");
        }
    }
}

// 初始化开局类型一级编码名词
static void initLevelOne__(void)
{
    char *tblName = "levelOne", createTableSql[WIDEWCHARSIZE];
    sprintf(createTableSql, "CREATE TABLE %s("
                            "TAG    CHAR(1) NOT NULL PRIMARY KEY,"
                            "NAME   TEXT    NOT NULL);",
        tblName);

    char tag[] = { 'A', 'B', 'C', 'D', 'E' };
    char* name[] = {
        "非中炮类开局(不包括仙人指路局)",
        "中炮对反宫马及其他",
        "中炮对屏风马",
        "顺炮局和列炮局(包括半途列炮局)",
        "仙人指路局"
    };
    char* insertFormat = "INSERT INTO %s (TAG,NAME) "
                         "VALUES ('%c', '%s' );";
    char insertValuesSql[WIDEWCHARSIZE], tempSql[WIDEWCHARSIZE];
    insertValuesSql[0] = '\x0';
    for (int i = 0; i < sizeof(tag) / sizeof(tag[0]); i++) {
        sprintf(tempSql, insertFormat, tblName, tag[i], name[i]);
        strcat(insertValuesSql, tempSql);
    }

    initTable__(tblName, createTableSql, insertValuesSql);
}

// 初始化开局类型二级编码名词
static void initLevelTwo__(void)
{
    char *tblName = "levelTwo", createTableSql[WIDEWCHARSIZE];
    sprintf(createTableSql, "CREATE TABLE %s("
                            "SN    TEXT NOT NULL PRIMARY KEY,"
                            "NAME   TEXT    NOT NULL);",
        tblName);

    char* sn_names[][2] = {
        { "A0", "非常见开局" },
        { "A1", "飞相局(一)" },
        { "A2", "飞相局(二)" },
        { "A3", "飞相局(三)" },
        { "A4", "起马局" },
        { "A5", "仕角炮局" },
        { "A6", "过宫炮局" },
        { "B0", "中炮对非常见开局" },
        { "B1", "中炮对单提马" },
        { "B2", "中炮对左三步虎(不包括转列炮)" },
        { "B3", "中炮对反宫马(一)" },
        { "B4", "中炮对反宫马(二)——五六炮对反宫马" },
        { "B5", "中炮对反宫马(三)——五七炮对反宫马" },
        { "C0", "中炮对屏风马(一)" },
        { "C1", "中炮对屏风马(二)" },
        { "C2", "中炮过河车七路马对屏风马两头蛇" },
        { "C3", "中炮过河车互进七兵对屏风马(不包括平炮兑车)" },
        { "C4", "中炮过河车互进七兵对屏风马平炮兑车" },
        { "C5", "五六炮对屏风马" },
        { "C6", "五七炮对屏风马(一)" },
        { "C7", "五七炮对屏风马(二)" },
        { "C8", "中炮巡河炮对屏风马" },
        { "C9", "五八炮和五九炮对屏风马" },
        { "D0", "顺炮局(不包括顺炮直车局)" },
        { "D1", "顺炮直车对缓开车" },
        { "D2", "顺炮直车对横车" },
        { "D3", "中炮对左炮封车转列炮" },
        { "D4", "中炮对左三步虎转列炮" },
        { "D5", "中炮对列炮(包括后补列炮)" },
        { "E0", "仙人指路局(不包括仙人指路对卒底炮和对兵局)" },
        { "E1", "仙人指路对卒底炮(不包括仙人指路转左中炮对卒底炮飞左象)" },
        { "E2", "仙人指路转左中炮对卒底炮飞左象(一)" },
        { "E3", "仙人指路转左中炮对卒底炮飞左象(二)" },
        { "E4", "对兵局" }
    };
    char* insertFormat = "INSERT INTO %s (SN, NAME) "
                         "VALUES ('%s', '%s' );";
    char insertValuesSql[SUPERWIDEWCHARSIZE], tempSql[WIDEWCHARSIZE];
    insertValuesSql[0] = '\x0';
    for (int i = 0; i < sizeof(sn_names) / sizeof(sn_names[0]); i++) {
        sprintf(tempSql, insertFormat, tblName, sn_names[i][0], sn_names[i][1]);
        strcat(insertValuesSql, tempSql);
    }

    initTable__(tblName, createTableSql, insertValuesSql);
}

void eccoInit(void)
{
    //char* zErrMsg = 0;
    int rc = sqlite3_open("ecco.db", &db);
    if (rc) {
        fprintf(stderr, "\nCan't open database: %s\n", sqlite3_errmsg(db));
    } else {
        fprintf(stderr, "\nOpened database successfully. \n");

        initLevelOne__();
        initLevelTwo__();
    }

    sqlite3_close(db);
}