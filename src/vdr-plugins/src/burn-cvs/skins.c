#include "burn.h"
#include "skins.h"
#include "proctools/logger.h"
#include "proctools/format.h"
#include <unistd.h>
#include <iostream>
#include <algorithm>
#include <boost/bind.hpp>
#include <vdr/i18n.h>
#include <vdr/tools.h>

namespace vdr_burn
{

	using namespace std;
	using namespace gdwrapper;
	using proctools::logger;
	using proctools::format;

	const int MAXPAGETITLES = 8;

	skin_list skin_list::instance;

	bool operator<(const skin& lhs, const skin& rhs)
	{
		if (lhs.m_default)
			return true;
		else if (rhs.m_default)
			return false;
		else
			return lhs.m_name < rhs.m_name;
	}

	// --- skin ---------------------------------------------------------------

	std::string skin::default_background;
	std::string skin::default_button;

	skin::skin(const std::string& name, const std::string& background,
			   const std::string& button_, bool default_):
			m_default( default_ ),
			m_name( name ),
			m_background( background.empty() ? default_background : background ),
			m_button( button_.empty() ? default_button : button_ ),
			m_mainMenuHeader( rectangle( 90, 52, 450, 30 ), font_spec( "Vera", 16 ),
							  color( 0xff, 0xff, 0xff ) ),
			m_mainMenuPrevious( m_button, point( 605, 509 ),
								text_spec( rectangle( 120, 513, 240, 25 ), font_spec( "Vera", 16 ),
										color( 0xff, 0xff, 0xff ) ) ),
			m_mainMenuNext( m_button, point( 80, 509 ),
							text_spec( rectangle( 360, 513, 240, 25 ), font_spec( "Vera", 16 ),
									color( 0xff, 0xff, 0xff ), 2 ) ),
			m_titleMenuHeader( rectangle( 90, 52, 450, 30 ), font_spec( "Vera", 16 ),
							   color( 0xff, 0xff, 0xff ) ),
			m_titleMenuPlay( m_button, point( 80, 509 ),
							 text_spec( rectangle( 120, 513, 240, 25 ), font_spec( "Vera", 16 ),
									 color( 0xff, 0xff, 0xff ) ) ),
			m_titleMenuExit( m_button, point( 605, 509 ),
							 text_spec( rectangle( 360, 513, 240, 25 ), font_spec( "Vera", 16 ),
									 color( 0xff, 0xff, 0xff ), 2 ) ),
			m_titleMenuPrevious( m_button, point( 80, 469 ),
								 text_spec( rectangle( 120, 472, 240, 25 ), font_spec( "Vera", 16 ),
										 color( 0xff, 0xff, 0xff ) ) ),
			m_titleMenuNext( m_button, point( 605, 469 ),
							 text_spec( rectangle( 360, 472, 240, 25 ), font_spec( "Vera", 16 ),
									 color( 0xff, 0xff, 0xff ), 2 ) ),
			m_titleMenuText( rectangle( 90, 120, 450, 340 ), font_spec( "Vera", 16 ),
									 color( 0xff, 0xff, 0xff ) )
	{
		for (int i = 0; i < MAXPAGETITLES; ++i)
			m_mainMenuTitles.push_back(
					button( m_button, point( 93, 118 + i * 40 ),
							text_spec( rectangle( 138, 124 + i * 40, 450, 30 ),
									font_spec( "Vera", 16 ),
									color( 0xff, 0xff, 0xff ) ) ) );

		replace(m_name.begin(), m_name.end(), '_', ' ');
		logger::debug(format("loaded skin {0} using {1} and {2}")
					  % m_name % m_background % m_button);
	}

	void skin::set_defaults(const std::string& background,
							const std::string& button)
	{
		default_background = background;
		default_button = button;
	}

	// --- skin_list ----------------------------------------------------------

	class skin_file_checker
	{
	public:
		skin_file_checker( const std::string& pathPrefix ):
				m_pathPrefix( pathPrefix ) {}

		void operator()( const char* extension )
		{
			string toCheck = format( "{0}.{1}" ) % m_pathPrefix % extension;
			if ( access( toCheck.c_str(), R_OK ) != -1 )
				path = toCheck;
		}

		std::string path;

	private:
		std::string m_pathPrefix;
	};

	string skin_list::check_skin_files( const std::string& pathOnly, const std::string& filePrefix, bool fatal )
	{
		static const char* extensions[] = { "jpg", "jpeg", "png" };

		string pathPrefix( format( "{0}/{1}" ) % pathOnly % filePrefix );
		skin_file_checker checker( for_each( extensions, extensions + sizeof(extensions)/sizeof(extensions[0]),
											 skin_file_checker( pathPrefix ) ) );
		if ( checker.path.empty() ) {
			if ( fatal )
				logger::error( format( "couldn't find image for {0}: no suitable format found" ) % pathPrefix );
			return string();
		}
		return checker.path;
	}

	bool skin_list::load( const std::string& path )
	{
		string background = check_skin_files( path, "menu-bg", true );
		string button     = check_skin_files( path, "menu-button", true );

		if (background.empty() || button.empty())
			return false;

		skin::set_defaults(background, button);

		get().insert(get().end(), skin(tr("Default"), string(), string(), true));

		string skinPath = format("{0}/skins") % path;
		cReadDir dir( skinPath.c_str() );
		struct dirent *ent;
		while ( ( ent = dir.Next() ) != NULL ) {
			if ( ent->d_name[0] == '.' ) // hide dotfiles, '.' and '..'
				continue;

			background = check_skin_files( format( "{0}/{1}" ) % skinPath % ent->d_name, "menu-bg" );
			button     = check_skin_files( format( "{0}/{1}" ) % skinPath % ent->d_name, "menu-button" );

			if ( !background.empty() || !button.empty() )
				get().insert( get().end(), skin( ent->d_name, background, button ) );
		}
		sort( get().begin(), get().end() );

		return true;
	}

	const string_list& skin_list::get_names()
	{
		static string_list skinNames;
		if ( skinNames.size() == 0 ) {
			skinNames.resize( get().size() );
			transform( get().begin(), get().end(), skinNames.begin(), bind( &string::c_str, bind( &skin::get_name, _1 ) ) );
		}
		return skinNames;
	}

}
