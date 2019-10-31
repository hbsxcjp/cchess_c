<<<<<<< HEAD
#include "piece.h"


int main(int argc, char const* argv[])
{
    setlocale(LC_ALL, "chs");
    wprintf(L"%s\n", L"输出中文成功了！");

    testPiece();
    return 0;
=======
#include "piece.h"

int main(int argc, char const *argv[])
{
    setlocale(LC_ALL, "chs");
    wprintf(L"%s\n", L"输出中文成功了！");

    void testPiece(void);
    testPiece();
    return 0;
>>>>>>> 1a60df0822700f6f18336bcd02d81aa29aa9ca10
}