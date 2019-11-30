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
    if (!fout)
        return -1;
    //fwprintf(fout, L"输出中文成功了！\n");
    //testPiece(fout);
    //testBoard(fout);
    //testInstance(fout);
    if (argc == 7) {
        testTransDir(atoi(argv[1]), atoi(argv[2]),
            atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]));
    } else {
        testTransDir(0, 2, 0, 1, 1, 6);
        //testTransDir(2, 3, 0, 3, 1, 3);
    }

    time_t time1;
    time(&time1);
    double t = difftime(time1, time0);
    //printf("  use:%6.2fs\n", t);
    printf("start:%s  end:%s  use:%6.2fs\n", ctime(&time0), ctime(&time1), t);

    fclose(fout);
    return 0;
}