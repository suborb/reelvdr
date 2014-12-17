#ifdef REELVDR
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

// ReelboxBase.h

#ifndef REEL_BOX_BASE_H
#define REEL_BOX_BASE_H
#include "debug.h"
#include "tools.h"
#include <stdio.h>
#include "channels.h"

class cReelBoxBase 
{   
private:
    static cReelBoxBase *reelbox;  
    cReelBoxBase(const cReelBoxBase &ReelBoxBase);
    cReelBoxBase &operator=(const cReelBoxBase &ReelBoxBase);

public:    
    virtual void PlayPipVideo(const unsigned char *Data, int Length){
    PRINTF("-------------cReelBoxBase::PlayPipVideo-----------------\n");
    };
    virtual void StartPip(bool start){ PRINTF("virtual void StartPip"); };
    virtual bool IsPipChannel(const cChannel *channel, bool *ret) { return false; }; // return true if function is supported
    virtual bool SetPipVideoType(int type) { return true; };
    virtual void SetPipDimensions(const uint x, const uint y, const uint width, const uint height){};
    virtual void ChannelSwitchInLiveMode(){ PRINTF("virtual void ChannelSwitchInLiveMode"); };
    virtual bool NeedPipStartInActivate() { return false;};
    static cReelBoxBase* Instance();
    virtual ~cReelBoxBase() {};
    
protected: 
    cReelBoxBase();
};

inline cReelBoxBase::cReelBoxBase()
{
    reelbox = this;
}

inline cReelBoxBase* cReelBoxBase::Instance()
{
    return reelbox;
}    

#endif // P__REEL_BOX_BASE_H
#endif /*REELVDR*/

