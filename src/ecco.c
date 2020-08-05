#include "head/ecco.h"
#include "head/tools.h"

// 获取查询记录结果的数量
static int getCount__(void* count, int argc, char** argv, char** azColName)
{
    *(int*)count = (argv[0] ? atoi(argv[0]) : 0);
    return 0;
}

static bool existTable__(sqlite3* db, char* tblName)
{
    // 查找表
    char searchSql[WCHARSIZE], *zErrMsg = 0;
    int count = 0;
    sprintf(searchSql, "SELECT count(tbl_name) FROM sqlite_master "
                       "WHERE type = 'table' AND name = '%s';",
        tblName);
    int rc = sqlite3_exec(db, searchSql, getCount__, &count, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Search table error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return false;
    }
    //printf("Table name: %s, record count: %d\n", tblName, count);

    return count > 0;
}

// 初始化表及原始数据
static void initTable__(sqlite3* db, char* tblName, char* creatTableSql, char* insertValuesSql)
{
    // 创建表
    char* zErrMsg = 0;
    int rc = sqlite3_exec(db, creatTableSql, NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Create table error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return;
    } //else
    //  fprintf(stdout, "Table created successfully.\n");

    // 插入记录
    rc = sqlite3_exec(db, insertValuesSql, NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Insert values error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } //else {
    //  fprintf(stdout, "Values inserted successfully.\n");
    //}
}

// 初始化开局类型编码名称
static void initSN_Name__(sqlite3* db)
{
    char* tblName = "sn_name";
    if (existTable__(db, tblName))
        return;

    char* sn_names[][2] = {
        { "A", "非中炮类开局(不包括仙人指路局)" },
        { "B", "中炮对反宫马及其他" },
        { "C", "中炮对屏风马" },
        { "D", "顺炮局和列炮局(包括半途列炮)" },
        { "E", "仙人指路局" },
        { "A0", "非常见开局" },
        { "A1", "飞相局(一)" },
        { "A2", "飞相局(二)" },
        { "A3", "飞相局(三)" },
        { "A4", "起马局" },
        { "A5", "仕角炮局" },
        { "A6", "过宫炮局" },
        { "A7", "" },
        { "A8", "" },
        { "A9", "" },
        { "B0", "中炮对非常见开局" },
        { "B1", "中炮对单提马" },
        { "B2", "中炮对左三步虎(不包括转列炮)" },
        { "B3", "中炮对反宫马(一)" },
        { "B4", "中炮对反宫马(二)——五六炮对反宫马" },
        { "B5", "中炮对反宫马(三)——五七炮对反宫马" },
        { "B6", "" },
        { "B7", "" },
        { "B8", "" },
        { "B9", "" },
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
        { "D6", "" },
        { "D7", "" },
        { "D8", "" },
        { "D9", "" },
        { "E0", "仙人指路局" },
        { "E1", "仙人指路对卒底炮(不包括仙人指路转左中炮对卒底炮飞左象)" },
        { "E2", "仙人指路转左中炮对卒底炮飞左象(一)" },
        { "E3", "仙人指路转左中炮对卒底炮飞左象(二)" },
        { "E4", "对兵局" },
        { "E5", "" },
        { "E6", "" },
        { "E7", "" },
        { "E8", "" },
        { "E9", "" },
        { "A00", "(其他开局)" },
        { "A01", "上仕局" },
        { "A02", "边马局" },
        { "A03", "边炮局" },
        { "A04", "巡河炮局" },
        { "A05", "过河炮局" },
        { "A06", "兵底炮局" },
        { "A07", "金钩炮局" },
        { "A08", "边兵局" },
        { "A09", "" },
        { "A10", "飞相局(对其他)" },
        { "A11", "顺相局" },
        { "A12", "列相局" },
        { "A13", "飞相对进左马" },
        { "A14", "飞相(其他)对进右马" },
        { "A15", "飞相进三兵对进右马" },
        { "A16", "飞相进七兵对进右马" },
        { "A17", "" },
        { "A18", "" },
        { "A19", "" },
        { "A20", "飞相对左士角炮" },
        { "A21", "飞相(其他)对右士角炮" },
        { "A22", "飞相进左马对右士角炮" },
        { "A23", "飞相左边马对右士角炮" },
        { "A24", "飞相横车对右士角炮" },
        { "A25", "飞相进三兵对右士角炮" },
        { "A26", "飞相进七兵对右士角炮" },
        { "A27", "飞相(转其他)对左中炮" },
        { "A28", "飞相转屏风马对左中炮" },
        { "A29", "飞相对右中炮" },
        { "A30", "飞相(其他)对左过宫炮" },
        { "A31", "飞相进右马对左过宫炮(其他)" },
        { "A32", "飞相进右马对左过宫炮——红直车(其他)对黑进７卒" },
        { "A33", "飞相进右马对左过宫炮——红直车边炮对黑进７卒" },
        { "A34", "飞相进右马对左过宫炮——互进七兵" },
        { "A35", "飞相对右过宫炮" },
        { "A36", "飞相(其他)对进７卒" },
        { "A37", "飞相进左马对进７卒" },
        { "A38", "飞相互进七兵局" },
        { "A39", "飞相对进３卒" },
        { "A40", "起马局(对其他)" },
        { "A41", "起马(其他)对进７卒" },
        { "A42", "起马转边炮对进７卒" },
        { "A43", "起马转仕角炮对进７卒" },
        { "A44", "起马转中炮对进７卒" },
        { "A45", "起马互进七兵局" },
        { "A46", "" },
        { "A47", "" },
        { "A48", "" },
        { "A49", "" },
        { "A50", "仕角炮局(对其他)" },
        { "A51", "仕角炮对进左马" },
        { "A52", "仕角炮(其他)对右中炮" },
        { "A53", "仕角炮转反宫马对右中炮" },
        { "A54", "仕角炮对进７卒" },
        { "A55", "" },
        { "A56", "" },
        { "A57", "" },
        { "A58", "" },
        { "A59", "" },
        { "A60", "过宫炮局(对其他)" },
        { "A61", "过宫炮对进左马" },
        { "A62", "过宫炮对横车" },
        { "A63", "过宫炮(其他)对左中炮" },
        { "A64", "过宫炮直车对左中炮(其他)" },
        { "A65", "过宫炮直车对左中炮横车" },
        { "A66", "" },
        { "A67", "" },
        { "A68", "" },
        { "A69", "" },
        { "A70", "" },
        { "A71", "" },
        { "A72", "" },
        { "A73", "" },
        { "A74", "" },
        { "A75", "" },
        { "A76", "" },
        { "A77", "" },
        { "A78", "" },
        { "A79", "" },
        { "A80", "" },
        { "A81", "" },
        { "A82", "" },
        { "A83", "" },
        { "A84", "" },
        { "A85", "" },
        { "A86", "" },
        { "A87", "" },
        { "A88", "" },
        { "A89", "" },
        { "A90", "" },
        { "A91", "" },
        { "A92", "" },
        { "A93", "" },
        { "A94", "" },
        { "A95", "" },
        { "A96", "" },
        { "A97", "" },
        { "A98", "" },
        { "A99", "" },
        { "B00", "中炮局(对其他)" },
        { "B01", "中炮对进右马(其他)" },
        { "B02", "中炮对进右马先上士" },
        { "B03", "中炮对鸳鸯炮" },
        { "B04", "中炮对右三步虎" },
        { "B05", "中炮对进左马(其他)" },
        { "B06", "中炮对龟背炮" },
        { "B07", "中炮对左炮封车(不包括转列炮)" },
        { "B08", "" },
        { "B09", "" },
        { "B10", "中炮对单提马(其他)" },
        { "B11", "中炮对士角炮转单提马" },
        { "B12", "中炮(其他)对单提马横车" },
        { "B13", "中炮巡河炮对单提马横车" },
        { "B14", "中炮进七兵对单提马横车" },
        { "B15", "" },
        { "B16", "" },
        { "B17", "" },
        { "B18", "" },
        { "B19", "" },
        { "B20", "中炮(其他)对左三步虎" },
        { "B21", "中炮边相对左三步虎骑河车" },
        { "B22", "中炮右横车对左三步虎" },
        { "B23", "中炮巡河炮对左三步虎" },
        { "B24", "中炮过河炮对左三步虎" },
        { "B25", "中炮两头蛇对左三步虎" },
        { "B26", "" },
        { "B27", "" },
        { "B28", "" },
        { "B29", "" },
        { "B30", "中炮对反宫马后补左马" },
        { "B31", "中炮(其他)对反宫马" },
        { "B32", "中炮急进左马对反宫马" },
        { "B33", "中炮过河车对反宫马" },
        { "B34", "中炮右横车对反宫马" },
        { "B35", "中炮巡河炮对反宫马" },
        { "B36", "五八炮对反宫马" },
        { "B37", "" },
        { "B38", "" },
        { "B39", "" },
        { "B40", "五六炮缓开车对反宫马" },
        { "B41", "五六炮左正马对反宫马(其他)" },
        { "B42", "五六炮左正马对反宫马——黑右直车(其他)" },
        { "B43", "五六炮左正马对反宫马——黑右直车边炮(其他)" },
        { "B44", "五六炮左正马对反宫马——黑右直车边炮进７卒" },
        { "B45", "五六炮左边马对反宫马" },
        { "B46", "" },
        { "B47", "" },
        { "B48", "" },
        { "B49", "" },
        { "B50", "五七炮对反宫马(其他)" },
        { "B51", "五七炮对反宫马左直车" },
        { "B52", "五七炮对反宫马左横车" },
        { "B53", "五七炮对反宫马右直车" },
        { "B54", "五七炮互进三兵对反宫马(其他)" },
        { "B55", "五七炮互进三兵对反宫马——(红其他对)黑右炮过河" },
        { "B56", "五七炮互进三兵对反宫马——红弃双兵对黑右炮过河" },
        { "B57", "" },
        { "B58", "" },
        { "B59", "" },
        { "B60", "" },
        { "B61", "" },
        { "B62", "" },
        { "B63", "" },
        { "B64", "" },
        { "B65", "" },
        { "B66", "" },
        { "B67", "" },
        { "B68", "" },
        { "B69", "" },
        { "B70", "" },
        { "B71", "" },
        { "B72", "" },
        { "B73", "" },
        { "B74", "" },
        { "B75", "" },
        { "B76", "" },
        { "B77", "" },
        { "B78", "" },
        { "B79", "" },
        { "B80", "" },
        { "B81", "" },
        { "B82", "" },
        { "B83", "" },
        { "B84", "" },
        { "B85", "" },
        { "B86", "" },
        { "B87", "" },
        { "B88", "" },
        { "B89", "" },
        { "B90", "" },
        { "B91", "" },
        { "B92", "" },
        { "B93", "" },
        { "B94", "" },
        { "B95", "" },
        { "B96", "" },
        { "B97", "" },
        { "B98", "" },
        { "B99", "" },
        { "C00", "中炮(其他)对屏风马" },
        { "C01", "中炮七路马(其他)对屏风马" },
        { "C02", "中炮七路马对屏风马——红左马盘河" },
        { "C03", "中炮七路马对屏风马——红进中兵(对黑其他)" },
        { "C04", "中炮七路马对屏风马——红进中兵对黑双炮过河" },
        { "C05", "中炮左边马(其他)对屏风马" },
        { "C06", "中炮左边马对屏风马——红左横车" },
        { "C07", "" },
        { "C08", "" },
        { "C09", "" },
        { "C10", "中炮右横车(其他)对屏风马" },
        { "C11", "中炮右横车对屏风马——红左马盘河" },
        { "C12", "中炮右横车对屏风马——红巡河炮" },
        { "C13", "中炮右横车对屏风马——红边炮" },
        { "C14", "中炮右横车对屏风马——红进中兵" },
        { "C15", "中炮巡河车对屏风马——红不进左马" },
        { "C16", "中炮巡河车对屏风马——红进左马" },
        { "C17", "中炮过河车(其他)对屏风马" },
        { "C18", "中炮过河车七路马对屏风马(不包括两头蛇)" },
        { "C19", "中炮过河车左边马对屏风马" },
        { "C20", "中炮过河车七路马(其他)对屏风马两头蛇" },
        { "C21", "中炮过河车七路马对屏风马两头蛇——红左横车(对黑其他)" },
        { "C22", "中炮过河车七路马对屏风马两头蛇——红左横车(其他)对黑高右炮" },
        { "C23", "中炮过河车七路马对屏风马两头蛇——红左横车兑三兵对黑高右炮" },
        { "C24", "中炮过河车七路马对屏风马两头蛇——红左横车兑七兵对黑高右炮" },
        { "C25", "中炮过河车七路马对屏风马两头蛇——红左横车兑双兵对黑高右炮" },
        { "C26", "" },
        { "C27", "" },
        { "C28", "" },
        { "C29", "" },
        { "C30", "中炮过河车互进七兵对屏风马(其他)" },
        { "C31", "中炮过河车互进七兵对屏风马上士" },
        { "C32", "中炮过河车互进七兵对屏风马飞象" },
        { "C33", "中炮过河车互进七兵对屏风马右横车" },
        { "C34", "中炮过河车互进七兵对屏风马右炮过河" },
        { "C35", "中炮过河车互进七兵(其他)对屏风马左马盘河" },
        { "C36", "中炮过河车互进七兵对屏风马左马盘河——红七路马(对黑其他)" },
        { "C37", "中炮过河车互进七兵对屏风马左马盘河——红七路马(其他)对黑飞右象" },
        { "C38", "中炮过河车互进七兵对屏风马左马盘河——红七路马高左炮对黑飞右象" },
        { "C39", "中炮过河车互进七兵对屏风马左马盘河——红边炮对黑飞右象" },
        { "C40", "中炮过河车互进七兵对屏风马平炮兑车(其他)" },
        { "C41", "中炮过河车互进七兵对屏风马平炮兑车——(红其他对)黑退边炮" },
        { "C42", "中炮过河车互进七兵对屏风马平炮兑车——红七路马对黑退边炮(其他)" },
        { "C43", "中炮过河车互进七兵对屏风马平炮兑车——红七路马(其他)对黑退边炮上右士" },
        { "C44", "中炮过河车互进七兵对屏风马平炮兑车——红左马盘河对黑退边炮上右士" },
        { "C45", "中炮过河车互进七兵对屏风马平炮兑车——红左边炮对黑退边炮上右士(其他)" },
        { "C46", "中炮过河车互进七兵对屏风马平炮兑车——红左边炮对黑退边炮上右士右直车" },
        { "C47", "中炮过河车互进七兵对屏风马平炮兑车——红左边马对黑退边炮" },
        { "C48", "中炮过河车互进七兵对屏风马平炮兑车——红仕角炮对黑退边炮" },
        { "C49", "中炮过河车互进七兵对屏风马平炮兑车——红进中兵对黑退边炮" },
        { "C50", "五六炮(其他)对屏风马" },
        { "C51", "五六炮左边马对屏风马(其他)" },
        { "C52", "五六炮左边马对屏风马——黑进７卒右直车(其他)" },
        { "C53", "五六炮左边马对屏风马——黑进７卒右直车右炮过河" },
        { "C54", "五六炮过河车对屏风马(其他)" },
        { "C55", "五六炮过河车对屏风马——黑进７卒右直车" },
        { "C56", "五六炮过河车对屏风马——黑两头蛇" },
        { "C57", "" },
        { "C58", "" },
        { "C59", "" },
        { "C60", "五七炮对屏风马(其他)" },
        { "C61", "五七炮对屏风马进７卒(其他)" },
        { "C62", "五七炮对屏风马进７卒——(红其他对)黑右直车" },
        { "C63", "五七炮对屏风马进７卒——红左直车对黑右直车(其他)" },
        { "C64", "五七炮对屏风马进７卒——红左直车对黑右直车左炮过河" },
        { "C65", "五七炮对屏风马进７卒——红左直车对黑右直车右炮巡河" },
        { "C66", "五七炮对屏风马进７卒——红左直车对黑右直车右炮过河" },
        { "C67", "五七炮对屏风马进７卒——黑右炮巡河" },
        { "C68", "五七炮互进七兵对屏风马" },
        { "C69", "" },
        { "C70", "五七炮对屏风马进３卒(其他)" },
        { "C71", "五七炮对屏风马进３卒右马外盘河" },
        { "C72", "五七炮互进三兵(其他)对屏风马边卒右马外盘河" },
        { "C73", "五七炮互进三兵对屏风马边卒右马外盘河——红左横车(对黑其他)" },
        { "C74", "五七炮互进三兵对屏风马边卒右马外盘河——红左横车(其他)对黑飞左象" },
        { "C75", "五七炮互进三兵对屏风马边卒右马外盘河——红左横车右马盘河对黑飞左象" },
        { "C76", "五七炮互进三兵对屏风马边卒右马外盘河——红左横车右车巡河对黑飞左象" },
        { "C77", "五七炮互进三兵对屏风马边卒右马外盘河——红左横车对黑飞右象" },
        { "C78", "五七炮互进三兵对屏风马边卒右马外盘河——红左横车对黑兑边卒" },
        { "C79", "" },
        { "C80", "中炮巡河炮对屏风马(其他)" },
        { "C81", "中炮巡河炮对屏风马——黑飞左象(其他)" },
        { "C82", "中炮巡河炮对屏风马——黑飞左象右横车" },
        { "C83", "中炮巡河炮对屏风马——黑飞左象左炮巡河" },
        { "C84", "中炮巡河炮对屏风马——黑飞右象" },
        { "C85", "中炮巡河炮对屏风马——红左马盘河(其他)对黑左马外盘河" },
        { "C86", "中炮巡河炮缓开车对屏风马左马外盘河——红右横车" },
        { "C87", "" },
        { "C88", "" },
        { "C89", "" },
        { "C90", "五八炮对屏风马(其他)" },
        { "C91", "五八炮互进三兵(其他)对屏风马" },
        { "C92", "五八炮互进三兵对屏风马——红左正马" },
        { "C93", "五八炮互进三兵对屏风马——红左边马(对黑其他)" },
        { "C94", "五八炮互进三兵对屏风马——红左边马对黑上士" },
        { "C95", "五八炮互进三兵对屏风马——红左边马对黑兑７卒" },
        { "C96", "五八炮互进三兵对屏风马——红左边马(其他)对黑边卒" },
        { "C97", "五八炮互进三兵对屏风马——红左边马平炮压马对黑边卒" },
        { "C98", "五八炮互进三兵对屏风马——红平炮压马" },
        { "C99", "五九炮对屏风马" },
        { "D00", "顺炮缓开车对其他" },
        { "D01", "顺炮缓开车对横车" },
        { "D02", "顺炮缓开车对直车" },
        { "D03", "顺炮横车对缓开车(包括顺炮顺横车)" },
        { "D04", "顺炮横车对直车(其他)" },
        { "D05", "顺炮横车对直车巡河" },
        { "D06", "" },
        { "D07", "" },
        { "D08", "" },
        { "D09", "" },
        { "D10", "顺炮直车对缓开车(其他)" },
        { "D11", "顺炮直车对缓开车——黑左横车" },
        { "D12", "顺炮直车对缓开车——黑右横车" },
        { "D13", "顺炮直车对缓开车——黑兑直车" },
        { "D14", "顺炮直车对缓开车——黑过河炮" },
        { "D15", "顺炮直车对缓开车——黑边炮" },
        { "D16", "" },
        { "D17", "" },
        { "D18", "" },
        { "D19", "" },
        { "D20", "顺炮直车(其他)对横车" },
        { "D21", "顺炮直车对横车——红先上仕" },
        { "D22", "顺炮直车对横车——红左边马" },
        { "D23", "顺炮直车对横车——红巡河车" },
        { "D24", "顺炮直车对横车——红过河车" },
        { "D25", "顺炮直车对横车——红仕角炮" },
        { "D26", "顺炮直车对横车——红进三兵(不包括两头蛇)" },
        { "D27", "顺炮直车对横车——红进七兵(不包括两头蛇)" },
        { "D28", "顺炮直车对横车——红两头蛇" },
        { "D29", "顺炮直车对横车——红两头蛇对黑双横车" },
        { "D30", "中炮不进三兵对左炮封车转列炮" },
        { "D31", "中炮进三兵(其他)对左炮封车转列炮" },
        { "D32", "中炮进三兵对左炮封车转列炮——红右马盘河" },
        { "D33", "中炮进三兵对左炮封车转列炮——红七路马" },
        { "D34", "中炮进三兵对左炮封车转列炮——红左边马" },
        { "D35", "中炮进三兵对左炮封车转列炮——红进炮打马" },
        { "D36", "中炮进三兵对左炮封车转列炮——红两头蛇" },
        { "D37", "" },
        { "D38", "" },
        { "D39", "" },
        { "D40", "中炮(其他)对左三步虎转列炮" },
        { "D41", "中炮进中兵对左三步虎骑河车转列炮" },
        { "D42", "中炮对左三步虎转列炮——红左直车" },
        { "D43", "中炮对左三步虎转列炮——红两头蛇" },
        { "D44", "" },
        { "D45", "" },
        { "D46", "" },
        { "D47", "" },
        { "D48", "" },
        { "D49", "" },
        { "D50", "中炮对列炮" },
        { "D51", "中炮(其他)对后补列炮" },
        { "D52", "中炮右直车(其他)对后补列炮" },
        { "D53", "中炮过河车对后补列炮" },
        { "D54", "中炮左直车对后补列炮" },
        { "D55", "中炮双直车对后补列炮" },
        { "D56", "" },
        { "D57", "" },
        { "D58", "" },
        { "D59", "" },
        { "D60", "" },
        { "D61", "" },
        { "D62", "" },
        { "D63", "" },
        { "D64", "" },
        { "D65", "" },
        { "D66", "" },
        { "D67", "" },
        { "D68", "" },
        { "D69", "" },
        { "D70", "" },
        { "D71", "" },
        { "D72", "" },
        { "D73", "" },
        { "D74", "" },
        { "D75", "" },
        { "D76", "" },
        { "D77", "" },
        { "D78", "" },
        { "D79", "" },
        { "D80", "" },
        { "D81", "" },
        { "D82", "" },
        { "D83", "" },
        { "D84", "" },
        { "D85", "" },
        { "D86", "" },
        { "D87", "" },
        { "D88", "" },
        { "D89", "" },
        { "D90", "" },
        { "D91", "" },
        { "D92", "" },
        { "D93", "" },
        { "D94", "" },
        { "D95", "" },
        { "D96", "" },
        { "D97", "" },
        { "D98", "" },
        { "D99", "" },
        { "E00", "仙人指路局(对其他)" },
        { "E01", "仙人指路(其他)对飞象" },
        { "E02", "仙人指路(进右马)对飞象" },
        { "E03", "仙人指路对中炮" },
        { "E04", "仙人指路对仕角炮或过宫炮" },
        { "E05", "仙人指路对金钩炮" },
        { "E06", "仙人指路(其他)对进右马" },
        { "E07", "仙人指路互进右马局(不包括转对兵互进右马局)" },
        { "E08", "两头蛇对进右马(其他)" },
        { "E09", "两头蛇对进右马转卒底炮" },
        { "E10", "仙人指路(其他)对卒底炮" },
        { "E11", "仙人指路飞相对卒底炮" },
        { "E12", "仙人指路转右中炮对卒底炮" },
        { "E13", "仙人指路转左中炮对卒底炮(其他)" },
        { "E14", "仙人指路转左中炮(其他)对卒底炮飞右象" },
        { "E15", "仙人指路转左中炮对卒底炮飞右象——红右边马(对黑其他)" },
        { "E16", "仙人指路转左中炮对卒底炮飞右象——互进边马" },
        { "E17", "仙人指路转左中炮对卒底炮转顺炮" },
        { "E18", "" },
        { "E19", "" },
        { "E20", "仙人指路转左中炮(其他)对卒底炮飞左象" },
        { "E21", "仙人指路转左中炮对卒底炮飞左象——红先上仕" },
        { "E22", "仙人指路转左中炮对卒底炮飞左象——红进左马(对黑其他)" },
        { "E23", "仙人指路转左中炮对卒底炮飞左象——红进左马(其他)对黑右横车" },
        { "E24", "仙人指路转左中炮对卒底炮飞左象——红左直车(其他)对黑右横车" },
        { "E25", "仙人指路转左中炮对卒底炮飞左象——红左直车右马盘河对黑右横车上士" },
        { "E26", "仙人指路转左中炮对卒底炮飞左象——红左直车右马盘河对黑右横车过河" },
        { "E27", "仙人指路转左中炮对卒底炮飞左象——红左直车右马盘河对黑右横车边卒" },
        { "E28", "" },
        { "E29", "" },
        { "E30", "仙人指路转左中炮对卒底炮飞左象——黑进７卒(其他)" },
        { "E31", "仙人指路转左中炮对卒底炮飞左象——(红其他对)黑连进７卒" },
        { "E32", "仙人指路转左中炮对卒底炮飞左象——红左直车右边马对黑连进７卒拐角马" },
        { "E33", "仙人指路转左中炮对卒底炮飞左象——红左直车右边马(其他)对黑连进７卒右横车" },
        { "E34", "仙人指路转左中炮对卒底炮飞左象——红左直车右边马上仕对黑连进７卒右横车" },
        { "E35", "仙人指路转左中炮对卒底炮飞左象——红巡河车右边马对黑连进７卒右横车" },
        { "E36", "仙人指路转左中炮对卒底炮飞左象——红双直车右边马对黑连进７卒右横车" },
        { "E37", "仙人指路转左中炮对卒底炮飞左象——红右边马" },
        { "E38", "仙人指路转左中炮对卒底炮飞左象——红炮打中卒" },
        { "E39", "" },
        { "E40", "对兵局(转其他)" },
        { "E41", "对兵进右马局(对其他)" },
        { "E42", "对兵互进右马局(红走其他)" },
        { "E43", "对兵对进右马局——红飞相" },
        { "E44", "对兵对进右马局——红横车" },
        { "E45", "对兵对进右马局——红边炮" },
        { "E46", "对兵转兵底炮(对其他)" },
        { "E47", "对兵转兵底炮对右中炮" },
        { "E48", "对兵转兵底炮对左中炮" },
        { "E49", "" },
        { "E50", "" },
        { "E51", "" },
        { "E52", "" },
        { "E53", "" },
        { "E54", "" },
        { "E55", "" },
        { "E56", "" },
        { "E57", "" },
        { "E58", "" },
        { "E59", "" },
        { "E60", "" },
        { "E61", "" },
        { "E62", "" },
        { "E63", "" },
        { "E64", "" },
        { "E65", "" },
        { "E66", "" },
        { "E67", "" },
        { "E68", "" },
        { "E69", "" },
        { "E70", "" },
        { "E71", "" },
        { "E72", "" },
        { "E73", "" },
        { "E74", "" },
        { "E75", "" },
        { "E76", "" },
        { "E77", "" },
        { "E78", "" },
        { "E79", "" },
        { "E80", "" },
        { "E81", "" },
        { "E82", "" },
        { "E83", "" },
        { "E84", "" },
        { "E85", "" },
        { "E86", "" },
        { "E87", "" },
        { "E88", "" },
        { "E89", "" },
        { "E90", "" },
        { "E91", "" },
        { "E92", "" },
        { "E93", "" },
        { "E94", "" },
        { "E95", "" },
        { "E96", "" },
        { "E97", "" },
        { "E98", "" },
        { "E99", "" }
    };

    char createTableSql[WIDEWCHARSIZE];
    sprintf(createTableSql, "CREATE TABLE %s("
                            "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                            "SN TEXT NOT NULL,"
                            "NAME TEXT NOT NULL);",
        tblName);

    char* insertFormat = "INSERT INTO %s (SN, NAME) "
                         "VALUES ('%s', '%s' );";
    char insertValuesSql[5 * SUPERWIDEWCHARSIZE], tempSql[WCHARSIZE];
    insertValuesSql[0] = '\x0';
    for (int i = 0; i < sizeof(sn_names) / sizeof(sn_names[0]); i++) {
        sprintf(tempSql, insertFormat, tblName, sn_names[i][0], sn_names[i][1]);
        strcat(insertValuesSql, tempSql);
    }

    initTable__(db, tblName, createTableSql, insertValuesSql);
}

//*
// 读取三级编号开局所需的插入字符串
void readSN_Name__(void)
{
    FILE *fin = fopen("chessManual/ecco.sn_name", "r"),
         *fout = fopen("chessManual/sn_names", "w");
    wchar_t *wstr = getWString(fin), resultWstr[5 * SUPERWIDEWCHARSIZE] = { 0 }, lineStr[WCHARSIZE];

    const char* error;
    int erroffset = 0;
    void* snReg[] = {
        pcrewch_compile(L"([A-Z])．([\\S]+)\\(共", 0, &error, &erroffset, NULL),
        pcrewch_compile(L"([A-Z][\\d]{1})．([\\S]+)\n", 0, &error, &erroffset, NULL),
        pcrewch_compile(L"([A-Z][\\d]{2})．([\\S]+)\n", 0, &error, &erroffset, NULL),
    };
    //void* snReg = pcrewch_compile(L"([A-Z][\\d]{2})．([\\S]{2,})\n", 0, &error, &erroffset, NULL);
    for (int i = 0; i < sizeof(snReg) / sizeof(snReg[0]); ++i) {
        assert(snReg[i]);
        int ovector[10] = { 0 }, regCount, length;
        wchar_t* tempWstr = wstr;
        while ((tempWstr += ovector[1]) && (length = wcslen(tempWstr)) > 0) {
            regCount = pcrewch_exec(snReg[i], NULL, tempWstr, length, 0, 0, ovector, 10);
            if (regCount <= 0)
                break;

            wchar_t sn[5] = { 0 }, name[WCHARSIZE] = { 0 };
            wcsncpy(sn, tempWstr + ovector[2], ovector[3] - ovector[2]);
            if (wcsstr(resultWstr, sn) != NULL)
                continue;
            wcsncpy(name, tempWstr + ovector[4], ovector[5] - ovector[4]);
            if (wcsstr(name, L"空") != NULL)
                wcscpy(name, L"");

            swprintf(lineStr, WCHARSIZE, L"{ \"%ls\", \"%ls\" },\n", sn, name);
            //fwprintf(fout, lineStr);

            wcscat(resultWstr, lineStr);
        }
        pcrewch_free(snReg[i]);
    }

    //fwprintf(fout, wstr);
    resultWstr[wcslen(resultWstr) - 2] = L'\x0';
    fwprintf(fout, resultWstr);
    free(wstr);

    fclose(fout);
    fclose(fin);
}
//*/

void eccoInit(char* dbName)
{
    sqlite3* db = NULL;
    int rc = sqlite3_open(dbName, &db);
    if (rc) {
        fprintf(stderr, "\nCan't open database: %s\n", sqlite3_errmsg(db));
    } else {
        //fprintf(stderr, "\nOpened database successfully. \n");

        readSN_Name__();

        initSN_Name__(db);
    }

    sqlite3_close(db);
}