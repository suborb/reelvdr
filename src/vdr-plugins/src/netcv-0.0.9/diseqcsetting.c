/***************************************************************************
 *   Copyright (C) 2007 by Reel Multimedia;  Author:  Florian Erfurth      *
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
 *                                                                         *
 ***************************************************************************
 *
 * diseqcsetting.c
 *
 ***************************************************************************/

#include "diseqcsetting.h"
#include "diseqcsettingitem.h"
#include <math.h>
#include <vdr/plugin.h>
#include <vdr/config.h>

#define MAX_PORT 16
#define DISEQC_1_1_MAXLNB 64
#define MAX_SATLIST 32

static const char* allowedChars = " .,:;+-*~?!\"$%&/\()=`'_abcdefghijklmnopqrstuvwxyz0123456789";

const char* LNB_Type_string[] = { "5150 Mhz", "9750 Mhz", "10600 Mhz", "11250 Mhz", "11475 Mhz", "9750/10600 MHz", "10750/11250 Mhz" };
const int LNB_Frequency[][2] =
{
    { 5150, 0 },
    { 9750, 0 },
    { 10600, 0 },
    { 11250, 0 },
    { 11475, 0 },
    { 9750, 10600 },
    { 10750, 11250 }
};

// class cLNBSetting --------------------------------------------------
cLNBSetting::cLNBSetting()
{
    iSource_ = DISEQC_SOURCE_ASTRA_19;
    iLNB_Type_ = 5;
}

cLNBSetting::~cLNBSetting()
{
}

std::string cLNBSetting::GetSatPosition()
{
    float fNumber = 0;
    int iNumber = 0;
    int fraction = 0;
    char buf[8];
    char Direction;
    strcpy(buf, *cSource::ToString(iSource_));

    for(int i = 1; buf[i]; ++i)
    {
        if(!((buf[i] >= '0' && buf[i] <= '9') || buf[i] == '.'))
            break;
        if(buf[i]=='.')
        {
            ++fraction;
            continue;
        }
        if (fraction)
        {
            fNumber += ((float)buf[i]-'0') / pow(10, fraction);
            ++fraction;
        }
        else
            fNumber = fNumber * 10 + buf[i]-'0';
    }

    Direction = buf[strlen(buf) - 1];

    if(Direction == 'E')
        iNumber = (int)(1800 + fNumber * 10);
    else if(Direction == 'W')
        iNumber = (int)(1800 - fNumber * 10);

    return std::string(itoa(iNumber));
}

int cLNBSetting::SetSource(const char *SatName)
{
    int Source = cSource::FromString(SatName);
    if(Source == cSource::stNone)
        return 1; // Source not found
    iSource_ = Source;

    return 0;
}

int cLNBSetting::SetLNBType(satellite_info_t *sat)
{
    int LowLOF, HighLOF;
    satellite_component_t *comp = sat->comp;
    int count = sizeof(LNB_Type_string)/sizeof(char*);

    LowLOF = comp->LOF;
    comp = sat->comp + 1;
    if(sat->comp_num == 2)
        HighLOF = 0;
    else
        HighLOF = comp->LOF;

    for(int i=0; i<count; ++i)
    {
        if((LNB_Frequency[i][0] == LowLOF) && (LNB_Frequency[i][1] == HighLOF))
        {
            iLNB_Type_ = i;
            return 0;
        }
    }

    return 1; // Type not found
}

std::string cLNBSetting::GetRangeMin(bool HighLOF)
{
    int retValue;
    if (LNB_Frequency[iLNB_Type_][0]==5150) { // IF < LOF
	    retValue=3400;
    }
    else {
	    if(HighLOF)
		    retValue = (LNB_Frequency[iLNB_Type_][1] + LNB_Frequency[iLNB_Type_][0] + 3100) / 2 + 1; // = SLOF
	    else
		    retValue = LNB_Frequency[iLNB_Type_][0] + 950;
    }
    return std::string(itoa(retValue));
}

std::string cLNBSetting::GetRangeMax(bool HighLOF)
{
    int retValue;
    if (LNB_Frequency[iLNB_Type_][0]==5150) { // IF < LOF
	    retValue=4800;
    }
    else {
	    if(HighLOF)
		    retValue = LNB_Frequency[iLNB_Type_][1] + 2150;
	    else
	    {
		    if(LNB_Frequency[iLNB_Type_][1] != 0)
			    retValue = (LNB_Frequency[iLNB_Type_][1] + LNB_Frequency[iLNB_Type_][0] + 3100) / 2; // = SLOF
		    else
			    retValue = LNB_Frequency[iLNB_Type_][0] + 2000;  // 2150 only for high band
	    }
    }
//    printf("MAX H%i, %i\n",HighLOF,retValue);
    return std::string(itoa(retValue));
}

std::string cLNBSetting::GetLOF(bool HighLOF)
{
    return std::string(itoa(LNB_Frequency[iLNB_Type_][HighLOF?1:0]));
}

// class cLNBSettingDiseqc1_1 -----------------------------------------
cLNBSettingDiseqc1_1::cLNBSettingDiseqc1_1()
{
    iSource_ = DISEQC_SOURCE_ASTRA_19;
    iLNB_Type_ = 5;
    iMultiswitch_ = 0;
    iCascade_ = 1;
}

cLNBSettingDiseqc1_1::~cLNBSettingDiseqc1_1()
{
}

// class cSatList -----------------------------------------------------
cSatList::cSatList(const char *name)
{
    if(name)
        Name_ = strndup(name, SATLIST_STRING_LENGTH);
    else
    {
        Name_ = (char *) malloc(SATLIST_STRING_LENGTH * sizeof(char));
        memset (Name_, ' ', SATLIST_STRING_LENGTH * sizeof(char));
        *Name_ = '\0';
    }
    bShowSettings_ = false;
//    DiseqcMode_.push_back(new cDiseqcDisabled()); //TODO: need more information about netceiver.conf
//    DiseqcMode_.push_back(new cDiseqcMini()); //TODO: need more information about netceiver.conf
    DiseqcMode_.push_back(new cDiseqc1_0());
    DiseqcMode_.push_back(new cDiseqc1_1());
//    DiseqcMode_.push_back(new cDiseqc1_2()); //TODO: need more information about netceiver.conf
    DiseqcMode_.push_back(new cGotoX());
    DiseqcMode_.push_back(new cDiseqcSCR());

//    DiseqcMode_.push_back(new cDisicon4()); //TODO: Not implemented in netceiver
    modeIter_ = DiseqcMode_.begin();
}

cSatList::~cSatList()
{
    free(Name_);
}

int cSatList::SetSettings(eDiseqcMode DiseqcMode, satellite_list_t *sat_list)
{
    if(!SetMode(DiseqcMode))
    {
        (*modeIter_)->SetSettings(sat_list);
        return 0;
    }
    else
        return 1; // DiSEqC Mode not supported
}

int cSatList::SetMode(eDiseqcMode DiseqcMode)
{
    for(std::vector<cDiseqcSettingBase*>::iterator modeIter = DiseqcMode_.begin(); modeIter < DiseqcMode_.end(); ++modeIter)
        if((*modeIter)->mode_ == DiseqcMode)
        {
            modeIter_ = modeIter;
            return 0;
        }
    return 1; // DiSEqC Mode not supported
}

eDiseqcMode cSatList::GetMode()
{
    return (*modeIter_)->mode_;
}

void cSatList::BuildMenu(cOsdMenu *menu)
{
    if(bShowSettings_)
    {
        (*modeIter_)->AddMode(menu, &modeIter_, DiseqcMode_.begin(), (DiseqcMode_.end()-1));
        (*modeIter_)->BuildMenu(menu);
    }
}

int cSatList::CheckSetting()
{
    if((*modeIter_)->mode_ == eDiseqc1_1)
    {
        cDiseqc1_1 *Diseqcsetting = static_cast<cDiseqc1_1*>(*modeIter_);
        if(Diseqcsetting->iMultiswitch_==1)
        {
            int Counter[MAX_PORT];
            for(int i=0; i < MAX_PORT; ++i)
                Counter[i] = 0;
            for(int i=0; i < (*modeIter_)->GetNumberOfLNB(); ++i)
                if(++Counter[Diseqcsetting->LNBSettingDiseqc1_1_.at(i)->iCascade_ - 1] > 4)
                    return 1;
        }
    }
    return 0;
}

// class cDiseqcSetting -----------------------------------------------
cDiseqcSetting::cDiseqcSetting(const char *TmpPath, const char *Interface, const char *NetceiverUUID, cNetCVCams *Cams, cNetCVTuners *Tuners)
{
    TmpPath_ = TmpPath;
    Interface_ = Interface;
    NetceiverUUID_ = NetceiverUUID;
    Cams_ = Cams;
    Tuners_ = Tuners;
}

cDiseqcSetting::~cDiseqcSetting()
{
}

void cDiseqcSetting::BuildMenu(cOsdMenu *menu)
{
        if(SatList_.empty())
        {
            std::string strBuff = "         ";
            strBuff.append(tr("empty"));
            menu->Add(new cOsdItem(strBuff.c_str(), osUnknown, true));
        }
        else
            for(std::vector<cSatList*>::iterator satlistIter = SatList_.begin(); satlistIter < SatList_.end(); ++satlistIter)
            {
                std::string strBuff;
                if ((*satlistIter)->bShowSettings_)
                    strBuff = "      -  ";
                else
                    strBuff = "      + ";
                strBuff.append(tr("DiSEqC list"));
                menu->Add(new cSatlistItem(satlistIter, this, strBuff.c_str(), allowedChars));

                if((*satlistIter)->bShowSettings_)
                    (*satlistIter)->BuildMenu(menu);
            }
}

std::string cDiseqcSetting::GenerateSatlistName()
{
    bool found;
    std::string strBuff;
    const char *multiswitch = "Multiswitch ";
    for(int i=1; i<MAX_SATLIST; ++i)
    {
        found = false;
        strBuff = multiswitch;
        strBuff.append(itoa(i));
        for(std::vector<cSatList*>::iterator satlistIter = SatList_.begin(); satlistIter < SatList_.end(); ++satlistIter)
        {
            if((strBuff.length() == strlen((*satlistIter)->Name_)) && (strBuff.find((*satlistIter)->Name_) == 0))
            {
                found = true;
                break;
            }
        }
        if(!found)
            return strBuff;
    }

    strBuff = multiswitch;
    strBuff.append(" X");
    return strBuff;
}

int cDiseqcSetting::CheckSetting()
{
    bool activetuner_found = false;

    for(std::vector<cSatList*>::iterator satlistIter = SatList_.begin(); satlistIter < SatList_.end(); ++satlistIter)
        if((*satlistIter)->CheckSetting())
            return 1; // Settings not conform

    for(std::vector<cSatList*>::iterator satlistIter1 = SatList_.begin(); satlistIter1 < SatList_.end(); ++satlistIter1)
    {
        if(std::string((*satlistIter1)->Name_).size() == 0)
            return 2; // Empty satlist name found
        if(satlistIter1 < SatList_.end()-1)
            for(std::vector<cSatList*>::iterator satlistIter2 = satlistIter1+1; satlistIter2 < SatList_.end(); ++satlistIter2)
                if(strcmp((*satlistIter1)->Name_, (*satlistIter2)->Name_) == 0)
                    return 3; // Duplicate satlist name found
    }

    for(std::vector<cNetCVTuner>::iterator tunerIter = Tuners_->Begin(); tunerIter < Tuners_->End(); ++tunerIter)
    {
        if(!GetSatlistByName(tunerIter->GetSatlist().c_str()) && (tunerIter->GetType().find("DVB-S") == 0))
            return 4; // Tuner without satlist found
        if(tunerIter->GetPreference() >= 0)
            activetuner_found = true;
    }

    if(!activetuner_found)
        return 5;

    return 0;
}

int cDiseqcSetting::AddCamComponent(xmlNodePtr node, NetCVCam *Cam)
{
    char buf[256];
    xmlNodePtr node_ptr, node_ptr1;

    node_ptr1 = xmlNewChild(node, NULL, BAD_CAST "ccpp:component", NULL);
    node_ptr = xmlNewChild (node_ptr1, NULL, BAD_CAST "rdf:Description", NULL);
    xmlNewProp (node_ptr, BAD_CAST "rdf:about", (unsigned char *) "CAM");
    sprintf (buf, "%i", Cam->cam_.slot);
    xmlNewChild (node_ptr, NULL, BAD_CAST "prf:Slot", BAD_CAST buf);
    sprintf (buf, "%i", Cam->cam_.flags);
    xmlNewChild (node_ptr, NULL, BAD_CAST "prf:Flags", BAD_CAST buf);

    //set "prf:PmtFlag" to 1 always according to issue #465
    xmlNewChild (node_ptr, NULL, BAD_CAST "prf:PmtFlag", BAD_CAST "0");

    sprintf (buf, "%i", Cam->cam_.status);
    xmlNewChild (node_ptr, NULL, BAD_CAST "prf:Status", BAD_CAST buf);

    return 0;
}

int cDiseqcSetting::AddTunerComponent(xmlNodePtr node, cNetCVTuner *Tuner, int Slot)
{
    char buf[256];
    xmlNodePtr node_ptr, node_ptr1;

/*
    switch (t->fe_info.type) {
        case FE_QPSK:
            strcpy (buf, "DVB-S");
            break;
        case FE_QAM:
            strcpy (buf, "DVB-C");
            break;
        case FE_OFDM:
            strcpy (buf, "DVB-T");
            break;
        case FE_ATSC:
            strcpy (buf, "ATSC");
            break;
#ifdef MICROBLAZE
        case FE_DVBS2:
            strcpy (buf, "DVB-S2");
            break;
#endif
    }
*/

    node_ptr1 = xmlNewChild (node, NULL, BAD_CAST "ccpp:component", NULL);
    node_ptr = xmlNewChild (node_ptr1, NULL, BAD_CAST "rdf:Description", NULL);
    xmlNewProp (node_ptr, BAD_CAST "rdf:about", (unsigned char *) "Tuner");
    xmlNewChild (node_ptr, NULL, BAD_CAST "prf:Name", BAD_CAST Tuner->GetTuner().c_str());
    xmlNewChild (node_ptr, NULL, BAD_CAST "prf:Type", BAD_CAST Tuner->GetType().c_str());
//    xmlNewChild (node_ptr, NULL, BAD_CAST "prf:Slot", BAD_CAST Tuner->GetSlot().c_str());
//    xmlNewChild (node_ptr, NULL, BAD_CAST "prf:Slot", BAD_CAST (const char*)itoa(Slot));
    xmlNewChild (node_ptr, NULL, BAD_CAST "prf:Slot", BAD_CAST Tuner->GetConvertedSlot().c_str());
    sprintf (buf, "%d", Tuner->GetPreference());
    xmlNewChild (node_ptr, NULL, BAD_CAST "prf:Preference", BAD_CAST buf);

    if (Tuner->GetType() == "DVB-S" || Tuner->GetType() == "DVB-S2")
        xmlNewChild (node_ptr, NULL, BAD_CAST "prf:SatelliteListName", BAD_CAST Tuner->GetSatlist().c_str());

    return 0;
}

int cDiseqcSetting::AddSatComponent(xmlNodePtr node, cDiseqcSettingBase *Diseqcsetting)
{
    char buf[256];
    int i;
    cLNBSetting *LNBSetting;
    cLNBSettingDiseqc1_1 *LNBSettingDiseqc1_1=NULL;
    cDiseqc1_1 *Diseqcsetting1_1;
    int Counter[MAX_PORT];
    for(int x=0; x<MAX_PORT; ++x)
        Counter[x]=0;

    xmlNodePtr node_ptr, node_ptr1;

    if(Diseqcsetting->mode_ == eGotoX)
    {
        cGotoX *DiseqcsettingGotoX = static_cast<cGotoX*>(Diseqcsetting);

        node_ptr1 = xmlNewChild (node, NULL, BAD_CAST "ccpp:component", NULL);
        node_ptr = xmlNewChild (node_ptr1, NULL, BAD_CAST "rdf:Description", NULL);
        xmlNewProp (node_ptr, BAD_CAST "rdf:about", (unsigned char *) "Satellite");
        xmlNewChild (node_ptr, NULL, BAD_CAST "prf:Name", BAD_CAST "S19.2E");
        xmlNewChild (node_ptr, NULL, BAD_CAST "prf:Position", BAD_CAST "1992");

        sprintf(buf, "%d", 1800 + DiseqcsettingGotoX->iPositionMin_);
        xmlNewChild(node_ptr, NULL, BAD_CAST "prf:PositionMin", BAD_CAST buf);
        sprintf(buf, "%d", 1800 + DiseqcsettingGotoX->iPositionMax_);
        xmlNewChild(node_ptr, NULL, BAD_CAST "prf:PositionMax", BAD_CAST buf);
        sprintf(buf, "%d", DiseqcsettingGotoX->iLongitude_);
        xmlNewChild(node_ptr, NULL, BAD_CAST "prf:Longitude", BAD_CAST buf);
        sprintf(buf, "%d", DiseqcsettingGotoX->iLatitude_);
        xmlNewChild(node_ptr, NULL, BAD_CAST "prf:Latitude", BAD_CAST buf);
        sprintf(buf, "%d", DiseqcsettingGotoX->iAutoFocus_*10);
        xmlNewChild(node_ptr, NULL, BAD_CAST "prf:AutoFocus", BAD_CAST buf);
        xmlNewChild (node_ptr, NULL, BAD_CAST "prf:Type", BAD_CAST "1");

        node_ptr1 = node_ptr;

        int Polarisation[] = { 0, 0, 1, 1 };
        int Voltage[] = { 0, 0, 1, 1 };
        int ToneMode[] = { 1, 0, 1, 0 };
        bool HighLOF[] = { false, true, false, true };

        for (i = 0; i < 4; ++i) {
            node_ptr = xmlNewChild (node_ptr1, NULL, BAD_CAST "ccpp:component", NULL);
            node_ptr = xmlNewChild (node_ptr, NULL, BAD_CAST "rdf:Description", NULL);
            xmlNewProp (node_ptr, BAD_CAST "rdf:about", (unsigned char *) "SatelliteComponent");

            sprintf (buf, "%d", Polarisation[i]);
            xmlNewChild (node_ptr, NULL, BAD_CAST "prf:Polarisation", BAD_CAST buf);
            xmlNewChild (node_ptr, NULL, BAD_CAST "prf:RangeMin", BAD_CAST (HighLOF[i]?"11726":"10700"));
            xmlNewChild (node_ptr, NULL, BAD_CAST "prf:RangeMax", BAD_CAST (HighLOF[i]?"12750":"11725"));

            xmlNewChild (node_ptr, NULL, BAD_CAST "prf:LOF", BAD_CAST (HighLOF[i]?"10600":"9750"));
            sprintf (buf, "%d", Voltage[i]);
            xmlNewChild (node_ptr, NULL, BAD_CAST "prf:Voltage", BAD_CAST buf);
            sprintf (buf, "%d", ToneMode[i]);
            xmlNewChild (node_ptr, NULL, BAD_CAST "prf:Tone", BAD_CAST buf);
            xmlNewChild (node_ptr, NULL, BAD_CAST "prf:MiniCmd", BAD_CAST "0");

            sprintf(buf, "e0 10 38 f%i", i);
            xmlNewChild (node_ptr, NULL, BAD_CAST "prf:DiSEqC_Cmd", BAD_CAST buf);
        }
    }
    else
    {
        for (int j=0; j < Diseqcsetting->GetNumberOfLNB(); ++j)
        {
            LNBSetting = Diseqcsetting->GetLNBSetting(j);

            node_ptr1 = xmlNewChild (node, NULL, BAD_CAST "ccpp:component", NULL);
            node_ptr = xmlNewChild (node_ptr1, NULL, BAD_CAST "rdf:Description", NULL);
            xmlNewProp (node_ptr, BAD_CAST "rdf:about", (unsigned char *) "Satellite");
            xmlNewChild (node_ptr, NULL, BAD_CAST "prf:Name", BAD_CAST LNBSetting->GetSatName().c_str());
            xmlNewChild (node_ptr, NULL, BAD_CAST "prf:Position", BAD_CAST LNBSetting->GetSatPosition().c_str());

            char type_str[] = "0";
            if (Diseqcsetting->mode_ == eSingleCableRouter)
                type_str[0] = '2';

            xmlNewChild (node_ptr, NULL, BAD_CAST "prf:Type", BAD_CAST type_str);

            node_ptr1 = node_ptr;

            int Polarisation[] = { 0, 0, 1, 1 };
            int Voltage[] = { 0, 0, 1, 1 };
            int ToneMode[] = { 1, 0, 1, 0 };
            bool HighLOF[] = { false, true, false, true };
            int steps = LNBSetting->GetLOF(true) == "0"?2:1;

            for (i = 0; i < 4; i+=steps) {
                node_ptr = xmlNewChild (node_ptr1, NULL, BAD_CAST "ccpp:component", NULL);
                node_ptr = xmlNewChild (node_ptr, NULL, BAD_CAST "rdf:Description", NULL);
                xmlNewProp (node_ptr, BAD_CAST "rdf:about", (unsigned char *) "SatelliteComponent");

                sprintf (buf, "%d", Polarisation[i]);
                xmlNewChild (node_ptr, NULL, BAD_CAST "prf:Polarisation", BAD_CAST buf);
                xmlNewChild (node_ptr, NULL, BAD_CAST "prf:RangeMin", BAD_CAST LNBSetting->GetRangeMin(HighLOF[i]).c_str());
                xmlNewChild (node_ptr, NULL, BAD_CAST "prf:RangeMax", BAD_CAST LNBSetting->GetRangeMax(HighLOF[i]).c_str());

                xmlNewChild (node_ptr, NULL, BAD_CAST "prf:LOF", BAD_CAST LNBSetting->GetLOF(HighLOF[i]).c_str());
                sprintf (buf, "%d", Voltage[i]);
                xmlNewChild (node_ptr, NULL, BAD_CAST "prf:Voltage", BAD_CAST buf);
                sprintf (buf, "%d", ToneMode[i]);
                xmlNewChild (node_ptr, NULL, BAD_CAST "prf:Tone", BAD_CAST buf);
                if (j<3)
                    sprintf (buf, "%d", j);
                else
                    sprintf (buf, "%d", 2);
                xmlNewChild (node_ptr, NULL, BAD_CAST "prf:MiniCmd", BAD_CAST buf);

                std::string strBuffer;

                if(Diseqcsetting->mode_ == eDiseqc1_0)
                {
                    LNBSetting = static_cast<cLNBSetting *>(Diseqcsetting->GetLNBSetting(j));

                    sprintf(buf, "%x", j*4 + i/steps);
                    strBuffer = "e0 10 38 f" + std::string(buf);
                }
                else if(Diseqcsetting->mode_ == eDiseqc1_1)
                {
                    Diseqcsetting1_1 = static_cast<cDiseqc1_1*>(Diseqcsetting);
                    LNBSettingDiseqc1_1 = static_cast<cLNBSettingDiseqc1_1 *>(Diseqcsetting->GetLNBSetting(j));

                    if(Diseqcsetting1_1->iMultiswitch_ == 0)
                    {
			sprintf(buf, "%x", j*4 + i/steps);
                        strBuffer = "e0 10 38 f" + std::string(buf);
                    }
                    else
                    {
                        sprintf(buf, "%x", LNBSettingDiseqc1_1->iCascade_ - 1);
                        strBuffer = "e0 10 39 f" + std::string(buf) + " , ";
                        sprintf(buf, "%x", Counter[LNBSettingDiseqc1_1->iCascade_ - 1] * 4 + i/steps);
                        strBuffer = strBuffer + "e0 10 38 f" + std::string(buf);
                    }
                } else if (Diseqcsetting->mode_ == eSingleCableRouter)
                {
                    cDiseqcSCR *diseqcSCR = dynamic_cast<cDiseqcSCR*> (Diseqcsetting);
                    if (diseqcSCR)
                    {
                        sprintf(buf, "SCR Diseqc cmd here. Tone %d Pol %d",
                                ToneMode[i], Polarisation[i]);
                        strBuffer = std::string(buf);

                        strBuffer = diseqcSCR->scrProps.DiseqcCommand(j,
                                                                      Polarisation[i],
                                                                      !ToneMode[i]);

                        printf("Diseq_cmd SCR: '%s'\n", strBuffer.c_str());
                    }
                }

                xmlNewChild (node_ptr, NULL, BAD_CAST "prf:DiSEqC_Cmd", BAD_CAST strBuffer.c_str());
            }
            if(Diseqcsetting->mode_ == eDiseqc1_1)
                ++Counter[LNBSettingDiseqc1_1->iCascade_ - 1];
        }
    }
    return 0;
}

int cDiseqcSetting::Upload(const char* FileName)
{
    std::string strBuff = "netcvupdate -i ";
    strBuff.append(NetceiverUUID_);
    if(Interface_ && strlen(Interface_))
    {
        strBuff.append(" -d ");
        strBuff.append(Interface_);
    }
    strBuff.append(" -U ");
    strBuff.append(FileName);
    strBuff.append(" -K");
    return SystemExec(strBuff.c_str());
}


void cDiseqcSetting::Restart(bool bRestartVDR)
{
    Skins.Message(mtStatus, tr("Restarting NetCeiver..."));
    sleep(11);
    SystemExec("/etc/init.d/mcli restart");
    if (bRestartVDR)
    {
        Skins.Message(mtStatus, tr("Restarting VDR..."));
        SystemExec("killall -HUP vdr"); // dirty hack
    }
    sleep(3);
#if VDRVERSNUM < 10716
    Diseqcs.Clear();
#endif
    Diseqcs.Load(AddDirectory("/etc/vdr", "diseqc.conf"), true, Setup.DiSEqC);
    cPlugin *plug = cPluginManager::GetPlugin("netcvrotor");
    const char *cmd = "netcvrotor.loadConfig";
    if(plug)
       plug->Service(cmd, (void*)cmd);
}

int cDiseqcSetting::WriteSetting()
{
    std::string strPath = TmpPath_;
    strPath.append("/netceiver-upload.conf");
    //printf("\033[0;93m %s \033[0m\n", strPath.c_str());
    int retValue = 0;
    int sat_support = 0;

    xmlDocPtr doc;
    xmlNodePtr root_node, node;
    xmlDtdPtr dtd;

    /* 
     * Creates a new document, a node and set it as a root node
     */
    doc = xmlNewDoc (BAD_CAST "1.0");
    root_node = xmlNewNode (NULL, BAD_CAST "rdf:RDF");
    xmlNewProp (root_node, (unsigned char *) "xmlns:rdf", (unsigned char *) "http://www.w3.org/1999/02/22-rdf-syntax-ns#");
    xmlNewProp (root_node, (unsigned char *) "xmlns:ccpp", (unsigned char *) "http://www.w3.org/2002/11/08-ccpp-schema#");
    xmlNewProp (root_node, (unsigned char *) "xmlns:prf", (unsigned char *) "http://www.w3.org/2000/01/rdf-schema#");
    xmlDocSetRootElement (doc, root_node);

    /*
     * Creates a DTD declaration. Isn't mandatory. 
     */
    dtd = xmlCreateIntSubset (doc, BAD_CAST "root", NULL, BAD_CAST "tree2.dtd");

    /* 
     * xmlNewChild() creates a new node, which is "attached" as child node
     * of root_node node. 
     */
    node = xmlNewChild (root_node, NULL, BAD_CAST "rdf:Description", NULL);
    xmlNewProp (node, BAD_CAST "rdf:about", (unsigned char *) "NetCeiver");

    // Tuners
    int TunerCounter = 0;
    for (tunerIt_t tunerIter=Tuners_->Begin(); tunerIter < Tuners_->End(); ++tunerIter) {
        AddTunerComponent(node, &(*tunerIter), TunerCounter);
        if (tunerIter->GetType() == "DVB-S" || tunerIter->GetType() == "DVB-S") {
            sat_support = 1;
        }
        ++TunerCounter;
    }

    // CAMs
    for (camIt_t camIter=Cams_->Begin(); camIter < Cams_->End(); ++camIter)
        AddCamComponent(node, &(*camIter));

    xmlNodePtr node_ptr, node_ptr1;

    for(std::vector<cSatList*>::iterator satlistIter = SatList_.begin(); satlistIter < SatList_.end(); ++satlistIter)
    {
        node_ptr1 = xmlNewChild (node, NULL, BAD_CAST "ccpp:component", NULL);
        node_ptr = xmlNewChild (node_ptr1, NULL, BAD_CAST "rdf:Description", NULL);
        xmlNewProp (node_ptr, BAD_CAST "rdf:about", (unsigned char *) "SatelliteList");
        xmlNewChild (node_ptr, NULL, BAD_CAST "prf:SatelliteListName", BAD_CAST (*satlistIter)->Name_);
        retValue = AddSatComponent(node_ptr, *((*satlistIter)->GetModeIterator()));
        if(retValue)
            return 1; // Adding sat component failed
    }

    retValue = xmlSaveFormatFileEnc (strPath.c_str(), doc, "UTF-8", 1);
    if(retValue != 0)
        retValue = Upload(strPath.c_str());
    else
        retValue = 2; // creating XML-file failed
    if(retValue == 0)
        Restart(false);
    else
        retValue = 3; // Uploading netceiver.conf to NetCeiver failed

    xmlFreeDoc (doc);

    return retValue;
}

static void clean_xml_parser_thread (void *arg)
{
    xml_parser_context_t *c = (xml_parser_context_t *) arg;
    if (c->str) {
        xmlFree (c->str);
    }
    if (c->key) {
        xmlFree (c->key);
    }
    if (c->doc) {
        xmlFreeDoc (c->doc);
    }
    //printf("\033[0;93m Free XML parser structures! \033[0m \n");
}

int cDiseqcSetting::LoadSetting()
{
    int retValue = 0;
    std::string strPath = TmpPath_;
    strPath.append("/netceiver.conf");

    if(Download())
        retValue = 1; // Downloading from Netceiver failed

    if(retValue == 0)
    {
        netceiver_info_t *nc_info = (netceiver_info_t *) malloc(sizeof(netceiver_info_t));
        if(ReadNetceiverConf(strPath.c_str(), nc_info) != 0)
            retValue = 2; // Parsing netceiver.conf failed

        if(retValue == 0)
        {
            if(SetSettings(nc_info))
                retValue = 3; // Setting settings failed
        }

        if(nc_info)
        {
            if(nc_info->sat_list)
            {
                if(nc_info->sat_list->sat)
                {
                    if(nc_info->sat_list->sat->comp)
                        free(nc_info->sat_list->sat->comp);
                    free(nc_info->sat_list->sat);
                }
                free(nc_info->sat_list);
            }
            free(nc_info);
        }
    }

    return retValue;
}

int cDiseqcSetting::SaveSetting()
{
    Skins.Message(mtStatus, tr("Please wait..."));
    int result = CheckSetting();
    switch(result)
    {
        case 0:
            result = WriteSetting();
            if(result == 0)
            {
                int OldDiSEqC = ::Setup.DiSEqC;
#if VDRVERSNUM < 10716
                Diseqcs.Clear();
#endif
                if(Diseqcs.Load("/etc/vdr/diseqc.conf", true, Setup.DiSEqC))
                {
                    // In case if DiSEqC is set to 0, set it to 1 if there are Sat-Tuners
                    if(::Setup.DiSEqC == 0)
                    {
                        if(OldDiSEqC == 1)
                            Skins.Message(mtError, tr("Reloading diseqc.conf failed!"), 3);
                        bool Diseqc = false;
                        for(std::vector<cNetCVTuner>::iterator tunerIter = Tuners_->Begin(); tunerIter < Tuners_->End(); ++tunerIter)
                        {
                            if(tunerIter->GetType().find("DVB-S") == 0)
                                Diseqc = true;
                        }
                        if(Diseqc)
                        {
                            ::Setup.DiSEqC = 1;
                            ::Setup.Save();
                        }
                    }

//                    if(Channels.Reload("/etc/vdr/channels.conf", false, true)) //Causes "crash" in vdr
                    if(Channels.Load("/etc/vdr/channels.conf", false, true))
                        Skins.Message(mtInfo, tr("Settings stored"), 3);
                    else
                    {
                        Skins.Message(mtError, tr("Reloading channels.conf failed"), 3);
                        return -1;
                    }
                }
                else
                {
                    Skins.Message(mtError, tr("Reloading diseqc.conf failed"), 3);
                    return -1;
                }
            }
            else
            {
                std::string errNumber = tr("Error: Store failed! Error Number: ");
                errNumber.append(itoa(result));
                Skins.Message(mtError, errNumber.c_str(), 3);
                return -1;
            }
            break;
        case 1:
            Skins.Message(mtWarning, tr("Error: Only 4 LNBs per cascade is allowed!"), 3);
            break;
        case 2:
            Skins.Message(mtWarning, tr("Error: Every DiSEqC list must have a name!"), 3);
            break;
        case 3:
            Skins.Message(mtWarning, tr("Error: Duplicate DiSEqC list name is not allowed!"), 3);
            break;
        case 4:
            Skins.Message(mtWarning, tr("Error: Every tuner must have a valid DiSEqC list!"), 3);
            break;
        case 5:
            Skins.Message(mtWarning, tr("Error: At least 1 tuner must be enabled!"), 3);
            break;
        default:
            std::string errNumber = tr("Unknown Error: ");
            errNumber.append(itoa(result));
            Skins.Message(mtError, errNumber.c_str(), 3);
            break;
    }
    return result;
}

int cDiseqcSetting::Download()
{
    std::string strBuff = "cd "; // first change the working directory to /tmp/Netcv.XXXXXX/
    strBuff.append(TmpPath_);
    strBuff.append(";netcvupdate -i ");
    strBuff.append(NetceiverUUID_);
    if(Interface_ && strlen(Interface_))
    {
        strBuff.append(" -d ");
        strBuff.append(Interface_);
    }
    strBuff.append(" -D");
    return SystemExec(strBuff.c_str());
}

int cDiseqcSetting::ReadNetceiverConf(const char *filename, netceiver_info_t *nc_info)
{
    int retValue = 0;
    xml_parser_context_t c;
    xmlNode *root_element, *cur_node;
    nc_info->sat_list_num = 0;
    nc_info->sat_list = NULL;

    xmlKeepBlanksDefault (0);	//reomve this f. "text" nodes

    c.doc = xmlReadFile(filename, NULL, 0);

//    c.doc = xmlParseMemory ((char *) xmlbuff, buffersize);
    root_element = xmlDocGetRootElement (c.doc);
    pthread_cleanup_push (clean_xml_parser_thread, &c);

    if (root_element != NULL) {
        cur_node = root_element->children;

        if (!xmlStrcmp (cur_node->name, (xmlChar *) "Description")) {

            root_element = cur_node->children;
            while (root_element != NULL) {
                c.key = NULL;
                c.str = NULL;
                if ((xmlStrcmp (root_element->name, (const xmlChar *) "component"))) {
                    esyslog("%s:%s: Cannot parse XML data: \"%s\"", __FILE__, __FUNCTION__, root_element->name);
                    root_element = root_element->next;
                    continue;
                }

                cur_node = root_element->children;
                if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "Description"))) {
                    c.str = xmlGetProp (cur_node, (unsigned char *) "about");
                } else {
                    esyslog("%s:%s: Cannot parse XML data: \"%s\"", __FILE__, __FUNCTION__, cur_node->name);
                    root_element = root_element->next;
                    continue;
                }
                if (c.str && !(xmlStrcmp (c.str, (xmlChar *) "SatelliteList"))) {
                    cur_node = cur_node->children;
                    nc_info->sat_list = (satellite_list_t *) realloc (nc_info->sat_list, (nc_info->sat_list_num + 1) * sizeof (satellite_list_t));
                    if (!nc_info->sat_list) {
//                        err ("Cannot get memory for sat_list\n");
                        esyslog("%s:%s: Cannot get memory for sat_list", __FILE__, __FUNCTION__);
                    }
                    satellite_list_t *sat_list = nc_info->sat_list + nc_info->sat_list_num;
                    memset (sat_list, 0, sizeof (satellite_list_t));

                    while (cur_node != NULL) {
                        if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "SatelliteListName"))) {
                            c.key = xmlNodeListGetString (c.doc, cur_node->xmlChildrenNode, 1);
                            if (c.key) {
                                strncpy (sat_list->Name, (char *) c.key, UUID_SIZE-1);
                                xmlFree (c.key);
                            }
                        } else if ((!xmlStrcmp (cur_node->name, (const xmlChar *) "component"))) {
                            xmlNode *l2_node = cur_node->children;
                            if ((!xmlStrcmp (l2_node->name, (const xmlChar *) "Description"))) {
                                c.key = xmlGetProp (l2_node, (unsigned char *) "about");
                                if (c.key && !xmlStrcmp (c.key, (xmlChar *) "Satellite")) {
                                    xmlFree (c.key);
                                    l2_node = l2_node->children;

                                    sat_list->sat = (satellite_info_t *) realloc (sat_list->sat, (sat_list->sat_num + 1) * sizeof (satellite_info_t));
                                    if (!sat_list->sat) {
//                                        err ("Cannot get memory for sat\n");
                                        esyslog("%s:%s: Cannot get memory for sat", __FILE__, __FUNCTION__);
                                    }

                                    satellite_info_t *sat = sat_list->sat + sat_list->sat_num;
                                    memset (sat, 0, sizeof (satellite_info_t));

                                    while (l2_node != NULL) {
                                        if ((!xmlStrcmp (l2_node->name, (const xmlChar *) "Name"))) {
                                            c.key = xmlNodeListGetString (c.doc, l2_node->xmlChildrenNode, 1);
                                            if(c.key) {
                                                strncpy (sat->Name, (char *) c.key, UUID_SIZE-1);
                                                xmlFree (c.key);
                                            }
                                        } else if ((!xmlStrcmp (l2_node->name, (const xmlChar *) "Position"))) {
                                            c.key = xmlNodeListGetString (c.doc, l2_node->xmlChildrenNode, 1);
                                            if(c.key) {
                                                sat->SatPos = atoi ((char *) c.key);
                                                xmlFree (c.key);
                                            }
                                        } else if ((!xmlStrcmp (l2_node->name, (const xmlChar *) "PositionMin"))) {
                                            c.key = xmlNodeListGetString (c.doc, l2_node->xmlChildrenNode, 1);
                                            if(c.key) {
                                                sat->SatPosMin = atoi ((char *) c.key);
                                                xmlFree (c.key);
                                            }
                                        } else if ((!xmlStrcmp (l2_node->name, (const xmlChar *) "PositionMax"))) {
                                            c.key = xmlNodeListGetString (c.doc, l2_node->xmlChildrenNode, 1);
                                            if(c.key) {
                                                sat->SatPosMax = atoi ((char *) c.key);
                                                xmlFree (c.key);
                                            }
                                        } else if ((!xmlStrcmp (l2_node->name, (const xmlChar *) "Longitude"))) {
                                            c.key = xmlNodeListGetString (c.doc, l2_node->xmlChildrenNode, 1);
                                            if(c.key) {
                                                sat->Longitude = atoi ((char *) c.key);
                                                xmlFree (c.key);
                                            }
                                        } else if ((!xmlStrcmp (l2_node->name, (const xmlChar *) "Latitude"))) {
                                            c.key = xmlNodeListGetString (c.doc, l2_node->xmlChildrenNode, 1);
                                            if(c.key) {
                                                sat->Latitude = atoi ((char *) c.key);
                                                xmlFree (c.key);
                                            }
                                        } else if ((!xmlStrcmp (l2_node->name, (const xmlChar *) "AutoFocus"))) {
                                            c.key = xmlNodeListGetString (c.doc, l2_node->xmlChildrenNode, 1);
                                            if(c.key) {
                                                sat->AutoFocus = atoi ((char *) c.key);
                                                xmlFree (c.key);
                                            }
                                        } else if ((!xmlStrcmp (l2_node->name, (const xmlChar *) "Type"))) {
                                            c.key = xmlNodeListGetString (c.doc, l2_node->xmlChildrenNode, 1);
                                            if(c.key) {
                                                sat->type = (satellite_source_t) atoi ((char *) c.key);
                                                xmlFree (c.key);
                                            }
                                        } else if ((!xmlStrcmp (l2_node->name, (const xmlChar *) "component"))) {
                                            xmlNode *l3_node = l2_node->children;

                                            if ((!xmlStrcmp (l3_node->name, (const xmlChar *) "Description"))) {
                                                c.key = xmlGetProp (l3_node, (unsigned char *) "about");
                                                if (c.key && !xmlStrcmp (c.key, (xmlChar *) "SatelliteComponent")) {
                                                    xmlFree (c.key);
                                                    l3_node = l3_node->children;
                                                    sat->comp = (satellite_component_t *) realloc (sat->comp, (sat->comp_num + 1) * sizeof (satellite_component_t));
                                                    if (!sat->comp) {
                                                        esyslog("%s:%s: Cannot get memory for comp", __FILE__, __FUNCTION__);
                                                    }

                                                    satellite_component_t *comp = sat->comp + sat->comp_num;
                                                    memset (comp, 0, sizeof (satellite_component_t));

                                                    while (l3_node != NULL) {
                                                        if ((!xmlStrcmp (l3_node->name, (const xmlChar *) "Polarisation"))) {
                                                            c.key = xmlNodeListGetString (c.doc, l3_node->xmlChildrenNode, 1);
                                                            if(c.key) {
                                                                comp->Polarisation = (polarisation_t) atoi ((char *) c.key);
                                                                xmlFree (c.key);
                                                            }
                                                        } else if ((!xmlStrcmp (l3_node->name, (const xmlChar *) "RangeMin"))) {
                                                            c.key = xmlNodeListGetString (c.doc, l3_node->xmlChildrenNode, 1);
                                                            if(c.key) {
                                                                comp->RangeMin = atoi ((char *) c.key);
                                                                xmlFree (c.key);
                                                            }
                                                        } else if ((!xmlStrcmp (l3_node->name, (const xmlChar *) "RangeMax"))) {
                                                            c.key = xmlNodeListGetString (c.doc, l3_node->xmlChildrenNode, 1);
                                                            if(c.key) {
                                                                comp->RangeMax = atoi ((char *) c.key);
                                                                xmlFree (c.key);
                                                            }
                                                        } else if ((!xmlStrcmp (l3_node->name, (const xmlChar *) "LOF"))) {
                                                            c.key = xmlNodeListGetString (c.doc, l3_node->xmlChildrenNode, 1);
                                                            if(c.key) {
                                                                comp->LOF = atoi ((char *) c.key);
                                                                xmlFree (c.key);
                                                            }
                                                        } else if ((!xmlStrcmp (l3_node->name, (const xmlChar *) "Voltage"))) {
                                                            c.key = xmlNodeListGetString (c.doc, l3_node->xmlChildrenNode, 1);
                                                            if(c.key) {
                                                                comp->sec.voltage = (fe_sec_voltage_t) atoi ((char *) c.key);
                                                                xmlFree (c.key);
                                                            }
                                                        } else if ((!xmlStrcmp (l3_node->name, (const xmlChar *) "Tone"))) {
                                                            c.key = xmlNodeListGetString (c.doc, l3_node->xmlChildrenNode, 1);
                                                            if(c.key) {
                                                                comp->sec.tone_mode = (fe_sec_tone_mode_t) atoi ((char *) c.key);
                                                                xmlFree (c.key);
                                                            }
                                                        } else if ((!xmlStrcmp (l3_node->name, (const xmlChar *) "MiniCmd"))) {
                                                            c.key = xmlNodeListGetString (c.doc, l3_node->xmlChildrenNode, 1);
                                                            if(c.key) {
                                                                comp->sec.mini_cmd = (fe_sec_mini_cmd_t) atoi ((char *) c.key);
                                                                xmlFree (c.key);
                                                            }
                                                        } else if ((!xmlStrcmp (l3_node->name, (const xmlChar *) "DiSEqC_Cmd"))) {
                                                            c.key = xmlNodeListGetString (c.doc, l3_node->xmlChildrenNode, 1);
                                                            if(c.key) {
                                                                int v[6], i, n=0;
                                                                char *s= (char *)c.key;
                                                                struct dvb_diseqc_master_cmd *diseqc_cmd=&comp->sec.diseqc_cmd;
                                                                do {
                                                                    diseqc_cmd->msg_len = sscanf (s, "%x %x %x %x %x %x", v, v + 1, v + 2, v + 3, v + 4, v + 5);
                                                                    for (i = 0; i < diseqc_cmd->msg_len; i++) {
                                                                        diseqc_cmd->msg[i] = v[i];
                                                                    }
                                                                    s=strchr(s,',');
                                                                    if(s) {
                                                                        s++;
                                                                    }
                                                                    diseqc_cmd=comp->diseqc_cmd+n;
                                                                    n++;
                                                                } while(s && n<=DISEQC_MAX_EXTRA);
                                                                xmlFree (c.key);
                                                                comp->diseqc_cmd_num=n;
                                                            }
                                                        }
                                                        l3_node = l3_node->next;
                                                    }
                                                    sat->comp_num++;
                                                } else {
                                                    xmlFree (c.key);
                                                }
                                            }
                                        }
                                        l2_node = l2_node->next;
                                    }
                                    sat_list->sat_num++;
                                } else {
                                    xmlFree (c.key);
                                }
                            }
                        }
                        cur_node = cur_node->next;
                    }
                    nc_info->sat_list_num++;
                }
                xmlFree (c.str);
                root_element = root_element->next;
            }
        }
    }
    else
        retValue = 1; // Read Error?

    xmlFreeDoc (c.doc);
    pthread_cleanup_pop (0);
    
    return retValue;
}

int cDiseqcSetting::SetSettings(netceiver_info_t *nc_info)
{
    satellite_list_t *sat_list;
    std::vector<cSatList*>::iterator satlistIter;

    for(int i=0; i < nc_info->sat_list_num; ++i)
    {
        sat_list = nc_info->sat_list + i;
        AddSatlist(sat_list->Name);
        satlistIter = SatList_.end()-1;

        eDiseqcMode DiseqcMode=DetectDiseqcMode(sat_list);
        int retValue = (*satlistIter)->SetSettings(DiseqcMode, sat_list);
        if (retValue)
            return retValue;
    }

    return 0;
}

eDiseqcMode cDiseqcSetting::DetectDiseqcMode(satellite_list_t *sat_list)
{
    satellite_info_t *sat;
    satellite_component_t *comp;
    char buf[256];
    std::string strBuff;

        for(int y=0; y<sat_list->sat_num; ++y)
        {
            sat = sat_list->sat + y;

            if(sat->type == 1)
                return eGotoX;
            if(sat->type == 2)
                return eSingleCableRouter;

            for(int z=0; z<sat->comp_num; ++z)
            {
                comp = sat->comp + z;

                struct dvb_diseqc_master_cmd *diseqc_cmd=&comp->sec.diseqc_cmd;
                strBuff="";
                for(int i=0; i<diseqc_cmd->msg_len; ++i)
                {
                    sprintf(buf, "%x", diseqc_cmd->msg[i]);
                    strBuff.append(buf);
                }
                if(strBuff.find("e01039") == 0)
                    return eDiseqc1_1;
            }
        }
    return eDiseqc1_0;
}

cSatList* cDiseqcSetting::GetSatlistByName(const char* Name)
{
    for(std::vector<cSatList*>::iterator satlistIter=SatList_.begin(); satlistIter < SatList_.end(); ++satlistIter)
        if(strcmp((*satlistIter)->Name_, Name) == 0)
            return *satlistIter;

    return NULL;
}

// class cDiseqcSettingBase -------------------------------------------
cDiseqcSettingBase::cDiseqcSettingBase(const char *Name, const eDiseqcMode Mode) : name_(Name), mode_(Mode)
{
}

void cDiseqcSettingBase::AddMode(cOsdMenu *menu, std::vector<cDiseqcSettingBase*>::iterator *modeIter, std::vector<cDiseqcSettingBase*>::iterator firstIter, std::vector<cDiseqcSettingBase*>::iterator lastIter)
{
    menu->Add(new cDiseqcSettingModeItem(name_, modeIter, firstIter, lastIter));
}

// class cDiseqcDisabled ----------------------------------------------
cDiseqcDisabled::cDiseqcDisabled() : cDiseqcSettingBase(tr("DiSEqC disabled"), eDiseqcDisabled)
{
    LNBSetting_ = new cLNBSetting();
    LNBSetting_->iSource_ = DISEQC_SOURCE_ASTRA_19;
    LNBSetting_->iLNB_Type_ = 5;
}

cDiseqcDisabled::~cDiseqcDisabled()
{
}

void cDiseqcDisabled::BuildMenu(cOsdMenu *menu)
{
    std::string strBuff = "         ";
    strBuff.append(tr("Diseqc$Source"));
    menu->Add(new cMenuEditSrcItem(strBuff.c_str(), &(LNBSetting_->iSource_)));

    strBuff = "         ";
    strBuff.append(tr("LNB Type"));
    menu->Add(new cMenuEditStraItem(strBuff.c_str(), &(LNBSetting_->iLNB_Type_), 7, LNB_Type_string));
}

// class cDiseqcMini --------------------------------------------------
cDiseqcMini::cDiseqcMini() : cDiseqcSettingBase(tr("DiSEqC Mini"), eDiseqcMini), iMaxLNB_(2)
{
    iNumberOfLNB_ = 1;
    for (int i = 0; i< iMaxLNB_; ++i)
        LNBSetting_.push_back(new cLNBSetting());
//    iDelay_ = 15;
}

cDiseqcMini::~cDiseqcMini()
{
}

void cDiseqcMini::BuildMenu(cOsdMenu *menu)
{
    std::string strBuff = "         ";
    strBuff.append(tr("Number of LNBs"));
    menu->Add(new cMenuEditIntRefreshItem(strBuff.c_str(), &iNumberOfLNB_, 1, iMaxLNB_));

    char cLNB = '1';
    std::vector<cLNBSetting*>::iterator iterLNB = LNBSetting_.begin();
    for (int i=0; i < iNumberOfLNB_; ++i)
    {
        strBuff = "         ";
        strBuff.append(tr("LNB"));
        strBuff = strBuff + " " + cLNB;
        strBuff.append(" ");
        strBuff.append(tr("Diseqc$Source"));
        menu->Add(new cMenuEditSrcItem(strBuff.c_str(), &((*iterLNB)->iSource_)));

        strBuff = "                  ";
        strBuff.append(tr("Type"));
        menu->Add(new cMenuEditStraItem(strBuff.c_str(), &((*iterLNB)->iLNB_Type_), 7, LNB_Type_string));

        ++cLNB;
        ++iterLNB;
    }

//    strBuff = "      ";
//    strBuff.append(tr("Delay (ms)"));
//    menu->Add(new cMenuEditIntItem(strBuff.c_str(), &iDelay_, 15, 100));
}

void cDiseqcMini::SetNumberOfLNB(int number)
{
    if(number < 1)
        iNumberOfLNB_ = 1;
    else if(number > iMaxLNB_)
        iNumberOfLNB_ = iMaxLNB_;
    else
        iNumberOfLNB_ = number;
}

cDiseqcSCR::cDiseqcSCR() : cDiseqcSettingBase(tr("Single Cable Router"), eSingleCableRouter), iMaxLNB_(2)
{
    iNumberOfLNB_ = 1;
    for (int i = 0; i< iMaxLNB_; ++i)
        LNBSetting_.push_back(new cLNBSetting());
    LNBSetting_.at(1)->iSource_ = DISEQC_SOURCE_HOTBIRD_13;

}

cDiseqcSCR::~cDiseqcSCR()
{
}

int cDiseqcSCR::SetSettings(satellite_list_t *sat_list)
{
    printf("%s\n", __PRETTY_FUNCTION__);
    satellite_info_t *sat;
    iNumberOfLNB_ = sat_list->sat_num;


    for(int i=0; i<iNumberOfLNB_; ++i)
    {
        sat = sat_list->sat + i;
        printf("Sat/LNB #%d/%d ", i, iNumberOfLNB_);
        for (int j=0 ; j < sat->comp_num; ++j)
        {
            printf("\n\tSat comp #%d/%d   ", j, sat->comp_num);
            satellite_component_t *comp = sat->comp + j;


            struct dvb_diseqc_master_cmd *diseqc_cmd = &comp->sec.diseqc_cmd;
            printf("Diseqc Cmd [total:=%d]   ", diseqc_cmd->msg_len);
            for (int k=0; k < diseqc_cmd->msg_len; ++k)
            {
                printf("%x ", diseqc_cmd->msg[k]);
            }
            printf("\n");

            /* pin */
            if (diseqc_cmd->msg_len >= 6) {
                scrProps.pin = diseqc_cmd->msg[5];
            }
            if (diseqc_cmd->msg_len >= 5) {
                /*XX*/
                int xx = diseqc_cmd->msg[3]; /* contains*/

                /*YY*/
                int yy = diseqc_cmd->msg[4]; /* lsb of Z()*/

                scrProps.get_from_xx_yy(xx, yy);
            }
        }
        printf("\n");

        printf("Source: %s\n", sat->Name);
        if(LNBSetting_.at(i)->SetSource(sat->Name))
            return 1; // Setting Source failed

        if(LNBSetting_.at(i)->SetLNBType(sat))
            return 2; // Setting LNB type failed
    }

    return 0;
}
void cDiseqcSCR::BuildMenu(cOsdMenu *menu)
{

    std::string strBuff = "         ";
    strBuff.append(tr("Number of LNBs"));
    menu->Add(new cMenuEditIntRefreshItem(strBuff.c_str(), &iNumberOfLNB_, 1, iMaxLNB_));

    char cLNB = '1';
    std::vector<cLNBSetting*>::iterator iterLNB = LNBSetting_.begin();
    for (int i=0; i < iNumberOfLNB_; ++i)
    {
        strBuff = "         ";
        strBuff.append(tr("LNB"));
        strBuff = strBuff + " " + cLNB;
        strBuff.append(" ");
        strBuff.append(tr("Diseqc$Source"));
        menu->Add(new cMenuEditSrcItem(strBuff.c_str(), &((*iterLNB)->iSource_)));

        strBuff = "                    ";
        strBuff.append(tr("Type"));
        menu->Add(new cMenuEditStraItem(strBuff.c_str(), &((*iterLNB)->iLNB_Type_), 7, LNB_Type_string));

        ++cLNB;
        ++iterLNB;
    }

    strBuff =  "         ";
    strBuff += tr("Unique Slot Nr.");
    menu->Add(new cMenuEditIntItem(strBuff.c_str(), &scrProps.uniqSlotNr, 0, 7));

    strBuff =  "         ";
    strBuff += tr("Downlink freq. (MHz)");
    menu->Add(new cMenuEditIntItem(strBuff.c_str(), &scrProps.downlinkFreq, 900, 2200));

    strBuff =  "         ";
    strBuff += tr("PIN (optional)");
    menu->Add(new cMenuEditIntItem(strBuff.c_str(), &scrProps.pin, -1, 255,
                             tr("No pin")));

//    strBuff = "      ";
//    strBuff.append(tr("Enable Toneburst"));
//    menu->Add(new cMenuEditBoolItem(strBuff.c_str(), &iToneburst_));

//    strBuff = "      ";
//    strBuff.append(tr("Delay (ms)"));
//    menu->Add(new cMenuEditIntItem(strBuff.c_str(), &iDelay_, 15, 100));

//    strBuff = "      ";
//    strBuff.append(tr("Repeat"));
//    menu->Add(new cMenuEditIntItem(strBuff.c_str(), &iRepeat_, 0, 3));
}

void cDiseqcSCR::SetNumberOfLNB(int number)
{
    if(number < 1)
        iNumberOfLNB_ = 1;
    else if(number > iMaxLNB_)
        iNumberOfLNB_ = iMaxLNB_;
    else
        iNumberOfLNB_ = number;
}

// class cDiseqc1_0 ---------------------------------------------------
cDiseqc1_0::cDiseqc1_0() : cDiseqcSettingBase(tr("DiSEqC 1.0"), eDiseqc1_0), iMaxLNB_(4)
{
    iNumberOfLNB_ = 1;
    for (int i = 0; i< iMaxLNB_; ++i)
        LNBSetting_.push_back(new cLNBSetting());
    LNBSetting_.at(1)->iSource_ = DISEQC_SOURCE_HOTBIRD_13;
    LNBSetting_.at(2)->iSource_ = DISEQC_SOURCE_ASTRA_28;
    LNBSetting_.at(3)->iSource_ = DISEQC_SOURCE_ASTRA_23;
//    iToneburst_ = 0;
//    iDelay_ = 15;
//    iRepeat_ = 0;
}

cDiseqc1_0::~cDiseqc1_0()
{
}

int cDiseqc1_0::SetSettings(satellite_list_t *sat_list)
{
    satellite_info_t *sat;
    iNumberOfLNB_ = sat_list->sat_num;

    for(int i=0; i<iNumberOfLNB_; ++i)
    {
        sat = sat_list->sat + i;
        if(LNBSetting_.at(i)->SetSource(sat->Name))
            return 1; // Setting Source failed

        if(LNBSetting_.at(i)->SetLNBType(sat))
            return 2; // Setting LNB type failed
    }

    return 0;
}

void cDiseqc1_0::BuildMenu(cOsdMenu *menu)
{

    std::string strBuff = "         ";
    strBuff.append(tr("Number of LNBs"));
    menu->Add(new cMenuEditIntRefreshItem(strBuff.c_str(), &iNumberOfLNB_, 1, iMaxLNB_));

    char cLNB = '1';
    std::vector<cLNBSetting*>::iterator iterLNB = LNBSetting_.begin();
    for (int i=0; i < iNumberOfLNB_; ++i)
    {
        strBuff = "         ";
        strBuff.append(tr("LNB"));
        strBuff = strBuff + " " + cLNB;
        strBuff.append(" ");
        strBuff.append(tr("Diseqc$Source"));
        menu->Add(new cMenuEditSrcItem(strBuff.c_str(), &((*iterLNB)->iSource_)));

        strBuff = "                    ";
        strBuff.append(tr("Type"));
        menu->Add(new cMenuEditStraItem(strBuff.c_str(), &((*iterLNB)->iLNB_Type_), 7, LNB_Type_string));

        ++cLNB;
        ++iterLNB;
    }

//    strBuff = "      ";
//    strBuff.append(tr("Enable Toneburst"));
//    menu->Add(new cMenuEditBoolItem(strBuff.c_str(), &iToneburst_));

//    strBuff = "      ";
//    strBuff.append(tr("Delay (ms)"));
//    menu->Add(new cMenuEditIntItem(strBuff.c_str(), &iDelay_, 15, 100));

//    strBuff = "      ";
//    strBuff.append(tr("Repeat"));
//    menu->Add(new cMenuEditIntItem(strBuff.c_str(), &iRepeat_, 0, 3));
}

void cDiseqc1_0::SetNumberOfLNB(int number)
{
    if(number < 1)
        iNumberOfLNB_ = 1;
    else if(number > iMaxLNB_)
        iNumberOfLNB_ = iMaxLNB_;
    else
        iNumberOfLNB_ = number;
}

// class cDiseqc1_1 ---------------------------------------------------
cDiseqc1_1::cDiseqc1_1() : cDiseqcSettingBase(tr("DiSEqC 1.1"), eDiseqc1_1), iMaxLNB_(DISEQC_1_1_MAXLNB)
{
    iNumberOfLNB_ = 1;
    for (int i = 0; i< iMaxLNB_; ++i)
    {
        LNBSettingDiseqc1_1_.push_back(new cLNBSettingDiseqc1_1());
        LNBSettingDiseqc1_1_.at(i)->iCascade_ = 1+i/4;
    }
    LNBSettingDiseqc1_1_.at(1)->iSource_ = DISEQC_SOURCE_HOTBIRD_13;
    LNBSettingDiseqc1_1_.at(2)->iSource_ = DISEQC_SOURCE_ASTRA_28;
    LNBSettingDiseqc1_1_.at(3)->iSource_ = DISEQC_SOURCE_ASTRA_23;
    LNBSettingDiseqc1_1_.at(4)->iSource_ = DISEQC_SOURCE_TURKSAT_42;
    LNBSettingDiseqc1_1_.at(5)->iSource_ = DISEQC_SOURCE_HISPASAT_30W;
    LNBSettingDiseqc1_1_.at(6)->iSource_ = DISEQC_SOURCE_INTELSAT_1W;
    iMultiswitch_ = 1;
//    iToneburst_ = 0;
//    iDelay_ = 15;
//    iRepeat_ = 0;
}

cDiseqc1_1::~cDiseqc1_1()
{
}

int cDiseqc1_1::SetSettings(satellite_list_t *sat_list)
{
    satellite_info_t *sat;
    iNumberOfLNB_ = sat_list->sat_num;

    iMultiswitch_ = 1;

    for(int i=0; i<iNumberOfLNB_; ++i)
    {
        sat = sat_list->sat + i;

        if(LNBSettingDiseqc1_1_.at(i)->SetSource(sat->Name))
            return 1; // Setting Source failed

        if(LNBSettingDiseqc1_1_.at(i)->SetLNBType(sat))
            return 2; // Setting LNB type failed

        struct dvb_diseqc_master_cmd *diseqc_cmd=&sat->comp->sec.diseqc_cmd;
        if((diseqc_cmd->msg_len >= 4)
                && (diseqc_cmd->msg[0] == 0xe0)
                && (diseqc_cmd->msg[1] == 0x10)
                && (diseqc_cmd->msg[2] == 0x39)
                && ((diseqc_cmd->msg[3] & 0xf0) == 0xf0))
            LNBSettingDiseqc1_1_.at(i)->iCascade_ = (diseqc_cmd->msg[3] & 0x0f) + 1;
        else
            return 3; // Error in parsing diseqc command
    }
    return 0;
}

void cDiseqc1_1::BuildMenu(cOsdMenu *menu)
{
    if((iMultiswitch_ == 0) && (iNumberOfLNB_ > 4)) //if there is no multiswitch, then max number of LNB is 4
    {
        iMaxLNB_ = 4;
        iNumberOfLNB_ = 4;
    }
    else
        iMaxLNB_ = DISEQC_1_1_MAXLNB;

    std::string strBuff = "         ";
    strBuff.append(tr("Number of LNBs"));
    menu->Add(new cMenuEditIntRefreshItem(strBuff.c_str(), &iNumberOfLNB_, 1, iMaxLNB_));

    strBuff = "         ";
    strBuff.append(tr("Multiswitch"));
    menu->Add(new cMenuEditBoolRefreshItem(strBuff.c_str(), &(iMultiswitch_)));

    std::vector<cLNBSettingDiseqc1_1*>::iterator iterLNB = LNBSettingDiseqc1_1_.begin();
    for (int i=1; i <= iNumberOfLNB_; ++i)
    {
        strBuff = "         ";
        strBuff.append(tr("LNB"));
        strBuff = strBuff + " ";
        if (i/10 < 1)
            strBuff.append("0");
        strBuff.append(itoa(i));
        strBuff.append(" ");
        strBuff.append(tr("Diseqc$Source"));
        menu->Add(new cMenuEditSrcItem(strBuff.c_str(), &((*iterLNB)->iSource_)));

        strBuff = "                      ";
        strBuff.append(tr("Type"));
        menu->Add(new cMenuEditStraItem(strBuff.c_str(), &((*iterLNB)->iLNB_Type_), 7, LNB_Type_string));

        if(iMultiswitch_ == 1)
        {
            strBuff = "                  ";
            strBuff.append(tr("Cascade"));
            menu->Add(new cMenuEditIntItem(strBuff.c_str(), &((*iterLNB)->iCascade_), 1, MAX_PORT));
        }

        ++iterLNB;
    }

//    strBuff = "      ";
//    strBuff.append(tr("Enable Toneburst"));
//    menu->Add(new cMenuEditBoolItem(strBuff.c_str(), &iToneburst_));

//    strBuff = "      ";
//    strBuff.append(tr("Delay (ms)"));
//    menu->Add(new cMenuEditIntItem(strBuff.c_str(), &iDelay_, 15, 100));

//    strBuff = "      ";
//    strBuff.append(tr("Repeat"));
//    menu->Add(new cMenuEditIntItem(strBuff.c_str(), &iRepeat_, 0, 3));
}

void cDiseqc1_1::SetNumberOfLNB(int number)
{
    if(number < 1)
        iNumberOfLNB_ = 1;
    else if(number > iMaxLNB_)
        iNumberOfLNB_ = iMaxLNB_;
    else
        iNumberOfLNB_ = number;
}

// class cDiseqc1_2 ---------------------------------------------------
cDiseqc1_2::cDiseqc1_2() : cDiseqcSettingBase(tr("DiSEqC 1.2"), eDiseqc1_2), iMaxSource_(64)
{
    iLNB_Type_ = 5;
    iNumberOfSource_ = 1;
    for (int i = 0; i< iMaxSource_; ++i)
        iSource_.push_back(DISEQC_SOURCE_ASTRA_19);
}

cDiseqc1_2::~cDiseqc1_2()
{
}

void cDiseqc1_2::BuildMenu(cOsdMenu *menu)
{
    std::string strBuff = "         ";
    strBuff.append(tr("LNB Type"));
    menu->Add(new cMenuEditStraItem(strBuff.c_str(), &iLNB_Type_, 7, LNB_Type_string));

    strBuff = "         ";
    strBuff.append(tr("Number of sources"));
    menu->Add(new cMenuEditIntRefreshItem(strBuff.c_str(), &iNumberOfSource_, 1, iMaxSource_));

    std::vector<int>::iterator iterSource = iSource_.begin();
    for (int i=1; i <= iNumberOfSource_; ++i)
    {
        strBuff = "         ";
        strBuff.append(tr("Diseqc$Source"));
        strBuff = strBuff + " ";
        if (i/10 < 1)
            strBuff.append("0");
        strBuff.append(itoa(i));
        menu->Add(new cMenuEditSrcItem(strBuff.c_str(), &(*iterSource)));

        ++iterSource;
    }
}

// class cDiseqcGotoX -------------------------------------------------
cGotoX::cGotoX() : cDiseqcSettingBase(tr("Rotor (GotoX)"), eGotoX)
{
    iPositionMin_ = -650;
    iPositionMax_ = 650;
    iLongitude_ = 0;
    iLatitude_ = 0;
    iAutoFocus_ = 0;
}

cGotoX::~cGotoX()
{
}

int cGotoX::SetSettings(satellite_list_t *sat_list)
{
    iPositionMin_ = sat_list->sat->SatPosMin-1800;
    iPositionMax_ = sat_list->sat->SatPosMax-1800;
    iLongitude_ = sat_list->sat->Longitude;
    iLatitude_ = sat_list->sat->Latitude;
    iAutoFocus_ = sat_list->sat->AutoFocus/10;

    return 0;
}

void cGotoX::BuildMenu(cOsdMenu *menu)
{
    char strBuff[32];
    const char* AutoFocusStrings[4];
    AutoFocusStrings[0] = tr("off");
    AutoFocusStrings[1] = tr("fine");
    AutoFocusStrings[2] = tr("coarse");
    AutoFocusStrings[3] = tr("maximum");

    sprintf(strBuff, "         %s", tr("Min Position"));
    menu->Add(new cMenuEditIntpItem(strBuff, &iPositionMin_,0,1800,tr("East"),tr("West")));
    sprintf(strBuff, "         %s", tr("Max Position"));
    menu->Add(new cMenuEditIntpItem(strBuff, &iPositionMax_,0,1800,tr("East"),tr("West")));
    sprintf(strBuff, "         %s", tr("Longitude"));
    menu->Add(new cMenuEditIntpItem(strBuff, &iLongitude_,0,1800,tr("East"),tr("West")));
    sprintf(strBuff, "         %s", tr("Latitude"));
    menu->Add(new cMenuEditIntpItem(strBuff, &iLatitude_,0,900, tr("North"), tr("South")));
    sprintf(strBuff, "         %s", tr("Autofocus"));
    menu->Add(new cMenuEditStraItem(strBuff, &iAutoFocus_, 4, AutoFocusStrings));
}

// class cDisicon4 ----------------------------------------------------
cDisicon4::cDisicon4() : cDiseqcSettingBase(tr("DisiCon 4"), eDisiCon4)
{
    LNBSetting_ = new cLNBSetting();
    LNBSetting_->iLNB_Type_ = -1;
    iSource_ = DISEQC_SOURCE_ASTRA_19;
}

cDisicon4::~cDisicon4()
{
}

void cDisicon4::BuildMenu(cOsdMenu *menu)
{
    std::string strBuff = "         ";
    strBuff.append(tr("Diseqc$Source"));
    menu->Add(new cMenuEditSrcItem(strBuff.c_str(), &iSource_));
}

