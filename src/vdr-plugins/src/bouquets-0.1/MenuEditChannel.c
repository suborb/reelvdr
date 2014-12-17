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

#include "MenuEditChannel.h"

#include "vdr/menu.h"
#if APIVERSNUM > 10700
  #include "vdr/channels.h"
#else
  #define CA_MCLI_LOWER 1
  #define CA_MCLI_UPPER 2
#endif

//---------------------------------------------------------------------
//----------------------------cMenuMyEditChannel-------------------------
//---------------------------------------------------------------------

cMenuMyEditChannel::cMenuMyEditChannel(cChannel * Channel, bool New):cOsdMenu(trVDR("Edit channel"),
         16)
{
    channel = Channel;
    new_ = New;
    if (strcmp(Skins.Current()->Name(), "Reel") == 0)
    {
        strcpy((char *)&titleBuf, "menunormalhidden$");
        strcat((char *)&titleBuf, trVDR("Edit channel"));
        SetTitle((const char *)&titleBuf);
    }
    if (channel)
    {
        data = *channel;
        if (New)
        {
            SetNew();
        }
        Setup();
    }
}

void cMenuMyEditChannel::SetNew()
{
    channel = NULL;
    data.nid = 0;
    data.tid = 0;
    data.rid = 0;
    data.sid = 0;
    data.apids[0] = data.apids[1] = 0;
    data.vpid = 0;
    data.tpid = 0;
    data.caids[0] = 0;
    data.ppid = 0;
    data.SetName(tr("New"), "", "");
}

void cMenuMyEditChannel::Setup(void)
{
    int current = Current();

    Clear();

    strn0cpy(name, data.name, sizeof(name));
    Add(new cMenuEditStrItem(trVDR("Name"), name, sizeof(name), trVDR(FileNameChars)));
    Add(new cMenuEditSrcItem(trVDR("Source"), &data.source));
    Add(new cMenuEditIntItem(trVDR("Frequency"), &data.frequency));
#if APIVERSNUM < 10700
    char type = **cSource::ToString(data.source);
#define ST(s) if (strchr(s, type))
    ST(" S ") Add(new cMenuEditChrItem(trVDR("Polarization"), &data.polarization, "hvlr"));
    ST("CS ") Add(new cMenuEditIntItem(tr("Symbolrate"), &data.srate));
    ST("C T") Add(new
                  cMenuEditMapItem(trVDR("CoderateH"), &data.coderateH, CoderateValues,
                                   trVDR("none")));
    ST(" S ") Add(new
                  cMenuEditMapItem(trVDR("CoderateH"), &data.coderateH, CoderateValuesS,
                                   trVDR("none")));
    ST("  T") Add(new
                  cMenuEditMapItem(trVDR("CoderateL"), &data.coderateL, CoderateValues,
                                   trVDR("none")));
#endif
    Add(new cMenuEditIntItem(tr("System-ID"), &data.sid, 1, 0xFFFF));
    Add(new cMenuEditIntItem(trVDR("Vpid"), &data.vpid, 0, 0x1FFF));
    Add(new cMenuEditIntItem(trVDR("Ppid"), &data.ppid, 0, 0x1FFF));
    Add(new cMenuEditIntItem(trVDR("Apid1"), &data.apids[0], 0, 0x1FFF));
    Add(new cMenuEditIntItem(trVDR("Dpid1"), &data.dpids[0], 0, 0x1FFF));
    Add(new cMenuEditIntItem(trVDR("Tpid"), &data.tpid, 0, 0x1FFF));
    Add(new cMenuEditMyCaItem(tr("CI-Slot"), &data.caids[0], false)); //XXX
#if APIVERSNUM < 10700
    ST("C T") Add(new
                  cMenuEditMapItem(trVDR("Modulation"), &data.modulation, ModulationValues,
                                   "QPSK"));
    ST(" S ") Add(new
                  cMenuEditMapItem(trVDR("Modulation"), &data.modulation, ModulationValuesS,
                                   "4"));
    ST("  T") Add(new
                  cMenuEditMapItem(trVDR("Bandwidth"), &data.bandwidth, BandwidthValues));
    ST("  T") Add(new
                  cMenuEditMapItem(trVDR("Transmission"), &data.transmission,
                                   TransmissionValues));
    ST("  T") Add(new cMenuEditMapItem(trVDR("Guard"), &data.guard, GuardValues));
    ST("  T") Add(new
                  cMenuEditMapItem(trVDR("Hierarchy"), &data.hierarchy, HierarchyValues,
                                   trVDR("none")));
    ST(" S ") Add(new
                  cMenuEditMapItem(trVDR("Rolloff"), &data.rolloff, RolloffValues, "35"));
#else
#ifdef USE_CHANNELBIND
    Add(new cMenuEditIntItem( trVDR("Rid"),          &data.rid, 0)); // channel binding patch
#endif /* CHANNELBIND */
    //RC: taken from vdr 1.7.16
    // Parameters for specific types of sources:
    sourceParam = SourceParams.Get(**cSource::ToString(data.source));
    if (sourceParam)
    {
        sourceParam->SetData(&data);
        cOsdItem *Item;
        while ((Item = sourceParam->GetOsdItem()) != NULL)
            Add(Item);
    }
#endif

    if (!new_)
    {
        SetHelp(NULL, NULL, NULL, tr("Button$New"));
    }

    SetCurrent(Get(current));
    Display();
}

eOSState cMenuMyEditChannel::ProcessKey(eKeys Key)
{
    int oldSource = data.source;
    eOSState state = cOsdMenu::ProcessKey(Key);

    if (state == osUnknown)
    {
        if (Key == kOk)
        {
#if APIVERSNUM > 10700
            if (sourceParam)
                sourceParam->GetData(&data);
#endif
	    if (Channels.HasUniqueChannelID(&data, channel))
            {
                data.name = strcpyrealloc(data.name, name);
                if (channel)
                {
                    *channel = data;
                    isyslog("edited channel %d %s", channel->Number(), *data.ToText());
                    state = osBack;
                }
                else
                {
                    channel = new cChannel;
                    *channel = data;
                    Channels.Add(channel);
                    Channels.ReNumber();
                    isyslog("added channel %d %s", channel->Number(), *data.ToText());
                    state = osUser1;
                }
                Channels.SetModified(true);
            }
            else
            {
                Skins.Message(mtError, trVDR("Channel settings are not unique!"));
                state = osContinue;
            }
        }
        if (Key == kBlue && !new_)
        {
            SetNew();
            Setup();
            state = osContinue;
        }
    }
    if (Key != kNone
        && (data.source & cSource::st_Mask) != (oldSource & cSource::st_Mask))
        Setup();
    return state;
}

#if APIVERSNUM < 10700
#define NUMCISLOTS 2
#endif
cMenuEditMyCaItem::cMenuEditMyCaItem(const char *Name, int *Value, bool EditingBouquet)
    : cMenuEditIntItem(Name, Value, 0, /*EditingBouquet ? NUMCISLOTS :*/ CA_ENCRYPTED_MAX)
{
    memset(Items, 0, sizeof(Items));
    bool hasInt=false;
    Items[CA_FTA] = true;
#if APIVERSNUM < 10700
    hasInt = true;
#else
    editingBouquet = EditingBouquet;
    for(int i=0; i<cDevice::NumDevices(); i++) {
        cDevice *dev = cDevice::GetDevice(i);
        if(dev) {
            if(dev->ProvidesSource(cSource::stSat) || dev->ProvidesSource(cSource::stCable) || dev->ProvidesSource(cSource::stTerr)) {
                if(cDevice::GetDevice(i)->HasInternalCam()) {
                    hasInt = true;
                } else {
                    int idx = cDevice::GetDevice(i)->CardIndex()+1;
                    if(idx < CA_ENCRYPTED_MIN) Items[idx] = true;
                } // if
            } // if
        } // if
    } // if
#endif /**/
    if(hasInt) {
        Items[CA_MCLI_LOWER] = true;
        Items[CA_MCLI_UPPER] = true;
    } // if
    Set();
};

void cMenuEditMyCaItem::Set(void)
{
    char s[64] = {0};
    if (*value == CA_FTA)
	strcpy(s, editingBouquet ? trVDR("no") : trVDR("Free To Air"));
    else if (*value == CA_MCLI_UPPER)
    {
#if APIVERSNUM < 10700
	// TODO: adjust for vdr 1.7
	if (cDevice::GetDevice(0)->CiHandler())
	    sprintf(s, "%s (%s)", (cDevice::GetDevice(0))->CiHandler()->GetCamName(1) ?
	                           (cDevice::GetDevice(0))->CiHandler()->GetCamName(1) : tr("No CAM at"), tr("upper slot"));
	else
#endif
	    sprintf(s, "%s", tr("upper slot"));
    }
    else if (*value == CA_MCLI_LOWER)
    {
#if APIVERSNUM < 10700
	// TODO: adjust for vdr 1.7
	if( cDevice::GetDevice(0)->CiHandler())
	    sprintf(s, "%s (%s)", (cDevice::GetDevice(0))->CiHandler()->GetCamName(0) ?
	                           (cDevice::GetDevice(0))->CiHandler()->GetCamName(0) : tr("No CAM at"), tr("lower slot"));
	else
#endif
	    sprintf(s, "%s", tr("lower slot"));
    }

    if (s[0])
	SetValue(s);
    else if (*value >= CA_ENCRYPTED_MIN)
	SetValue(trVDR("encrypted"));
    else
	cMenuEditIntItem::Set();
};

eOSState cMenuEditMyCaItem::ProcessKey(eKeys Key)
{
  eOSState state = cMenuEditItem::ProcessKey(Key);
  // FTA - Lower - Upper - DVB_MIN..DVB_MAX -Encrypted - FTA
  if (state == osUnknown) {
     if (NORMALKEY(Key) == kLeft) {
        if(CA_FTA == *value) {
           *value = CA_ENCRYPTED_MAX;
        } else {
           if(*value >= CA_ENCRYPTED_MIN) *value = CA_ENCRYPTED_MIN;
           (*value)--;
           while(!Items[*value]) (*value)--;
        } // if
     } else if (NORMALKEY(Key) == kRight) {
        (*value)++;
        if(*value >= CA_ENCRYPTED_MIN) *value = CA_FTA;
        while((*value < CA_ENCRYPTED_MIN) && !Items[*value]) (*value)++;
        if(*value >= CA_ENCRYPTED_MIN) *value = CA_ENCRYPTED_MAX;
     } else return cMenuEditIntItem::ProcessKey(Key);
     Set();
     state = osContinue;
     }
  return state;
}
