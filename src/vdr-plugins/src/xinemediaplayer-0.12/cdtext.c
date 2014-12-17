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
 * $Id: cdtext.cc,v 1.4 2004/09/20 14:56:03 adrian Exp $
 *
 * $Author: adrian $
 */

#include "./cdtext.h"
#include <stdio.h>
#include <iostream>

void cCdText::set_track_title(int tracknumber, string title)
{
    track_name[tracknumber] = title;
}

void cCdText::set_track_performer(int tracknumber, string performer)
{
    track_performer[tracknumber] = performer;
}

void cCdText::add_disc_title(string title)
{
    disc_title = title;
}

void cCdText::add_disc_performer(string performer)
{
    disc_performer = performer;
}

void cCdText::add_disc_discid(string discid)
{
    disc_discid = discid;
}

void cCdText::add_disc_upc(string upc)
{
    disc_upc = upc;
}

cCdText::cCdText(char *path):cdrom(path)
{
    disc_title = "";
    disc_performer = "";
    disc_discid = "";
    disc_upc = "";
}

int cCdText::get_disc_track_count()
{
    return track_name.size() == track_performer.size()? track_performer.size() : 0;
}
void cCdText::dump()
{
    cout << "disc_title=" << disc_title << endl;
    cout << "disc_performer=" << disc_performer << endl;
    cout << "disc_discid=" << disc_discid << endl;
    cout << "disc_upc=" << disc_upc << endl;
    map < int, string >::iterator cur;

    for (cur = track_name.begin(); cur != track_name.end(); cur++) {
        if ((*cur).second == "")
            continue;
        cout << "track number " << (*cur).first << " has title=" << (*cur).second << endl;
    }
    for (cur = track_performer.begin(); cur != track_performer.end();
        cur++) {
        if ((*cur).second == "")
            continue;
        cout << "track number " << (*cur).first << " has performer=" << (*cur).second << endl;
    }
}

//from k3b
int cCdText::read_cdtext()
{
    struct cdrom_generic_command m_cmd;

    int dataLen;

    int format = 5;
    bool time = false;
    int track = 0;
    unsigned char header[2048];

    if(open())
        return -1;

    m_cmd.cmd[0] = 0x43;
    m_cmd.cmd[1] = (time ? 0x2 : 0x0);
    m_cmd.cmd[2] = format & 0x0F;
    m_cmd.cmd[6] = track;
    m_cmd.cmd[8] = 2;       // we only read the length first

    m_cmd.buffer = (unsigned char *) header;
    m_cmd.buflen = 2;
    m_cmd.data_direction = CGC_DATA_READ;

    if (ioctl(cdrom_fd, CDROM_SEND_PACKET, &m_cmd)) {
        fprintf(stderr, "%s:%d:error accessing cdrom drive!\n", __FILE__,__LINE__);
        return -1;
    }

    dataLen = from2Byte(header) + 2;
    m_cmd.cmd[7] = 2048 >> 8;
    m_cmd.cmd[8] = (char)2048;
    m_cmd.buflen = 2048;
    ::ioctl(cdrom_fd, CDROM_SEND_PACKET, &m_cmd);
    dataLen = from2Byte(header) + 2;

    ::memset(header, 0, dataLen);

    m_cmd.cmd[7] = dataLen >> 8;
    m_cmd.cmd[8] = dataLen;
    m_cmd.buffer = (unsigned char *) header;
    m_cmd.buflen = dataLen;
    ::ioctl(cdrom_fd, CDROM_SEND_PACKET, &m_cmd);

    return parse_cdtext(header);

    // return 0;
}

#define SIZE 61
//from k3b and xsadp (which has it from cdda2wav)
int cCdText::parse_cdtext(unsigned char *buffer)
{
    short pos_buffer2;
    char block_no, old_block_no, dbcc, track, code, c;
    int length, rc, j, buffer_size;
    char buffer2[SIZE];
    unsigned char *bufptr, *txtstr;

    rc = 0;
    bufptr = buffer;
    buffer_size = (buffer[0] << 8) | buffer[1]; 
    buffer_size -= 2; 

    pos_buffer2 = 0;
    old_block_no = 0xff;

    for (bufptr = buffer + 4; buffer_size >= 18;
            bufptr += 18, buffer_size -= 18) {
        code = *bufptr;

        if ((code & 0x80) != 0x80)
            continue;

        block_no = *(bufptr + 3);
        dbcc = block_no & 0x80;
        block_no &= 0x70;  

        if (block_no != old_block_no) {
            if (rc)
                break;
            pos_buffer2 = 0;
            old_block_no = block_no;
        }

        if (dbcc) {
            fprintf(stderr,"%s:%d: Double byte code not supported\n",__FILE__,__LINE__);
            return 1;
        }

        track = *(bufptr + 1);
        if (track & 0x80)
            continue;   

        txtstr = bufptr + 4; 

        for (length = 11;
                length >= 0 && *(txtstr + length) == '\0';
                length--);


        length++;
        if (length < 12)
            length++;   

        for (j = 0; j < length; j++) {
            c = *(txtstr + j);

            if (c == '\0') {
                buffer2[pos_buffer2] = c;
                if (save_cdtext(code, track,
                            (char *) buffer2))
                    rc = 1;

                pos_buffer2 = 0;
                track++;
            } else if (pos_buffer2 < (SIZE - 1))
                buffer2[pos_buffer2++] = c;
        }

    }

    return rc;
}

//inspired by k3b
int cCdText::save_cdtext(char code, char track_no, char *data)
{
    if (track_no == 0) {
        if (code == (char) 0xFFFFFF80)
            add_disc_title(data);
        if (code == (char) 0xFFFFFF81)
            add_disc_performer(data);
        if (code == (char) 0xFFFFFF86) 
            add_disc_discid(data);
        if (code == (char) 0xFFFFFF8E)
            add_disc_upc(data);
    } else {
        if (code == (char) 0xFFFFFF80)
            set_track_title(track_no, data);
        if (code == (char) 0xFFFFFF81)
            set_track_performer(track_no, data);
    }
    return 1;
}

//from k3b
unsigned short cCdText::from2Byte(unsigned char *d)
{
            return (((d[0] << 8) & 0xFF00) | (d[1] & 0xFF));
}

string cCdText::get_name(int track){
    return track_name[track];
}
string cCdText::get_performer(int track){
    return track_performer[track];
}
string cCdText::get_disc_title(){
    return disc_title;
}
string cCdText::get_disc_performer(){
    return disc_performer;
}
string cCdText::get_disc_discid(){
    return disc_discid;
}
string cCdText::get_disc_upc(){
    return disc_upc;
}

