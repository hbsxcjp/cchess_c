#ifndef HTMLFILE_H
#define HTMLFILE_H

#include "base.h"
#include "mylinkedlist.h"
#include "tools.h"

#define ECCO_IDMAX 12141

wchar_t* getWebWstr(const wchar_t* wurl);

wchar_t* getEccoLibWebClearWstring(void);

MyLinkedList getIdUrlMyLinkedList_xqbase_range(int start, int end);

//traverseMyLinkedList(idUrlMyLinkedList, (void (*)(void*, void*, void*, void*))printWstr, fout, NULL, NULL);

#endif