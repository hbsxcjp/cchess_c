#include "head/tools.h"

char* trim(char* str)
{
    size_t size = strlen(str);
    for (int i = size - 1; i >= 0; --i)
        if (isspace(str[i]))
            str[i] = '\x0';
        else
            break;
    size = strlen(str);
    int offset = 0;
    for (int i = 0; i < size; ++i)
        if (isspace(str[i]))
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

char* getFileName_cut(char* filename)
{
    char* pext = strrchr(filename, '.');
    if (pext != NULL)
        *pext = '\0';
    return filename;
}

const char* getExt(const char* filename)
{
    return strrchr(filename, '.');
}

/*
FILE* fin = _wfopen(findName, L"r");
wchar_t* wstr = getWString(fin);
wprintf(L"%s:\n%s\n\n", findName, wstr);
free(wstr);
fclose(fin);
//*/
wchar_t* getWString(FILE* fin)
{
    long start = ftell(fin);
    fseek(fin, 0, SEEK_END);
    long end = ftell(fin);
    fseek(fin, start, SEEK_SET);
    wchar_t* wStr = malloc((end - start + 1) * sizeof(wchar_t));
    int index = 0;
    while (!feof(fin))
        wStr[index++] = fgetwc(fin);
    wStr[index] = L'\x0';
    return wStr;
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

void getFileNames(wchar_t* fileNames[], int* fileCount, int maxCount, const wchar_t* dirName)
{
    if (*fileCount == maxCount) {
        wprintf(L"文件数量已达到最大值。\n");
        return;
    }
    long hFile = 0; //文件句柄
    struct _wfinddata_t fileinfo; //文件信息
    wchar_t findDirName[FILENAME_MAX] = { 0 };
    wcscpy(findDirName, dirName);
    //wprintf(L"%d: %s\n", __LINE__, findDirName);
    if ((hFile = _wfindfirst(wcscat(findDirName, L"\\*"), &fileinfo)) == -1)
        return;
    do {
        if (wcscmp(fileinfo.name, L".") == 0 || wcscmp(fileinfo.name, L"..") == 0)
            continue;
        wchar_t* findName = malloc(FILENAME_MAX);
        wcscat(wcscat(wcscpy(findName, dirName), L"\\"), fileinfo.name);
        //wprintf(L"%d: %s\n", __LINE__, findName);
        fileNames[(*fileCount)++] = findName;
        if (fileinfo.attrib & _A_SUBDIR) //如果是目录,迭代之
            getFileNames(fileNames, fileCount, maxCount, findName);
    } while (_wfindnext(hFile, &fileinfo) == 0);
    _findclose(hFile);
}

// 测试函数
void testTools(void)
{
    const wchar_t* paths[] = {
        L"c:\\棋谱\\示例文件.pgn_zh",
        L"c:\\棋谱\\象棋杀着大全.pgn_zh",
        L"c:\\棋谱\\疑难文件.pgn_iccs",
        //L"c:\\棋谱\\中国象棋棋谱大全"
    };
    int sum = 0;
    for (int i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i) {
        int count = 0;
        wchar_t* fileNames[THOUSAND_SIZE];
        getFileNames(fileNames, &count, 1000, paths[i]);
        for (int i = 0; i < count; ++i) {
            wprintf(L"%d: %s\n", __LINE__, fileNames[i]);
            free(fileNames[i]);
        }
        sum += count;
        wprintf(L"%s 包含: %d个文件。\n\n", paths[i], count);
    }
    wprintf(L"总共包括:%d个文件。\n", sum);
}
