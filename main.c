#include "board.h"
#include "instance.h"
#include "move.h"
#include "piece.h"

int main(int argc, char const* argv[])
{
    setlocale(LC_ALL, "chs");
    FILE* fout = stdout;
    //FILE *fout = fopen("s", "w");
    fwprintf(fout, L"输出中文成功了！");

    testPiece(fout);
    testBoard(fout);

    Move* rootMove = newMove();
    wchar_t str[TEMPSTR_SIZE];
    //fwprintf(fout, getMovString(str, TEMPSTR_SIZE, rootMove));

    fclose(fout);
    return 0;
}