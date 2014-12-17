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
 * diseqcsettingsimple.c
 *
 ***************************************************************************/

#include <vdr/diseqc.h>
#include "diseqcsettingsimple.h"
#include "diseqcsettingitem.h"

#define SATLIST_PREFIX "Tuner"

// class cNetCVTunerSimple --------------------------------------------
cNetCVTunerSimple::cNetCVTunerSimple(bool Diseqc)
{
    Enabled_ = 0;
    Mode_ = eTunerDiseqc;
    // Rotor
    PositionMin_ = -650;
    PositionMax_ = 650;
    Longitude_ = 0;
    Latitude_ = 0;
    AutoFocus_ = 0;

    // DiSEqC
    Diseqc_ = Diseqc;
    NumberOfLNB_ = 1;
    for(unsigned int i=0; i<LNB_MAX; ++i)
        Sources_.push_back(DISEQC_SOURCE_ASTRA_19); // Diseqcs.First()->Source() => 35008?
    Sources_.at(1) = DISEQC_SOURCE_HOTBIRD_13;
    Sources_.at(2) = DISEQC_SOURCE_ASTRA_28;
    Sources_.at(3) = DISEQC_SOURCE_ASTRA_23;
    Sources_.at(4) = DISEQC_SOURCE_TURKSAT_42;
    Sources_.at(5) = DISEQC_SOURCE_HISPASAT_30W;
    Sources_.at(6) = DISEQC_SOURCE_INTELSAT_1W;
}

cNetCVTunerSimple::~cNetCVTunerSimple()
{
    Sources_.clear();
}

// class cNetCVTunerSlot ----------------------------------------------
cNetCVTunerSlot::cNetCVTunerSlot(const char *Name, const char *TunerType) : Name_(Name), Type_(TunerType)
{
}

cNetCVTunerSlot::~cNetCVTunerSlot()
{
    for(unsigned int i=0; i < Tuners_.size(); ++i)
        delete Tuners_.at(i);
    Tuners_.clear();
}

// class cDiseqcSettingSimple -----------------------------------------
cDiseqcSettingSimple::cDiseqcSettingSimple(cDiseqcSetting *DiseqcSetting, cNetCVTuners *Tuners)
{
    ExpertTuners_ = Tuners;
    DiseqcSetting_ = DiseqcSetting;

    bool found;

    for(int i=0; i < NUMBER_OF_SLOTS; ++i)
    {
        found = false;
        tunerIt_t tunerIter = ExpertTuners_->Begin();
	if (tunerIter!=ExpertTuners_->End()){ // List can be empty
		do
		{		
			if(tunerIter->GetSlot().at(0) == '0'+i)
				found = true;
			else
				++tunerIter;
		}

		while(!found && tunerIter != ExpertTuners_->End());
	}

        if(found)
        {
            cNetCVTunerSlot *TunerSlot = new cNetCVTunerSlot(tunerIter->GetTuner().c_str(), tunerIter->GetType().c_str());
            TunerSlots_.push_back(TunerSlot);
            while(tunerIter != ExpertTuners_->End())
            {
                if(tunerIter->GetSlot().at(0) == '0'+i)
                {
                    bool isSatTuner = false;
                    if((tunerIter->GetType() == "DVB-S") || (tunerIter->GetType() == "DVB-S2"))
                        isSatTuner = true;
                    cNetCVTunerSimple *Tuner = new cNetCVTunerSimple(isSatTuner);

                    //Load Settings
                    Tuner->Enabled_ = tunerIter->GetPreference()>=0?1:0;
                    if(isSatTuner)
                    {
                        std::vector<cSatList*> *Satlists = DiseqcSetting->GetSatlistVector();
                        found = false;
                        unsigned int j=0;
                        do
                        {
                            if(!strcmp(Satlists->at(j)->Name_, tunerIter->GetSatlist().c_str()))
                                found = true;
                            else
                                ++j;
                        }
                        while(!found && j < Satlists->size());

                        if(found)
                        {
                            std::vector<cDiseqcSettingBase*>::iterator modeIter = Satlists->at(j)->GetModeIterator();
                            switch((*(modeIter))->mode_)
                            {
                                case eDiseqcDisabled:
                                case eDiseqcMini:
                                    // not implemented
                                    break;
                                case eDiseqc1_0:
                                case eDiseqc1_1:
                                    Tuner->NumberOfLNB_ = (*modeIter)->GetNumberOfLNB();
                                    for(int k=0; k<Tuner->NumberOfLNB_; ++k)
                                        Tuner->Sources_.at(k) = (*modeIter)->GetLNBSetting(k)->iSource_;
                                    break;

                                case eDiseqc1_2:
                                case eGotoX:
                                    {
                                        cGotoX *GotoX = static_cast<cGotoX*>(*modeIter);
                                        if(Tuner->Mode_ == eTunerDiseqc)
                                            Tuner->Mode_ = eTunerRotor;
                                        Tuner->PositionMin_ = GotoX->iPositionMin_;
                                        Tuner->PositionMax_ = GotoX->iPositionMax_;
                                        Tuner->Longitude_ = GotoX->iLongitude_;
                                        Tuner->Latitude_ = GotoX->iLatitude_;
                                        Tuner->AutoFocus_ = GotoX->iAutoFocus_;
                                    }
                                    break;
                                case eDisiCon4:
                                    // not implemented
                                    break;
                            case eSingleCableRouter:
                            {
                                Tuner->NumberOfLNB_ = (*modeIter)->GetNumberOfLNB();
                                for(int k=0; k<Tuner->NumberOfLNB_; ++k)
                                    Tuner->Sources_.at(k) = (*modeIter)->GetLNBSetting(k)->iSource_;

                                cDiseqcSCR *diseqcSCR = dynamic_cast<cDiseqcSCR *> (*modeIter);
                                if (diseqcSCR)
                                {
                                    Tuner->scrProps = diseqcSCR->scrProps;
                                    Tuner->Mode_ = eTunerSingleCable;
                                }
                            }
                            break;
                            }
                        }
                    }

                    TunerSlot->Tuners_.push_back(Tuner);
                }
                ++tunerIter;
            }
        }
        else
            TunerSlots_.push_back(new cNetCVTunerSlot("", ""));
    }
}

cDiseqcSettingSimple::~cDiseqcSettingSimple()
{
    for(unsigned int i=0; i < TunerSlots_.size(); ++i)
        delete TunerSlots_.at(i);
    TunerSlots_.clear();
}

void cDiseqcSettingSimple::WriteSetting()
{
    char strBuff[256];
    bool found;
    int TunerCount = 0;

    for(unsigned int i=0; i < TunerSlots_.size(); ++i)
    {
        cNetCVTunerSlot *TunerSlot = TunerSlots_.at(i);

        found = false;
        tunerIt_t tunerIter = ExpertTuners_->Begin();
        do
        {
            if(tunerIter->GetSlot().at(0) == (char)('0'+i))
                found = true;
            else
                ++tunerIter;
        }
        while(!found && tunerIter != ExpertTuners_->End());

        for(unsigned int j=0; j < TunerSlot->Tuners_.size(); ++j)
        {
            cNetCVTunerSimple *Tuner = TunerSlot->Tuners_.at(j);
            eDiseqcMode DiseqcMode = eDiseqcDisabled;

            if(Tuner->Diseqc_)
            {
                if(Tuner->Enabled_)
                {
                    if(Tuner->Mode_ == eTunerDiseqc)
                    {
                        tunerIter->SetPreference(0);
                        if(Tuner->NumberOfLNB_ < 5)
                            DiseqcMode = eDiseqc1_0;
                        else
                            DiseqcMode = eDiseqc1_1;
                    }
                    else if(Tuner->Mode_ == eTunerSingleCable)
                    {
                        tunerIter->SetPreference(0);
                        DiseqcMode = eSingleCableRouter;
                    }
                    else if(Tuner->Mode_ == eTunerRotor)
                    {
                        tunerIter->SetPreference(0);
                        DiseqcMode = eGotoX;
                    }
                }
                else
                    tunerIter->SetPreference(-1);

                found = false;
                unsigned int j=0;
                sprintf(strBuff, "%s%i", SATLIST_PREFIX, ++TunerCount);
                do
                {
                    if(!strcmp(DiseqcSetting_->GetSatlistVector()->at(j)->Name_, strBuff))
                        found = true;
                    else
                        ++j;
                }
                while(!found && j < DiseqcSetting_->GetSatlistVector()->size());

                if(!found)
                    DiseqcSetting_->AddSatlist(strBuff);

                tunerIter->SetSatList(strBuff);
                DiseqcSetting_->GetSatlistVector()->at(j)->SetMode(DiseqcMode);
                std::vector<cDiseqcSettingBase*>::iterator modeIter = DiseqcSetting_->GetSatlistVector()->at(j)->GetModeIterator();
                switch(DiseqcMode)
                {
                    case eDiseqcDisabled:
                    case eDiseqcMini:
                        // not implemented
                        break;
                    case eDiseqc1_0:
                        (*modeIter)->SetNumberOfLNB(Tuner->NumberOfLNB_);
                        for(int x=0; x < Tuner->NumberOfLNB_; ++x)
                            (*modeIter)->GetLNBSetting(x)->iSource_ = Tuner->Sources_.at(x);
                        break;
                    case eDiseqc1_1:
                        {
                            cDiseqc1_1 *Diseqc1_1 = static_cast<cDiseqc1_1*>(*modeIter);
                            Diseqc1_1->iMultiswitch_ = 1;
                            Diseqc1_1->SetNumberOfLNB(Tuner->NumberOfLNB_);
                            for(int x=0; x < Tuner->NumberOfLNB_; ++x)
                            {
                                Diseqc1_1->GetLNBSetting(x)->iSource_ = Tuner->Sources_.at(x);
                                Diseqc1_1->GetLNBSetting(x)->iCascade_ = (x/4)+1;
                            }
                        }
                        break;
                    case eDiseqc1_2:
                        // not implemented
                        break;
                    case eGotoX:
                        {
                            cGotoX *GotoX = static_cast<cGotoX*>(*modeIter);
                            GotoX->iPositionMin_ = Tuner->PositionMin_;
                            GotoX->iPositionMax_ = Tuner->PositionMax_;
                            GotoX->iLongitude_ = Tuner->Longitude_;
                            GotoX->iLatitude_ = Tuner->Latitude_;
                            GotoX->iAutoFocus_ = Tuner->AutoFocus_;
                        }
                        break;
                    case eDisiCon4:
                        // not implemented
                        break;
                    case eSingleCableRouter:
                     {
                        /* copy */
                           cDiseqcSCR *diseqcSCR = dynamic_cast<cDiseqcSCR*> (*modeIter);
                           diseqcSCR->scrProps = Tuner->scrProps;

                           diseqcSCR->SetNumberOfLNB(Tuner->NumberOfLNB_);
                           for(int i=0; i < Tuner->NumberOfLNB_; ++i)
                           {
                               diseqcSCR->GetLNBSetting(i)->iSource_ = Tuner->Sources_.at(i);
                               //diseqcSCR->GetLNBSetting(x)->iCascade_ = (x/4)+1;
                           }

                      }
                        break;

                }

            }
            else
            {
                if(Tuner->Enabled_)
                    tunerIter->SetPreference(0);
                else
                    tunerIter->SetPreference(-1);
            }
           
            ++tunerIter;
        }
    }
}
