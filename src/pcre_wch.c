#include "head/pcre_wch.h"

#define wc_short (sizeof(wchar_t) == sizeof(unsigned short))

void* pcrewch_compile(const wchar_t* wstr, int n, const char** error, int* erroffset, const unsigned char* s)
{
    if (wc_short)
        return pcre16_compile((PCRE_SPTR16)wstr, n, error, erroffset, s);
    else
        return pcre32_compile((PCRE_SPTR32)wstr, n, error, erroffset, s);
}

int pcrewch_exec(const void* reg, const void* p, const wchar_t* wstr, int len, int x, int y, int* ovector, int ovsize)
{
    return (wc_short ? pcre16_exec(reg, p, (PCRE_SPTR16)wstr, len, x, y, ovector, ovsize)
                     : pcre32_exec(reg, p, (PCRE_SPTR32)wstr, len, x, y, ovector, ovsize));
}

int pcrewch_copy_substring(const wchar_t* wstr, int* ovector, int subStrCount, int subStrNum, wchar_t* subStr, int subSize)
{
    return (wc_short ? pcre16_copy_substring((PCRE_SPTR16)wstr, ovector, subStrCount, subStrNum, (PCRE_UCHAR16*)subStr, subSize)
                     : pcre32_copy_substring((PCRE_SPTR32)wstr, ovector, subStrCount, subStrNum, (PCRE_UCHAR32*)subStr, subSize));
}

void pcrewch_free(void* reg)
{
    wc_short ? pcre16_free(reg) : pcre32_free(reg);
}