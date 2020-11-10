#ifndef HTMLFILE_H
#define HTMLFILE_H

#include "base.h"
#include "mylinkedlist.h"
#include "tools.h"

wchar_t* getWebWstr(const wchar_t* wurl);

wchar_t* getEccoLibWebClearWstring(void);

MyLinkedList getIdUrlMyLinkedList_xqbase(wchar_t sn_0);

void html_test(void);
#endif