#include "head/unitTest.h"
#include "CUnit/Basic.h"
#include "CUnit/CUnit.h"
#include "head/aspect.h"
#include "head/base.h"
#include "head/board.h"
#include "head/chessManual.h"
#include "head/md5.h"
#include "head/move.h"
#include "head/piece.h"
#include "head/sha1.h"
#include "head/tools.h"
#include <time.h>
//#include "head/console.h"

extern const char* EXTNAMES[];
static char* xqfFileName__ = "chessManual/01.xqf";

static const char* dirNames__[] = {
    "chessManual/示例文件",
    "chessManual/象棋杀着大全",
    "chessManual/疑难文件",
    "chessManual/中国象棋棋谱大全"
};

static int dirSize__ = sizeof(dirNames__) / sizeof(dirNames__[0]);
static int dirNum__ = 2; // 测试目录个数

static void test_md5(void)
{
    unsigned char str[] = "admin", str1[] = "21232f297a57a5a743894a0e4a801fc3";
    //unsigned char str[] = "陈建平", str1[] = "21bb705f61cb371b96a9f0c48ec68896"; // 未成功！

    unsigned char md5[MD5HashSize];
    ustrToMD5(md5, str);
    char str2[33];
    hashToStr(str2, md5, MD5HashSize);

    //printf("\n%s\n%s\n", str1, str2);
    CU_ASSERT_STRING_EQUAL(str1, str2);
    //testMD5_1();
    //testMD5_2("s");
}

static void test_sha1(void)
{
    unsigned char str[] = "admin", str1[] = "d033e22ae348aeb5660fc2140aec35850c4da997";
    //unsigned char str[] = "陈建平", str1[] = "2b86ed62ae08865d16c6d4f86c5b79f695e6c723"; // 未成功！

    unsigned char sha1[SHA1HashSize];
    ustrToSHA1(sha1, str);
    char str2[41];
    hashToStr(str2, sha1, SHA1HashSize);

    //printf("\n%s\n%s\n", str1, str2);
    CU_ASSERT_STRING_EQUAL(str1, str2);
    //testsha1();
}

static void test_fileInfos(void)
{
    FILE* fout = fopen("chessManual/fnames", "w");
    char dirName[FILENAME_MAX];

    for (int dir = 0; dir < dirSize__ && dir < dirNum__; ++dir) {
        // 调节控制转换目录  XQF, BIN, JSON, PGN_ICCS, PGN_ZH, PGN_CC
        for (RecFormat fromFmt = XQF; fromFmt <= PGN_CC; ++fromFmt) {
            sprintf(dirName, "%s%s", dirNames__[dir], EXTNAMES[fromFmt]);
            writeFileInfos(fout, dirName);
        }
    }
    fclose(fout);
}

static CU_TestInfo tests_tools[] = {
    { "test_md5", test_md5 },
    { "test_sha1", test_sha1 },
    { "test_fileInfos", test_fileInfos },
    CU_TEST_INFO_NULL,
};

static void test_piece_str(void)
{
    char str1[] = "红帅K@FF 红仕A@FF 红仕A@FF 红相B@FF 红相B@FF 红马N@FF 红马N@FF 红车R@FF 红车R@FF 红炮C@FF 红炮C@FF 红兵P@FF 红兵P@FF 红兵P@FF 红兵P@FF 红兵P@FF "
                  "黑将k@FF 黑士a@FF 黑士a@FF 黑象b@FF 黑象b@FF 黑馬n@FF 黑馬n@FF 黑車r@FF 黑車r@FF 黑砲c@FF 黑砲c@FF 黑卒p@FF 黑卒p@FF 黑卒p@FF 黑卒p@FF 黑卒p@FF ",
         str2[WIDEWCHARSIZE];
    wchar_t wstr[WIDEWCHARSIZE];
    testPieceString(wstr);
    wcstombs(str2, wstr, WIDEWCHARSIZE);

    //printf("\n%s\n%s\n", str1, str2);
    CU_ASSERT_STRING_EQUAL(str1, str2);
}

static CU_TestInfo tests_piece[] = {
    { "test_piece_str", test_piece_str },
    CU_TEST_INFO_NULL,
};

static wchar_t* FENs[] = {
    L"rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR",
    L"5a3/4ak2r/6R2/8p/9/9/9/B4N2B/4K4/3c5",
    L"2b1kab2/4a4/4c4/9/9/3R5/9/1C7/4r4/2BK2B2",
    L"4kab2/4a4/4b4/3N5/9/4N4/4n4/4B4/4A4/3AK1B2"
};

static void setBoard__(Board board, wchar_t* FEN)
{
    wchar_t pieChars[SEATNUM + 1];
    getPieChars_FEN(pieChars, FEN);
    setBoard_pieChars(board, pieChars);
}

static void test_board_FEN_str(void)
{
    wchar_t pieChars[SEATNUM + 1], pieChars2[SEATNUM + 1], FEN[SEATNUM + 1];
    char str1[SEATNUM + 1], str2[SEATNUM + 1], str3[SEATNUM + 1], resultStr[SEATNUM + 1];
    Board board = newBoard(), board1 = newBoard();
    for (int i = 0; i < sizeof(FENs) / sizeof(FENs[0]); ++i) {
        setBoard__(board, FENs[i]);
        setBoard__(board1, FENs[i]);
        // board对象相同比较
        CU_ASSERT_TRUE(board_equal(board, board1));

        getPieChars_FEN(pieChars, FENs[i]);
        wcstombs(str1, pieChars, WIDEWCHARSIZE);
        getPieChars_board(pieChars2, board);
        wcstombs(str2, pieChars2, WIDEWCHARSIZE);
        CU_ASSERT_STRING_EQUAL(str1, str2);

        // PieChars转换成FEN
        wcstombs(str3, FENs[i], WIDEWCHARSIZE);
        getFEN_pieChars(FEN, pieChars);
        wcstombs(resultStr, FEN, WIDEWCHARSIZE);
        CU_ASSERT_STRING_EQUAL(str3, resultStr);
    }
    delBoard(board);
    delBoard(board1);
}

static void getBoardStr__(char* str, Board board)
{
    wchar_t preStr[WCHARSIZE], boardStr[WIDEWCHARSIZE], sufStr[WCHARSIZE], seatStr[WCHARSIZE];
    wchar_t wstr[SUPERWIDEWCHARSIZE];
    swprintf(wstr, sizeof(wstr), L"%ls%ls%ls%ls\n",
        getBoardPreString(preStr, board),
        getBoardString(boardStr, board),
        getBoardSufString(sufStr, board),
        getSeatString(seatStr, getKingSeat(board, RED)));
    wcstombs(str, wstr, sizeof(wstr));
}

static void test_board_Txt_str(void)
{
    char *str1[] = {
        "　　　　　　　黑　方　　　　　　　\n"
        "１　２　３　４　５　６　７　８　９\n"
        "車━馬━象━士━将━士━象━馬━車\n"
        "┃　│　│　│╲│╱│　│　│　┃\n"
        "┠─┼─┼─┼─╳─┼─┼─┼─┨\n"
        "┃　│　│　│╱│╲│　│　│　┃\n"
        "┠─砲─┼─┼─┼─┼─┼─砲─┨\n"
        "┃　│　│　│　│　│　│　│　┃\n"
        "卒─┼─卒─┼─卒─┼─卒─┼─卒\n"
        "┃　│　│　│　│　│　│　│　┃\n"
        "┠─┴─┴─┴─┴─┴─┴─┴─┨\n"
        "┃　　　　　　　　　　　　　　　┃\n"
        "┠─┬─┬─┬─┬─┬─┬─┬─┨\n"
        "┃　│　│　│　│　│　│　│　┃\n"
        "兵─┼─兵─┼─兵─┼─兵─┼─兵\n"
        "┃　│　│　│　│　│　│　│　┃\n"
        "┠─炮─┼─┼─┼─┼─┼─炮─┨\n"
        "┃　│　│　│╲│╱│　│　│　┃\n"
        "┠─┼─┼─┼─╳─┼─┼─┼─┨\n"
        "┃　│　│　│╱│╲│　│　│　┃\n"
        "车━马━相━仕━帅━仕━相━马━车\n"
        "九　八　七　六　五　四　三　二　一\n"
        "　　　　　　　红　方　　　　　　　\n"
        "04红帅K@04\n",
        "　　　　　　　黑　方　　　　　　　\n"
        "１　２　３　４　５　６　７　８　９\n"
        "┏━┯━┯━┯━┯━士━┯━┯━┓\n"
        "┃　│　│　│╲│╱│　│　│　┃\n"
        "┠─┼─┼─┼─士─将─┼─┼─車\n"
        "┃　│　│　│╱│╲│　│　│　┃\n"
        "┠─╬─┼─┼─┼─┼─车─╬─┨\n"
        "┃　│　│　│　│　│　│　│　┃\n"
        "┠─┼─╬─┼─╬─┼─╬─┼─卒\n"
        "┃　│　│　│　│　│　│　│　┃\n"
        "┠─┴─┴─┴─┴─┴─┴─┴─┨\n"
        "┃　　　　　　　　　　　　　　　┃\n"
        "┠─┬─┬─┬─┬─┬─┬─┬─┨\n"
        "┃　│　│　│　│　│　│　│　┃\n"
        "┠─┼─╬─┼─╬─┼─╬─┼─┨\n"
        "┃　│　│　│　│　│　│　│　┃\n"
        "相─╬─┼─┼─┼─马─┼─╬─相\n"
        "┃　│　│　│╲│╱│　│　│　┃\n"
        "┠─┼─┼─┼─帅─┼─┼─┼─┨\n"
        "┃　│　│　│╱│╲│　│　│　┃\n"
        "┗━┷━┷━砲━┷━┷━┷━┷━┛\n"
        "九　八　七　六　五　四　三　二　一\n"
        "　　　　　　　红　方　　　　　　　\n"
        "14红帅K@14\n",
        "　　　　　　　黑　方　　　　　　　\n"
        "１　２　３　４　５　６　７　８　９\n"
        "┏━┯━象━┯━将━士━象━┯━┓\n"
        "┃　│　│　│╲│╱│　│　│　┃\n"
        "┠─┼─┼─┼─士─┼─┼─┼─┨\n"
        "┃　│　│　│╱│╲│　│　│　┃\n"
        "┠─╬─┼─┼─砲─┼─┼─╬─┨\n"
        "┃　│　│　│　│　│　│　│　┃\n"
        "┠─┼─╬─┼─╬─┼─╬─┼─┨\n"
        "┃　│　│　│　│　│　│　│　┃\n"
        "┠─┴─┴─┴─┴─┴─┴─┴─┨\n"
        "┃　　　　　　　　　　　　　　　┃\n"
        "┠─┬─┬─车─┬─┬─┬─┬─┨\n"
        "┃　│　│　│　│　│　│　│　┃\n"
        "┠─┼─╬─┼─╬─┼─╬─┼─┨\n"
        "┃　│　│　│　│　│　│　│　┃\n"
        "┠─炮─┼─┼─┼─┼─┼─╬─┨\n"
        "┃　│　│　│╲│╱│　│　│　┃\n"
        "┠─┼─┼─┼─車─┼─┼─┼─┨\n"
        "┃　│　│　│╱│╲│　│　│　┃\n"
        "┗━┷━相━帅━┷━┷━相━┷━┛\n"
        "九　八　七　六　五　四　三　二　一\n"
        "　　　　　　　红　方　　　　　　　\n"
        "03红帅K@03\n",
        "　　　　　　　黑　方　　　　　　　\n"
        "１　２　３　４　５　６　７　８　９\n"
        "┏━┯━┯━┯━将━士━象━┯━┓\n"
        "┃　│　│　│╲│╱│　│　│　┃\n"
        "┠─┼─┼─┼─士─┼─┼─┼─┨\n"
        "┃　│　│　│╱│╲│　│　│　┃\n"
        "┠─╬─┼─┼─象─┼─┼─╬─┨\n"
        "┃　│　│　│　│　│　│　│　┃\n"
        "┠─┼─╬─马─╬─┼─╬─┼─┨\n"
        "┃　│　│　│　│　│　│　│　┃\n"
        "┠─┴─┴─┴─┴─┴─┴─┴─┨\n"
        "┃　　　　　　　　　　　　　　　┃\n"
        "┠─┬─┬─┬─马─┬─┬─┬─┨\n"
        "┃　│　│　│　│　│　│　│　┃\n"
        "┠─┼─╬─┼─馬─┼─╬─┼─┨\n"
        "┃　│　│　│　│　│　│　│　┃\n"
        "┠─╬─┼─┼─相─┼─┼─╬─┨\n"
        "┃　│　│　│╲│╱│　│　│　┃\n"
        "┠─┼─┼─┼─仕─┼─┼─┼─┨\n"
        "┃　│　│　│╱│╲│　│　│　┃\n"
        "┗━┷━┷━仕━帅━┷━相━┷━┛\n"
        "九　八　七　六　五　四　三　二　一\n"
        "　　　　　　　红　方　　　　　　　\n"
        "04红帅K@04\n"
    },
         str2[SUPERWIDEWCHARSIZE];
    Board board = newBoard();
    for (int i = 0; i < sizeof(FENs) / sizeof(FENs[0]); ++i) {
        setBoard__(board, FENs[i]);
        getBoardStr__(str2, board);

        CU_ASSERT_STRING_EQUAL(str1[i], str2);
    }
    delBoard(board);
}

static void test_board_change_str(void)
{
    char *str1[] = {
        "　　　　　　　红　方　　　　　　　\n"
        "一　二　三　四　五　六　七　八　九\n"
        "┏━┯━┯━┯━┯━仕━┯━┯━┓\n"
        "┃　│　│　│╲│╱│　│　│　┃\n"
        "┠─┼─┼─┼─仕─帅─┼─┼─车\n"
        "┃　│　│　│╱│╲│　│　│　┃\n"
        "┠─╬─┼─┼─┼─┼─車─╬─┨\n"
        "┃　│　│　│　│　│　│　│　┃\n"
        "┠─┼─╬─┼─╬─┼─╬─┼─兵\n"
        "┃　│　│　│　│　│　│　│　┃\n"
        "┠─┴─┴─┴─┴─┴─┴─┴─┨\n"
        "┃　　　　　　　　　　　　　　　┃\n"
        "┠─┬─┬─┬─┬─┬─┬─┬─┨\n"
        "┃　│　│　│　│　│　│　│　┃\n"
        "┠─┼─╬─┼─╬─┼─╬─┼─┨\n"
        "┃　│　│　│　│　│　│　│　┃\n"
        "象─╬─┼─┼─┼─馬─┼─╬─象\n"
        "┃　│　│　│╲│╱│　│　│　┃\n"
        "┠─┼─┼─┼─将─┼─┼─┼─┨\n"
        "┃　│　│　│╱│╲│　│　│　┃\n"
        "┗━┷━┷━炮━┷━┷━┷━┷━┛\n"
        "９　８　７　６　５　４　３　２　１\n"
        "　　　　　　　黑　方　　　　　　　\n"
        "85红帅K@85\n",
        "　　　　　　　黑　方　　　　　　　\n"
        "１　２　３　４　５　６　７　８　９\n"
        "┏━┯━┯━┯━┯━炮━┯━┯━┓\n"
        "┃　│　│　│╲│╱│　│　│　┃\n"
        "┠─┼─┼─┼─将─┼─┼─┼─┨\n"
        "┃　│　│　│╱│╲│　│　│　┃\n"
        "象─╬─┼─馬─┼─┼─┼─╬─象\n"
        "┃　│　│　│　│　│　│　│　┃\n"
        "┠─┼─╬─┼─╬─┼─╬─┼─┨\n"
        "┃　│　│　│　│　│　│　│　┃\n"
        "┠─┴─┴─┴─┴─┴─┴─┴─┨\n"
        "┃　　　　　　　　　　　　　　　┃\n"
        "┠─┬─┬─┬─┬─┬─┬─┬─┨\n"
        "┃　│　│　│　│　│　│　│　┃\n"
        "兵─┼─╬─┼─╬─┼─╬─┼─┨\n"
        "┃　│　│　│　│　│　│　│　┃\n"
        "┠─╬─車─┼─┼─┼─┼─╬─┨\n"
        "┃　│　│　│╲│╱│　│　│　┃\n"
        "车─┼─┼─帅─仕─┼─┼─┼─┨\n"
        "┃　│　│　│╱│╲│　│　│　┃\n"
        "┗━┷━┷━仕━┷━┷━┷━┷━┛\n"
        "九　八　七　六　五　四　三　二　一\n"
        "　　　　　　　红　方　　　　　　　\n"
        "13红帅K@13\n",
        "　　　　　　　黑　方　　　　　　　\n"
        "１　２　３　４　５　６　７　８　９\n"
        "┏━┯━┯━炮━┯━┯━┯━┯━┓\n"
        "┃　│　│　│╲│╱│　│　│　┃\n"
        "┠─┼─┼─┼─将─┼─┼─┼─┨\n"
        "┃　│　│　│╱│╲│　│　│　┃\n"
        "象─╬─┼─┼─┼─馬─┼─╬─象\n"
        "┃　│　│　│　│　│　│　│　┃\n"
        "┠─┼─╬─┼─╬─┼─╬─┼─┨\n"
        "┃　│　│　│　│　│　│　│　┃\n"
        "┠─┴─┴─┴─┴─┴─┴─┴─┨\n"
        "┃　　　　　　　　　　　　　　　┃\n"
        "┠─┬─┬─┬─┬─┬─┬─┬─┨\n"
        "┃　│　│　│　│　│　│　│　┃\n"
        "┠─┼─╬─┼─╬─┼─╬─┼─兵\n"
        "┃　│　│　│　│　│　│　│　┃\n"
        "┠─╬─┼─┼─┼─┼─車─╬─┨\n"
        "┃　│　│　│╲│╱│　│　│　┃\n"
        "┠─┼─┼─┼─仕─帅─┼─┼─车\n"
        "┃　│　│　│╱│╲│　│　│　┃\n"
        "┗━┷━┷━┷━┷━仕━┷━┷━┛\n"
        "九　八　七　六　五　四　三　二　一\n"
        "　　　　　　　红　方　　　　　　　\n"
        "15红帅K@15\n"
    },
         str2[SUPERWIDEWCHARSIZE];
    Board board = newBoard();
    setBoard__(board, FENs[1]); // 选择第2个FEN

    for (int ct = EXCHANGE; ct <= SYMMETRY; ++ct) {
        changeBoard(board, ct);
        getBoardStr__(str2, board);
        //if (strcmp(str1[ct], str2) != 0)
        //    printf("%s\n%s\n", str1[ct], str2);

        CU_ASSERT_STRING_EQUAL(str1[ct], str2);
    }
    delBoard(board);
}

static void appendWstr__(wchar_t* wstr, const wchar_t* format, wchar_t* tmpWstr)
{
    wchar_t tmpWstr1[WIDEWCHARSIZE];
    swprintf(tmpWstr1, WIDEWCHARSIZE, format, tmpWstr);
    wcscat(wstr, tmpWstr1);
}

static void test_board_putSeats_str(void)
{
    char str1[] = "红帅K@14=【03黑砲c@0304空05空13空14红帅K@1415空23空24空25红马N@25】9\n"
                  "红相B@20=【02空06空20红相B@2024空28红相B@2842空46空】7\n"
                  "红相B@28=【02空06空20红相B@2024空28红相B@2842空46空】7\n"
                  "红马N@25=【00空01空02空03黑砲c@0304空05空06空07空08空10空11空12空13空"
                  "14红帅K@1415空16空17空18空20红相B@2021空22空23空24空25红马N@2526空27空"
                  "28红相B@2830空31空32空33空34空35空36空37空38空40空41空42空43空44空45空"
                  "46空47空48空50空51空52空53空54空55空56空57空58空60空61空62空63空64空"
                  "65空66空67空68黑卒p@6870空71空72空73空74空75空76红车R@7677空78空80空"
                  "81空82空83空84黑士a@8485黑将k@8586空87空88黑車r@8890空91空92空93空94空"
                  "95黑士a@9596空97空98空】90\n"
                  "红车R@76=【00空01空02空03黑砲c@0304空05空06空07空08空10空11空12空13空"
                  "14红帅K@1415空16空17空18空20红相B@2021空22空23空24空25红马N@2526空27空"
                  "28红相B@2830空31空32空33空34空35空36空37空38空40空41空42空43空44空45空"
                  "46空47空48空50空51空52空53空54空55空56空57空58空60空61空62空63空64空"
                  "65空66空67空68黑卒p@6870空71空72空73空74空75空76红车R@7677空78空80空"
                  "81空82空83空84黑士a@8485黑将k@8586空87空88黑車r@8890空91空92空93空94空"
                  "95黑士a@9596空97空98空】90\n"
                  "黑将k@85=【03黑砲c@0304空05空13空14红帅K@1415空23空24空25红马N@25】9\n"
                  "黑士a@95=【03黑砲c@0305空14红帅K@1423空25红马N@25】5\n"
                  "黑士a@84=【03黑砲c@0305空14红帅K@1423空25红马N@25】5\n"
                  "黑車r@88=【00空01空02空03黑砲c@0304空05空06空07空08空10空11空12空13空"
                  "14红帅K@1415空16空17空18空20红相B@2021空22空23空24空25红马N@2526空27空"
                  "28红相B@2830空31空32空33空34空35空36空37空38空40空41空42空43空44空45空"
                  "46空47空48空50空51空52空53空54空55空56空57空58空60空61空62空63空64空"
                  "65空66空67空68黑卒p@6870空71空72空73空74空75空76红车R@7677空78空80空"
                  "81空82空83空84黑士a@8485黑将k@8586空87空88黑車r@8890空91空92空93空94空"
                  "95黑士a@9596空97空98空】90\n"
                  "黑砲c@03=【00空01空02空03黑砲c@0304空05空06空07空08空10空11空12空13空"
                  "14红帅K@1415空16空17空18空20红相B@2021空22空23空24空25红马N@2526空27空"
                  "28红相B@2830空31空32空33空34空35空36空37空38空40空41空42空43空44空45空"
                  "46空47空48空50空51空52空53空54空55空56空57空58空60空61空62空63空64空"
                  "65空66空67空68黑卒p@6870空71空72空73空74空75空76红车R@7677空78空80空"
                  "81空82空83空84黑士a@8485黑将k@8586空87空88黑車r@8890空91空92空93空94空"
                  "95黑士a@9596空97空98空】90\n"
                  "黑卒p@68=【30空32空34空36空38空40空42空44空46空48空50空51空52空53空"
                  "54空55空56空57空58空60空61空62空63空64空65空66空67空68黑卒p@6870空71空"
                  "72空73空74空75空76红车R@7677空78空80空81空82空83空84黑士a@8485黑将k@85"
                  "86空87空88黑車r@8890空91空92空93空94空95黑士a@9596空97空98空】55\n",
         str2[SUPERWIDEWCHARSIZE];
    Board board = newBoard();
    setBoard__(board, FENs[1]); // 选择第2个FEN

    //* 取得各棋子的可放置位置
    wchar_t wstr[SUPERWIDEWCHARSIZE], tmpWstr[WIDEWCHARSIZE];
    wstr[0] = L'\x0';
    for (int color = RED; color <= BLACK; ++color) {
        Seat pseats[SIDEPIECENUM];
        int pcount = getLiveSeats_bc(pseats, board, color);
        for (int i = 0; i < pcount; ++i) {
            Piece piece = getPiece_s(pseats[i]);
            Seat seats[BOARDROW * BOARDCOL] = { 0 };
            int count = putSeats(seats, board, true, getKind(piece));
            appendWstr__(wstr, L"%ls=【", getPieString(tmpWstr, piece));

            for (int i = 0; i < count; ++i) {
                appendWstr__(wstr, L"%ls", getSeatString(tmpWstr, seats[i]));
            }
            swprintf(tmpWstr, WIDEWCHARSIZE, L"】%d\n", count);
            wcscat(wstr, tmpWstr);
        }
    }
    wcstombs(str2, wstr, sizeof(wstr));
    //if (strcmp(str1, str2) != 0)
    //    printf("\n%s\n\n%s\n", str1, str2);

    CU_ASSERT_STRING_EQUAL(str1, str2);
    delBoard(board);
}

static void test_board_liveSeats_str(void)
{
    char str1[] = "红：14红帅K@14 20红相B@20 28红相B@28 25红马N@25 76红车R@76 count:5\n"
                  "黑：85黑将k@85 95黑士a@95 84黑士a@84 88黑車r@88 03黑砲c@03 68黑卒p@68 count:6\n",
         str2[SUPERWIDEWCHARSIZE];
    Board board = newBoard();
    setBoard__(board, FENs[1]); // 选择第2个FEN

    //* 取得各种条件下活的棋子
    wchar_t wstr[SUPERWIDEWCHARSIZE], tmpWstr[WIDEWCHARSIZE];
    wstr[0] = L'\x0';
    for (int color = RED; color <= BLACK; ++color) {
        Seat lvseats[PIECENUM] = { NULL };
        int count = getLiveSeats_bc(lvseats, board, color);
        appendWstr__(wstr, L"%ls：", color == RED ? L"红" : L"黑");
        for (int i = 0; i < count; ++i) {
            appendWstr__(wstr, L"%ls ", getSeatString(tmpWstr, lvseats[i]));
        }
        swprintf(tmpWstr, WIDEWCHARSIZE, L"count:%d\n", count);
        wcscat(wstr, tmpWstr);
    }
    wcstombs(str2, wstr, sizeof(wstr));
    //if (strcmp(str1, str2) != 0)
    //    printf("\n%s\n\n%s\n", str1, str2);

    CU_ASSERT_STRING_EQUAL(str1, str2);
    delBoard(board);
}

static void test_board_moveSeats_str(void)
{
    char str1[] = "14红帅K@14 >>【13空 15空 04空 24空 】4 =【13空 15空 04空 24空 】4 +【】0\n"
                  "20红相B@20 >>【02空 42空 】2 =【02空 42空 】2 +【】0\n"
                  "28红相B@28 >>【06空 46空 】2 =【06空 46空 】2 +【】0\n"
                  "25红马N@25 >>【04空 06空 44空 46空 13空 17空 33空 37空 】8 =【04空 06空 44空 46空 13空 17空 33空 37空 】8 +【】0\n"
                  "76红车R@76 >>【75空 74空 73空 72空 71空 70空 77空 78空 66空 56空 46空 36空 26空 16空 06空 86空 96空 】17 "
                  "=【75空 74空 73空 72空 71空 70空 77空 78空 66空 56空 46空 36空 26空 16空 06空 86空 96空 】17 +【】0\n"
                  "85黑将k@85 >>【75空 】1 =【】0 +【75空 】1\n"
                  "95黑士a@95 >>【】0 =【】0 +【】0\n"
                  "84黑士a@84 >>【73空 75空 93空 】3 =【73空 75空 93空 】3 +【】0\n"
                  "88黑車r@88 >>【87空 86空 98空 78空 】4 =【87空 86空 98空 78空 】4 +【】0\n"
                  "03黑砲c@03 >>【02空 01空 00空 04空 05空 06空 07空 08空 13空 23空 33空 43空 53空 63空 73空 83空 93空 】17 "
                  "=【02空 01空 00空 04空 05空 06空 07空 08空 13空 23空 33空 43空 53空 63空 73空 83空 93空 】17 +【】0\n"
                  "68黑卒p@68 >>【78空 】1 =【78空 】1 +【】0\n",
         str2[SUPERWIDEWCHARSIZE];
    Board board = newBoard();
    setBoard__(board, FENs[1]); // 选择第2个FEN

    //* 取得各活棋子的可移动位置
    wchar_t wstr[SUPERWIDEWCHARSIZE], tmpWstr1[WIDEWCHARSIZE], tmpWstr2[WIDEWCHARSIZE];
    wstr[0] = L'\x0';
    for (int color = RED; color <= BLACK; ++color) {
        Seat lvseats[PIECENUM] = { NULL };
        int count = getLiveSeats_bc(lvseats, board, color);
        for (int i = 0; i < count; ++i) {
            Seat fseat = lvseats[i];
            appendWstr__(wstr, L"%ls >>【", getSeatString(tmpWstr2, fseat));

            Seat mseats[BOARDROW + BOARDCOL] = { NULL };
            int mcount = moveSeats(mseats, board, fseat);
            for (int i = 0; i < mcount; ++i)
                appendWstr__(wstr, L"%ls ", getSeatString(tmpWstr2, mseats[i]));
            swprintf(tmpWstr1, WIDEWCHARSIZE, L"】%d", mcount);
            wcscat(wstr, tmpWstr1);

            wcscat(wstr, L" =【");
            int cmcount = ableMoveSeats(mseats, mcount, board, fseat);
            for (int i = 0; i < cmcount; ++i)
                appendWstr__(wstr, L"%ls ", getSeatString(tmpWstr2, mseats[i]));
            swprintf(tmpWstr1, WIDEWCHARSIZE, L"】%d", cmcount);
            wcscat(wstr, tmpWstr1);

            wcscat(wstr, L" +【");
            int kmcount = unableMoveSeats(mseats, mcount, board, fseat);
            for (int i = 0; i < kmcount; ++i)
                appendWstr__(wstr, L"%ls ", getSeatString(tmpWstr2, mseats[i]));
            swprintf(tmpWstr1, WIDEWCHARSIZE, L"】%d\n", kmcount);
            wcscat(wstr, tmpWstr1);
        }
    }
    wcstombs(str2, wstr, sizeof(wstr));
    //if (strcmp(str1, str2) != 0)
    //    printf("\n%s\n\n%s\n", str1, str2);

    CU_ASSERT_STRING_EQUAL(str1, str2);
    delBoard(board);
}

static CU_TestInfo tests_board[] = {
    { "test_board_FEN_str", test_board_FEN_str },
    { "test_board_Txt_str", test_board_Txt_str },
    { "test_board_change_str", test_board_change_str },
    { "test_board_putSeats_str", test_board_putSeats_str },
    { "test_board_liveSeats_str", test_board_liveSeats_str },
    { "test_board_moveSeats_str", test_board_moveSeats_str },
    CU_TEST_INFO_NULL,
};

static void test_chessManual_xqf(void)
{
    char *str1 = "getChessManualNumStr: movCount:44 remCount:6 remLenMax:35 maxRow:22 maxCol:6\n",
         *pgn_ccStr = "[TitleA \"第01局\"]\n"
                      "[Event \"\"]\n"
                      "[Date \"\"]\n"
                      "[Site \"\"]\n"
                      "[Red \"\"]\n"
                      "[Black \"\"]\n"
                      "[Opening \"\"]\n"
                      "[RMKWriter \"\"]\n"
                      "[Author \"\"]\n"
                      "[PlayType \"残局\"]\n"
                      "[FEN \"5a3/4ak2r/6R2/8p/9/9/9/B4N2B/4K4/3c5 -b\"]\n"
                      "[Result \"红胜\"]\n"
                      "[Version \"18\"]\n"
                      "\n"
                      "　开始　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　\n"
                      "　　↓　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　\n"
                      "马四进三……………………………………………………………………马四进五　\n"
                      "　　↓　　　　　　　　　　　　　　　　　　　　　　　　　　　　　↓　　\n"
                      "炮４退９…炮４退７……………………………士５进６…卒９进１　炮４退９　\n"
                      "　　↓　　　　↓　　　　　　　　　　　　　　↓　　　　↓　　　　↓　　\n"
                      "马三进五　马三进五　　　　　　　　　　　马三进二　马三进五　马五进三　\n"
                      "　　↓　　　　↓　　　　　　　　　　　　　　　　　　　↓　　　　↓　　\n"
                      "炮４平５　炮４平５…车９平８　　　　　　　　　　　车９进２　炮４平５　\n"
                      "　　↓　　　　↓　　　　↓　　　　　　　　　　　　　　↓　　　　↓　　\n"
                      "车三平四　马五进三　车三平四　　　　　　　　　　　车三进一　帅五平六　\n"
                      "　　↓　　　　　　　　　↓　　　　　　　　　　　　　　　　　　　↓　　\n"
                      "士５进６　　　　　　士５进６　　　　　　　　　　　　　　　　车９平８　\n"
                      "　　↓　　　　　　　　　↓　　　　　　　　　　　　　　　　　　　↓　　\n"
                      "马五进三　　　　　　马五进三…马五进六　　　　　　　　　　　马三进二　\n"
                      "　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　↓　　\n"
                      "　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　车８进１　\n"
                      "　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　↓　　\n"
                      "　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　车三平二　\n"
                      "　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　↓　　\n"
                      "　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　士５退４　\n"
                      "　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　↓　　\n"
                      "　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　车二进一　\n"
                      "　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　↓　　\n"
                      "　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　将６进１　\n"
                      "　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　↓　　\n"
                      "　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　车二退二　\n"
                      "　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　↓　　\n"
                      "　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　士６进５　\n"
                      "　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　↓　　\n"
                      "　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　车二平四　\n"
                      "　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　↓　　\n"
                      "　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　将６平５　\n"
                      "　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　↓　　\n"
                      "　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　车四平一　\n"
                      "　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　↓　　\n"
                      "　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　将５平６　\n"
                      "　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　↓　　\n"
                      "　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　车一平四　\n"
                      "　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　↓　　\n"
                      "　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　将６平５　\n"
                      "　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　↓　　\n"
                      "　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　帅六退一　\n"
                      "　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　↓　　\n"
                      "　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　炮５平６　\n"
                      "　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　　\n"
                      "\n"
                      "(1,0): {　　从相肩进马是取胜的正确途径。其它着法，均不能取胜。\r\n}\n"
                      "(12,6): {　　至此，形成少见的高将底炮双士和单车的局面。\r\n}\n"
                      "(22,6): {　　和棋。\r\n}\n"
                      "(3,4): {　　叫杀得车。\r\n}\n"
                      "(3,0): {　　不怕黑炮平中拴链，进观的攻势含蓄双有诱惑性，是红方制胜的关键。\r\n}\n"
                      "(5,0): {　　弃车，与前着相联系，由此巧妙成杀。\r\n}\n";

    ChessManual cm = newChessManual(xqfFileName__);

    char* resultStr = NULL;
    wchar_t* wstr = NULL;
    writePGN_CCtoWstr(&wstr, cm);
    int len = (wcslen(wstr) + 1) * sizeof(wchar_t);
    resultStr = malloc(len);
    wcstombs(resultStr, wstr, len);

    CU_ASSERT_STRING_EQUAL(pgn_ccStr, resultStr);
    free(wstr);
    free(resultStr);

    for (int ct = EXCHANGE; ct <= SYMMETRY; ++ct) {
        changeChessManual(cm, ct);
        char fname[32];
        sprintf(fname, "chessManual/01%d.pgn_cc", ct);
        writeChessManual(cm, fname);
    }

    char str2[WIDEWCHARSIZE];
    getChessManualNumStr(str2, cm);
    //if (strcmp(str1, str2) != 0)
    //    printf("\n%s\n\n%s\n", str1, str2);
    CU_ASSERT_STRING_EQUAL(str1, str2);

    delChessManual(cm);
}

static void test_chessManual_otherExt(void)
{
    ChessManual cm = newChessManual(xqfFileName__),
                cm1 = newChessManual(NULL); //""

    char fileName[FILENAME_MAX];
    for (RecFormat fmt = BIN; fmt <= PGN_CC; ++fmt) {
        sprintf(fileName, "chessManual/01%s", EXTNAMES[fmt]);
        writeChessManual(cm, fileName);
        resetChessManual(&cm1, fileName);
        CU_ASSERT_TRUE(chessManual_equal(cm, cm1));
    }

    delChessManual(cm);
    delChessManual(cm1);
}

static void test_chessManual_dir(void)
{
    bool isPrint = false;
    //bool isPrint = true;
    for (int dir = 0; dir < dirSize__ && dir < dirNum__; ++dir) {
        // 调节控制转换目录  XQF, BIN, JSON, PGN_ICCS, PGN_ZH, PGN_CC
        for (RecFormat fromFmt = XQF; fromFmt <= PGN_CC; ++fromFmt)
            for (RecFormat toFmt = BIN; toFmt <= PGN_CC; ++toFmt)
                if (fromFmt != toFmt) { // && fromFmt != PGN_ICCS
                    //printf("\nline:%d %s %d->%d", __LINE__, dirNames__[dir], fromFmt, toFmt);
                    transDir(dirNames__[dir], fromFmt, toFmt, isPrint);
                }
    }
}

static CU_TestInfo suite_chessManual[] = {
    { "test_chessManual_xqf", test_chessManual_xqf },
    { "test_chessManual_otherExt", test_chessManual_otherExt },
    { "test_chessManual_dir", test_chessManual_dir },
    CU_TEST_INFO_NULL,
};

static void test_aspect_file(void)
{
    char *libs = "chessManual/libs",
         *hash = "chessManual/hash";

    Aspects asps = newAspects(FEN_MovePtr, 0);
    appendAspects_file(asps, xqfFileName__);
    testAspects(asps);

    storeAspectFEN(libs, asps);
    storeAspectHash(hash, asps);
    Aspects aspsl = getAspects_fs(libs),
            aspsh = getAspects_fb(hash);
    CU_ASSERT_TRUE(aspects_equal(aspsl, aspsh));

    delAspects(aspsl);
    delAspects(aspsh);
    delAspects(asps);
}

static void test_aspect_dir(void)
{
    Aspects asps = newAspects(FEN_MovePtr, 0);
    for (int dir = 0; dir < dirSize__ && dir < dirNum__; ++dir) {
        char fromDir[FILENAME_MAX];
        sprintf(fromDir, "%s%s", dirNames__[dir], ".xqf");
        //printf("%d: %s\n", __LINE__, fromDir);

        appendAspects_dir(asps, fromDir);
    }

    testAspects(asps);

    delAspects(asps);
}

static CU_TestInfo suite_aspect[] = {
    { "test_aspect_file", test_aspect_file },
    { "test_aspect_dir", test_aspect_dir },
    CU_TEST_INFO_NULL,
};

static CU_SuiteInfo suites[] = {
    { "suite_tools", NULL, NULL, NULL, NULL, tests_tools },
    { "suite_piece", NULL, NULL, NULL, NULL, tests_piece },
    { "suite_board", NULL, NULL, NULL, NULL, tests_board },
    { "suite_chessManual", NULL, NULL, NULL, NULL, suite_chessManual },
    { "suite_aspect", NULL, NULL, NULL, NULL, suite_aspect },
    CU_SUITE_INFO_NULL,
};

int unitTest(void)
{
    //CU_pSuite pSuite = NULL;

    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry()) {
        return CU_get_error();
    }

    /* Register suites. */
    if (CUE_SUCCESS != CU_register_suites(suites)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Run all tests using the CUnit Basic interface */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    /* Clean up registry and return */
    CU_cleanup_registry();
    return CU_get_error();
}

/*
sqlite3* db;
//char* zErrMsg = 0;
int rc = sqlite3_open("test.db", &db);
if (rc) {
    fprintf(stderr, "\nCan't open database: %s\n", sqlite3_errmsg(db));
    exit(0);
} else {
    fprintf(stderr, "\nOpened database successfully\n");
}
sqlite3_close(db);
//*/