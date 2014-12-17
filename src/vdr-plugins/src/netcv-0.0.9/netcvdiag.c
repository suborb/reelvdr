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
 * netcvdiag.c
 *
 ***************************************************************************/

#include "netcvdiag.h"

#include <vdr/tools.h>
#include <mcliheaders.h>
#include <stdlib.h>

cNetCVDiag::cNetCVDiag()
{
    process_ = NULL;
    buffer_ = NULL;
}

cNetCVDiag::~cNetCVDiag()
{
    pclose(process_);
    free(buffer_);
}

bool cNetCVDiag::RunProcess()
{
    process_ = popen("netcvdiag -a", "r");
    if(process_)
    {
        return true;
    } else
        return false;
};

int cNetCVDiag::GetNetceiverCount()
{
    int iCount = -1;
    std::string tmpStdOut;
    char *ptr = NULL;

    process_ = popen("netcvdiag -c", "r");
    if(process_) {
        ptr = GetStdOut();
        if(ptr) tmpStdOut = std::string(ptr);

        while (ptr != NULL) {
            if (tmpStdOut.find("Count: ") != std::string::npos)
                iCount = GrepIntValue(tmpStdOut, "Count:");
            free(ptr);
            ptr = GetStdOut();
            if(ptr) tmpStdOut = std::string(ptr);
        }
        free(ptr);
        pclose(process_);
        return iCount;
    } else
        return -1;
};

bool cNetCVDiag::GetInformation(cNetCVDevices *netcvdevices)
{
    if(RunProcess()) {
        std::string tmpStdOut;
        char *ptr = NULL;
        int NetceiverCount;
        int i = 0;

        ptr = GetStdOut();
        if(ptr) tmpStdOut = std::string(ptr);
        while(ptr != NULL && (tmpStdOut.find("Count: ") == std::string::npos)) {
            free(ptr);
            ptr = GetStdOut();
            if(ptr) tmpStdOut = std::string(ptr);
        }
        if (ptr != NULL && (tmpStdOut.find("Count: ") != std::string::npos))
            NetceiverCount = GrepIntValue(tmpStdOut, "Count:");
        else {
            esyslog("ERROR netcv(%s/%i): Couldn't get NetceiverCount from netcvdiag '%s'", __FILE__, __LINE__, tmpStdOut.c_str());
            return false;
        }

        for (i=0; i < NetceiverCount; ++i) {
            devIt_t it = netcvdevices->Add();
            while (ptr != NULL && tmpStdOut.find("Tuner") == std::string::npos) {
                (*it)->SetDeviceInformation(tmpStdOut);
                free(ptr);
                ptr = GetStdOut();
                if(ptr) tmpStdOut = std::string(ptr);
            }

            netceiver_info_list_t *nc_list = nc_get_list();
            nc_lock_list();

            for(int n=0; n<nc_list->nci_num; ++n)
            {
                netceiver_info_t *nci = nc_list->nci + n;
                if(!strcmp(nci->uuid, (*it)->GetUUID()))
                {
                    // Tuners
                    for(int i=0; i<nci->tuner_num; ++i)
                    {
                        tunerIt_t tunerIter = (*it)->AddTuner();
                        char buffer[32];
                        int slot;

                        snprintf(buffer, 32, "%s %i", tr("Tuner"), i);
                        (*tunerIter).SetTunerID(buffer);
                        (*tunerIter).SetTunerName(nci->tuner[i].fe_info.name);
                        (*tunerIter).SetPreference(nci->tuner[i].preference);
                        (*tunerIter).SetSatList(nci->tuner[i].SatelliteListName);
                        slot = nci->tuner[i].slot;
                        buffer[0] = '0' + slot/2;
                        buffer[1] = '.';
                        buffer[2] = '0' + slot%2;
                        buffer[3] = '\0';
                        (*tunerIter).SetSlot(buffer);
                    }

                    // Cams
                    for(int i=0; i<nci->cam_num; ++i)
                        camIt_t camIter = (*it)->cams_.Add(&(nci->cam[i]));
                }
            }
            nc_unlock_list();

            // A) Skip while lines contains "UUID <"
            // B) Don't skip if "NetCeiver" is found but not "SatList <"
            while (ptr != NULL
                    && (tmpStdOut.find("UUID <") == std::string::npos) // A)
                    && !((tmpStdOut.find("NetCeiver") != std::string::npos) && (tmpStdOut.find("SatList <") == std::string::npos)) // B)
                    ) {
                free(ptr);
                ptr = GetStdOut();
                if(ptr) tmpStdOut = std::string(ptr);
            }
        }

        while(ptr != NULL) {
            i=0;
            devIt_t it = netcvdevices->Begin();
            while (it != netcvdevices->End() && tmpStdOut.find((*it)->GetUUID()) == std::string::npos) {
                ++it;
                ++i;
            }
            free(ptr);
            ptr = GetStdOut();
            if(ptr) tmpStdOut = std::string(ptr);

            if(i == NetceiverCount) {
                esyslog("ERROR netcv(%s/%i): Couldn't get Tuner Information from netcvdiag", __FILE__, __LINE__);
                return false;
            } else
            {
                //TODO: better access directly to mcli-data like above
                int number = -1;
                int index = tmpStdOut.find("slot ")+5;
                if(index!=-1)
                {
                    int i=0;
                    for (tunerIt_t tunerIter = (*it)->tuners_.Begin(); tunerIter != (*it)->tuners_.End(); ++tunerIter)
                    {
                        if((tmpStdOut.at(index)==tunerIter->GetSlot().at(0)) && (tmpStdOut.at(index+2)==tunerIter->GetSlot().at(2)))
                        {
                            number = i;
                            break;
                        }
                        ++i;
                    }
                }

                while (ptr != NULL && tmpStdOut.find("UUID <") == std::string::npos) {
                    if(number!=-1)
                        (*it)->SetTunerInformation(tmpStdOut, number);
                    free(ptr);
                    ptr = GetStdOut();
                    if(ptr) tmpStdOut = std::string(ptr);
                }
            }
        }

        return true; //successful
    } else {
        esyslog("ERROR netcv(%s/%i): Couldn't run 'netcvdiag -a'", __FILE__, __LINE__);
        return false; //failed
    }
}

char *cNetCVDiag::GetStdOut(){
    cReadLine rl;
    buffer_ = NULL;
    if(process_)
        asprintf(&buffer_, rl.Read(process_));
    if(buffer_ != NULL)
        return buffer_;
    else
        return NULL;
}

int cNetCVDiag::GrepIntValue(std::string strString, std::string strKeyword)
{
    int index = strString.find(strKeyword, 0);
    std::string::size_type length;

    if (index == -1)
        return -1;
    else
    {
        index += strKeyword.length() + 1;
        length = strString.find(" ", index);
        if (length != std::string::npos)
            length = length - index - 1;
        strString.assign(strString, index, length);

        return atoi(strString.c_str());
    }
}

