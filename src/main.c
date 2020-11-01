//#define NDEBUG
#include "head/base.h"
#include "head/unitTest.h"

//* 输出字符串，检查用
FILE* fout;

int main(int argc, char const* argv[])
{
    char* oldlocale = setlocale(LC_ALL, NULL);
    setlocale(LC_ALL, "");
    //char* newlocale = setlocale(LC_ALL, "");
    //printf("old:%s\nnew:%s\n", oldlocale, newlocale);
    setbuf(stdout, NULL);

    char* wm;
#ifdef __linux
    wm = "w";
#else
    wm = "w, ccs=UTF-8";
#endif

    fout = fopen("test_out", wm);
    setbuf(fout, NULL);
    unitTest();
    fclose(fout);

    setlocale(LC_ALL, oldlocale);

    return 0;
}