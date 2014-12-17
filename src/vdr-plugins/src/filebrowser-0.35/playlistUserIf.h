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

 
#ifndef P__PLAYLISTUSERIF_H
#define P__PLAYLISTUSERIF_H

#include "userIfBase.h"

//user interface for playlist menu (state pattern)

class cMenuPlayList;

//----------------------------------------------------------------
//-----------------cMenuPlaylistBaseIf----------------------------
//----------------------------------------------------------------

class cMenuPlayListBaseIf : public cUserIfBase
{
    public:
    virtual ~cMenuPlayListBaseIf(){}
    virtual eOSState RedKey(cMenuPlayList *menu){ return osContinue; }
    virtual eOSState GreenKey(cMenuPlayList *menu){ return osContinue; }
    virtual eOSState YellowKey(cMenuPlayList *menu){ return osContinue; }
    virtual eOSState BlueKey(cMenuPlayList *menu){ return osContinue; }
    virtual eOSState OkKey(cMenuPlayList *menu){ return osContinue; }
    virtual eOSState BackKey(cMenuPlayList *menu){ return osContinue; }
    virtual eOSState FastFwdKey(cMenuPlayList *menu){ return osContinue; }
    virtual eOSState FastRewKey(cMenuPlayList *menu){ return osContinue; }
    virtual eOSState AnyKey(cMenuPlayList *menu, eKeys Key);
    virtual void EnterState(cMenuPlayList *menu, cMenuPlayListBaseIf *lastIf = NULL);
    virtual void SetHelp(cMenuPlayList *menu);

    protected:
    cMenuPlayListBaseIf(){};
};

//----------------------------------------------------------------
//-----------------cMenuPlayListStandardIf------------------------
//----------------------------------------------------------------

class cMenuPlayListStandardIf : public cMenuPlayListBaseIf
{
    public:
    /*override*/ eOSState RedKey(cMenuPlayList *menu);
    /*override*/ eOSState GreenKey(cMenuPlayList *menu);
    /*override*/ eOSState YellowKey(cMenuPlayList *menu);
    /*override*/ eOSState BlueKey(cMenuPlayList *menu);
    /*override*/ eOSState OkKey(cMenuPlayList *menu);
    /*override*/ eOSState BackKey(cMenuPlayList *menu);
    /*override*/ eOSState FastFwdKey(cMenuPlayList *menu);
    /*override*/ eOSState FastRewKey(cMenuPlayList *menu);
    /*override*/ eOSState AnyKey(cMenuPlayList *menu, eKeys Key);
    /*override*/ void SetHelp(cMenuPlayList *menu);
    static cMenuPlayListStandardIf *Instance(){ return &instance; }

    protected:
    cMenuPlayListStandardIf(){};

    private:
    static cMenuPlayListStandardIf instance;
};

//----------------------------------------------------------------
//----------------cMenuPlayListOptionsIf--------------------------
//----------------------------------------------------------------

class cMenuPlayListOptionsIf : public cMenuPlayListBaseIf
{
    public:  
    /*override*/ eOSState OkKey(cMenuPlayList *menu);
    /*override*/ eOSState BackKey(cMenuPlayList *menu); 
    /*override*/ void EnterState(cMenuPlayList *menu, cMenuPlayListBaseIf *lastIf = NULL);
    static cMenuPlayListOptionsIf *Instance(){return &instance;}
 
    protected:
    cMenuPlayListOptionsIf(){}; 
    virtual void CreateOptions(cMenuPlayList *menu){};
    virtual void AddOptions(cMenuPlayList *menu);
    virtual eOSState LeaveStateDefault(cMenuPlayList *menu);
    virtual eOSState ExecuteOptions(cMenuPlayList *menu, std::string option){return osContinue;} 
    std::vector<std::string> options_;
    cMenuPlayListBaseIf *lastIf_;

    private:
    static cMenuPlayListOptionsIf instance;
};

//----------------------------------------------------------------
//-----------------cMenuPlayListCmdIf-----------------------------
//----------------------------------------------------------------

class cMenuPlayListCmdIf : public cMenuPlayListOptionsIf
{
    public:
    static cMenuPlayListCmdIf *Instance(){ return &instance; }

    protected: 
    /*override*/ void CreateOptions(cMenuPlayList *menu);
    /*override*/ eOSState ExecuteOptions(cMenuPlayList *menu, std::string option);
    cMenuPlayListCmdIf(){};
    eOSState DeleteEntries(cMenuPlayList *menu);
    eOSState ClosePlayList(cMenuPlayList *menu);
    eOSState SavePlayList(cMenuPlayList *menu);
    eOSState SavePlayListAs(cMenuPlayList *menu);
    eOSState CopyPlayListToCD(cMenuPlayList *menu);

    private:
    static cMenuPlayListCmdIf instance;
};

//----------------------------------------------------------------
//-----------------cMenuPlayListShutdownCmdIf---------------------
//----------------------------------------------------------------

class cMenuPlayListShutdownCmdIf : public cMenuPlayListCmdIf
{
    public:
    /*override*/ eOSState BackKey(cMenuPlayList *menu);
    static cMenuPlayListShutdownCmdIf *Instance(){ return &instance; }

    protected: 
    cMenuPlayListShutdownCmdIf(){};
    /*override*/ void CreateOptions(cMenuPlayList *menu);
    /*override*/ eOSState ExecuteOptions(cMenuPlayList *menu, std::string option);
    /*override*/ eOSState LeaveStateDefault(cMenuPlayList *menu);

    private:
    static cMenuPlayListShutdownCmdIf instance;
};

//----------------------------------------------------------------
//----------------cMenuBrowserRenameEntryIf-----------------------
//----------------------------------------------------------------

class cMenuPlayListRenamePlayListIf : public cMenuPlayListOptionsIf
{
    public:
    /*override*/ eOSState OkKey(cMenuPlayList *menu);
    static cMenuPlayListRenamePlayListIf *Instance(){return &instance;}
 
    protected:
    cMenuPlayListRenamePlayListIf(){name_[0] = 0;}
    /*override*/ void CreateOptions(cMenuPlayList *menu);
    /*override*/ void AddOptions(cMenuPlayList *menu);
    virtual eOSState BackKey(cMenuPlayList *menu);

    private: 
    bool RenamePlayList(cMenuPlayList *menu);
    static cMenuPlayListRenamePlayListIf instance;
    std::string oldname_;
    char name_[256];
};

#endif //PLAYLISTUSERIF_H
