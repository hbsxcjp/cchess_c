//#define NDEBUG
#include "head/base.h"
#include "head/unitTest.h"

int main(int argc, char const* argv[])
{
    char* oldlocale = setlocale(LC_ALL, NULL);
    setlocale(LC_ALL, "");
    //char* newlocale = setlocale(LC_ALL, "");
    //printf("old:%s\nnew:%s\n", oldlocale, newlocale);

    setbuf(stdout, NULL);
    unitTest();
    
    setlocale(LC_ALL, oldlocale);
    return 0;
}