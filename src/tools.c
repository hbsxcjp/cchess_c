#include "head/tools.h"

bool isPrime(int n)
{
    if (n <= 3)
        return n > 1;
    for (int i = 2, sn = sqrt(n); i <= sn; i++)
        if (n % i == 0)
            return false;
    return true;
}

int getPrimes(int* primes, int bitCount)
{
    int index = 0;
    for (int i = 1; i < bitCount; ++i) {
        int n = (1 << i) - 1;
        while (n > 1 && !isPrime(n))
            n--;
        primes[index++] = n;
    }
    return index;
}
/*
    int primes[32];
    int count = getPrimes(primes, 32);
    for (int i = 0; i < count; i++)
        fwprintf(fout, L"%d, ", primes[i]);
//*/

int getPrime(int size)
{
    //static int primes[] = { 509, 509, 1021, 2053, 4093, 8191, 16381, 32771, 65521, INT_MAX };
    static int primes[] = { 3, 7, 13, 31, 61, 127, 251, 509, 1021, 2039, 4093, 8191, 16381, 32749, 65521,
        131071, 262139, 524287, 1048573, 2097143, 4194301, 8388593, 16777213, 33554393,
        67108859, 134217689, 268435399, 536870909, 1073741789, 2147483647, INT_MAX };
    int i = 0;
    while (primes[i] <= size)
        i++;
    return primes[i];
}

// BKDR Hash Function
unsigned int BKDRHash(const wchar_t* wstr)
{
    unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
    unsigned int hash = 0;

    while (*wstr) {
        hash = hash * seed + (*wstr++);
    }

    return (hash & 0x7FFFFFFF);
}

char* trim(char* str)
{
    size_t size = strlen(str);
    for (int i = size - 1; i >= 0; --i)
        if (isspace((int)str[i]))
            str[i] = '\x0';
        else
            break;
    size = strlen(str);
    int offset = 0;
    for (int i = 0; i < size; ++i)
        if (isspace((int)str[i]))
            ++offset;
        else
            break;
    return str + offset;
}

wchar_t* wtrim(wchar_t* wstr)
{
    size_t size = wcslen(wstr);
    for (int i = size - 1; i >= 0; --i)
        if (iswspace(wstr[i]))
            wstr[i] = L'\x0';
        else
            break;
    size = wcslen(wstr);
    int offset = 0;
    for (int i = 0; i < size; ++i)
        if (iswspace(wstr[i]))
            ++offset;
        else
            break;
    return wstr + offset;
}

static char* getSplitChar__(char* filename)
{
    char* pext = strrchr(filename, '/');
    if (pext == NULL)
        pext = strrchr(filename, '\\');
    return pext;
}

char* getDirName(char* filename)
{
    char* pext = getSplitChar__(filename);
    if (pext != NULL)
        *pext = '\0';
    return filename;
}

char* getFileName(char* filename)
{
    char* pext = getSplitChar__(filename);
    return pext ? pext + 1 : filename;
}

const char* getExtName(const char* filename)
{
    return strrchr(filename, '.');
}

char* transFileExtName(char* filename, const char* extname)
{
    char* pext = strrchr(filename, '.');
    if (pext != NULL)
        *pext = '\0';
    return strcat(filename, extname);
}

wchar_t* getWString(FILE* fin)
{
    long start = ftell(fin);
    fseek(fin, 0, SEEK_END);
    long end = ftell(fin);
    fseek(fin, start, SEEK_SET);
    wchar_t* wstr = malloc((end - start + 1) * sizeof(wchar_t));
    int index = 0;
    while (!feof(fin))
        wstr[index++] = fgetwc(fin);
    wstr[index] = L'\x0';
    return wstr;
}

void writeWString(wchar_t** pstr, int* size, const wchar_t* wstr)
{
    int len = wcslen(wstr);
    // 如字符串分配的长度不够，则增加长度
    if (wcslen(*pstr) + len > *size - 1) {
        *size += WIDEWCHARSIZE + len;
        *pstr = realloc(*pstr, *size * sizeof(wchar_t));
        assert(*pstr);
    }
    wcscat(*pstr, wstr);
}

/*****************************************************************************************
Function:       CopyFile
Description:    复制文件
Input:          SourceFile:原文件路径 NewFile:复制后的文件路径
Return:         1:成功 0:失败
******************************************************************************************/
int copyFile(const char* SourceFile, const char* NewFile)
{
    FILE* fin = fopen(SourceFile, "rb"); //打开源文件
    if (fin == NULL) //打开源文件失败
    {
        printf("Error 1: Fail to open the source file.");
        return 0;
    }
    FILE* fout = fopen(NewFile, "wb"); //创建目标文件
    if (fout == NULL) //创建文件失败
    {
        printf("Error 2: Fail to create the new file.");
        return 0;
    } else { //复制文件
        char data[1024] = { 0 };
        size_t n = 0;
        while (!feof(fin)) {
            n = fread(data, sizeof(char), 1024, fin);
            fwrite(data, sizeof(char), n, fout);
        }
        return 1;
    }
}

static void __getFileInfos(struct _wfinddata_t fileInfos[], int* fileCount, int maxCount, const wchar_t* dirName, bool isRecursive)
{
    if (*fileCount > maxCount) {
        wprintf(L"文件数量已达到最大值。\n");
        return;
    }

    //wprintf(L"%d: %s\n", __LINE__, dirName);
    long hFile = 0; //文件句柄
    struct _wfinddata_t fileinfo; //文件信息
    wchar_t findDirName[FILENAME_MAX];
    wcscat(wcscpy(findDirName, dirName), L"\\*");
    if ((hFile = _wfindfirst(findDirName, &fileinfo)) == -1)
        return;

    do {
        if (wcscmp(fileinfo.name, L".") == 0) // 本级目录
            continue;
        struct _wfinddata_t* afileInfo = &fileInfos[(*fileCount)++];
        afileInfo->attrib = fileinfo.attrib;
        afileInfo->size = fileinfo.size;
        wcscpy(afileInfo->name, fileinfo.name);
        if (wcscmp(fileinfo.name, L"..") == 0) // 上一级目录
            continue;
        if (isRecursive && fileinfo.attrib & _A_SUBDIR) {
            wcscat(wcscat(wcscpy(findDirName, dirName), L"\\"), fileinfo.name);
            __getFileInfos(fileInfos, fileCount, maxCount, findDirName, isRecursive);
        }
    } while (_wfindnext(hFile, &fileinfo) == 0);

    _findclose(hFile);
}

void getFileInfos(struct _wfinddata_t fileInfos[], int* fileCount, int maxCount, const wchar_t* dirName, bool isRecursive)
{
    __getFileInfos(fileInfos, fileCount, maxCount, dirName, isRecursive);
}

// 测试函数
void testTools(FILE* fout)
{
    wchar_t* paths[] = {
        L"c:\\棋谱\\示例文件.pgn_zh",
        L"c:\\棋谱\\象棋杀着大全.pgn_zh",
        L"c:\\棋谱\\疑难文件.pgn_iccs",
        L"c:\\棋谱\\中国象棋棋谱大全"
    };
    int sum = 0;
    for (int i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i) {
        int count = 0;
        struct _wfinddata_t fileInfos[THOUSAND];
        getFileInfos(fileInfos, &count, THOUSAND, paths[i], true);
        for (int i = 0; i < count; ++i)
            fwprintf(fout, L"attr:%2x size:%9d %s\n", fileInfos[i].attrib, fileInfos[i].size, fileInfos[i].name);
        sum += count;
        fwprintf(fout, L"%s 包含: %d个文件。\n\n", paths[i], count);
    }
    fwprintf(fout, L"总共包括:%d个文件。\n", sum);
}
