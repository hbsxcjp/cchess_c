#include "piece.h"


int main(int argc, char const* argv[])
{
    setlocale(LC_ALL, "chs");
    wprintf(L"%s\n", L"输出中文成功了！");

    testPiece();
    return 0;
}