/**************************************************************************
*   Copyright (C) 2008 by Reel Multimedia                                 *
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
***************************************************************************/

#ifndef RBM_DEBUG_H_INCLUDED
#define RBM_DEBUG_H_INCLUDED

#define DBG_NONE   0
#define DBG_STDOUT 1
#define DBG_SYSLOG 2

#define DBG(level, text...) {if(DBG_STDOUT==level){printf(text);printf("\n");}else if(DBG_SYSLOG==level){isyslog(text);}}

#endif /*RBM_DEBUG_H_INCLUDED*/
