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
 * netcvthread.c
 *
 ***************************************************************************/

#include "netcvthread.h"
#include <vdr/i18n.h>

cNetCVThread::cNetCVThread()
    : cThread("NetCeiver Update")
{
    iProgress_ = -1;
    strCommand_ = NULL;
    buffer_ = NULL;
}

cNetCVThread::~cNetCVThread()
{
    if(strCommand_)
        free(strCommand_);
}

bool cNetCVThread::SetCommand(const char *strCommand)
{
    if (!Running())
    {
        if(strCommand_)
            free(strCommand_);
        strCommand_ = strdup(strCommand);
        return true;
    }
    return false;
}

void cNetCVThread::Action()
{
    iProgress_ = 0;
    FILE *process;
    process = popen(strCommand_, "r");
    if (process)
    {
        cReadLine readline_;

        buffer_ = readline_.Read(process);
        while (Running() && buffer_)
        {
            std::string strBuff(buffer_);
            if (strBuff.find("Upload") == 0)
            {
                strBuff.assign(strBuff, 7, strBuff.find("%") - 7);
                iProgress_ = (atoi(strBuff.c_str()) * 50) / 100;
            }
            else if (strBuff.find("Install") == 0)
            {
                strBuff.assign(strBuff, 8, strBuff.find("%") - 8);
                iProgress_ = 50 + (atoi(strBuff.c_str()) * 50) / 100;
            }
            else
            {
                strBuff = buffer_;
                if (strBuff.find("UPDATE RESULT:") == 0)
                iProgress_ = 100;
            }
            buffer_ = readline_.Read(process);
        }
    }
    pclose(process);
    iProgress_ = -2; // update was sucessful
    std::string strBuff = "svdrpsend.sh mesg ";
    strBuff.append(tr("Update finished"));
    SystemExec(strBuff.c_str());
}

