/***************************************************************************
 *   Copyright (C) 2009 by Reel Multimedia;  Author:  Florian Erfurth      *
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
 * avahilog.c
 *
 ***************************************************************************/

#include "avahilog.h"

#include <vdr/tools.h>

#include <ctime>

cMutex cAvahiLog::Mutex_;
const char* cAvahiLog::Logfile_ = "/var/log/vdr-avahi.log";

void cAvahiLog::Log(const char* text, const char* sourcefile, int sourceline)
{
    cMutexLock MutexLock(&Mutex_);
    FILE* fp = fopen(Logfile_, "a");
    if(fp)
    {
        time_t t=time(NULL);
        struct tm *tm=localtime(&t);
        char date[32];
        strftime(date, sizeof(date), "%b %e %T", tm);
        if(sourcefile)
        {
            if(sourceline > 0)
                fprintf(fp, "%s - %s(%i): %s\n", date, sourcefile, sourceline, text);
            else
                fprintf(fp, "%s - %s: %s\n", date, sourcefile, text);
        }
        else
            fprintf(fp, "%s: %s\n", date, text);
        fclose(fp);
    }
    else
        esyslog("%s(%i): Couldn't open logfile %s", __FILE__, __LINE__, Logfile_);
}

