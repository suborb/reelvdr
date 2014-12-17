/*
 * vdr-ttxtsubs - A plugin for the Linux Video Disk Recorder
 * Copyright (c) 2003 - 2008 Ragnar Sundblad <ragge@nada.kth.se>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <ctype.h>
#include <string.h>

//#include <linux/dvb/dmx.h>
//#include <vdr/osd.h>
#include <vdr/config.h>

#include "teletext.h"
#include "teletext-tables.h"
#include "teletext-chars.h"
#include "utils.h"


// FROM vbidecode
// unham 2 bytes into 1, report 2 bit errors but ignore them
unsigned char unham(unsigned char a,unsigned char b)
{
  unsigned char c1,c2;
  
  c1=unhamtab[a];
  c2=unhamtab[b];
//  if ((c1|c2)&0x40) 
//      dprint("bad ham!");
  return (c2<<4)|(0x0f&c1);
}


// ham 8/4 the byte in into two bytes pointed to by out
// should make a table instead
void ham8_4byte(uint8_t in, uint8_t *out)
{
  out[0] = ham8_4nibble(in & 0xF);
  out[1] = ham8_4nibble(in >> 4);

  if (1) { // XXX debug
    int a;
    if(unham(out[0], out[1]) != in) {
      dprint("ham8_4: 1 - result not correct %02x -> %02x %02x!\n", in, out[0], out[1]);
    }
    a = unhamtab[out[0]];
    a ^= 0x80;
    if(a & 0xf0 || (a != (in & 0xF))) {
      dprint("ham8_4: 2 - result not correct %02x -> %02x %02x, %02x!\n", in, out[0], out[1], a);
    }
    a = unhamtab[out[1]];
    a ^= 0x80;
    if(a & 0xf0 || (a != (in >> 4))) {
      dprint("ham8_4: 3 - result not correct %02x -> %02x %02x, %02x!\n", in, out[0], out[1], a);
    }
  }
}

// should be a table instead
int parity(uint8_t x) {
  int res = 1;
  int count = x & 0xf0 ? 8 : 4;

  while(count--) {
    if(x & 0x01)
      res = !res;
    x >>= 1;
  }

  return res;
}

// ham 8/4 the nibble in into the byte pointed to by out
// should make a table instead
// D4 P4 D3 P3 D2 P2 D1 P1
// P4 = all the rest
uint8_t ham8_4nibble(uint8_t in)
{
  uint8_t o = 0;

  // insert the data bits
  o |= (in << 4) & 0x80;
  o |= (in << 3) & 0x20;
  o |= (in << 2) & 0x08;
  o |= (in << 1) & 0x02;

  if(parity(in & 0x0d)) // P1 = 1 . D1 . D3 . D4
    o |= 0x01;
  if(parity(in & 0x0b)) // P2 = 1 . D1 . D2 . D4
    o |= 0x04;
  if(parity(in & 0x07)) // P3 = 1 . D1 . D2 . D3
    o |= 0x10;
  if(parity(o & 0xbf)) // P4 = 1 . P1 . D1 . ... . D3 . D4
    o |= 0x40;

  return o;
}


/*
 * Map Latin G0 teletext characters into a ISO-8859-1 approximation.
 * Trying to use similar looking or similar meaning characters.
 * Gtriplet - 4 bits = triplet-1 bits 14-11 (14-8) of a X/28 or M/29 packet, if unknown - use 0
 * natopts - 3 bits = national_options field of a Y/0 packet (or triplet-1 bits 10-8 as above?)
 * inchar - 7 bits = characted to remap
 * Also strips parity
 */

uint16_t ttxt_laG0_la1_char(int Gtriplet, int natopts, uint8_t inchar)
{
  int no = laG0_nat_opts_lookup[Gtriplet & 0xf][natopts & 0x7];
  uint8_t c = inchar & 0x7f;

  //dprint("\n%x/%c/%x/%x\n", c, c, laG0_nat_replace_map[c], laG0_nat_opts[no][laG0_nat_replace_map[c]]);

  if(!laG0_nat_replace_map[c])
    return c;
  else
    if (cCharSetConv::SystemCharacterTable())
       return laG0_nat_opts[no][laG0_nat_replace_map[c]];
    else
       return laG0_nat_opts16[no][laG0_nat_replace_map[c]];
}
