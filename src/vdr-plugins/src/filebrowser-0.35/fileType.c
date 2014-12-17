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

#include <stdio.h>
#include <cstring>
#include <sstream>
#include <iostream>
#include <fstream>

#include <vdr/plugin.h>

/* taglib for id3 */
#include <tag.h>
#include <fileref.h>
#include <mpegfile.h>
#include <oggfile.h>
#include <vorbisfile.h>
#include <vorbisproperties.h>

/* for fstat() */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "menuBrowserBase.h"
#include "stringtools.h"
#include "fileType.h"
#include "convert.h"
#include "setup.h"

//----------cFileType-----------------------

cFileType::cFileType(efiletype ftype)
{
    GetFileType(ftype);
}

cFileType::cFileType(std::string path)
{
    GetFileType(path);
}

efiletype cFileType::GetType() const
{
    return type_->GetType();
}

efileclass cFileType::GetClass() const
{
    return type_->GetClass();
}

std::string cFileType::GetSymbol() const
{
    return type_->GetSymbol();
}

bool cFileType::IsOfSimilarType(cFileType filetype) const
{
    return type_->IsOfSimilarType(filetype);
}

bool cFileType::IsPlayable() const
{
    return type_->IsPlayable();
}

bool cFileType::IsXinePlayable() const
{
    return type_->IsXinePlayable();
}

bool cFileType::IsImage() const
{
    return type_->IsImage();
}

bool cFileType::HasInfo() const
{
    return type_->HasInfo();
}

std::vector<std::string> cFileType::GetFileInfo(std::string path) const
{
    return type_->GetFileInfo(path);
}

eOSState cFileType::Open(cPlayListItem &item, cMenuFileBrowserBase *menu, bool asPlaylist) const
{
    return type_->Open(item, menu, asPlaylist);
}

eOSState cFileType::Update(cPlayListItem &item, cMenuFileBrowserBase *menu, bool currentChanched) const
{
    return type_->Update(item, menu, currentChanched);
}

void cFileType::GetFileType(std::string path)
{ 
    //printf (" cFileType::GetFileType %s \n", path.c_str());
    type_ = &cFileTypeBase::Instance();

    if(cFileTypeBaseCUE::IsCUE(path))
        type_ = &cFileTypeBaseCUE::Instance();
    /*else if(cFileTypeBaseMPG::IsMPG(path))
        type_ = &cFileTypeBaseMPG::Instance();
    else if(cFileTypeBaseAVI::IsAVI(path))
        type_ = &cFileTypeBaseAVI::Instance();
    else if(cFileTypeBaseMP3::IsMP3(path))
        type_ = &cFileTypeBaseMP3::Instance();*/
    if(cFileTypeBasePlaylist::IsPlaylist(path))
        type_ = &cFileTypeBasePlaylist::Instance();
    if(cFileTypeBaseDebPacket::IsDebPacket(path))
        type_ = &cFileTypeBaseDebPacket::Instance();
    if(cFileTypeBasePoFile::IsPoFile(path))
        type_ = &cFileTypeBasePoFile::Instance();
    if(cFileTypeBaseUnspecifiedPicture::IsUnspecifiedPicture(path))
        type_ = &cFileTypeBaseUnspecifiedPicture::Instance();
    if(cFileTypeBaseUnspecifiedVideo::IsUnspecifiedVideo(path))
        type_ = &cFileTypeBaseUnspecifiedVideo::Instance();
    if(cFileTypeBaseUnspecifiedAudio::IsUnspecifiedAudio(path))
        type_ = &cFileTypeBaseUnspecifiedAudio::Instance();  
    if(cFileTypeBaseISO::IsISO(path))
        type_ = &cFileTypeBaseISO::Instance();
    if(cFileTypeBaseMP3::IsMP3(path))
       type_ = &cFileTypeBaseMP3::Instance();

#ifndef RBMINI
    if(cFileTypeBaseJPG::IsJPG(path))
        type_ = &cFileTypeBaseJPG::Instance();
    if(cFileTypeBasePNG::IsPNG(path))
        type_ = &cFileTypeBasePNG::Instance();
    if(cFileTypeBaseTIFF::IsTIFF(path))
        type_ = &cFileTypeBaseTIFF::Instance();
    if(cFileTypeBaseBMP::IsBMP(path))
        type_ = &cFileTypeBaseBMP::Instance();
    if(cFileTypeBaseGIF::IsGIF(path))
        type_ = &cFileTypeBaseGIF::Instance(); 
#endif
}

void cFileType::GetFileType(efiletype type)
{
    //printf (" cFileType::GetFileType %d \n", type);
#ifndef RBMINI
    if(type == tp_jpg)
        type_ = &cFileTypeBaseJPG::Instance();
    else if(type == tp_png)
        type_ = &cFileTypeBasePNG::Instance();
    else if(type == tp_gif)
        type_ = &cFileTypeBaseTIFF::Instance();
    else if(type == tp_bmp)
        type_ = &cFileTypeBaseBMP::Instance();
    else if(type == tp_gif)
        type_ = &cFileTypeBaseGIF::Instance();
    else
#endif
    if(type == tp_cue)
        type_ = &cFileTypeBaseCUE::Instance();
    /*else if(type == tp_mpg)
        type_ = &cFileTypeBaseMPG::Instance();
    else if(type == tp_avi)
        type_ = &cFileTypeBaseAVI::Instance();
    else if(type == tp_avi)
        type_ = &cFileTypeBaseMP3::Instance();*/
    else if(type == tp_mp3)
        type_= &cFileTypeBasePlaylist::Instance();    
    else if(type == tp_iso)
	     type_ = &cFileTypeBaseISO::Instance();
    else if(type == tp_deb)
        type_= &cFileTypeBaseDebPacket::Instance();
    else if(type == tp_po)
        type_= &cFileTypeBasePoFile::Instance();
    else if(type == tp_unspecifiedpicture)
        type_ = &cFileTypeBaseUnspecifiedPicture::Instance();
    else if(type == tp_unspecifiedvideo)
        type_ = &cFileTypeBaseUnspecifiedVideo::Instance();
    else if(type == tp_unspecifiedaudio)
        type_ = &cFileTypeBaseUnspecifiedAudio::Instance();
    else
        type_ = &cFileTypeBase::Instance();
}

//----------cFileTypeBase-----------------------

cFileTypeBase cFileTypeBase::instance;

bool cFileTypeBase::IsOfSimilarType(cFileType filetype) const
{
    return (fclass_ == filetype.GetClass());
}

eOSState cFileTypeBase::Update(cPlayListItem &item, cMenuFileBrowserBase *menu, bool currentChanged) const
{
    //printf("-------eOSState cFileTypeBase::UpdateFile-----------\n");
    menu->ShowThumbnail(item, true, currentChanged);
    menu->ShowID3TagInfo(item, true, currentChanged);
    return osContinue;
}

eOSState cFileTypeBase::Open(cPlayListItem &item, cMenuFileBrowserBase *menu, bool asPlaylist) const
{
    std::string message = tr("Cannot open file");
    Skins.Message(mtInfo,message.c_str());
    return osContinue;
}

//----------cFileTypeBaseXine---------------------------

eOSState cFileTypeBaseXine::Open(cPlayListItem &item, cMenuFileBrowserBase *menu, bool asPlaylist)  const
{
    return menu->ShowFileWithXine(item, asPlaylist);
}

//----------cFileTypeBaseAudio-------------------------

std::string  cFileTypeBaseAudio::GetSymbol() const
{
    return "\x8b\t";
}

//----------cFileTypeBasePicture-----------------------

eOSState cFileTypeBasePicture::Open(cPlayListItem &item, cMenuFileBrowserBase *menu, bool asPlaylist) const
{
    return menu->ShowFileWithImageViewer(item, asPlaylist);
}

eOSState cFileTypeBasePicture::Update(cPlayListItem &item, cMenuFileBrowserBase *menu, bool currentChanched) const
{
    printf("-------cFileTypeBasePicture::UpdateFile-----------\n");
    return menu->ShowThumbnail(item, false, currentChanched);
}

//----------cFileTypeBaseUnspecifiedPicture------------

cFileTypeBaseUnspecifiedPicture cFileTypeBaseUnspecifiedPicture::instance;

bool cFileTypeBaseUnspecifiedPicture::IsUnspecifiedPicture(std::string path)
{
    if(IsInsideStringChain(fileBrowserSetup.PictureFileTypes, GetEnding(path), false))
    {
        return true;
    }
    return false;
}

//----------cFileTypeBaseJPG-----------------------

cFileTypeBaseJPG cFileTypeBaseJPG::instance;

bool cFileTypeBaseJPG::IsJPG(std::string path)
{
    if(strcasecmp(GetEnding(path).c_str(), "jpg")
    && strcasecmp(GetEnding(path).c_str(), "jpeg"))
        return false;
    return true;
}

//----------cFileTypeBasePNG-----------------------

cFileTypeBasePNG cFileTypeBasePNG::instance;

bool cFileTypeBasePNG::IsPNG(std::string path)
{
    if(strcasecmp(GetEnding(path).c_str(), "png"))
        return false;
    return true;
} 

//----------cFileTypeBaseGIF-----------------------

cFileTypeBaseGIF cFileTypeBaseGIF::instance;

bool cFileTypeBaseGIF::IsGIF(std::string path)
{
   if(strcasecmp(GetEnding(path).c_str(), "gif"))
        return false;
   return true;
}

//----------cFileTypeBaseTIFF-----------------------

cFileTypeBaseTIFF cFileTypeBaseTIFF::instance;

bool cFileTypeBaseTIFF::IsTIFF(std::string path)
{
    if(strcasecmp(GetEnding(path).c_str(), "tiff"))
        return false;
    return true; 
}

//----------cFileTypeBaseBMP-----------------------

cFileTypeBaseBMP cFileTypeBaseBMP::instance;

bool cFileTypeBaseBMP::IsBMP(std::string path)
{
    if(strcasecmp(GetEnding(path).c_str(), "bmp"))
        return false;
    return true;
}

//----------cFileTypeBaseUnspecifiedVideo------------

cFileTypeBaseUnspecifiedVideo cFileTypeBaseUnspecifiedVideo::instance;

bool cFileTypeBaseUnspecifiedVideo::IsUnspecifiedVideo(std::string path)
{
    if(IsInsideStringChain(fileBrowserSetup.VideoFileTypes, GetEnding(path), false))
    {
        return true;
    }
    return false;
}

//----------cFileTypeBaseCUE-----------------------

cFileTypeBaseCUE cFileTypeBaseCUE::instance;

bool cFileTypeBaseCUE::IsCUE(std::string path)
{
    if(strcasecmp(GetEnding(path).c_str(), "cue"))
        return false;
    return true; 
}

/*eOSState cFileTypeBaseCUE::Open(cPlayListItem &item, cMenuFileBrowserBase *menu) const
{
    struct
    {
        char const *cuePath;
        cOsdMenu   *osdMenu;
    } binCueData = { 
                       item.GetPath().c_str(),
                       NULL 
                   };
    cPluginManager::CallAllServices("VCD Play BIN/CUE", &binCueData);
    menu->PlayerStarted();
    return osEnd;
}*/

//----------cFileTypeBaseMPG-----------------------

cFileTypeBaseMPG cFileTypeBaseMPG::instance;

bool cFileTypeBaseMPG::IsMPG(std::string path)
{ 
    if(strcasecmp(GetEnding(path).c_str(), "mpg")
    && strcasecmp(GetEnding(path).c_str(), "mpeg")
    && strcasecmp(GetEnding(path).c_str(), "rec")
    && strcasecmp(GetEnding(path).c_str(), "vob")
    && strcasecmp(GetEnding(path).c_str(), "vdr"))
        return false;
    return true;  
}

//----------cFileTypeBaseAVI-----------------------

cFileTypeBaseAVI cFileTypeBaseAVI::instance;

bool cFileTypeBaseAVI::IsAVI(std::string path)
{ 
    if(strcasecmp(GetEnding(path).c_str(), "avi")
    && strcasecmp(GetEnding(path).c_str(), "AVI"))
        return false;
    return true;  
}

//----------cFileTypeBaseUnspecifiedAudio------------

cFileTypeBaseUnspecifiedAudio cFileTypeBaseUnspecifiedAudio::instance;

bool cFileTypeBaseUnspecifiedAudio::IsUnspecifiedAudio(std::string path)
{
    if(IsInsideStringChain(fileBrowserSetup.AudioFileTypes, GetEnding(path), false))
    {
        return true;
    }
    return false;
}

//----------cFileTypeBaseMP3-----------------------

cFileTypeBaseMP3 cFileTypeBaseMP3::instance;

bool cFileTypeBaseMP3::IsMP3(std::string path)
{ 
    if(strcasecmp(GetEnding(path).c_str(), "mp3")
#ifndef RBMINI
    && strcasecmp(GetEnding(path).c_str(), "wav")
    && strcasecmp(GetEnding(path).c_str(), "ogg")
#endif
    )
        return false;
    return true;  
}

eOSState cFileTypeBaseMP3::Update(cPlayListItem &item, cMenuFileBrowserBase *menu, bool currentChanched) const
{
    return menu->ShowID3TagInfo(item, false, currentChanched);
}

#define MINCHARS 15

std::vector<std::string> cFileTypeBaseMP3::GetFileInfo(std::string path) const
{
    std::vector<std::string> fileInfo;
    std::string title, artist, album, genre, comment;

    TagLib::FileRef f( path.c_str() );
    if(!f.isNull() && f.tag()) {
        TagLib::Tag *tag = f.tag();
        title   = tag->title().stripWhiteSpace().toCString(false)  ; // unicode = false
        if(title.size()) {
            std::stringstream temp;
            temp << tr("Title") << ":\t";
            temp << title;
            fileInfo.push_back(temp.str());
        }
        artist  = tag->artist().stripWhiteSpace().toCString(false) ;
	if(artist.size()) {
            std::stringstream temp;
            temp << tr("Artist") << ":\t";
            temp << artist;
            fileInfo.push_back(temp.str());
        }
        album   = tag->album().stripWhiteSpace().toCString(false)  ;
        if(album.size()) {
            std::stringstream temp;
	    temp << tr("Album") << ":\t";
            temp << album;
            fileInfo.push_back(temp.str());
        }
        genre   = tag->genre().stripWhiteSpace().toCString(false)  ;
        if(genre.size()){
            std::stringstream temp;
	    temp << tr("Genre") << ":\t";
            temp << genre;
            fileInfo.push_back(temp.str());
        }
        comment   = tag->comment().stripWhiteSpace().toCString(false)  ;
        if(comment.size()) {
            std::stringstream temp;
	    temp << tr("Comment") << ":\t";
            temp << comment;
            fileInfo.push_back(temp.str());
        }
        if(tag->year()) {
            std::stringstream temp;
            temp << tr("Year") << ":\t";
            temp << tag->year();
	    fileInfo.push_back(temp.str());
        }
        if(tag->track()) {
            std::stringstream temp;
            temp << tr("Track") << ":\t";
            temp << tag->track();
	    fileInfo.push_back(temp.str());
        }
     }

    if(!f.isNull() && f.audioProperties())
    {
	TagLib::AudioProperties *props = f.audioProperties();
	if(props->length()) {
            int min = props->length()/60;
            int sec = props->length()%60;
            std::stringstream temp;
            temp << tr("Length") << ":\t";
            temp << min << ":";
            if(sec<=9)
                 temp << "0";
            temp << sec << " " << tr("min");
	    fileInfo.push_back(temp.str());
        }
	if(props->bitrate()) {
            std::stringstream temp;
            temp << tr("Bitrate") << ":\t";
            temp << props->bitrate() << " kb/s";
	    fileInfo.push_back(temp.str());
        }
	if(props->sampleRate()) {
            std::stringstream temp;
            temp << tr("Samplerate") << ":\t";
            temp << props->sampleRate() << " Hz";
	    fileInfo.push_back(temp.str());
        }
	if(props->channels()) {
            std::stringstream temp;
            temp << tr("ID3$Channels") << ":\t";
            temp << props->channels();
	    fileInfo.push_back(temp.str());
        }
	if(TagLib::MPEG::Properties* mpegProp = dynamic_cast<TagLib::MPEG::Properties*>(props)) {
#if 0
	    std::stringstream temp;
	    temp << tr("Original") << ":\t";
	    temp << mpegProp->isOriginal();
	    fileInfo.push_back(temp.str());
#endif

	    std::stringstream temp2;
	    temp2 << tr("MPEG-Layer") << ":\t";
	    temp2 << mpegProp->layer();
	    fileInfo.push_back(temp2.str());

#if 0
	    std::stringstream temp3;
	    temp3 << tr("Copyrighted") << ":\t";
	    temp3 << mpegProp->isCopyrighted();
	    fileInfo.push_back(temp3.str());
#endif

#if 0
	    std::stringstream temp4;
	    temp4 << tr("Protected") << ":\t";
	    temp4 << mpegProp->protectionEnabled();
	    fileInfo.push_back(temp4.str());
#endif
	} else if(TagLib::Vorbis::Properties* oggProp = dynamic_cast<TagLib::Vorbis::Properties*>(props)) {
	    std::stringstream temp;
	    temp << tr("Minimal bitrate") << ":\t";
	    temp << oggProp->bitrateMinimum();
	    fileInfo.push_back(temp.str());

	    std::stringstream temp2;
	    temp2 << tr("Maximal bitrate") << ":\t";
	    temp2 << oggProp->bitrateMaximum();
	    fileInfo.push_back(temp2.str());

	    std::stringstream temp3;
	    temp3 << tr("Nominal bitrate") << ":\t";
	    temp3 << oggProp->bitrateNominal();
	    fileInfo.push_back(temp3.str());
        }
    }

    struct stat stats;
    if(stat(path.c_str(), &stats) == 0) {
        std::stringstream temp;
        char buf[32];
        temp << tr("File size") << ":\t";
	if(stats.st_size < 1024) {
		temp << stats.st_size << " B";
	} else if (stats.st_size < 1024*1024) {
                sprintf(buf, "%0.1f", ((float)stats.st_size)/1024);
                temp << buf << " KB";
        } else {
                sprintf(buf, "%0.1f", ((float)stats.st_size)/(1024*1024));
		temp << buf << " MB";
        }
	fileInfo.push_back(temp.str());

	std::stringstream temp2;
        temp2 << tr("Last Modified") << ":\t";
        temp2 << ctime(&stats.st_mtime);
        std::string temp3 = temp2.str();
        /* elimitate '\n'-character */
        temp3[temp3.size()-1] = '\0';
	fileInfo.push_back(temp3.c_str());
    }

    return fileInfo;
}

//----------cFileTypeBasePlaylist-----------------------

cFileTypeBasePlaylist cFileTypeBasePlaylist::instance;

bool cFileTypeBasePlaylist::IsPlaylist(std::string path)
{ 
    if(strcasecmp(GetEnding(path).c_str(), "playlist")&&
       strcasecmp(GetEnding(path).c_str(), "m3u"))
        return false;
    return true;
}

eOSState cFileTypeBasePlaylist::Open(cPlayListItem &item, cMenuFileBrowserBase *menu, bool asPlaylist) const
{
    return menu->LoadAndEnterPlayList(item);
}

//----------cFileTypeBaseISO-----------------------

cFileTypeBaseISO cFileTypeBaseISO::instance;

bool cFileTypeBaseISO::IsISO(std::string path)
{ 
    if(strcasecmp(GetEnding(path).c_str(), "iso"))
        return false;
    return true;
}

std::string  cFileTypeBaseISO::GetSymbol() const
{
    return "\x81\t";
}

eOSState cFileTypeBaseISO::Open(cPlayListItem &item, cMenuFileBrowserBase *menu, bool asPlaylist) const
{
    return menu->ShowDvdWithXine(item, asPlaylist);
}

//----------cFileTypeBaseDebPacket-----------------------
cFileTypeBaseDebPacket cFileTypeBaseDebPacket::instance;

bool cFileTypeBaseDebPacket::IsDebPacket(std::string path)
{
#ifdef RBLITE
    if(GetLastN(path, 4) == ".ipk")
        return true;
#else
    if(GetLastN(path, 4) == ".deb")
        return true;
#endif
    return false;
}

eOSState cFileTypeBaseDebPacket::Open(cPlayListItem &item, cMenuFileBrowserBase *menu, bool asPlaylist) const
{
    /*std::string message;
    message = tr("please wait ...");
    Skins.Message(mtInfo,message.c_str());
#ifndef RBLITE
    std::string cmd = std::string("dpkg -i ") + item.GetPath();
#else
    std::string cmd = std::string("ipkg-cl install ") + item.GetPath();
#endif
    int ret = ::SystemExec(cmd.c_str());
    if(ret == 0)
    {
        message = tr("packet sucessfully installed");
    }
    else
    {
        message = tr("packet installation failed");
    }
    Skins.Message(mtInfo,message.c_str());*/
    
    return menu->InstallDebPacket(item.GetPath());
}

//----------cFileTypeBasePoFile--------------------------
cFileTypeBasePoFile cFileTypeBasePoFile::instance;

bool cFileTypeBasePoFile::IsPoFile(std::string path)
{
    if(GetLastN(path, 3) == ".po")
        return true;
    return false;
}

eOSState cFileTypeBasePoFile::Open(cPlayListItem &item, cMenuFileBrowserBase *menu, bool asPlaylist) const
{
    std::string message;
    message = tr("please wait ...");
    Skins.Message(mtInfo,message.c_str());

    std::string line;
    std::ifstream myfile (item.GetPath().c_str());
    char buf[128];
    bool foundVDR = false;
    bool foundPlugin = false;
    if (myfile.is_open()) {
      while (! myfile.eof() ) {
        getline (myfile,line);
        //std::cout << line << std::endl;
        if(sscanf(line.c_str(), "\"Project-Id-Version: vdr-%s\n\"", buf)==1) {
          if(buf[strlen(buf)-3] == '\\')
		buf[(strlen(buf)-3)] = '\0';
	  std::cout << "Found lang-file for plugin " << buf << std::endl;
          foundPlugin = true;
        } else if (sscanf(line.c_str(), "\"Project-Id-Version: VDR %s\n\"", buf)==1) {
          if(buf[strlen(buf)-3] == '\\')
		buf[(strlen(buf)-3)] = '\0';
          foundVDR = true;
	  std::cout << "Found lang-file for VDR version " << buf << std::endl;
        }
      }
      myfile.close();
    } else {
        std::cout << "Unable to open file" << std::endl; 
        return osContinue;
    }

    if(!foundVDR && !foundPlugin)
	return osContinue;

    std::string lang = GetLast(item.GetPath());
    lang.erase(lang.length()-3, lang.length()-1);
    std::cout << "language: " << lang << std::endl;

    std::string cmd = std::string("msgfmt -c -o /tmp/") + lang + std::string(".mo ") + item.GetPath();
    std::cout << "executing: " << cmd << std::endl;
    int ret = ::SystemExec(cmd.c_str());
    if(foundPlugin)
      cmd = std::string("cp /tmp/") + lang + std::string(".mo /usr/share/locale/") + lang + std::string("/LC_MESSAGES/vdr-") + std::string(buf) + std::string(".mo");
    else
      cmd = std::string("cp /tmp/") + lang + std::string(".mo /usr/share/locale/") + lang + std::string("/LC_MESSAGES/vdr.mo");
    std::cout << "executing: " << cmd << std::endl;
    ret += ::SystemExec(cmd.c_str());
    if(ret == 0)
    {
        message = tr("Language packet sucessfully installed");
    }
    else
    {
        message = tr("Language packet installation failed");
    }
    Skins.Message(mtInfo,message.c_str());
    return osContinue;
}

