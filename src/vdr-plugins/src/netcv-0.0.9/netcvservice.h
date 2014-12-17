/***************************************************************************
 *   Copyright (C) 2008 by Reel Multimedia;  Author:  Florian Erfurth      *
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
 * netcvservice.h
 *
 ***************************************************************************/

#ifndef NETCVSERVICE_H
#define NETCVSERVICE_H

#include <vector>

// Common Menu-Remotecall
struct netcv_service_data
{
    char *title;
    cOsdMenu *menu;
};

// Tunerlist
enum eTunerType { eSattuner, eSatS2tuner, eCabletuner, eTerrtuner };
struct netcv_service_tunerlist_data_tuner
{
    int Tuner;
    bool Active;
    int TunerType; // See enumerator eTunerType
    char *Name;
};

struct netcv_service_tunerlist_data
{
    // Input
    bool SatTuner;
    bool SatS2Tuner;
    bool CableTuner;
    bool TerrTuner;
    // Output
    std::vector<netcv_service_tunerlist_data_tuner> Tuners;
};

// Rotor setting store
struct netcv_service_storerotor_data
{
    int Tuner;
    int Longitude;
    int Latitude;
    int PositionMin;
    int PositionMax;
    int Autofocus;
};


#endif
