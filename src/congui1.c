#include <locale.h>
#include <stdio.h>
#include <windows.h>

int color(int c);
void gotoxy(int x, int y);

int main()
{
    setlocale(LC_ALL, "");
    PCONSOLE_SCREEN_BUFFER_INFO screenInfo;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), screenInfo);
    system("cls");

    /*
    for (int i = 0; i < 16; i++) {
        color(i);
        wprintf(L"这是第%2d号颜色┏━┯━┯━┯━┯━┯━┯━┯━┓\n", i);
        //wprintf(L"这是第%2d号颜色＋－－－－－－－－－－－－－－－＋\n", i);
    }
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), screenInfo->wAttributes);

    for (int i = 0; i < 16; i++) {
        gotoxy(50, i);
        printf("hello world!");
    }
    //*/

    //*
    wchar_t line[] = L"┏━┯━┯━┯━┯━┯━┯━┯━┓\n";
    int len = wcslen(line);
    for (int i = 0; i < len; ++i) {
        wprintf(L"%c", line[i]);
        wprintf(L"%x %c - ", line[i], line[i]);
    }

    wchar_t line1[] = L"＋－－－－－－－－－－－－－－－＋\n";
    int len1 = wcslen(line1);
    for (int i = 0; i < len1; ++i) {
        wprintf(L"%c", line1[i]);
        wprintf(L"%x %c - ", line1[i], line1[i]);
    } //*/
    //*/
    return 0;
}

int color(int c)
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), c);
    return 0;
}

void gotoxy(int x, int y)
{
    COORD pos;
    pos.X = x;
    pos.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}