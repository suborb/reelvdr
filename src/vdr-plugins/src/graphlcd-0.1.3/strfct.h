/**
 *  GraphLCD plugin for the Video Disk Recorder 
 *
 *  strfct.h  -  various string functions
 *
 *  (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online de>
 *  (c) 2004 Andreas Regel <andreas.regel AT powarman de>
 **/

/***************************************************************************
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
 *   along with this program;                                              *
 *   if not, write to the Free Software Foundation, Inc.,                  *
 *   59 Temple Place, Suite 330, Boston, MA  02111-1307  USA               *
 *                                                                         *
 ***************************************************************************/

#ifndef _GRAPHLCD_FCT_H_
#define _GRAPHLCD_FCT_H_

#include <string>


char * strncopy(char * dest , const char * src , size_t n);
char * trimleft(char * str);
char * trimright(char * str);
char * trim(char * str);
std::string trim(const std::string & s);
std::string compactspace(const std::string & s);

#endif

