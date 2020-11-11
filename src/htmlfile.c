#include "head/htmlfile.h"
#include "head/chessManual.h"
#include <curl/curl.h>

extern FILE* fout;

// 输出到字符串
static size_t write_str__(void* inbuf, size_t size, size_t nmemb, void* str)
{
    strcat(str, inbuf);
    return size * nmemb;
}

/*/ 输出到文件
static size_t write_file__(void* inbuf, size_t size, size_t nmemb, FILE* fout)
{
    return fwrite(inbuf, size, nmemb, fout);
}
//*/

wchar_t* getWebWstr(const wchar_t* wurl)
{
    wchar_t* wstr = NULL;
    curl_global_init(CURL_GLOBAL_ALL);
    CURL* curl = curl_easy_init();

    static char str[SUPERWIDEWCHARSIZE * 200];
    strcpy(str, "");
    if (curl) {
        char url[WIDEWCHARSIZE];
        wcstombs(url, wurl, WIDEWCHARSIZE);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        //curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_str__);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, str);

        //curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_file__);
        //curl_easy_setopt(curl, CURLOPT_WRITEDATA, fout);

        curl_easy_perform(curl); //执行请求

        wstr = malloc((strlen(str) + 1) * sizeof(wchar_t));
        mbstowcs_gbk(wstr, str);

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return wstr;
}

typedef struct NoMatchOffset* NoMatchOffset;
struct NoMatchOffset {
    int start;
    int end;
};

static NoMatchOffset newNoMatchOffset__(int start, int end)
{
    NoMatchOffset noMatchOffset = malloc(sizeof(struct NoMatchOffset));
    noMatchOffset->start = start;
    noMatchOffset->end = end;

    return noMatchOffset;
}

static MyLinkedList getNoMatchOffsetMyLinkedList__(const wchar_t* wstr, void* reg)
{
    MyLinkedList noMatchOffsetMyLinkedList = newMyLinkedList(free);

    int ovector[PCREARRAY_SIZE], first = 0, last = wcslen(wstr);
    while (first < last) {
        const wchar_t* tempWstr = wstr + first;
        int count = pcrewch_exec(reg, NULL, tempWstr, last - first, 0, 0, ovector, PCREARRAY_SIZE);
        if (count <= 0)
            break;

        addMyLinkedList(noMatchOffsetMyLinkedList, newNoMatchOffset__(first, first + ovector[0]));
        first += ovector[1];
    }

    return noMatchOffsetMyLinkedList;
}

/*
static void printNoMatchOffset__(NoMatchOffset noMatchOffset, FILE* fout, int* pno, void* _)
{
    fwprintf(fout, L"No:%d %d-%d\n", (*pno)++, noMatchOffset->start, noMatchOffset->end);
}

static void printNoMatchOffsetMyLinkedList__(FILE* fout, MyLinkedList noMatchOffsetMyLinkedList)
{
    fwprintf(fout, L"noMatchOffsetMyLinkedList:\n");
    int no = 0;
    traverseMyLinkedList(noMatchOffsetMyLinkedList, (void (*)(void*, void*, void*, void*))printNoMatchOffset__,
        fout, &no, NULL);
}
//*/

static void wcscatDestStr__(NoMatchOffset noMatchOffset, wchar_t* destWstr, const wchar_t* wstr, void* _)
{
    wchar_t subStr[noMatchOffset->end - noMatchOffset->start + 1];
    copySubStr(subStr, wstr, noMatchOffset->start, noMatchOffset->end);
    wcscat(destWstr, subStr);
}

static wchar_t* getEccoWebWstring__(wchar_t sn_0)
{
    wchar_t *wurl = L"http://www.xqbase.com/ecco/ecco_%c.htm",
            wurl_x[WCHARSIZE];
    swprintf(wurl_x, WCHARSIZE, wurl, sn_0);
    wchar_t* wstr = getWebWstr(wurl_x);
    if (!wstr)
        fwprintf(fout, L"\n页面没有找到：%ls\n", wurl_x);

    return wstr;
}

static wchar_t* getAllEccoWebWstring__(void)
{
    size_t size = SUPERWIDEWCHARSIZE;
    wchar_t* webWstr = malloc(size * sizeof(wchar_t));
    for (wchar_t sn_0 = L'a'; sn_0 <= L'e'; ++sn_0) {
        wchar_t* tempWstr = getEccoWebWstring__(sn_0);
        if (!tempWstr)
            continue;

        supper_wcscat(&webWstr, &size, tempWstr);
        free(tempWstr);
    }

    return webWstr;
}

wchar_t* getEccoLibWebClearWstring(void)
{
    wchar_t *webWstr = getAllEccoWebWstring__(),
            *clearwstr = malloc((wcslen(webWstr) + 1) * sizeof(wchar_t));
    const char* error;
    int errorffset;
    wchar_t* regStr[] = {
        L"</?(?:div|font|img|strong|center|meta|dl|dt|table|tr|td|em|p|li|dir"
        "|html|head|body|title|a|b|tbody|script|br|span)[^>]*>",
        L"(?<=\\n|\\r)\\s+" //(?=\\n)
    };
    wchar_t* tempWstr = webWstr;
    for (int i = 0; i < sizeof(regStr) / sizeof(regStr[0]); ++i) {
        void* reg = pcrewch_compile(regStr[i], 0, &error, &errorffset, NULL);
        MyLinkedList noMatchOffsetMyLinkedList = getNoMatchOffsetMyLinkedList__(tempWstr, reg);
        wcscpy(clearwstr, L"");
        traverseMyLinkedList(noMatchOffsetMyLinkedList, (void (*)(void*, void*, void*, void*))wcscatDestStr__,
            clearwstr, (void*)tempWstr, NULL);

        tempWstr = clearwstr;
        //printNoMatchOffsetMyLinkedList__(fout, noMatchOffsetMyLinkedList);
        delMyLinkedList(noMatchOffsetMyLinkedList);
        pcrewch_free(reg);
    }

    free(webWstr);
    return clearwstr;
}

static MyLinkedList getEccoUrlMyLinkedList__(void* reg_sn, wchar_t sn_0)
{
    MyLinkedList eccoUrlMyLinkedList = newMyLinkedList((void (*)(void*))free);
    wchar_t* wstr = getEccoWebWstring__(sn_0);
    //wchar_t* wstr = L"http://www.xqbase.com/xqbase/?ecco=A02";
    int first = 0, last = wcslen(wstr), ovector[PCREARRAY_SIZE];
    while (first < last) {
        wchar_t* tempWstr = wstr + first;
        int count = pcrewch_exec(reg_sn, NULL, tempWstr, last - first, 0, 0, ovector, PCREARRAY_SIZE);
        if (count <= 0)
            break;

        addMyLinkedList(eccoUrlMyLinkedList, getSubStr(tempWstr, ovector[2], ovector[3]));
        first += ovector[1];
    }
    free(wstr);

    return eccoUrlMyLinkedList;
}

static void addIdUrlMyLinkedList__(MyLinkedList idUrlMyLinkedList, const wchar_t* gameids, void* reg_id)
{
    int ovector[PCREARRAY_SIZE];
    int first = 0, last = wcslen(gameids);
    while (first < last) {
        const wchar_t* tempWstr = gameids + first;
        int count = pcrewch_exec(reg_id, NULL, tempWstr, last - first, 0, 0, ovector, PCREARRAY_SIZE);
        if (count <= 0)
            break;

        wchar_t id[WCHARSIZE], wurl[WCHARSIZE];
        pcrewch_copy_substring(tempWstr, ovector, count, 1, id, WCHARSIZE);
        swprintf(wurl, WCHARSIZE, L"http://www.xqbase.com/xqbase/?gameid=%ls", id);
        addMyLinkedList(idUrlMyLinkedList, getSubStr(wurl, 0, wcslen(wurl)));
        first += ovector[1];
    }
}

static void appendIdUrlMyLinkedList__(const wchar_t* wurl, MyLinkedList idUrlMyLinkedList,
    void* reg_ids, void* reg_id)
{
    wchar_t* wstr = getWebWstr(wurl);
    if (!wstr) {
        fwprintf(fout, L"\n页面没有找到：%ls\n", wurl);
        return;
    }

    int ovector[PCREARRAY_SIZE],
        count = pcrewch_exec(reg_ids, NULL, wstr, wcslen(wstr), 0, 0, ovector, PCREARRAY_SIZE);
    if (count > 0) {
        wchar_t gameids[SUPERWIDEWCHARSIZE];
        pcrewch_copy_substring(wstr, ovector, count, 1, gameids, SUPERWIDEWCHARSIZE);
        addIdUrlMyLinkedList__(idUrlMyLinkedList, gameids, reg_id);
    }

    free(wstr);
}

MyLinkedList getIdUrlMyLinkedList_xqbase(wchar_t sn_0)
{
    MyLinkedList idUrlMyLinkedList = newMyLinkedList((void (*)(void*))free);

    const char* error;
    int errorffset;
    void *reg_sn = pcrewch_compile(L"(http://www.xqbase.com/xqbase/\\?ecco=[A-E]\\d{2})",
             0, &error, &errorffset, NULL),
         *reg_ids = pcrewch_compile(L"gameids = \\[([^\\]]+)\\]", 0, &error, &errorffset, NULL),
         *reg_id = pcrewch_compile(L"\\b(\\d+)\\b", 0, &error, &errorffset, NULL);

    MyLinkedList eccoUrlMyLinkedList = getEccoUrlMyLinkedList__(reg_sn, sn_0);
    //traverseMyLinkedList(eccoUrlMyLinkedList, (void (*)(void*, void*, void*, void*))printWstr__, fout, NULL, NULL);
    traverseMyLinkedList(eccoUrlMyLinkedList, (void (*)(void*, void*, void*, void*))appendIdUrlMyLinkedList__,
        idUrlMyLinkedList, reg_ids, reg_id);

    //traverseMyLinkedList(idUrlMyLinkedList, (void (*)(void*, void*, void*, void*))printWstr__, fout, NULL, NULL);
    delMyLinkedList(eccoUrlMyLinkedList);
    return idUrlMyLinkedList;
}

MyLinkedList getIdUrlMyLinkedList_xqbase_2(int start, int end)
{
    MyLinkedList idUrlMyLinkedList = newMyLinkedList((void (*)(void*))free);
    const wchar_t* wurl = L"http://www.xqbase.com/xqbase/?gameid=";
    wchar_t wurl_id[WCHARSIZE];
    for (int id = start; id < end; ++id) {
        swprintf(wurl_id, WCHARSIZE, L"%ls%d", wurl, id);
        addMyLinkedList(idUrlMyLinkedList, getSubStr(wurl_id, 0, wcslen(wurl_id)));
    }

    return idUrlMyLinkedList;
}

void html_test(void)
{
    //char* webFileName = "chessManual/ecco.htm";
    //getEccoLibSrcFile(webFileName);

    /*
    FILE* fout = openFile_utf8(webFileName, "w");
    fwprintf(fout, webwstr);
    fclose(fout);
    free(webwstr);
    //*/

    //ChessManual cm = getChessManual_idUrl(L"2010");
    //printCmMyLinkedList(fout, );
    //delChessManual(cm);

    //char* cleanFileName = "chessManual/ecco_lib.txt";
    //getCleanWebFile(cleanFileName, webFileName);

    //* 读取网页文件存入数据库
    //*/
}
