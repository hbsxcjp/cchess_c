#include "head/view.h"
#include "head/board.h"
#include "head/instance.h"
#include "head/move.h"
#include "head/tools.h"
//#include <conio.h>

// _WIN32 _LINUX
static const wchar_t* BLANKSTR = L"－－";

static void __displayFileList(wchar_t* fileNames[], int first, int last, Pos pos)
{
    wchar_t wstr[THOUSAND_SIZE], pageWstr[THOUSAND_SIZE * 8];
    swprintf(pageWstr, FILENAME_MAX, L"%s >> 第%d～%d个文件：\n\n", fileNames[0], first, last);
    for (int i = first; i <= last; ++i) {
        swprintf(wstr, THOUSAND_SIZE, L"%3d. %s\n", i, fileNames[i]);
        wcscat(pageWstr, wstr);
    }
    swprintf(wstr, THOUSAND_SIZE, L"\n\n操作>>  b/p(回车):%s  空格/n(回车):%s  q(回车):退出\n"
                                  L"\n选择>> 0～9:文件编号(回车)\n",
        pos & FIRST ? BLANKSTR : L"前页",
        pos & END ? BLANKSTR : L"后页");
    wcscat(pageWstr, wstr);
    //system("clear");
    wprintf(L"%s\n\n", pageWstr);
    fflush(stdout);
}

static int getFileIndex(wchar_t* fileNames[], int fileCount)
{
    int key = 0, pageIndex = 0, perPageCount = 20,
        pageCount = (fileCount - 1) / perPageCount + 1,
        first = 1, last = fileCount - 1 < perPageCount ? fileCount - 1 : perPageCount;
    Pos pos = FIRST;
    if (fileCount - 1 <= perPageCount)
        pos |= END;
    __displayFileList(fileNames, first, last, pos);
    while ((key = getchar()) != 'q') {
        switch (key) {
        case ' ':
        case 'n':
            if (pageIndex < pageCount - 1) {
                first = ++pageIndex * perPageCount + 1;
                last = first + perPageCount;
                pos = MIDDLE;
                if (last >= fileCount - 1) {
                    last = fileCount - 1;
                    pos |= END;
                }
                __displayFileList(fileNames, first, last, pos);
            }
            break;
        case 'b':
        case 'p':
            if (pageIndex > 0) {
                first = --pageIndex * perPageCount + 1;
                last = first + perPageCount;
                pos = MIDDLE;
                if (pageIndex == 0)
                    pos |= FIRST;
                __displayFileList(fileNames, first, last, pos);
            }
            break;
        default:
            if (isdigit(key)) {
                char str[10] = { key, 0 };
                int i = 1;
                while ((key = getchar()) && isdigit(key) && i < 10)
                    str[i++] = key;
                int fileIndex = atoi(str);
                if (fileIndex > fileCount - 1)
                    wprintf(L" 输入的编号不在可选范围之内，请重新输入.\n");
                else
                    return fileIndex;
            }
            break;
        }
    }
    return 0;
}

static void __displayInstance(const Instance* ins, const wchar_t* fileName)
{
    wchar_t wstr[THOUSAND_SIZE * 2], pageWstr[THOUSAND_SIZE * 8];
    swprintf(pageWstr, FILENAME_MAX, L"%s(着数:%d 注数:%d 着深:%d 变深:%d):\n\n",
        fileName, ins->movCount_, ins->remCount_, ins->maxRow_, ins->maxCol_);
    getBoardString(wstr, ins->board);
    wcscat(pageWstr, wstr);
    swprintf(wstr, THOUSAND_SIZE, L"\n操作>> b/p(回车):%s  空格/n/回车:%s  g/o(回车):%s  q(回车):退出\n"
                                  L"\n着法>> (%2d,%2d) %c=>%s 注解:%s\n",
        hasPre(ins) ? L"前着" : BLANKSTR,
        hasNext(ins) ? L"后着" : BLANKSTR,
        hasOther(ins) ? L"变着" : BLANKSTR,
        ins->currentMove->nextNo_,
        ins->currentMove->otherNo_,
        (isStart(ins) ? L'－' : (getColor_zh(ins->currentMove->zhStr) == RED ? L'红' : L'黑')),
        (isStart(ins) ? BLANKSTR : ins->currentMove->zhStr),
        (ins->currentMove->remark ? ins->currentMove->remark : BLANKSTR));
    wcscat(pageWstr, wstr);
    //system("clear");
    wprintf(L"%s\n", pageWstr);
    fflush(stdout);
}

void playInstance(const wchar_t* fileName)
{
    char fileName_c[FILENAME_MAX];
    wcstombs(fileName_c, fileName, FILENAME_MAX);
    Instance* ins = newInstance();
    if (readInstance(ins, fileName_c) != NULL) {
        __displayInstance(ins, fileName);
        bool changed = false;
        int key = 0;
        while ((key = getchar()) != 'q') {
            switch (key) {
            case ' ':
            case 'n':
            case '\n':
                go(ins);
                changed = true;
                break;
            case 'b':
            case 'p':
                back(ins);
                changed = true;
                break;
            case 'g':
            case 'o':
                goOther(ins);
                changed = true;
                break;
            default:
                changed = false;
                break;
            }
            if (changed)
                __displayInstance(ins, fileName);
        }
    }
    if (ins != NULL)
        delInstance(ins);
}

void textView(const wchar_t* dirName)
{
#define maxCount 1000
    int fileCount = 0;
    wchar_t* fileNames[maxCount];
    getFileNames(fileNames, &fileCount, maxCount, dirName);

    int fileIndex = getFileIndex(fileNames, fileCount);
    if (fileIndex == 0) {
        wprintf(L" 未输入编号，退出.");
        return;
    }
    //wprintf(L" line%3d %3d. %s\n", __LINE__, fileIndex, fileNames[fileIndex]);
    playInstance(fileNames[fileIndex]);

    for (int i = 0; i < fileCount; ++i)
        free(fileNames[i]);
}