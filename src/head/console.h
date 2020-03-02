#ifndef CONSOLE_H
#define CONSOLE_H

#include "base.h"

PConsole newConsole(const char* fileName);

void delConsole(PConsole con);

void operateWin(PConsole con);

void keyEventProc(PConsole con, PKEY_EVENT_RECORD ker);
void mouseEventProc(PConsole con, PMOUSE_EVENT_RECORD ker);

// 返回是否还停留在菜单区
bool operateMenu(PConsole con, PKEY_EVENT_RECORD ker);

int getMaxSize(PMenu menu);
wchar_t* getWstr(wchar_t* wstr, PMenu menu);
SHORT getPosL(PMenu menu);
PMenu getTopIndexMenu(PMenu rootMenu, int index);
PMenu getTopMenu(PMenu menu);
PMenu getSameLevelMenu(PMenu rootMenu, PMenu menu, bool isLeft);
PMenu getBottomMenu(PMenu menu);

void initMenu(PConsole con);
// 生成一个菜单
PMenu newMenu(PMenuData menuData);
// 增加一个子菜单项
PMenu addChildMenu(PMenu preMenu, PMenu childMenu);
// 增加一个兄弟菜单项
PMenu addBrotherMenu(PMenu preMenu, PMenu brotherMenu);
void delMenu(PMenu menu);

bool operateBoard(PConsole con, PKEY_EVENT_RECORD ker);

// 返回当前着法是否变化
bool operateMove(PConsole con, PKEY_EVENT_RECORD ker);

// 返回当前注解是否变化
bool operateCurMove(PConsole con, PKEY_EVENT_RECORD ker);

void writeAreas(PConsole con);

COORD getSeatCoord(PConsole con, Seat seat);
void writeBoard(PConsole con);
void writeCurmove(PConsole con);
void writeMove(PConsole con);
void writeStatus(PConsole con);
void writeAreaLineChars(PConsole con, wchar_t* lineChars, const PSMALL_RECT rc, int firstRow, int firstCol, bool cutLine);
int getLine(wchar_t* lineChar, wchar_t** lineChars, int cols, bool cutLine);

void setArea(PConsole con, Area curArea, Area oldArea);
void initArea(PConsole con, WORD attr, const PSMALL_RECT rc, bool drawShadow, bool drawFrame);
// 底、右阴影色
void initAreaShadow(PConsole con, const PSMALL_RECT rc);
void initAreaFrame(PConsole con, const PSMALL_RECT rc);

void cleanAreaWIN(PConsole con);
void cleanSubMenuArea(PConsole con, const PSMALL_RECT rc, bool storgeMenu);

void cleanAreaChar(PConsole con, const PSMALL_RECT rc);
void cleanAreaAttr(PConsole con, WORD attr, const PSMALL_RECT rc);


#endif
