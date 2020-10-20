//*
#include <curl/curl.h>
#include <stdio.h>
#include <string.h>

CURL* curl;
CURLcode res;

size_t write_data(void* ptr, size_t size, size_t nmemb, void* stream)
{
    if (strlen((char*)stream) + strlen((char*)ptr) > 999999)
        return 0;
    strcat(stream, (char*)ptr);
    return size * nmemb;
}

char* down_file(char* filename)
{
    static char str[10000000];
    strcpy(str, "");
    //return “<a href=/”http://gtk.awaysoft.com//”>”;
    curl_easy_setopt(curl, CURLOPT_URL, filename); //设置下载地址
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3); //设置超时时间
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data); //设置写数据的函数
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, str); //设置写数据的变量
    res = curl_easy_perform(curl); //执行下载
    str[9999999] = '0';
    if (CURLE_OK != res)
        return NULL; //判断是否下载成功
    return str;
}

int main()
{
    char url[200];
    curl = curl_easy_init(); //对curl进行初始化
    char* result;
    while (fgets(url, 200, stdin)) {
        result = down_file(url);
        if (result)
            puts(result);
        else
            puts("Get Error !");
        printf("\nPlease Input a url:");
    }
    curl_easy_cleanup(curl); //释放curl资源

    return 0;
}
//*/

/*
#include <curl/curl.h>
#include <stdio.h>
#include <string.h>

//输出到字符串再打印到屏幕上
ssize_t write_data_s(void* ptr, size_t size, size_t nmemb, void* stream)
{
    strcat(stream, (char*)ptr);
    puts(stream);
    return size * nmemb;
}

//输出到文件
ssize_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream)
{
    size_t written;
    written = fwrite(ptr, size, nmemb, stream);
    return written;
}

int main(int argc, char* argv[])
{
    CURL* curl2;
    CURLcode res2;
    //FILE *fp2;
    //struct curl_slist *list=NULL;
    //list = curl_slist_append(list, argv[1]);//这个其实是-H但是这边没用到所以注释
    //list = curl_slist_append(list, argv[2]);//有几个-H头就append几次
    static char str[20480];
    res2 = curl_global_init(CURL_GLOBAL_ALL);
    curl2 = curl_easy_init();
    if (curl2) {
        //fp2=fopen("UsefullInfo.json", "w+");
        //curl_easy_setopt(curl2, CURLOPT_URL, "https://192.168.112.3:8006/api2/json/access/ticket"); //这是请求的url
        curl_easy_setopt(curl2, CURLOPT_URL, "http://www.xqbase.com/ecco/ecco_c.htm"); //这是请求的url
        //curl_easy_setopt(curl2, CURLOPT_POSTFIELDS, "username=root@pam&password=nicaiba_88"); //这是post的内容
        //curl_easy_setopt(curl2, CURLOPT_HTTPHEADER, list);//若需要带头，则写进list
        //curl_easy_setopt(curl2, CURLOPT_SSL_VERIFYPEER, 0); //-k
        //curl_easy_setopt(curl2, CURLOPT_SSL_VERIFYHOST, 0); //-k
        //curl_easy_setopt(curl2, CURLOPT_VERBOSE, 1); //这是请求过程的调试log
        curl_easy_setopt(curl2, CURLOPT_WRITEFUNCTION, write_data); //数据请求到以后的回调函数
        curl_easy_setopt(curl2, CURLOPT_WRITEDATA, str); //选择输出到字符串
        //curl_easy_setopt(curl2, CURLOPT_WRITEDATA, fp2);//选择输出到文件
        res2 = curl_easy_perform(curl2); //这里是执行请求

        printf(str);
        curl_easy_cleanup(curl2);
        //fclose(fp2);
    }
    curl_global_cleanup();
    return 0;
}
//*/