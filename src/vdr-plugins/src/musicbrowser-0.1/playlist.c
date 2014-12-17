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

#include <map> 
 
#include "browserItems.h"
#include "../filebrowser/stringtools.h"

#include "playlist.h"
#include "browserItems.h"
#include "def.h"

#define MAX_PLAYLIST_SIZE 1000

//----------------itemId---------------------

itemId::itemId(std::string path, int num)
: path_(path), num_(num)
{
    st_ino_ = Filetools::GetInodeNr(path);
    size = Filetools::GetFileSize(path);
}

bool itemId::operator==(const itemId &item) const 
{
    return st_ino_ == item.st_ino_ && num_ == item.num_;
}

bool itemId::operator<(const itemId &item) const
{
    if (false)//st_ino_ == item.st_ino_)
    {
        return num_ < item.num_;
    }
    else
    {
        return st_ino_ < item.st_ino_;
    }
}

//----------------cPlayListItem---------------------

cPlayListItem::cPlayListItem(const cBrowserItem &item)
: id_(item.GetPath())
{
    //type_ = &(item.GetFileType());
}

cPlayListItem::cPlayListItem(std::string path, int num)
:id_(path, num)
{
}

cPlayListItem::cPlayListItem(itemId id)
:id_(id)
{

}

cPlayListItem::~cPlayListItem()
{
}

itemId cPlayListItem::GetId() const
{
    return id_;
}

std::string cPlayListItem::GetPath() const
{
    return id_.path_;
}

int cPlayListItem::GetNum() const
{
    return id_.num_;
} 

/*eOSState cPlayListItem::Open(cMenuFileBrowserBase *menu, bool asPlaylist)
{
    return type_.get()->Open(*this, menu, asPlaylist);
}

eOSState cPlayListItem::Update(cMenuFileBrowserBase *menu, bool currentChanged)
{
    return type_.get()->Update(*this, menu, currentChanged);
}

eOSState cPlayListItem::Play(cMenuFileBrowserBase *menu, bool asPlaylist)
{
    return type_.get()->Play(*this, menu, asPlaylist);
}*/


bool cPlayListItem::operator==(const cPlayListItem &item) const
{
    return id_ == item.GetId();
    //return GetPath() == item.GetPath();
}

bool cPlayListItem::operator<(const cPlayListItem &item) const
{
    return id_ < item.GetId();
}

//----------------cPlayList---------------------

cPlayList::cPlayList(playlistType type, std::string name, std:: string basedir)
: currentIsSet(false), type_(type), basedir_(basedir), name_(name == "" ? GetDefaultName(basedir) : name), 
  uniqueNum_(0), isDirty_(false), isNew_(true)
{
}

cPlayList::~cPlayList()
{
}

std::string cPlayList::GetDefaultName(std::string basedir)
{
    if(basedir == "")
    {
        return std::string(PLAYLIST_DEFAULT_PATH) + "/" +  PLAYLIST_DEFAULT_NAME +".playlist";
    }
    else
    {
        return basedir + "/playlists/" + PLAYLIST_DEFAULT_NAME +".playlist";
    }
} 

bool cPlayList::CurrentIsSet() const
{
    return currentIsSet;
}

bool cPlayList::IsInList(const cPlayListItem &item) const
{
    //cPlayListItem firstItem(item.GetPath());
    //printf("IsInList,item.GetPath() = %s\n", item.GetPath().c_str());
    //printf("IsInList,item.GetNum() = %d\n", item.GetNum());
    std::set<cPlayListItem>::iterator first =  set_.lower_bound(item);
    
    //if (first != set_.end()) 
    //  printf("IsInList,(*first).GetPath()  = %s\n", (*first).GetPath() .c_str());
    if (first == set_.end() || !(first->GetId() == item.GetId()))
    {
        return false;
    }
    return true;
}

bool cPlayList::Insert(const cPlayListItem &item, playlistMode mode)
{
    //printf("----------cPlayList::Insert,item.GetPath() = %s\n", item.GetPath().c_str());
    isDirty_ = true;
    if(list_.size() > MAX_PLAYLIST_SIZE)
    {
        return false;
    }
    if(mode == multipleUnique)
    {
        cPlayListItem uniqueItem = MakeUnique(item);
	//printf("----------cPlayList::Insert:  multipleUnique\n");
        list_.push_back(uniqueItem); 
        set_.insert(uniqueItem);
    }
    else if(mode == multiple)
    {
        //printf("----------cPlayList::Insert:  unique\n");
        list_.push_back(item);
        set_.insert(item);
    }
    else if(mode == single)
    {
        //printf("----------cPlayList::Insert:  single\n");
	if(IsInList(item))
	{
	    return true;
	}  
	list_.push_back(item);
        set_.insert(item);
    }  
    return true;
}

bool cPlayList::Insert(const cBrowserItem *item, playlistMode mode)
{
    isDirty_ = true;
    if(item->IsFileItem())
    {
        if(!Insert(item->GetPath(), mode))
        {
            return false;
        }
    }
    else if(item->IsFolderItem())
    {
        /*if(!InsertDirItemsRecursively(item->GetPath(), mode))
        {
            return false;
        }*/
    }    
    isDirty_ = true;
    return true;
}

bool cPlayList::Insert(const std::set<cBrowserItem*> &markedEntries, playlistMode mode)
{ 
    isDirty_ = true;
    for(std::set<cBrowserItem*>::iterator i = markedEntries.begin(); i != markedEntries.end(); ++i)
    {
        if((*i)->IsFileItem())
        {
            if(!Insert((*i)->GetPath(), mode))
            {
                return false;
            }
        }
        else if((*i)->IsFolderItem())
        {
            /*if(!InsertDirItemsRecursively((*i)->GetPath(), mode))
            {
                return false;
            }*/
        }
    }
    return true;
}

/*bool cPlayList::Insert(const std::set<itemId> &markedEntries, playlistMode mode)
{
    isDirty_ = true;
    for(std::set<itemId>::iterator i = markedEntries.begin(); i != markedEntries.end(); ++i)
    {
        if(i->IsFileItem())
        {
            if(!Insert((*i).path_, mode))
            {
                return false;
            }
        }
        else if(i->IsFolderItem())
        {
            if(!InsertDirItemsRecursively((*i).path_, mode))
            {
                return false;
            }
        }
    }
    return true;
}*/

void cPlayList::Remove(const cPlayListItem &item)
{
    isDirty_ = true;
 
    cPlayListItem currentItem;
    if(CurrentIsSet())
    {
   	currentItem = GetCurrent();
    }

    for(std::list<cPlayListItem>::iterator i = list_.begin(); i != list_.end(); ++i)
    {
         printf("cPlayList::Remove *i.GetPath() = %s\n", (*i).GetPath().c_str());
	 printf("(*i).id_.path = %s\n", (*i).id_.path_.c_str());
	 printf("(*i).id_.st_ino_ = %d\n", (*i).id_.st_ino_);
	 printf("(*i).id_.size = %lld\n", (*i).id_.size);
	 printf("(*i).id_.num_ = %d\n", (*i).id_.num_);
         if(item == *i)
         {
             list_.erase(i);
             currentIsSet = false;
             break;
         }
     }
     set_.erase(item);

     if(!(currentItem == cPlayListItem()))
     {
         SetCurrent(currentItem);
     }
}

void cPlayList::Remove(const cBrowserItem *item)
{
    if(item->IsFileItem())
    {
	Remove(item->GetPath());
    }
    else if(item->IsFolderItem())
    {
        ;//InsertDirItemsRecursively(item->GetPath(), multipleUnique, true);
    }  
}

void cPlayList::Append(const cPlayList& newList)
{
    std::vector<std::string> pathlist = newList.GetPathList();

    for (uint i = 0; i < pathlist.size(); ++i)
    {  
	Insert(pathlist[i]);
    }
}  

cPlayListItem cPlayList::GetCurrent() const
{
    return *current;
}

cPlayListItem cPlayList::GetNthItem(uint n) const
{
    if(n > list_.size() - 1)
        n = list_.size() - 1;
    if(n < 0)
        n = 0;

    std::list<cPlayListItem>::const_iterator it = list_.begin();
    uint i = 0;
    while(++i <= n)
        ++it;

    return *it;
}

cPlayListItem cPlayList::NextRandom()
{
    uint n = rand() % list_.size();
    cPlayListItem item = GetNthItem(n);
    if(item == *current)
        NextCyclic(true);    //avoid returning the current item
    else
        SetCurrent(item);
    return *current;
}

cPlayListItem cPlayList::NextCyclic(bool forward)
{
    if(forward)
    {
        current++;
        if (current == list_.end())
            current = list_.begin();
        return *current;
    }
    else
    {
        if (current == list_.begin())
            current = list_.end();
        current--;
        return *current;
    }
}

bool cPlayList::Next(bool forward)
{
    cPlayListItem item = *current;
    return Next(item, forward);
}

bool cPlayList::Next(cPlayListItem &item, bool forward)
{
    if(forward)
    {
        if (++current == list_.end())
        {
            current--;
            return false; 
        }
        else
        {
            item = *current;
            return true;
        }
    }
    else 
    {
        if (current == list_.begin())
        {
            return false; 
        }
        else
        {
            item = *(--current);
            return true;
        }
    }
} 

void cPlayList::ShiftItem(cPlayListItem item, bool up)
{
   std::list<cPlayListItem>::iterator curr = GetIterator(item);
   std::list<cPlayListItem>::iterator next = curr;
   if(!up)
   {
       if (curr == list_.begin())
       {
           next = --list_.end();
       }
       else
           next--;
   }
   else
   {
       if (curr == --list_.end())
       {
           next = list_.begin();
       }
       else
           next++;
   }
   std::swap(*curr, *next);
}

bool cPlayList::IsEmpty() const
{
    return list_.empty();
}

bool cPlayList::IsDirty() const
{
    return isDirty_;
}

int cPlayList::Size() const
{
    return list_.size();
}

void cPlayList::SetCurrent(const cPlayListItem &item, bool any)
{
    current = list_.begin();
    for(std::list<cPlayListItem>::iterator i = list_.begin(); i != list_.end(); ++i)
    {
        if(*i == item || (any && (*i).GetPath() == item.GetPath()))
        {
            current = i;
            currentIsSet = true; 
            break;
        }
    }
}

bool cPlayList::LoadFromFile(const cPlayListItem &item)
{
    Clear();
    std::string dir = RemoveName(item.GetPath());
    FILE *plFile = ::fopen64(item.GetPath().c_str(), "rb");
    if(plFile)
    {
        long long size = Filetools::GetFileSize(item.GetPath().c_str());
        std::vector<char> fileContent(size);
        fread(&(fileContent[0]), size, 1, plFile);
        uint linelength = 0;
        char *start = &fileContent[0];

        for (uint i = 0; i<size; i++)
        {
            if(fileContent[i] == '\n')
            {
                if(*start != '#') //no comment
                {
                    std::string path(start, linelength);
                    //adjust windows pathes
                    ReplaceAll(path, '\\', '/'); 
		    
		    if(!StartsWith(path, "/")) //item path is probably relative
		    {
			path = dir + "/" + path; 
		    }  
		    
                    Insert(cPlayListItem(path));
                }
                start = &fileContent[i+1];
                linelength = 0;
            }
            else
            {
                linelength++;
            }
        }
        fclose(plFile);
        name_ = item.GetPath();
        isDirty_ = false;
        isNew_ = false;
        return true;
    }
    return false;
}

bool cPlayList::SaveToFile(std::string fileName)
{
    FILE *plFile = ::fopen64(fileName.c_str(), "w");
    if(plFile)
    {
        for(std::list<cPlayListItem>::const_iterator i = list_.begin(); i != list_.end(); ++i)
        {
            fprintf(plFile, "%s\n", i->GetPath().c_str());
        }
        fclose(plFile);
        isDirty_ = false;
        return true;
    }
    return false;
}

std::string cPlayList::SaveToFile()
{
    //create standard directory if it does't exist
    if(!Filetools::DirExists(PLAYLIST_DEFAULT_PATH) && !Filetools::CreateDir(PLAYLIST_DEFAULT_PATH))
    {   
	return false;  
    }
    
#define CREATE_NEW_PLAYLIST_NAMES
#ifdef CREATE_NEW_PLAYLIST_NAMES
    //printf("-----cPlayList::SaveToFile, %s,  size = %d   isNew_ = %d-----\n", name_.c_str(), Size(), isNew_);
    if(isNew_ && name_ == GetDefaultName(basedir_))
    {
        int i = 0;
        char newName[64];
        std::string baseName = RemoveEnding(name_);
        strcpy(newName, name_.c_str());
        while(Filetools::FileExists(newName))
        {
            ::sprintf(newName, "%s%u%s", baseName.c_str(), ++i, ".playlist");
        }
        name_ = newName;
    }
#endif
    SaveToFile(name_);
    return name_;
}

void cPlayList::CopyPlayListToCD()
{ 
    SaveToFile();
    std::string cmd =  std::string("sudo mp3_2_media.sh ") + GetName();
    printf("-----CopyPlaylistToCD %s--------\n", cmd.c_str());
    ::SystemExec(cmd.c_str());
}

void cPlayList::Clear(std::string basedir)
{
    name_ = GetDefaultName(basedir);
    currentIsSet = false;
    list_.clear();
    set_.clear();
    isDirty_ = false;
    isNew_ = true;
}

std::string cPlayList::GetName() const
{
    return name_;
} 

int cPlayList::GetPos(const cPlayListItem &item) const
{
    int pos = 0;
    for(std::list<cPlayListItem>::const_iterator i = list_.begin(); i != list_.end(); ++i)
    {
        if(*i == item)
        {
            return pos;
        }
        ++pos;
    }
    return pos;
}

void cPlayList::SetName(std::string name)
{
    name_ = name;
}

std::vector<std::string> cPlayList::GetPathList() const
{
    std::vector<std::string> entries;
    for(std::list<cPlayListItem>::const_iterator i = list_.begin(); i != list_.end(); ++i)
    {
        entries.push_back(i->GetPath());
    } 
    return entries;
}

std::list<cPlayListItem>::iterator cPlayList::GetIterator(const cPlayListItem &item)
{
    for(std::list<cPlayListItem>::iterator i = list_.begin(); i != list_.end(); ++i)
    {
         if(item == *i)
         {
             return i;
         }
    }
    return list_.begin();
}

cPlayListItem cPlayList::MakeUnique(const cPlayListItem &item)
{
    cPlayListItem uniqueItem(item.GetPath(), ++uniqueNum_);
    //printf("--------cPlayList::MakeUnique, path = %s, num = %d-----\n", item.GetPath().c_str(), uniqueNum_);
    return uniqueItem;
}

/*bool cPlayList::InsertDirItemsRecursively(const std::string &dir, playlistMode mode, bool remove)
{
    DIR *dirp;
    struct dirent64 *dp; 
    struct stat64 buf;
    std::vector<std::string> dirEntries;
    std::vector<std::string> fileEntries;

    if ((dirp = opendir(dir.c_str())) == NULL) 
    {
        printf("couldn't open %s", dir.c_str());
        return true;
    }

    while((dp = readdir64(dirp)) != NULL)
    {
        std::string path = dir + "/" + dp->d_name;
        stat64(path.c_str(), &buf);
        if(S_ISDIR(buf.st_mode))
        {
            if(!(GetLast(path) == std::string(".")) &&
               !(GetLast(path) == std::string("..")))
                dirEntries.push_back(path);
        }
        if(S_ISREG(buf.st_mode))    
        {
            fileEntries.push_back(path);
        }
    }
    closedir(dirp);

    std::stable_sort(dirEntries.begin(), dirEntries.end());
    std::stable_sort(fileEntries.begin(), fileEntries.end());

    for(std::vector<std::string>::iterator i = dirEntries.begin(); i != dirEntries.end(); ++i)
    {
        if(!InsertDirItemsRecursively(*i, mode, remove))
        {
            return false;
        }
    }
    for(std::vector<std::string>::iterator i = fileEntries.begin(); i != fileEntries.end(); ++i)
    {
        if(!remove && !Insert(*i, mode))
        {
            return false;
        }
        if(remove)
	{
	    Remove(*i);
	}  
    }
    return true;
}*/

void cPlayList::SetPlayListEntries(std::vector<std::string> entries)
{
  Clear();
  for(uint i = 0; i < entries.size(); ++i)
  {
      //printf("--------------Insert entries[i].c_str() = %s\n", entries[i].c_str());
      Insert(cPlayListItem(itemId(entries[i])), multiple);
  }  
}  
