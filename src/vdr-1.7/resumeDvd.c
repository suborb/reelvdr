#ifdef REELVDR
// resume DB

#include "resumeDvd.h"
#include "tools.h"
#include <stdio.h>
#include <unistd.h>

const time_t cResumeInfo::ExpireTime = 604800; //7*24*3600; // 1 week in seconds

/// class cResumeInfo -----------------------------------------------------------------
cResumeInfo::cResumeInfo()
{
	Init();
}
		
cResumeInfo::cResumeInfo(std::string line)
{
	if (!FromString(line))
		Init();
}
		
void cResumeInfo::Init()
{
	lastSeen = time(NULL); // now
	titleChapter.clear();
}
		
std::string cResumeInfo::ToString()
{
	std::stringstream os;
	os <<titleChapter<<" "<<lastSeen;
			
	return os.str();
}
		
bool cResumeInfo::FromString(std::string line)
{
			//1.33 1236347970
	size_t idx = line.find(" ");
	if (idx == std::string::npos) return false;
			
	titleChapter = line.substr(0,idx);
			
			// parse for time
	line = line.substr(idx+1);
	if (line.length()>0)
		lastSeen = atol(line.c_str());
	else
		lastSeen = time(NULL);
	return true;
}
		
bool cResumeInfo::Expired()
{
	return time(NULL) > (lastSeen + ExpireTime);
}


/// class cResumeDvd --------------------------------------------------------------------
cResumeDvd::cResumeDvd (std::string fileName)
{
    DvdList_.clear ();
    currDvdId_.clear ();
    fileName_ = fileName;
    //Read ();
}

cResumeDvd::~cResumeDvd ()
{
    DvdList_.clear ();
}

//makes no entry in the list

bool cResumeDvd::SetCurrentDvd (std::string dvdId)
{
    currDvdId_ = dvdId;
    return DvdList_.find (dvdId) != DvdList_.end (); // is it present in the list ?
}

// sets title chapter of current DVD

bool cResumeDvd::SetTitleChapter (std::string titleChapter)
{
    if (currDvdId_.length () > 0)
    {
            DvdList_[currDvdId_].titleChapter = titleChapter;
	    DvdList_[currDvdId_].lastSeen = time(NULL);
            Save ();
            return true;
    }

    return false;
}

// get title and chapter of current DVD

std::string cResumeDvd::TitleChapter ()
{
    return DvdList_[currDvdId_].titleChapter;
}

bool cResumeDvd::Read ()
{
    // discard current list and reload from file
    // loose all unsave information
    esyslog(*cString::sprintf("'%s': reading '%s'", __PRETTY_FUNCTION__, fileName_.c_str()));
    FILE *fp = fopen (fileName_.c_str (), "r");
    if (!fp) return false;
    // clear list
    DvdList_.clear ();
    char* line = NULL;
    cReadLine ReadLine;
    char *p = NULL;

    while ((line = ReadLine.Read (fp)) != NULL)
    {
            p = strchr (line, '=');
            if (p)
            {
                    *p = 0;
		    cResumeInfo resumeInfo(std::string (p + 1 ? p + 1 : ""));
		    if (!resumeInfo.Expired())
		    {
		    	DvdList_[line] = resumeInfo;
                    	printf ("'%s' '%s'\n", line, p + 1 ? p + 1 : "");
		    } 
		    else
			esyslog(*cString::sprintf("'%s':Expired : '%s' '%s'\n", __PRETTY_FUNCTION__, line, p + 1 ? p + 1 : ""));
            }
    }


    fclose (fp);
    return true;
}

bool cResumeDvd::Save ()
{
    //(over)write list to a file
    FILE *fp = fopen (fileName_.c_str (), "w");
    if (!fp) return false;

    std::map<std::string, cResumeInfo>::iterator it;
    for (it = DvdList_.begin (); it != DvdList_.end (); it++)
	    if (!it->second.Expired()) // save only if resume data has not expired; ie. 1 week
            	fprintf (fp, "%s=%s\n",
                	     it->first.c_str (), it->second.ToString().c_str());
    fclose (fp);
    return true;
}

cResumeDvd ResumeDvd ("/etc/vdr/plugins/resumeDVD.list");



// Get DVD Id of the dvd in /dev/dvd
// dvd id is made by "dvdname+dvdserialnumber"
#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#ifndef DVD_VIDEO_LB_LEN
#define DVD_VIDEO_LB_LEN 2048
#endif

#define MSG_OUT stdout

// taken from function of the same name in 
// xine-lib-1.1.8/src/input/libdvdnav/vm.c

int dvd_read_name (char *name, const char *device)
{
    /* Because we are compiling with _FILE_OFFSET_BITS=64
     * all off_t are 64bit.
     */
    off_t off;
    int fd, i, idx = 0;
    uint8_t data[DVD_VIDEO_LB_LEN];

    /* Read DVD name */
    fd = open (device, O_RDONLY);
    if (fd > 0)
    {
            off = lseek (fd, 32 * (off_t) DVD_VIDEO_LB_LEN, SEEK_SET);
            if (off == (32 * (off_t) DVD_VIDEO_LB_LEN))
            {
                    off = read (fd, data, DVD_VIDEO_LB_LEN);
                    close (fd);
                    if (off == ((off_t) DVD_VIDEO_LB_LEN))
                    {

                            /*title*/
                            for (i = 25; i < 73; i++)
                            {
                                    if ((data[i] == 0)) break;
                                    name[idx++] = ((data[i] > 32) && (data[i] < 127)) ? data[i] : ' ';
                            }


                            /*serial number*/
                            for (i = 73; i < 89; i++)
                            {
                                    if ((data[i] == 0)) break;
                                    name[idx++] = ((data[i] > 32) && (data[i] < 127)) ? data[i] : ' ';
                            }

                            /* Alternate title
                            for(i=89; i < 128; i++ )
                                 {
                              if((data[i] == 0)) break;
                              name[idx++] = ((data[i] > 32) && (data[i] < 127))?data[i]:' ';
                                }
                             */

                    }
                    else
                    {
                            fprintf (MSG_OUT, "libdvdnav: Can't read name block. Probably not a DVD-ROM device.\n");
                            return 0;
                    }
            }
            else
            {
                    fprintf (MSG_OUT, "libdvdnav: Can't seek to block %u\n", 32);
                    return 0;
            }
            close (fd);
    }
    else
    {
            fprintf (MSG_OUT, "Opening dvd device FAILED\n");
            return 0;
    }
    name[idx] = 0;
    return 1;
}

std::string DvdId ()
{
    std::string id;
    char *name = (char*) malloc (128 * sizeof (char));
    if (dvd_read_name (name, "/dev/dvd"))
    {
            id = name;
            //PRINTF ("\033[1:92m DVD id: '%s'\033[0m\n", name);
    }
    else id.clear ();

    free (name);
    return id;
}

#endif /*REELVDR*/

