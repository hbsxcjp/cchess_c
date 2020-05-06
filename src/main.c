//#define NDEBUG

#include "head/base.h"
#include "head/board.h"
#include "head/chessManual.h"
//#include "head/console.h"
//#include "head/move.h"
#include "head/md5.h"
#include "head/piece.h"
#include "head/tools.h"
#include <time.h>

int main(int argc, char const* argv[])
{
    setlocale(LC_ALL, "");
    setbuf(stdin, NULL);

    time_t time0;
    time(&time0);

    //FILE* fout = stdout;
    FILE* fout = fopen("s", "w");
    if (!fout)
        return -1;
    //fwprintf(fout, L"输出中文成功了！\n");

    unsigned char digest[16];
    getMD5(digest, "123.txt");
    printf("加密:123.txt 结果:");
    for (int i = 0; i < 16; i++) {
        printf("%02x", digest[i]);
    }
    printf("\n");

    testPiece(fout);
    testBoard(fout);
    testChessManual(fout);

    const char* chessManualDirName[] = {
        "chessManual/示例文件",
        "chessManual/象棋杀着大全",
        "chessManual/疑难文件",
        "chessManual/中国象棋棋谱大全"
    };
    int size = sizeof(chessManualDirName) / sizeof(chessManualDirName[0]);
    testTools(fout, chessManualDirName, size, ".xqf");
    //*
    if (argc == 4)
        testTransDir(chessManualDirName, size, atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));
    else {
        testTransDir(chessManualDirName, size, 2, 1, 2);
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