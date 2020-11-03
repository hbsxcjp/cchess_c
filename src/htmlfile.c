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
    fwprintf(fout, L"No:%d start:%d end:%d\n", (*pno)++, subOffset->start, subOffset->end);
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
    wchar_t* regStr[] = { L"</?(?:div|font|img|strong|center|meta|dl|dt|table|tr|td|em|p|li|dir"
                          "|html|head|body|title|a)[^>]*>" };
    wcscpy(destWstr, L"");
    for (int i = 0; i < sizeof(regStr) / sizeof(regStr[0]); ++i) {
        void* reg = pcrewch_compile(regStr[i], 0, &error, &errorffset, NULL);
        MyLinkedList subOffsetMyLinkedList = getSubOffsetMyLinkedList(wstr, reg);
        wstr = getCutedSubStr__(destWstr, wstr, subOffsetMyLinkedList);

        //printSubOffsetMyLinkedList__(fout, subOffsetMyLinkedList);
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

void getWebFile(const char* webFileName, const char* url)
{
    curl_global_init(CURL_GLOBAL_ALL);
    CURL* curl = curl_easy_init();

    static char str[SUPERWIDEWCHARSIZE * 200];
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        //curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_str__);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, str);

        //curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_file__);
        //curl_easy_setopt(curl, CURLOPT_WRITEDATA, fout);

        curl_easy_perform(curl); //执行请求

        wchar_t wstr[strlen(str) + 1];
        mbstowcs_gbk(wstr, str);

        FILE* fout = openFile(webFileName, "w", "UTF-8");
        fwprintf(fout, wstr);
        fclose(fout);

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
}

void getCleanWebFile(const char* cleanFileName, const char* fileName)
{
    FILE *fin = openFile(fileName, "r", "UTF-8"),
         *fout = openFile(fileName, "w", "UTF-8");
    wchar_t* fileWstring = getWString(fin);
    assert(fileWstring);

    wchar_t clearwstr[wcslen(fileWstring) + 1];
    getClearStr__(clearwstr, fileWstring);
    fwprintf(fout, clearwstr);

    free(fileWstring);
    fclose(fin);
    fclose(fout);
}

void html_test(void)
{
    char *webFileName = "ecco_c.htm",
         *txtFileName = "ecco_c.txt";
    getWebFile(webFileName, "http://www.xqbase.com/ecco/ecco_c.htm");

    //getCleanWebFile(txtFileName, webFileName);
}
