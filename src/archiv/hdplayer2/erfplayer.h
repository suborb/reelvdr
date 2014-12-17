/***************************************************************************
 *   Copyright (C) 2007 by Dirk Leber                                      *
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

#ifndef ERFPLAYER_H_INCLUDED
#define ERFPLAYER_H_INCLUDED

#include <hdshm_user_structs.h>

void erf_init(hd_packet_erf_init_t const *packet);
void erf_play(hd_packet_erf_play_t const *packet);
void erf_cmd(hd_packet_erf_cmd_t const *packet);
void erf_done(hd_packet_erf_done_t const *packet);

#endif // ERFPLAYER_H_INCLUDED
