#include "board.h"
#include "piece.h"

int main(int argc, char const *argv[])
{
    setlocale(LC_ALL, "chs");
    FILE *fout = stdout;
    //FILE *fout = fopen("s", "w");

    fwprintf(fout, L"输出中文成功了！");
    testPiece(fout);
    testBoard(fout);

    fclose(fout);
    return 0;
}