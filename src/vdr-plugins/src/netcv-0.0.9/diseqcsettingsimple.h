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
 * diseqcsettingsimple.h
 *
 ***************************************************************************/

#ifndef DISEQCSETTINGSIMPLE_H
#define DISEQCSETTINGSIMPLE_H

#include <vector>
#include "diseqcsetting.h"

#define NUMBER_OF_SLOTS 3
#define LNB_MAX 16

enum eTunerSimpleMode { eTunerDiseqc, eTunerSingleCable /*EN50494*/, eTunerRotor };

class cNetCVTunerSimple
{
    public:
        cNetCVTunerSimple(bool Diseqc);
        ~cNetCVTunerSimple();

        int Enabled_;
        int Mode_;
        // Rotor
        int PositionMin_;
        int PositionMax_;
        int Longitude_;
        int Latitude_;
        int AutoFocus_;
        // DiSEqC
        bool Diseqc_;
        int NumberOfLNB_;
        std::vector<int> Sources_;

        // Single cable support, properties
        SingleCableRouter_t scrProps;
};

class cNetCVTunerSlot
{
    public:
        cNetCVTunerSlot(const char *Name, const char* TunerType);
        ~cNetCVTunerSlot();

        const char *Name_;
        const char* Type_;
        std::vector<cNetCVTunerSimple*> Tuners_;
};

class cDiseqcSettingSimple
{
    private:
        cDiseqcSetting *DiseqcSetting_;
        cNetCVTuners *ExpertTuners_;

    public:
        cDiseqcSettingSimple(cDiseqcSetting *DiseqcSetting, cNetCVTuners *Tuners);
        ~cDiseqcSettingSimple();

        std::vector<cNetCVTunerSlot*> TunerSlots_;
        void WriteSetting();
};

#endif
