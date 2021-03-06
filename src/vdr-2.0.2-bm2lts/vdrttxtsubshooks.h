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

#ifndef __VDRTTXTSUBSHOOKS_H
#define __VDRTTXTSUBSHOOKS_H

#define TTXTSUBSVERSNUM 2

class cDevice;
class cChannel;
struct tTeletextSubtitlePage;

class cVDRTtxtsubsHookListener {
 public:
  cVDRTtxtsubsHookListener(void) {};
  virtual ~cVDRTtxtsubsHookListener();

  void HookAttach(void);

  virtual void HideOSD(void) {};
  virtual void ShowOSD(void) {};
  virtual void PlayerTeletextData(uint8_t *p, int length, bool IsPesRecording = true, const struct tTeletextSubtitlePage teletextSubtitlePages[] = NULL, int pageCount = 0) {};
  virtual int ManualPageNumber(const cChannel *channel) { return 0; };

  // used by VDR to call hook listeners
  static cVDRTtxtsubsHookListener *Hook(void);
};

#endif
