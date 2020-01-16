#include "head/conview.h"
#include "head/board.h"
#include "head/instance.h"
#include "head/move.h"
#include "head/tools.h"
#include <windows.h>

#define MENUCOLOR (COLOR_RED)
//#define MENUCOLOR (COLOR_CYAN | A_BOLD)
#define MENUREVCOLOR (COLOR_MAGENTA | A_BOLD | A_REVERSE)
#define STATUSCOLOR (COLOR_BLUE | A_BOLD)

#define MAXSTRLEN 256
#define MENULEVEL 10
#define MENUWIDTH 9
#define KEY_ESC 0x1b /* Escape */

// 控制台焦点区域类型
typedef enum {
    BODYA,
    MENUA,
    BOARDA,
    MOVESA,
    CURMOVEA
} FocusArea;

// 菜单命令
typedef void (*MENU_FUNC)(void);
// 菜单结构
typedef struct Menu_ {
    wchar_t name[12]; /* item label */
    MENU_FUNC func; /* (pointer to) function */
    wchar_t desc[50]; /* function description */
    struct Menu_ *preMenu, *nextMenu, *otherMenu;
    int menuIndex, rowIndex;
} Menu;

// 本翻译单元的公用变量
static WINDOW *menuWin, *bodyWin, *boardWin, *movesWin, *curmoveWin, *statusWin; // 六个子窗口
static Menu *rootMenu, *selectMenu; // 根菜单、选择菜单
static wchar_t wstr[THOUSAND_SIZE * 2], showWstr[THOUSAND_SIZE * 8]; // 临时字符串

// 使用公用变量，获取用于屏幕显示的字符串（经试验，只有在全角字符后插入空格，才能显示正常）
static wchar_t* getShowWstr(const wchar_t* srcWstr)
{
    int srcIndex = 0, desIndex = 0, srcLen = wcslen(srcWstr);
    while (srcIndex < srcLen) {
        wchar_t wch = srcWstr[srcIndex++];
        showWstr[desIndex++] = wch;
        if (wch > 255) // 非ASCII字符
            showWstr[desIndex++] = L' ';
    }
    showWstr[desIndex] = L'\x0';
    return showWstr;
}

// 生成一个菜单
static Menu* newMenu(wchar_t* name, MENU_FUNC func, wchar_t* desc)
{
    Menu* menu = malloc(sizeof(Menu));
    wcsncpy(menu->name, name, 10);
    menu->func = func;
    wcsncpy(menu->desc, desc, 48);
    menu->preMenu = menu->nextMenu = menu->otherMenu = NULL;
    menu->menuIndex = menu->rowIndex = 0;
    return menu;
}

// 增加一个子菜单项
static Menu* addNextMenu(Menu* preMenu, Menu* nextMenu)
{
    nextMenu->preMenu = preMenu;
    nextMenu->menuIndex = preMenu->menuIndex;
    nextMenu->rowIndex = preMenu->rowIndex + 1;
    return preMenu->nextMenu = nextMenu;
}

// 增加一个兄弟菜单
static Menu* addOtherMenu(Menu* preMenu, Menu* otherMenu)
{
    otherMenu->preMenu = preMenu;
    otherMenu->menuIndex = preMenu->menuIndex + 1;
    otherMenu->rowIndex = preMenu->rowIndex;
    return preMenu->otherMenu = otherMenu;
}

// 初始化菜单
static void initMenu(void)
{
    rootMenu = newMenu(L"", NULL, L"");
    Menu* file = addNextMenu(rootMenu, newMenu(L"文件(F)", NULL, L"新建、打开、保存文件，退出"));
    Menu* file_new = addNextMenu(file, newMenu(L"新建(N)", NULL, L"新建一个棋局"));
    Menu* file_open = addNextMenu(file_new, newMenu(L"打开(O)", NULL, L"打开已有的一个棋局"));
    Menu* file_save = addNextMenu(file_open, newMenu(L"保存(S)", NULL, L"保存正在显示的棋局"));
    addNextMenu(file_save, newMenu(L"退出(X)", NULL, L"退出程序"));

    Menu* board = addOtherMenu(file, newMenu(L"棋局(B)", NULL, L"对棋盘局面进行操作"));
    Menu* board_exc = addNextMenu(board, newMenu(L"换棋(E)", NULL, L"红黑棋子互换"));
    Menu* board_rot = addNextMenu(board_exc, newMenu(L"换位(R)", NULL, L"红黑位置互换"));
    addNextMenu(board_rot, newMenu(L"左右(Y)", NULL, L"左右棋子互换"));

    Menu* option = addOtherMenu(board, newMenu(L"设置(S)", NULL, L"设置显示主题，主要是棋盘、棋子的颜色配置"));
    Menu* option_gen = addNextMenu(option, newMenu(L"普通(G)", NULL, L"比较朴素的颜色配置"));
    Menu* option_bea = addNextMenu(option_gen, newMenu(L"漂亮(B)", NULL, L"比较鲜艳的颜色配置"));
    addNextMenu(option_bea, newMenu(L"高亮(I)", NULL, L"高对比度的颜色配置"));

    Menu* about = addOtherMenu(option, newMenu(L"关于(A)", NULL, L"帮助、程序信息"));
    Menu* about_help = addNextMenu(about, newMenu(L"帮助(H)", NULL, L"显示帮助信息"));
    addNextMenu(about_help, newMenu(L"程序(H)", NULL, L"程序有关的信息"));

    selectMenu = rootMenu;
    int index = 0;
    Menu* tempMenu = rootMenu->nextMenu;
    // 绘制菜单区域
    while (tempMenu != NULL) {
        mvwaddwstr(menuWin, 0, 1 + (index++) * MENUWIDTH, getShowWstr(tempMenu->name));
        tempMenu = tempMenu->otherMenu;
    }
    wrefresh(menuWin);
}

// 重绘一级菜单，返回子菜单窗口
static WINDOW* repaintMenu(void)
{
    WINDOW* win = NULL;
    if (selectMenu == rootMenu)
        mvwchgat(menuWin, 0, 0, -1, A_STANDOUT, MENUCOLOR, NULL);
    else {
        int menuIndex = selectMenu->menuIndex;
        mvwchgat(menuWin, 0, menuIndex * MENUWIDTH, MENUWIDTH, A_REVERSE, MENUCOLOR, NULL);
        if (selectMenu->preMenu != rootMenu) { // 若不是一级菜单项
            int row = 0, maxCol = 0;
            Menu* tempMenu = selectMenu;
            while (tempMenu->preMenu != rootMenu) {
                maxCol = max(maxCol, wcslen(getShowWstr(tempMenu->name)));
                tempMenu = tempMenu->preMenu;
            } // 结束循环后，tempMenu为一级菜单
            win = newwin(selectMenu->rowIndex + 1, maxCol + 2, 1, menuIndex * MENUWIDTH);
            while (tempMenu != NULL) {
                mvwaddwstr(win, row++, 1 + menuIndex * MENUWIDTH, getShowWstr(tempMenu->name));
                tempMenu = tempMenu->nextMenu;
            }
            mvwchgat(win, selectMenu->rowIndex, 0, -1, A_REVERSE, MENUCOLOR, NULL);
        }
    }
    wrefresh(win);
    return win;
}

// 操作菜单
static void operateMenu(void)
{
    int ch;
    char keystr[64] = { 0 };
    while ((ch = getch()) != KEY_ESC) {
        sprintf(keystr, "input(ch):%3x", ch);
        mvwaddstr(curmoveWin, 1, 2, keystr);
        wrefresh(curmoveWin);

        switch (ch) {
        //case KEY_ALT_L:
        //case KEY_ALT_R:
        case KEY_DOWN:
            if (selectMenu->nextMenu != NULL)
                selectMenu = selectMenu->nextMenu;
            repaintMenu();
            break;
        case KEY_UP:
            if (selectMenu->preMenu != NULL)
                selectMenu = selectMenu->preMenu;
            repaintMenu();
            break;
        }
    }
}

// 释放菜单资源
static void delMenu(Menu* menu)
{
    if (menu->nextMenu != NULL)
        delMenu(menu->nextMenu);
    if (menu->otherMenu != NULL)
        delMenu(menu->otherMenu);
    free(menu);
}

// 重绘主体窗口
static void repaintBody(void) {}

// 重绘棋盘窗口
static void repaintBoard(void) {}

// 重绘着法窗口
static void repaintMoves(void) {}

// 重绘当前着法窗口
static void repaintCurMove(void) {}

// 重绘状态窗口
static void repaintStatus(FocusArea area)
{
    switch (area) {
    case MENUA:
        //mvwaddwstr(statusWin, 0, 1, selectMenu->name);
        break;

    default:
        break;
    }
}

// 显示一个棋局实例
static void viewInstance(Instance* ins, const wchar_t* fileName)
{
    char fileName_c[FILENAME_MAX];
    wcstombs(fileName_c, fileName, FILENAME_MAX);

    //*
    if (readInstance(ins, fileName_c) != NULL) {
        //SetConsoleOutputCP(936); // 设置代码页 <windows.h>
        SetConsoleTitle(fileName_c); // 设置窗口标题 <windows.h>

        mvwaddstr(bodyWin, 0, 1, "=>");
        mvwaddwstr(bodyWin, 0, 4, getShowWstr(fileName));

        getBoardString(wstr, ins->board);
        mvwaddwstr(boardWin, 0, 0, getShowWstr(wstr));

        swprintf(wstr, FILENAME_MAX, L"  着数：%d 注数：%d 着深：%d 变深：%d",
            ins->movCount_, ins->remCount_, ins->maxRow_, ins->maxCol_);
        mvwaddwstr(movesWin, 2, 2, getShowWstr(wstr));

        box(movesWin, 0, 0);
        mvwaddstr(movesWin, 0, 1, " 着 法 图 示 ");

        box(curmoveWin, 0, 0);
        mvwaddstr(curmoveWin, 0, 1, " 当 前 着 法 ");

        box(statusWin, 0, 0);
        mvwaddstr(statusWin, 0, 1, " 状 态 窗 口 ");

        touchwin(stdscr);
        wrefresh(stdscr);
    } //*/
}

// 启动窗口
static void startWin()
{
    initscr();
    start_color();

    int bodyHeight = LINES - 4, bodyWidth = COLS - 1,
        boardHeight = 4 + 19 * 1 + 4, boardWidth = 2 + 17 * 2 + 2,
        movesHeight = bodyHeight - 3, movesWidth = COLS - boardWidth - 8;
    menuWin = subwin(stdscr, 1, 4 * 20, 0, 0);
    bodyWin = subwin(stdscr, bodyHeight, bodyWidth, 2, 0);
    boardWin = subwin(bodyWin, boardHeight, boardWidth, 4, 4);
    movesWin = subwin(bodyWin, movesHeight, movesWidth, 4, boardWidth + 5);
    curmoveWin = subwin(bodyWin, movesHeight - boardHeight, boardWidth, boardHeight + 4, 2);
    statusWin = subwin(stdscr, 2, bodyWidth, LINES - 2, 0);

    //wbkgd(win, attr);
    initMenu();

    cbreak(); /* direct input (no newline required)... */
    noecho(); /* ... without echoing */
    curs_set(0); /* hide cursor (if possible) */
    //nodelay(wbody, TRUE); /* don't wait for input... */
    //halfdelay(10); /* ...well, no more than a second, anyway */
    keypad(stdscr, TRUE); /* enable cursor keys */
    scrollok(movesWin, TRUE); /* enable scrolling in main window */

    leaveok(stdscr, TRUE);
    //touchwin(stdscr);
    //wrefresh(stdscr);
}

// 结束窗口
static void finishWin(void)
{
    delMenu(rootMenu);
    delwin(boardWin);
    delwin(movesWin);
    delwin(curmoveWin);
    delwin(statusWin);
    delwin(bodyWin);
    delwin(menuWin);
    curs_set(1);
    endwin();
}

// 执行棋局演示
void doView(void)
{
    startWin();

    wchar_t fileName[] = L"01.XQF";
    Instance* ins = newInstance();
    viewInstance(ins, fileName);

    int ch;
    unsigned long keym;
    PDC_return_key_modifiers(true);
    while (true) {
        ch = getch();
        if (ch == 'q')
            break;
        operateMenu();
        /*
        switch (ch) {
        case KEY_ALT_L:
        case KEY_ALT_R:
        case KEY_DOWN:
            curFocusArea = MENUA;
            if (selectMenu->nextMenu != NULL)
                selectMenu = selectMenu->nextMenu;
            //repaintMenu();
            break;
        case KEY_UP:
            curFocusArea = MENUA;
            if (selectMenu != NULL)
                selectMenu = selectMenu->preMenu;
            //repaintMenu();
            break;
        case CTL_TAB:
            curFocusArea = (curFocusArea + 1) % 5;

            //wclear(statusWin);
            mvwaddstr(statusWin, 1, 1, "CTL_TAB");
            break;
        default:
            switch (curFocusArea) {
            case BODYA:
                repaintBody();
                break;
            case MENUA:
                keym = PDC_get_key_modifiers();
                if (keym == KEY_ALT_L || keym == KEY_ALT_R) {
                    //wclear(statusWin);
                    mvwaddstr(statusWin, 1, 1, "KEY_ALT_L || KEY_ALT_R");
                } else {
                    //wclear(statusWin);
                    mvwaddstr(statusWin, 1, 1, "not (KEY_ALT_L || KEY_ALT_R)");
                }
                //repaintMenu();
                break;
            case BOARDA:
                repaintBoard();
                break;
            case MOVESA:
                repaintMoves();
                break;
            case CURMOVEA:
                repaintCurMove();
                break;
            default:
                break;
            }
            break;
        }
        repaintStatus(curFocusArea);
        //*/
    }

    delInstance(ins);
    finishWin();
}

// 测试翻译单元
void testConview(void)
{
    Instance* ins = newInstance();
    readInstance(ins, "01.XQF");
    delInstance(ins);
}

/*
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
//*/