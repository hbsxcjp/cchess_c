#include "head/htmlfile.h"
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

int html_test(void)
{
    curl_global_init(CURL_GLOBAL_ALL);
    CURL* curl = curl_easy_init();

    static char str[SUPERWIDEWCHARSIZE * 200];
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "http://www.xqbase.com/ecco/ecco_c.htm");
        //curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_str__);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, str);

        //curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_file__);
        //curl_easy_setopt(curl, CURLOPT_WRITEDATA, fout);

        curl_easy_perform(curl); //执行请求

        //*
        //printf(str);
        //wchar_t* wstr = malloc((strlen(str) + 1) * sizeof(wchar_t));
        wchar_t wstr[strlen(str) + 1];
#ifdef __linux
        gbk_mbstowcs_linux(wstr, str);
#else
        mbstowcs(wstr, str, mbstowcs(NULL, str, 0));
#endif
        fwprintf(fout, wstr);
        //free(wstr);
        //*/

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return 0;
}
