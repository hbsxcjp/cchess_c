#include "piece.h"
#include "board.h"

int main(int argc, char const *argv[])
{
    setlocale(LC_ALL, "chs");
    wprintf(L"%s\n", L"输出中文成功了！");

    wprintf(L"\n%s\n", L"测试piece：");
    void testPiece(void);
    testPiece();

    wprintf(L"\n%s\n", L"测试board：");
    void testBoard(void);
    testBoard();
    return 0;
}