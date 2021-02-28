#include "head/operatefile.h"
#include "head/base.h"
#include "head/mylinkedlist.h"
#include "head/tools.h"


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

FILE* openFile_utf8(const char* fileName, const char* modes)
{
#ifdef __linux
    return fopen(fileName, modes);
#else
    char useModes[WCHARSIZE];
    strcpy(useModes, modes);
    //strcat(useModes, ", ccs=");
    //strcat(useModes, charSet);
    strcat(useModes, ", ccs=UTF-8");
    return fopen(fileName, useModes);
#endif
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
        wstr = malloc(size * sizeof(wchar_t));
        wcscpy(wstr, L"");
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

size_t mbstowcs_gbk(wchar_t* dest, char* src_gbk)
{
    size_t size = 0;
#ifdef __linux
    size_t src_len = strlen(src_gbk) + 1,
           utf8_size = src_len * 6;
    char src_utf8[utf8_size];
    code_convert("gbk", "utf-8", src_gbk, src_utf8, &utf8_size);
    size = mbstowcs(dest, src_utf8, utf8_size);
#else
    size = mbstowcs(dest, src_gbk, mbstowcs(NULL, src_gbk, 0) + 1);
#endif

    return size;
}

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

static void addFI_LinkedList__(FileInfo fileInfo, MyLinkedList fi_LinkedList)
{
    addMyLinkedList(fi_LinkedList, newFileInfo__(fileInfo->name, fileInfo->attrib, fileInfo->size));
}

MyLinkedList getFileInfo_MyLinkedList(const char* dirName, bool recursive)
{
    MyLinkedList fi_LinkedList = newMyLinkedList((void (*)(void*))delFileInfo__);
    operateDir(dirName, (void (*)(void*, void*))addFI_LinkedList__, fi_LinkedList, recursive);

    return fi_LinkedList;
}

static void printfFileInfo__(FileInfo fileInfo, FILE* fout, void* _0, void* _1)
{
    fprintf(fout, "attr:%4x size:%lu %s\n",
        fileInfo->attrib, fileInfo->size, fileInfo->name);
}

void writeFileInfos(FILE* fout, const char* dirName)
{
    fprintf(fout, "\n");

    MyLinkedList fi_LinkedList = getFileInfo_MyLinkedList(dirName, true);
    traverseMyLinkedList(fi_LinkedList, (void (*)(void*, void*, void*, void*))printfFileInfo__,
        fout, NULL, NULL);

    fprintf(fout, "%s 包含: %d个文件。\n\n", dirName, myLinkedList_size(fi_LinkedList));
    delMyLinkedList(fi_LinkedList);
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
    intptr_t hFile = 0; //文件句柄
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
            //printf("%d: %s\n", __LINE__, findName);
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
