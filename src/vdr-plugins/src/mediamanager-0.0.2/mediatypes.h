/*
 * mediatypes.h:
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#ifndef _MEDIA_TYPES_H
#define _MEDIA_TYPES_H

enum eMediaType {
	mtInvalid	= -1,
	mtNoDisc	= 0,
	mtBlank 	= 1,
	mtAudio 	= 2,
	mtVideoDvd	= 3,
	mtSvcd		= 4,
	mtVcd		= 5,
	mtData		= 6,
	mtVDRdata	= 7,
	mtAudioData	= 8,
	mtImageData = 9,
	mtExternUse = 10
};

#endif
