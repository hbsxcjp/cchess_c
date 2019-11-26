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

char* getFileName(char* filename)
{
    int offset = strrchr(filename, '.') - filename;
    filename[offset] = '\x0';
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

int getFiles(char* fileNames[], const char* path)
{
    /* long hFile = 0; //文件句柄
    struct _finddata_t fileinfo; //文件信息
    string p{};
    if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1) {
        do { //如果是目录,迭代之  //如果不是,加入列表
            if (fileinfo.attrib & _A_SUBDIR) {
                if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0)
                    getFiles(p.assign(path).append("\\").append(fileinfo.name), files);
            } else
                files.push_back(p.assign(path).append("\\").append(fileinfo.name));
        } while (_findnext(hFile, &fileinfo) == 0);
        _findclose(hFile);
    }
    */
    return 0;
}

// 测试
// 测试函数
const wchar_t* testTools(wchar_t* wstr)
{
    return wstr;
}
