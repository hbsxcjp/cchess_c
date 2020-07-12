#include "head/tools.h"

#define wc_short (sizeof(wchar_t) == sizeof(unsigned short))

typedef struct FileInfo {
    char* name;
    unsigned int attrib;
    unsigned long size;
} * FileInfo;

typedef struct FileInfos {
    FileInfo* fis;
    int size;
    int count;
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
        67108859, 134217689, 268435399, 536870909, 1073741789, INT32_MAX };
    int i = 0;
    while (primes[i] < size)
        i++;
    return primes[i];
}

// BKDR Hash Function

unsigned int BKDRHash_c(const char* src, int size)
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

bool str_equal(const char* dst, const char* src, int len)
{
    for (int i = 0; i < len; ++i)
        if (dst[i] != src[i])
            return false;
    return true;
}

void hashToStr(char* str, unsigned char* hash, int length)
{
    str[0] = '\x0';
    char tmpStr[3];
    for (int i = 0; i < length; i++) {
        sprintf(tmpStr, "%02x", hash[i]);
        strcat(str, tmpStr);
    }
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

// 返回指向目录与文件的分界字符指针，如不存在则返回NULL
static char* getSplitChar__(const char* fileName)
{
    char *sp0 = strrchr(fileName, '/'),
         *sp1 = strrchr(fileName, '\\');
    // 两个符号都存在，则返回较后的那个
    if (sp0 && sp1)
        return sp0 > sp1 ? sp0 : sp1;
    return sp0 ? sp0 : sp1;
}

void getDirName(char* dirName, const char* fileName)
{
    int size = 0;
    char* sp = getSplitChar__(fileName);
    if (sp != NULL) {
        size = sp - fileName;
        strncpy(dirName, fileName, size);
    }
    dirName[size] = '\x0';
}

const char* getFileName(const char* fileName)
{
    char* sp = getSplitChar__(fileName);
    return sp ? sp + 1 : fileName;
}

const char* getExtName(const char* fileName)
{
    return strrchr(fileName, '.');
}

void transFileExtName(char* fileName, const char* extname)
{
    char *sp0 = strrchr(fileName, '.'),
         *sp1 = getSplitChar__(fileName);
    // 有文件扩展名，且无目录名或扩展点字符指针大于目录分界字符指针
    if (sp0 != NULL && (sp1 == NULL || sp0 > sp1))
        *sp0 = '\0';
    strcat(fileName, extname);
}

void supper_wcscat(wchar_t** pwstr, size_t* size, const wchar_t* wstr)
{
    // 如字符串分配的长度不够，则增加长度
    size_t len = wcslen(*pwstr) + wcslen(wstr) + 1;
    if (len > *size) {
        *size = len + WIDEWCHARSIZE;
        *pwstr = realloc(*pwstr, *size * sizeof(wchar_t));
        assert(*pwstr);
    }
    wcscat(*pwstr, wstr);
}

wchar_t* getWString(FILE* fin)
{
    wchar_t* wstr = NULL;
    if (!feof(fin)) {
        size_t size = WIDEWCHARSIZE;
        wchar_t lineStr[WIDEWCHARSIZE];
        wstr = calloc(size, sizeof(wchar_t));
        assert(wstr);
        while (fgetws(lineStr, WIDEWCHARSIZE, fin) != NULL)
            supper_wcscat(&wstr, &size, lineStr);
    }
    return wstr;
}

void* pcrewch_compile(const wchar_t* wstr, int n, const char** error, int* erroffset, const unsigned char* s)
{
    if (wc_short)
        return pcre16_compile((const unsigned short*)wstr, n, error, erroffset, s);
    else
        return pcre32_compile((const unsigned int*)wstr, n, error, erroffset, s);
}

/*
// 返回值 <0:表示匹配发生error，==0:没有匹配上，>0:返回匹配到的元素数量
ovector是一个int型数组，其长度必须设定为3的倍数，若为3n，则最多返回n个元素，显然有rc<=n
其中ovector[0],[1]为整个匹配上的字符串的首尾偏移；其他[2*i][2*i+1]为对应第i个匹配上的子串的偏移,
子串意思是正则表达式中被第i个()捕获的字符串，计数貌似是按照(出现的顺序。
如正则式/abc((.*)cf(exec))test/,在目标字符串11111abcword1cfexectest11111中匹配，将返回4个元素，其首尾偏移占用ovector的0~7位
元素0=abcword1cfexectest,
元素1=word1cfexec
元素2=word1
元素3=exec
ovector的最后1/3个空间，即[2n~3n-1]，貌似为pcre正则匹配算法预留，不返回结果
//*/
int pcrewch_exec(const void* reg, const void* p, const wchar_t* wstr, int len, int x, int y, int* ovector, int ovsize)
{
    return (wc_short ? pcre16_exec(reg, p, (const unsigned short*)wstr, len, x, y, ovector, ovsize)
                     : pcre32_exec(reg, p, (const unsigned int*)wstr, len, x, y, ovector, ovsize));
}

void pcrewch_free(void* reg)
{
    wc_short ? pcre16_free(reg) : pcre32_free(reg);
}

int makeDir(const char* dirName)
{
#ifdef __linux
    return mkdir(dirName, S_IRWXU);
#else
    return mkdir(dirName);
#endif
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

#ifdef __linux
int code_convert(const char* from_charset, const char* to_charset, char* inbuf, char* outbuf, size_t* outlen)
{
    int tag = 0;
    iconv_t cd = iconv_open(to_charset, from_charset);
    if (cd == (iconv_t)-1)
        return -1;
    size_t inlen = strlen(inbuf);
    memset(outbuf, 0, *outlen);
    if (iconv(cd, &inbuf, &inlen, &outbuf, outlen) == (size_t)-1)
        tag = -1;
    iconv_close(cd);
    return tag;
}
#endif

static FileInfo newFileInfo__(char* name, int attrib, long unsigned int size)
{
    FileInfo fileInfo = malloc(sizeof(struct FileInfo));
    fileInfo->name = malloc(strlen(name) + 1);
    strcpy(fileInfo->name, name);
    fileInfo->attrib = attrib;
    fileInfo->size = size;
    return fileInfo;
}

static void delFileInfo__(FileInfo fileInfo)
{
    free(fileInfo->name);
    free(fileInfo);
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
        //free(fileInfos->fis[i]);
        delFileInfo__(fileInfos->fis[i]);
    free(fileInfos->fis);
    free(fileInfos);
}

void operateDir(const char* dirName, void operateFile(void*, void*), void* ptr, bool recursive)
{
#ifdef __linux
    struct dirent* dp;
    DIR* dfd;

    if ((dfd = opendir(dirName)) == NULL) {
        fprintf(stderr, "dirwalk: can't open %s\n", dirName);
        return;
    }

    while ((dp = readdir(dfd)) != NULL) { //读目录记录项
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
            continue; //跳过当前目录以及父目录
        }

        char findName[FILENAME_MAX];
        if (strlen(dirName) + strlen(dp->d_name) + 2 > sizeof(findName)) {
            fprintf(stderr, "operateDir: name %s/%s too long!\n", dirName, dp->d_name);
        } else {
            snprintf(findName, FILENAME_MAX, "%s/%s", dirName, dp->d_name);
            struct stat stbuf;
            if (stat(findName, &stbuf) == -1) {
                fprintf(stderr, "file size: open %s failed!\n", findName);
                return;
            }

            //如果是目录且要求递归，则递归调用
            if ((stbuf.st_mode & __S_IFMT) == __S_IFDIR) {
                if (recursive)
                    operateDir(findName, operateFile, ptr, recursive);
            } else {
                FileInfo fi = newFileInfo__(findName, stbuf.st_mode, stbuf.st_size);
                operateFile(fi, ptr);
                delFileInfo__(fi);
            }
        }
    }
    closedir(dfd);
    //#endif

#else
    //#ifdef _WIN32
    long hFile = 0; //文件句柄
    struct _finddata_t wfileinfo = { 0 }; //文件信息
    char findName[FILENAME_MAX];
    snprintf(findName, FILENAME_MAX, "%s/*", dirName);
    //printf("%d: %s\n", __LINE__, findName);
    //fflush(stdout);
    if ((hFile = _findfirst(findName, &wfileinfo)) == -1)
        return;

    do {
        if (strcmp(wfileinfo.name, ".") == 0 || strcmp(wfileinfo.name, "..") == 0)
            continue;
        snprintf(findName, FILENAME_MAX, "%s/%s", dirName, wfileinfo.name);
        //如果是目录且要求递归，则递归调用
        if (wfileinfo.attrib & _A_SUBDIR) {
            if (recursive)
                operateDir(findName, operateFile, ptr, recursive);
        } else {
            //strcpy(wfileinfo.name, findName);
            //operateFile(&fileinfo, ptr);
            FileInfo fi = newFileInfo__(findName, wfileinfo.attrib, wfileinfo.size);
            operateFile(fi, ptr);
            delFileInfo__(fi);
        }
    } while (_findnext(hFile, &wfileinfo) == 0);
    _findclose(hFile);
#endif
}

static void addFileInfos__(FileInfo fileInfo, FileInfos fileInfos)
{
    //FileInfos fileInfos = (FileInfos)ptr;
    if (fileInfos->count == fileInfos->size) {
        fileInfos->size += fileInfos->size;
        fileInfos->fis = realloc(fileInfos->fis, fileInfos->size * sizeof(FileInfo));
        assert(fileInfos->fis);
    }
    //FileInfo fi = malloc(sizeof(struct _finddata_t));
    //*fi = *fileInfo; //复制结构
    //fileInfos->fis[fileInfos->count++] = fi;
    fileInfos->fis[fileInfos->count++] = fileInfo;
}

void getFileInfos(FileInfos fileInfos, const char* dirName, bool recursive)
{
    operateDir(dirName, (void (*)(void*, void*))addFileInfos__, fileInfos, recursive);
}

void getFileInfoName(char* fileName, FileInfo fileInfo)
{
    strcpy(fileName, fileInfo->name);
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
