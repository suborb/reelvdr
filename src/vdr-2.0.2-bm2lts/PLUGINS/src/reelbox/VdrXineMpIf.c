/***************************************************************************
 *   Copyright (C) 2005 by Reel Multimedia                                 *
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

// VdrXineMpIf.c

#include "VdrXineMpIf.h"

#include "Reel.h"
#include "ReelBoxDevice.h"
#include "ReelBoxMenu.h"
#include "BspOsdProvider.h"
#include "BspCommChannel.h"
#include "HdOsdProvider.h"

#include <vdr/config.h>

#include <stdio.h>
#include <map>

namespace Reel
{
    class BufferPtsMap /* final */
    {
    public:
        // Default constructors, copying and copy-assignment ok.

        // All public members are fully thread-safe.

        void Clear();
            // clear mapping.

        UInt FetchPts(void const *mem);
            // return pts for mem or 0 if not found. Invalidates the entry.

        void Put(void const *mem, UInt pts);
            // insert mem-pts pair.

    private:
        typedef std::map<void const *, UInt> Map_;
        typedef Map_::iterator It_;
        Map_ map_; // maps mem-pointers to pts.
        Mutex mutex_;
    };

    inline void BufferPtsMap::Clear()
    {
        MutexLocker lock(mutex_);

        map_.clear();
    }

    inline UInt BufferPtsMap::FetchPts(void const *mem)
    {
        MutexLocker lock(mutex_);

        It_ it = map_.find(mem);
        if (it == map_.end())
        {
            // not found.
            return 0;
        }
        UInt ret = it->second;
        it->second = 0; // invalidate.
        return ret;
    }

    inline void BufferPtsMap::Put(void const *mem, unsigned int pts)
    {
        MutexLocker lock(mutex_);

        map_[mem] = pts;
    }

    static BufferPtsMap bufferPtsMap_;
    static SampleRate sampleRate_ = SampleRate::SampleRate44_1K();

    static void AoClose()
    {
        ::printf("AoClose()\n");
    }
    
    static int AoControl(::VdrXineMpAoCtrl ctrlCmd)
    {
        // TODO: implement
        ::printf("AoControl(%d)\n", ctrlCmd);
        if (ctrlCmd == VdrXineMpAoCtrlFlushBuffers)
        {
            if (ReelBoxDevice::Instance())
                ReelBoxDevice::Instance()->FlushAudio();
        }
        return 0;
    }
    
    static int AoDelay()
    {
        if (ReelBoxDevice::Instance())
           return ReelBoxDevice::Instance()->AudioDelay() / sampleRate_.GetTicksPerFrame();
        return 0;
    }
    
    static int AoOpen(unsigned int       bits,
                      unsigned int       rate,
                      ::VdrXineMpAoCapMode mode)
    {
        bufferPtsMap_.Clear();

        ::printf("AoOpen() mode = %d, bits = %d, rate = %d\n", mode, bits, rate);

        if (bits != 16)
        {
            return 0;
        }

        if (mode != VdrXineMpAoCapModeStereo && mode != VdrXineMpAoCapModeA52) // TODO: support Mono
        {
            return 0;
        }

        sampleRate_ = SampleRate::HzToSampleRate(rate);
        int hz = sampleRate_.GetHz();
        ReelBoxDevice::Instance()->SetDigitalAudioDevice(mode == VdrXineMpAoCapModeA52);
        if (hz)
        {
            ::printf("hz = %d\n", hz);
            // Ok, no resampling necessary.
            return hz;
        }
        else
        {
            // use 48 kHz, xine will resample.
            sampleRate_ = SampleRate::SampleRate48K();
            return sampleRate_.GetHz();
        }
    }

    static int AoUseDigitalOut()
    {
        return ::Setup.UseDolbyDigital;
    }
    
    static int AoWrite(unsigned short *data,
                       unsigned int    numFrames)
    {
        UInt pts = bufferPtsMap_.FetchPts(data);
        ReelBoxDevice *device = ReelBoxDevice::Instance();
        if (!device) return 0;
         
        ::cPoller poller;
        device->Poll(poller, 100);
        device->PollAudio(100); 

        device->PlayAudioRaw(reinterpret_cast<AudioFrame *>(data),
                                                numFrames,
                                                sampleRate_,
                                                pts);
        return 1;
    }
    
    static void AoPutMemPts(void const   *mem,
                            unsigned int  pts)
    {
        bufferPtsMap_.Put(mem, pts);
    }

    static void VdDecodeData(unsigned char const *data,
                             unsigned int         length,
                             unsigned int         pts)
    {
        // TODO: Refine implementation: Do not block in this call, because this may cause the 
        //       following situation:
        //
        //          The xine input-engine stalls. Then all audio-packets will be too late, and
        //          xine drops them. We will not see any more audio packets and therefore therefore
        //          AV-Sync won't be there to help us dropping video frames.
        //
        // Solution: Buffer some video data.
        ReelBoxDevice *device = ReelBoxDevice::Instance();
        static int packetCount = 0;
        if (!packetCount)
        {
            ::cPoller poller;
            device->Poll(poller, 1000);
            //Start by Klaus
            device->PollVideo(1000);
            //End by Klaus
            packetCount = 10; // Poll only for every 10th packet.
        }
        -- packetCount;
        device->PlayVideoEs(data, length, pts);
    }

    static void VdDiscontinuity()
    {
        ::printf("VdDiscontinuity()\n");
    }

    static void VdFlush()
    {
        ::printf("VdFlush()\n");
        //ReelBoxDevice *device = ReelBoxDevice::Instance();
        //device->Flush(5000);
    }

    static void VdReset()
    {
        //ReelBoxDevice *device = ReelBoxDevice::Instance();
        //device->Clear();
        ::printf("VdReset()\n");
    }

    //Start by Klaus
    static void SetOsdScaleMode(int on)
    {
        if(!RBSetup.usehdext)
        BspOsdProvider::SetOsdScaleMode(on);
    }

    static void SetPlayMode(int on)
    {
        printf("DEBUG [reelbox] VdrXineMpIf::SetPlayMode \n");
        ReelBoxDevice *device = ReelBoxDevice::Instance();
        if(on && device)
        {
            device->SetPlayMode(pmAudioVideo);
        }
        else
        {
            device->SetPlayMode(pmNone);
        }
    }
    //End by Klaus
    
    static VdrXineMpIf /*const*/ vdrXineMpIf_ =
    {
        AoClose,
        AoControl,
        AoDelay,
        AoOpen,
        AoUseDigitalOut,
        AoWrite,
        AoPutMemPts,
        VdDecodeData,
        VdDiscontinuity,
        VdFlush,
        VdReset,
        //Start by Klaus
        SetOsdScaleMode,
        SetPlayMode
        //End by Klaus 
    };

    ::VdrXineMpIf /*const*/ *GetVdrXineMpIf() NO_THROW
    {
        return &vdrXineMpIf_;
    }
}
