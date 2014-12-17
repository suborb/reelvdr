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
 * netcvthread.h
 *
 ***************************************************************************/

#ifndef NETCVTHREAD_H
#define NETCVTHREAD_H

#include <vdr/thread.h>
#include <vdr/tools.h>
#include <string>

class cNetCVThread : public cThread
{
    private:
        char *strCommand_;

    public:
        cNetCVThread();
        ~cNetCVThread();

        int iProgress_;
        char *buffer_;
        void Action();
        bool SetCommand(const char *strCommand);
};

#endif
