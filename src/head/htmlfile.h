#ifndef HTMLFILE_H
#define HTMLFILE_H

#include "base.h"
#include "mylinkedlist.h"
#include "tools.h"

#define ECCO_IDMAX 12141

wchar_t* getWebWstr(const wchar_t* wurl);

wchar_t* getEccoLibWebClearWstring(void);

//
//for (wchar_t sn_0 = L'a'; sn_0 <= L'e'; ++sn_0) {
//for (wchar_t sn_0 = L'a'; sn_0 <= L'a'; ++sn_0) {
//MyLinkedList cmMyLinkedList = getCmMyLinkedList_xqbase__(sn_0);
//
//MyLinkedList idUrlMyLinkedList = getIdUrlMyLinkedList_xqbase_sn(sn_0);
//traverseMyLinkedList(idUrlMyLinkedList, (void (*)(void*, void*, void*, void*))printWstr, fout, NULL, NULL);
MyLinkedList getIdUrlMyLinkedList_xqbase_sn(wchar_t sn_0);

MyLinkedList getIdUrlMyLinkedList_xqbase_range(MyLinkedList _, int start, int end);

MyLinkedList getIdUrlMyLinkedList_xqbase_list(MyLinkedList widMyLinkedList, int start, int end);

void html_test(void);
#endif