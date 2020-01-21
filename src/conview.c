#include "head/conview.h"
#include "head/board.h"
#include "head/instance.h"
#include "head/move.h"
#include "head/tools.h"
#include <windows.h>

//#define MENUCOLOR (COLOR_BLUE)
#define MENUCOLOR (COLOR_CYAN | A_BOLD)
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
static WINDOW *menuWin, *statusWin, *tempWin = NULL,
                                    *boardWin, *movesWin, *curMoveWin,
                                    *iboardWin, *imovesWin, *icurMoveWin;
static FocusArea curArea = MOVESA, oldArea;
static Menu *rootMenu, *selectMenu; // 根菜单
static Instance* ins = NULL;
static wchar_t fileName[FILENAME_MAX], wstr[THOUSAND_SIZE * 2], showWstr[THOUSAND_SIZE * 8]; // 临时字符串

//static void repaintBoard(void);
static void repaintInstance(void);
static void finishWin(void);

static void destroyTempWin(void)
{
    if (tempWin != NULL) {
        werase(tempWin);
        wrefresh(tempWin);
        delwin(tempWin);
        tempWin = NULL;
        repaintInstance();
    }
}

// 使用公用变量，获取用于屏幕显示的字符串（经试验，只有在全角字符后插入空格，才能显示正常）
static wchar_t* getShowWstr(const wchar_t* srcWstr)
{
    int srcIndex = 0, desIndex = 0;
    wchar_t wch;
    while ((wch = srcWstr[srcIndex++]) != L'\x0') {
        showWstr[desIndex++] = wch;
        // 非ASCII字符
        if (wch > 255) {
            showWstr[desIndex++] = L' ';
            // 调整'↓'的位置
            if (wch == L'↓') {
                showWstr[desIndex - 3] = wch;
                showWstr[desIndex - 2] = L' ';
            }
        }
    }
    //showWstr[desIndex++] = L'.'; // 消除刷新后遗留半边汉字的情况
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

// 退出程序
static void goodbye(void)
{
    finishWin();
    exit(0);
}

// 初始化菜单
static void initMenu(void)
{
    MenuData menuDatas[MENUNUM][MENULEVEL] = {
        { { L"文件(F)", NULL, L"新建、打开、保存文件，退出" },
            { L"新建", NULL, L"新建一个棋局" },
            { L"打开...", NULL, L"打开已有的一个棋局" },
            { L"另存为...", NULL, L"将显示的棋局另存为一个棋局" },
            { L"保存", NULL, L"保存正在显示的棋局" },
            { L"退出", goodbye, L"退出程序" },
            { L"", NULL, L"" } },
        { { L"棋局(B)", NULL, L"对棋盘局面进行操作" },
            { L"对换棋局", NULL, L"红黑棋子互换" },
            { L"对换位置", NULL, L"红黑位置互换" },
            { L"棋子左右换位", NULL, L"棋子的位置左右对称换位" },
            { L"", NULL, L"" } },
        { { L"设置(S)", NULL, L"设置显示主题，主要是棋盘、棋子的颜色配置" },
            { L"静雅朴素", NULL, L"比较朴素的颜色配置" },
            { L"鲜艳亮丽", NULL, L"比较鲜艳的颜色配置" },
            { L"高对比度", NULL, L"高对比度的颜色配置" },
            { L"", NULL, L"" } },
        { { L"关于(A)", NULL, L"帮助、程序信息" },
            { L"帮助", NULL, L"显示帮助信息" },
            { L"版本信息", NULL, L"程序有关的信息" },
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
    selectMenu = rootMenu->otherMenu;

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
static void repaintMenu()
{
    mvwchgat(menuWin, 0, 0, -1, getattrs(stdscr), COLOR_BLUE, NULL);

    if (selectMenu != rootMenu) {
        int startx = (selectMenu->colIndex - 1) * MENUWIDTH;
        Menu* tempMenu = selectMenu;
        setBottomMenu(&tempMenu); // 移至最底级菜单
        int height = tempMenu->rowIndex, width = MENUWIDTH;
        while (tempMenu->rowIndex > 0) {
            int len = wcslen(getShowWstr(tempMenu->name));
            width = max(width, len + 2);
            tempMenu = tempMenu->preMenu;
        } // 结束循环后，tempMenu为一级菜单
        destroyTempWin();
        tempWin = newwin(height, width, getmaxy(menuWin), startx);

        while ((tempMenu = tempMenu->nextMenu) != NULL)
            mvwaddwstr(tempWin, tempMenu->rowIndex - 1, 1, getShowWstr(tempMenu->name));
        if (selectMenu->rowIndex == 0)
            mvwchgat(menuWin, 0, startx, MENUWIDTH, A_REVERSE, COLOR_BLUE, NULL);
        else
            mvwchgat(tempWin, selectMenu->rowIndex - 1, 0, -1, A_REVERSE, COLOR_BLUE, NULL);
        wrefresh(tempWin);
    }

    wrefresh(menuWin);
}

// 操作菜单
static bool operateMenu(int ch)
{
    int row = 0;
    switch (ch) {
    case KEY_ALT_L:
    case KEY_ALT_R:
        if (curArea == MENUA)
            return true;
    case ALT_F:
        selectMenu = rootMenu->otherMenu;
        break;
    case ALT_B:
        selectMenu = rootMenu->otherMenu->otherMenu;
        break;
    case ALT_S:
        selectMenu = rootMenu->otherMenu->otherMenu->otherMenu;
        break;
    case ALT_A:
        selectMenu = rootMenu->otherMenu->otherMenu->otherMenu->otherMenu;
        break;
    case KEY_DOWN:
        if (selectMenu->nextMenu != NULL)
            selectMenu = selectMenu->nextMenu;
        break;
    case KEY_UP:
        if (selectMenu->rowIndex > 0)
            selectMenu = selectMenu->preMenu;
        break;
    case KEY_LEFT:
        if (selectMenu->colIndex > 1) {
            row = selectMenu->rowIndex;
            setTopMenu(&selectMenu);
            selectMenu = selectMenu->preMenu;
            setRowMenu(&selectMenu, row);
        }
        break;
    case KEY_RIGHT:
        row = selectMenu->rowIndex;
        setTopMenu(&selectMenu);
        if (selectMenu->otherMenu != NULL)
            selectMenu = selectMenu->otherMenu;
        setRowMenu(&selectMenu, row);
        break;
    case KEY_PPAGE:
    case KEY_HOME:
        setTopMenu(&selectMenu);
        break;
    case KEY_NPAGE:
    case KEY_END:
        setBottomMenu(&selectMenu);
        break;
    case '\n': // 执行菜单命令
        if (selectMenu->func != NULL)
            (selectMenu->func)();
        return true;
    default:
        break;
    }

    repaintMenu();
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

// 绘制棋盘窗口
static void repaintBoard()
{
    box(boardWin, 0, 0);
    //box(iboardWin, 0, 0);
    getBoardString(wstr, ins->board);
    mvwaddwstr(iboardWin, 0, 0, getShowWstr(wstr));

    touchwin(boardWin);
    wrefresh(boardWin);
}

// 绘制着法窗口
static void repaintMoves()
{
    box(movesWin, 0, 0);
    //box(imovesWin, 0, 0);
    //mvwaddstr(movesWin, 0, 1, " 着 法 图 示 ");

    wchar_t* lineStr = NULL;
    writeMove_PGN_CCtoWstr(ins, &lineStr);
    mvwaddwstr(imovesWin, 0, 0, getShowWstr(lineStr));
    free(lineStr);

    touchwin(movesWin);
    wrefresh(movesWin);
}

// 绘制当前着法窗口
static void repaintCurMove()
{
    box(curMoveWin, 0, 0);
    //box(icurMoveWin, 0, 0);
    //mvwaddstr(icurMoveWin, 0, 0, "当 前 着 法 ");

    wchar_t* remarkStr = NULL;
    writeRemark_PGN_CCtoWstr(ins, &remarkStr);
    mvwaddwstr(icurMoveWin, 0, 0, getShowWstr(remarkStr));
    free(remarkStr);

    touchwin(curMoveWin);
    wrefresh(curMoveWin);
}

// 操作棋盘窗口
static void operateBoard(int ch)
{
}

// 操作着法窗口
static void operateMoves(int ch)
{
}

// 操作当前着法窗口
static void operateCurMove(int ch)
{
}

// 重绘状态窗口
static void repaintStatus()
{
    wclear(statusWin);
    //werase(statusWin);
    //box(statusWin, 0, 0);

    //wmove(statusWin, 0, 0);
    //wclrtoeol(statusWin);
    //wmove(statusWin, 1, 0);
    //wclrtoeol(statusWin);

    switch (curArea) {
    case MENUA:
        swprintf(wstr, FILENAME_MAX, L"\n【菜单】<%s> %s", selectMenu->name, selectMenu->desc);
        break;
    case BOARDA:
        swprintf(wstr, FILENAME_MAX, L"\n【棋盘】使用箭头键移动光标在棋盘上的位置 h%3d w%3d", LINES, COLS);
        break;
    case MOVESA:
        swprintf(wstr, FILENAME_MAX, L" =>%s\n【棋着】着数:%2d 注数:%2d 着深:%2d 变深:%2d",
            ins->currentMove->zhStr, ins->movCount_, ins->remCount_, ins->maxRow_, ins->maxCol_);
        break;
    case CURMOVEA:
        swprintf(wstr, FILENAME_MAX, L"\n【注解】%s", ins->currentMove->remark ? ins->currentMove->remark : L"");
        break;
    default:
        break;
    }
    //mvwaddstr(statusWin, 0, 0, "\n【棋盘】使用箭头键移动光标在棋盘上的位置");
    //mvwaddwstr(statusWin, 0, 0, wstr);
    mvwaddwstr(statusWin, 0, 0, getShowWstr(wstr));
    wrefresh(statusWin);
}

// 重绘一个棋局实例
static void repaintInstance()
{
    repaintBoard();
    repaintMoves();
    repaintCurMove();
}

// 启动窗口
static bool startWin()
{
    initscr();
    start_color();
    //wbkgd(win, attr);

    cbreak(); /* direct input (no newline required)... */
    noecho(); /* ... without echoing */
    curs_set(0); /* hide cursor (if possible) */
    //nodelay(wbody, TRUE); /* don't wait for input... */
    //halfdelay(10); /* ...well, no more than a second, anyway */
    keypad(stdscr, TRUE); /* enable cursor keys */
    scrollok(movesWin, TRUE); /* enable scrolling in main window */
    scrollok(curMoveWin, TRUE); /* enable scrolling in main window */

    if (COLS < 120 || LINES < 40) {
        swprintf(wstr, FILENAME_MAX, L"程序要求窗口高度＞40、宽度＞120，当前窗口高度：%d 宽度：%d",
            LINES, COLS);
        getShowWstr(wstr);
        int startx = (COLS - wcslen(showWstr)) / 2;
        if (startx < 0)
            startx = 0;
        mvaddwstr(LINES / 2, startx, showWstr);
        getShowWstr(L"调整后，请关闭窗口，再重新打开。");
        mvaddwstr(LINES / 2 + 2, (COLS - wcslen(showWstr)) / 2, showWstr);
        refresh();
        getch();
        return false;
    }

    int boardHeight = 4 + 19 * 1 + 4, boardWidth = 2 + 17 * 2 + 2,
        movesHeight = LINES - 3, movesWidth = COLS - boardWidth;
    menuWin = newwin(1, COLS, 0, 0);
    init_pair(1, COLOR_BLUE, COLOR_YELLOW);
    wbkgd(menuWin, COLOR_PAIR(1));

    boardWin = newwin(boardHeight, boardWidth, 1, 0);
    iboardWin = subwin(boardWin, boardHeight - 2, boardWidth - 3, 2, 2); // 为打印图形周边留出空白
    movesWin = newwin(movesHeight, movesWidth, 1, boardWidth);
    imovesWin = subwin(movesWin, movesHeight - 2, movesWidth - 4, 2, boardWidth + 2);
    curMoveWin = newwin(movesHeight - boardHeight, boardWidth, boardHeight + 1, 0);
    icurMoveWin = subwin(curMoveWin, movesHeight - boardHeight - 2, boardWidth - 2, boardHeight + 2, 1);
    statusWin = newwin(2, COLS, LINES - 2, 0);

    leaveok(stdscr, TRUE);
    leaveok(menuWin, TRUE);
    leaveok(boardWin, TRUE);
    leaveok(movesWin, TRUE);
    leaveok(curMoveWin, TRUE);

    touchwin(stdscr);
    refresh();
    return true;
}

// 结束窗口
static void finishWin(void)
{
    delInstance(ins);
    delMenu(rootMenu);
    if (tempWin != NULL)
        delwin(tempWin);
    delwin(menuWin);
    delwin(iboardWin);
    delwin(boardWin);
    delwin(imovesWin);
    delwin(movesWin);
    delwin(icurMoveWin);
    delwin(curMoveWin);
    delwin(statusWin);
    curs_set(1);
    endwin();
}

// 打开一个棋局文件
static void openFile()
{
    char fileName_c[FILENAME_MAX];
    wcstombs(fileName_c, fileName, FILENAME_MAX);
    //SetConsoleOutputCP(936); // 设置代码页 <windows.h>
    SetConsoleTitle(fileName_c); // 设置窗口标题 <windows.h>

    delInstance(ins);
    ins = newInstance();
    readInstance(ins, fileName_c);
}

// 执行棋局演示
void doView(void)
{
    if (!startWin())
        return;
    initMenu();
    ins = newInstance();

    wcscpy(fileName, L"01.XQF");
    openFile();
    repaintInstance();
    //getch();

    // 告诉getch（）返回单独按下的修饰键作为击键（KEY_ALT_L等）
    PDC_return_key_modifiers(true);
    int ch;
    //bool menuDone = false;
    while (true) {
        ch = getch();
        if (ch == KEY_F(40) || ch == 0x17) // alt+F4 ctr+w
            break;

        //*
        switch (ch) { // 快捷键选择区域
        case KEY_ALT_L:
        case KEY_ALT_R:
        case ALT_F:
        case ALT_B:
        case ALT_S:
        case ALT_A:
            oldArea = curArea;
            operateMenu(ch);
            curArea = MENUA;
            break;
        case KEY_ESC:
            curArea = MOVESA;
            break;
        case 9: // Tab键
            ++curArea;
            if (curArea == 4)
                curArea = MENUA; // 焦点区域循环
            break;
        default:
            switch (curArea) { // 区分不同区域, 进行操作
            case MENUA:
                if (operateMenu(ch))
                    curArea = oldArea;
                break;
            case BOARDA:
                operateBoard(ch);
                break;
            case MOVESA:
                operateMoves(ch);
                break;
            case CURMOVEA:
                operateCurMove(ch);
                break;
            default:
                break;
            }
        }
        repaintStatus();
        //*/
        //repaintInstance();
    }

    finishWin();
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