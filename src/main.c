#include "head/board.h"
#include "head/instance.h"
#include "head/move.h"
#include "head/piece.h"
#include <time.h>

int main(int argc, char const* argv[])
{
    setlocale(LC_ALL, "chs");

    time_t time0;
    time(&time0);
    FILE* fout = stdout;
    //FILE *fout = fopen("s", "w");
    if (fout) {
        //fwprintf(fout, L"输出中文成功了！\n");
        //testPiece(fout);
        //testBoard(fout);
        testInstance(fout);
        testTransDir(0, 2, 3, 4, 1, 6);  // 4,5 ?
    }

    time_t time1;
    time(&time1);
    double t = difftime(time1, time0);
    //printf("  use:%6.2fs\n", t);
    printf("start:%s  end:%s  use:%6.2fs\n", ctime(&time0), ctime(&time1), t);

    fclose(fout);
    return 0;
}