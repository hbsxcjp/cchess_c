#ifndef HTMLFILE_H
#define HTMLFILE_H

#include "base.h"
#include "mylinkedlist.h"
#include "tools.h"

wchar_t* getWebWstr(const wchar_t* wurl);

wchar_t* getXqbaseEccoLibSrcWstring(void);

MyLinkedList getXqbaseGameidMyLinkedList(void);

void html_test(void);
#endif