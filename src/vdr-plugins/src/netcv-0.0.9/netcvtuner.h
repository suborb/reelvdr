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
 * netcvtuner.h
 *
 ***************************************************************************/

#ifndef NETCVSLOT_H
#define NETCVSLOT_H

#include <string>
#include <vector>

#include "diseqcsetting.h"

class cNetCVTuner;

typedef std::vector<cNetCVTuner>::const_iterator tunerConstIt_t;
typedef std::vector<cNetCVTuner>::iterator tunerIt_t;

class cNetCVTuner
{
    private:
        std::string ID_;
        std::string tuner_;
        std::string satlist_;
        int preference_;
        std::string slot_;
        bool slotactive_;
        std::string type_;
        int used_;
        bool LOCK_;
        std::string frequency_;
        std::string strength_;
        std::string snr_;
        std::string ber_;
        std::string unc_;
        std::string nimcurrent_;

    public:
        cNetCVTuner();
        ~cNetCVTuner();

        bool bShowinfo_;

        void SetTunerID(std::string ID) { ID_ = ID; };
        void SetTunerName(std::string tuner) { tuner_ = tuner; };
        void SetSatList(std::string satlist) { satlist_ = satlist; };
        void SetPreference(int preference) { preference_ = preference; };
        void SetSlot(const char* slot) { slot_ = slot; };
        void SetTunerInformation(std::string tmpStdOut);
        std::string GrepValue(std::string strString, int index);
        int GrepIntValue(std::string strString, std::string strKeyword);
        std::string GetConvertedSlot();
        std::string GetID() const { return ID_; };
        std::string GetTuner() const { return tuner_; };
        std::string GetSatlist() const { return satlist_; };
        int GetPreference() const { return preference_; };
        std::string GetState();
        std::string GetSlot() const { return slot_; };
        std::string GetType() const { return type_; };
        int GetUsed() const { return used_; };
        bool GetLOCK() const { return LOCK_; };
        std::string GetFrequency() const { return frequency_; };
        std::string GetStrength() const { return strength_; };
        std::string GetSNR() const { return snr_; };
        std::string GetBER() const { return ber_; };
        std::string GetUNC() const { return unc_; };
        std::string GetNimCurrent() const { return nimcurrent_; };
};

class cNetCVTuners
{
    private:
        std::vector<cNetCVTuner> netcvtuner_;

    public:
        cNetCVTuners();
        ~cNetCVTuners();
        tunerIt_t Add() { netcvtuner_.push_back(cNetCVTuner()); return netcvtuner_.end() -1; };  //returns pointer to last item
        tunerIt_t Begin() { return netcvtuner_.begin(); };
        tunerIt_t End() { return netcvtuner_.end(); };
        int Size() { return netcvtuner_.size(); };

        void SetTunerID(std::string strString, int number) { netcvtuner_.at(number).SetTunerID(strString); };
        void SetTunerName(std::string strString, int number) { netcvtuner_.at(number).SetTunerName(strString); };
        void SetSatList(std::string satlist, int number) { netcvtuner_.at(number).SetSatList(satlist); };
        void SetPreference(int preference, int number) { netcvtuner_.at(number).SetPreference(preference); };
        void SetTunerInformation(std::string tmpStdOut, unsigned int number);
};

class cDiseqcSetting;
class cNetCVTunerPreferenceItem : public cOsdItem
{
    private:
        cNetCVTuner *Tuner_;
        cDiseqcSetting *Diseqcsetting_;

    public:
        cNetCVTunerPreferenceItem(cNetCVTuner *Tuner, cDiseqcSetting *Diseqcsetting);
        ~cNetCVTunerPreferenceItem();

        eOSState ProcessKey(eKeys Key);
};

#endif
