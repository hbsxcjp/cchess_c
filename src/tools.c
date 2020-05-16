#include "head/tools.h"

typedef struct FileInfos {
    FileInfo* fis;
    int size, count;
} * FileInfos;

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
    static int primes[] = { 61, 127, 251, 509, 1021, 2039, 4093, 8191, 16381, 32749, 65521, //3, 7, 13, 31,
        131071, 262139, 524287, 1048573, 2097143, 4194301, 8388593, 16777213, 33554393,
        67108859, 134217689, 268435399, 536870909, 1073741789, 2147483647, INT_MAX };
    int i = 0;
    while (primes[i] < size)
        i++;
    return primes[i];
}

// BKDR Hash Function

unsigned int BKDRHash_c(char* src, int size)
{
    unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
    unsigned int hash = 0;

    for (int i = 0; i < size; ++i)
        hash = hash * seed + src[i];

    return (hash & 0x7FFFFFFF);
}

unsigned int BKDRHash_s(const wchar_t* wstr)
{
    //*
    int len = wcslen(wstr) * 2;
    char str[len + 1];
    wcstombs(str, wstr, len);
    return BKDRHash_c(str, strlen(str));
    //*/

    /*
    unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
    unsigned int hash = 0;

    while (*wstr) {
        hash = hash * seed + (*wstr++);
    }

    return (hash & 0x7FFFFFFF);
    //*/
}

unsigned int DJBHash(const wchar_t* wstr)
{
    unsigned int hash = 5381;
    int len = wcslen(wstr);
    for (int i = 0; i < len; i++) {
        hash = ((hash << 5) + hash) + wstr[i];
    }
    return hash;
}

unsigned int SDBMHash(const wchar_t* wstr)
{
    unsigned int hash = 0;
    int len = wcslen(wstr);
    for (int i = 0; i < len; i++) {
        hash = (hash << 5) + (hash << 16) - hash + wstr[i];
    }
    return hash;
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

static char* getSplitChar__(char* fileName)
{
    char* pext = strrchr(fileName, '/');
    if (pext == NULL)
        pext = strrchr(fileName, '\\');
    return pext;
}

char* getDirName(char* fileName)
{
    char* pext = getSplitChar__(fileName);
    if (pext != NULL)
        *pext = '\0';
    else
        fileName[0] = '\0';
    return fileName;
}

char* getFileName(char* fileName)
{
    char* pext = getSplitChar__(fileName);
    return pext ? pext + 1 : fileName;
}

const char* getExtName(const char* fileName)
{
    return strrchr(fileName, '.');
}

char* transFileExtName(char* fileName, const char* extname)
{
    char* pext = strrchr(fileName, '.');
    if (pext != NULL)
        *pext = '\0';
    return strcat(fileName, extname);
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

void operateDir(const char* dirName, void operateFile(FileInfo, void*), void* ptr, bool recursive)
{
    long hFile = 0; //文件句柄
    struct _finddata_t fileinfo = { 0 }; //文件信息
    char findName[FILENAME_MAX];
    snprintf(findName, FILENAME_MAX, "%s/*", dirName);
    //printf("%d: %s\n", __LINE__, findName);
    //fflush(stdout);
    if ((hFile = _findfirst(findName, &fileinfo)) == -1)
        return;

    do {
        if (strcmp(fileinfo.name, ".") == 0 || strcmp(fileinfo.name, "..") == 0)
            continue;
        snprintf(findName, FILENAME_MAX, "%s/%s", dirName, fileinfo.name);
        //如果是目录且要求递归，则递归调用
        if (fileinfo.attrib & _A_SUBDIR) {
            if (recursive)
                operateDir(findName, operateFile, ptr, recursive);
        } else {
            strcpy(fileinfo.name, findName);
            operateFile(&fileinfo, ptr);
        }
    } while (_findnext(hFile, &fileinfo) == 0);
    _findclose(hFile);
}

FileInfos newFileInfos(void)
{
    FileInfos fileInfos = malloc(sizeof(struct FileInfos));
    fileInfos->count = 0;
    fileInfos->size = 256;
    fileInfos->fis = malloc(fileInfos->size * sizeof(FileInfo));
    assert(fileInfos);
    return fileInfos;
}

void delFileInfos(FileInfos fileInfos)
{
    for (int i = 0; i < fileInfos->count; ++i)
        free(fileInfos->fis[i]);
    free(fileInfos->fis);
    free(fileInfos);
}

static void addFileInfos__(FileInfo fileInfo, void* ptr)
{
    FileInfos fileInfos = (FileInfos)ptr;
    if (fileInfos->count == fileInfos->size) {
        fileInfos->size += fileInfos->size;
        fileInfos->fis = realloc(fileInfos->fis, fileInfos->size * sizeof(FileInfo));
        assert(fileInfos->fis);
    }
    FileInfo fi = malloc(sizeof(struct _finddata_t));
    *fi = *fileInfo; //复制结构
    fileInfos->fis[fileInfos->count++] = fi;
}

void getFileInfos(FileInfos fileInfos, const char* dirName, bool recursive)
{
    operateDir(dirName, addFileInfos__, fileInfos, recursive);
}

// 测试函数
void testTools(FILE* fout, const char** chessManualDirName, int size, const char* ext)
{
    fprintf(fout, "\n");
    int sum = 0;
    for (int i = 0; i < size; ++i) {
        FileInfos fileInfos = newFileInfos();

        char dirName[FILENAME_MAX];
        sprintf(dirName, "%s%s", chessManualDirName[i], ext);
        getFileInfos(fileInfos, dirName, true);
        for (int i = 0; i < fileInfos->count; ++i)
            fprintf(fout, "attr:%2u size:%lu %s\n", fileInfos->fis[i]->attrib, fileInfos->fis[i]->size, fileInfos->fis[i]->name);
        sum += fileInfos->count;
        fprintf(fout, "%s 包含: %d个文件。\n\n", chessManualDirName[i], fileInfos->count);

        delFileInfos(fileInfos);
    }
    fprintf(fout, "总共包括:%d个文件。\n", sum);
}
