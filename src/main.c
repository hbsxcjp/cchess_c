//#define NDEBUG
#include "head/base.h"
#include "head/unitTest.h"

int main(int argc, char const* argv[])
{
    setlocale(LC_ALL, "");
    setbuf(stdin, NULL);

    unitTest();
    
    implodedTest(argc, argv);

    return 0;
}