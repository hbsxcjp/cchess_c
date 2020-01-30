#ifndef CONVIEW_H
#define CONVIEW_H

#define PDC_WIDE

#include <curses.h>
//#include <panel.h>

#define MAXSTRLEN 256
#define MENUNUM 4
#define MENULEVEL 10
#define MENUWIDTH 9
#define KEY_ESC 0x1b /* Escape */

#define COLOR_V(num) ((short)((num) / 255.0 * 1000.0))
#define COLOR_PAIR_NUM(theme, area) ((theme) | (area))
#define THEMECOLOR(theme, area) COLOR_PAIR(COLOR_PAIR_NUM(theme, area))

// 控制台焦点区域类型
typedef enum {
    MENUA,
    BOARDA,
    MOVESA,
    CURMOVEA,
    STATUSA
} FocusArea;

// 区域主题颜色配置类型
typedef enum {
    SIMPLE = 0x10,
    SHOWY = 0x20,
    HIGHLIGHT = 0x30
} Theme;

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

void doView(void);

#endif