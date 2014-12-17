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
 
#include "cover.h"
#include "filetools.h"
#include "../filebrowser/stringtools.h"

//id3tags : using TagLib
#include <fileref.h>
#include <tag.h>
#include <id3v2tag.h>
#include <id3v2frame.h>
#include <attachedpictureframe.h>
//#include <taglib_export.h>
#include "tfile.h"
#include "mpegproperties.h"
//#include "taglib_export.h"
#include <mpegfile.h>

namespace Cover
{
    cTempFileCache *cache;
    cFileCache *id3cache;
    cThumbConvert *thumbConvert;
  
    void GetImageInfoFromID3(std::string path, std::string &image, uint &size)
    {
      TagLib::MPEG::File f(path.c_str());
      if(f.ID3v2Tag())
      {
	  TagLib::ID3v2::FrameList l = f.ID3v2Tag()->frameListMap()["APIC"];
	  if(!l.isEmpty())
	  {
	      //TODO: look up other image formats
	      //std::string bla = l.front()->toString() ;
	      //std::string ending =
	      image = RemoveEnding(GetLast(path)) + "." + "jpg"; //ending;
	      size = l.front()->size();
	      return;
	  }
      }
      image = "";
      size = 0;
    }

    void ExtractImageFromID3(std::string path)
    {
      TagLib::MPEG::File f(path.c_str());
      if(f.ID3v2Tag())
      {
	  TagLib::ID3v2::FrameList l = f.ID3v2Tag()->frameListMap()["APIC"];
	  if(!l.isEmpty())
	  {
	      TagLib::ID3v2::AttachedPictureFrame *frame =  dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(l.front());
	      if(frame)
	      {
		  std::string name = RemoveEnding(GetLast(path)) + "." + "jpg"; //ending;
		  std::vector<unsigned char> buffer(frame->size());
		  memcpy(&buffer[0], frame->picture().data(), sizeof(unsigned char)*frame->size());
		  if(id3cache)
		  {
		      id3cache->PutInCache(name, buffer, "", true);
		  }
	      }
	  }
      }
    }

    bool IsImage(std::string name)
    {
      if(IsInsideStringChain("bmp jpeg jpg tif tiff png", GetEnding(name), false))
      {
	  return true;
      }
      return false;
    }

    std::string GetCoverFromDir(std::string mrl)
    {
      std::string dir = RemoveName(mrl);

      if(StartsWith(dir, "http:")) //internet radio
      {
	  return "";
      }

      std::vector<Filetools::dirEntryInfo> files;
      Filetools::GetDirFiles(dir, files);

      bool foundFirst = false;
      int first = 0;

      for(unsigned int i = 0; i < files.size(); ++i)
      {
	  if(IsImage(files[i].name_))
	  {
	      if(!foundFirst)
	      {
		  foundFirst = true;
		  first = i;
	      }

	      if(strcasecmp(RemoveEnding(GetLast(files[i].name_)).c_str(), "cover") == 0
	      || strcasecmp(RemoveEnding(GetLast(files[i].name_)).c_str(), "artist") == 0
	      || strcasecmp(RemoveEnding(GetLast(files[i].name_)).c_str(), "background") == 0
	      || strcasecmp(RemoveEnding(GetLast(files[i].name_)).c_str(), "front") == 0)
	      {
		  return files[i].name_;
	      }
	  }
      }

      if(foundFirst)
      {
	  return files[first].name_;
      }

      return "";
    }

    void ConvertCover(std::string path, std::string &image, uint &imagesize)
    {
	/*if(cache->InCache(const std::string &item, int filesize, std::string ending = "") )
	{
	    return; //image already in cache
	}
	std::string image = "";
	int imagesize = 0;*/

	//try to find an image in the ID3-tag
	if(strcasecmp(GetEnding(path).c_str(), "mp3") == 0 && cache && id3cache)
	{
	    GetImageInfoFromID3(path, image, imagesize);
	    if(image != "" && !cache->InCache(image, imagesize, thumbConvert->GetEnding()))
	    {
		if(!id3cache->InCache(image, "", true))
		{
		    ExtractImageFromID3(path);
		}
		else
		{
		    std::string file;
		    id3cache->FetchFromCache(image, file, "", true);
		    if(file != "")
		    {
			thumbConvert->InsertInConversionList(file, true);
			thumbConvert->StartConversion();
		    }
		}
	    }
	}

	//no image in I3D, look for appropriate image in directory
	if(image == "" && cache)
	{
	    image = GetCoverFromDir(path);
	    if(image != "")
	    {
		imagesize = Filetools::GetFileSize(image);
	    }
	    if (image != "" && !cache->InCache(image, imagesize, thumbConvert->GetEnding()))
	    {
		thumbConvert->InsertInConversionList(image, true);
		thumbConvert->StartConversion();
	    }
	}
    }
    
    std::string GetConvertedCover(std::string &image, uint imagesize)
    {
         std::string cover = ""; 
         cache->FetchFromCache(image, cover, imagesize, thumbConvert->GetEnding());
	 return cover;
    }  
    
} //namespace

