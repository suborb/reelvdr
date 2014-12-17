#include <curl/curl.h>
#include <string.h>
#include <vdr/tools.h>

#define USERAGENTSTRING "libcurl"
#define DOWNLOAD_TIMEOUT 3

int isUrl(const char*url)
{
 return strstr(url,"http://")!=NULL || strstr(url,"https://")!=NULL;
}

static char errorStr[CURL_ERROR_SIZE];

int curl_download(const char *URL, const char *filename)
{
    CURL *curl;
    CURLcode res;
    int returnFlag = 1;

    curl = curl_easy_init ();
    if (curl)
    {
        FILE *fp = fopen (filename, "w"); // should overwrite file
        curl_easy_setopt (curl, CURLOPT_URL, URL);
        curl_easy_setopt (curl, CURLOPT_USERAGENT, USERAGENTSTRING);
        curl_easy_setopt (curl, CURLOPT_WRITEDATA, fp);

        curl_easy_setopt (curl, CURLOPT_TIMEOUT, DOWNLOAD_TIMEOUT);
        curl_easy_setopt (curl, CURLOPT_ERRORBUFFER, errorStr);
        res = curl_easy_perform (curl);
        if (res)
        {
            returnFlag = 0;
            printf("\033[0;91mCurl Error (timeout: %i secs): %s\033[0m\n",DOWNLOAD_TIMEOUT, errorStr);
            esyslog("Curl Error (timeout: %i secs): %s",DOWNLOAD_TIMEOUT, errorStr);
            printf("curl_download.c:%i Network Error\n", __LINE__);
        }

        fclose (fp);
    }
    /* always cleanup */
    curl_easy_cleanup (curl);
    esyslog("downloading %s to %s : success? %d ", URL, filename, returnFlag);
    return returnFlag;
}


