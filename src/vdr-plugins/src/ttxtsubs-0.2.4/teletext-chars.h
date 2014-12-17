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

/*
 * Character mappings
 * An attempt to map Teletext characters into ISO-8859-1.
 * Trying to use similar looking or similar meaning
 * characters.
 */


/*
 * G0 and G2 national option table given Triplet 1 bits 14-11 and Control bits from C12-14
 * ETSI EN 300 706 Table 33
 */
uint8_t laG0_nat_opts_lookup[16][8] = {
  {1, 4, 11, 5, 3, 8, 0, 1},
  {7, 4, 11, 5, 3, 1, 0, 1},
  {1, 4, 11, 5, 3, 8, 12, 1},
  {1, 1, 1, 1, 1, 10, 1, 9},
  {1, 4, 2, 6, 1, 1, 0, 1},
  {1, 1, 1, 1, 1, 1, 1, 1}, // 5 - reserved
  {1, 1, 1, 1, 1, 1, 12, 1},
  {1, 1, 1, 1, 1, 1, 1, 1}, // 7 - reserved
  {1, 1, 1, 1, 3, 1, 1, 1},
  {1, 1, 1, 1, 1, 1, 1, 1}, // 9 - reserved
  {1, 1, 1, 1, 1, 1, 1, 1},
  {1, 1, 1, 1, 1, 1, 1, 1}, // 11 - reserved
  {1, 1, 1, 1, 1, 1, 1, 1}, // 12 - reserved
  {1, 1, 1, 1, 1, 1, 1, 1}, // 13 - reserved
  {1, 1, 1, 1, 1, 1, 1, 1}, // 14 - reserved
  {1, 1, 1, 1, 1, 1, 1, 1}  // 15 - reserved
};


uint8_t laG0_nat_replace_map[128] = {
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 5, 6, 7, 8,
9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 0
};


/*
 * Latin National Option Sub-Sets
 * ETSI EN 300 706 Table 36
 */

uint8_t laG0_nat_opts[13][14] = {
{0, '#', 'u', 'c', 't', 'z', 'ý', 'í', 'r', 'é', 'á', 'e', 'ú', 's'}, // 0 - Czech/Slovak
{0, '£', '$', '@', '-', '½', '-', '|', '#', '-', '¼', '#', '¾', '÷'}, // 1 - English
{0, '#', 'õ', 'S', 'Ä', 'Ö', 'Z', 'Ü', 'Õ', 's', 'ä', 'ö', 'z', 'ü'}, // 2 - Estonian
{0, 'é', 'ï', 'à', 'ë', 'ê', 'ù', 'î', '#', 'è', 'â', 'ô', 'û', 'ç'}, // 3 - French
{0, '#', '$', '§', 'Ä', 'Ö', 'Ü', '^', '_', 'º', 'ä', 'ö', 'ü', 'ß'}, // 4 - German
{0, '£', '$', 'é', 'º', 'ç', '-', '|', '#', 'ù', 'à', 'ò', 'è', 'ì'}, // 5 - Italian
{0, '#', '$', 'S', 'e', 'e', 'Z', 'c', 'u', 's', 'a', 'u', 'z', 'i'}, // 6 - Lettish/Lithuanian
{0, '#', 'n', 'a', 'Z', 'S', 'L', 'c', 'ó', 'e', 'z', 's', 'l', 'z'}, // 7 - Polish
{0, 'ç', '$', 'i', 'á', 'é', 'í', 'ó', 'ú', '¿', 'ü', 'ñ', 'è', 'à'}, // 8 - Portuguese/Spanish
{0, '#', '¤', 'T', 'Â', 'S', 'A', 'Î', 'i', 't', 'â', 's', 'a', 'î'}, // 9 - Rumanian
{0, '#', 'Ë', 'C', 'C', 'Z', 'D', 'S', 'ë', 'c', 'c', 'z', 'd', 's'}, // 10 - Serbian/Croation/Slovenian
{0, '#', '¤', 'É', 'Ä', 'Ö', 'Å', 'Ü', '_', 'é', 'ä', 'ö', 'å', 'ü'}, // 11 - Swedish/Finnish/Hungarian
{0, 'T', 'g', 'I', 'S', 'Ö', 'Ç', 'Ü', 'G', 'i', 's', 'ö', 'ç', 'ü'}  // 12 - Turkish
};

uint16_t laG0_nat_opts16[13][14] = {
{0, '#',    'u',    'c',    't',    'z',    0xc3bd, 0xc3ad, 'r',    0xc3a9, 0xc3a1, 'e',    0xc3ba, 's'   }, // 0 - Czech/Slovak
{0, 0xc2a3, '$',    '@',    '-',    0xc2bd, '-',    '|',    '#',    '-',    0xc2bc, '#',    0xc2be, 0xc3b7}, // 1 - English
{0, '#',    0xc3b5, 'S',    0xc384, 0xc396, 'Z',    0xc39c, 0xc395, 's',    0xc3a4, 0xc3b6, 'z',    0xc3bc}, // 2 - Estonian
{0, 0xc3a9, 0xc3af, 0xc3a0, 0xc3ab, 0xc3aa, 0xc3b9, 0xc3ae, '#',    0xc3a8, 0xc3a2, 0xc3b4, 0xc3bb, 0xc3a7}, // 3 - French
{0, '#',    '$',    0xc2a7, 0xc384, 0xc396, 0xc39c, '^',    '_',    0xc2ba, 0xc3a4, 0xc3b6, 0xc3bc, 0xc39f}, // 4 - German
{0, 0xc2a3, '$',    0xc3a9, 0xc2ba, 0xc3a7, '-',    '|',    '#',    0xc3b9, 0xc3a0, 0xc3b2, 0xc3a8, 0xc3ac}, // 5 - Italian
{0, '#',    '$',    'S',    'e',    'e',    'Z',    'c',    'u',    's',    'a',    'u',    'z',    'i'   }, // 6 - Lettish/Lithuanian
{0, '#',    'n',    'a',    'Z',    'S',    'L',    'c',    0xc3b3, 'e',    'z',    's',    'l',    'z'   }, // 7 - Polish
{0, 0xc3a7, '$',    'i',    0xc3a1, 0xc3a9, 0xc3ad, 0xc3b3, 0xc3ba, 0xc2bf, 0xc3bc, 0xc3b1, 0xc3a8, 0xc3a0}, // 8 - Portuguese/Spanish
{0, '#',    0xc2a4, 'T',    0xc382, 'S',    'A',    0xc38e, 'i',    't',    0xc3a2, 's',    'a',    0xc3ae}, // 9 - Rumanian
{0, '#',    0xc38b, 'C',    'C',    'Z',    'D',    'S',    0xc3ab, 'c',    'c',    'z',    'd',    's'   }, // 10 - Serbian/Croation/Slovenian
{0, '#',    0xc2a4, 0xc389, 0xc384, 0xc396, 0xc385, 0xc39c, '_',    0xc3a9, 0xc3a4, 0xc3b6, 0xc3a5, 0xc3bc}, // 11 - Swedish/Finnish/Hungarian
{0, 'T',    'g',    'I',    'S',    0xc396, 0xc387, 0xc39c, 'G',    'i',    's',    0xc3b6, 0xc3a7, 0xc3bc}  // 12 - Turkish
};
