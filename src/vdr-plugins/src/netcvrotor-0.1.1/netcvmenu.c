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

#include <iostream>
#include <fstream>
#include <sstream>

#include <boost/iostreams/copy.hpp>

#include "vdr/osdbase.h"
#include "vdr/sources.h"
#include "vdr/menu.h"
#include "vdr/channels.h"
#include "vdr/diseqc.h"
#include "vdr/osd.h"
#include "vdr/remote.h"
#include "vdr/device.h"
#include "vdr/sources.h"

#include "netcvmenu.h"
#include "rotortools.h"


int strToInt(const std::string &s)
{
    std::istringstream i(s);
    int
        x = -1;
    if (!(i >> x))
        ;                       // no error message
    //std::cerr << " error in strToInt(): " << s << std::endl;
    return x;
}

int IntToFec(int val)
{
    // needed for old transponder lists
    switch (val)
    {
    case 12:
        return FEC_1_2;
    case 23:
        return FEC_2_3;
    case 34:
        return FEC_3_4;
    case 45:
        return FEC_4_5;
    case 56:
        return FEC_5_6;
    case 67:
        return FEC_6_7;
    case 78:
        return FEC_7_8;
    default:
        return FEC_AUTO;
    }
    return FEC_NONE;
}

//----------cTransponder-----------------------

std:: string cTransponder::TplFileName(int satCodec)
{
    std::string tmp = cPlugin::ConfigDirectory();
    tmp += "/transponders/";

    if (satCodec == 0)
    {
        //tmp += "SatNitScan";
        tmp += "Sat";
    }
    else if (satCodec == 1)
    {
        tmp += "NIT";
    }
    else
    {
        tmp += *cSource::ToString(satCodec);
    }

    tmp += ".tpl";
    return tmp;
}

bool cTransponder::LoadTpl(const std::string & tplFileName, std::vector <cTransponder *> &transponders)
{
    //printf("-------cTransponder::LoadTpl %s-------------\n", tplFileName.c_str());

    std::ifstream transponderList(tplFileName.c_str(), std::ios_base::in | std::ios_base::binary);
    std::stringstream buffer;
 
    if (!transponderList)
    {   
        printf("ERROR: netcvrotor can`t open LoadSatTpls %s\n", tplFileName.c_str());
        return false;
    } 
    boost::iostreams::copy(transponderList, buffer);

    std::string line;
    int lc = 0;

    while (!buffer.eof())
    {
        getline(buffer, line);

        if (line.find('[') == line.npos)
        {

            cTransponder *t = new cTransponder();

            if (t->Parse(line))
            {
                transponders.push_back(t);
            }
        }
        lc++;
    }

    transponderList.close();

    return true;
}

bool cTransponder::Parse(const std::string & Line)
{
    //printf("-------cTransponder::Parse %s-------------\n", Line.c_str());   
    std::string tpNumb(Line);

    int index = tpNumb.find_first_of('=');
    if (index == -1)
        return false;

    tpNumb = tpNumb.erase(0, index + 1);

    //   chop  string
    std::string tmp = Line.substr(index + 1);

    // get Frequenz
    index = tmp.find_first_of(',');

    if (index == -1)
        return false;

    std::string freq = tmp;
    freq.erase(index);

    // get polarisation
    std::string polar = tmp.substr(index + 1);
    index = polar.find_first_of(',');
    if (index == -1)
        return false;

    // get symbol rate
    std::string symRate = polar.substr(index + 1);
    polar.erase(index);

    index = symRate.find_first_of(',');
    if (index == -1)
        return false;

    std::string sFec = symRate.substr(index + 1);
    symRate.erase(index);

    channelNum_ = strToInt(tpNumb.c_str());
    frequency_ = strToInt(freq.c_str());
    if (frequency_ == -1)
        return false;
    pol_ = polar[0];
    symbolrate_ = strToInt(symRate.c_str());
    if (symbolrate_ == -1)
        return false;
    //coderate_ = strToInt(sFec.c_str());
    coderate_ = IntToFec(strToInt(sFec.c_str()));

    infoString_ = freq + " " +  pol_ + " " + symRate + " " + sFec;
    return true;
}

// ---------------cMenuSignalItem------------------

cMenuSignalItem::cMenuSignalItem(std::string title)
:cOsdItem(title.c_str() , osContinue, false), title_(title)
{
}

void cMenuSignalItem::Update(int value)
{
    value_ = value;
    Set();
}
  
void cMenuSignalItem::Set()
{
    char szProgress[9]; 
    //char szProgressPerc[4];
    //int perc = 100 * value_ / 0xFFFF;
    int frac = 10 * value_ / 0xFFFF;
    //perc = min(100, max(0, perc));
    frac = min(8, max(0, frac));

    for(int i = 0; i < frac; i++)
    {
        szProgress[i] = '|';
    }
    szProgress[frac]=0;
    sprintf(szProgressPart, "[%-8s]\t", szProgress);

    char perc[6];

    snprintf(perc, 6, ": %i%%", (100*value_/0xffff));

    titleBuf_ = title_;
    titleBuf_ += perc;
    titleBuf_ += std::string("\t") + szProgressPart;
    //printf("-----------titleBuf_ = %s------------\n", titleBuf_.c_str());

    SetText(titleBuf_.c_str(), true);
}

// ---------cMyMenuEditStraItem--------------------

cMyMenuEditStraItem::cMyMenuEditStraItem(const char *Name, int *Value, int NumStrings, const char * const *Strings)
:cMenuEditIntItem(Name, Value, 0, NumStrings - 1)
{ 
    strings = Strings;
    Set();
}

void cMyMenuEditStraItem::ReSet(const char *Name, int *Value, int NumStrings, const char * const *Strings)
{
    strings = Strings;
    value = Value;
    min = 0;
    max = NumStrings - 1; 
    minString = NULL;
    maxString = NULL;
    if (*value < min)
         *value = min;
    else if (*value > max)
        *value = max;
    cMenuEditIntItem::Set();
    Set(); 
}   

void cMyMenuEditStraItem::Set()
{
    if(strings)
    {
        SetValue(strings[*value]);
    }
}

// --- cMyMenuEditSrcItem (taken from channelscan plugin)-------------------

class cMyMenuEditSrcItem: public cMenuEditIntItem
{
  private:
    const cSource * source;
    int currentTuner;
  public:
    virtual void Set(void);
    void SetCurrentTuner(int nrTuner) { currentTuner = nrTuner; }
    cMyMenuEditSrcItem(const char *Name, int *Value, int currentTuner);
    void Next();
    eOSState ProcessKey(eKeys Key);
};

cMyMenuEditSrcItem::cMyMenuEditSrcItem(const char *Name, int *Value, int CurrentTuner):
cMenuEditIntItem(Name, Value, 0)
{
    source = Sources.Get(*Value);
    currentTuner = CurrentTuner;
    Set();
}

void cMyMenuEditSrcItem::Set(void)
{
    if (source)
    {
        char *buffer = NULL;
        //printf("%s\n", *cSource::ToString(source->Code()));
        asprintf(&buffer, "%s - %s", *cSource::ToString(source->Code()),
                 source->Description());
        SetValue(buffer);
        free(buffer);
    }
    else
    {
        cMenuEditIntItem::Set();
    }
}           

void cMyMenuEditSrcItem::Next()
{ 
    const cSource *oldSrc = source;
    bool found = false;

    if (cSource::IsSat(source->Code())) {
        for (cDiseqc *p = Diseqcs.First(); p && !found; p = Diseqcs.Next(p)) {
            source = oldSrc;
    /* only look at sources configured for this tuner */
            while (source && source->Next() && p && (p->Tuner() == currentTuner+1 || (TunerIsRotor(currentTuner) && IsWithinConfiguredBorders(currentTuner, source)))) {
            source = (cSource *) source->Next();
    if(source && (source->Code() == p->Source() || (TunerIsRotor(currentTuner) && IsWithinConfiguredBorders(currentTuner, source)))) {
                *value = source->Code();
                found = true;
                break;
            }
            }
        }
    } else {
        if (source && source->Next()) {
        source = (cSource *) source->Next();
        *value = source->Code();
        }
    }
}

eOSState cMyMenuEditSrcItem::ProcessKey(eKeys Key)
{
    eOSState state = cMenuEditItem::ProcessKey(Key);

    int oldCode = source->Code();
    const cSource *oldSrc = source;
    bool found = false;

    if (state == osUnknown)
    {
        if (NORMALKEY(Key) == kLeft)    // TODO might want to increase the delta if repeated quickly?
        {
           if(cSource::IsSat(source->Code())) {
             for (cDiseqc *p = Diseqcs.First(); p && !found; p = Diseqcs.Next(p)) {
                source = oldSrc;
        /* only look at sources configured for this tuner */
                while (source && source->Prev() && p && (p->Tuner() == currentTuner+1 || (TunerIsRotor(currentTuner) && IsWithinConfiguredBorders(currentTuner, source)))) {
                    source = (cSource *) source->Prev();
            if(source && (source->Code() == p->Source() || (TunerIsRotor(currentTuner) && IsWithinConfiguredBorders(currentTuner, source)))) {
                      *value = source->Code();
                      found = true;
              break;
            }
                }
            }
           } else {
             if (source && source->Prev()) {
                source = (cSource *) source->Prev();
                *value = source->Code();
             }
           }
        } else if (NORMALKEY(Key) == kRight) {
            if (cSource::IsSat(source->Code())) {
               for (cDiseqc *p = Diseqcs.First(); p && !found; p = Diseqcs.Next(p)) {
                  source = oldSrc;
          /* only look at sources configured for this tuner */
                  while (source && source->Next() && p && (p->Tuner() == currentTuner+1 || (TunerIsRotor(currentTuner) && IsWithinConfiguredBorders(currentTuner, source)))) {
                    source = (cSource *) source->Next();
            if(source && (source->Code() == p->Source() || (TunerIsRotor(currentTuner) && IsWithinConfiguredBorders(currentTuner, source)))) {
                      *value = source->Code();
                      found = true;
                      break;
                    }
                  }
               }
            } else {
              if (source && source->Next()) {
                source = (cSource *) source->Next();
                *value = source->Code();
              }
            }
        }
        else
            return state;       // we don't call cMenuEditIntItem::ProcessKey(Key) here since we don't accept numerical input

        if(!found)
           source = Sources.Get(oldCode);

        Set();
        state = osContinue;
    }
    return state;
}

//-----------------------------------------------------
//----------cMenuNCRotor-------------------------------
//-----------------------------------------------------

cMenuNCRotor::cMenuNCRotor(const char *title)
:cOsdMenu(title), srcCode_(0), lastSrcCode_(0), transpNum_(0), tuner_(0), 
snr_(0), lastSnr_(0), str_(0), lastStr_(0), diff_(0.0), 
posItem_(0), scanItem_(0), diffItem_(0), transponderItem_(0), snrItem_(0), strItem_(0)
{
    SetCols(12);
    const char *Title = tr("Rotor settings");
    SetTitle(Title);

    srcCode_ = GetCurrentSource();
    if(srcCode_ == -1)
    {
        srcCode_ = Sources.First()->Code();
    }

    tuner_ = GetTuner();
    //printf("-------tuner_ = %d------\n", tuner_);
    if(tuner_ == -1)
    {
        Skins.Message(mtInfo, tr("no rotor available")); 
        cRemote::Put(kBack);
    }
    else
    {     
        char pos[8];
        if(GetCurrentRotorPosition(tuner_, pos) && std::string(pos) != "S180.0W")
        {
            srcCode_ = GetSourceByPos(pos);
        }
        else
        {
            srcCode_ = GetDefaultSource();
        } 
        Set(true);
    }
}

void cMenuNCRotor::ReSet(bool first)
{ 
    if(tuner_ == -1)
    {
        return;
    }
    
    cTransponder::LoadTpl(cTransponder::TplFileName(srcCode_), transponders_);

    if(transponders_.size())
    {  
        posItem_->SetText(tr("Go to position")); 
        posItem_->SetSelectable(true);
        tpStringVec_.resize(transponders_.size());
        for(uint i = 0; i < transponders_.size(); ++i)
        {
            tpStringVec_[i] = transponders_[i]->GetInfoString().c_str();
        }
        transponderItem_->ReSet(tr("Transponders"), &transpNum_, transponders_.size(), &tpStringVec_[0]);
        transponderItem_->SetSelectable(true);
    }  
    else
    {
        posItem_->SetText("");
        transponderItem_->SetSelectable(false);
        transponderItem_->SetText("");
        transponderItem_->SetSelectable(false);
    }

    if(transponders_.size())
    {
        scanItem_->SetText(tr("Perform transponder scan"));
        scanItem_->SetSelectable(true);
    }
    else
    {
        scanItem_->SetText("");
        scanItem_->SetSelectable(false);
    }

    /*if(first)
    {
        UpdateSnr(true);    
        UpdateStr(true);
    }*/
 
    Display();
}

void cMenuNCRotor::Set(bool first)
{ 
    //printf("-----cMenuNCRotor::Set---------, first = %d-----\n", first);
    Add(new cOsdItem(tr("Please select the rotor position"), osUnknown, false));

    cMyMenuEditSrcItem *item = new cMyMenuEditSrcItem(tr("Satellite"), &srcCode_, tuner_);
    if(!IsWithinConfiguredBorders(tuner_, Sources.Get(srcCode_)))
    {
        //item->Next();
    } 
    Add(item);
    //printf("IsWithinConfiguredBorders(currentTuner, source) = %d\n", IsWithinConfiguredBorders(tuner_, Sources.Get(srcCode_)));

    cTransponder::LoadTpl(cTransponder::TplFileName(srcCode_), transponders_);

    posItem_ = new cOsdItem(tr("Go to position"));
    Add(posItem_);
    Add(new cOsdItem("", osUnknown, false));
    tpStringVec_.resize(transponders_.size());
    for(uint i = 0; i < transponders_.size(); ++i)
    {
        tpStringVec_[i] = transponders_[i]->GetInfoString().c_str();
    }
    transponderItem_ = new cMyMenuEditStraItem(tr("Transponders"), &transpNum_, transponders_.size(), &tpStringVec_[0]);
    Add(transponderItem_);

    if(!transponders_.size())
    {
        posItem_->SetText("");
        transponderItem_->SetText("");
    }

    scanItem_ = new cOsdItem(tr("Perform transponder scan"));
    Add(scanItem_);

    if(!transponders_.size())
    {
        scanItem_->SetText("");
    }
  
    Add(new cOsdItem("", osUnknown, false));     
    Add(new cOsdItem("", osUnknown, false));
    Add(new cOsdItem("", osUnknown, false));    
    Add(new cOsdItem("", osUnknown, false));
    Add(new cOsdItem("", osUnknown, false)); 

    diffItem_ = new cOsdItem("", osUnknown, false);
    Add(diffItem_);
    UpdateStatus();

    Add(new cOsdItem("", osUnknown, false));
 
    lockItem_ = new cOsdItem("", osUnknown, false);  
    Add(lockItem_); 
    UpdateLock();   

    strItem_ = new cMenuSignalItem(tr("STR")); 
    Add(strItem_);
    UpdateStr(true);

    snrItem_ = new cMenuSignalItem(tr("SNR")); 
    Add(snrItem_);
    UpdateSnr(true);    

    Display();
}

eOSState cMenuNCRotor::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);

    if (tuner_ != -1 && lastSrcCode_ != srcCode_)
    {
        lastSrcCode_ = srcCode_;
        transponders_.clear();
        tpStringVec_.clear();  
        ReSet(false);
    }
 
    static int num = 0;
    //if(++num%5 == 0)
    if (tuner_ != -1)
    {   
        UpdateStr(true);
        UpdateSnr(true);
        UpdateStatus(true);
        UpdateLock();
    }

    switch (Key)
    {
        case kBack: 
            return osBack;

        case kOk:  
            if(std::string(Get(Current())->Text()) == tr("Go to position"))
            {
                num = 0; //force Snr/Str update next time
                return MoveRotor();
            }
            else if(std::string(Get(Current())->Text()) == tr("Perform transponder scan"))
            {
                return PerformChannelScan();
            }
            break;

        default:
            break;
    }
    //PrintStatus();

    return state;
}

eOSState cMenuNCRotor::MoveRotor()
{  
    //printf("----------MoveRotor-------\n");
    int cardnr = tuner_;
    cDevice *device = cDevice::GetDevice(cardnr);
    //printf("--device = %d, device->ProvidesSource = %d, transponders_.size() = %d--\n", device, device->ProvidesSource(cSource::stSat),transponders_.size() );
    if (device && device->ProvidesSource(cSource::stSat) && transponders_.size())
    {
        /*printf("\n\n####cMenuNCRotor::MoveRotor src = %d, freq = %d, pol = %c, symrate = %d, coderate = %d\n", 44, srcCode_, transponders_[transpNum_]->GetFreq(),
        1111,
        *(transponders_[transpNum_]->GetPol().c_str()), 
        transponders_[transpNum_]->GetSymRate(),  
        transponders_[transpNum_]->GetCodeRate());
        333);*/

        cChannel dummyChan;
        dummyChan.SetNumber(0);
        dummyChan.SetSatTransponderData(srcCode_, 
                                        transponders_[transpNum_]->GetFreq(), 
                                        *(transponders_[transpNum_]->GetPol().c_str()),
                                        transponders_[transpNum_]->GetSymRate(),
                                        transponders_[transpNum_]->GetCodeRate());            

        int Vpid = 1;
        //int Ppid = pmt.getPCRPid();
        int Apids[MAXAPIDS + 1] = { 0 };
        int Dpids[MAXDPIDS + 1] = { 0 };
        int Spids[MAXSPIDS + 1] = { 0 };

        char ALangs[MAXAPIDS + 1][MAXLANGCODE2] = { "" };
        char DLangs[MAXDPIDS + 1][MAXLANGCODE2] = { "" };
        char SLangs[MAXSPIDS + 1][MAXLANGCODE2] = { "" };

        int Tpid = 0;

        dummyChan. SetPids(Vpid, 0, Apids, ALangs, Dpids, DLangs, Spids, SLangs, Tpid);


        device->SetChannel(&dummyChan, false, false); 
        //cDevice::PrimaryDevice()->SwitchChannel(&dummyChan, true);
        Skins.Message(mtInfo, tr("moving rotor"));
        //WaitForRotorMovement(cardnr);
        return osContinue;
    } 
    return osContinue;
}

int cMenuNCRotor::GetTuner()
{
    for(int tuner = 0; tuner < MAXTUNERS; tuner++)
    {
       // if(TunerIsRotor(tuner))
        //    return tuner;
#if 1
        //TODO: This ones doesn't work well. Fix it!
        cDevice *device = cDevice::GetDevice(tuner);  
        if (device)// && device->Priority() < 0) // omit tuners that are recording
        {
            if (device->ProvidesSource(cSource::stSat))
            { 
                if(TunerIsRotor(tuner))
                    return tuner;
            }
        }
#endif
    }
    return -1;
}

void cMenuNCRotor::UpdateStatus(bool forceUpdate)
{   
    //printf("---------------cMenuNCRotor::UpdateDiff------------------\n");
    rotorStatus rStat;
    GetRotorStatus(tuner_, &rStat);
    //printf("rStat: %i lastRStat: %i\n", rStat, lastRStat);
    switch(rStat) {
        case(R_NOT_MOVING):
            {
                double diff;
                if(GetCurrentRotorDiff(tuner_, &diff))
                {
                    if((rStat == 0 || lastRStat != rStat) || diff != diff_ || std::string(diffItem_->Text()) == "")
                    {    
                        diff_ = diff;
                        char val[8];
                        sprintf(val, "%2.2f", diff_);
                        std::string title = std::string(tr("Status: Rotor stopped. Current deviation: ")) + val + "°"; 
                        //printf("UpdateDiff----title = %s-------\n", title.c_str());
                        diffItem_->SetText(tr(title.c_str()));
                        lastRStat = rStat;
                    }
                }
            }
            break;
        case (R_MOVE_TO_CACHED_POS):
            diffItem_->SetText(tr("Status: Rotor is moving to a cached position"));
            break;
        case (R_MOVE_TO_UNKNOWN_POS):
            diffItem_->SetText(tr("Status: Rotor is moving to an uncached position"));
            break;
        case (R_AUTOFOCUS):
            diffItem_->SetText(tr("Status: Rotor is doing autofocus"));
            break;
        default:
            diffItem_->SetText("");
            break;
    }
}

void cMenuNCRotor::UpdateSnr(bool forceUpdate)
{   
    snr_ = getSNR(tuner_);
    //printf("---------------snr_ = %d--------------------\n", snr_);
    if(lastSnr_ != snr_ || forceUpdate)
    {
        lastSnr_ = snr_;
        snrItem_->Update(snr_);
        Display();
    }
}

void cMenuNCRotor::UpdateStr(bool forceUpdate)
{   
    str_ = getSignalStrength(tuner_);
    //printf("---------------str_ = %d--------------------\n", str_);
    if(lastStr_ != str_|| forceUpdate)
    {
        lastStr_ = str_; 
        strItem_->Update(str_);
        Display();
    }
}     

void cMenuNCRotor::UpdateLock(bool forceUpdate)
{
    fe_status_t status;
    if(GetSignal(tuner_, &status) && (status & FE_HAS_LOCK))
    {
        lockItem_->SetText(tr("Lock: yes"));
    }
    else
    {
        lockItem_->SetText(tr("Lock: no"));
    } 
}

void  cMenuNCRotor::PrintStatus()
{
    rotorStatus rStat;
    GetRotorStatus(tuner_, &rStat);
}

eOSState cMenuNCRotor::PerformChannelScan()
{    
    if(transponders_.size())
    {
        struct ChannelScanData
        {
            int source;
            int frequency;
            char polarization;
            int symbolrate;
            int fec;
        } scanData =
        {
            srcCode_,
            transponders_[transpNum_]->GetFreq(),
            *(transponders_[transpNum_]->GetPol().c_str()),
            transponders_[transpNum_]->GetSymRate(),
            transponders_[transpNum_]->GetCodeRate() 
        };
    
        cPluginManager::CallAllServices("Perform channel scan", &scanData);
        return osEnd;
    }
    return osContinue;
} 

int cMenuNCRotor::GetCurrentSource()
{
    cChannel *currentchannel = Channels.Get(cDevice::CurrentChannel());
    if(currentchannel)
    {
        return currentchannel->Source();
    }
    return -1;
}  

int cMenuNCRotor::GetSourceByPos(const char *pos)
{
    cSource *source = Sources.First();
    while(source != Sources.Last())
    {
        if(strcmp(*cSource::ToString(source->Code()), pos) == 0)
        {
            break;
        }
        source = Sources.Next(source);
    }
    return source->Code();
}    

int cMenuNCRotor::GetDefaultSource()
{
    cSource *source = Sources.First();
    while(source != Sources.Last())
    {
        //printf("---sourcestring = %s--\n",*cSource::ToString(source->Code()));
        if(strrchr(*cSource::ToString(source->Code()), 'E') != 0)
        {
            break;
        }
        source = Sources.Next(source);
    }
    return source->Code();
}
