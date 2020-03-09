#include "head/tools.h"

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
