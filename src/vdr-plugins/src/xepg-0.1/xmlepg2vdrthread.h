
#ifndef __XMLEPG2VDRTHREAD_H_
#define __XMLEPG2VDRTHREAD_H_

/*
 * A thread is created that runs in the background collecting
 * xml-EPG data from websites and loading it into vdr schedules
 */


#include <vdr/thread.h>
#include <curl/curl.h>
#include <math.h>
#include <stdio.h>

#include <sys/types.h>
#include <dirent.h>

#include "channelData.h"

/* for libCURL
 * some websites demand email
 */

#define USERAGENTSTRING "reelbox reelbox-devel@mailings.reelbox.org"

//#define URLLISTFILENAME "/home/reel/tmp/url.list" 
#define URLLISTFILENAME "url.list" 
//list of urls and channel name
       //TODO put it in a place where it is not periodically deleted
       // eg. /var/config ?

#define TMP_PATH "/tmp/tmpfs/xepg"

/*
 * format of url.list
 *
 * CHANNEL NAME1; URL1
 * CHANNEL NAME2; URL2
 */



/********** download URL from network *********/
int
downloadURL (char *URL, char *filename)
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
    res = curl_easy_perform (curl);
    if (res)
    {
      returnFlag = 0;
    }

    fclose (fp);
  }
  /* always cleanup */
  curl_easy_cleanup (curl);

  return returnFlag;
}

/***************** URL FILE class********************/
class cURLFile
{

public:
  string ChannelName;
  string ChannelURL;
  string EpgFileName;           // seperate files for each channel 
  bool isError;
    cURLFile ()
  {
    isError = true;
  }

};                              // end class


/************** cThreadXmlepg2vdr Class *************/
class cThreadXmlepg2vdr:public cThread
{

private:
  bool isAlreadyRunning;

public:
  cThreadXmlepg2vdr ():cThread ("Xmlepg2vdr")
  {
    isAlreadyRunning = false;
  }

   ~cThreadXmlepg2vdr ()
  {
    Cancel (2);
  }

  void killThread (int sec)
  {
    Cancel (sec);
  }


  void Action ()
  {
    // this function runs in the background as a thread
    // cThread::Start() call this function

    if (isAlreadyRunning)
      return;
    isAlreadyRunning = true;

    Lock ();

    /* 
     * at times Action() was called twice, cause unknow. 
     * 
     * So, checks isAlreadyRunning and Lock() in place
     * */


    char url_str[512], line[1024];
    char channelName_str[64];
    char ch, tmp_EpgFileName[256];
    char urlListFileName[64] ; 
    sprintf(urlListFileName,"%s/%s", cPlugin::ConfigDirectory(PLUGIN_NAME_I18N), URLLISTFILENAME);  //TODO implement Name()
    FILE *urlFile;



            /********* open URL file ***************/
    //urlFile = fopen (URLLISTFILENAME, "r");
    urlFile = fopen (urlListFileName, "r");
    if (urlFile == NULL)
    {
      //TODO error msg
      esyslog("xepg: error opening %s. Aborting", urlListFileName);
      return;
    }

      esyslog("xepg: opened URLLISTFILE %s for reading.", urlListFileName);
    std::vector < cURLFile > Urls;
    cURLFile tmp_obj;

    int i = 0;
    /*
     * first get all the XML(EPG) files
     */
    esyslog ("xepg: downloading URLs from %s", urlListFileName);
    while (Running () && !feof (urlFile))
    {

      /* parsing URL file */
      fscanf (urlFile, "%[^\n] ", line);
      sscanf (line, "%[^;] %c %[^\n]", channelName_str, &ch, url_str); // 'ch' removes any trailing characters like ';'
      // TODO handle error in (parsing) URL.list
      tmp_obj.ChannelName = channelName_str;
      tmp_obj.ChannelURL = url_str;

      sprintf (tmp_EpgFileName, TMP_PATH"/epg%d", i);
      tmp_obj.EpgFileName = tmp_EpgFileName;

      DIR *tmpdir;
      
      if ((tmpdir=opendir(TMP_PATH)) == NULL)
      {
          isyslog("Creating %s\n", TMP_PATH);
          if (mkdir(TMP_PATH, ACCESSPERMS))
          {
              esyslog("Creation of %s failed.\n", TMP_PATH);
              isAlreadyRunning = false;
              return;
          }
      } 
      else
      {
          closedir(tmpdir);
      }


      /*get EPG from network */
      sleep (2);                // avoid overloading server
      // also some websites impose a 2 sec gap between fetches


      dsyslog ("xepg: getting URL for %s : %s", channelName_str, url_str);
      if (downloadURL (url_str, tmp_EpgFileName))
        tmp_obj.isError = false;
      else
      {
        tmp_obj.isError = true;
	esyslog("xepg: Error downloading URL %s",url_str);
      }

      Urls.push_back (tmp_obj);
      i++;
    }                           // end while: reading URL file and downloading URLs
    fclose (urlFile);
    isyslog ("xepg: EPG download from internet complete");

    /*
     * load xml files got earlier to Schedules
     */
    isyslog ("xepg: Begin loading EPG data");
    std::vector < cURLFile >::iterator iter = Urls.begin ();
    int prgCount;
    for (; iter != Urls.end (); iter++)
    {

      if (iter->isError)
        continue;               // donot load channel data if error in downloading URL

      /* load EPG data into schedule */
      cChannelData *channelData = new cChannelData (iter->EpgFileName.c_str ()); // TODO Check for errors

      // Reads EPGDATAFILE, Parses and adds EPG data to channel "channelName_str"
      prgCount = channelData->loadEpgData (iter->ChannelName.c_str ());
      if (prgCount<0) 
        {
	 esyslog("xepg: Error loading/parsing EPG data for channel %s",iter->ChannelName.c_str());
	 }
      else 
        { 
	 esyslog("xepg: Sucessfully loaded EPG for channel %s. Number of programmes = %d",iter->ChannelName.c_str() ,prgCount); 
        }

      delete channelData;
    }                           // for loop
    esyslog ("xepg: finished loading EPG data");
    Unlock ();
    isAlreadyRunning = false;

//TODO    if (ManualDownload)
    Skins.Message (mtInfo, tr("EPG data imported from Internet successfully"), 1);
  }                             // end Action()

};                              // end class

/***** END cThreadXmlepg2vdr class *****************/

#endif
