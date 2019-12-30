#include "head/view.h"
#include "head/board.h"
#include "head/instance.h"
#include "head/move.h"
#include "head/tools.h"
#include <conio.h>
//#include <termios.h>

// _WIN32 _LINUX

static int getFileIndex(wchar_t* fileNames[], int fileCount, const wchar_t* dirName)
{
#define perPageCount 20
    int key, pageIndex = 0, fileIndex = -1,
             pageCount = fileCount / perPageCount + 1;
    while (pageIndex < pageCount) {
        int first = pageIndex++ * perPageCount, last = first + perPageCount;
        if (last > fileCount)
            last = fileCount;
        wchar_t wstr[FILENAME_MAX], pageWstr[FILENAME_MAX * perPageCount];
        swprintf(wstr, FILENAME_MAX, L"%s >> 第%3d～%3d个文件：\n\n", dirName, first + 1, last);
        wcscpy(pageWstr, wstr);
        for (int i = first; i < last; ++i) {
            swprintf(wstr, 102, L"%3d. %s\n", i + 1, fileNames[i]);
            wcscat(pageWstr, wstr);
        }
        wcscat(pageWstr, L"\n0～9:选择编号(非0～9结束)  非0～9:下页  q:退出\n");
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
    wchar_t* BLANKSTR = L"－";
    while (key != 'q') {
        wchar_t wstr[THOUSAND_SIZE * 2], pageWstr[THOUSAND_SIZE * 8];
        swprintf(pageWstr, FILENAME_MAX, L"%s(着数:%d 注数:%d 着深:%d 变深:%d):\n\n",
            fileName, ins->movCount_, ins->remCount_, ins->maxRow_, ins->maxCol_);
        getBoardString(wstr, ins->board);
        wcscat(pageWstr, wstr);
        swprintf(wstr, THOUSAND_SIZE, L"\n操作>>  b/p:%s  空格/n:%s  g/o:%s  q:退出\n"
                L"\n着法>> (%2d,%2d) %c=>%s\n       注解:%s\n",
            hasPre(ins) ? L"前" : BLANKSTR, 
            hasNext(ins) ? L"后" : BLANKSTR, 
            hasOther(ins) ? L"变" : BLANKSTR,
            ins->currentMove->nextNo_, 
            ins->currentMove->otherNo_, 
            (!isStart(ins) ? (getColor_zh(ins->currentMove->zhStr)==RED? L'红' : L'黑'):L'　'),
            (hasNext(ins) ? ins->currentMove->zhStr : BLANKSTR),
            (ins->currentMove->remark ? ins->currentMove->remark : BLANKSTR));
        wcscat(pageWstr, wstr);
        system("cls");
        wprintf(L"%s\n", pageWstr);

        switch (key = getch()) {
        case ' ':
        case 'n':
            go(ins);
            break;
        case 'b':
        case 'p':
            back(ins);
            break;
        case 'g':
        case 'o':
            goOther(ins);
            break;
        default:
            break;
        }
    }
    delInstance(ins);
}

void textView(const wchar_t* dirName)
{
#ifdef _WIN32
#define maxCount 1000
    //system("cls");
    int fileCount = 0;
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