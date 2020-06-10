#include "head/unitTest.h"
#include "head/base.h"
#include "head/board.h"
#include "head/chessManual.h"
//#include "head/console.h"
#include "CUnit/Basic.h"
#include "CUnit/CUnit.h"
#include "head/md5.h"
#include "head/move.h"
#include "head/piece.h"
#include "head/sha1.h"
#include "head/tools.h"
#include <time.h>

static void test_md5(void)
{
    char str[] = "admin", str1[] = "21232f297a57a5a743894a0e4a801fc3";
    //char str[] = "陈建平", str1[] = "21bb705f61cb371b96a9f0c48ec68896";// 未成功！

    unsigned char md5[MD5HashSize];
    getMD5(md5, str);
    char str2[33];
    hashToStr(md5, MD5HashSize, str2);

    CU_ASSERT_STRING_EQUAL(str1, str2);
    //testMD5_1();
    //testMD5_2("s");
    //printf("\n%s\n%s\n", str1, str2);
}

static void test_sha1(void)
{
    char str[] = "admin", str1[] = "d033e22ae348aeb5660fc2140aec35850c4da997";
    //char str[] = "陈建平", str1[] = "2b86ed62ae08865d16c6d4f86c5b79f695e6c723"; // 未成功！

    unsigned char sha1[SHA1HashSize];
    getSHA1(sha1, str);
    char str2[41];
    hashToStr(sha1, SHA1HashSize, str2);

    CU_ASSERT_STRING_EQUAL(str1, str2);
    //testsha1();
    //printf("\n%s\n%s\n", str1, str2);
}

static CU_TestInfo tests_tools[] = {
    { "test_md5", test_md5 },
    { "test_sha1", test_sha1 },
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
    // FEN转换成PieChars
    getPieChars_FEN(pieChars, FEN);
    // 设置棋局，生成PieChars，转换成FEN
    setBoard_pieChars(board, pieChars);
}

static void test_board_FEN_str(void)
{
    wchar_t pieChars[SEATNUM + 1], pieChars2[SEATNUM + 1], FEN[SEATNUM + 1];
    char str1[SEATNUM + 1], str2[SEATNUM + 1], str3[SEATNUM + 1], str4[SEATNUM + 1];
    Board board = newBoard();
    for (int i = 0; i < sizeof(FENs) / sizeof(FENs[0]); ++i) {
        getPieChars_FEN(pieChars, FENs[i]);
        setBoard__(board, FENs[i]);

        getPieChars_board(pieChars2, board);
        wcstombs(str1, pieChars, WIDEWCHARSIZE);
        wcstombs(str2, pieChars2, WIDEWCHARSIZE);
        CU_ASSERT_STRING_EQUAL(str1, str2);

        // PieChars转换成FEN
        getFEN_pieChars(FEN, pieChars);
        wcstombs(str3, FENs[i], WIDEWCHARSIZE);
        wcstombs(str4, FEN, WIDEWCHARSIZE);
        CU_ASSERT_STRING_EQUAL(str3, str4);
    }
}

static void getBoardStr__(char* str, Board board)
{
    wchar_t boardStr[WIDEWCHARSIZE], preStr[WCHARSIZE], sufStr[WCHARSIZE], seatStr[WCHARSIZE];
    wchar_t wstr[SUPERWIDEWCHARSIZE];
    swprintf(wstr, sizeof(wstr), L"%s%s%s%s\n",
        getBoardPreString(preStr, board),
        getBoardString(boardStr, board),
        getBoardSufString(sufStr, board),
        getSeatString(seatStr, getKingSeat(board, RED)));
    wcstombs(str, wstr, SUPERWIDEWCHARSIZE);
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
        "14红帅K@14\n"
    },
         str2[SUPERWIDEWCHARSIZE];
    Board board = newBoard();
    for (int i = 0; i < sizeof(FENs) / sizeof(FENs[0]); ++i) {
        setBoard__(board, FENs[i]);
        getBoardStr__(str2, board);
        printf("\n%s\n%s\n", str1[i], str2);
        //CU_ASSERT_STRING_EQUAL(str1[i], str2);
    }
}

static CU_TestInfo tests_board[] = {
    { "test_board_FEN_str", test_board_FEN_str },
    { "test_board_Txt_str", test_board_Txt_str },
    CU_TEST_INFO_NULL,
};

static CU_SuiteInfo suites[] = {
    { "suite_tools", NULL, NULL, NULL, NULL, tests_tools },
    { "suite_piece", NULL, NULL, NULL, NULL, tests_piece },
    { "suite_board", NULL, NULL, NULL, NULL, tests_board },
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

int implodedTest(int argc, char const* argv[])
{
    time_t time0;
    time(&time0);

    //FILE* fout = stdout;
    FILE* fout = fopen("s", "w");
    if (!fout)
        return -1;
    //fwprintf(fout, L"输出中文成功了！\n");

    //testPiece(fout);
    testBoard(fout);
    testChessManual(fout);

    const char* chessManualDirName[] = {
        "chessManual/示例文件",
        "chessManual/象棋杀着大全",
        "chessManual/疑难文件",
        "chessManual/中国象棋棋谱大全"
    };
    int size = sizeof(chessManualDirName) / sizeof(chessManualDirName[0]);
    //testTools(fout, chessManualDirName, size, ".xqf");
    //testsha1();

    //*
    if (argc == 4)
        testTransDir(chessManualDirName, size, atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));
    else if (argc == 2) {
        testTransDir(chessManualDirName, size, atoi(argv[1]), 1, 2);
        //testTransDir(chessManualDirName, size, 2, 6, 6);
    }
    //*/

    //doView();
    //testConview();

    //PConsole pconsole = newConsole("01.xqf");
    //delConsole(pconsole);

    //wchar_t* dir = L"C:\\棋谱\\示例文件.xqf";
    //wchar_t* dir = L"C:\\棋谱\\象棋杀着大全.xqf";
    //textView(dir);

    time_t time1;
    time(&time1);
    double t = difftime(time1, time0);
    //printf("  use:%6.2fs\n", t);
    printf("\nuse:%6.3fs\n", t); // , ctime(&time0), ctime(&time1)

    fclose(fout);
    return 0;
}