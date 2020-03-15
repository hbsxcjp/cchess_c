#ifndef CONSOLE_H
#define CONSOLE_H

#include "base.h"

PConsole newConsole(const char* fileName);
void delConsole(PConsole con);

// 菜单命令
void openFileArea(PConsole con);
void saveAsFileArea(PConsole con);
void exitPrograme(PConsole con);
void exchangeBoard(PConsole con);
void rotateBoard(PConsole con);
void symmetryBoard(PConsole con);
void setSimpleThema(PConsole con);
void setShowyThema(PConsole con);
void about(PConsole con);

// 输入控制函数
void operateWin(PConsole con);
void keyEventProc(PConsole con, PKEY_EVENT_RECORD ker);
void mouseEventProc(PConsole con, PMOUSE_EVENT_RECORD ker);

// 操纵区域
void operateBoard(PConsole con, PKEY_EVENT_RECORD ker);
void operateMove(PConsole con, PKEY_EVENT_RECORD ker);
void operateCurMove(PConsole con, PKEY_EVENT_RECORD ker);
void operateMenu(PConsole con, PKEY_EVENT_RECORD ker);
void operateOpenFile(PConsole con, PKEY_EVENT_RECORD ker);
void operateSaveFile(PConsole con, PKEY_EVENT_RECORD ker);

// 显示/清除 弹出区域
void showSubMenu(PConsole con, bool show);
void showOpenFile(PConsole con, bool show, bool openDir);
void showSaveFile(PConsole con, bool show);
void showAbout(PConsole con);

// 重新写入内容
void writeFixedAreas(PConsole con);
void writeBoard(PConsole con);
void writeCurmove(PConsole con);
void writeMove(PConsole con);
void writeStatus(PConsole con, Area area);

// 设置当前区域，并显示/清除 弹出区域
void setArea(PConsole con, Area area, int inc);

#endif
