/***************************************************************************
 *   Copyright (C) 2005 by Reel Multimedia                                 *
 *                                                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <iostream>
#include <fstream>
#include <sstream>

#include "vdr/config.h"
#include "vdr/debug.h"
#include "vdr/plugin.h"

#include "convert.h"
#include "filecache.h"
#include "def.h"
#include "../filebrowser/stringtools.h"
#include "filetools.h"

/*----------------------------------------
constants fo screen parameters
-----------------------------------------*/
//TODO: sync with definitions in reelbox pl
int resolutions[] =
{
    1080,
    720,
    576,
    480
};

const char *aspects[] =
{
    "4:3",
    "16:9"
};

inline int StrToInt(const std::string& s)
{
   std::istringstream i(s);
   int x = -1;
   if (!(i >> x))
     ; // no error message
     //std::cerr << " error in strToInt(): " << s << std::endl;
   return x;
}

inline std::string IntToStr(int i)
{
   std::ostringstream s;
   s << i;
   return s.str();
}

/*******************************************************************
************************* cImageConvert ****************************
********************************************************************/

cImageConvert::~cImageConvert()
{
    if(status_ == stRun || status_ == stCopying)
    {
        Cancel(1000);
    }
}

/*-----------------------------------------
Starts conversion thread if not yet running
-------------------------------------------*/

void cImageConvert::StartConversion()
{
    Lock();
    if(status_ == stNone || status_ == stFin)
    {
        status_ = stRun;
        Start();
    }
    Unlock();
}

/*--------------------------------------------
Removes all items from the input list and
cancels the conversion thread
----------------------------------------------*/
void cImageConvert::CancelConversion()
{
   Lock();
   inFiles_.clear();
   Unlock();
   Cancel(1000);
}

std::string cImageConvert::GetFileTypeString(std::string file)
{
    std::string ending = GetEnding(file);
    if(strcasecmp(ending.c_str(), "jpg") == 0)
        return "jpg";
    else if(strcasecmp(ending.c_str(), "png") == 0)
        return "png";
    else if(strcasecmp(ending.c_str(), "tiff") == 0)
        return "tiff";
    else if(strcasecmp(ending.c_str(), "bmp") == 0)
        return "bmp";
    else if(strcasecmp(ending.c_str(), "gif") == 0)
        return "gif";
    else return "";
}

/*----------------------------------------------
Inserts file item at the end of the conversion list.
Multiple entrys of the same file are deleted
live = true: display file
-----------------------------------------------*/

void cImageConvert::InsertInConversionList(std::string item/*const cPlayListItem &item*/, bool live)
{
    Lock();
    if(live)
    {
       RemoveFromConversionList(item, false);
    }
    InListElement elem;
    elem.item = item;
    elem.live = live;
    inFiles_.push_back(elem);
    Unlock();
}

/*----------------------------------------------
Inserts a set of new items in the conversion list.
Multiple entries are deleted.
-----------------------------------------------*/

void cImageConvert::InsertInConversionList(const std::set<std::string> &newfiles)
{
    Lock();
    std::list<InListElement>::iterator it = inFiles_.begin();
    for(std::list<InListElement>::iterator it = inFiles_.begin(); it != inFiles_.end();)
    {
        if (newfiles.find(it->item) != newfiles.end())
        {
            it = inFiles_.erase(it);
        }
        else
            ++it;
    }
    InListElement elem;

    typedef std::set<std::string>::const_reverse_iterator setConstRevIter;
    for(setConstRevIter it = newfiles.rbegin(); it != newfiles.rend(); ++it)
    {
        elem.item = *it;
        elem.live = false;
        inFiles_.push_back(elem);
    }

    Unlock();
}

/*----------------------------------------------
Removes the specified file from the conversion list
"lock" = false avoids nested locks
-----------------------------------------------*/

void cImageConvert::RemoveFromConversionList(std::string item, bool lock)
{
    if(lock)
    {
        Lock();
    }

    for(std::list<InListElement>::iterator it = inFiles_.begin(); it != inFiles_.end(); it++)
        if (it->item == item)
        {
            inFiles_.erase(it);
            break;
        }

    if(lock)
    {
        Unlock();
    }
}

/*----------------------------------------------
Gets a copy of the specified conversion element,
removes element from outlist
-----------------------------------------------*/

bool cImageConvert::GetConvertedFile(std::string item, OutListElement& elem)
{
    Lock();
    for(std::list<OutListElement>::iterator it = outFiles_.begin(); it != outFiles_.end(); it++)
        if (it->item == item)
        {
            elem = outFiles_.back();
            outFiles_.pop_back();
            Unlock();
            return true;
        }
    Unlock();
    return  false;
}

/*----------------------------------------------
removes all elements from the outlist
-----------------------------------------------*/

void cImageConvert::ClearConvertedFiles()
{
     Lock();
     outFiles_.clear();
     Unlock();
}

/*----------------------------------------------
Waits until image data has been copied to temp file
and the image file can be deleted.
Make sure to remove all items from conversion list
prior to calling this function!
-----------------------------------------------*/

void cImageConvert::WaitUntilCopied()
{
   Lock();
   if(status_ == stCopying)
       cond.Wait();
   Unlock();
}

/*----------------------------------------------
//Returns true, if item is beeing converted, not synced
-----------------------------------------------*/

bool cImageConvert::IsInConversion(std::string item)
{
    return (currentFile_ == item);
}

/*----------------------------------------------
gets the next file from the input list,
registers/deregesters output list
-----------------------------------------------*/

bool cImageConvert::GetFromInputList()
{
    Lock();
    if (inFiles_.empty())
    {
        Unlock();
        return false;
    }
    currentFile_ = inFiles_.back().item;
    live_ = inFiles_.back().live;
    inFiles_.pop_back();
    status_ = stCopying;
    Unlock();
    return true;
}

/*----------------------------------------------
Inserts converted file data in the output list
You are not owner of the output list!
-----------------------------------------------*/

void cImageConvert::PutInOutputList(bool state)
{
    Lock();
    OutListElement elem;
    elem.item = currentFile_;
    elem.valid = state;
    if(elem.valid)
    {
        //copy converted image data
        elem.buffer.resize(buffer_.size());
        elem.buffer = buffer_;
    }
    outFiles_.push_back(elem);
    Unlock();
}

/*----------------------------------------------
Gets the actual screen parameters
----------------------------------------------*/

void cImageConvert::GetScreenParams(std::string &widthStr, std::string &heightStr, std::string &aspectStr)
{
    std::string name;
    std::string plugin;
    int height = 0;
    int width = 0;
    int aspect = 0;

    cPlugin* plug = cPluginManager::CallFirstService("IsOutputPlugin");
    if(plug) {
	int Width, Height;
	double Aspect;
	if(plug->Service("GetDisplayWidth",  &Width) && plug->Service("GetDisplayHeight", &Height) && plug->Service("GetDisplayAspect", &Aspect)) {
		widthStr = IntToStr(Width);
		heightStr = IntToStr(Height);
		if(Aspect>1.7)
			aspectStr = "16:9";
		else if (Aspect>1.3)
			aspectStr = "4:3";
		else
			aspectStr = "1:1";
		return;
	} // if
    } // if
    for (cSetupLine *sl = Setup.First(); sl; sl= Setup.Next(sl))
    {
        //printf ("setup Key %s:\t%s-%s  \n", sl->Name(), sl->Plugin(), sl->Value());
        if(sl->Name() && sl->Plugin() && sl->Value())
        {
            plugin = sl->Plugin();
            name = sl->Name();

            if(plugin == "reelbox" && name == "HDResolution")
            {
                height = resolutions[StrToInt(sl->Value())];
            }
            else if(plugin == "reelbox" && name == "HDDisplayType")
            {
                aspect = StrToInt(sl->Value());
            }
        }
    }

    if(aspect == 0)
    {
        width = height * 4 / 3;
    }
    else
    {
        width = height * 16 / 9;
    }

    widthStr = IntToStr(width);
    heightStr = IntToStr(height);
    aspectStr = aspects[aspect];
}

/*----------------------------------------------
does the conversion
----------------------------------------------*/

void cImageConvert::Action(void)
{
    std::string image, resImage, tmpImage, type, width, height, aspect;

    SetPriority(10);

    while (Running())
    {
        //get new file from input list
        if(!GetFromInputList())
        {
            //no more files in conversion list
            goto FIN;
        }

        image = currentFile_;
        type = GetFileTypeString(currentFile_);

        //copy img file into temporary file
        tmpImage = IMAGE_CACHE + GetLast(image);
        CopyFile(tmpImage, image);

        //Signalize that image file has been copied into temporary file
        Lock();
        status_ = stRun;
        cond.Signal();
        Unlock();

        //call conversion script
        GetScreenParams(width, height, aspect);
        resImage = IMAGE_CACHE + RemoveEnding(GetLast(image)) + ".mpg";
        int res = CallConversionScript(tmpImage, type, resImage, width, height, aspect);

        //read converted files into temp buffer and copy in cache
        if(res == 0)
        {
            res = ReadFileInBuffer(resImage);
            if(cache_ && res == 0)
            {
                Lock(); //sync????
                cache_->PutInCache(image, buffer_);
                Unlock();
            }
        }

        //insert converted image in the output list, if list exists
        PutInOutputList(res == 0);

        //remove temporary files
        ///TODO: test me
        unlink(tmpImage.c_str());
        //std::string cmd = std::string("nice -n 10 rm -f ") + "\"" + tmpImage + "\"";
        //printf("%s\n", cmd.c_str());
        //::SystemExec(cmd.c_str());

        // sleep a bit
        cCondWait::SleepMs(50);
    }

    FIN:
    Lock();
    status_ = stFin;
    Unlock();
}

int cImageConvert::CallConversionScript(std::string img, std::string type, std::string resImg,
                                        std::string width, std::string height, std::string aspect)
{
    //filebrowser: convert.c +416, CallConversionScript()]: image_convert.sh \
    //  "/tmp/browsercache/Gesicht Braun.jpg" "/tmp/browsercache/Gesicht Braun.mpg" "jpg" "0" "0" "4:3"
    //build cmd line for conversion script
    std:: string cmd = std::string(IMAGE_CONV) + " \"" + img + "\"" + " \"" + resImg + "\"" + " \"" + type + "\"";
    cmd += " \"" + width + "\"" + " \"" + height + "\"" + " \"" + aspect + "\"";

    DDD("%s", cmd.c_str());
    int r = SystemExec(cmd.c_str());
    if (r != 0)
    {
        printf("image_convert returned with code %d. Failed?\n",r);
    }
    //DDD("image_convert finished");
    return r;
}

int cImageConvert::ReadFileInBuffer(std::string img)
{
    FILE *imgFile = NULL;
    long long fileSize = Filetools::GetFileSize(img);
    int res = 1;
    imgFile = ::fopen64(img.c_str(), "rb");
    if(imgFile)
    {
        if (fileSize > 0)
        {
            buffer_.resize(fileSize);
            unsigned char* buf = &buffer_[0];
            int r = ::fread(buf, 1, fileSize, imgFile);
            if (r == fileSize)
            {
                res = 0;
            }
        }
        ::fclose(imgFile);
        unlink(img.c_str()); //??
    }
    return res;
}

int cImageConvert::CopyFile(std::string destFile, std::string srcFile)
{
    std::string cmd = "cp \"" + srcFile + "\"  \"" +  destFile + "\"";
    //printf("%s\n", cmd.c_str());
    return SystemExec(cmd.c_str());
}

/*******************************************************************
************************* cThumbConvert ****************************
********************************************************************/

/*----------------------------------------------
does the conversion
----------------------------------------------*/

cThumbConvert::cThumbConvert(cFileCache *cache)
: cImageConvert(cache), width_(150), height_(150), aspect_(0), ending_(".thumb.png")
{
}

void cThumbConvert::SetDimensions(int width, int height)
{
    width_ = width;
    height_ = height;
}

void cThumbConvert::SetDimensions(int width, int height, int aspect)
{
    width_ = width;
    height_ = height;
    aspect_ = aspect;
}

void cThumbConvert::SetEnding(std::string ending)
{
    ending_ = ending;
}

std::string cThumbConvert::GetEnding() const
{
    return ending_;
}

void cThumbConvert::Action()
{
    std::string image, resImage, tmpImage, type, width, height, aspect;

    SetPriority(10);

    while (Running())
    {
        //get new file from input list
        if(!GetFromInputList())
        {
            //no more files in conversion list
            goto FIN;
        }

        image = currentFile_;
        type = GetFileTypeString(currentFile_);

        //copy img file into temporary file
        tmpImage = std::string(IMAGE_CACHE) + "thumb_" + GetLast(image);
        CopyFile(tmpImage, image);

        //Signalize that image file has been copied into temporcPlayListItemary file
        Lock();
        status_ = stRun;
        cond.Signal();
        Unlock();

        //call conversion script
        std::stringstream width, height;
        width<<width_;
        height<<width_;
        resImage = IMAGE_CACHE + GetLast(image) + ending_;
        if (aspect_)
            aspect = "yes";
        int res = CallConversionScript(tmpImage, type, resImage, width.str(), height.str(), aspect);

        //read converted files into temp buffer and copy in cache
        if(res == 0)
        {
            res = ReadFileInBuffer(resImage);
            if(cache_ && res == 0)
            {
                Lock(); //sync????
                cache_->PutInCache(image, buffer_, ending_);
                Unlock();
            }
        }

        //remove temporary files
	unlink(tmpImage.c_str());
        //std::string cmd = std::string("nice -n 10 rm -f ") + "\"" + tmpImage + "\"";
        //printf("Executing \"%s\"\n", cmd.c_str());
        //::SystemExec(cmd.c_str());

        // sleep a bit
        cCondWait::SleepMs(50);
    }

    FIN:
    Lock();
    status_ = stFin;
    Unlock();
}

int cThumbConvert::CallConversionScript(std::string img, std::string type, std::string resImg,
                                        std::string width, std::string height, std::string aspect)
{
    //build cmd line for conversion script
    //char cmd[256];
    //sprintf(cmd, "anytopnm %s | pnmscale -xysize %d %d | pnmtopng > %s", img.c_str(), w, h, resImg.c_str());
    std:: string cmd;

    //if (aspect.size())
        cmd = std::string("anytopnm \"") + img + "\" | pnmscale -width" + " " + width + " -height " + height + " " +
        + " | pnmtopng > \"" + resImg + "\"";
    //else
    //    cmd = std::string("anytopnm \"") + img + "\" | pnmscale -xysize" + " " + width + "  " + height + " " +
    //    + " | pnmtopng > \"" + resImg + "\"";

    //printf("Executing: \"%s\"\n", cmd.c_str());
    int r = SystemExec(cmd.c_str());
    if (r != 0)
    {
        printf("anytopnm returned with code %d. Failed?\n",r);
    }
    //printf("anytopnm finished\n");
    return r;
}
