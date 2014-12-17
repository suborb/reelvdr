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


const uint MAX_CACHE_SIZE =   30000000;
const uint TMP_CACHE_SIZE =   500000;
const char *const IMAGE_CONV = "image_convert.sh";
const char *const PLAYLIST_DEFAULT_NAME = "myplaylist";

#ifdef RBLITE
const char *const MOUNT_SH = "/sbin/mount.sh";
const char *const IMAGE_CACHE = "/var/cache/images/";
const char *const BROWSER_CACHE = "/opt/tmp/browsercache";
const char *const TMP_CACHE = "/tmp/browsercache";
#else
const char *const MOUNT_SH = "/usr/sbin/mount.sh";
const char *const IMAGE_CACHE = "/tmp/browsercache/";
const char *const BROWSER_CACHE = "/var/cache/images";
const char *const TMP_CACHE = "/tmp/browsercache";
#endif

const char *const BASE_DIR = "/media";
const char *const MEDIA_DIR = "/media/reel";
const char *const MUSIC_DIR = "/media/reel/music";
const char *const PICTURES_DIR = "/media/reel/pictures";
const char *const VIDEO_DIR = "/media/reel/video";
const char *const RECORDINGS_DIR = "/media/reel/recordings";
const char *const PLAYLIST_DEFAULT_PATH ="/media/reel/music/playlists";

/*const char *const MEDIA_DIR = "/media/hd";
const char *const MUSIC_DIR = "/media/hd/music";
const char *const PICTURES_DIR = "/media/hd/pictures";
const char *const VIDEO_DIR = "/media/hd/video";
const char *const RECORDINGS_DIR = "/media/hd/recordings";
const char *const PLAYLIST_DEFAULT_PATH ="/media/hd";*/

const char *const dvdPath = "/media/dvd";

