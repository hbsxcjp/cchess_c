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

void getFiles(char* fileNames[], int* pcount, const char* path)
{
    long hFile = 0; //文件句柄
    struct _finddata_t fileinfo; //文件信息
    char dirName[FILENAME_MAX] = { 0 };
    strcpy(dirName, path);
    //printf("%d: %s\n", __LINE__, dirName);

    if ((hFile = _findfirst(strcat(dirName, "\\*"), &fileinfo)) == -1)
        return;

    do { //如果是目录,迭代之  //如果不是,加入列表
        char findName[FILENAME_MAX];
        strcat(strcat(strcpy(findName, path), "\\"), fileinfo.name);
        //printf("%d: %s\n", __LINE__, findName);
        if (fileinfo.attrib & _A_SUBDIR) {
            if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0)
                getFiles(fileNames, pcount, findName);
        } else {
            char* fileName = malloc(FILENAME_MAX);
            strcpy(fileName, findName);
            //printf("%d: %s\n", __LINE__, fileName);
            fileNames[(*pcount)++] = fileName;
        }
    } while (_findnext(hFile, &fileinfo) == 0);
    _findclose(hFile);
}

// 测试函数
void testTools(void)
{
    const wchar_t wpath[] = L"c:\\棋谱\\象棋杀着大全.xqf";
    //L"c:\\棋谱\\示例文件"
    //L"c:\\棋谱\\象棋杀着大全"
    //L"c:\\棋谱\\疑难文件"
    //L"c:\\棋谱\\中国象棋棋谱大全"

    int count = 0;
    char path[FILENAME_MAX];
    char* fileNames[THOUSAND_SIZE];
    wcstombs(path, wpath, FILENAME_MAX);
    getFiles(fileNames, &count, path);
    for (int i = 0; i < count; ++i)
        printf("%s %d: %s\n", __FILE__, __LINE__, fileNames[i]);
    printf("%s %d: %s count:%d\n", __FILE__, __LINE__, path, count);

    for (int i = 0; i < count; ++i)
        free(fileNames[i]);
}
