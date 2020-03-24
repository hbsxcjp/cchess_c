#ifndef BASE_H
#define BASE_H

#include <assert.h>
#include <conio.h>
#include <ctype.h>
#include <io.h>
#include <locale.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>
#include <windows.h>

#define THOUSAND 1024
#define WCHARSIZE 256
#define WIDEWCHARSIZE THOUSAND
#define SUPERWIDEWCHARSIZE (WIDEWCHARSIZE * 12)

/*

//=================================================================
//棋局演示相关的类型
//=================================================================

// 区域主题颜色配置类型
typedef enum {
    SIMPLE,
    SHOWY,
    //HIGHLIGHT
} Thema;

// 控制台焦点区域类型
typedef enum {
    MOVEA,
    CURMOVEA,
    BOARDA,
    OPENFILEA,
    SAVEFILEA,
    ABOUTA,
    MENUA,
    DECAREA, // 代表递减
    INCAREA, // 代表递增
    OLDAREA // 代表旧区域
} Area;

struct _Menu;
// 演示类型结构
typedef struct _Console {
    HANDLE hIn, hOut;
    PChessManual cm;
    Thema thema;
    Area curArea;
    int fileIndex;
    char fileName[FILENAME_MAX];
    SMALL_RECT WinRect, MenuRect, BoardRect, CurmoveRect, MoveRect, StatusRect, OpenFileRect;
    SMALL_RECT iMenuRect, iBoardRect, iCurmoveRect, iMoveRect, iStatusRect, iOpenFileRect;
    struct _Menu *rootMenu, *curMenu;
    //CHAR_INFO chBuf[40 * 120];
    //SMALL_RECT chBufRect;
} Console, *PConsole;

// 菜单命令
typedef void (*MENU_FUNC)(PConsole);

// 菜单结构
typedef struct _Menu {
    wchar_t name[WCHARSIZE], desc[WCHARSIZE];
    MENU_FUNC func; // 菜单关联的命令函数，如有子菜单则应为空
    struct _Menu *preMenu, *brotherMenu, *childMenu;
    int brotherIndex, childIndex;
} Menu, *PMenu;

// 菜单初始信息结构
typedef struct _MenuData {
    wchar_t name[WCHARSIZE], desc[WCHARSIZE];
    MENU_FUNC func;
} MenuData, *PMenuData;
//*/

#endif