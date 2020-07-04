//#define NDEBUG
#include "head/base.h"
#include "head/unitTest.h"

int main(int argc, char const* argv[])
{
    char* oldlocale = setlocale(LC_ALL, NULL);
    char* newlocale = setlocale(LC_ALL, "");
    setbuf(stdout, NULL);

    unitTest();
    
    printf("old:%s new:%s\n", oldlocale, newlocale);
    setlocale(LC_ALL, oldlocale);
    return 0;
}