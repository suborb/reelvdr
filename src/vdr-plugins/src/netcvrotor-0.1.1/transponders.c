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
 *
 ***************************************************************************
 *
 *   transponders.c provides Select Channels(.conf) via service interface
 *   Todo Autoscanner  for install wizard
 *
 ***************************************************************************/

#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
//#include <boost/iostreams/filter/gzip.hpp>
//#include <boost/iostreams/filter/zlib.hpp>

#include "transponders.h"
#include <vdr/plugin.h>
#include <vdr/sources.h>
#ifdef REELVDR
#include <vdr/submenu.h>        // setup::
#endif
#include <linux/dvb/frontend.h>


//#define DEBUG_TRANSPONDER(format, args...) printf (format, ## args)
#define DEBUG_TRANSPONDER(format, args...)
#define DEBUG_printf(format, args...) printf (format, ## args)

using
    std::cerr;
using
    std::endl;
using
    std::string;
using
    std::ifstream;
using
    std::ofstream;
using
    std::stringstream;
using
    std::cout;
using
    std::vector;

/* Notation for Europe (region 0)
   channel 1-4: VHF band 1
    5-13:   VHF band 3
   21-69:   UHF band 4/5
   101-110/111-120: cable frequencies (aka Sonderkanal/Midband/Superband S1-S20)
   121-141: cable frquencies (aka Hyperband, S21-S41)
   173/181: D73 and D81 in C-networks (Germany)
*/

//----------- Class Transponder -------------------------------


cTransponder::cTransponder(int Frequency):
channelNum_(0),
frequency_(Frequency),
symbolrate_(0),
scanned_(false)
{
};

int
cTransponders::channel2Frequency(int region, int channel, int &bandwidth)
{
    bandwidth = BANDWIDTH_7_MHZ;

    if (region == 0)
    {                           /// are there others ?
        if (channel >= 1 && channel <= 4)
        {
            return 38000000 + (channel - 1) * 7000000;
        }
        if (channel >= 5 && channel <= 13)
        {
            return 177500000 + 7000000 * (channel - 5);
        }
        if (channel >= 21 && channel <= 69)
        {
            bandwidth = BANDWIDTH_8_MHZ;
            return 474000000 + 8000000 * (channel - 21);
        }
        if (channel == 101)
            return 107500000;

        if (channel == 102 || channel == 103)
        {
            bandwidth = BANDWIDTH_8_MHZ;
            return 113000000 + 8000000 * (channel - 102);
        }
        if (channel >= 104 && channel <= 110)
            return 128000000 + 7000000 * (channel - 104);       // Fixme +500khz Offset?

        if (channel >= 111 && channel <= 120)
            return 233000000 + 7000000 * (channel - 111);       // Fixme +500khz Offset?

        if (channel >= 121 && channel <= 141)
        {
            bandwidth = BANDWIDTH_8_MHZ;
            return 306000000 + 8000000 * (channel - 121);
        }
        if (channel == 173)
        {
            bandwidth = BANDWIDTH_8_MHZ;
            return 73000000;
        }
        if (channel == 181)
        {
            bandwidth = BANDWIDTH_8_MHZ;
            return 81000000;
        }

    }
    return 0;
}


int
cTransponder::IntToFec(int val)
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

int
cTransponders::StatToS2Fec(int mod)
{

    int stat2fecs2[] = {
        999,                    // FEC_AUTO,
        12,                     // FEC_1_2,
        13,                     // FEC_1_3, //S2
        14,                     // FEC_1_4, //S2
        23,                     // FEC_2_3,
        25,                     // FEC_2_5, //S2
        34,                     // FEC_3_4,
        35,                     // FEC_3_5, //S2
        45,                     // FEC_4_5,
        56,                     // FEC_5_6,
        67,                     // FEC_6_7,
        78,                     // FEC_7_8,
        89,                     // FEC_8_9,
        910,                    // FEC_9_10,//S2
    };
    /*int stat2fecs2[] = {
        FEC_AUTO,
        FEC_1_2,
        FEC_1_3, //S2
        FEC_1_4, //S2
        FEC_2_3,
        FEC_2_5, //S2
        FEC_3_4,
        FEC_3_5, //S2
        FEC_4_5,
        FEC_5_6,
        FEC_6_7,
        FEC_7_8,
        FEC_8_9,
        FEC_9_10,//S2
    };*/

    DEBUG_printf(" cTransponders::StatToSFec(%d) return %d \n", mod,
           stat2fecs2[mod]);
    return stat2fecs2[mod];

}

int
cTransponders::StatToS2Mod(int mod)
{
    switch (mod)
    {
    case 0:
        return 0;               // auto  (SD only)
    case 1:
        return 0;//QPSK_S2; !!!!!!!!!!
    case 2:
        return 0;//PSK8;!!!!!!!!!!
    case 3:
        return VSB_8;
    case 4:
        return VSB_16;
    case 5:
        return 999;             // try all
    }
    return 0;//PSK8;!!!!!!!!!!
}

bool LoadTpl(const string & tplFileName, std::vector < cTransponder * > &transponders)
{
    DEBUG_TRANSPONDER(DBGT "LoadSatTpl  %s\n", tplFileName.c_str());

    string tplFileNameComp(tplFileName + ".bz2");
    ifstream transponderList(tplFileNameComp.c_str(),
                             std::ios_base::in | std::ios_base::binary);
    stringstream buffer;

    if (!transponderList)
    {
        esyslog
            ("ERROR: [channelscan] can`t open LoadSatTpls %s --- try uncompressed ",
             tplFileNameComp.c_str());

        transponderList.close();
        transponderList.clear();
        transponderList.open(tplFileName.c_str());
        if (!transponderList)
        {
            esyslog("ERROR: [channelscan] can`t open LoadSatTpls %s",
                    tplFileName.c_str());
            return false;
        }
        boost::iostreams::copy(transponderList, buffer);
    }
    else
    {
        // TODO chose decompressor by file name ending
        boost::iostreams::filtering_istreambuf in;
        in.push(boost::iostreams::bzip2_decompressor());

        // in.push(boost::iostreams::gzip_decompressor());
        // in.push(boost::iostreams::zlib_decompressor());

        in.push(transponderList);
        boost::iostreams::copy(in, buffer);
    }

    string line;
    int lc = 0;

    while (!buffer.eof())
    {

        getline(buffer, line);

        if (line.find('[') == line.npos)
        {

            cSatTransponder *t = new cSatTransponder();

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

//----------- Class cSatTransponder -------------------------------

cSatTransponder::cSatTransponder():cTransponder(0), pol_(' '), fec_(9)
{
    modulation_ = 0;
}

cSatTransponder::cSatTransponder(int Frequency, char Pol, int SymbolRate, int Modulation, int FEC, int RollOff, int ModSystem):cTransponder(Frequency), pol_(Pol), fec_(FEC),
rolloff_
(RollOff)
{
    channelNum_ = 0;
    modulation_ = Modulation;
    symbolrate_ = SymbolRate;

    DEBUG_TRANSPONDER(DBGT
                      " new cSatTransponder(f: %d,p: %c,sRate: %d  mod:%d ,fec %d\n",
                      frequency_, pol_, symbolrate_, modulation_, fec_);
}

bool
cSatTransponder::SetTransponderData(cChannel * c, int Code)
{
    DEBUG_TRANSPONDER(DBGT
                      " SetSatTransponderData(source:%d,f:%6d,p:%c,sRate:%d,mod%3d,fec%d \n",
                      Code, frequency_, pol_, symbolrate_, modulation_, fec_);
    return c->SetSatTransponderData(Code, frequency_, pol_, symbolrate_, fec_,
                                    modulation_, rolloff_);
}


bool
cSatTransponder::Parse(const string & Line)
{

    DEBUG_TRANSPONDER(" %s  %s \n", __PRETTY_FUNCTION__, Line.c_str());
    string tpNumb(Line);

    int index = tpNumb.find_first_of('=');
    if (index == -1)
        return false;

    tpNumb = tpNumb.erase(0, index + 1);

    //   chop  string
    string tmp = Line.substr(index + 1);

    // get Frequenz
    index = tmp.find_first_of(',');

    if (index == -1)
        return false;

    string freq = tmp;
    freq.erase(index);

    // get polarisation
    string polar = tmp.substr(index + 1);
    index = polar.find_first_of(',');
    if (index == -1)
        return false;

    // get symbol rate
    string symRate = polar.substr(index + 1);
    polar.erase(index);

    index = symRate.find_first_of(',');
    if (index == -1)
        return false;

    string sFec = symRate.substr(index + 1);
    symRate.erase(index);

    channelNum_ = strToInt(tpNumb.c_str());
    frequency_ = strToInt(freq.c_str());
    if (frequency_ == -1)
        return false;
    pol_ = polar[0];
    symbolrate_ = strToInt(symRate.c_str());
    if (symbolrate_ == -1)
        return false;
    fec_ = IntToFec(strToInt(sFec.c_str()));

    //DEBUG_TRANSPONDER(" transp.c Parse()  return true f:%d p%c sRate %d fec %d \n", frequency_,pol_,symbolrate_,fec_);
    // dsyslog (" transp.c Parse()  return true f:%d p%c sRate %d fec %d ", frequency_,pol_,symbolrate_,fec_);

    return true;
}

//----------- Class cTerrTransponder -------------------------------

cTerrTransponder::cTerrTransponder(int ChannelNr, int Frequency, int Bandwith):cTransponder
    (Frequency)
{
    channelNum_ = ChannelNr;
    symbolrate_ = 27500;
    bandwidth_ = Bandwith;
    // fec is called Srate in vdr
    fec_h_ = FEC_AUTO;
    fec_l_ = FEC_AUTO;
    hierarchy_ = HIERARCHY_NONE;
    modulation_ = FE_OFDM;
    guard_ = GUARD_INTERVAL_AUTO;
    transmission_ = TRANSMISSION_MODE_AUTO;
}

cTerrTransponder::~cTerrTransponder()
{
}

bool
cTerrTransponder::SetTransponderData(cChannel * c, int Code)
{

    int type = cSource::stTerr;

    if (bandwidth_ == 0)
    {
        if (frequency_ >= 306 * 1000 * 1000)
            bandwidth_ = BANDWIDTH_8_MHZ;
        else
            bandwidth_ = BANDWIDTH_7_MHZ;
    }
    else if (bandwidth_ == 1)
        bandwidth_ = BANDWIDTH_7_MHZ;
    else
        bandwidth_ = BANDWIDTH_8_MHZ;

    return c->SetTerrTransponderData(type, frequency_, bandwidth_, modulation_, hierarchy_, fec_h_, fec_l_, guard_, transmission_);     //

}

//----------- Class cCableTransponder -------------------------------

cCableTransponder::cCableTransponder(int ChannelNr, int Frequency, int Bandwith, int sRate, int Mod):cTransponder
    (Frequency)
{
    int
        modTab[6] = {
        QAM_64,                 //Auto QAM64/QAM256
        QAM_256,
        QPSK,
        QAM_16,
        QAM_32,
        QAM_128
    };
    // QAM_64 is Auto QAM64/QAM256

    DEBUG_TRANSPONDER(DBGT
                      " new cCableTransponder Channel: %d f: %d, sRate :%d  BW %d ",
                      ChannelNr, Frequency, sRate, Bandwith);

    channelNum_ = ChannelNr;
    symbolrate_ = sRate;
    bandwidth_ = Bandwith;

    fec_h_ = FEC_AUTO;
    hierarchy_ = HIERARCHY_NONE;

    modulation_ = modTab[Mod];
}

cCableTransponder::~cCableTransponder()
{
}

bool
cCableTransponder::SetTransponderData(cChannel * c, int Code)
{
    int type = cSource::stCable;

    DEBUG_TRANSPONDER(DBGT
                      " SetCableTransponderData(f:%d, m :%d ,sRate: %d, fec %d",
                      frequency_, modulation_, symbolrate_, fec_h_);
    return c->SetCableTransponderData(type, frequency_, modulation_,
                                      symbolrate_, fec_h_);
}

//----------- Class Transponders -------------------------------

cTransponders::cTransponders():sourceCode_(0)
{
}

void
cTransponders::Load(cScanParameters * scp)
{

    DEBUG_TRANSPONDER(DBGT "  %s \n", __PRETTY_FUNCTION__);
    Clear();

    sourceCode_ = scp->source;

    if (scp->type == SAT || scp->type == SATS2)
    {
        lockMs_ = 500;

        if (scp->frequency < 5) //  auto
        {                       // try to start with nit scan  on ASTRA or HOTBIRD
            if (!LoadNitTransponder(sourceCode_) || scp->frequency == 2)
            {
                fileName_ = TplFileName(sourceCode_);
                position_ = SetPosition(fileName_);
                // load old single SAT transponder list
                LoadTpl(fileName_);
            }
        }
        else
        {                       // manual single transponder

            if (scp->type == SATS2)
            {
                scp->modulation = StatToS2Mod(scp->modulation);
                scp->fec = StatToS2Fec(scp->fec);
                // TODO explain
            }

            DEBUG_TRANSPONDER(DBGT
                              " Load single SatTransponderData(f:\"%d\", pol[%c] -- symbolRate  %d,  modulation %d  fec %d",
                              scp->frequency, scp->polarization,
                              scp->symbolrate, scp->modulation, scp->fec);
            cSatTransponder *t =
                new cSatTransponder(scp->frequency, scp->polarization,
                                    scp->symbolrate, scp->modulation,
                                    scp->fec);
            printf("%s:%i new cSatTransponder created with fec %i\n", __FILE__, __LINE__, scp->fec);

            fileName_ = TplFileName(sourceCode_);
            position_ = SetPosition(fileName_); // need this to display Satellite info
	    fileName_.clear(); // donot load transponder list during manual scan
	    // what about description_ ? //TODO
            
	    v_tp_.push_back(t);
        }

    }
    else if (scp->type == TERR)
    {
        position_ = "Terrestrial";

        if (scp->frequency == 1)
            CalcTerrTpl();
        else
        {
            int channel = 0;
            cTerrTransponder *t =
                new cTerrTransponder(channel, scp->frequency * 1000,
                                     scp->bandwidth);
            v_tp_.push_back(t);
        }
    }
    else if (scp->type == CABLE)
    {
        position_ = "Cable";
        if (scp->frequency == 1)
            CalcCableTpl(0, scp);
        else
        {
            int channel = 0;
            int sRate;
            if (scp->symbolrate_mode == 2)      /// XXX check this markus
                sRate = scp->symbolrate;
            else
                sRate = 6900;

            cCableTransponder *t =
                new cCableTransponder(channel, scp->frequency * 1000,
                                      scp->
                                      bandwidth ? BANDWIDTH_7_MHZ :
                                      BANDWIDTH_8_MHZ,
                                      sRate, scp->modulation);
            v_tp_.push_back(t);
        }
    }
    else
        esyslog(DBGT "   Wrong  sourceCode %d", sourceCode_);

    DEBUG_TRANSPONDER(DBGT "  %s end \n", __PRETTY_FUNCTION__);
}

void
cTransponders::Add(int Source, const cScanParameters & scp)
{
    // type S/T/C
    sourceCode_ = Source;

    if (cSource::IsSat(sourceCode_))
    {
        cSatTransponder *t =
            new cSatTransponder(scp.frequency, scp.polarization,
                                scp.symbolrate, scp.modulation, scp.fec);
        v_tp_.push_back(t);
    }

}
bool
cTransponders::LoadNitTransponder(int Source)
{

    DEBUG_TRANSPONDER(DBGT " %s ... \"%d\" \n", __PRETTY_FUNCTION__, Source);

    int found = 0;

    position_ = *cSource::ToString(Source);

    // generate filelist
    vector < string > fileList;
    fileList.push_back(TplFileName(0));
    fileList.push_back(TplFileName(0) + ".bz2");
    fileList.push_back(TplFileName(1));
    fileList.push_back(TplFileName(1) + ".bz2");

    stringstream buffer;

    for (consStrIter it = fileList.begin(); it != fileList.end(); ++it)
    {
        if ((*it).find(".bz2") != std::string::npos)
        {
            ifstream transponderList((*it).c_str(),
                                     std::ios_base::in | std::ios_base::
                                     binary);

            if (transponderList.fail())
            {
                DEBUG_TRANSPONDER(DBGT
                                  " can not load %s  try to load next list\n",
                                  (*it).c_str());

                transponderList.close();
                transponderList.clear();
                continue;
            }
            boost::iostreams::filtering_istreambuf in;
            in.push(boost::iostreams::bzip2_decompressor());

            // in.push(boost::iostreams::gzip_decompressor());
            // in.push(boost::iostreams::zlib_decompressor());

            in.push(transponderList);
            boost::iostreams::copy(in, buffer);
            boost::iostreams::copy(transponderList, buffer);
        }
        else
        {
            ifstream transponderList((*it).c_str(), std::ios_base::in);

            if (transponderList.fail())
            {
                DEBUG_TRANSPONDER(DBGT
                                  " can not load %s  try to load next list\n",
                                  fileName_.c_str());

                transponderList.close();
                transponderList.clear();
                continue;
            }

            boost::iostreams::copy(transponderList, buffer);

        }
    }

    int lc = 0;

    while (buffer.good() && !buffer.eof())
    {
        lc++;

        string line;
        getline(buffer, line);

        // skip lines with #
        if (line.find('#') == 0)
        {
            continue;
        }

        if (line.find(position_) == 0)
        {

            // second entry support
            if (found == lc - 1)
            {
                cSatTransponder *t = new cSatTransponder();
                if (t->Parse(line))
                {
                    v_tp_.push_back(t);
                }
            }
            else
            {

                found = lc;
                nitStartTransponder_.reset(new cSatTransponder);
                nitStartTransponder_->Parse(line);
                DEBUG_TRANSPONDER(DBGT " found first  entry  %d: %s \n", lc,
                                  line.c_str());
            }
        }
        else
        {
            if (found > 0 && found == lc - 1)
            {
                DEBUG_TRANSPONDER(DBGT " return true  found: %d: lc:%d \n",
                                  found, lc);
                return true;
            }
        }
    }

    if (!found)
    {
        esyslog("ERROR: [channelscan] in  %s :  no values for \"%s\"",
                fileName_.c_str(), position_.c_str());
    }

    return found;
}

bool 
cTransponders::LoadTpl(const string & tplFileName)
{

    lockMs_ = 500;
    DEBUG_TRANSPONDER(DBGT "LoadSatTpl  %s\n", tplFileName.c_str());

    sourceCode_ = cSource::FromString(position_.c_str());

    string tplFileNameComp(tplFileName + ".bz2");
    ifstream transponderList(tplFileNameComp.c_str(),
                             std::ios_base::in | std::ios_base::binary);
    stringstream buffer;

    if (!transponderList)
    {
        esyslog
            ("ERROR: [channelscan] can`t open LoadSatTpls %s --- try uncompressed ",
             tplFileNameComp.c_str());

        transponderList.close();
        transponderList.clear();
        transponderList.open(tplFileName.c_str());
        if (!transponderList)
        {
            esyslog("ERROR: [channelscan] can`t open LoadSatTpls %s",
                    tplFileName.c_str());
            return false;
        }
        boost::iostreams::copy(transponderList, buffer);
    }
    else
    {
        // TODO chose decompressor by file name ending
        boost::iostreams::filtering_istreambuf in;
        in.push(boost::iostreams::bzip2_decompressor());

        // in.push(boost::iostreams::gzip_decompressor());
        // in.push(boost::iostreams::zlib_decompressor());

        in.push(transponderList);
        boost::iostreams::copy(in, buffer);
    }

    string line;
    int lc = 0;

    while (!buffer.eof())
    {

        getline(buffer, line);

        if (line.find('[') == line.npos)
        {

            cSatTransponder *t = new cSatTransponder();

            if (t->Parse(line))
            {
                v_tp_.push_back(t);
            }
        }
        lc++;
    }

    transponderList.close();

    return true;
}

void
cTransponders::CalcCableTpl(bool Complete, cScanParameters * scp)
{
    int bandwidth;
    int f, channel = 0;
    int sRate, qam;

    Clear();
    /// refactor: mov for "try all" scan all oher values to here

    if (scp->symbolrate_mode == 2)      /// check this markus
        sRate = scp->symbolrate;
    else
        sRate = 6900;

    qam = scp->modulation;

    // Give the user the most popular cable channels first, speeds up SR auto detect
    for (channel = 121; channel < 200; channel++)
    {
        f = channel2Frequency(0, channel, bandwidth);
        if (f)
        {
            cCableTransponder *t =
                new cCableTransponder(channel, f, bandwidth, sRate, qam);
            v_tp_.push_back(t);
        }
    }

    for (channel = 101; channel < 121; channel++)
    {
        f = channel2Frequency(0, channel, bandwidth);
        if (f)
        {
            cCableTransponder *t =
                new cCableTransponder(channel, f, bandwidth, sRate, qam);
            v_tp_.push_back(t);
        }
    }

    for (channel = 1; channel < 100; channel++)
    {
        f = channel2Frequency(0, channel, bandwidth);
        if (f)
        {
            cCableTransponder *t =
                new cCableTransponder(channel, f, bandwidth, sRate, qam);
            v_tp_.push_back(t);
        }
    }

}

void
cTransponders::CalcTerrTpl()
{
    Clear();
    int f;
    int channel;
    int bandwidth;

    position_ = "Terrestrial";

    for (channel = 5; channel <= 69; channel++)
    {
        f = channel2Frequency(0, channel, bandwidth);
        if (f)
        {
            cTerrTransponder *t = new cTerrTransponder(channel, f, bandwidth);
            v_tp_.push_back(t);
        }
    }
}

string
cTransponders::SetPosition(const string & tplPath)
{
    if (cSource::IsSat(sourceCode_))
    {
        string tmp = fileName_.substr(tplPath.find_last_of('/') + 1);
        int index = tmp.find_last_of('.');
        tmp.erase(index);
        return tmp;
    }
    if (cSource::IsTerr(sourceCode_))
        return "DVB-T";

    if (cSource::IsCable(sourceCode_))
        return "DVB-C";

    return "";
}

string
cTransponders::TplFileName(int satCodec)
{
    string tmp = cPlugin::ConfigDirectory();
    tmp += "/transponders/";

    if (satCodec == 0)
    {
        //tmp += "SatNitScan";
        tmp += "Sat";
        isyslog("INFO [channelscan]: Please update Transponder lists ");
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

void
cTransponders::Clear()
{
    for (TpIter iter = v_tp_.begin(); iter != v_tp_.end(); ++iter)
    {
        delete *iter;
        *iter = NULL;
    }

    v_tp_.clear();
}

bool
cTransponders::MissingTransponder(int Transponder)
{
    for (constTpIter iter = v_tp_.begin(); iter != v_tp_.end(); ++iter)
    {
        if (Transponder == (*iter)->Frequency())
            return false;
    }
    return true;
}

#ifndef REELVDR

namespace setup
{
    // TEST this !!
    string FileNameFactory(string FileType)
    {
        string configDir = cPlugin::ConfigDirectory();
          string::size_type pos = configDir.find("plugin");
          configDir.erase(pos - 1);


        if (FileType == "configDirecory")       // vdr config base directory
        {
            DEBUG_TRANSPONDER(DBGT " Load configDir %s  \n",
                              configDir.c_str());
        }
        else if (FileType == "setup")   // returns plugins/setup dir; change to  "configDir"
        {
            configDir += "/plugins/setup";
        }

        DEBUG_TRANSPONDER(DBGT " Load configDir %s  \n", configDir.c_str());
        return configDir;
    }
}

#endif
