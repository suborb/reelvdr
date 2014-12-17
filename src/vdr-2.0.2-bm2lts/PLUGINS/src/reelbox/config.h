/***************************************************************************
 *   Copyright (C) 2005 by Reel Multimedia                                 *
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

// config.h

#ifndef PLAYER_VERSION
#define VERSION           "1.9.0";
#else
#define VERSION PLAYER_VERSION
#endif

#ifdef RBLITE
  #define BACKGROUND_IMAGE  "/etc/reel/images/audio_%d.mpv"
#else
  #define BACKGROUND_IMAGE  "/usr/share/reel/audio/audio_%d.mpv"
  #define BACKGROUND_IMAGE2  "/usr/share/reel/audio/still/audio_%d.mpv"
  #define NOSIGNAL_IMAGE    "/usr/share/reel/nosignal/nosignal_%d.mpv"
  #define NONETCEIVER_IMAGE "/usr/share/reel/nonetceiver/nonetceiver_%d.mpv"
#endif

#define REEL_SKIN_IMAGES_FOLDER "/usr/share/reel/skinreel"
#define DEFAULT_BASE "/usr/share/reel/skinreel"

