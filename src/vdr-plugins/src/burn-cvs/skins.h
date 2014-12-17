#ifndef VDR_BURN_SKIN_H
#define VDR_BURN_SKIN_H

#include "common.h"
#include "gdwrapper.h"
#include <list>
#include <string>
#include <vector>

namespace vdr_burn
{

	// --- skin ---------------------------------------------------------------

	class skin
	{
	public:
		struct button
		{
			std::string image;
			gdwrapper::point position;
			gdwrapper::text_spec region;

			button( const std::string& image_, const gdwrapper::point& position_, const gdwrapper::text_spec& region_ ):
					image( image_ ), position( position_ ), region( region_ ) {}
		};

		typedef std::list<button> menu_list;

	private:
		static std::string default_background;
		static std::string default_button;

		bool        m_default;
		std::string m_name;
		std::string m_background;
		std::string m_button;

		gdwrapper::text_spec m_mainMenuHeader;
		button m_mainMenuPrevious;
		button m_mainMenuNext;
		menu_list m_mainMenuTitles;

		gdwrapper::text_spec m_titleMenuHeader;
		button m_titleMenuPlay;
		button m_titleMenuExit;
		button m_titleMenuPrevious;
		button m_titleMenuNext;
		gdwrapper::text_spec m_titleMenuText;

	public:
		skin(const std::string& name, const std::string& background,
			 const std::string& button_, bool default_ = false);

		static void set_defaults(const std::string& background, const std::string& button);

		const std::string& get_name() const { return m_name; }
		const std::string& get_background() const { return m_background; }

		const gdwrapper::text_spec& get_main_menu_header() const { return m_mainMenuHeader; }
		const button& get_main_menu_previous() const { return m_mainMenuPrevious; }
		const button& get_main_menu_next() const { return m_mainMenuNext; }
		const menu_list& get_main_menu_titles() const { return m_mainMenuTitles; }

		const gdwrapper::text_spec& get_title_menu_header() const { return m_titleMenuHeader; }
		const button& get_title_menu_play() const { return m_titleMenuPlay; }
		const button& get_title_menu_exit() const { return m_titleMenuExit; }
		const button& get_title_menu_previous() const { return m_titleMenuPrevious; }
		const button& get_title_menu_next() const { return m_titleMenuNext; }
		const gdwrapper::text_spec& get_title_menu_text() const { return m_titleMenuText; }

		friend bool operator<(const skin& lhs, const skin& rhs);
	};

	// --- skin_list ----------------------------------------------------------

	// TODO aggregate
	class skin_list: public std::vector<skin>
	{
	private:
		static skin_list instance;

	protected:
		static std::string check_skin_files( const std::string& pathOnly, const std::string& filePrefix, bool fatal = false );

	public:
		static bool load( const std::string& path );

		static const string_list& get_names();

		static skin_list& get() { return instance; }
	};

}

#endif // VDR_BURN_SKIN_H
