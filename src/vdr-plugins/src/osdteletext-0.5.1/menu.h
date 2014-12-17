/***************************************************************************
 *       Copyright (c) 2003,2004 by Marcel Wiesweg                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef OSDTELETEXT_MENU__H
#define OSDTELETEXT_MENU__H

#include <vdr/osd.h>
#include <vdr/osdbase.h>
#include <vdr/status.h>

#include "txtrecv.h"
#include "setup.h"

extern int Stretch;

class ChannelStatus:public cStatus
{
  public:
    ChannelStatus();
  protected:
    virtual void ChannelSwitch(const cDevice * Device, int ChannelNumber);
};


class TeletextBrowser:public cOsdObject
{
  public:
    TeletextBrowser(cTxtStatus * txtSt);
    ~TeletextBrowser();
    void Show(void);
    static void ChannelSwitched(int ChannelNumber);
    virtual eOSState ProcessKey(eKeys Key);
  protected:
    enum Direction
    { DirectionForward, DirectionBackward };
    void SetNumber(int i);
    void ShowPage();
    void UpdateClock();
    bool DecodePage();
    void ChangePageRelative(bool back = false);
    void ChangeSubPageRelative(bool back = false);
    bool CheckPage();
    bool CheckFirstSubPage(int startWith);
    void ShowAskForChannel();
    void SetPreviousPage(int oldPage, int oldSubPage, int newPage);
    int PageCheckSum();
    void ShowPageNumber();
    void ExecuteAction(eTeletextAction e);
    int nextValidPageNumber(int start, bool direction = true);
    char fileName[PATH_MAX];
    char page[40][24];
    int cursorPos;
    eTeletextAction TranslateKey(eKeys Key);
    bool pageFound;
    bool selectingChannel;
    bool needClearMessage;
    int selectingChannelNumber;
    int checkSum;
    cTxtStatus *txtStatus;
    bool suspendedReceiving;
    int previousPage;
    int previousSubPage;
    int pageBeforeNumberInput;
    time_t lastActivity;
    int inactivityTimeout;
    static int currentPage;
    static int currentSubPage;
    static const cChannel* channel;
    static int currentChannelNumber;
    static TeletextBrowser *self;
};


#endif
