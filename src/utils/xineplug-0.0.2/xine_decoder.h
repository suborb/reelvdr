/*
 * Copyright (C) 2000-2004 the xine project
 * 
 * Copyright (C) James Courtier-Dutton James@superbug.demon.co.uk - July 2001
 *
 * This file is part of xine, a unix video player.
 * 
 * xine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * xine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 * $Id: xine_decoder.c,v 1.116 2006/07/10 22:08:30 dgp85 Exp $
 *
 * stuff needed to turn libspu into a xine decoder plugin
 */

#include <xine_internal.h>
#include <buffer.h>
//#endif

/*
#define LOG_DEBUG 1
#define LOG_BUTTON 1
*/

void *init_reel_spu_decode_plugin (xine_t *xine, void *data); 

