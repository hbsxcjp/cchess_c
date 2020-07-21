#include "head/ecco.h"

sqlite3* db = NULL;

static void initLevelOne__(void)
{
    char tag[] = { 'A', 'B', 'C', 'D', 'E' },
         *name[] = {
             "非中炮类开局(不包括仙人指路局)",
             "中炮对反宫马及其他",
             "中炮对屏风马",
             "顺炮局和列炮局(包括半途列炮局)",
             "仙人指路局"
         },

         *createSql = "CREATE TABLE levelOne("
                      "TAG    CHAR(1) NOT NULL,"
                      "NAME   TEXT    NOT NULL)",

         *insertSql_t = "INSERT INTO levelOne (TAG,NAME) "
                        "VALUES ('%c', '%s' );";

    char* zErrMsg = 0;
    int rc = sqlite3_exec(db, createSql, NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        fprintf(stdout, "Table created successfully.\n");

        char insertSql[WIDEWCHARSIZE],
            tempSql[WIDEWCHARSIZE];
        insertSql[0] = '\x0';
        for (int i = 0; i < sizeof(tag) / sizeof(tag[0]); i++) {
            sprintf(tempSql, insertSql_t, tag[i], name[i]);
            strcat(insertSql, tempSql);
        }
        rc = sqlite3_exec(db, insertSql, NULL, NULL, &zErrMsg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        } else {
            fprintf(stdout, "Insert values successfully.\n");
        }
    }
}

void eccoInit(void)
{
    //char* zErrMsg = 0;
    int rc = sqlite3_open("ecco.db", &db);
    if (rc) {
        fprintf(stderr, "\nCan't open database: %s\n", sqlite3_errmsg(db));
    } else {
        fprintf(stderr, "Opened database successfully. ");

        initLevelOne__();
    }

    sqlite3_close(db);
}