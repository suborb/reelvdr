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



#ifndef P__DIRTYPE_H
#define P__DIRTYPE_H

#include <vdr/osdbase.h>
#include <vector>

class cMenuFileBrowserBase;
class cFileTypeBase;
class cMenuDirItem;
class cDirTypeBase;
class cPlayListItem;

enum edirtype
{
    tp_dunknown,
    tp_vdr_rec,
    tp_dvd,
    tp_dvd_dir
};


//----------cDirEntryType-----------------------

class cDirEntryType 
{
public:
    cDirEntryType(){};
    ~cDirEntryType(){};
    virtual eOSState Open(cPlayListItem &item, cMenuFileBrowserBase *menu, bool asPlaylist = false) const {return osContinue;} 
    virtual eOSState Play(cPlayListItem &item, cMenuFileBrowserBase *menu, bool asPlaylist = false) const {return osContinue;}
    virtual eOSState Update(cPlayListItem &item, cMenuFileBrowserBase *menu, bool currentChanged) const {return osContinue;}
    virtual bool IsPlayable() const {return false;}
    virtual bool IsXinePlayable() const {return false;}
    virtual bool IsImage() const  {return false;}
};


//----------cDirType-----------------------

class cDirType : public cDirEntryType
{
public:
    cDirType(edirtype dtype);
    cDirType(std::string path);
    ~cDirType(){};
    edirtype GetType() const;
    std::string GetSymbol() const;
    /*override*/ eOSState Open(cPlayListItem &item, cMenuFileBrowserBase *menu, bool asPlaylist = false) const; 
    /*override*/ eOSState Play(cPlayListItem &item, cMenuFileBrowserBase *menu, bool asPlaylist = false) const;
    /*override*/ eOSState Update(cPlayListItem &item, cMenuFileBrowserBase *menu, bool currentChanged) const;
    /*override*/ bool IsPlayable() const;
    /*override*/ bool IsXinePlayable() const;
    /*override*/ bool IsImage() const;
    bool operator == (cDirType type) const
    {
        return type_ == type.type_;
    } 

private:
    void GetDirType(std::string path);
    void GetDirType(edirtype dtype);
    cDirTypeBase *type_;
};

//----------cDirTypeBase-----------------------

class cDirTypeBase 
{
protected:
    cDirTypeBase(edirtype dtype = tp_dunknown)
    :dtype_(dtype) {}; 
    static cDirTypeBase instance;
public:
    static cDirTypeBase &Instance(){return instance;}
    virtual ~cDirTypeBase(){};
    edirtype GetType() const {return dtype_;}
    virtual std::string GetSymbol() const;
    virtual eOSState Open(cPlayListItem &item, cMenuFileBrowserBase *menu, bool asPlaylist = false) const;
    virtual eOSState Play(cPlayListItem &item, cMenuFileBrowserBase *menu, bool asPlaylist = false) const;
    virtual eOSState Update(cPlayListItem &item, cMenuFileBrowserBase *menu, bool currentChanged = false) const;
    virtual bool IsPlayable() const;
    virtual bool IsXinePlayable() const;
    virtual bool IsImage() const;

protected:
    edirtype dtype_;
};

//----------cDirTypeBaseVdrRec---------------------------

class cDirTypeBaseVdrRec : public cDirTypeBase
{
protected:
    cDirTypeBaseVdrRec()
    :cDirTypeBase(tp_vdr_rec){};
    static cDirTypeBaseVdrRec instance;
public:
    static cDirTypeBaseVdrRec &Instance(){return instance;}
    /*override*/ eOSState Open(cPlayListItem &item, cMenuFileBrowserBase *menu, bool asPlaylist = false) const; 
    /*override*/ eOSState Play(cPlayListItem &item, cMenuFileBrowserBase *menu, bool asPlaylist = false) const;
    /*override*/ bool IsPlayable() const;
    static bool IsVdrRec(std::string path);
};

//----------cDirTypeBaseDvd---------------------------

//should not be used
class cDirTypeBaseDvd : public cDirTypeBase
{
protected:
    cDirTypeBaseDvd()
    :cDirTypeBase(tp_dvd){};
    static cDirTypeBaseDvd instance;
public:
    static cDirTypeBaseDvd &Instance(){return instance;}
    /*override*/ eOSState Open(cPlayListItem &item, cMenuFileBrowserBase *menu, bool asPlaylist = false) const; 
    /*override*/ eOSState Play(cPlayListItem &item, cMenuFileBrowserBase *menu, bool asPlaylist = false) const;
    /*override*/ bool IsPlayable() const;
    /*override*/ bool IsXinePlayable() const;
    static bool IsDvd(std::string path);
};

//----------cDirTypeBaseDvdDir------------------------

class cDirTypeBaseDvdDir : public cDirTypeBase
{
protected:
    cDirTypeBaseDvdDir()
    :cDirTypeBase(tp_dvd_dir){};
    static cDirTypeBaseDvdDir instance;
public:
    static cDirTypeBaseDvdDir &Instance(){return instance;}
    /*override*/ std::string GetSymbol() const;
    /*override*/ eOSState Open(cPlayListItem &item, cMenuFileBrowserBase *menu, bool asPlaylist = false) const;
    /*override*/ eOSState Play(cPlayListItem &item, cMenuFileBrowserBase *menu, bool asPlaylist = false) const;
    /*override*/ bool IsPlayable() const;
    static bool IsDvdDir(std::string path);
};

#endif //P__DIRTYPE_H
