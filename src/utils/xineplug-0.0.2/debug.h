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
//#include <xine-engine/bswap.h>
#include <bswap.h>
#include <xineutils.h>
#include <video_overlay.h>


//------------test---------------------------------------------

static void my_video_overlay_print_button(vo_buttons_t *buttons)
{
   printf("--button--hili_top = %d-----\n", buttons->hili_top); 
   printf("--button--hili_bottom = %d-----\n", buttons->hili_bottom) ;
   printf("--button--hili_left = %d-----\n", buttons->hili_left) ;
   printf("--button--hili_right = %d-----\n", buttons->hili_right); 
   printf("--button--up = %d-down = %d-left = %d-right = %d\n",
          buttons->up, buttons->down, buttons->left, buttons->right); 
   printf("--button--hili_rgb_clut = %d\n", buttons->hili_rgb_clut);
 
  //xine_event_t      select_event;
  //xine_event_t      active_event;
}

static void my_video_overlay_print_buttons(vo_buttons_t *buttons)
{
  uint i = 0;
  for(i = 0; i < 32; i++)
  {
      my_video_overlay_print_button(&buttons[i]);
  } 
}

static void my_video_overlay_print_overlay( vo_overlay_t *ovl ) {
  printf ("video_overlay: OVERLAY to show\n");
  printf ("video_overlay: \tx = %d y = %d width = %d height = %d\n",
	  ovl->x, ovl->y, ovl->width, ovl->height );
  printf ("video_overlay: \tclut [%x %x %x %x]\n",
	  ovl->color[0], ovl->color[1], ovl->color[2], ovl->color[3]);
  printf ("video_overlay: \ttrans [%d %d %d %d]\n",
	  ovl->trans[0], ovl->trans[1], ovl->trans[2], ovl->trans[3]);
  printf ("video_overlay: \tclip top=%d bottom=%d left=%d right=%d\n",
	  ovl->hili_top, ovl->hili_bottom, ovl->hili_left, ovl->hili_right);
  printf ("video_overlay: \tclip_clut [%x %x %x %x]\n",
	  ovl->hili_color[0], ovl->hili_color[1], ovl->hili_color[2], ovl->hili_color[3]);
  printf ("video_overlay: \thili_trans [%d %d %d %d]\n",
	  ovl->hili_trans[0], ovl->hili_trans[1], ovl->hili_trans[2], ovl->hili_trans[3]);
  return;
} 

static void video_overlay_print_event(video_overlay_event_t *event)
{
  printf("verfickte verdammte Scheisse!!!!!\n");
  printf("--event_type = %d\n", event->event_type);
  printf("--vpts = %lld\n", event->vpts);
  printf("--handle = %d\n--object_type = %d\n--pts = %lld\n--buttonN = %d\n", 
        event->object.handle, event->object.object_type, event->object.pts, event->object.buttonN);
  my_video_overlay_print_overlay(event->object.overlay);
  //my_video_overlay_print_buttons(event->object.button);
}
