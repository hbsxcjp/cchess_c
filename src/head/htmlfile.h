#ifndef HTMLFILE_H
#define HTMLFILE_H

#include "base.h"
#include "mylinkedlist.h"
#include "tools.h"

#define ECCO_IDMAX 12141

wchar_t* getWebWstr(const wchar_t* wurl);

wchar_t* getEccoLibWebClearWstring(void);

MyLinkedList getIdUrlMyLinkedList_xqbase(wchar_t sn_0);

MyLinkedList getIdUrlMyLinkedList_xqbase_2(int start, int end);

void html_test(void);
#endif