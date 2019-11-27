#include "head/board.h"
#include "head/instance.h"
#include "head/move.h"
#include "head/piece.h"

int main(int argc, char const* argv[])
{
    setlocale(LC_ALL, "chs");

    FILE* fout = stdout;
    //FILE *fout = fopen("s", "w");
    if (fout) {
        //fwprintf(fout, L"输出中文成功了！\n");
        //testPiece(fout);
        //testBoard(fout);
        testInstance(fout);
        testTransDir(0, 1, 0, 1, 1, 6);

        fclose(fout);
    }
    return 0;
}