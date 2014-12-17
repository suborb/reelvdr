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

 
#ifndef P__NETCVMENU_H
#define P__NETCVMENU_H

#include <vector>
#include <string>
#include <sstream>

uint16_t getSNR();
int strToInt(const std::string & s);

  
//----------cTransponder-----(taken from channelscan plugin)--------------

class cTransponder
{
    private:
    std::string infoString_;
    std::string pol_;
    int frequency_;
    int channelNum_;
    int symbolrate_; 
    int coderate_;

    public:
    static bool LoadTpl(const std::string &tplFileName, std::vector <cTransponder *> &transponders);
    static std::string TplFileName(int satCodec);
    bool Parse(const std::string & Line);
    std::string &GetInfoString()  { return infoString_; }
    int GetFreq() { return frequency_; } 
    int GetSymRate() { return symbolrate_;} 
    int GetCodeRate() { return coderate_; }
    std::string GetPol() {return pol_; }
};

// ---------cMenuSignalItem--------------------

class cMenuSignalItem: public cOsdItem
{
    private:
    int value_; 
    char szProgressPart[12];
    std::string title_;
    std::string titleBuf_;
    public:
    cMenuSignalItem(std::string title);
    /*override*/ void Set();
    void Update(int value);
};

// ---------cMyMenuEditStraItem--------------------

class cMyMenuEditStraItem : public cMenuEditIntItem 
{
    private:
    const char * const *strings;
    protected:
    /*override*/ void Set(void);
    public:
    cMyMenuEditStraItem(const char *Name, int *Value, int NumStrings, const char * const *Strings);
    void ReSet(const char *Name, int *Value, int NumStrings, const char * const *Strings);
};

//----------cMenuNCRotor-----------------------

class cMenuNCRotor : public cOsdMenu
{
public:
    cMenuNCRotor(const char *title);
    virtual ~cMenuNCRotor(){}; 
         
private:
    //forbid copying
    cMenuNCRotor(const cMenuNCRotor &);
    cMenuNCRotor &operator=(const cMenuNCRotor &);
    
    virtual eOSState ProcessKey(eKeys Key);
    void Set(bool first = false);
    void ReSet(bool first = false);
    eOSState MoveRotor();
    int GetTuner();
    void UpdateSnr(bool forceUpdate = false);
    void UpdateStr(bool forceUpdate = false);
    void UpdateStatus(bool forceUpdate = false);
    void UpdateLock(bool forceUpdate = false);
    void PrintStatus();
    eOSState PerformChannelScan();
    int GetCurrentSource();
    int GetSourceByPos(const char *pos);
    int GetDefaultSource();

    int srcCode_;
    int lastSrcCode_;
    int transpNum_;
    int tuner_;
    int snr_;
    char snrbuf_[256];
    int lastSnr_;    
    int str_;
    char strbuf_[256];
    int lastStr_;
    double diff_;
    int lastRStat;
    std::vector < cTransponder * > transponders_;
    std::vector<const char*> tpStringVec_;
    cOsdItem *posItem_; 
    cOsdItem *scanItem_;
    cOsdItem *diffItem_;
    cOsdItem *lockItem_;
    cMyMenuEditStraItem *transponderItem_;
    cMenuSignalItem *snrItem_;
    cMenuSignalItem *strItem_;
};

//----------cMenuNCRotorFunctions--------------
class cMenuNCRotorFunctions : public cOsdMenu
{
    public:
        cMenuNCRotorFunctions(const char *Title);
        ~cMenuNCRotorFunctions();

        void Set();
        eOSState ProcessKey(eKeys Key);
};

#endif //P__NETCVMENU_H

