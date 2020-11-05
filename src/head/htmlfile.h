#ifndef HTMLFILE_H
#define HTMLFILE_H

#include "base.h"
#include "mylinkedlist.h"
#include "tools.h"

wchar_t* getWebWstr(const char* url);

void getEccoLibSrcFile(const char* fileName);

void getCleanWebFile(const char* cleanFileName, const char* fileName);

void html_test(void);
#endif