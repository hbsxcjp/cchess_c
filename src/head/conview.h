#ifndef CONVIEW_H
#define CONVIEW_H

//#define PDC_WIDE

#include "base.h"
#include "curses.h"

typedef void (*MENU_FUNC)(void);

typedef struct menuNode {
    wchar_t* name; /* item label */
    MENU_FUNC func; /* (pointer to) function */
    wchar_t* desc; /* function description */
    struct menuNode *parentMenu, *childMenu, *preBrotherMenu, *nextBrotherMenu;
    bool selected;
    bool childsVisible;
} MENU;


void startView(void);

// 文本界面互动
//void textView(const wchar_t* dirName);

#endif