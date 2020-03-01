#include "head/console.h"
#include "head/board.h"
#include "head/chessManual.h"
#include "head/move.h"
#include "head/piece.h"

DWORD rwNum; // 公用变量
wchar_t wstr[WIDEWCHARSIZE];
const wchar_t ProgramName[] = L"中国象棋 ";

const SHORT WinRows = 50, WinCols = 150;
const SHORT BoardRows = BOARDROW * 2 - 1, BoardCols = (BOARDCOL * 2 - 1) * 2, BoardTitleRows = 2;
const SHORT ShadowCols = 2, ShadowRows = 1, BorderCols = 2, BorderRows = 1;
const SHORT StatusRows = 2;

/*
颜色属性由两个十六进制数字指定 -- 第一个对应于背景，第二个对应于前景。每个数字可以为以下任何值:
    0 = 黑色       8 = 灰色       1 = 蓝色       9 = 淡蓝色    2 = 绿色       A = 淡绿色
    3 = 浅绿色     B = 淡浅绿色   4 = 红色       C = 淡红色    5 = 紫色       D = 淡紫色
    6 = 黄色       E = 淡黄色     7 = 白色       F = 亮白色
//*/
const WORD WINATTR[] = { 0x07, 0x77 };
const WORD MENUATTR[] = { 0x80, 0x4F };
const WORD BOARDATTR[] = { 0xF8, 0xE2 };
const WORD CURMOVEATTR[] = { 0x70, 0xB4 };
const WORD MOVEATTR[] = { 0x70, 0x1F };
const WORD STATUSATTR[] = { 0xF0, 0x5F };
const WORD SHADOWATTR[] = { 0x08, 0x82 };

const WORD ShowMenuAttr[] = { 0x80, 0x4F };
const WORD SelMenuAttr[] = { 0x10, 0x2F };
const WORD SelMoveAttr[] = { 0x07, 0xF1 };

const WORD RedSideAttr[] = { 0x0C | (BOARDATTR[0] & 0xF0), 0x0C | (BOARDATTR[1] & 0xF0) };
const WORD BlackSideAttr[] = { 0x00 | (BOARDATTR[0] & 0xF0), 0x00 | (BOARDATTR[1] & 0xF0) };
const WORD CurmoveAttr[] = { 0x0C | (CURMOVEATTR[0] & 0xF0), 0x0C | (CURMOVEATTR[1] & 0xF0) };
const WORD RedAttr[] = { 0xCF, 0xCF };
const WORD BlackAttr[] = { 0x0F, 0x0F };
//const WORD SelRedAttr[] = { 0xFC, 0xC0 };
//const WORD SelBlackAttr[] = { 0xF0, 0xE0 };

const wchar_t* const TabChars[] = { L"─│┌┐└┘", L"═║╔╗╚╝" };

PConsole newConsole(const char* fileName)
{
    PConsole con = calloc(sizeof(Console), 1);
    con->hIn = GetStdHandle(STD_INPUT_HANDLE);
    con->hOut = CreateConsoleScreenBuffer(
        GENERIC_READ | GENERIC_WRITE, // read/write access
        FILE_SHARE_READ | FILE_SHARE_WRITE, // shared
        NULL, // default security attributes
        CONSOLE_TEXTMODE_BUFFER, // must be TEXTMODE
        NULL);
    con->cm = newChessManual();
    readChessManual(con->cm, fileName);
    writeMove_PGN_CCtoWstr(con->cm, &con->wstr_PGN_CC);

    con->thema = SHOWY;
    con->oldArea = con->curArea = MOVEA;

    SMALL_RECT WinRect = { 0, 0, (WinCols - 1), (WinRows - 1) };
    con->WinRect = WinRect;

    SMALL_RECT MenuRect = { WinRect.Left + ShadowCols, WinRect.Top,
        WinRect.Right - ShadowCols, ShadowRows };
    con->MenuRect = MenuRect;
    SMALL_RECT iMenuRect = { MenuRect.Left, MenuRect.Top,
        (MenuRect.Right - BorderCols - ShadowCols), (MenuRect.Bottom - 1) };
    con->iMenuRect = iMenuRect;

    SMALL_RECT StatusRect = { WinRect.Left + ShadowCols, (WinRect.Bottom - StatusRows - ShadowRows + 1),
        (WinRect.Right - ShadowCols), WinRect.Bottom };
    con->StatusRect = StatusRect;
    SMALL_RECT iStatusRect = { StatusRect.Left, StatusRect.Top,
        (StatusRect.Right - BorderCols - ShadowCols), (StatusRect.Bottom - 1) };
    con->iStatusRect = iStatusRect;

    SMALL_RECT BoardRect = { WinRect.Left + ShadowCols, (MenuRect.Bottom + 1 + ShadowRows),
        (WinRect.Left + 1 + BoardCols + BorderCols * 2 + ShadowCols * 2), (MenuRect.Bottom + BoardRows + (ShadowRows + BorderRows + BoardTitleRows) * 2) };
    con->BoardRect = BoardRect;
    SMALL_RECT iBoardRect = { (BoardRect.Left + 1 + BorderCols), (BoardRect.Top + BorderRows),
        (BoardRect.Right - BorderCols - ShadowCols), (BoardRect.Bottom - BorderRows - ShadowRows) };
    con->iBoardRect = iBoardRect;

    SMALL_RECT CurmoveRect = { BoardRect.Left, (BoardRect.Bottom + ShadowRows + 1),
        BoardRect.Right, (StatusRect.Top - 1 - ShadowRows) };
    con->CurmoveRect = CurmoveRect;
    SMALL_RECT iCurmoveRect = { (CurmoveRect.Left + BorderCols), (CurmoveRect.Top + BorderRows),
        (CurmoveRect.Right - BorderCols - ShadowCols), (CurmoveRect.Bottom - BorderRows - ShadowRows) };
    con->iCurmoveRect = iCurmoveRect;

    SMALL_RECT MoveRect = { (BoardRect.Right + 1 + ShadowCols), BoardRect.Top,
        (WinRect.Right - ShadowCols), CurmoveRect.Bottom };
    con->MoveRect = MoveRect;
    SMALL_RECT iMoveRect = { (MoveRect.Left + BorderCols), (MoveRect.Top + BorderRows),
        (MoveRect.Right - BorderCols - ShadowCols), iCurmoveRect.Bottom }; // 填充字符区域
    con->iMoveRect = iMoveRect;

    SetConsoleCP(936);
    SetConsoleOutputCP(936);
    SetConsoleMode(con->hIn, ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT); //ENABLE_PROCESSED_INPUT |
    SetConsoleTitleW(ProgramName); // 设置窗口标题

    COORD winCoord = { WinCols, WinRows };
    SetConsoleScreenBufferSize(con->hOut, winCoord);
    CONSOLE_CURSOR_INFO cInfo = { 5, false };
    SetConsoleCursorInfo(con->hOut, &cInfo);
    SetConsoleWindowInfo(con->hOut, true, &WinRect);
    SetConsoleActiveScreenBuffer(con->hOut);
    //SetConsoleTextAttribute(con->hOut, WINATTR[con->thema]);
    //GetConsoleScreenBufferInfo(con->hOut, &bInfo); // 获取窗口信息

    cleanAreaAttr(con, WINATTR[con->thema], &WinRect);
    //cleanAreaChar(con, &WinRect);
    initArea(con, MENUATTR[con->thema], &MenuRect, true, false);
    initArea(con, BOARDATTR[con->thema], &BoardRect, true, true);
    initArea(con, CURMOVEATTR[con->thema], &CurmoveRect, true, true);
    initArea(con, MOVEATTR[con->thema], &MoveRect, true, true);
    initArea(con, STATUSATTR[con->thema], &StatusRect, true, false);

    initMenu(con);
    writeAreas(con);
    operateWin(con);

    return con;
}

void delConsole(PConsole con)
{
    CloseHandle(con->hOut);
    delMenu(con->rootMenu);
    delChessManual(con->cm);
    free(con->wstr_PGN_CC);
    free(con);
}

void operateWin(PConsole con)
{
    INPUT_RECORD irInBuf[128];
    PKEY_EVENT_RECORD ker;
    while (true) {
        ReadConsoleInput(con->hIn, irInBuf, 128, &rwNum);
        for (DWORD i = 0; i < rwNum; i++) {
            switch (irInBuf[i].EventType) {
            case KEY_EVENT: // keyboard input
                ker = &irInBuf[i].Event.KeyEvent;
                if (!ker->bKeyDown)
                    break;
                if (((ker->dwControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED))
                        && ker->wVirtualKeyCode == VK_F4)
                    || ((ker->dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
                           && ker->wVirtualKeyCode == 'C'))
                    return;
                else if (ker->wVirtualKeyCode == VK_ESCAPE) {
                    if (con->curArea != MENUA)
                        return;
                    setArea(con, con->oldArea, con->oldArea);
                } else if (ker->wVirtualKeyCode == VK_TAB) {
                    Area oldArea = con->curArea;
                    Area curArea = (4 + oldArea + ((ker->dwControlKeyState & SHIFT_PRESSED) ? -1 : 1)) % 4; // 四个区域循环
                    setArea(con, curArea, oldArea);
                } else if ((ker->dwControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED))
                    && ker->wVirtualKeyCode == 0x12) {
                    if (con->curArea != MENUA)
                        setArea(con, MENUA, con->curArea);
                    else
                        setArea(con, con->oldArea, con->oldArea);
                }
                keyEventProc(con, ker);
                break;
            case MOUSE_EVENT: // mouse input
                mouseEventProc(con, &irInBuf[i].Event.MouseEvent);
                break;
            default:
                break;
            }
        }
    }
}

void keyEventProc(PConsole con, PKEY_EVENT_RECORD ker)
{
    switch (con->curArea) { // 区分不同区域, 进行操作
    case MOVEA:
        if (operateMove(con, ker))
            writeAreas(con);
        break;
    case CURMOVEA:
        operateCurMove(con, ker);
        break;
    case BOARDA:
        operateBoard(con, ker);
        break;
    case MENUA:
        if (!operateMenu(con, ker))
            setArea(con, con->oldArea, con->oldArea);
        break;
    default:
        break;
    }
}

void mouseEventProc(PConsole con, PMOUSE_EVENT_RECORD ker)
{
}

bool operateMenu(PConsole con, PKEY_EVENT_RECORD ker)
{
    bool stayMenu = true;
    switch (ker->wVirtualKeyCode) {
    case 'F':
        con->curMenu = getTopIndexMenu(con->rootMenu, 0)->childMenu;
        break;
    case 'B':
        con->curMenu = getTopIndexMenu(con->rootMenu, 1)->childMenu;
        break;
    case 'S':
        con->curMenu = getTopIndexMenu(con->rootMenu, 2)->childMenu;
        break;
    case 'A':
        con->curMenu = getTopIndexMenu(con->rootMenu, 3)->childMenu;
        break;
    case VK_DOWN:
        con->curMenu = (con->curMenu->childMenu ? con->curMenu->childMenu : getTopMenu(con->curMenu));
        break;
    case VK_UP:
        con->curMenu = (con->curMenu->childIndex > 0) ? con->curMenu->preMenu : getBottomMenu(con->curMenu);
        break;
    case VK_HOME:
    case VK_PRIOR:
        con->curMenu = getTopMenu(con->curMenu);
        break;
    case VK_END:
    case VK_NEXT:
        con->curMenu = getBottomMenu(con->curMenu);
        break;
    case VK_LEFT:
        con->curMenu = getSameLevelMenu(con->rootMenu, con->curMenu, true);
        break;
    case VK_RIGHT:
        con->curMenu = getSameLevelMenu(con->rootMenu, con->curMenu, false);
        break;
    case VK_RETURN:
        stayMenu = false;
        break;
    default:
        break;
    }

    SHORT level = con->curMenu->childIndex;
    SHORT posL = getPosL(con->curMenu) + ShadowCols,
          posT = con->iMenuRect.Bottom + (level == 0 ? 0 : 1),
          posR = posL + (level == 0 ? (wcslen(con->curMenu->name) + 3) : getMaxSize(con->curMenu)) + ShadowCols,
          posB = posT + (level == 0 ? 0 : getBottomMenu(con->curMenu)->childIndex - 1) + ShadowRows;
    SMALL_RECT rect = { posL, posT, posR, posB };

    cleanSubMenuArea(con, &rect, (stayMenu && level > 0));
    if (stayMenu) {
        if (level > 0) {
            SMALL_RECT irect = { posL, posT, posR - ShadowCols, posB - ShadowRows };
            // 显示子菜单屏幕区
            initArea(con, ShowMenuAttr[con->thema], &rect, true, false);
            writeAreaLineChars(con, getWstr(wstr, con->curMenu), &irect, 0, 0, false);
        }
        // 选择项目突显
        COORD pos = { posL, level };
        FillConsoleOutputAttribute(con->hOut, SelMenuAttr[con->thema], posR - posL - ShadowCols + 1, pos, &rwNum);
        writeStatus(con);
    }
    return stayMenu;
}

// 菜单最大尺寸
int getMaxSize(PMenu menu)
{
    menu = getTopMenu(menu)->childMenu;
    int maxSize = wcslen(menu->name);
    while ((menu = menu->childMenu))
        maxSize = max(maxSize, wcslen(menu->name));
    return maxSize * 2; // 中文字符占两个位置
}

// 同组菜单组合成字符串
wchar_t* getWstr(wchar_t* wstr, PMenu menu)
{
    wstr[0] = L'\x0';
    menu = getTopMenu(menu)->childMenu;
    do
        wcscat(wcscat(wstr, menu->name), L"\n");
    while ((menu = menu->childMenu));
    return wstr;
}

SHORT getPosL(PMenu menu)
{
    SHORT width = 0;
    menu = getTopMenu(menu);
    while ((menu = menu->preMenu)->brotherIndex > 0) // 顶层菜单往左推进
        width += wcslen(menu->name) + 3;
    return width;
}

// 菜单顶层第i个
PMenu getTopIndexMenu(PMenu rootMenu, int index)
{
    PMenu menu = rootMenu->brotherMenu;
    while (index-- > 0 && menu->brotherMenu)
        menu = menu->brotherMenu;
    return menu;
}

// 同组菜单顶层
PMenu getTopMenu(PMenu menu)
{
    while (menu->childIndex != 0)
        menu = menu->preMenu;
    return menu;
}

// 向左或右菜单第n个
PMenu getSameLevelMenu(PMenu rootMenu, PMenu menu, bool isLeft)
{
    int level = menu->childIndex;
    menu = getTopIndexMenu(rootMenu, (getTopMenu(menu)->brotherIndex - 1 + (isLeft ? -1 : 1) + 4) % 4);
    while (level-- > 0 && menu->childMenu)
        menu = menu->childMenu;
    return menu;
}

// 菜单底部
PMenu getBottomMenu(PMenu menu)
{
    while (menu->childMenu)
        menu = menu->childMenu;
    return menu;
}

bool operateBoard(PConsole con, PKEY_EVENT_RECORD keroldArea)
{
    return false;
}

bool operateMove(PConsole con, PKEY_EVENT_RECORD ker)
{
    PMove oldCurMove = con->cm->currentMove;
    switch (ker->wVirtualKeyCode) {
    case VK_DOWN:
        go(con->cm);
        break;
    case VK_UP:
        backNext(con->cm);
        break;
    case VK_LEFT:
        backOther(con->cm);
        break;
    case VK_RIGHT:
        goOther(con->cm);
        break;
    case VK_PRIOR:
        goInc(con->cm, -10);
        break;
    case VK_NEXT:
        goInc(con->cm, 10);
        break;
    case VK_HOME:
        backFirst(con->cm);
        break;
    case VK_END:
        goEnd(con->cm);
        break;
    case VK_RETURN:
        setArea(con, CURMOVEA, con->curArea);
        break;
    default:
        printf("key:%x/", ker->wVirtualKeyCode);
        break;
    }
    return oldCurMove != con->cm->currentMove; // 根据当前着法变化判断是否需要刷新屏幕
}

bool operateCurMove(PConsole con, PKEY_EVENT_RECORD ker)
{
    return false;
}

void writeAreas(PConsole con)
{
    writeBoard(con);
    writeCurmove(con);
    writeMove(con);
    writeStatus(con);
}

void writeBoard(PConsole con)
{
    SMALL_RECT iBoardRect = con->iBoardRect;
    // 清除背景颜色，重绘棋盘字符串
    SMALL_RECT iPieBoardRect = { iBoardRect.Left, iBoardRect.Top + BoardTitleRows, iBoardRect.Right, iBoardRect.Bottom - BoardTitleRows };
    cleanAreaAttr(con, BOARDATTR[con->thema], &iPieBoardRect);
    // 棋子文字上颜色
    wchar_t ch;
    getPieChars_board(wstr, con->cm->board); // 行:从上到下
    for (int i = SEATNUM - 1; i >= 0; --i) {
        if ((ch = wstr[i]) == BLANKCHAR)
            continue;
        // 字符属性函数不识别全角字符，均按半角字符计数
        COORD ciBoardRect = { (iBoardRect.Left + (i % BOARDCOL) * 4), (iBoardRect.Top + BoardTitleRows + (i / BOARDCOL) * 2) };
        FillConsoleOutputAttribute(con->hOut, (getColor_ch(ch) == RED ? RedAttr[con->thema] : BlackAttr[con->thema]), 2, ciBoardRect, &rwNum);
    }
    writeAreaLineChars(con, getBoardString(wstr, con->cm->board), &iPieBoardRect, 0, 0, true);

    static bool isStart = true;
    if (isStart) {
        SMALL_RECT iPreBoardRect = { iBoardRect.Left, iBoardRect.Top, iBoardRect.Right, iBoardRect.Top + BoardTitleRows - 1 },
                   iSufBoardRect = { iBoardRect.Left, iBoardRect.Bottom - BoardTitleRows + 1, iBoardRect.Right, iBoardRect.Bottom };
        writeAreaLineChars(con, getBoardPreString(wstr, con->cm->board), &iPreBoardRect, 0, 0, true);
        writeAreaLineChars(con, getBoardSufString(wstr, con->cm->board), &iSufBoardRect, 0, 0, true);

        bool bottomIsRed = isBottomSide(con->cm->board, RED);
        WORD bottomAttr = bottomIsRed ? RedSideAttr[con->thema] : BlackSideAttr[con->thema],
             topAttr = bottomIsRed ? BlackSideAttr[con->thema] : RedSideAttr[con->thema];
        // 顶、底两行上颜色
        for (int row = 0; row < BoardTitleRows; ++row) {
            COORD tpos = { iBoardRect.Left, (iBoardRect.Top + row) }, bpos = { iBoardRect.Left, (iBoardRect.Bottom - row) };
            FillConsoleOutputAttribute(con->hOut, topAttr, BoardCols, tpos, &rwNum);
            FillConsoleOutputAttribute(con->hOut, bottomAttr, BoardCols, bpos, &rwNum);
        }
        isStart = false;
    }
}

void writeCurmove(PConsole con)
{
    SMALL_RECT iCurmoveRect = con->iCurmoveRect;
    writeAreaLineChars(con, getMoveStr(wstr, con->cm->currentMove), &iCurmoveRect, 0, 0, false);

    // 设置当前着法、注解的突显颜色
    int cols = iCurmoveRect.Right - iCurmoveRect.Left + 1;
    int rows[] = { 2, 8, 9, 10, 11, 12 };
    for (int i = sizeof(rows) / sizeof(rows[0]) - 1; i >= 0; --i) {
        COORD pos = { iCurmoveRect.Left, (iCurmoveRect.Top + rows[i]) };
        FillConsoleOutputAttribute(con->hOut, CurmoveAttr[con->thema], cols, pos, &rwNum);
    }
}

void writeMove(PConsole con)
{
    static bool isStart = true;
    static int firstRow = 0, firstCol = 0; //静态变量，只初始化一次
    SMALL_RECT iMoveRect = con->iMoveRect;
    COORD showCoord = { (iMoveRect.Left + con->cm->currentMove->CC_ColNo_ * 5 * 2),
        (iMoveRect.Top + con->cm->currentMove->nextNo_ * 2) };
    SMALL_RECT showRect = { iMoveRect.Left + firstCol, iMoveRect.Top + firstRow,
        iMoveRect.Right + firstCol, iMoveRect.Bottom + firstRow };
    int offsetCol = ((showCoord.X < showRect.Left) ? showCoord.X - showRect.Left
                                                   : ((showCoord.X > showRect.Right) ? showCoord.X - showRect.Right
                                                                                     : 0)),
        offsetRow = ((showCoord.Y < showRect.Top) ? showCoord.Y - showRect.Top
                                                  : ((showCoord.Y > showRect.Bottom) ? showCoord.Y - showRect.Bottom
                                                                                     : 0));
    // 写入字符串
    if (offsetCol != 0 || offsetRow != 0 || isStart) {
        firstRow += offsetRow;
        firstCol += offsetCol;
        writeAreaLineChars(con, con->wstr_PGN_CC, &iMoveRect, firstRow, firstCol, true);
        isStart = false;
    }

    // 清除背景，设置选定Move背景
    cleanAreaAttr(con, MOVEATTR[con->thema], &iMoveRect);
    showCoord.X -= firstCol;
    showCoord.Y -= firstRow;
    FillConsoleOutputAttribute(con->hOut, SelMoveAttr[con->thema], 4 * 2, showCoord, &rwNum);
}

void writeStatus(PConsole con)
{
    switch (con->curArea) {
    case MOVEA:
        wcscpy(wstr, L"【导航区】  <↓>下着    <→>变着    <↑>前着    <←>前变   <PageDown>前进10着   <PageUp>回退10着   <Home>开始   <End>结束    <Enter>转入注解区");
        break;
    case CURMOVEA:
        wcscpy(wstr, L"【注解区】  <↓>下行    <→>右列    <↑>上行    <←>左列    注解文字编辑: 插入模式");
        break;
    case BOARDA:
        wcscpy(wstr, L"【盘面区】  <↓>下行    <→>右列    <↑>上行    <←>左列    <PageDown><End>底行    <PageUp><Home>顶行");
        break;
    case MENUA:
        wcscpy(wstr, L"【菜单区】  <↓>下  <→>右  <↑>上  <←>左  <PageDown><End>底  <PageUp><Home>顶  <");
        wcscat(wstr, con->curMenu->name);
        wcscat(wstr, L" > ");
        wcscat(wstr, con->curMenu->desc);
        break;
    default:
        wcscpy(wstr, L"???");
        break;
    }
    SMALL_RECT arect = { con->iStatusRect.Left, con->iStatusRect.Top, con->iStatusRect.Right, con->iStatusRect.Top };
    writeAreaLineChars(con, wstr, &arect, 0, 0, false);

    static bool isStart = true;
    if (isStart) {
        wcscpy(wstr, L" <Tab><Shift+Tab>区间转换(导航>>注解>>盘面>>菜单)          <Alt>进入/退出(菜单)          <Esc>退出(菜单/程序)         <Ctrl+C><Alt+F4>退出程序");
        SMALL_RECT brect = { con->iStatusRect.Left, con->iStatusRect.Bottom, con->iStatusRect.Right, con->iStatusRect.Bottom };
        writeAreaLineChars(con, wstr, &brect, 0, 0, false);
        isStart = false;
    }
}

void writeAreaLineChars(PConsole con, wchar_t* lineChars, const PSMALL_RECT rc, int firstRow, int firstCol, bool cutLine)
{
    int cols = rc->Right - rc->Left + 1;
    wchar_t lineChar[WIDEWCHARSIZE];

    cleanAreaChar(con, rc);
    while (firstRow-- > 0) // 去掉开始数行
        getLine(lineChar, &lineChars, cols, cutLine);
    int lineSize;
    for (SHORT row = rc->Top; row <= rc->Bottom; ++row)
        if ((lineSize = getLine(lineChar, &lineChars, cols, cutLine) - firstCol) > 0) {
            COORD pos = { rc->Left, row };
            WriteConsoleOutputCharacterW(con->hOut, lineChar, lineSize, pos, &rwNum);
        }
}

int getLine(wchar_t* lineChar, wchar_t** plineChars, int cols, bool cutLine)
{
    int srcIndex = 0, desIndex = 0, showCols = 0;
    wchar_t wch = (*plineChars)[0];
    while (wch != L'\x0' && wch != L'\n') {
        lineChar[desIndex++] = wch;
        ++showCols;
        if (wch >= 0x2500) {
            ++showCols; // 显示位置加一
            if (wch <= 0x2573) // 制表字符后加一空格 wch >= 0x2500 &&
                lineChar[desIndex++] = L' ';
        }
        wch = (*plineChars)[++srcIndex];
        // 行尾前一个且下一个为全角 已至行尾最后一个
        if ((showCols == cols - 1 && wch > 0x2573) || showCols == cols) {
            if (cutLine)
                while (wch != L'\x0' || wch != L'\n')
                    wch = (*plineChars)[++srcIndex];
            break;
        }
    }
    if (wch == L'\n')
        ++srcIndex; // 消除换行符
    (*plineChars) += srcIndex;
    lineChar[desIndex] = L'\x0';
    return desIndex;
}

void initMenu(PConsole con)
{
    MenuData menuDatas[4][10] = {
        { { L" 文件(F)", L"新建、打开、保存文件，退出", NULL },
            { L" 新建", L"新建一个棋局", NULL },
            { L" 打开...", L"打开已有的一个棋局", NULL },
            { L" 另存为...", L"将显示的棋局另存为一个棋局", NULL },
            { L" 保存", L"保存正在显示的棋局", NULL },
            { L" 退出", L"退出程序", NULL },
            { L"", L"", NULL } },
        { { L" 棋局(B)", L"对棋盘局面进行操作", NULL },
            { L" 对换棋局", L"红黑棋子互换", NULL },
            { L" 对换位置", L"红黑位置互换", NULL },
            { L" 棋子左右换位", L"棋子的位置左右对称换位", NULL },
            { L"", L"", NULL } },
        { { L" 设置(S)", L"设置显示主题，主要是棋盘、棋子的颜色配置", NULL },
            { L" 静雅朴素", L"比较朴素的颜色配置", NULL },
            { L" 鲜艳亮丽", L"比较鲜艳的颜色配置", NULL },
            { L" 高对比度", L"高对比度的颜色配置", NULL },
            { L"", L"", NULL } },
        { { L" 关于(A)", L"帮助、程序信息", NULL },
            { L" 帮助", L"显示帮助信息", NULL },
            { L" 版本信息", L"程序有关的信息", NULL },
            { L"", L"", NULL } }
    };

    MenuData rmData = { L"", L"", NULL };
    con->rootMenu = newMenu(&rmData);
    PMenu brotherMenu = con->rootMenu, childMenu;
    int levelOneNum = sizeof(menuDatas) / sizeof(menuDatas[0]);
    for (int oneIndex = 0; oneIndex < levelOneNum; ++oneIndex) {
        brotherMenu = addBrotherMenu(brotherMenu, newMenu(&menuDatas[oneIndex][0])); // 第一个为菜单组名称
        childMenu = brotherMenu;
        int levelTwoNum = sizeof(menuDatas[0]) / sizeof(menuDatas[0][0]);
        for (int twoIndex = 1; twoIndex < levelTwoNum && wcslen(menuDatas[oneIndex][twoIndex].name) > 0; ++twoIndex)
            childMenu = addChildMenu(childMenu, newMenu(&menuDatas[oneIndex][twoIndex]));
    }

    //*// 绘制菜单区域
    brotherMenu = con->rootMenu;
    int pos = ShadowCols;
    while ((brotherMenu = brotherMenu->brotherMenu)) {
        int namelen = wcslen(brotherMenu->name);
        COORD posCoord = { pos, con->MenuRect.Top };
        WriteConsoleOutputCharacterW(con->hOut, brotherMenu->name, namelen, posCoord, &rwNum);
        pos += namelen + 3;
    }
    //*/
    con->curMenu = con->rootMenu->brotherMenu;
}

PMenu newMenu(PMenuData menuData)
{
    PMenu menu = (PMenu)calloc(sizeof(Menu), 1);
    wcscpy(menu->name, menuData->name);
    wcscpy(menu->desc, menuData->desc);
    menu->func = menuData->func;
    return menu;
}

PMenu addChildMenu(PMenu preMenu, PMenu childMenu)
{
    childMenu->preMenu = preMenu;
    childMenu->brotherIndex = preMenu->brotherIndex;
    childMenu->childIndex = preMenu->childIndex + 1;
    return preMenu->childMenu = childMenu;
}

PMenu addBrotherMenu(PMenu preMenu, PMenu brotherMenu)
{
    brotherMenu->preMenu = preMenu;
    brotherMenu->brotherIndex = preMenu->brotherIndex + 1;
    brotherMenu->childIndex = preMenu->childIndex;
    return preMenu->brotherMenu = brotherMenu;
}

void delMenu(PMenu menu)
{
    if (menu == NULL)
        return;
    delMenu(menu->brotherMenu);
    delMenu(menu->childMenu);
    free(menu);
}

void setArea(PConsole con, Area curArea, Area oldArea)
{
    bool changed = con->curArea != curArea;
    if (con->curArea == MENUA && changed) {
        SMALL_RECT rect = { 0, 0, 0, 0 };
        cleanSubMenuArea(con, &rect, false);
    }
    con->curArea = curArea;
    con->oldArea = oldArea;
    if (changed)
        writeStatus(con);
}

void initArea(PConsole con, WORD attr, const PSMALL_RECT rc, bool drawShadow, bool drawFrame)
{
    if (drawShadow)
        initAreaShadow(con, rc);
    SMALL_RECT irc = { rc->Left, rc->Top, rc->Right - ShadowCols, rc->Bottom - ShadowRows };
    cleanAreaAttr(con, attr, &irc);
    cleanAreaChar(con, &irc);
    if (drawFrame)
        initAreaFrame(con, &irc);
}

void initAreaShadow(PConsole con, const PSMALL_RECT rc)
{
    //FillConsoleOutputAttribute(con->hOut, WINATTR[con->thema], ShadowCols, { rc->Left, rc->Bottom }, &rwNum);
    COORD pos = { (rc->Left + ShadowCols), rc->Bottom };
    FillConsoleOutputAttribute(con->hOut, SHADOWATTR[con->thema], rc->Right - rc->Left + 1 - ShadowCols, pos, &rwNum);
    SHORT right = rc->Right - ShadowCols + 1;
    //if (rc->Bottom > rc->Top + 1)
    //    FillConsoleOutputAttribute(con->hOut, WINATTR[con->thema], ShadowCols, { right, rc->Top }, &rwNum);
    for (SHORT row = rc->Top + (rc->Bottom > rc->Top + 1 ? 1 : 0); row <= rc->Bottom; ++row) {
        COORD pos = { right, row };
        FillConsoleOutputAttribute(con->hOut, SHADOWATTR[con->thema], 2, pos, &rwNum);
    }
}

void initAreaFrame(PConsole con, const PSMALL_RECT rc)
{
    SHORT left = rc->Left + 1, right = rc->Right - 1;
    const wchar_t* const tabChar = TabChars[con->thema];
    int rows[] = { rc->Top, rc->Bottom };
    for (int i = sizeof(rows) / sizeof(rows[0]) - 1; i >= 0; --i) {
        COORD pos = { left, rows[i] };
        FillConsoleOutputCharacterW(con->hOut, tabChar[0], rc->Right - rc->Left - 2, pos, &rwNum);
    }
    for (SHORT row = rc->Top + 1; row < rc->Bottom; ++row) {
        int cols[] = { rc->Left, right };
        for (int i = sizeof(cols) / sizeof(cols[0]) - 1; i >= 0; --i) {
            COORD pos = { cols[i], row };
            FillConsoleOutputCharacterW(con->hOut, tabChar[1], 1, pos, &rwNum);
        }
    }
    COORD posLT = { rc->Left, rc->Top };
    COORD posRT = { right, rc->Top };
    COORD posLB = { rc->Left, rc->Bottom };
    COORD posRB = { right, rc->Bottom };
    FillConsoleOutputCharacterW(con->hOut, tabChar[2], 1, posLT, &rwNum);
    FillConsoleOutputCharacterW(con->hOut, tabChar[3], 1, posRT, &rwNum);
    FillConsoleOutputCharacterW(con->hOut, tabChar[4], 1, posLB, &rwNum);
    FillConsoleOutputCharacterW(con->hOut, tabChar[5], 1, posRB, &rwNum);
}

void cleanAreaWIN(PConsole con)
{
    SMALL_RECT rc = { con->WinRect.Left, con->MenuRect.Bottom + 1, con->WinRect.Right, con->StatusRect.Top - 1 };
    int rows[] = { rc.Top }; //, rc.Bottom };
    for (int i = sizeof(rows) / sizeof(rows[0]) - 1; i >= 0; --i) { // 顶、底行
        int width = rc.Right - rc.Left + 1;
        COORD pos = { rc.Left, rows[i] };
        FillConsoleOutputAttribute(con->hOut, WINATTR[con->thema], width, pos, &rwNum);
        FillConsoleOutputCharacterW(con->hOut, L' ', width, pos, &rwNum);
    }
    /*
    for (SHORT row = rc.Top; row < rc.Bottom; ++row) { // 左、右列
        int cols[] = { rc.Left, (rc.Right - ShadowCols + 1) };
        for (int i = sizeof(cols) / sizeof(cols[0]) - 1; i >= 0; --i) {
            COORD pos = { cols[i], row };
            FillConsoleOutputAttribute(con->hOut, WINATTR[con->thema], ShadowCols, pos, &rwNum);
            FillConsoleOutputCharacterW(con->hOut, L' ', ShadowCols, pos, &rwNum);
        }
    }
    //*/
}

void cleanSubMenuArea(PConsole con, const PSMALL_RECT rc, bool storgeMenu)
{
    cleanAreaAttr(con, MENUATTR[con->thema], &con->iMenuRect); // 清除菜单顶行背景
    COORD pos = { con->MenuRect.Left, con->MenuRect.Bottom };
    FillConsoleOutputCharacterW(con->hOut, L' ', WinCols, pos, &rwNum); // 清除菜单阴影行文字
    FillConsoleOutputAttribute(con->hOut, WINATTR[con->thema], ShadowCols, pos, &rwNum); // 清除阴影最左侧背景块
    initAreaShadow(con, &con->MenuRect); // 清除菜单阴影区背景
    cleanAreaWIN(con); // 清楚窗口背景区

    // 静态变量，第一次初始化
    static CHAR_INFO boardCharBuf[10 * 40];
    static bool hasStorgeMenu = false;
    static COORD chBufSize = { 0, 0 }, chBufCoord = { 0, 0 };
    static SMALL_RECT chBufRect = { 0, 0, 0, 0 };
    // 恢复已存储的Board区屏幕块信息
    if (hasStorgeMenu)
        WriteConsoleOutputW(con->hOut, boardCharBuf, chBufSize, chBufCoord, &chBufRect);

    // 存储将被覆盖的Board区屏幕块信息
    chBufRect.Left = rc->Left;
    chBufRect.Top = con->BoardRect.Top;
    chBufRect.Right = rc->Right;
    chBufRect.Bottom = rc->Bottom;
    chBufSize.X = chBufRect.Right - chBufRect.Left + 1;
    chBufSize.Y = chBufRect.Bottom - chBufRect.Top + 1;
    if ((hasStorgeMenu = storgeMenu))
        ReadConsoleOutputW(con->hOut, boardCharBuf, chBufSize, chBufCoord, &chBufRect);
}

void cleanAreaChar(PConsole con, PSMALL_RECT rc)
{
    int cols = rc->Right - rc->Left + 1;
    for (SHORT row = rc->Top; row <= rc->Bottom; ++row) {
        COORD pos = { rc->Left, row };
        FillConsoleOutputCharacterW(con->hOut, L' ', cols, pos, &rwNum);
    }
}

void cleanAreaAttr(PConsole con, WORD attr, const PSMALL_RECT rc)
{
    int cols = rc->Right - rc->Left + 1;
    for (SHORT row = rc->Top; row <= rc->Bottom; ++row) {
        COORD pos = { rc->Left, row };
        FillConsoleOutputAttribute(con->hOut, attr, cols, pos, &rwNum);
    }
}

/*
void DrawBox(bool bSingle, SMALL_RECT rc); // 函数功能：画边框

void ShadowWindowLine(char* str)
{
    SMALL_RECT rc;
    CONSOLE_SCREEN_BUFFER_INFO bInfo; // 窗口缓冲区信息
    WORD att0, att1; //,attText;
    int i, chNum = strlen(str);
    GetConsoleScreenBufferInfo(con->hOut, &bInfo); // 获取窗口缓冲区信息
    // 计算显示窗口大小和位置
    rc->Left = (bInfo.dwSize.X - chNum) / 2 - 2;
    rc->Top = 10; // 原代码段中此处为bInfo.dwSize.Y/2 - 2，但是如果您的DOS屏幕有垂直滚动条的话，还需要把滚动条下拉才能看到,为了方便就把它改为10
    rc->Right = rc->Left + chNum + 3;
    rc->Bottom = rc->Top + 4;
    att0 = BACKGROUND_INTENSITY; // 阴影属性
    att1 = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_BLUE; // 文本属性
    //attText = FOREGROUND_RED |FOREGROUND_INTENSITY; // 文本属性
    // 设置阴影然后填充
    COORD posShadow = { SHORT(rc->Left + 1), SHORT(rc->Top + 1) }, posText = { rc->Left, rc->Top };
    DWORD rwNum;
    for (i = 0; i < 5; i++) {
        FillConsoleOutputAttribute(con->hOut, att0, chNum + 4, posShadow, &rwNum);
        posShadow.Y++;
    }
    for (i = 0; i < 5; i++) {
        FillConsoleOutputAttribute(con->hOut, att1, chNum + 4, posText, &rwNum);
        posText.Y++;
    }
    // 写文本和边框
    posText.X = rc->Left + 2;
    posText.Y = rc->Top + 2;
    WriteConsoleOutputCharacterA(con->hOut, str, strlen(str), posText, &rwNum);
    //DrawBox(true, rc);
    DrawBox(false, rc);
    SetConsoleTextAttribute(con->hOut, bInfo.wAttributes); // 恢复原来的属性
}

void DrawBox(bool bSingle, SMALL_RECT rc) // 函数功能：画边框
{
    char chBox[6];
    COORD pos;
    if (bSingle) {
        chBox[0] = (char)0xda; // 左上角点
        chBox[1] = (char)0xbf; // 右上角点
        chBox[2] = (char)0xc0; // 左下角点
        chBox[3] = (char)0xd9; // 右下角点
        chBox[4] = (char)0xc4; // 水平
        chBox[5] = (char)0xb3; // 坚直
    } else {
        chBox[0] = (char)0xc9; // 左上角点
        chBox[1] = (char)0xbb; // 右上角点
        chBox[2] = (char)0xc8; // 左下角点
        chBox[3] = (char)0xbc; // 右下角点
        chBox[4] = (char)0xcd; // 水平
        chBox[5] = (char)0xba; // 坚直
    }
    DWORD rwNum;
    // 画边框的上 下边界
    for (pos.X = rc->Left + 1; pos.X < rc->Right - 1; pos.X++) {
        pos.Y = rc->Top;
        // 画上边界
        WriteConsoleOutputCharacterA(con->hOut, &chBox[4], 1, pos, &rwNum);
        // 画左上角
        if (pos.X == rc->Left + 1) {
            pos.X--;
            WriteConsoleOutputCharacterA(con->hOut, &chBox[0], 1, pos, &rwNum);
            pos.X++;
        }
        // 画右上角
        if (pos.X == rc->Right - 2) {
            pos.X++;
            WriteConsoleOutputCharacterA(con->hOut, &chBox[1], 1, pos, &rwNum);
            pos.X--;
        }
        pos.Y = rc->Bottom;
        // 画下边界
        WriteConsoleOutputCharacterA(con->hOut, &chBox[4], 1, pos, &rwNum);
        // 画左下角
        if (pos.X == rc->Left + 1) {
            pos.X--;
            WriteConsoleOutputCharacterA(con->hOut, &chBox[2], 1, pos, &rwNum);
            pos.X++;
        }
        // 画右下角
        if (pos.X == rc->Right - 2) {
            pos.X++;
            WriteConsoleOutputCharacterA(con->hOut, &chBox[3], 1, pos, &rwNum);
            pos.X--;
        }
    }
    // 画边框的左右边界
    for (pos.Y = rc->Top + 1; pos.Y <= rc->Bottom - 1; pos.Y++) {
        pos.X = rc->Left;
        // 画左边界
        WriteConsoleOutputCharacterA(con->hOut, &chBox[5], 1, pos, &rwNum);
        pos.X = rc->Right - 1;
        // 画右边界
        WriteConsoleOutputCharacterA(con->hOut, &chBox[5], 1, pos, &rwNum);
    }
}

void DeleteLine(int row); // 删除一行
void MoveText(int x, int y, SMALL_RECT rc); // 移动文本块区域
void ClearScreen(void); // 清屏

void DeleteLine(int row)
{
    SMALL_RECT rcScroll, rcClip;
    COORD crDest = { 0, SHORT(row - 1) };
    CHAR_INFO chFill;
    CONSOLE_SCREEN_BUFFER_INFO bInfo;
    GetConsoleScreenBufferInfo(con->hOut, &bInfo);
    rcScroll.Left = 0;
    rcScroll.Top = row;
    rcScroll.Right = bInfo.dwSize.X - 1;
    rcScroll.Bottom = bInfo.dwSize.Y - 1;
    rcClip = rcScroll;
    chFill.Attributes = bInfo.wAttributes;
    chFill.Char.AsciiChar = ' ';
    ScrollConsoleScreenBuffer(con->hOut, &rcScroll, &rcClip, crDest, &chFill);
}

void MoveText(int x, int y, SMALL_RECT rc)
{
    COORD crDest = { SHORT(x), SHORT(y) };
    CHAR_INFO chFill;
    CONSOLE_SCREEN_BUFFER_INFO bInfo;
    GetConsoleScreenBufferInfo(con->hOut, &bInfo);
    chFill.Attributes = bInfo.wAttributes;
    chFill.Char.AsciiChar = ' ';
    SMALL_RECT rcClip;
    ScrollConsoleScreenBuffer(con->hOut, &rc, &rcClip, crDest, &chFill);
}

void ClearScreen(void)
{
    CONSOLE_SCREEN_BUFFER_INFO bInfo;
    GetConsoleScreenBufferInfo(con->hOut, &bInfo);
    COORD home = { 0, 0 };
    WORD att = bInfo.wAttributes;
    unsigned long size = bInfo.dwSize.X * bInfo.dwSize.Y;
    FillConsoleOutputAttribute(con->hOut, att, size, home, NULL);
    FillConsoleOutputCharacter(con->hOut, ' ', size, home, NULL);
}

//HANDLE hIn;
void CharWindow(char ch, SMALL_RECT rc); // 将ch输入到指定的窗口中
void ControlStatus(DWORD state); // 在最后一行显示控制键的状态
void DeleteTopLine(SMALL_RECT rc); // 删除指定窗口中最上面的行并滚动

void CharWindow(char ch, SMALL_RECT rc) // 将ch输入到指定的窗口中
{
    COORD chPos = { SHORT(rc->Left + 1), SHORT(rc->Top + 1) };
    SetConsoleCursorPosition(con->hOut, chPos); // 设置光标位置
    if ((ch < 0x20) || (ch > 0x7e)) // 如果是不可打印的字符，具体查看ASCII码表
        return;
    WriteConsoleOutputCharacter(con->hOut, &ch, 1, chPos, NULL);
    if (chPos.X >= (rc->Right - 2)) {
        chPos.X = rc->Left;
        chPos.Y++;
    }
    if (chPos.Y > (rc->Bottom - 1)) {
        DeleteTopLine(rc);
        chPos.Y = rc->Bottom - 1;
    }
    chPos.X++;
    SetConsoleCursorPosition(con->hOut, chPos); // 设置光标位置
}

void ControlStatus(DWORD state) // 在第一行显示控制键的状态
{
    CONSOLE_SCREEN_BUFFER_INFO bInfo;
    GetConsoleScreenBufferInfo(con->hOut, &bInfo);
    COORD home = { 0, 24 }; // 原来此处为bInfo.dwSize.Y-1，但为了更便于观察，我把这里稍微修改了一下
    WORD att0 = BACKGROUND_INTENSITY;
    WORD att1 = FOREGROUND_GREEN | FOREGROUND_INTENSITY | BACKGROUND_RED;
    FillConsoleOutputAttribute(con->hOut, att0, bInfo.dwSize.X, home, NULL);
    FillConsoleOutputCharacter(con->hOut, ' ', bInfo.dwSize.X, home, NULL);
    SetConsoleTextAttribute(con->hOut, att1);
    COORD staPos = { SHORT(bInfo.dwSize.X - 16), 24 }; // 原来此处为bInfo.dwSize.Y-1
    SetConsoleCursorPosition(con->hOut, staPos);
    if (state & NUMLOCK_ON)
        WriteConsole(con->hOut, "NUM", 3, NULL, NULL);
    staPos.X += 4;
    SetConsoleCursorPosition(con->hOut, staPos);
    if (state & CAPSLOCK_ON)
        WriteConsole(con->hOut, "CAPS", 4, NULL, NULL);
    staPos.X += 5;
    SetConsoleCursorPosition(con->hOut, staPos);
    if (state & SCROLLLOCK_ON)
        WriteConsole(con->hOut, "SCROLL", 6, NULL, NULL);
    SetConsoleTextAttribute(con->hOut, bInfo.wAttributes); // 恢复原来的属性
    SetConsoleCursorPosition(con->hOut, bInfo.dwCursorPosition); // 恢复原来的光标位置
}

void DeleteTopLine(SMALL_RECT rc)
{
    COORD crDest;
    CHAR_INFO chFill;
    SMALL_RECT rcClip = rc;
    rcClip.Left++;
    rcClip.Right -= 2;
    rcClip.Top++;
    rcClip.Bottom--;
    crDest.X = rcClip.Left;
    crDest.Y = rcClip.Top - 1;
    CONSOLE_SCREEN_BUFFER_INFO bInfo;
    GetConsoleScreenBufferInfo(con->hOut, &bInfo);
    chFill.Attributes = bInfo.wAttributes;
    chFill.Char.AsciiChar = ' ';
    ScrollConsoleScreenBuffer(con->hOut, &rcClip, &rcClip, crDest, &chFill);
}

void DispMousePos(COORD pos) // 在第24行显示鼠标位置
{
    CONSOLE_SCREEN_BUFFER_INFO bInfo;
    GetConsoleScreenBufferInfo(con->hOut, &bInfo);
    COORD home = { 0, 24 };
    WORD att0 = BACKGROUND_INTENSITY;
    FillConsoleOutputAttribute(con->hOut, att0, bInfo.dwSize.X, home, NULL);
    FillConsoleOutputCharacter(con->hOut, ' ', bInfo.dwSize.X, home, NULL);
    char s[20];
    sprintf(s, "X = %2d, Y = %2d", pos.X, pos.Y);
    SetConsoleTextAttribute(con->hOut, att0);
    SetConsoleCursorPosition(con->hOut, home);
    WriteConsole(con->hOut, s, strlen(s), NULL, NULL);
    SetConsoleTextAttribute(con->hOut, bInfo.wAttributes); // 恢复原来的属性
    SetConsoleCursorPosition(con->hOut, bInfo.dwCursorPosition); // 恢复原来的光标位置
}

HANDLE hStdout;
int readAndWirte(void)
{
    HANDLE hStdout, hNewScreenBuffer;
    SMALL_RECT srctReadRect;
    SMALL_RECT srctWriteRect;
    CHAR_INFO chiBuffer[160]; // [2][80];
    COORD coordBufSize;
    COORD coordBufCoord;
    BOOL fSuccess;

    // Get a handle to the STDOUT screen buffer to copy from and
    // create a new screen buffer to copy to.

    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    hNewScreenBuffer = CreateConsoleScreenBuffer(
        GENERIC_READ | // read/write access
            GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, // shared
        NULL, // default security attributes
        CONSOLE_TEXTMODE_BUFFER, // must be TEXTMODE
        NULL); // reserved; must be NULL
    if (hStdout == INVALID_HANDLE_VALUE || hNewScreenBuffer == INVALID_HANDLE_VALUE) {
        printf("CreateConsoleScreenBuffer failed - (%d)\n", GetLastError());
        return 1;
    }

    // Make the new screen buffer the active screen buffer.

    if (!SetConsoleActiveScreenBuffer(hNewScreenBuffer)) {
        printf("SetConsoleActiveScreenBuffer failed - (%d)\n", GetLastError());
        return 1;
    }

    // Set the source rectangle.

    srctReadRect.Top = 0; // top left: row 0, col 0
    srctReadRect.Left = 0;
    srctReadRect.Bottom = 1; // bot. right: row 1, col 79
    srctReadRect.Right = 79;

    // The temporary buffer size is 2 rows x 80 columns.

    coordBufSize.Y = 2;
    coordBufSize.X = 80;

    // The top left destination cell of the temporary buffer is
    // row 0, col 0.

    coordBufCoord.X = 0;
    coordBufCoord.Y = 0;

    // Copy the block from the screen buffer to the temp. buffer.

    fSuccess = ReadConsoleOutput(
        hStdout, // screen buffer to read from
        chiBuffer, // buffer to copy into
        coordBufSize, // col-row size of chiBuffer
        coordBufCoord, // top left dest. cell in chiBuffer
        &srctReadRect); // screen buffer source rectangle
    if (!fSuccess) {
        printf("ReadConsoleOutput failed - (%d)\n", GetLastError());
        return 1;
    }

    // Set the destination rectangle.

    srctWriteRect.Top = 10; // top lt: row 10, col 0
    srctWriteRect.Left = 0;
    srctWriteRect.Bottom = 11; // bot. rt: row 11, col 79
    srctWriteRect.Right = 79;

    // Copy from the temporary buffer to the new screen buffer.

    fSuccess = WriteConsoleOutput(
        hNewScreenBuffer, // screen buffer to write to
        chiBuffer, // buffer to copy from
        coordBufSize, // col-row size of chiBuffer
        coordBufCoord, // top left src cell in chiBuffer
        &srctWriteRect); // dest. screen buffer rectangle
    if (!fSuccess) {
        printf("WriteConsoleOutput failed - (%d)\n", GetLastError());
        return 1;
    }
    Sleep(5000);

    // Restore the original active screen buffer.

    if (!SetConsoleActiveScreenBuffer(hStdout)) {
        printf("SetConsoleActiveScreenBuffer failed - (%d)\n", GetLastError());
        return 1;
    }

    return 0;
}

void cls(HANDLE hConsole)
{
    COORD coordScreen = { 0, 0 }; // home for the cursor
    DWORD cCharsWritten;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD dwConSize;

    // Get the number of character cells in the current buffer.

    if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) {
        return;
    }

    dwConSize = csbi.dwSize.X * csbi.dwSize.Y;

    // Fill the entire screen with blanks.

    if (!FillConsoleOutputCharacter(hConsole, // Handle to console screen buffer
            (TCHAR)' ', // Character to write to the buffer
            dwConSize, // Number of cells to write
            coordScreen, // Coordinates of first cell
            &cCharsWritten)) // Receive number of characters rwNum
    {
        return;
    }

    // Get the current text attribute.

    if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) {
        return;
    }

    // Set the buffer's attributes accordingly.

    if (!FillConsoleOutputAttribute(hConsole, // Handle to console screen buffer
            csbi.wAttributes, // Character attributes to use
            dwConSize, // Number of cells to set attribute
            coordScreen, // Coordinates of first cell
            &cCharsWritten)) // Receive number of characters rwNum
    {
        return;
    }

    // Put the cursor at its home coordinates.

    SetConsoleCursorPosition(hConsole, coordScreen);
}

int ScrollByAbsoluteCoord(int iRows)
{
    CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
    SMALL_RECT srctWindow;

    // Get the current screen buffer size and window position.

    if (!GetConsoleScreenBufferInfo(hStdout, &csbiInfo)) {
        printf("GetConsoleScreenBufferInfo (%d)\n", GetLastError());
        return 0;
    }

    // Set srctWindow to the current window size and location.

    srctWindow = csbiInfo.srWindow;

    // Check whether the window is too close to the screen buffer top

    if (srctWindow.Top >= iRows) {
        srctWindow.Top -= (SHORT)iRows; // move top up
        srctWindow.Bottom -= (SHORT)iRows; // move bottom up

        if (!SetConsoleWindowInfo(
                hStdout, // screen buffer handle
                TRUE, // absolute coordinates
                &srctWindow)) // specifies new location
        {
            printf("SetConsoleWindowInfo (%d)\n", GetLastError());
            return 0;
        }
        return iRows;
    } else {
        printf("\nCannot scroll; the window is too close to the top.\n");
        return 0;
    }
}

int ScrollByRelativeCoord(int iRows)
{
    CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
    SMALL_RECT srctWindow;

    // Get the current screen buffer window position.

    if (!GetConsoleScreenBufferInfo(hStdout, &csbiInfo)) {
        printf("GetConsoleScreenBufferInfo (%d)\n", GetLastError());
        return 0;
    }

    // Check whether the window is too close to the screen buffer top

    if (csbiInfo.srWindow.Top >= iRows) {
        srctWindow.Top = -(SHORT)iRows; // move top up
        srctWindow.Bottom = -(SHORT)iRows; // move bottom up
        srctWindow.Left = 0; // no change
        srctWindow.Right = 0; // no change

        if (!SetConsoleWindowInfo(
                hStdout, // screen buffer handle
                FALSE, // relative coordinates
                &srctWindow)) // specifies new location
        {
            printf("SetConsoleWindowInfo (%d)\n", GetLastError());
            return 0;
        }
        return iRows;
    } else {
        printf("\nCannot scroll; the window is too close to the top.\n");
        return 0;
    }
}
//*/

/*
void view0()
{
    CONSOLE_SCREEN_BUFFER_INFO bInfo; // 存储窗口信息
    COORD pos = {0, 0};
    // 获取标准输出设备句柄
    hOut = GetStdHandle(STD_OUTPUT_HANDLE); 
    // 获取窗口信息
    GetConsoleScreenBufferInfo(con->hOut, &bInfo ); 
    printf("\n\nThe soul selects her own society\n");
    printf("Then shuts the door\n");
    printf("On her devine majority\n");
    printf("Obtrude no more\n\n");
    _getch();
    // 向窗口中填充字符以获得清屏的效果
    FillConsoleOutputCharacter(con->hOut,'-', bInfo.dwSize.X * bInfo.dwSize.Y, pos, NULL);
    // 关闭标准输出设备句柄
    CloseHandle(hOut); 
    //*/

/*
    setlocale(LC_ALL, "C");
    char strTitle[255];
    CONSOLE_SCREEN_BUFFER_INFO bInfo; // 窗口缓冲区信息
    COORD size = { 80, 25 };
    hOut = GetStdHandle(STD_OUTPUT_HANDLE); // 获取标准输出设备句柄
    GetConsoleScreenBufferInfo(con->hOut, &bInfo); // 获取窗口缓冲区信息
    GetConsoleTitle(strTitle, 255); // 获取窗口标题
    printf("当前窗口标题是：%s\n", strTitle);
    _getch();
    SetConsoleTitle("控制台窗口操作"); // 设置窗口标题
    GetConsoleTitle(strTitle, 255);
    printf("当前窗口标题是：%s\n", strTitle);
    _getch();
    SetConsoleScreenBufferSize(con->hOut, size); // 重新设置缓冲区大小
    _getch();
    SMALL_RECT rc = { 0, 0, 80 - 1, 25 - 1 }; // 重置窗口位置和大小 (窗口边框会消失？)
    SetConsoleWindowInfo(con->hOut, true, &rc);
    setlocale(LC_ALL, "chs");

    CloseHandle(hOut); // 关闭标准输出设备句柄
    //*/

/*
    hOut = GetStdHandle(STD_OUTPUT_HANDLE); // 获取标准输出设备句柄
    SetConsoleOutputCP(437); // 设置代码页，这里如果设置成936（简体中文），那么程序会怎样？那样的话，将画不出边框。
    //SetConsoleOutputCP(936); // 设置代码页，这里如果设置成936（简体中文），那么程序会怎样？那样的话，将画不出边框。
    char str[] = "Display a line of words, and center the window with shadow.";
    ShadowWindowLine(str);
    CloseHandle(hOut); // 关闭标准输出设备句柄
    //*/

/*
    hOut = GetStdHandle(STD_OUTPUT_HANDLE); // 获取标准输出设备句柄
    WORD att = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY | BACKGROUND_BLUE;// 背景是蓝色，文本颜色是黄色
    SetConsoleTextAttribute(con->hOut, att);
    ClearScreen();
    printf("\n\nThe soul selects her own society\n");
    printf("Then shuts the door;\n");
    printf("On her devine majority;\n");
    printf("Obtrude no more.\n\n");
    COORD endPos = {0, 15};
    SetConsoleCursorPosition(con->hOut, endPos); // 设置光标位置
    SMALL_RECT rc = {0, 2, 40, 5};
    _getch();
    MoveText(10, 5, rc);
    _getch();
    DeleteLine(5);
    CloseHandle(hOut); // 关闭标准输出设备句柄
    //*/

/*
    hOut = GetStdHandle(STD_OUTPUT_HANDLE); // 获取标准输出设备句柄
    hIn = GetStdHandle(STD_INPUT_HANDLE); // 获取标准输入设备句柄
    WORD att = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY | BACKGROUND_BLUE ; // 背景是蓝色，文本颜色是黄色
    SetConsoleTextAttribute(con->hOut, att);
    ClearScreen(); // 清屏
    INPUT_RECORD keyRec;
    DWORD state = 0, res;
    char ch;
    SMALL_RECT rc = {20, 2, 40, 12};
    DrawBox(false, rc);
    COORD pos = {rc->Left+1, rc->Top+1};
    SetConsoleCursorPosition(con->hOut, pos); // 设置光标位置
    for(;;) // 循环
    {
        ReadConsoleInput(hIn, &keyRec, 1, &res);
        if (state != keyRec.Event.KeyEvent.dwControlKeyState)
        {
            state = keyRec.Event.KeyEvent.dwControlKeyState;
            ControlStatus(state);
        }
        if (keyRec.EventType == KEY_EVENT)
        {
            if (keyRec.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE) 
                break;
            // 按ESC键退出循环
            if (keyRec.Event.KeyEvent.bKeyDown)
            {
                ch = keyRec.Event.KeyEvent.uChar.AsciiChar;
                CharWindow(ch, rc);
            }
        }
    }
    pos.X = 0; pos.Y = 0;
    SetConsoleCursorPosition(con->hOut, pos); // 设置光标位置
    CloseHandle(hOut); // 关闭标准输出设备句柄
    CloseHandle(hIn); // 关闭标准输入设备句柄
    //*/

/*
    hOut = GetStdHandle(STD_OUTPUT_HANDLE); // 获取标准输出设备句柄
    hIn = GetStdHandle(STD_INPUT_HANDLE); // 获取标准输入设备句柄
    WORD att = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY | BACKGROUND_BLUE ;
    // 背景是蓝色，文本颜色是黄色
    SetConsoleTextAttribute(con->hOut, att);
    ClearScreen(); // 清屏
    INPUT_RECORD mouseRec;
    DWORD state = 0, res;
    COORD pos = {0, 0};
    for(;;) // 循环
    {
        ReadConsoleInput(hIn, &mouseRec, 1, &res);
        if (mouseRec.EventType == MOUSE_EVENT)
        {
            if (mouseRec.Event.MouseEvent.dwEventFlags == DOUBLE_CLICK) 
                break; // 双击鼠标退出循环		
            pos = mouseRec.Event.MouseEvent.dwMousePosition;
            DispMousePos(pos);
            if (mouseRec.Event.MouseEvent.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED)
                FillConsoleOutputCharacter(con->hOut, 'A', 1, pos, NULL); 
        }
    } 
    pos.X = pos.Y = 0;
    SetConsoleCursorPosition(con->hOut, pos); // 设置光标位置
    CloseHandle(hOut); // 关闭标准输出设备句柄
    CloseHandle(hIn); // 关闭标准输入设备句柄
}
    //*/
