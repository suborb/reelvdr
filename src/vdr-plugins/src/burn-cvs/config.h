/*
 * See the files COPYING and README for copyright information and how to reach
 * the author.
 *
 *  $Id: config.h,v 1.12 2006/09/16 18:33:36 lordjaxom Exp $
 */

#ifndef VDR_BURN_CONFIG_H
#define VDR_BURN_CONFIG_H

#include "burn.h"
#include "common.h"
#include "etsi-const.h"
#include "jobs.h"
#include "proctools/format.h"
#include "proctools/logger.h"
#include <fstream>
#include <string>
#include <vdr/i18n.h>
#include <vdr/tools.h>

namespace vdr_burn
{

	// --- submux_xml ---------------------------------------------------------

	template<typename T>
	class submux_xml
	{
	public:
		submux_xml(const T& item);

		void write(int page);

		std::string get_xml_path(int page) const;

	private:
		const T& m_item;
	};

	template<typename T>
	inline
	std::string submux_xml<T>::get_xml_path(int page) const
	{
		return proctools::format("{0}/menu-{1}.xml")
			   % m_item.get_paths().data % page;
	}

	template<typename T>
	submux_xml<T>::submux_xml(const T& item):
		m_item(item)
	{
	}

	template<typename T>
	void submux_xml<T>::write(int page)
	{
		std::string path = get_xml_path(page);
		std::ofstream f(path.c_str(), std::ios::out);
		if (!f) {
			proctools::logger::error(proctools::format("couldn't create {0}")
									 % path);
			return;
		}
		f << "<?xml version=\"1.0\" encoding=\"" << plugin::get_character_encoding() << "\"?>"
			 "<subpictures><stream><spu force=\"yes\" start=\"00:00:00.00\" image=\""
		  << m_item.get_buttons_normal() << "\" select=\""
		  << m_item.get_buttons_normal() << "\" highlight=\""
		  << m_item.get_buttons_highlight(page) << "\" autooutline=\"infer\" "
			 "outlinewidth=\"5\" autoorder=\"columns\"/></stream></subpictures>"
		  << std::endl;
	}

	// --- dvdauthor_xml  -----------------------------------------------------

	class dvdauthor_xml
	{
	public:
		dvdauthor_xml(const job& job_);

		void write();

		std::string get_xml_path() const;
		std::string get_author_path() const;

		static std::string get_author_path(const job& job_);

	protected:
//		static const char* get_video_format(etsi_const::component_type::type type_);
		static std::string get_video_aspect(track_info::aspectratio typeBits);
		static std::string get_audio_language(int language);

		void write_main_menu();
		void write_title_menu(recording_list::const_iterator rec);

	private:
		const job&    m_job;
		std::ofstream m_file;
	};

	inline
	std::string dvdauthor_xml::get_xml_path() const
	{
		return proctools::format("{0}/dvd.xml") % m_job.get_paths().data;
	}

	inline
	std::string dvdauthor_xml::get_author_path() const
	{
		return get_author_path(m_job);
	}

	inline
	std::string dvdauthor_xml::get_author_path(const job& job_)
	{
		return proctools::format("{0}/DVDAUTHOR") % job_.get_paths().data;
	}

}

#endif // VDR_BURN_CONFIG_H
