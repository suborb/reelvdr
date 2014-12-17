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

#ifndef P__USERIF_H
#define P__USERIF_H

#include "menuBrowser.h"
#include "userIfBase.h"

//warning: non blocking bkg actions may interfere, yet no compliance check implemented!
//#define BLOCK_DURING_BKG_ACTIVITY

#define APPLY_COMMANDS_ONLY_TO_MARKED_ENTRIES

//user interface for browser menu (state pattern)

//----------------------------------------------------------------
//-----------------cMenuBrowserBaseIf-----------------------------
//----------------------------------------------------------------

class cMenuBrowserBaseIf: public cUserIfBase
{
    public:
    virtual ~cMenuBrowserBaseIf(){}
    virtual eOSState RedKey(cMenuFileBrowser *menu){ return osContinue; }
    virtual eOSState GreenKey(cMenuFileBrowser *menu){ return osContinue; }
    virtual eOSState YellowKey(cMenuFileBrowser *menu){ return osContinue; }
    virtual eOSState BlueKey(cMenuFileBrowser *menu){ return osContinue; }
    virtual eOSState OkKey(cMenuFileBrowser *menu){ return osContinue; }
    virtual eOSState BackKey(cMenuFileBrowser *menu){ return osContinue; }
    virtual eOSState InfoKey(cMenuFileBrowser *menu){ return osContinue; }
    virtual eOSState AnyKey(cMenuFileBrowser *menu, eKeys Key){ return menu->StandardKeyHandling(Key); }
    virtual void EnterState(cMenuFileBrowser *menu, cMenuBrowserBaseIf *lastIf = NULL, eOSState defOSState = osContinue);
    virtual void SetHelp(cMenuFileBrowser *menu); 
    virtual void SetMode(cMenuFileBrowser *menu, browseMode mode, bool force = false){}

    protected:
    virtual void SetMessage(){};
    cMenuBrowserBaseIf(){};
    std::string message_;
};

//----------------------------------------------------------------
//-----------------cMenuBrowserBlockedIf--------------------------
//----------------------------------------------------------------

class cMenuBrowserBlockedIf : public cMenuBrowserBaseIf
{
    public:
    /*override*/ eOSState AnyKey(cMenuFileBrowser *menu, eKeys Key);
    static cMenuBrowserBlockedIf *Instance(){return &instance;}

    protected:
    cMenuBrowserBlockedIf(){};

    private:
    static cMenuBrowserBlockedIf instance;
};

//----------------------------------------------------------------
//-----------------cMenuBrowserNavigateIf-------------------------
//----------------------------------------------------------------

class cMenuBrowserNavigateIf : public cMenuBrowserBaseIf
{
    public:
    /*override*/ eOSState OkKey(cMenuFileBrowser *menu);
    /*override*/ eOSState BackKey(cMenuFileBrowser *menu);
    /*override*/ eOSState InfoKey(cMenuFileBrowser *menu);
    /*override*/ eOSState AnyKey(cMenuFileBrowser *menu, eKeys Key);

    protected:
    cMenuBrowserNavigateIf(){}; 
};

//----------------------------------------------------------------
//-----------------cMenuBrowserExternalIf---------------
//----------------------------------------------------------------

//class cMenuBrowserExternalIf : public cMenuBrowserNavigateIf
//{
//    public:
//    static cMenuBrowserExternalIf *Instance(){return &instance;} 
//    /*override*/ eOSState OkKey(cMenuFileBrowser *menu);
//    /*override*/ eOSState RedKey(cMenuFileBrowser *menu);
//    /*override*/ eOSState GreenKey(cMenuFileBrowser *menu);   
//    /*override*/ void EnterState(cMenuFileBrowser *menu, cMenuBrowserBaseIf *lastIf, eOSState defOSState); 
//    /*override*/ void SetHelp(cMenuFileBrowser *menu);

//    protected:
//    cMenuBrowserExternalIf(){};
//    // /*override*/ void SetMessage();
//    // virtual void Action(cMenuFileBrowser *menu){};
//    // virtual void LeaveState(cMenuFileBrowser *menu, bool ok);   
//
//    private:
//    static cMenuBrowserExternalIf instance;
//};

//----------------------------------------------------------------
//-----------------cMenuBrowserRequestIf--------------------------
//----------------------------------------------------------------

class cMenuBrowserRequestIf : public cMenuBrowserBaseIf
{
    public:
    /*override*/ eOSState AnyKey(cMenuFileBrowser *menu, eKeys Key);
    /*override*/ void EnterState(cMenuFileBrowser *menu, cMenuBrowserBaseIf *lastIf = NULL, eOSState defOSState = osContinue); 

    protected:
    cMenuBrowserRequestIf(){}; 
    virtual void Action(cMenuFileBrowser *menu){};
    virtual eOSState LeaveState(cMenuFileBrowser *menu, bool ok){return osContinue;};
    virtual eOSState LeaveStateDefault(cMenuFileBrowser *menu);
    virtual std::string GetRequestString(){return "";}
    cMenuBrowserBaseIf *lastIf_;
    eOSState defOSState_;
};

//----------------------------------------------------------------
//----------------cMenuBrowserDeleteRequestIf---------------------
//----------------------------------------------------------------

class cMenuBrowserDeleteRequestIf : public cMenuBrowserRequestIf
{
    public:
    static cMenuBrowserDeleteRequestIf *Instance(){return &instance;} 
    /*override*/ void EnterState(cMenuFileBrowser *menu, cMenuBrowserBaseIf *lastIf = NULL, eOSState defOSState = osContinue);

    protected:
    cMenuBrowserDeleteRequestIf(){}; 
    /*override*/ void Action(cMenuFileBrowser *menu);
    /*override*/ eOSState LeaveState(cMenuFileBrowser *menu, bool ok); 
    /*override*/ std::string GetRequestString();

    private:
    static cMenuBrowserDeleteRequestIf instance;
};

//----------------------------------------------------------------
//----------------cMenuBrowserPlaylistRequestIf---------------------
//----------------------------------------------------------------

class cMenuBrowserPlaylistRequestIf : public cMenuBrowserRequestIf
{
    public:
    static cMenuBrowserPlaylistRequestIf *Instance(){return &instance;} 

    protected:
    cMenuBrowserPlaylistRequestIf(){};
    /*override*/ eOSState  LeaveState(cMenuFileBrowser *menu, bool ok);
    /*override*/ std::string GetRequestString();

    private:
    static cMenuBrowserPlaylistRequestIf instance;
};

//----------------------------------------------------------------
//----------------cMenuBrowserShortPlaylistRequestIf---------------------
//----------------------------------------------------------------

class cMenuBrowserShortPlaylistRequestIf : public cMenuBrowserPlaylistRequestIf
{
    public:
    static cMenuBrowserShortPlaylistRequestIf *Instance(){return &instance;} 

    protected:
    cMenuBrowserShortPlaylistRequestIf(){};
    /*override*/ eOSState  LeaveState(cMenuFileBrowser *menu, bool ok);

    private:
    static cMenuBrowserShortPlaylistRequestIf instance;
};

//----------------------------------------------------------------
//-----------------cMenuBrowserRequestAndNavigateIf---------------
//----------------------------------------------------------------

class cMenuBrowserRequestAndNavigateIf : public cMenuBrowserNavigateIf
{
    public:
    /*override*/ eOSState RedKey(cMenuFileBrowser *menu);
    /*override*/ eOSState GreenKey(cMenuFileBrowser *menu);   

    protected:
    cMenuBrowserRequestAndNavigateIf(){};
    /*override*/ void SetMessage();
    virtual void Action(cMenuFileBrowser *menu){};
    virtual void LeaveState(cMenuFileBrowser *menu, bool ok);
};

//----------------------------------------------------------------
//-----------------cMenuBrowserExternalIf---------------
//----------------------------------------------------------------

class cMenuBrowserExternalIf : public cMenuBrowserRequestAndNavigateIf
{
    public:
    static cMenuBrowserExternalIf *Instance(){return &instance;} 
    /*override*/ eOSState OkKey(cMenuFileBrowser *menu);
    /*override*/ eOSState RedKey(cMenuFileBrowser *menu);
    /*override*/ eOSState GreenKey(cMenuFileBrowser *menu);   
    /*override*/ void EnterState(cMenuFileBrowser *menu, cMenuBrowserBaseIf *lastIf, eOSState defOSState); 
    /*override*/ void SetHelp(cMenuFileBrowser *menu);

    protected:
    cMenuBrowserExternalIf(){};
    // /*override*/ void SetMessage();
    /*override*/ void Action(cMenuFileBrowser *menu); 

    private:
    static cMenuBrowserExternalIf instance;
};

//----------------------------------------------------------------
//----------------cMenuBrowserCopyRequestIf-----------------------
//----------------------------------------------------------------

class cMenuBrowserCopyRequestIf : public cMenuBrowserRequestAndNavigateIf
{
    public:
    /*override*/ eOSState YellowKey(cMenuFileBrowser *menu);
    /*override*/ void EnterState(cMenuFileBrowser *menu, cMenuBrowserBaseIf *lastIf = NULL, eOSState defOSState = osContinue);
    /*override*/ void SetHelp(cMenuFileBrowser *menu);
    static cMenuBrowserCopyRequestIf *Instance(){return &instance;}

    protected:
    cMenuBrowserCopyRequestIf(){}; 
    /*override*/ void SetMessage();
    /*override*/ void Action(cMenuFileBrowser *menu);

    private:
    static cMenuBrowserCopyRequestIf instance;
};

//----------------------------------------------------------------
//----------------cMenuBrowserMoveRequestIf-----------------------
//----------------------------------------------------------------

class cMenuBrowserMoveRequestIf : public cMenuBrowserCopyRequestIf
{
    public: 
    static cMenuBrowserMoveRequestIf *Instance(){return &instance;}

    protected:
    cMenuBrowserMoveRequestIf(){}; 
    /*override*/ void SetMessage();
    /*override*/ void Action(cMenuFileBrowser *menu);

    private:
    static cMenuBrowserMoveRequestIf instance;
};

//----------------------------------------------------------------
//----------------cMenuBrowserCopyToHdRequestIf-------------------
//----------------------------------------------------------------

class cMenuBrowserCopyToHdRequestIf : public cMenuBrowserCopyRequestIf
{
    public:
    /*override*/ eOSState BlueKey(cMenuFileBrowser *menu);
    /*override*/ void EnterState(cMenuFileBrowser *menu, cMenuBrowserBaseIf *lastIf = NULL, eOSState defOSState = osContinue); 
    /*override*/ void SetHelp(cMenuFileBrowser *menu);
    static cMenuBrowserCopyToHdRequestIf *Instance(){return &instance;}

    protected:
    cMenuBrowserCopyToHdRequestIf(){}; 

    private:
    static cMenuBrowserCopyToHdRequestIf instance;
};

//----------------------------------------------------------------
//----------------cMenuBrowserMoveToHdRequestIf-------------------
//----------------------------------------------------------------

class cMenuBrowserMoveToHdRequestIf : public cMenuBrowserCopyToHdRequestIf
{
    public:
    static cMenuBrowserMoveToHdRequestIf *Instance(){return &instance;}

    protected:
    cMenuBrowserMoveToHdRequestIf(){}; 
    /*override*/ void SetMessage();

    private:
    static cMenuBrowserMoveToHdRequestIf instance;
};

//----------------------------------------------------------------
//----------------cMenuBrowserStandardIf--------------------------
//----------------------------------------------------------------

class cMenuBrowserStandardIf : public cMenuBrowserNavigateIf
{
    public:
    /*override*/ eOSState OkKey(cMenuFileBrowser *menu);
    /*override*/ eOSState RedKey(cMenuFileBrowser *menu);
    /*override*/ eOSState GreenKey(cMenuFileBrowser *menu);
    /*override*/ eOSState YellowKey(cMenuFileBrowser *menu);
    /*override*/ eOSState BlueKey(cMenuFileBrowser *menu);
    /*override*/ void EnterState(cMenuFileBrowser *menu, cMenuBrowserBaseIf *lastIf = NULL, eOSState defOSState = osContinue); 
    /*override*/ void SetHelp(cMenuFileBrowser *menu); 
    /*override*/ void SetMode(cMenuFileBrowser *menu, browseMode mode, bool force = false);
    static cMenuBrowserStandardIf *Instance(){return &instance;}

    protected:
    cMenuBrowserStandardIf(){};
    void SetHelpMarkUnmark(cMenuFileBrowser *menu);

    private:
    static cMenuBrowserStandardIf instance;
};

//----------------------------------------------------------------
//----------------cMenuBrowserRestrictedIf--------------------------
//----------------------------------------------------------------

class cMenuBrowserRestrictedIf : public cMenuBrowserStandardIf
{
    public:
    /*override*/ eOSState OkKey(cMenuFileBrowser *menu);
    /*override*/ eOSState YellowKey(cMenuFileBrowser *menu);
    /*override*/ void EnterState(cMenuFileBrowser *menu, cMenuBrowserBaseIf *lastIf = NULL, eOSState defOSState = osContinue); 
    /*override*/ void SetHelp(cMenuFileBrowser *menu);
    /*override*/ void SetMode(cMenuFileBrowser *menu, browseMode mode, bool force = false);
    static cMenuBrowserRestrictedIf *Instance(){return &instance;}

    protected:
    cMenuBrowserRestrictedIf(){};

    private:
    static cMenuBrowserRestrictedIf instance;
};

//----------------------------------------------------------------
//----------------cMenuBrowserBrowsePlayListIf-------------
//----------------------------------------------------------------

class cMenuBrowserBrowsePlayListIf : public cMenuBrowserRestrictedIf
{
    public:
    /*override*/ eOSState OkKey(cMenuFileBrowser *menu);
    /*override*/ eOSState BackKey(cMenuFileBrowser *menu);
    /*override*/ eOSState YellowKey(cMenuFileBrowser *menu); 
    /*override*/ eOSState BlueKey(cMenuFileBrowser *menu){return osContinue;}
    /*override*/ void SetHelp(cMenuFileBrowser *menu); 
    /*override*/ void EnterState(cMenuFileBrowser *menu, cMenuBrowserBaseIf *lastIf = NULL, eOSState defOSState = osContinue); 
    /*override*/ void SetMode(cMenuFileBrowser *menu, browseMode mode, bool force = false);
    static cMenuBrowserBrowsePlayListIf *Instance(){return &instance;}

    protected:
    cMenuBrowserBrowsePlayListIf(){};

    private:
    static cMenuBrowserBrowsePlayListIf instance;
};

//----------------------------------------------------------------
//----------------cMenuBrowserAutoStartIf--------------------------
//----------------------------------------------------------------

class cMenuBrowserAutoStartIf : public cMenuBrowserStandardIf
{
    public:
    /*override*/ eOSState YellowKey(cMenuFileBrowser *menu);
    /*override*/ void SetHelp(cMenuFileBrowser *menu);
    static cMenuBrowserAutoStartIf *Instance(){return &instance;}

    protected:
    cMenuBrowserAutoStartIf(){};

    private:
    static cMenuBrowserAutoStartIf instance;
};

//----------------------------------------------------------------
//----------------cMenuBrowserCommandIf---------------------------
//----------------------------------------------------------------

#ifdef USE_KEYS_FOR_COMMANDS
class cMenuBrowserCommandIf : public cMenuBrowserNavigateIf
{
    public:
    /*override*/ eOSState RedKey(cMenuFileBrowser *menu);
    /*override*/ eOSState GreenKey(cMenuFileBrowser *menu);
    /*override*/ eOSState YellowKey(cMenuFileBrowser *menu);
    /*override*/ eOSState BlueKey(cMenuFileBrowser *menu);
    /*override*/ eOSState BackKey(cMenuFileBrowser *menu);
    /*override*/ void SetHelp(cMenuFileBrowser *menu);
    static cMenuBrowserCommandIf *Instance(){return &instance;}
 
    protected:
    cMenuBrowserCommandIf(){};

    private:
    static cMenuBrowserCommandIf instance;
};
#endif

//----------------------------------------------------------------
//----------------cMenuBrowserOptionsIf---------------------------
//----------------------------------------------------------------

class cMenuBrowserOptionsIf : public cMenuBrowserBaseIf
{
    public:  
    /*override*/ eOSState OkKey(cMenuFileBrowser *menu);
    /*override*/ eOSState BackKey(cMenuFileBrowser *menu); 
    /*override*/ void EnterState(cMenuFileBrowser *menu, cMenuBrowserBaseIf *lastIf = NULL, eOSState defOSState = osContinue);
    static cMenuBrowserOptionsIf *Instance(){return &instance;}
 
    protected:
    cMenuBrowserOptionsIf(){}; 
    virtual void CreateOptions(cMenuFileBrowser *menu){};
    virtual void AddOptions(cMenuFileBrowser *menu);
    virtual eOSState LeaveStateDefault(cMenuFileBrowser *menu);
    virtual eOSState ExecuteOptions(cMenuFileBrowser *menu, std::string option){return osContinue;} 
    std::vector<std::string> options_;
    cMenuBrowserBaseIf *lastIf_;
    eOSState defOSState_;

    private:
    static cMenuBrowserOptionsIf instance;
};

//----------------------------------------------------------------
//----------------cMenuBrowserCmdOptionsIf------------------------
//----------------------------------------------------------------

class cMenuBrowserCmdOptionsIf : public cMenuBrowserOptionsIf
{
    public:
    static cMenuBrowserCmdOptionsIf *Instance(){return &instance;}
 
    protected:
    cMenuBrowserCmdOptionsIf(){};
    /*override*/ void CreateOptions(cMenuFileBrowser *menu);
    /*override*/ eOSState ExecuteOptions(cMenuFileBrowser *menu, std::string option);

    private:
    eOSState Copy(cMenuFileBrowser *menu);
    eOSState Move(cMenuFileBrowser *menu);
    eOSState Delete(cMenuFileBrowser *menu);
    eOSState Rename(cMenuFileBrowser *menu);
    eOSState CreateDir(cMenuFileBrowser *menu);
    eOSState InsertInPlayList(cMenuFileBrowser *menu);
    eOSState ShowPlayList(cMenuFileBrowser *menu);
    eOSState ConvertToUTF8(cMenuFileBrowser *menu);
    eOSState BurnIso(cMenuFileBrowser *menu);
    eOSState MakeAndBurnIso(cMenuFileBrowser *menu);

    static cMenuBrowserCmdOptionsIf instance;
};

//----------------------------------------------------------------
//----------------cMenuBrowserShutdownOptionsIf------------------------
//----------------------------------------------------------------

class cMenuBrowserPlaylistOptionsIf : public cMenuBrowserOptionsIf
{
    public:
    static cMenuBrowserPlaylistOptionsIf *Instance(){return &instance;}
 
    protected:
    cMenuBrowserPlaylistOptionsIf(){};
    /*override*/ eOSState BackKey(cMenuFileBrowser *menu);
    /*override*/ void CreateOptions(cMenuFileBrowser *menu);
    /*override*/ eOSState ExecuteOptions(cMenuFileBrowser *menu, std::string option);

    private:
    eOSState KeepPlayList(cMenuFileBrowser *menu);
    eOSState SavePlayList(cMenuFileBrowser *menu);
    eOSState DiscardPlayList(cMenuFileBrowser *menu);
    eOSState CopyPlayListToUSB(cMenuFileBrowser *menu);
    eOSState CopyPlayListToDVD(cMenuFileBrowser *menu);

    static cMenuBrowserPlaylistOptionsIf instance;
};

//----------------------------------------------------------------
//----------------cMenuBrowserCreateDirIf-------------------------
//----------------------------------------------------------------

class cMenuBrowserCreateDirIf : public cMenuBrowserOptionsIf
{
    public:  
    /*override*/ eOSState OkKey(cMenuFileBrowser *menu);
    static cMenuBrowserCreateDirIf *Instance(){return &instance;}
 
    protected:
    cMenuBrowserCreateDirIf(){name_[0] = 0;}
    /*override*/ void CreateOptions(cMenuFileBrowser *menu);
    /*override*/ void AddOptions(cMenuFileBrowser *menu);
    virtual eOSState BackKey(cMenuFileBrowser *menu);

    private:
    bool CreateDir(cMenuFileBrowser *menu);
    static cMenuBrowserCreateDirIf instance;
    char name_[256];
};

//----------------------------------------------------------------
//----------------cMenuBrowserRenameEntryIf---------------------------
//----------------------------------------------------------------

class cMenuBrowserRenameEntryIf : public cMenuBrowserOptionsIf
{
    public:  
    /*override*/ eOSState OkKey(cMenuFileBrowser *menu);
    static cMenuBrowserRenameEntryIf *Instance(){return &instance;}
 
    protected:
    cMenuBrowserRenameEntryIf(){name_[0] = 0;}
    /*override*/ void CreateOptions(cMenuFileBrowser *menu);
    /*override*/ void AddOptions(cMenuFileBrowser *menu);
    virtual eOSState BackKey(cMenuFileBrowser *menu);

    private: 
    bool RenameEntry(cMenuFileBrowser *menu);
    static cMenuBrowserRenameEntryIf instance;
    std::string oldname_;
    char name_[256];
};

#endif //P__USERIF_H
