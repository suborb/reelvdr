/*
 * See the files COPYING and README for copyright information and how to reach
 * the author.
 *
 *  $Id: tracks.h,v 1.7 2006/09/16 18:33:37 lordjaxom Exp $
 */

#ifndef VDR_BURN_TRACKS_H
#define VDR_BURN_TRACKS_H

#include "common.h"
#include <string>
#include <list>
#include <memory>
#include <vdr/epg.h>

namespace vdr_burn
{

	// --- track_info ---------------------------------------------------------

	struct track_info
	{
		enum streamtype
		{
			streamtype_video = 1,
			streamtype_audio,
			streamtype_subtitle
		};

		enum aspectratio
		{
			aspectratio_quadric = 1,
			aspectratio_4_3,
			aspectratio_16_9,
			aspectratio_21_9
		};

		enum framerate
		{
			framerate_23_967 = 1,
			framerate_24,
			framerate_25,
			framerate_29_97,
			framerate_30_97
		};

		enum audiotype
		{
			audiotype_mpeg_2_5 = 1,
			audiotype_mpeg_2,
			audiotype_mpeg_1,
			audiotype_ac3
		};

		enum audiolayer
		{
			audiolayer_1 = 1,
			audiolayer_2,
			audiolayer_3
		};

		enum audiomode
		{
			audiomode_stereo = 1,
			audiomode_joint_stereo,
			audiomode_dual_channel,
			audiomode_mono
		};

		enum ac3mode
		{
		};

		enum subtitletype
		{
			subtitletype_dvb = 1,
			subtitletype_teletext
		};

		int cid;
		std::string filename;
		int language;
		std::string description;
		streamtype type;
		int bitrate;
		size_pair size;
		bool used;

		union
		{ // XXX polymorphic
			struct
			{
				aspectratio aspect;
				framerate frameRate;
			} video;

			struct
			{
				audiotype type;
				union
				{
					struct
					{
						audiolayer layer;
						audiomode mode;
					} mpeg;

					struct
					{
						ac3mode mode;
					} ac3;
				};
			} audio;

			struct
			{
				subtitletype type;
				int teletextpage;
			} subtitle;
		};

		track_info(int cid, streamtype type);

		static const string_list& get_language_codes();

		void set_language(const std::string& language);

		bool get_is_video() const { return type == streamtype_video; }
		bool get_is_audio() const { return type == streamtype_audio; }
		const int get_pesid() const { return (cid & 0xFFFF); }
		const int get_pid() const { return (cid >> 16); }

		std::string get_type_description() const;
	};

	// --- track_info_list ----------------------------------------------------

	class track_info_list: public std::list<track_info>
	{
	public:
		const track_info& operator[](int index) const;
		track_info& operator[](int index);

		const_iterator find_cid(int cid) const;
		iterator find_cid(int cid);
	};

}

#endif // VDR_BURN_TRACKS_H
