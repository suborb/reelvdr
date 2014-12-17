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

// VideoPlayerHd.c

#include "VideoPlayerHd.h"
#include <vdr/remux.h>
#include "ReelBoxDevice.h"
#include "fs453settings.h"

namespace Reel
{
	void hexdump ( uint8_t *x, int len )
	{
		int n;
		for ( n=0;n<len;n+=16 )
		{
			printf ( "%p: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
			         x,x[0],x[1],x[2],x[3],x[4],x[5],x[6],x[7],
			         x[8],x[9],x[10],x[11],x[12],x[13],x[14],x[15] );
			x+=16;
		}
	}

	//--------------------------------------------------------------------------------------------------------------

	void VideoPlayerHd::Create()
	{
		if ( !instance_ )
		{
			instance_ = new VideoPlayerHd;
		}
	}

	//--------------------------------------------------------------------------------------------------------------
	void VideoPlayerHd::Clear()
	{
		//printf ( "VideoPlayerHd::Clear()\n" );
		hdPlayer_.decoder_frames=0;
		++ generation_;
	}

	//--------------------------------------------------------------------------------------------------------------

	VideoPlayerHd::VideoPlayerHd() NO_THROW
:   hdPlayer_ ( HdCommChannel::hda->player[0] ),
	generation_ ( 0 ),
	freeze_ ( false ),
	pts_shift_ ( 0 ),
	ac3_pts_shift_ ( 0 )
	{
		hdPlayer_.pts_shift = pts_shift_;
		hdPlayer_.ac3_pts_shift = ac3_pts_shift_;
	}

	//--------------------------------------------------------------------------------------------------------------

	bool VideoPlayerHd::Flush()
	{
		hd_packet_clear_t packet;
		//printf ( "VideoPlayerHd::Flush()\n" );
		static int waitAproxFramesInQueue = 0;
		static int lastAproxFramesInQueue = 0;
		int aproxFramesInQueue = AproxFramesInQueue();
		if(aproxFramesInQueue == lastAproxFramesInQueue) {
			waitAproxFramesInQueue--; // Don't wait for ever if AproxFramesInQueue is not updated
		} else {
			lastAproxFramesInQueue = aproxFramesInQueue;
			waitAproxFramesInQueue = lastAproxFramesInQueue * (1000/25) /*assume 25fps*/ / 5 /*5ms sleep in ReelBoxDevice::Flush*/;
		} // if
		if(waitAproxFramesInQueue > 0)
			return false;
		hd_channel_invalidate ( HdCommChannel::chStream1.ch_, 1 );
		HdCommChannel::chStream1.SendPacket ( HD_PACKET_CLEAR, packet, 0, 0 );
		return true;
	}

	//--------------------------------------------------------------------------------------------------------------

	void VideoPlayerHd::Freeze()
	{
		//printf ( "VideoPlayerHd::Freeze()\n" );
		freeze_ = true;
		hdPlayer_.pause=1;
	}

	//--------------------------------------------------------------------------------------------------------------

	void VideoPlayerHd::Play()
	{
		//printf("VideoPlayerHd::Play()\n");

		Trickmode ( 0 );
		hdPlayer_.pause=0;
	}

	//--------------------------------------------------------------------------------------------------------------

	static unsigned int oldvpid = 0;
	static unsigned int oldapidpatpmt[MAXAPIDS] = {0};
	static unsigned int oldAudioIndex = 0;
	// static int off;
	static Byte sendbuf[64*TS_SIZE];
	static int buffill = 0;

	void VideoPlayerHd::PlayTsPacket ( void *data, int length, unsigned char* PATPMT )
	{
		hd_packet_ts_data_t packet;

		if ( pts_shift_ != 10 * RBSetup.delay_stereo )
		{
			hdPlayer_.pts_shift = pts_shift_ = 10 * RBSetup.delay_stereo;
		}
		if ( ac3_pts_shift_ != 10 * RBSetup.delay_ac3 )
		{
			hdPlayer_.ac3_pts_shift = ac3_pts_shift_ = 10 * RBSetup.delay_ac3;
		}

		hdPlayer_.pause=0;
		hdPlayer_.data_generation = generation_; // Must be done _before_ sending the packet!

		unsigned int audioIndex = ReelBoxDevice::Instance()->GetAudioTrack();

		packet.generation = generation_;

		int bytes_sent = 0;
		Byte const* d = ( Byte const* ) data;

		packet.vpid = 0; packet.apid = 0;

		if ( PATPMT )
		{
			// ++generation_; // no longer required
			//int ix;
			//printf("%x %x %d %d\n", PATPMT[TS_SIZE + 19], PATPMT[TS_SIZE + 18], PATPMT[TS_SIZE + 19], PATPMT[TS_SIZE + 18]);
			int vpidpatpmt = PATPMT[TS_SIZE + 19]&0xff | ( ( PATPMT[TS_SIZE+18]&0x1f )  << 8 );
			int apidpatpmt[MAXAPIDS] = { 0 };
			unsigned int apidsfound = 0;
			int offset = 17+5; /* after header and vpid */
			/* as long as there are audio tracks */
			while ( PATPMT[TS_SIZE + offset] == 0x06 || PATPMT[TS_SIZE + offset] == 0x04 )
			{
				/* test for mpa */
				if ( PATPMT[TS_SIZE + offset]==0x04 && apidsfound < MAXAPIDS )
				{
					apidpatpmt[apidsfound] = PATPMT[TS_SIZE + offset + 2]&0xff | ( ( PATPMT[TS_SIZE + offset + 1]&0x1f ) << 8 );
					offset += 5;
					apidsfound++;
				}
				/* test for dolby */
				if ( PATPMT[TS_SIZE + offset]==0x06 && apidsfound < MAXAPIDS )
				{
					/* set first mpa also, will be overriden if a real mpa-track is found */
					apidpatpmt[apidsfound] = PATPMT[TS_SIZE + offset + 2]&0xff | ( ( PATPMT[TS_SIZE + offset + 1]&0x1f ) << 8 );
					apidsfound++;
					offset += 8;
				}
			}
			if ( vpidpatpmt != 0 )
			{
	                        /* A audiotrack is desired that doesn't exist */
        	                if (audioIndex >= apidsfound){
                	                 ReelBoxDevice::Instance()->SetAudioTrack(0);
                        	         audioIndex = 0;
                        	}
				packet.vpid = vpidpatpmt;
				packet.apid = apidpatpmt[audioIndex];
				//printf ( "VALID PATPMT: vpid: %x apid: %x audioIndex: %x\n",packet.vpid, packet.apid, audioIndex );
				//    if(oldvpid != packet.vpid || oldapid != packet.apid)
				//		packet.generation++;
				HdCommChannel::chStream1.SendPacket ( HD_PACKET_TS_DATA, packet, d, 0 );

				oldvpid = vpidpatpmt;
				int i = 0;
				while ( apidpatpmt[i] )
				{
					oldapidpatpmt[i] = apidpatpmt[i];
					i++;
				}
			}
		}
		if ( !d || !length )
			return;

		//printf("DEBUG: PlayTsPacket: len: %i \n", length);

		// sync to 0x47
		if ( length>=2*TS_SIZE )
		{
			while (length>=TS_SIZE &&  d[0]!=0x47 && d[TS_SIZE]!=0x47 )
			{
				sendbuf[buffill++] = *d;
				d++;
				length--;
			}
		}

		if ( oldAudioIndex != audioIndex )
		{
			//printf("Audio Index changed from %i to %i\n", oldAudioIndex, audioIndex);
			oldAudioIndex = audioIndex;
		}
		packet.vpid=oldvpid; // Always include PIDs in packet
		packet.apid=oldapidpatpmt[oldAudioIndex];

		if ( buffill>=TS_SIZE )
		{
			HdCommChannel::chStream1.SendPacket ( HD_PACKET_TS_DATA, packet, ( Byte const* ) &sendbuf, buffill );
			buffill=0;
		}

#define MAXPACKLEN (TS_SIZE*64)
		while ( length>=TS_SIZE )
		{
			int len_rounded = length - ( length%TS_SIZE );
			int tosend = len_rounded > MAXPACKLEN ? MAXPACKLEN : len_rounded;
			HdCommChannel::chStream1.SendPacket ( HD_PACKET_TS_DATA, packet, d+bytes_sent, tosend );
//		HdCommChannel::chStream1.SendPacket(HD_PACKET_TS_DATA, packet, d, length);
			bytes_sent += tosend;
			length -= tosend;
		}
		if ( length>0 )
		{
			memcpy ( &sendbuf, d+bytes_sent, length );
			buffill+=length;
		}
	}

	void VideoPlayerHd::PlayPacket ( Mpeg::EsPacket const &esPacket, bool still )
	{
		Byte const *data = esPacket.GetData();
		int dataLength = esPacket.GetDataLength();
		//set vpts shift in shm: late audio corresponds to early video
		if ( pts_shift_ != 10 * RBSetup.delay_stereo )
		{
			hdPlayer_.pts_shift = pts_shift_ = 10 * RBSetup.delay_stereo;
		}
		if ( ac3_pts_shift_ != 10 * RBSetup.delay_ac3 )
		{
			hdPlayer_.ac3_pts_shift = ac3_pts_shift_ = 10 * RBSetup.delay_ac3;
		}

		hdPlayer_.data_generation = generation_; // Must be done _before_ sending the packet!

		hd_packet_es_data_t packet;

		packet.generation = generation_;
		packet.timestamp = esPacket.HasPts() ? esPacket.GetPts() : 0;
		packet.stream_id = esPacket.GetStreamId();
		packet.still_frame = still;
		HdCommChannel::chStream1.SendPacket ( HD_PACKET_ES_DATA, packet.header, sizeof ( packet ), data, dataLength );
		freeze_ = false;
	}

	//--------------------------------------------------------------------------------------------------------------

	void VideoPlayerHd::PlayPesPacket ( void *data, int length, int av ) // av=0 -> Audio
	{
		hd_packet_pes_data_t packet;

		if ( pts_shift_ != 10 * RBSetup.delay_stereo )
		{
			hdPlayer_.pts_shift = pts_shift_ = 10 * RBSetup.delay_stereo;
		}
		if ( ac3_pts_shift_ != 10 * RBSetup.delay_ac3 )
		{
			hdPlayer_.ac3_pts_shift = ac3_pts_shift_ = 10 * RBSetup.delay_ac3;
		}
		packet.generation = generation_;
		packet.av=av;
		HdCommChannel::chStream1.SendPacket ( HD_PACKET_PES_DATA, packet.header, sizeof ( packet ), ( const Reel::Byte* ) data, length );
		freeze_ = false;
	}

	int VideoPlayerHd::AproxFramesInQueue(void) {
		return hdPlayer_.decoder_frames;
	}

	//--------------------------------------------------------------------------------------------------------------

	bool VideoPlayerHd::Poll()
	{
		return !freeze_;
	}

	//--------------------------------------------------------------------------------------------------------------

	void VideoPlayerHd::StillPicture ( Mpeg::EsPacket const esPackets[], UInt packetCount )
	{
		//printf("VideoPlayerHd::StillPicture()\n");
		/* TB: resend picture settings, auto format may have resetted them */
                Reel::HdCommChannel::SetPicture(&RBSetup);
//		const UInt repeat = 40; //send the frame several times (still frame prob with hdext)
		// Set to 4 or showing picture will result in audio scatch
//		const UInt repeat = 4; //send the frame several times (still frame prob with hdext)
#if VDRVERSNUM < 10716
		const UInt repeat = 6; //send the frame several times (still frame prob with hdext) - increased again due to fade-in
#else
		const UInt repeat = 15;
#endif
		for ( UInt rep = 0; rep < repeat; ++rep )
		{
			for ( UInt n = 0; n < packetCount; ++n )
			{
				Mpeg::EsPacket const &esPacket = esPackets[n];
				PlayPacket ( esPacket, true );
			}
		}
	}

	//--------------------------------------------------------------------------------------------------------------

	void VideoPlayerHd::Start()
	{
		/* fallback against old BSP-config values when using the HD-ext */
		hd_channel_invalidate ( HdCommChannel::chStream1.ch_, 1 );
		Clear();

		hd_packet_rpc_done_t packet;
		memset(&packet, 0, sizeof(packet));
		HdCommChannel::chStream1.SendPacket ( HD_PACKET_RPC_DONE, packet, 0, 0 );
		if (RBSetup.usehdext && ((RBSetup.brightness > 255 || RBSetup.contrast > 255 || RBSetup.gamma > 195) || (RBSetup.brightness == 0 || RBSetup.contrast == 0 || RBSetup.gamma == 0))) {
			esyslog("ERROR: Picture settings out of range. Resetting to factory default.\n");
			RBSetup.brightness = fs453_defaultval_tab0_HD;
			RBSetup.contrast   = fs453_defaultval_tab1_HD;
			RBSetup.colour     = fs453_defaultval_tab4_HD;
			RBSetup.sharpness  = fs453_defaultval_tab3_HD;
			RBSetup.gamma      = fs453_defaultval_tab2_HD;
			RBSetup.flicker    = fs453_defaultval_tab5_HD;
		}
		else
			dsyslog("Picture settings OK.");

                /* TB: resend picture settings, auto format may have resetted them */
		Reel::HdCommChannel::SetPicture(&RBSetup);
		Play();
	}

	//--------------------------------------------------------------------------------------------------------------

	void VideoPlayerHd::Stop()
	{
		//printf ( "VideoPlayerHd::Stop()\n" );

		hd_channel_invalidate ( HdCommChannel::chStream1.ch_, 1 );
		Clear();

		hd_packet_off_t packet;
		HdCommChannel::chStream1.SendPacket ( HD_PACKET_OFF, packet, NULL, 0 );
	}

	//--------------------------------------------------------------------------------------------------------------

	void VideoPlayerHd::Trickmode ( UInt trickSpeed )
	{
		//printf ( "VideoPlayerHd::Trickmode(%d)\n", trickSpeed );
		bool iFramesOnly = trickSpeed != 2 && trickSpeed != 4 && trickSpeed != 8;
		// HACK: VDR will uses these trick speeds for "slow forward", the only trickmode with all frame types.

		hdPlayer_.pause=0;
		hdPlayer_.trickmode=iFramesOnly;
		hdPlayer_.trickspeed=trickSpeed;

		freeze_ = false;
	}

	//--------------------------------------------------------------------------------------------------------------
}
