#include "board.h"
#include "piece.h"

int main(int argc, char const* argv[])
{
    setlocale(LC_ALL, "chs");
    wprintf(L"%s\n", L"输出中文成功了！");

    wchar_t* testPiece(void);
    wprintf(L"%s", testPiece());

    wchar_t* testBoard(void);
    wprintf(L"%s", testBoard());
    return 0;
}