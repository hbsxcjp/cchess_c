#include "head/view.h"
#include "head/board.h"
#include "head/instance.h"
#include "head/tools.h"
#include <conio.h>
//#include <termios.h>

// _WIN32 _LINUX

static int getFileIndex(wchar_t* fileNames[], int fileCount, const wchar_t* dirName)
{

    int key, pageIndex = 0, perPageCount = 25, fileIndex = -1,
             pageCount = fileCount / perPageCount + 1;
    while (pageIndex < pageCount) {
        int first = pageIndex++ * perPageCount, last = first + perPageCount;
        if (last > fileCount)
            last = fileCount;
        wchar_t wstr[FILENAME_MAX], pageWstr[FILENAME_MAX * perPageCount];
        swprintf(wstr, FILENAME_MAX, L"%s => 第%3d-%3d个文件：\n\n", dirName, first + 1, last);
        wcscpy(pageWstr, wstr);
        for (int i = first; i < last; ++i) {
            swprintf(wstr, 102, L"%3d. %s\n", i + 1, fileNames[i]);
            wcscat(pageWstr, wstr);
        }
        wcscat(pageWstr, L"\n0-9:选择文件(非数字结束) 非0-9:下页 q:退出\n");
        system("cls");
        wprintf(L"%s\n\n", pageWstr);
        key = getche();
        if (key == 'q')
            break;
        else if (isdigit(key)) {
            char str[10] = { key, 0 };
            int i = 1;
            while ((key = getche()) && isdigit(key) && i < 10)
                str[i++] = key;
            fileIndex = atoi(str) - 1;
            break;
        } else
            continue;
    }
    return fileIndex;
}

void displayInstance(const wchar_t* fileName)
{
    Instance* ins = newInstance();
    char fileName_c[FILENAME_MAX];
    wcstombs(fileName_c, fileName, FILENAME_MAX);
    if (readInstance(ins, fileName_c) == NULL) {
        delInstance(ins);
        return;
    }

    int key = 0;
    while (key != 'q') {
        wchar_t wstr[THOUSAND_SIZE], pageWstr[THOUSAND_SIZE * 8];
        swprintf(pageWstr, FILENAME_MAX, L"%s:\n\n", fileName);
        getBoardString(wstr, ins->board);
        wcscat(pageWstr, wstr);
        swprintf(wstr, 100, L"着法位置(n,o)：(%d,%d) 前着:%c 后着:%c 变着:%c\n\n"
                            L"操作提示:\n空格/n:后着 b/p:前着 g/o:变着 q:退出\n",
            ins->currentMove->nextNo_, ins->currentMove->otherNo_,
            hasPre(ins) ? L'有' : L'无', hasNext(ins) ? L'有' : L'无', hasOther(ins) ? L'有' : L'无');
        wcscat(pageWstr, wstr);
        //wcscat(pageWstr, L"\n操作提示:\n空格/n:后着 b/p:前着 g/o:变着 q:退出\n");
        system("cls");
        wprintf(L"%s\n", pageWstr);

        key = getch();
        if (key == ' ' || key == 'n')
            go(ins);
        else if (key == 'b' || key == 'p')
            back(ins);
        else if (key == 'g' || key == 'o')
            goOther(ins);
    }
    delInstance(ins);
}

void textView(const wchar_t* dirName)
{
#ifdef _WIN32
    //system("cls");
    int fileCount = 0;
    const int maxCount = 1000;
    wchar_t* fileNames[maxCount];
    getFileNames(fileNames, &fileCount, maxCount, dirName);

    int fileIndex = getFileIndex(fileNames, fileCount, dirName);
    if (fileIndex < 0) {
        wprintf(L" 未输入编号.");
        return;
    } else if (fileIndex >= fileCount) {
        wprintf(L" 输入的编号不在可选范围之内.");
        return;
    }
    wprintf(L" line%3d %3d. %s\n", __LINE__, fileIndex, fileNames[fileIndex]);
    displayInstance(fileNames[fileIndex]);

    for (int i = 0; i < fileCount; ++i)
        free(fileNames[i]);

#endif
#ifdef _LINUX

#endif
}