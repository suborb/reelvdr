#include "Playlist.h"
#include "stringtools.h"

cPlaylist CurrentPlaylist;

cPlaylist::cPlaylist()
{
    list_.clear();
    SetChanged();
}

void cPlaylist::Add(cCacheItem* item)
{
    if (item)
    {
        list_.push_back(item);
        SetChanged();

        item->SetInPlaylist();
    }
}

bool cPlaylist::Remove(unsigned int i)
{
    if ( i<0 || i>= Size()) return false;

    list_.erase( list_.begin() + i);
    SetChanged();
    return true;
}


bool cPlaylist::Remove(cCacheItem* item)
{
    if (!item) return false;

    unsigned int i = 0;

    for ( ; i < Size(); ++i)
        if (item == At(i)) 
        {
            item->SetNotInPlaylist();
            list_.erase( list_.begin() + i);
            SetChanged();
            return true;
        }// if

    return false;
}


bool cPlaylist::Remove(const std::string& fullpath)
{
    unsigned int i = 0;
    for ( ; i < Size(); ++i)
        if (fullpath == At(i)->FullPath()) 
        {
            At(i)->SetNotInPlaylist();
            list_.erase( list_.begin() + i);
            SetChanged();
            return true;
        }// if

    return false;
}


bool cPlaylist::RemoveDirTree(const std::string& fullpath)
{
    unsigned int i = 0;
    bool found = false;

    for ( ; i < Size(); ++i)
        if (StartsWith(At(i)->FullPath(), fullpath)) 
        {
            At(i)->SetNotInPlaylist();
            list_.erase( list_.begin() + i);
            SetChanged();
            found = true;
        }// if

    return found;
}


unsigned int cPlaylist::Size() const
{
    return list_.size();
}


int cPlaylist::Find(const cCacheItem* item)
{
    if (!item) return -1;

    int i = 0;
    for ( ; (unsigned int) i < Size(); ++i)
        if (item == At(i)) 
            return i;

    return -1;
}


cCacheItem* cPlaylist::At(unsigned int i)
{
    if( i >= Size() || i < 0) return NULL;
    printf("at\n");
    return list_.at(i);
}


void cPlaylist::Clear()
{
    unsigned int i = 0;
    for ( ; i < Size(); ++i)
        list_.at(i)->SetNotInPlaylist();

    list_.clear();
    SetChanged();
}

