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

#include "spu_If.h"
#include "spu.h"

#include <metronom.h>
#include <video_overlay.h>

extern VdrXineMpIf *VXI;

void SpuPutEvents(video_overlay_event_t *event, struct spudec_decoder_s *instance)
{
    //printf("-----------------spu_If:spuEvents.PutEvent------------------------\n");
    size_t paletteSize = 0; //???
    size_t rleSize = event->object.overlay->num_rle;
    reel_video_overlay_event_t *newEvent = VXI->SpuGetEventStruct(paletteSize, rleSize);
    newEvent->decoder_instance = instance;
    //printf("-----------------spu_If:spuEvents.PutEvent, vor SpuCopyEvent-----------------\n");
    SpuCopyEvent(newEvent, event);
    //printf("-----------------spu_If:spuEvents.PutEvent, vor SpuPutEvents-----------------\n");
    VXI->SpuPutEvents(newEvent);
}

void SpuCopyEvent(reel_video_overlay_event_t *newEvent, video_overlay_event_t *event)
{
    //printf("-----------------spu_If:SpuCopyEvent-----------------\n");
    newEvent->event_type = event->event_type;
    newEvent->vpts = event->vpts;
    SpuCopyOverlayObject(&(newEvent->object), &(event->object));
    //printf("-----------------spu_If:SpuCopyEvent, vor return-------------\n");
}

void SpuCopyOverlayObject(reel_video_overlay_object_t *newObject, video_overlay_object_t *object)
{
     //printf("-----------------spu_If:SpuCopyOverlayObject-----------------\n");
    uint i;
    newObject->handle = object->handle;
    newObject->object_type = object->object_type;
    newObject->pts = object->pts;  
    SpuCopyOverlay(newObject->overlay, object->overlay);
    newObject->palette_type = object->palette_type; 
    newObject->palette_exists = newObject->palette != NULL; //additonal member
    if(newObject->palette != NULL)
    {
        for(i = 0 ; i < 0/*OVL_PALETTE_SIZE*/; i++) //???
        {
            (newObject->palette)[i] = (object->palette)[i];
        }
    }
    newObject->buttonN = object->buttonN;
    for(i = 0; i < 32; i++)
    {
        SpuCopyButtons(&(newObject->button)[i], &(object->button)[i]);
    } 
} 

void SpuCopyOverlay(reel_vo_overlay_t *newOverlay, vo_overlay_t *overlay)
{
    //printf("-------spu_If:SpuCopyOverlay, data_size = %u, num_rle = %u-----\n", overlay->data_size, overlay->num_rle);
    //printf("-------spu_If:SpuCopyOverlay, newOverlay->rle = %u, overlay->rle = %u-----\n", (uint) newOverlay->rle, (uint) overlay->rle);
    uint i;
    //Copy rle data
    newOverlay->data_valid = 0;
    if (overlay->num_rle > 0 && overlay->rle)
    {
        memcpy(newOverlay->rle, overlay->rle, overlay->num_rle * sizeof(reel_rle_elem_t));
        newOverlay->data_valid = 1;
    } 
     //printf("---------spu_If:SpuCopyOverlay, nach memcpy--------\n");

    newOverlay->data_size = overlay->data_size; 
    newOverlay->num_rle = overlay->num_rle; 
    newOverlay->x = overlay->x ; 
    newOverlay->y =  overlay->y;
    newOverlay->width = overlay->width;
    newOverlay->height = overlay->height;
    newOverlay->rgb_clut = overlay->rgb_clut;

    newOverlay->hili_top = overlay->hili_top;
    newOverlay->hili_bottom =  overlay->hili_bottom;
    newOverlay->hili_left = overlay->hili_left;
    newOverlay->hili_right = overlay->hili_right;  
    newOverlay->hili_rgb_clut = overlay->hili_rgb_clut;

    for(i = 0; i < OVL_PALETTE_SIZE; i++) 
    {
        (newOverlay->hili_color)[i] = (overlay->hili_color)[i];
        (newOverlay->hili_trans)[i] = (overlay->hili_trans)[i];
        (newOverlay->color)[i] = (overlay->color)[i];
        (newOverlay->trans)[i] = (overlay->trans)[i];
    }
    newOverlay->unscaled = overlay->unscaled;
}

void SpuCopyRle(reel_rle_elem_t *newRle,  rle_elem_t *rle)
{
    //printf("-----------------spu_If:SpuCopyRle-----------------\n");
    newRle->len = rle->len;
    newRle->color = rle->color;
}

void SpuCopyButtons(reel_vo_buttons_t *newButton, vo_buttons_t *button)
{ 
    //printf("-----------------spu_If:SpuCopyButtons-----------------\n");
    uint i;
    newButton->type = button->type;
    newButton->hili_top = button->hili_top;
    newButton->hili_bottom = button->hili_bottom;
    newButton->hili_left = button->hili_left;
    newButton->hili_right = button->hili_right;
    newButton->up = button->up; 
    newButton->down = button->down; 
    newButton->left = button->left; 
    newButton->right = button->right;

    //do we need this?
    //reel_xine_event_t      select_event;
    //reel_xine_event_t      active_event;

    newButton->hili_rgb_clut = button->hili_rgb_clut; 
    for(i = 0; i < OVL_PALETTE_SIZE; i++)
    {
        (newButton->select_color)[i] = (button->select_color)[i];
        (newButton->select_trans)[i] = (button->select_trans)[i];
        (newButton->active_color)[i] = (button->active_color)[i];
        (newButton->active_trans)[i] = (button->active_trans)[i];
    }
}

int64_t get_metronom_time(struct spudec_decoder_s *instance)
{
    //printf("--------------spu_If:GetMetronomTime--------------------------\n");
    if(instance)
    {
        return instance->stream->xine->clock->get_current_time(instance->stream->xine->clock);
        //return  instance->stream->metronom->get_current_time(instance->stream->metronom);
    }
    else
    {
        return -1;
    }
} 

void InitIfFunctions()
{ 
    VXI->GetMetronomTime = get_metronom_time;
}  

