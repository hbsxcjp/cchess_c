//#define NDEBUG
#include "head/operatefile.h"
#include "head/unitTest.h"
#include <locale.h>

//* 输出字符串，检查用
FILE* test_out = NULL;

int main(int argc, char const* argv[])
{
    char* oldlocale = setlocale(LC_ALL, NULL);
    setlocale(LC_ALL, "");
    
    //char* newlocale = setlocale(LC_ALL, "");
    //printf("old:%s\nnew:%s\n", oldlocale, newlocale);
    setbuf(stdout, NULL);

    test_out = fopen("test_out.txt", "w");
    //test_out = openFile_utf8("test_out.txt", "w");
    setbuf(test_out, NULL);

    unitTest();
    fclose(test_out);

    setlocale(LC_ALL, oldlocale);
    return 0;
}