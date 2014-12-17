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


#ifndef P__BROWSERSTATUS_H
#define P__BROWSERSTATUS_H

#include <boost/shared_ptr.hpp>

#include <vdr/status.h>

#include "playlist.h"

class cBrowserBaseIf;


//----------cMp3BrowserStatus-------------------- 

class cMp3BrowserStatus: public cStatus
{
    //friend class cFileBrowserInstances;
public:
    cMp3BrowserStatus();
    ~cMp3BrowserStatus(){};
    // /*override*/ void Replaying(const cControl *Control, const char *Name, const char *FileName, bool On);

    cPlayList *GetPlayList() const;
    cBrowserBaseIf *GetLastIf() const;
    void SetLastIf(cBrowserBaseIf *lastIf);
    
private:
  
    boost::shared_ptr<cPlayList> playlist_;
    cBrowserBaseIf *lastIf_;
};

inline cPlayList *cMp3BrowserStatus::GetPlayList() const
{
    return playlist_.get();
}

inline cBrowserBaseIf *cMp3BrowserStatus::GetLastIf() const
{
    return lastIf_;
}

inline void cMp3BrowserStatus::SetLastIf(cBrowserBaseIf *lastIf)
{
    lastIf_ = lastIf;
} 


//----------cFileBrowserInstances--------------------

/*class cFileBrowserInstances
{
    public:
    std::map<int, cFileBrowserStatus> instances_;
    void Add(cFileBrowserStatus &status);
    void Del(cFileBrowserStatus &status); 
    void Del(int id); 
    void DeleteInvalid();
    cFileBrowserStatus &Get(int id);
    cFileBrowserStatus &Last();
    bool Find(int id);
    int GetLastMusicInst(); 
    int GetLastImagesInst();
    int GetLastMoviesInst();
    int GetLastFilter(uint filter);
    std::vector<int> GetInstanceVec();

    private:
    int id_;
};*/

#endif //P__BROWSERSTATUS_H

