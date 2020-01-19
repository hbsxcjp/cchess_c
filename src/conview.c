#include "head/conview.h"
#include "head/board.h"
#include "head/instance.h"
#include "head/move.h"
#include "head/tools.h"
#include <windows.h>

#define MENUCOLOR (COLOR_BLUE)
//#define MENUCOLOR (COLOR_CYAN | A_BOLD)
#define MENUREVCOLOR (COLOR_MAGENTA | A_BOLD | A_REVERSE)
#define STATUSCOLOR (COLOR_BLUE | A_BOLD)

#define MAXSTRLEN 256
#define MENUNUM 4
#define MENULEVEL 10
#define MENUWIDTH 9
#define KEY_ESC 0x1b /* Escape */

// 控制台焦点区域类型
typedef enum {
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
    int colIndex, rowIndex;
} Menu;

// 菜单初始数据结构
typedef struct MenuData_ {
    wchar_t name[12]; /* item label */
    MENU_FUNC func; /* (pointer to) function */
    wchar_t desc[50]; /* function description */
} MenuData;

// 本翻译单元的公用变量
static WINDOW *menuWin, *bodyWin, *boardWin, *movesWin, *curmoveWin, *statusWin; // 六个子窗口
static Menu* rootMenu; // 根菜单、选择菜单
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
static Menu* newMenu(const MenuData menuData)
{
    Menu* menu = malloc(sizeof(Menu));
    wcsncpy(menu->name, menuData.name, 10);
    menu->func = menuData.func;
    wcsncpy(menu->desc, menuData.desc, 48);
    menu->preMenu = menu->nextMenu = menu->otherMenu = NULL;
    menu->colIndex = menu->rowIndex = 0;
    return menu;
}

// 增加一个兄弟菜单
static Menu* addOtherMenu(Menu* preMenu, Menu* otherMenu)
{
    otherMenu->preMenu = preMenu;
    otherMenu->colIndex = preMenu->colIndex + 1;
    otherMenu->rowIndex = preMenu->rowIndex;
    return preMenu->otherMenu = otherMenu;
}

// 增加一个子菜单项
static Menu* addNextMenu(Menu* preMenu, Menu* nextMenu)
{
    nextMenu->preMenu = preMenu;
    nextMenu->colIndex = preMenu->colIndex;
    nextMenu->rowIndex = preMenu->rowIndex + 1;
    return preMenu->nextMenu = nextMenu;
}

// 初始化菜单
static void initMenu(void)
{
    MenuData menuDatas[MENUNUM][MENULEVEL] = {
        { { L"文件(F)", NULL, L"新建、打开、保存文件，退出" },
            { L"新建(N)", NULL, L"新建一个棋局" },
            { L"打开...(O)", NULL, L"打开已有的一个棋局" },
            { L"保存(S)", NULL, L"保存正在显示的棋局" },
            { L"退出(X)", NULL, L"退出程序" },
            { L"", NULL, L"" } },
        { { L"棋局(B)", NULL, L"对棋盘局面进行操作" },
            { L"对换棋子(E)", NULL, L"红黑棋子互换" },
            { L"对换位置(R)", NULL, L"红黑位置互换" },
            { L"棋子左右换位(Y)", NULL, L"棋子的位置左右对称换位" },
            { L"", NULL, L"" } },
        { { L"设置(S)", NULL, L"设置显示主题，主要是棋盘、棋子的颜色配置" },
            { L"朴素静雅(G)", NULL, L"比较朴素的颜色配置" },
            { L"鲜艳亮丽(B)", NULL, L"比较鲜艳的颜色配置" },
            { L"高对比度(I)", NULL, L"高对比度的颜色配置" },
            { L"", NULL, L"" } },
        { { L"关于(A)", NULL, L"帮助、程序信息" },
            { L"帮助(H)", NULL, L"显示帮助信息" },
            { L"版本信息(H)", NULL, L"程序有关的信息" },
            { L"", NULL, L"" } }
    };
    rootMenu = newMenu((MenuData){ L"", NULL, L"" });
    Menu *otherMenu = rootMenu, *nextMenu;
    for (int col = 0; col < MENUNUM; ++col) {
        otherMenu = addOtherMenu(otherMenu, newMenu(menuDatas[col][0]));
        nextMenu = otherMenu;
        for (int row = 1; row < MENULEVEL && wcslen(menuDatas[col][row].name) > 0; ++row)
            nextMenu = addNextMenu(nextMenu, newMenu(menuDatas[col][row]));
    }

    // 绘制菜单区域
    while (otherMenu->colIndex > 0) {
        mvwaddwstr(menuWin, 0, (otherMenu->colIndex - 1) * MENUWIDTH + 1, getShowWstr(otherMenu->name));
        otherMenu = otherMenu->preMenu;
    }
    wrefresh(menuWin);
}

// 选定菜单定位至顶层菜单
static void setTopMenu(Menu** pmenu)
{
    while ((*pmenu)->rowIndex > 0)
        *pmenu = (*pmenu)->preMenu;
}

// 选定菜单定位至底层菜单
static void setBottomMenu(Menu** pmenu)
{
    while ((*pmenu)->nextMenu != NULL)
        *pmenu = (*pmenu)->nextMenu;
}

// 选定菜单定位至某一层菜单
static void setRowMenu(Menu** pmenu, int row)
{
    while ((*pmenu)->nextMenu != NULL && row-- > 0)
        *pmenu = (*pmenu)->nextMenu;
}

// 重绘菜单
static void repaintMenu(const WINDOW** pwin, const Menu** pmenu)
{
    mvwchgat(menuWin, 0, 0, -1, getattrs(stdscr), COLOR_BLUE, NULL);

    int startx = ((*pmenu)->colIndex - 1) * MENUWIDTH;
    Menu* tempMenu = *pmenu;
    setBottomMenu(&tempMenu); // 移至最底级菜单
    int height = tempMenu->rowIndex, width = MENUWIDTH;
    while (tempMenu->rowIndex > 0) {
        int len = wcslen(getShowWstr(tempMenu->name));
        width = max(width, len + 2);
        tempMenu = tempMenu->preMenu;
    } // 结束循环后，tempMenu为一级菜单
    WINDOW* win = newwin(height, width, getmaxy(menuWin), startx);

    mvwaddwstr(curmoveWin, 2, 3, getShowWstr((*pmenu)->name));
    char str[80] = { 0 };
    sprintf(str, "sy:%2d sx:%2d hi:%2d wi:%2d", getmaxy(menuWin), startx, height, width);
    mvwaddstr(curmoveWin, 4, 3, str);
    char str1[80] = { 0 };
    sprintf(str1, "by:%2d bx:%2d my:%2d mx:%2d", getbegy(win), getbegx(win), getmaxy(win), getmaxx(win));
    mvwaddstr(curmoveWin, 6, 3, str1);
    wrefresh(curmoveWin);

    while ((tempMenu = tempMenu->nextMenu) != NULL)
        mvwaddwstr(win, tempMenu->rowIndex - 1, 1, getShowWstr(tempMenu->name));
    if ((*pmenu)->rowIndex == 0)
        mvwchgat(menuWin, 0, startx, MENUWIDTH, A_REVERSE, COLOR_BLUE, NULL);
    else
        mvwchgat(win, (*pmenu)->rowIndex - 1, 0, -1, A_REVERSE, COLOR_BLUE, NULL);

    wrefresh(win);
    wrefresh(menuWin);
    *pwin = win;
}

// 操作菜单
static bool operateMenu(const WINDOW** pwin, const Menu** pmenu, int ch)
{
    int row = 0;
    switch (ch) {
    case KEY_ALT_L:
    case KEY_ALT_R:
    case ALT_F:
        *pmenu = rootMenu->otherMenu;
        break;
    case ALT_B:
        *pmenu = rootMenu->otherMenu->otherMenu;
        break;
    case ALT_S:
        *pmenu = rootMenu->otherMenu->otherMenu->otherMenu;
        break;
    case ALT_A:
        *pmenu = rootMenu->otherMenu->otherMenu->otherMenu->otherMenu;
        break;
    case KEY_DOWN:
        if ((*pmenu)->nextMenu != NULL)
            *pmenu = (*pmenu)->nextMenu;
        break;
    case KEY_UP:
        if ((*pmenu)->rowIndex > 0)
            *pmenu = (*pmenu)->preMenu;
        break;
    case KEY_LEFT:
        if ((*pmenu)->colIndex > 1) {
            row = (*pmenu)->rowIndex;
            setTopMenu(pmenu);
            *pmenu = (*pmenu)->preMenu;
            setRowMenu(pmenu, row);
        }
        break;
    case KEY_RIGHT:
        row = (*pmenu)->rowIndex;
        setTopMenu(pmenu);
        if ((*pmenu)->otherMenu != NULL)
            *pmenu = (*pmenu)->otherMenu;
        setRowMenu(pmenu, row);
        break;
    case KEY_PPAGE:
    case KEY_HOME:
        setTopMenu(pmenu);
        break;
    case KEY_NPAGE:
    case KEY_END:
        setBottomMenu(pmenu);
        break;
    case '\n': // 执行菜单命令
        if ((*pmenu)->func != NULL)
            (*pmenu)->func();
        return true;
    default:
        break;
    }

    repaintMenu(pwin, pmenu);
    return false;
}

// 释放菜单资源
static void delMenu(Menu* menu)
{
    if (menu->otherMenu != NULL)
        delMenu(menu->otherMenu);
    if (menu->nextMenu != NULL)
        delMenu(menu->nextMenu);
    free(menu);
}
// 操作棋盘窗口
static void repaintBoard(Instance* ins, int ch)
{
}

// 操作着法窗口
static void repaintMoves(Instance* ins, int ch)
{
}

// 操作当前着法窗口
static void repaintCurMove(Instance* ins, int ch)
{
}

// 重绘状态窗口
static void repaintStatus(FocusArea curArea)
{
    switch (curArea) {
    case BOARDA:
        break;
    case MOVESA:
        break;
    case CURMOVEA:
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
    leaveok(menuWin, TRUE);
    leaveok(bodyWin, TRUE);
    leaveok(boardWin, TRUE);
    leaveok(movesWin, TRUE);
    leaveok(curmoveWin, TRUE);
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

    PDC_return_key_modifiers(true); // 告诉getch（）返回单独按下的修饰键作为击键（KEY_ALT_L等）
    int ch;
    FocusArea curArea = BOARDA, oldArea = curArea;
    WINDOW* win = NULL;
    Menu* menu = rootMenu->otherMenu;
    bool menuDone = false;
    while (true) {
        ch = getch();
        if (win != NULL) {
            werase(win);
            wrefresh(win);
            delwin(win);
        }
        if (ch == KEY_ESC || ch == KEY_F(40) || ch == 0x17) // alt+F4 ctr+w
            break;

        //if (ch == KEY_ESC || ch == KEY_ALT_L || ch == KEY_ALT_R)
        //    return rootMenu;

        switch (ch) {
        case 9: // Tab键
            ++curArea;
            curArea %= 4; // 焦点区域循环
            break;
        case KEY_ALT_L:
        case KEY_ALT_R:
        case ALT_F:
        case ALT_B:
        case ALT_S:
        case ALT_A:
            oldArea = curArea;
            curArea = MENUA;
            menuDone = operateMenu(&win, &menu, ch);
            if (menuDone)
                curArea = oldArea;
            break;
        default:
            switch (curArea) {
            case MENUA:
                menuDone = operateMenu(&win, &menu, ch);
                if (menuDone)
                    curArea = oldArea;
                break;
            case BOARDA:
                repaintBoard(ins, ch);
                break;
            case MOVESA:
                repaintMoves(ins, ch);
                break;
            case CURMOVEA:
                repaintCurMove(ins, ch);
                break;
            default:
                break;
            }
            break;
        }
        if (menuDone)
            ; // 菜单命令执行后，刷新其他区域数据
        repaintStatus(curArea);
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