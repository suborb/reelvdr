/***************************************************************************
 *   Copyright (C) 2005 by Reel Multimedia;  Author:  Markus Hahn          * 
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
 *
 *************************************************************************** 
 *   
 *   service.h: provides Select Channels(.conf) via service interface
 *   Todo Autoscanner  for nstall wizard 
 *
 ***************************************************************************/

#ifndef __SERVICE_H
#define __SERVICE_H


#include <vdr/osdbase.h>
#include <string>
#include <vector>

// --- struct sSelectChannelsMenu   ---------------------------------------
// used to trigger Channelscan out of other plugins eg. install wizzard
struct sSelectChannelsMenu
{
    // in 
    // out
    cOsdMenu *pSelectMenu;      // pointer to the select channels menu
};

struct sSelectChannelsList      // for Install Wizard
{
    // in 
    const char *newTitle;
    const char *Path;
      std::string AdditionalText;
    bool WizardMode;
    // out
    cOsdMenu *pSelectMenu;      // pointer to the select channels menu
};

// --- class AutoScan  ----------------------------------------------------
/*
class cAutoScan 
{
public:

  void LoadSources();
 
private:
  std::vector<int> v_sources;
};
*/


#endif
