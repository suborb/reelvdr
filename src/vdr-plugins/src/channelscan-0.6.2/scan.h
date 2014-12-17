/***************************************************************************
 *   Copyright (C) 2005 by Reel Multimedia;  Author:  Markus Hahn          *
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
 *
 ***************************************************************************
 *
 *  scan.h provides scanning through given tansponder lists
 *
 ***************************************************************************/

#ifndef _SCAN_H
#define _SCAN_H


#include <vdr/sources.h>
#include <vdr/device.h>
#include <vdr/channels.h>
#include <vdr/eit.h>
#include "filter.h"

#include "transponders.h"
#include "channelscan.h"

class NitFilter;
class SdtFilter;
class PatFilter;

enum eScanState
{ ssInit,
    ssGetTransponders,
    ssGetChannels,
    ssInterrupted,
    ssDeviceFailure,
    ssNoTransponder,
    ssNoLock,
    ssFilterFailure,
    ssSuccess
};

// -- cScan --------------------------------------------------------

class cScan:public cThread
{
  private:

    cScan(const cScan &);
      cScan & operator=(const cScan &);

    int ScanServices();
    void ScanNitServices();

    void waitSignal(void);
    int waitLock(int timeout);
    void ScanNitDVB_S(cTransponder * tp, cChannel * c);
    void ScanDVB_S(cTransponder * tp, cChannel * c);
    void ScanDVB_T(cTransponder * tp, cChannel * c);
    void ScanDVB_C(cTransponder * tp, cChannel * c);

    void AddTransponders();
    void ClearMap();

    cScanParameters scanParameter_;

#ifndef DEVICE_ATTRIBUTES
    int fd_frontend;
#endif
    cDevice *device;
    cChannel *channel;
    int origUpdateChannels;
    bool nitScan;
    int sourceCode;             // internal VDR Code for dvb Sources  see vdr/sources.c
    int cardnr;
    int transponderNr;
    int channelNumber;
    int frequency;

    int newChannels;
    int detailedSearch;
    int srModes;
    int srstat;
    int maxWaits_;
    int lastMod;

    int symbolrate;             // current state
    int modulation;             // current state
    int fec;                    // current state

    int lastLocked;
    int foundNum, totalNum;

    NitFilter *nitFilter_;
    SdtFilter *SFilter;
    PatFilter *PFilter;
    cEitFilter *EFilter;

    uint16_t getSignal(void);
    uint16_t getSNR(void);
    uint16_t getStatus(void);

  protected:
      virtual void Action(void);
  public:
      cScan();
     ~cScan();
    void ShutDown();
    bool StartScanning(cScanParameters * scp);
    bool Signal(int frequency, char pol, int symbolrate);
    int GetCurrentTransponderNr() const
    {
        return transponderNr;
    }
    ///< GetCurrentTransponderNr returns the sequential transponder number
    int GetCurrentChannelNumber() const
    {
        return channelNumber;
    }
    int GetCurrentMaxWaits() const
    {
        return maxWaits_;
    }
    ///< GetCurrentTransponderNr returns the channel number according to frequencies
    int GetCurrentFrequency() const
    {
        return frequency;
    }
    int GetCurrentSR() const
    {
        return symbolrate;
    }

    int GetCurrentModulation() const
    {
        return modulation;
    }

    int GetCurrentFEC() const
    {
        return fec;
    }

    void GetFoundNum(int &current, int &total) const
    {
        current = foundNum;
        total = abs(totalNum < 200) ? totalNum : 0;
    };
    void DumpHdTransponder();

};

#endif //_SCAN__H
