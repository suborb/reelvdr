#include <string>
#include <vector>
#include <sstream>

/* taglib for id3 */
#include <tag.h>
#include <fileref.h>
#include <mpegfile.h>

/* for fstat() */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "fileInfoMenu.h"

std::vector<std::string> GetFileInfo(std::string path)
{
	std::vector<std::string> fileInfo;
	std::string title, artist, album, genre, comment;

	TagLib::FileRef f( path.c_str() );
	if(!f.isNull() && f.tag())
	{
		TagLib::Tag *tag = f.tag();
								 // unicode = false
		title   = tag->title().stripWhiteSpace().toCString(false)  ;
		if(title.size())
		{
			std::stringstream temp;
			temp << tr("Title") << ":\t";
			temp << title;
			fileInfo.push_back(temp.str());
		}

		artist  = tag->artist().stripWhiteSpace().toCString(false) ;
		if(artist.size())
		{
			std::stringstream temp;
			temp << tr("Artist") << ":\t";
			temp << artist;
			fileInfo.push_back(temp.str());
		}

		album   = tag->album().stripWhiteSpace().toCString(false)  ;
		if(album.size())
		{
			std::stringstream temp;
			temp << tr("Album") << ":\t";
			temp << album;
			fileInfo.push_back(temp.str());
		}

		genre   = tag->genre().stripWhiteSpace().toCString(false)  ;
		if(genre.size())
		{
			std::stringstream temp;
			temp << tr("Genre") << ":\t";
			temp << genre;
			fileInfo.push_back(temp.str());
		}

		comment   = tag->comment().stripWhiteSpace().toCString(false)  ;
		if(comment.size())
		{
			std::stringstream temp;
			temp << tr("Comment") << ":\t";
			temp << comment;
			fileInfo.push_back(temp.str());
		}

		if(tag->year())
		{
			std::stringstream temp;
			temp << tr("Year") << ":\t";
			temp << tag->year();
			fileInfo.push_back(temp.str());
		}

		if(tag->track())
		{
			std::stringstream temp;
			temp << tr("Track") << ":\t";
			temp << tag->track();
			fileInfo.push_back(temp.str());
		}
	}

	if(!f.isNull() && f.audioProperties())
	{
		TagLib::AudioProperties *props = f.audioProperties();
		if(props->length())
		{
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

		if(props->bitrate())
		{
			std::stringstream temp;
			temp << tr("Bitrate") << ":\t";
			temp << props->bitrate() << " kb/s";
			fileInfo.push_back(temp.str());
		}

		if(props->sampleRate())
		{
			std::stringstream temp;
			temp << tr("Samplerate") << ":\t";
			temp << props->sampleRate() << " Hz";
			fileInfo.push_back(temp.str());
		}

		if(props->channels())
		{
			std::stringstream temp;
			temp << tr("ID3$Channels") << ":\t";
			temp << props->channels();
			fileInfo.push_back(temp.str());
		}

		if(TagLib::MPEG::Properties* mpegProp = dynamic_cast<TagLib::MPEG::Properties*>(props))
		{
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

			std::stringstream temp4;
			temp4 << tr("Protected") << ":\t";
			temp4 << mpegProp->protectionEnabled();
			fileInfo.push_back(temp4.str());
			#endif
		}
	}

	struct stat stats;
	if(stat(path.c_str(), &stats) == 0)
	{
		std::stringstream temp;
		char buf[32];
		temp << tr("File size") << ":\t";
		if(stats.st_size < 1024)
		{
			temp << stats.st_size << " B";
		}
		else if (stats.st_size < 1024*1024)
		{
			sprintf(buf, "%0.1f", ((float)stats.st_size)/1024);
			temp << buf << " KB";
		}
		else
		{
			sprintf(buf, "%0.1f", ((float)stats.st_size)/(1024*1024));
			temp << buf << " MB";
		}
		fileInfo.push_back(temp.str());

		std::stringstream temp2;
		temp2 << tr("Last Modified") << ":\t";
		temp2 << ctime(&stats.st_mtime);
                // remove '\n'
                std::string time_str = temp2.str();
                if (time_str.length()>0 && time_str.at(time_str.length()-1) == '\n')
                    time_str = time_str.substr(0,time_str.length()-1);
		fileInfo.push_back(time_str);
	}

	return fileInfo;
}


////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
cFileInfoMenu::cFileInfoMenu(std::string mrl) : cOsdMenu( tr("File Info:"), 12)
{
	//set title
	//SetTitle(tr("mediaplayer - Id3 Info:"));
	mrl_ = mrl;
	fileInfoVec_ = GetFileInfo(mrl_);

	// Get title
	std::string title;
	TagLib::FileRef f( mrl.c_str() );
	if(!f.isNull() && f.tag())
	{
		TagLib::Tag *tag = f.tag();
								 // unicode = false
		title   = tag->title().stripWhiteSpace().toCString(false)  ;

		if (title.size())
		{
			char buffer[128];
			snprintf(buffer, 127, "File Info: %s", title.c_str());
			SetTitle(buffer);
			printf("setting title to : %s\n", buffer);
		}
	}

	ShowInfo();
}


////////////////////////////////////////////////////////////////////////

cFileInfoMenu::~cFileInfoMenu()
{
	fileInfoVec_.clear();
}


////////////////////////////////////////////////////////////////////////

eOSState cFileInfoMenu::ProcessKey(eKeys Key)
{   
    eOSState state = cOsdMenu::ProcessKey(Key);

if (state == osUnknown)
    switch(Key)
    {
       case kBack:
        return osBack;

        default:
            break;
    }

    return state;
}
    


////////////////////////////////////////////////////////////////////////

void cFileInfoMenu::ShowInfo()
{
        Clear();
        SetHelp(NULL);
	for (unsigned int i=0; i<fileInfoVec_.size(); ++i)
		Add(new cOsdItem(fileInfoVec_.at(i).c_str(), osUnknown, false));

	if (fileInfoVec_.size() <= 0)
	{
		char buffer[128];
		Add( new cOsdItem ("", osUnknown,false));
		snprintf(buffer, 127,"%s", tr("No Id3 info found!"));
		Add( new cOsdItem (buffer, osUnknown,false));
	}

	Display();
}
