/**
 * kover - Kover is an easy to use WYSIWYG CD cover printer with CDDB support.
 * Copyright (C) 2004 Adrian Reber
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: cdtext.h,v 1.2 2004/09/20 14:56:03 adrian Exp $
 *
 * $Author: adrian $
 */

#ifndef CDTEXT_H
#define CDTEXT_H

#include "./cdrom.h"
#include <string>
#include <map>

using namespace std;

class cCdText:public cdrom {

      public:
    void dump();
    void set_track_title(int tracknumber, string title);
    void set_track_performer(int tracknumber, string performer);
    void add_disc_title(string title);
    void add_disc_performer(string performer);
    void add_disc_discid(string discid);
    void add_disc_upc(string upc);
     cCdText(char *path);
    int read_cdtext();
        int get_disc_track_count();
    string get_name(int track);
    string get_performer(int track);
    string get_disc_title();
    string get_disc_performer();
    string get_disc_discid();
    string get_disc_upc();

      private:
     string disc_title;
    string disc_performer;
    string disc_discid;
    string disc_upc;
     map < int, string > track_name;
     map < int, string > track_performer;
    int parse_cdtext(unsigned char *buffer);
    int save_cdtext(char code, char track_no, char *data);
    unsigned short from2Byte(unsigned char *d);
};

#endif
