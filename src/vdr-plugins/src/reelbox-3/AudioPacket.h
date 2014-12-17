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

// AudioPacket.h

#ifndef AUDIO_PACKET_H_INCLUDED
#define AUDIO_PACKET_H_INCLUDED

#include "Reel.h"

namespace Reel
{
    class SampleRate
    {
    public:
        enum
        {
            TicksPerSecond = 70560000,  // The lowest common multiple of all supported samplerates (32000, 44100, 48000)
                                        // and 90000. All audio timing calculations are done in 70.56-MHz-ticks.

            TicksPerMpegTick = TicksPerSecond / 90000
        };

        SampleRate() NO_THROW;
            ///< Construct the "unsupported" samplerate.

        // Default copying and copy assigning is ok.

        UInt GetHz() const NO_THROW;
            ///< Return the samplerate in Hz if supported (0 for the unsupported samplerate).

        UInt GetTicksPerFrame() const NO_THROW;
            ///< Return the duration of a audio frame with this samplerate in ticks.

        bool IsSupported() const NO_THROW;
            ///< Returns true if samplerate is supported.

        static SampleRate HzToSampleRate(UInt hz);
            ///< Convert a frequency in Hz to a SampleRate.

        static SampleRate SampleRate32K();
        static SampleRate SampleRate44_1K();
        static SampleRate SampleRate48K();
        static SampleRate SampleRateUnsupported();

    private:
        SampleRate(UInt ticksPerFrame);

        UInt ticksPerFrame_;
    };

    //--------------------------------------------------------------------------------------------------------------

    union AudioFrame
    {
        UInt val;
        struct
        {
            Short Right;
            Short Left;
        } Samples;
    };

    //--------------------------------------------------------------------------------------------------------------

    class AudioPacket
    {
    public:
        enum
        {
            MaxFrames = 0x800 // FIXME
        };

        AudioPacket();
        ~AudioPacket();

        AudioFrame *GetFrames();
            ///< Return ptr to frame data.

        AudioFrame const *GetFrames() const;
            ///< Return ptr to frame data (read-only).

        UInt GetGeneration() const;
            ///< Return the generation of the packet.

        UInt GetNumFrames() const;
            ///< Return the number of frames contained in the packet.

        UInt GetPts() const;
            ///< Return the presentation timestamp.

        SampleRate GetSampleRate() const;
            ///< Return the samplerate.

        bool HasPts() const;
            ///< Return true iff the packet carries a presentation timestamp.

        void SetGeneration(UInt generation);
            ///< Set the generation of the packet.

        void SetNumFrames(UInt numFrames);
            ///< Set the numer of frames contained in the packet.

        void SetPts(UInt pts);
            ///< Set the presentation timestamp.

        void SetPtsValid(bool ptsValid);
            ///< Set the presentation timestamp valid flag.

        void SetSampleRate(SampleRate sampleRate);
            ///< Set the samplerate.

    private:
        AudioPacket(AudioPacket const &); // Forbid copy construction.
        AudioPacket &operator=(AudioPacket const &); // Forbid copy assignment.

        SampleRate   sampleRate_;
        UInt         numFrames_;
        UInt         pts_;
        UInt         generation_;
        bool         hasPts_;
        bool         isPcm_;
        AudioFrame   frames_[MaxFrames];
    };

    //--------------------------------------------------------------------------------------------------------------

    inline SampleRate::SampleRate() NO_THROW
    :   ticksPerFrame_(0)
    {
    }

    //--------------------------------------------------------------------------------------------------------------

    inline UInt SampleRate::GetHz() const NO_THROW
    {
        return ticksPerFrame_ ? TicksPerSecond / ticksPerFrame_ : 0; 
    }

    //--------------------------------------------------------------------------------------------------------------

    inline UInt SampleRate::GetTicksPerFrame() const NO_THROW
    {
        return ticksPerFrame_;
    }

    //--------------------------------------------------------------------------------------------------------------

    inline bool SampleRate::IsSupported() const NO_THROW
    {
        return ticksPerFrame_ != 0;
    }

    //--------------------------------------------------------------------------------------------------------------

    inline SampleRate SampleRate::HzToSampleRate(UInt hz)
    {
        if (hz == 48000 || hz == 44100 || hz == 32000)
        {
            return TicksPerSecond / hz;
        }
        return 0;
    }

    //--------------------------------------------------------------------------------------------------------------

    inline SampleRate SampleRate::SampleRate32K()
    {
        return TicksPerSecond / 32000;
    }

    //--------------------------------------------------------------------------------------------------------------

    inline SampleRate SampleRate::SampleRate44_1K()
    {
        return TicksPerSecond / 44100;
    }

    //--------------------------------------------------------------------------------------------------------------

    inline SampleRate SampleRate::SampleRate48K()
    {
        return TicksPerSecond / 48000;
    }

    //--------------------------------------------------------------------------------------------------------------

    inline SampleRate SampleRate::SampleRateUnsupported()
    {
        return 0;
    }

    //--------------------------------------------------------------------------------------------------------------

    inline SampleRate::SampleRate(UInt ticksPerFrame)
    :   ticksPerFrame_(ticksPerFrame)
    {
    }

    //--------------------------------------------------------------------------------------------------------------

    inline bool operator==(SampleRate sr1, SampleRate sr2)
    {
        return sr1.GetTicksPerFrame() == sr2.GetTicksPerFrame();
    }

    //--------------------------------------------------------------------------------------------------------------

    inline bool operator!=(SampleRate sr1, SampleRate sr2)
    {
        return sr1.GetTicksPerFrame() != sr2.GetTicksPerFrame();
    }

    //--------------------------------------------------------------------------------------------------------------

    inline AudioPacket::AudioPacket()
    {
    }

    //--------------------------------------------------------------------------------------------------------------

    inline AudioPacket::~AudioPacket()
    {
    }

    //--------------------------------------------------------------------------------------------------------------

    inline AudioFrame *AudioPacket::GetFrames()
    {
        return frames_;
    }

    //--------------------------------------------------------------------------------------------------------------

    inline AudioFrame const *AudioPacket::GetFrames() const
    {
        return frames_;
    }

    //--------------------------------------------------------------------------------------------------------------

    inline UInt AudioPacket::GetGeneration() const
    {
        return generation_;
    }

    //--------------------------------------------------------------------------------------------------------------

    inline UInt AudioPacket::GetNumFrames() const
    {
        return numFrames_;
    }

    //--------------------------------------------------------------------------------------------------------------

    inline UInt AudioPacket::GetPts() const
    {
        return pts_;
    }

    //--------------------------------------------------------------------------------------------------------------

    inline SampleRate AudioPacket::GetSampleRate() const
    {
        return sampleRate_;
    }

    //--------------------------------------------------------------------------------------------------------------

    inline bool AudioPacket::HasPts() const
    {
        return hasPts_;
    }

    //--------------------------------------------------------------------------------------------------------------

    inline void AudioPacket::SetGeneration(UInt generation)
    {
        generation_ = generation;
    }

    //--------------------------------------------------------------------------------------------------------------

    inline void AudioPacket::SetNumFrames(UInt numFrames)
    {
        numFrames_ = numFrames;
    }

    //--------------------------------------------------------------------------------------------------------------

    inline void AudioPacket::SetPts(UInt pts)
    {
        pts_ = pts;
    }

    //--------------------------------------------------------------------------------------------------------------

    inline void AudioPacket::SetPtsValid(bool ptsValid)
    {
        hasPts_ = ptsValid;
    }

    //--------------------------------------------------------------------------------------------------------------

    inline void AudioPacket::SetSampleRate(SampleRate sampleRate)
    {
        sampleRate_ = sampleRate;
    }

    //--------------------------------------------------------------------------------------------------------------

    ///< Convenience function. Return play time of audio packet in ticks.
    inline UInt AudioPacketPlayTime(AudioPacket const &audioPacket)
    {
        return audioPacket.GetSampleRate().GetTicksPerFrame() * audioPacket.GetNumFrames();
    }
}

#endif // def AUDIO_PACKET_H_INCLUDED
