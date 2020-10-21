#include "head/tools.h"

#define wc_short (sizeof(wchar_t) == sizeof(unsigned short))

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

unsigned int BKDRHash_w(const wchar_t* wstr)
{
    /*
    int len = wcstombs(NULL, wstr, 0)+1;
    char str[len];
    wcstombs(str, wstr, len);
    return BKDRHash_c(str, strlen(str));
    //*/

    //*
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

bool chars_equal(const char* dst, const char* src, int len)
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

wchar_t* setPwstr_value(wchar_t** pwstr, const wchar_t* value)
{
    if (value == NULL)
        return NULL;

    if (*pwstr)
        free(*pwstr);
    *pwstr = malloc((wcslen(value) + 1) * sizeof(wchar_t));
    return wcscpy(*pwstr, value);
}

int filterObjects(void** objs, int count, void* arg1, void* arg2,
    bool (*filterFunc__)(void*, void*, void*), bool sure)
{
    int index = 0;
    while (index < count) {
        // 如符合条件，先减少count再比较序号，如小于则需要交换；如不小于则指向同一元素，不需要交换；
        if (filterFunc__(arg1, arg2, objs[index]) == sure && index < --count) {
            void* tempObj = objs[count];
            objs[count] = objs[index];
            objs[index] = tempObj;
        } else
            ++index; // 不符合筛选条件，index前进一个
    }
    return count; // 以返回的count分界，大于count的对象，均为已过滤对象
}

char* trim(char* str)
{
    size_t first = 0, last = strlen(str);
    while (isspace(str[--last]))
        str[last] = '\x0';
    while (first < last && isspace(str[first]))
        ++first;
    return str + first;
}

wchar_t* wtrim(wchar_t* wstr)
{
    size_t first = 0, last = wcslen(wstr);
    while (iswspace(wstr[--last]))
        wstr[last] = '\x0';
    while (first < last && iswspace(wstr[first]))
        ++first;
    return wstr + first;
}

void copySubStr(wchar_t* subStr, const wchar_t* srcWstr, int first, int last)
{
    int len = last - first;
    wcsncpy(subStr, srcWstr + first, len);
    subStr[len] = L'\x0';
}

wchar_t* getSubStr(const wchar_t* srcWstr, int first, int last)
{
    wchar_t* subStr = NULL;
    if (last - first > 0) {
        subStr = malloc((last - first + 1) * sizeof(wchar_t));
        assert(subStr);
        copySubStr(subStr, srcWstr, first, last);
    }
    return subStr;
}

void supper_wcscat(wchar_t** pwstr, size_t* size, const wchar_t* wstr)
{
    // 如字符串分配的长度不够，则增加长度
    if (*size < wcslen(*pwstr) + wcslen(wstr) + 1) {
        *size = *size * 2 + wcslen(wstr) + 1;
        *pwstr = realloc(*pwstr, *size * sizeof(wchar_t));
        assert(*pwstr);
    }
    wcscat(*pwstr, wstr);
}

void* pcrewch_compile(const wchar_t* wstr, int n, const char** error, int* erroffset, const unsigned char* s)
{
    if (wc_short)
        return pcre16_compile((const unsigned short*)wstr, n, error, erroffset, s);
    else
        return pcre32_compile((const unsigned int*)wstr, n, error, erroffset, s);
}

int pcrewch_exec(const void* reg, const void* p, const wchar_t* wstr, int len, int x, int y, int* ovector, int ovsize)
{
    return (wc_short ? pcre16_exec(reg, p, (const unsigned short*)wstr, len, x, y, ovector, ovsize)
                     : pcre32_exec(reg, p, (const unsigned int*)wstr, len, x, y, ovector, ovsize));
}

void pcrewch_free(void* reg)
{
    wc_short ? pcre16_free(reg) : pcre32_free(reg);
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

wchar_t* getWString(FILE* fin)
{
    wchar_t* wstr = NULL;
    if (!feof(fin)) {
        size_t size = SUPERWIDEWCHARSIZE;
        wchar_t lineStr[SUPERWIDEWCHARSIZE];
        wstr = calloc(size, sizeof(wchar_t));
        assert(wstr);
        while (fgetws(lineStr, SUPERWIDEWCHARSIZE, fin) != NULL)
            supper_wcscat(&wstr, &size, lineStr);
    }
    return wstr;
}

int makeDir(const char* dirName)
{
#ifdef __linux
    return mkdir(dirName, S_IRWXU);
#else
    return mkdir(dirName);
#endif
}

int copyFile(const char* SourceFile, const char* NewFile)
{
    FILE *fin = fopen(SourceFile, "rb"),
         *fout = fopen(NewFile, "wb");
    if (fin == NULL || fout == NULL)
        return 0;

    char data[1024];
    while (!feof(fin)) {
        size_t n = fread(data, sizeof(char), 1024, fin);
        fwrite(data, sizeof(char), n, fout);
    }
    return 1;
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

static void addFileInfos__(FileInfo fileInfo, FileInfos fileInfos)
{
    if (fileInfos->count == fileInfos->size) {
        fileInfos->size *= 2;
        fileInfos->fis = realloc(fileInfos->fis, fileInfos->size * sizeof(FileInfo));
        assert(fileInfos->fis);
    }

    fileInfos->fis[fileInfos->count++] = newFileInfo__(fileInfo->name, fileInfo->attrib, fileInfo->size);
}

void getFileInfos(FileInfos fileInfos, const char* dirName, bool recursive)
{
    operateDir(dirName, (void (*)(void*, void*))addFileInfos__, fileInfos, recursive);
}

void writeFileInfos(FILE* fout, const char* dirName)
{
    fprintf(fout, "\n");
    int sum = 0;
    FileInfos fileInfos = newFileInfos();
    getFileInfos(fileInfos, dirName, true);

    for (int i = 0; i < fileInfos->count; ++i)
        fprintf(fout, "attr:%4x size:%lu %s\n",
            fileInfos->fis[i]->attrib, fileInfos->fis[i]->size, fileInfos->fis[i]->name);
    fprintf(fout, "%s 包含: %d个文件。\n\n", dirName, fileInfos->count);

    sum += fileInfos->count;
    delFileInfos(fileInfos);

    fprintf(fout, "总共包括:%d个文件。\n", sum);
}

void operateDir(const char* dirName, void operateFile(void*, void*), void* ptr, bool recursive)
{
#ifdef __linux
    struct dirent* dp;
    DIR* dfd;

    if ((dfd = opendir(dirName)) == NULL) {
        fprintf(stderr, "operateDir: can't open %s\n", dirName);
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
                fprintf(stderr, "operateDir: open %s failed!\n", findName);
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

// 获取查询记录结果的数量
static int sqlite3_callCount__(void* count, int argc, char** argv, char** colNames)
{
    *(int*)count = (argv[0] ? atoi(argv[0]) : 0);
    return 0;
}

int sqlite3_getRecCount(sqlite3* db, const char* tblName, char* condition)
{
    int count = 0;
    char sql[WCHARSIZE], *zErrMsg = 0;
    sprintf(sql, "SELECT count(*) FROM %s WHERE %s;", tblName, condition);
    int rc = sqlite3_exec(db, sql, sqlite3_callCount__, &count, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "\nTable %s get records error: %s", tblName, zErrMsg);
        sqlite3_free(zErrMsg);
        return -1;
    }

    return count;
}

bool sqlite3_existTable(sqlite3* db, const char* tblName)
{
    char sql[WCHARSIZE];
    sprintf(sql, "type = 'table' AND name = '%s'", tblName);
    int count = sqlite3_getRecCount(db, "sqlite_master", sql);
    return count > 0;
}

// 初始化表
int sqlite3_createTable(sqlite3* db, const char* tblName, const char* colNames)
{
    char sql[WCHARSIZE];
    sprintf(sql, "CREATE TABLE %s(%s);", tblName, colNames);
    return sqlite3_exec_showErrMsg(db, sql);
}

int sqlite3_deleteTable(sqlite3* db, const char* tblName, char* condition)
{
    char sql[WCHARSIZE];
    sprintf(sql, "DELETE FROM %s WHERE %s;", tblName, condition);
    return sqlite3_exec_showErrMsg(db, sql);
}

int sqlite3_exec_showErrMsg(sqlite3* db, const char* sql)
{
    char* zErrMsg = NULL;
    int resultCode = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
    if (resultCode != SQLITE_OK)
        fprintf(stderr, "\nErrMsg: %s", zErrMsg);
    sqlite3_free(zErrMsg);
    return resultCode;
}

struct SingleList {
    void* body;
    SingleList next;
};

SingleList newSingleList(SingleList preSingleList, void* body)
{
    SingleList slist = malloc(sizeof(struct SingleList));
    slist->body = body;
    slist->next = NULL;
    if (preSingleList)
        preSingleList->next = slist;
    return slist;
}

void delSingleList(SingleList slist, void (*delBody)(void*))
{
    if (slist == NULL)
        return;

    SingleList next = slist->next;
    if (slist->body)
        delBody(slist->body);
    free(slist);

    delSingleList(next, delBody);
}

SingleList getNextSlist(SingleList slist)
{
    return slist->next;
}

void* getBody(SingleList slist)
{
    return slist->body;
}
