//#define NDEBUG
#include "head/base.h"
#include "head/unitTest.h"

int main(int argc, char const* argv[])
{
    setlocale(LC_ALL, "");
    setbuf(stdout, NULL);

    unitTest();
    return 0;
}