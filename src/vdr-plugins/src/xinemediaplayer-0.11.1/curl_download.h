#ifndef CURL_DOWNLOAD__H
#define CURL_DOWNLOAD__H

/*
 * checks if the current string contains 
 * either "http://" or "https://"
 * TODO: check if it begins with these sub-strings
*/
int isUrl(const char*url);

/*
 * downloads "URL" into "filename" 
 * CAUTION: overwrites "filename" !!
 *
 * needs : curl-config --libs
 */
int curl_download(const char *URL, const char *filename);

#endif

