#include "piece.h"
#include "board.h"

int main(int argc, char const *argv[])
{
    setlocale(LC_ALL, "chs");
    wprintf(L"%s\n", L"输出中文成功了！");

    void testPiece(void);
    testPiece();

    void testBoard(void);
    testBoard();
    return 0;
}