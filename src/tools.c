#include "head/tools.h"

typedef struct FileInfo {
    char* name;
    int attrib;
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

bool charIsSame(const char* dst, const char* src, int len)
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
    char* sp0 = strrchr(fileName, '/');
    char* sp1 = strrchr(fileName, '\\');
    // 两个符号都存在，则返回较后的那个
    if (sp0 && sp1)
        return sp0 - fileName > sp1 - fileName ? sp0 : sp1;
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
    char* sp = strrchr(fileName, '.');
    if (sp != NULL)
        *sp = '\0';
    strcat(fileName, extname);
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

void appendWString(wchar_t** pstr, int* size, const wchar_t* wstr)
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

int makeDir(const char* dirName)
{
#ifdef WINVER
    return mkdir(dirName);
#else
    return mkdir(dirName, S_IRWXU);
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

int code_convert(const char* from_charset, const char* to_charset, char* inbuf, char* outbuf)
{
    int tag = 0;
    iconv_t cd = iconv_open(to_charset, from_charset);
    if (cd == (iconv_t)-1)
        return -1;
    size_t inlen = strlen(inbuf), outlen = inlen * 2 + 1;
    memset(outbuf, 0, outlen);
    if (iconv(cd, &inbuf, &inlen, &outbuf, &outlen) == (size_t)-1)
        tag = -1;
    iconv_close(cd);
    return tag;
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

void operateDir(const char* dirName, void operateFile(FileInfo, void*), void* ptr, bool recursive)
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

        char name[512];
        if (strlen(dirName) + strlen(dp->d_name) + 2 > sizeof(name)) {
            fprintf(stderr, "dirwalk : name %s %s too long\n", dirName, dp->d_name);
        } else {
            sprintf(name, "%s/%s", dirName, dp->d_name);
            //(*func)(name);

            struct stat stbuf;
            if (stat(name, &stbuf) == -1) {
                fprintf(stderr, "file size: open %s failed\n", name);
                return;
            }

            //如果是目录且要求递归，则递归调用
            if ((stbuf.st_mode & __S_IFMT) == __S_IFDIR) {
                //dirwalk(name, print_file_info); //如果是目录遍历下一级目录
                if (recursive)
                    operateDir(name, operateFile, ptr, recursive);
            } else {
                //printf("%8ld    %s\n", stbuf.st_size, name); //不是目录，打印文件size及name
                FileInfo fi = newFileInfo__(name, stbuf.st_mode, stbuf.st_size);
                operateFile(fi, ptr);
            }
        }
    }
    closedir(dfd);
#endif

#ifdef WINVER
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
#endif
}

static void addFileInfos__(FileInfo fileInfo, void* ptr)
{
    FileInfos fileInfos = (FileInfos)ptr;
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
    operateDir(dirName, addFileInfos__, fileInfos, recursive);
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

/*
#define MAX_PATH 512  //最大文件长度定义为512

// 对目录中所有文件执行print_file_info操作
void dirwalk(char *dir, void (*func)(char *))
{
	char name[MAX_PATH];
	struct dirent *dp;
	DIR *dfd;
	
	if((dfd = opendir(dir)) == NULL){
		fprintf(stderr, "dirwalk: can't open %s\n", dir);
		return;
	}
	
	while((dp = readdir(dfd)) != NULL){ //读目录记录项
		if(strcmp(dp->d_name, ".") == 0 || strcmp(dp -> d_name, "..") == 0){
			continue;  //跳过当前目录以及父目录
		}
		
		if(strlen(dir) + strlen(dp -> d_name) + 2 > sizeof(name)){
			fprintf(stderr, "dirwalk : name %s %s too long\n", dir, dp->d_name);
		}else{
			sprintf(name, "%s/%s", dir, dp->d_name);
			(*func)(name);
		}
	}
	closedir(dfd);
}

// 打印文件信息
void print_file_info(char *name)
{
	struct stat stbuf;
	
	if(stat(name, &stbuf) == -1){
		fprintf(stderr, "file size: open %s failed\n", name);
		return;
	}
	
	if((stbuf.st_mode & S_IFMT) == S_IFDIR){ 
		dirwalk(name, print_file_info);	 //如果是目录遍历下一级目录
	}else{							
		printf("%8ld    %s\n", stbuf.st_size, name);//不是目录，打印文件size及name
	}
}

int main(int argc, char *argv[])
{
	printf("file size    file name\n");
	if(argc == 1){
		print_file_info(".");//未加参数执行时，从当前目录开始遍历
	}else{
		while(--argc>0){
			print_file_info(*++argv);
		}
	}
	
	return 0;
}

*/