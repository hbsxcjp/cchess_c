#include "head/htmlfile.h"
#include <curl/curl.h>

extern FILE* fout;

typedef struct SubOffset* SubOffset;
struct SubOffset {
    int start;
    int end;
};

static SubOffset newSubOffset__(int start, int end)
{
    SubOffset subOffset = malloc(sizeof(struct SubOffset));
    subOffset->start = start;
    subOffset->end = end;

    return subOffset;
}

static void delSubOffset__(SubOffset subOffset)
{
    free(subOffset);
}

static MyLinkedList getSubOffsetMyLinkedList(const wchar_t* wstr, void* reg)
{
    MyLinkedList subOffsetMyLinkedList = newMyLinkedList((void (*)(void*))delSubOffset__);

    int ovector[PCREARRAY_SIZE], first = 0, last = wcslen(wstr);
    while (first < last) {
        const wchar_t* tempWstr = wstr + first;
        int count = pcrewch_exec(reg, NULL, tempWstr, last - first, 0, 0, ovector, PCREARRAY_SIZE);
        if (count <= 0)
            break;

        addMyLinkedList(subOffsetMyLinkedList, newSubOffset__(first, first + ovector[0]));
        first += ovector[1];
    }

    return subOffsetMyLinkedList;
}

static void printSubOffset__(SubOffset subOffset, FILE* fout, int* pno, void* _)
{
    fwprintf(fout, L"No:%d %d-%d\n", (*pno)++, subOffset->start, subOffset->end);
}

static void printSubOffsetMyLinkedList__(FILE* fout, MyLinkedList subOffsetMyLinkedList)
{
    fwprintf(fout, L"subOffsetMyLinkedList:\n");
    int no = 0;
    traverseMyLinkedList(subOffsetMyLinkedList, (void (*)(void*, void*, void*, void*))printSubOffset__,
        fout, &no, NULL);
}

static void wcscatDestStr__(SubOffset subOffset, wchar_t* destWstr, const wchar_t* wstr, void* _)
{
    wchar_t subStr[subOffset->end - subOffset->start + 1];
    copySubStr(subStr, wstr, subOffset->start, subOffset->end);
    wcscat(destWstr, subStr);
}

static wchar_t* getCutedSubStr__(wchar_t* destWstr, const wchar_t* wstr, MyLinkedList subOffsetMyLinkedList)
{
    wcscpy(destWstr, L"");
    traverseMyLinkedList(subOffsetMyLinkedList, (void (*)(void*, void*, void*, void*))wcscatDestStr__,
        destWstr, (void*)wstr, NULL);
    return destWstr;
}

static wchar_t* getClearStr__(wchar_t* destWstr, const wchar_t* wstr)
{
    const char* error;
    int errorffset;
    wchar_t* regStr[] = {
        L"</?(?:div|font|img|strong|center|meta|dl|dt|table|tr|td|em|p|li|dir"
        "|html|head|body|title|a)[^>]*>",
        L"(?<=\\n)\\s+" //(?=\\n)
    };
    wcscpy(destWstr, L"");
    for (int i = 0; i < sizeof(regStr) / sizeof(regStr[0]); ++i) {
        void* reg = pcrewch_compile(regStr[i], 0, &error, &errorffset, NULL);
        MyLinkedList subOffsetMyLinkedList = getSubOffsetMyLinkedList(wstr, reg);
        wstr = getCutedSubStr__(destWstr, wstr, subOffsetMyLinkedList);

        printSubOffsetMyLinkedList__(fout, subOffsetMyLinkedList);

        delMyLinkedList(subOffsetMyLinkedList);
        pcrewch_free(reg);
    }

    return destWstr;
}

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

wchar_t* getWebWstr(const char* url)
{
    wchar_t* wstr = NULL;
    curl_global_init(CURL_GLOBAL_ALL);
    CURL* curl = curl_easy_init();

    static char str[SUPERWIDEWCHARSIZE * 200];
    strcpy(str, "");
    if (curl) {
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

void getEccoLibSrcFile(const char* fileName)
{
    char *url = "http://www.xqbase.com/ecco/ecco_%c.htm", url_x[WCHARSIZE];
    size_t size = SUPERWIDEWCHARSIZE;
    wchar_t* webwstr = malloc(size * sizeof(wchar_t));
    wcscpy(webwstr, L"");
    for (char c = 'a'; c <= 'e'; ++c) {
        sprintf(url_x, url, c);
        wchar_t* wstr = getWebWstr(url_x);
        supper_wcscat(&webwstr, &size, wstr);
        wcscpy(wstr, L"");
        free(wstr);
    }

    FILE* fout = openFile_utf8(fileName, "w");
    fwprintf(fout, webwstr);
    fclose(fout);
    free(webwstr);
}

void getCleanWebFile(const char* cleanFileName, const char* fileName)
{
    FILE *fin = openFile_utf8(fileName, "r"),
         *fout = openFile_utf8(cleanFileName, "w");
    wchar_t* fileWstring = getWString(fin);
    assert(fileWstring);

    wchar_t clearwstr[wcslen(fileWstring) + 1];
    getClearStr__(clearwstr, fileWstring);
    fwprintf(fout, clearwstr);

    free(fileWstring);
    fclose(fout);
    fclose(fin);
}

void html_test(void)
{
    char *webFileName = "ecco.htm",
         *txtFileName = "ecco.txt";
    //getEccoLibSrcFile(webFileName);

    getCleanWebFile(txtFileName, webFileName);
}
