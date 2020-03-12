#include "head/console.h"
#include "head/board.h"
#include "head/chessManual.h"
#include "head/move.h"
#include "head/piece.h"
#include "head/tools.h"

// 公用变量
static DWORD rwNum;
static wchar_t wstr[SUPERWIDEWCHARSIZE];
static COORD homePos = { 0, 0 };

// 界面初始化数据
static const char ProgramName[] = "中国象棋"; // 程序标题
static const SHORT WinRows = 45, WinCols = 130; // 窗口行列数
static const SHORT BoardRows = BOARDROW * 2 - 1, BoardCols = (BOARDCOL * 2 - 1) * 2, BoardTitleRows = 2; //棋盘行列数，标题行数
static const SHORT ShadowCols = 2, ShadowRows = 1, BorderCols = 2, BorderRows = 1; //阴影行列数，边框行列数
static const SHORT StatusRows = 2; //状态区行数
static const SHORT OpenFileRows = 35, OpenFileCols = 100;

/*
颜色属性由两个十六进制数字指定 -- 第一个对应于背景，第二个对应于前景。每个数字可以为以下任何值:
    0 = 黑色       8 = 灰色       
    1 = 蓝色       9 = 淡蓝色    
    2 = 绿色       A = 淡绿色
    3 = 浅绿色     B = 淡浅绿色   
    4 = 红色       C = 淡红色    
    5 = 紫色       D = 淡紫色
    6 = 黄色       E = 淡黄色     
    7 = 白色       F = 亮白色
//*/
// 与程序显示主题相关的属性数组，以主题序号引用属性数组
static const WORD WINATTR[] = { 0xF0, 0x70 }; // 窗口区
static const WORD MENUATTR[] = { 0x70, 0x4F }; // 菜单区
static const WORD BOARDATTR[] = { 0x70, 0xE2 }; // 棋盘区
static const WORD CURMOVEATTR[] = { 0x70, 0xB4 }; // 当前着法区
static const WORD MOVEATTR[] = { 0x70, 0x1F }; // 着法区
static const WORD STATUSATTR[] = { 0x70, 0x5F }; // 状态区
static const WORD SHADOWATTR[] = { 0x8F, 0x82 }; // 阴影

static const WORD ShowMenuAttr[] = { 0x70, 0x4F }; // 弹出菜单
static const WORD SelMenuAttr[] = { 0x07, 0x2F }; // 选中菜单项
//static const WORD SelMoveAttr[] = { 0x07, 0xF1 }; // 选中着法项
static const WORD OpenFileAttr[] = { 0x70, 0x2F }; // 打开文件区域
static const WORD SelFileAttr[] = { 0x07, 0xF1 }; // 选中文件项
//static const WORD SaveFileAttr[] = { 0x70, 0x2F }; // 保存区域
static const WORD AboutAttr[] = { 0x70, 0x2F }; // 关于区域

static const WORD RedAttr[] = { 0x4F, 0x4F }; // 红色棋子
static const WORD BlackAttr[] = { 0x0F, 0x0F }; // 黑色棋子
static const WORD RedSideAttr[] = { 0x74, 0xEC }; // 红色标题
static const WORD BlackSideAttr[] = { 0x70, 0xE0 }; // 黑色标题
static const WORD RedMoveAttr[] = { 0xCF, 0xCF }; // 移动红子
static const WORD BlackMoveAttr[] = { 0x1F, 0x9F }; // 移动黑子
//static const WORD SelRedAttr[] = { 0xFC, 0xC0 }; // 选中红子
//static const WORD SelBlackAttr[] = { 0xF0, 0xE0 }; // 选中黑子

static const WORD RedMoveLineAttr[] = { 0x74, 0x1C }; // 红方着法行
//static const WORD BlackMoveLineAttr[] = { 0x70, 0x1F }; // 黑方着法行
static const WORD CurmoveAttr[] = { 0x74, 0xBC }; // 当前着法
static const wchar_t* const TabChars[] = { L"─│┌┐└┘", L"═║╔╗╚╝" }; //L"─│┌┐└┘" 区域边框格式字符

// 本模块相关的内部函数
// 菜单相关
static int getMaxSize(PMenu menu);
static wchar_t* getWstr(wchar_t* wstr, PMenu menu);
static SHORT getPosL(PMenu menu);
static PMenu getTopIndexMenu(PMenu rootMenu, int index);
static PMenu getTopMenu(PMenu menu);
static PMenu getSameLevelMenu(PMenu rootMenu, PMenu menu, bool isLeft);
static PMenu getBottomMenu(PMenu menu);
static void initMenu(PConsole con);
static PMenu newMenu(PMenuData menuData);
static PMenu addChildMenu(PMenu preMenu, PMenu childMenu);
static PMenu addBrotherMenu(PMenu preMenu, PMenu brotherMenu);
static void delMenu(PMenu menu);

// 写入区域内容相关
static COORD getBoardSeatCoord(PConsole con, Seat seat);
static void writeAreaLineChars(PConsole con, wchar_t* lineChars, const PSMALL_RECT rc, int firstRow, int firstCol, bool cutLine);
static int getLine(wchar_t* lineChar, wchar_t** lineChars, int cols, bool cutLine);

// 存储/恢复 弹出区域屏幕块信息
static bool storageArea(PConsole con, const PSMALL_RECT rc);
static bool restoreArea(PConsole con);

// 初始化区域相关
static void initAreas(PConsole con);
static void clearArea(PConsole con, WORD attr, const PSMALL_RECT rc, bool drawShadow, bool drawFrame);
static void clearAreaShadow(PConsole con, const PSMALL_RECT rc); // 底、右阴影色
static void clearAreaFrame(PConsole con, const PSMALL_RECT rc); // 矩形边框
static void cleanAreaChar(PConsole con, const PSMALL_RECT rc);
static void cleanAreaAttr(PConsole con, WORD attr, const PSMALL_RECT rc);

// 便利函数
static COORD getCoordSize(const PSMALL_RECT rc);
static WORD reverseAttr(WORD attr);

static void setTitle(const char* fileName)
{
    char str[FILENAME_MAX];
    sprintf(str, "%s - %s", ProgramName, fileName);
    SetConsoleTitleA(str); // 设置窗口标题
}

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
    con->cm = newChessManual(fileName);
    con->thema = SHOWY;
    con->curArea = MOVEA;

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

    SMALL_RECT OpenFileRect = { (WinCols - OpenFileCols) / 2, (WinRows - OpenFileRows) / 2,
        (WinCols + OpenFileCols) / 2 - 1, (WinRows + OpenFileRows) / 2 - 1 };
    con->OpenFileRect = OpenFileRect;
    SMALL_RECT iOpenFileRect = { OpenFileRect.Left + BorderCols, OpenFileRect.Top + BorderRows,
        OpenFileRect.Right - BorderCols - ShadowCols, OpenFileRect.Bottom - BorderRows - ShadowRows };
    con->iOpenFileRect = iOpenFileRect;

    SetConsoleCP(936);
    SetConsoleOutputCP(936);
    SetConsoleMode(con->hIn, ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT); //ENABLE_PROCESSED_INPUT |

    COORD winCoord = { WinCols, WinRows };
    SetConsoleScreenBufferSize(con->hOut, winCoord);
    CONSOLE_CURSOR_INFO cInfo = { 5, false };
    SetConsoleCursorInfo(con->hOut, &cInfo);
    SetConsoleWindowInfo(con->hOut, true, &WinRect);
    SetConsoleActiveScreenBuffer(con->hOut);
    //SetConsoleTextAttribute(con->hOut, WINATTR[con->thema]);
    //GetConsoleScreenBufferInfo(con->hOut, &bInfo); // 获取窗口信息

    setTitle(fileName);
    initMenu(con);

    initAreas(con);
    writeFixedAreas(con, true);
    operateWin(con);

    return con;
}

void delConsole(PConsole con)
{
    CloseHandle(con->hOut);
    delMenu(con->rootMenu);
    delChessManual(con->cm);
    free(con);
}

void openFileArea(PConsole con) { setArea(con, OPENFILEA, 0); }

void saveAsFileArea(PConsole con) { setArea(con, SAVEFILEA, 0); }

void exitPrograme(PConsole con) { exit(0); }

void __changeBoard(PConsole con, ChangeType ct)
{
    setArea(con, OLDAREA, 0);
    changeChessManual(con->cm, ct);
    writeFixedAreas(con, true);
}

void exchangeBoard(PConsole con) { __changeBoard(con, EXCHANGE); }

void rotateBoard(PConsole con) { __changeBoard(con, ROTATE); }

void symmetryBoard(PConsole con) { __changeBoard(con, SYMMETRY); }

void __setThema(PConsole con, Thema thema)
{
    if (thema != con->thema) {
        setArea(con, OLDAREA, 0);
        con->thema = thema;
        initAreas(con);
        writeFixedAreas(con, true);
    }
}

void setSimpleThema(PConsole con) { __setThema(con, SIMPLE); }

void setShowyThema(PConsole con) { __setThema(con, SHOWY); }

void about(PConsole con)
{
    setArea(con, ABOUTA, 0);
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
                WORD key = ker->wVirtualKeyCode;
                DWORD keys = ker->dwControlKeyState;
                if (!ker->bKeyDown)
                    break;

                if (((keys & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED))
                        && key == VK_F4)
                    || ((keys & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
                           && key == 'C'))
                    return;
                else if (key == VK_ESCAPE) {
                    setArea(con, OLDAREA, 0);
                    break;
                } else if (key == VK_TAB) {
                    setArea(con, OLDAREA, (keys & SHIFT_PRESSED) ? -1 : 1);
                    break;
                } else if (keys & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) {
                    setArea(con, MENUA, 0);
                }
                printf("k:%x ", key);
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
        operateMove(con, ker);
        break;
    case CURMOVEA:
        operateCurMove(con, ker);
        break;
    case BOARDA:
        operateBoard(con, ker);
        break;
    case MENUA:
        operateMenu(con, ker);
        break;
    case OPENFILEA:
        operateOpenFile(con, ker);
        break;
    case SAVEFILEA:
        operateSaveFile(con, ker);
        break;
    case ABOUTA:
        setArea(con, OLDAREA, 0);
        break;
    default:
        break;
    }
}

void mouseEventProc(PConsole con, PMOUSE_EVENT_RECORD ker)
{
}

void operateBoard(PConsole con, PKEY_EVENT_RECORD keroldArea)
{
}

void operateMove(PConsole con, PKEY_EVENT_RECORD ker)
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
        setArea(con, CURMOVEA, 0);
        break;
    default:
        printf("key:%x/", ker->wVirtualKeyCode);
        break;
    }
    if (oldCurMove != con->cm->currentMove)
        writeFixedAreas(con, false); // 根据当前着法变化判断是否需要刷新屏幕
}

void operateCurMove(PConsole con, PKEY_EVENT_RECORD ker)
{
    COORD pos = { con->iCurmoveRect.Left, con->iCurmoveRect.Bottom - 3 };
    SetConsoleCursorPosition(con->hOut, pos);
}

void operateMenu(PConsole con, PKEY_EVENT_RECORD ker)
{
    PMenu curMenu = con->curMenu;
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
        if (con->curMenu->func)
            con->curMenu->func(con); // 执行当前菜单对应的命令
        return;
    default:
        return;
    }

    if (curMenu != con->curMenu)
        setArea(con, MENUA, 0); //showSubMenu(con, true);
}

void operateOpenFile(PConsole con, PKEY_EVENT_RECORD ker)
{
    bool openDir = false;
    int pageRows = con->iOpenFileRect.Bottom - con->iOpenFileRect.Top + 1 - 2; // -2:标题占2行
    switch (ker->wVirtualKeyCode) {
    case VK_DOWN:
        ++con->fileIndex;
        break;
    case VK_UP:
        --con->fileIndex;
        break;
    case VK_NEXT:
        con->fileIndex += pageRows;
        break;
    case VK_PRIOR:
        con->fileIndex -= pageRows;
        break;
    case VK_END:
        con->fileIndex = THOUSAND;
        break;
    case VK_HOME:
        con->fileIndex = 0;
        break;
    case VK_RETURN:
        if (strlen(con->fileName) > 0) { // 在showOpenFile函数里设置fileName
            if (getRecFormat(getExt(con->fileName)) != NOTFMT) {
                setArea(con, OLDAREA, 0);
                resetChessManual(&con->cm, con->fileName);
                setTitle(con->fileName);
                writeFixedAreas(con, true);
            }
            return;
        } else
            openDir = true;
        break;
    default:
        return;
    }

    showOpenFile(con, true, openDir);
}

void operateSaveFile(PConsole con, PKEY_EVENT_RECORD ker)
{
}

void showSubMenu(PConsole con, bool show)
{
    static bool hasTopSelMenu = false, hasStorageArea = false;
    if (hasTopSelMenu) {
        cleanAreaAttr(con, MENUATTR[con->thema], &con->iMenuRect); // 清除菜单顶行背景
        hasTopSelMenu = false;
    }
    if (hasStorageArea)
        hasStorageArea = restoreArea(con);
    if (!show) // 不需要显示
        return;

    SHORT level = con->curMenu->childIndex;
    SHORT posL = getPosL(con->curMenu) + ShadowCols,
          posT = con->iMenuRect.Bottom + (level == 0 ? 0 : 1),
          posR = posL + (level == 0 ? (wcslen(con->curMenu->name) + 3) : getMaxSize(con->curMenu)) + ShadowCols,
          posB = posT + (level == 0 ? 0 : getBottomMenu(con->curMenu)->childIndex - 1) + ShadowRows;
    SMALL_RECT rect = { posL, posT, posR, posB };

    if (level > 0) {
        hasStorageArea = storageArea(con, &rect);

        // 显示子菜单屏幕区
        clearArea(con, ShowMenuAttr[con->thema], &rect, true, false);
        SMALL_RECT irect = { posL, posT, posR - ShadowCols, posB - ShadowRows };
        writeAreaLineChars(con, getWstr(wstr, con->curMenu), &irect, 0, 0, false);
    }
    hasTopSelMenu = level == 0; // 保存是否有顶层菜单被选
    // 选择项目突显
    COORD pos = { posL, level };
    FillConsoleOutputAttribute(con->hOut, SelMenuAttr[con->thema], posR - posL - ShadowCols + 1, pos, &rwNum);
}

static int __getFileIndex(int index, int fileCount)
{
    return (index >= fileCount) ? fileCount - 1 : (index < 0 ? 0 : index);
}

void showOpenFile(PConsole con, bool show, bool openDir)
{
    static bool hasStorageArea = false;
    // 恢复显示区域
    if (!show) {
        if (hasStorageArea)
            hasStorageArea = restoreArea(con);
        return;
    }
    // 初始化显示区域
    if (!hasStorageArea) {
        hasStorageArea = storageArea(con, &con->OpenFileRect);
        clearArea(con, OpenFileAttr[con->thema], &con->OpenFileRect, true, true);
        COORD pos = { con->iOpenFileRect.Left, con->iOpenFileRect.Top + 1 };
        wcscpy(wstr, L"序号  属性  大小(B)  文件名称");
        WriteConsoleOutputCharacterW(con->hOut, wstr, wcslen(wstr), pos, &rwNum);
    }

    // 打开指定目录
    static bool hasFileInfos = false;
    static wchar_t dirName[FILENAME_MAX] = L"."; //c:\\棋谱
    static struct _wfinddata_t fileInfos[THOUSAND] = { 0 };
    static int fileCount = 0;
    if (openDir && hasFileInfos) {
        con->fileIndex = __getFileIndex(con->fileIndex, fileCount);
        if (fileInfos[con->fileIndex].attrib == _A_SUBDIR) {
            wchar_t* pwch = NULL;
            if (wcscmp(fileInfos[con->fileIndex].name, L"..") == 0 && (pwch = wcsrchr(dirName, L'\\')) != NULL)
                pwch[0] = L'\x0';
            else
                wcscat(wcscat(dirName, L"\\"), fileInfos[con->fileIndex].name);
            hasFileInfos = false;
            con->fileIndex = 0;
        }
    }
    // 提取目录下文件清单
    int cols = OpenFileCols - BorderCols * 2 - ShadowCols;
    if (!hasFileInfos) {
        fileCount = 0;
        getFileInfos(fileInfos, &fileCount, THOUSAND, dirName, false);
    }
    if (fileCount > 0)
        hasFileInfos = true;
    else
        return;

    // 存储选定的文件名
    con->fileIndex = __getFileIndex(con->fileIndex, fileCount);
    if (fileInfos[con->fileIndex].attrib != _A_SUBDIR) {
        swprintf(wstr, FILENAME_MAX, L"%s\\%s", dirName, fileInfos[con->fileIndex].name);
        wcstombs(con->fileName, wstr, FILENAME_MAX);
    } else
        strcpy(con->fileName, "");

    // 文件清单转换为字符串, 再写入区域
    SMALL_RECT irect = { con->iOpenFileRect.Left, con->iOpenFileRect.Top + 2, con->iOpenFileRect.Right, con->iOpenFileRect.Bottom };
    int pageRows = irect.Bottom - irect.Top + 1,
        firstRow = con->fileIndex / pageRows * pageRows,
        selIndex = con->fileIndex % pageRows;
    wcscpy(wstr, L"");
    wchar_t tempStr[FILENAME_MAX];
    for (int i = 0; i < fileCount; ++i) {
        swprintf(tempStr, FILENAME_MAX, L"%3d   %s%9d  %s\n",
            i, fileInfos[i].attrib == 0x10 ? L"目录" : L"文件", fileInfos[i].size, fileInfos[i].name);
        wcscat(wstr, tempStr);
    }
    writeAreaLineChars(con, wstr, &irect, firstRow, 0, true);

    // 写入标题行
    COORD dpos = { con->iOpenFileRect.Left, con->iOpenFileRect.Top };
    wcscat(wcscpy(wstr, L"当前目录："), dirName);
    FillConsoleOutputCharacter(con->hOut, L' ', cols, dpos, &rwNum);
    WriteConsoleOutputCharacterW(con->hOut, wstr, wcslen(wstr), dpos, &rwNum);

    // 选择项目突显
    cleanAreaAttr(con, OpenFileAttr[con->thema], &irect);
    COORD pos = { irect.Left, irect.Top + selIndex % pageRows };
    FillConsoleOutputAttribute(con->hOut, SelFileAttr[con->thema], cols, pos, &rwNum);
}

void showSaveFile(PConsole con, bool show)
{
}

void showAbout(PConsole con)
{
    static bool hasStorageArea = false;
    if (hasStorageArea)
        hasStorageArea = restoreArea(con);
    else {
        int width = 36, height = 9;
        COORD pos = { (WinCols - width) / 2, (WinRows - height) / 2 };
        SMALL_RECT rect = { pos.X, pos.Y, pos.X + width - 1, pos.Y + height - 1 };
        hasStorageArea = storageArea(con, &rect); // 存储本显示区

        clearArea(con, AboutAttr[con->thema], &rect, true, true);
        wcscpy(wstr, L"       中国象棋打谱软件\n\n"
                     "            陈建平\n"
                     "           2020.3.6\n\n"
                     "  防控新冠病毒，宅家修炼技术！");
        SMALL_RECT irect = { rect.Left + BorderCols, rect.Top + BorderRows,
            rect.Right - BorderCols - ShadowCols, rect.Bottom - BorderRows - ShadowRows };
        writeAreaLineChars(con, wstr, &irect, 0, 0, false);
    }
}

void writeFixedAreas(PConsole con, bool refresh)
{
    writeBoard(con);
    writeCurmove(con);
    writeMove(con, refresh);
    //setArea(con, OLDAREA, 0);
}

void writeBoard(PConsole con)
{
    // 棋盘顶、底标题行初始化
    static PieceColor bottomColor = -1;
    if (bottomColor != con->cm->board->bottomColor) {
        SMALL_RECT iBoardRect = con->iBoardRect,
                   iPreBoardRect = { iBoardRect.Left, iBoardRect.Top, iBoardRect.Right, iBoardRect.Top + BoardTitleRows - 1 },
                   iSufBoardRect = { iBoardRect.Left, iBoardRect.Bottom - BoardTitleRows + 1, iBoardRect.Right, iBoardRect.Bottom };
        writeAreaLineChars(con, getBoardPreString(wstr, con->cm->board), &iPreBoardRect, 0, 0, true);
        writeAreaLineChars(con, getBoardSufString(wstr, con->cm->board), &iSufBoardRect, 0, 0, true);
        // 顶、底两行上颜色
        bool bottomIsRed = isBottomSide(con->cm->board, RED);
        WORD bottomAttr = bottomIsRed ? RedSideAttr[con->thema] : BlackSideAttr[con->thema],
             topAttr = bottomIsRed ? BlackSideAttr[con->thema] : RedSideAttr[con->thema];
        for (int row = 0; row < BoardTitleRows; ++row) {
            COORD tpos = { iBoardRect.Left, (iBoardRect.Top + row) }, bpos = { iBoardRect.Left, (iBoardRect.Bottom - row) };
            FillConsoleOutputAttribute(con->hOut, topAttr, BoardCols, tpos, &rwNum);
            FillConsoleOutputAttribute(con->hOut, bottomAttr, BoardCols, bpos, &rwNum);
        }
        bottomColor = con->cm->board->bottomColor;
    }

    SMALL_RECT iBoardRect = con->iBoardRect;
    // 清除背景颜色，重绘棋盘字符串
    SMALL_RECT iPieBoardRect = { iBoardRect.Left, iBoardRect.Top + BoardTitleRows, iBoardRect.Right, iBoardRect.Bottom - BoardTitleRows };
    cleanAreaAttr(con, BOARDATTR[con->thema], &iPieBoardRect);
    // 棋子文字上颜色
    wchar_t ch;
    getPieChars_board(wstr, con->cm->board); // 行:从上到下
    for (int i = 0; i < SEATNUM; ++i) {
        if ((ch = wstr[i]) != BLANKCHAR)
            //字符属性函数不识别全角字符，均按半角字符计数
            FillConsoleOutputAttribute(con->hOut, (getColor_ch(ch) == RED ? RedAttr[con->thema] : BlackAttr[con->thema]), 2,
                getBoardSeatCoord(con, getSeat_rc(BOARDROW - 1 - i / BOARDCOL, i % BOARDCOL)), &rwNum);
    }
    writeAreaLineChars(con, getBoardString(wstr, con->cm->board), &iPieBoardRect, 0, 0, true);
}

void writeCurmove(PConsole con)
{
    SMALL_RECT iCurmoveRect = con->iCurmoveRect;
    writeAreaLineChars(con, getMoveStr(wstr, con->cm->currentMove), &iCurmoveRect, 0, 0, false);
}

void writeMove(PConsole con, bool refresh)
{
    static int firstRow = 0, firstCol = 0, noWidth = 4; //静态变量，只初始化一次
    SMALL_RECT iMoveRect = { con->iMoveRect.Left + noWidth, con->iMoveRect.Top, con->iMoveRect.Right, con->iMoveRect.Bottom };
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
    // 当行、列超出显示范围，或主题更改时，写入字符串
    if (refresh || offsetCol != 0 || offsetRow != 0) {
        firstRow += offsetRow;
        firstCol += offsetCol;
        wchar_t* wstr_PGN_CC = NULL;
        writeMove_PGN_CCtoWstr(con->cm, &wstr_PGN_CC);
        writeAreaLineChars(con, wstr_PGN_CC, &iMoveRect, firstRow, firstCol, true);
        free(wstr_PGN_CC);

        wcscpy(wstr, L"");
        wchar_t tempStr[8];
        SMALL_RECT iLineRect = { iMoveRect.Left - noWidth, iMoveRect.Top, iMoveRect.Left - 1, iMoveRect.Bottom };
        int maxRow = min(iLineRect.Bottom + firstRow, con->cm->maxRow_ * 2);
        for (int row = firstRow; row <= maxRow; ++row) {
            if (row % 2 == 0) {
                swprintf(tempStr, 6, L"%3d", row / 2);
                wcscat(wstr, tempStr);
            }
            wcscat(wstr, L"\n");
        }
        writeAreaLineChars(con, wstr, &iLineRect, 0, 0, true);
    }

    // 清除背景
    cleanAreaAttr(con, MOVEATTR[con->thema], &con->iMoveRect);

    //*// 设置着法间隔行颜色
    int cols = con->iMoveRect.Right - con->iMoveRect.Left + 1;
    for (int row = iMoveRect.Top + ((firstRow % 4 == 0) ? 2 : 0);
         row <= iMoveRect.Bottom; row += 4) {
        COORD pos = { con->iMoveRect.Left, row }; //
        //FillConsoleOutputAttribute(con->hOut, RedMoveLineAttr[con->thema], cols, pos, &rwNum);
    } //*/

    // 设置rootMove背景
    PMove curMove = con->cm->currentMove;
    if (curMove == con->cm->rootMove) {
        COORD pos = { iMoveRect.Left, iMoveRect.Top };
        //FillConsoleOutputAttribute(con->hOut, reverseAttr(MOVEATTR[con->thema]), 4 * 2, pos, &rwNum);
        return;
    }
    // 设置选定Move背景
    Piece tpiece = getPiece_s(con->cm->board, curMove->tseat);
    WORD attr = (getColor(tpiece) == RED ? RedMoveAttr[con->thema] : BlackMoveAttr[con->thema]);
    showCoord.X -= firstCol;
    showCoord.Y -= firstRow;
    //FillConsoleOutputAttribute(con->hOut, attr, 4 * 2, showCoord, &rwNum);

    // 设置棋盘区移动棋子突显
    FillConsoleOutputAttribute(con->hOut, attr, 2, getBoardSeatCoord(con, curMove->fseat), &rwNum);
    FillConsoleOutputAttribute(con->hOut, attr, 2, getBoardSeatCoord(con, curMove->tseat), &rwNum);
    // 如对方被将毙，棋盘区将帅棋子突显
    PieceColor otherColor = getOtherColor(tpiece);
    if (isDied(con->cm->board, otherColor)) {
        COORD kcoord = getBoardSeatCoord(con, getKingSeat(con->cm->board, otherColor));
        WORD kattr = reverseAttr(otherColor == RED ? RedAttr[con->thema] : BlackAttr[con->thema]); // 反转颜色
        FillConsoleOutputAttribute(con->hOut, kattr, 2, kcoord, &rwNum);
    }
}

void writeStatus(PConsole con, Area area)
{
    switch (area) {
    case MOVEA:
        wcscpy(wstr, L"【导航】 <↑>前  <↓>下  <←>前变  <→>下变  <PageDown>进10着   <PageUp>退10着   <Home>开始   <End>结束    <Enter>转注解区");
        break;
    case CURMOVEA:
        wcscpy(wstr, L"【注解】 <↑>上  <↓>下  <←>左  <→>右  进入注解文字编辑状态: 插入模式");
        break;
    case BOARDA:
        wcscpy(wstr, L"【盘面】 <↑>上  <↓>下  <←>左  <→>右  <PageDown><End>底  <PageUp><Home>顶");
        break;
    case MENUA:
        wcscpy(wstr, L"【菜单】 <↑>上  <↓>下  <←>左  <→>右  <PageDown><End>底  <PageUp><Home>顶  <");
        wcscat(wstr, con->curMenu->name);
        wcscat(wstr, L" > ");
        wcscat(wstr, con->curMenu->desc);
        break;
    case OPENFILEA:
        wcscpy(wstr, L"【打开】 <↑>上  <↓>下  <PageDown><End>底  <PageUp><Home>顶    <Enter>打开      <Esc>放弃");
        break;
    case SAVEFILEA:
        wcscpy(wstr, L"【保存】 <↑>上  <↓>下  <←>左  <→>右      <Enter>保存      <Esc>放弃");
        break;
    case ABOUTA:
        wcscpy(wstr, L"【关于】 按任意键返回原操作区域");
        break;
    default:
        wcscpy(wstr, L"???");
        break;
    }
    COORD pos = { con->iStatusRect.Left, con->iStatusRect.Top };
    FillConsoleOutputCharacter(con->hOut, L' ', con->iStatusRect.Right - con->iStatusRect.Left + 1, pos, &rwNum);
    WriteConsoleOutputCharacterW(con->hOut, wstr, wcslen(wstr), pos, &rwNum);
}

COORD getBoardSeatCoord(PConsole con, Seat seat)
{
    int row = getRow_s(seat), col = getCol_s(seat);
    COORD coord = { con->iBoardRect.Left + col * 4, con->iBoardRect.Bottom - BoardTitleRows - row * 2 };
    return coord;
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
        { { L" 文件(F)", L"打开、保存文件，退出", NULL }, //新建、
            //{ L" 新建", L"新建一个棋局", NULL },
            { L" 打开...", L"打开已有的一个棋局", &openFileArea },
            { L" 另存为...", L"将显示的棋局另存为一个棋局", &saveAsFileArea },
            //{ L" 保存", L"保存正在显示的棋局", NULL },
            { L" 退出", L"退出程序", &exitPrograme },
            { L"", L"", NULL } },
        { { L" 棋局(B)", L"对棋盘局面进行操作", NULL },
            { L" 对换棋子", L"红黑棋子互换", &exchangeBoard },
            { L" 对换座位", L"红黑位置互换", &rotateBoard },
            { L" 棋子左右换位", L"棋子的位置左右对称换位", &symmetryBoard },
            { L"", L"", NULL } },
        { { L" 设置(S)", L"设置显示主题，配置不同颜色", NULL },
            { L" 静雅朴素", L"比较朴素的颜色配置", &setSimpleThema },
            { L" 鲜艳亮丽", L"比较鲜艳的颜色配置", &setShowyThema },
            //{ L" 高对比度", L"高对比度的颜色配置", NULL },
            { L"", L"", NULL } },
        { { L" 关于(A)", L"程序信息", NULL }, //帮助、
            //{ L" 帮助", L"显示帮助信息", NULL },
            { L" 程序信息", L"展示与本程序有关的信息", &about },
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

void setArea(PConsole con, Area area, int inc)
{
    static Area oldArea = MOVEA;
    if (inc != 0)
        area = (con->curArea + inc + 3) % 3; // 可存入oldArea的三个常显区域循环
    else if (area == OLDAREA)
        area = oldArea;

    switch (con->curArea) { // 退出弹出区域
    case MENUA:
        showSubMenu(con, false);
        break;
    case OPENFILEA:
        showOpenFile(con, false, false);
        break;
    case SAVEFILEA:
        showSaveFile(con, false);
        break;
    case ABOUTA:
        showAbout(con);
        break;
    default:
        break;
    }

    if (con->curArea == MOVEA || con->curArea == CURMOVEA || con->curArea == BOARDA)
        oldArea = con->curArea;
    switch ((con->curArea = area)) { // 进入弹出区域
    case MENUA:
        showSubMenu(con, true);
        break;
    case OPENFILEA:
        showOpenFile(con, true, false);
        break;
    case SAVEFILEA:
        showSaveFile(con, true);
        break;
    case ABOUTA:
        showAbout(con);
        break;
    default:
        break;
    }
    writeStatus(con, area);
}

bool storageArea(PConsole con, const PSMALL_RECT rc)
{
    con->chBufRect = *rc;
    ReadConsoleOutputW(con->hOut, con->chBuf, getCoordSize(rc), homePos, rc);
    return true;
}

bool restoreArea(PConsole con)
{
    WriteConsoleOutputW(con->hOut, con->chBuf, getCoordSize(&con->chBufRect), homePos, &con->chBufRect);
    return false;
}

void initAreas(PConsole con)
{
    clearArea(con, WINATTR[con->thema], &con->WinRect, false, false);
    clearArea(con, MENUATTR[con->thema], &con->MenuRect, true, false);
    clearArea(con, BOARDATTR[con->thema], &con->BoardRect, true, true);
    clearArea(con, CURMOVEATTR[con->thema], &con->CurmoveRect, true, true);
    clearArea(con, MOVEATTR[con->thema], &con->MoveRect, true, true);
    clearArea(con, STATUSATTR[con->thema], &con->StatusRect, true, false);

    // 菜单区域初始化
    PMenu brotherMenu = con->rootMenu;
    int col = ShadowCols;
    while ((brotherMenu = brotherMenu->brotherMenu)) {
        int namelen = wcslen(brotherMenu->name);
        COORD pos = { col, con->MenuRect.Top };
        WriteConsoleOutputCharacterW(con->hOut, brotherMenu->name, namelen, pos, &rwNum);
        col += namelen + 3;
    }

    // 注解区设置当前着法、注解的突显颜色
    SMALL_RECT iCurmoveRect = con->iCurmoveRect;
    int cols = iCurmoveRect.Right - iCurmoveRect.Left + 1;
    int rows[] = { 1, 5, 6, 7 };
    WORD attr = CurmoveAttr[con->thema];
    for (int i = sizeof(rows) / sizeof(rows[0]) - 1; i >= 0; --i) {
        COORD pos = { iCurmoveRect.Left, (iCurmoveRect.Top + rows[i]) };
        FillConsoleOutputAttribute(con->hOut, attr, cols, pos, &rwNum);
    }

    // 状态区底行初始化
    wcscpy(wstr, L" <Tab><Shift+Tab>区间转换(导航>>注解>>盘面>>菜单)   <Alt>进入/退出(菜单)"
                 "   <Esc>退出(菜单/程序)   <Ctrl+C><Alt+F4>退出程序");
    COORD pos = { con->iStatusRect.Left, con->iStatusRect.Bottom };
    WriteConsoleOutputCharacterW(con->hOut, wstr, wcslen(wstr), pos, &rwNum);
}

void clearArea(PConsole con, WORD attr, const PSMALL_RECT rc, bool drawShadow, bool drawFrame)
{
    SMALL_RECT irc = *rc;
    if (drawShadow) {
        clearAreaShadow(con, rc);
        irc.Right -= ShadowCols;
        irc.Bottom -= ShadowRows;
    }
    cleanAreaAttr(con, attr, &irc);
    cleanAreaChar(con, &irc);
    if (drawFrame)
        clearAreaFrame(con, &irc);
}

void clearAreaShadow(PConsole con, const PSMALL_RECT rc)
{
    // 底行
    COORD pos = { (rc->Left + ShadowCols), rc->Bottom };
    FillConsoleOutputAttribute(con->hOut, SHADOWATTR[con->thema], rc->Right - rc->Left + 1 - ShadowCols, pos, &rwNum);
    SHORT right = rc->Right - ShadowCols + 1;
    // 右列 只有两行的区域无顶部空白
    for (SHORT row = rc->Top + (rc->Bottom == rc->Top + 1 ? 0 : 1); row <= rc->Bottom; ++row) {
        COORD pos = { right, row };
        FillConsoleOutputAttribute(con->hOut, SHADOWATTR[con->thema], ShadowCols, pos, &rwNum);
    }
}

void clearAreaFrame(PConsole con, const PSMALL_RECT rc)
{
    SHORT left = rc->Left + 1, right = rc->Right - 1;
    const wchar_t* const tabChar = TabChars[con->thema];
    // 顶、底行
    int rows[] = { rc->Top, rc->Bottom };
    for (int i = sizeof(rows) / sizeof(rows[0]) - 1; i >= 0; --i) {
        COORD pos = { left, rows[i] };
        FillConsoleOutputCharacterW(con->hOut, tabChar[0], rc->Right - rc->Left - 2, pos, &rwNum);
    }
    // 左、右列
    for (SHORT row = rc->Top + 1; row < rc->Bottom; ++row) {
        int cols[] = { rc->Left, right };
        for (int i = sizeof(cols) / sizeof(cols[0]) - 1; i >= 0; --i) {
            COORD pos = { cols[i], row };
            FillConsoleOutputCharacterW(con->hOut, tabChar[1], 1, pos, &rwNum);
        }
    }
    // 四个角
    COORD posLT = { rc->Left, rc->Top };
    COORD posRT = { right, rc->Top };
    COORD posLB = { rc->Left, rc->Bottom };
    COORD posRB = { right, rc->Bottom };
    FillConsoleOutputCharacterW(con->hOut, tabChar[2], 1, posLT, &rwNum);
    FillConsoleOutputCharacterW(con->hOut, tabChar[3], 1, posRT, &rwNum);
    FillConsoleOutputCharacterW(con->hOut, tabChar[4], 1, posLB, &rwNum);
    FillConsoleOutputCharacterW(con->hOut, tabChar[5], 1, posRB, &rwNum);
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

COORD getCoordSize(const PSMALL_RECT rc)
{
    COORD rcSize = { rc->Right - rc->Left + 1, rc->Bottom - rc->Top + 1 };
    return rcSize;
}

WORD reverseAttr(WORD attr)
{
    return (attr >> 4) | ((attr & 0x0F) << 4);
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
