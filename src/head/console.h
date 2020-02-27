#ifndef CONSOLE_H
#define CONSOLE_H

#include "base.h"

//#define WINROWS 50
//#define WINCOLS 150


PConsole newConsole(const wchar_t* fileName);

void delConsole(PConsole con);

void operateWin(PConsole con);

void keyEventProc(PConsole con, PKEY_EVENT_RECORD ker, Area oldArea);
void mouseEventProc(PConsole con, PMOUSE_EVENT_RECORD ker, Area oldArea);

bool operateMenu(PConsole con, PKEY_EVENT_RECORD ker, Area oldArea);
int getMaxSize(PMenu menu);
wchar_t* getWstr(PMenu menu, wchar_t* wstr);
SHORT getPosL(PMenu menu);
PMenu getTopIndexMenu(PMenu rootMenu, int index);
PMenu getTopMenu(PMenu menu);
PMenu getSameLevelMenu(PMenu menu, bool isLeft);
PMenu getBottomMenu(PMenu menu);
void initMenu(PConsole con);
void delMenu(PMenu menu);

bool operateBoard(PConsole con, PKEY_EVENT_RECORD ker, Area oldArea);
bool operateMove(PConsole con, PKEY_EVENT_RECORD ker, Area oldArea);
bool operateCurMove(PConsole con, PKEY_EVENT_RECORD ker, Area oldArea);

void writeAreas(PConsole con, );

void writeBoard(PConsole con);
void writeCurmove(PConsole con);
void writeMove(PConsole con);
void writeStatus(PConsole con);
void writeAreaLineChars(PConsole con, WORD attr, const wchar_t* lineChars, PSMALL_RECT rc, int firstRow, int firstCol, bool cutLine);

void initArea(PConsole con, WORD attr, PSMALL_RECT rc, bool drawFrame);

// 底、右阴影色
void initAreaShadow(PConsole con, PSMALL_RECT rc);

void cleanAreaWIN(PConsole con);
void cleanSubMenuArea(PConsole con);

void cleanAreaChar(PConsole con, PSMALL_RECT rc);
void cleanAreaAttr(PConsole con, WORD attr, PSMALL_RECT rc);

void writeCharBuf(PConsole con, PCHAR_INFO charBuf, COORD bufSize, COORD bufCoord, PSMALL_RECT writeRect);
void setCharBuf(PConsole con, PCHAR_INFO charBuf, COORD charCoord, const wchar_t* wchars, WORD attr);

#endif
