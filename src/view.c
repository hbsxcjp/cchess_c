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
        wcscat(pageWstr, L"\n数字键: 选择文件(非数字结束)   非数字键: 下一页      q: 退出\n");
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
    wchar_t boardStr[HUNDRED_THOUSAND_SIZE];
    system("cls");
    wprintf(L"s\uA9A4\uA9A5\uA9A6\uA9A7\uA9A8\uA9A9e\n");
    wprintf(L"s｜＝－︱﹁﹂―─━│┃═║e\n");
    wprintf(L"s１２３４５６e\n");
    wprintf(L"%sboard：@%p bottomColor:%d\n",
        getBoardString(boardStr, ins->board), *ins->board, ins->board->bottomColor);

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