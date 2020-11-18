//#define NDEBUG
#include "head/operatefile.h"
#include "head/unitTest.h"
#include <locale.h>

//* 输出字符串，检查用
FILE* fout = NULL;

int main(int argc, char const* argv[])
{
    char* oldlocale = setlocale(LC_ALL, NULL);
    setlocale(LC_ALL, "");
    //char* newlocale = setlocale(LC_ALL, "");
    //printf("old:%s\nnew:%s\n", oldlocale, newlocale);
    setbuf(stdout, NULL);

    fout = openFile_utf8("test_out", "w");
    setbuf(fout, NULL);

    unitTest();
    fclose(fout);

    setlocale(LC_ALL, oldlocale);
    return 0;
}