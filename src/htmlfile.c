#include "head/htmlfile.h"
#include "head/base.h"
#include "head/operatefile.h"
#include "head/pcre_wch.h"
#include <curl/curl.h>

extern FILE* test_out;

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
        //printf("\n%d: %s\n", __LINE__, url);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        //curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_str__);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, str);

        //curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_file__);
        //curl_easy_setopt(curl, CURLOPT_WRITEDATA, fout);

        curl_easy_perform(curl); //执行请求
        //assert(strlen(str) > 0);

        wstr = malloc((strlen(str) + 1) * sizeof(wchar_t));
        mbstowcs_gbk(wstr, str);
        //assert(wcslen(wstr) > 0);

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return wstr;
}