/*
 * See the files COPYING and README for copyright information and how to reach
 * the author.
 *
 *  $Id: common.c,v 1.29 2006/10/01 21:22:27 lordjaxom Exp $
 */

#include "burn.h"
#include "common.h"
#include "iconvwrapper.h"
#include "setup.h"
#include "proctools/logger.h"
#include "proctools/format.h"
#include "boost/bind.hpp"
#include "patchfont.h"
#include <sstream>
#include <string>
#include <iterator>
#include <algorithm>
#include <iomanip>
#include <ctime>
#include <vdr/recording.h>
#include <vdr/config.h>
#include <vdr/i18n.h>
#include <sys/types.h>
#include <glob.h>

namespace vdr_burn
{

	using namespace std;
	using proctools::logger;
	using proctools::format;
	using boost::bind;

	// --- disktype -----------------------------------------------------------

	const char* disktype_strings[disktype_count] =
			{ trNOOP("DVD with menus"), trNOOP("DVD without menus"), trNOOP("Archive disk") };

	// --- storemode ----------------------------------------------------------

	const char* storemode_strings[storemode_count] =
			{ trNOOP("ISO file only"), trNOOP("Disc"), trNOOP("ISO file and Disc") };

	// --- chaptersmode -------------------------------------------------------

	const char* chaptersmode_strings[chaptersmode_count] =
			{ trNOOP("No chapters"), trNOOP("Use editing marks"), trNOOP("Every 5 minutes"), trNOOP("Every 10 minutes"),
			  trNOOP("Every 15 minutes"), trNOOP("Every 30 minutes"), trNOOP("Every hour") };

	const int chaptersmode_intervals[chaptersmode_count] =
			{ -1, -1, 5, 10, 15, 30, 60 };

	// --- disksize -----------------------------------------------------------

	const char* disksize_strings[disksize_count] =
			{ "CD-R", "Single Layer", "Double Layer", "BlueRay", "BlueRay" };

	const int disksize_values[disksize_count] =
			{ 1, 690, 4472, 7944, 23500, 47300 }; // TODO: Which value should we use for BlueRay?

	const char* disksize_strings_capacity[disksize_count] =
			{ trNOOP("none"), "CD-R (700MB)", "DVD (4.7GB)", "DVD-DL (8.5GB)", "BlueRay (25GB)", "BlueRay (50GB)" };

	// --- demuxtype ----------------------------------------------------------

	const char* demuxtype_strings[demuxtype_count] =
			{ "VDRSync", "ProjectX", "Replex" };

	// --- requanttype --------------------------------------------------------

	const char* requanttype_strings[requanttype_count] =
			{ "M2VRequantizer", "Transcode", "Don't requant" };

const char *TitleChars = "abcdefghijklmnopqrstuvwxyz"
						 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
						 "0123456789"
						 "�������:- ";

int ScanPageCount(const std::string& Path)
{
	cString path = AddDirectory(Path.c_str(), "menu-bg-*.png");
	glob_t globbuf;
	int res;

	if ((res = glob(path, 0, NULL, &globbuf)) != 0) {
		const char *error = "Unknown error";

		switch (res) {
			case GLOB_NOSPACE: error = "Out of memory"; break;
			case GLOB_ABORTED: error = "I/O error"; break;
			case GLOB_NOMATCH: error = "File not found"; break;
		}

		esyslog("ERROR[burn]: Error while scanning %s: %s", *path, error);
		return -1;
	}

	res = globbuf.gl_pathc;
	globfree(&globbuf);
	return res;
}

std::string progress_bar(double current, double total, int length)
{
	if (length < 2)
		return "";

	if (current > total)
		current = total;

	int l = static_cast<int>(current * (length - 2) / total);
	ostringstream builder;

	builder << '[';
	fill_n(ostream_iterator<char>(builder), l, '|');
	fill_n(ostream_iterator<char>(builder), length - 2 - l, ' ');
	builder << ']';
	return builder.str();
}

	void trim_left( std::string& text_, const char* characters_, std::string::size_type offset_ )
	{
	    std::string::size_type pos;
	    if ( ( pos = text_.find_first_not_of( characters_, offset_ ) ) > 0 )
	        text_.erase( offset_, pos - offset_ );
	}

string get_recording_datetime(const cRecording* recording_, char delimiter)
{
	string title = recording_->Title('\t', false, -1
#ifdef LIEMIKUUTIO // hatred... HATRED!!!
									 , true
#endif
									);

	string::iterator it = title.begin();
	for (int i = 0; i < 2 && it != title.end(); ++i)
		it = find(it + 1, title.end(), '\t');

	title.erase(it, title.end());
	if (title[title.length() - 1] == ' ')
		title.erase(title.length() - 1);
	if (delimiter != '\t')
		replace(title.begin(), title.end(), '\t', delimiter);
	return title;
}

string get_recording_date(const cRecording* recording_, char delimiter)
{
	string title = recording_->Title('\t', false, -1
#ifdef LIEMIKUUTIO // hatred... HATRED!!!
									 , true
#endif
									);

	string::iterator it = title.begin();
	for (int i = 0; i < 1 && it != title.end(); ++i)
		it = find(it + 1, title.end(), '\t');

	title.erase(it, title.end());
	if (title[title.length() - 1] == ' ')
		title.erase(title.length() - 1);
	if (delimiter != '\t')
		replace(title.begin(), title.end(), '\t', delimiter);
	return title;
}

string get_recording_title(const cRecording* recording_, int level)
{
	stringstream result;
	string name( recording_->Name());
#if 0
	PatchFont(fontSml);
#endif
        /*
        if (recording_->Info()->Title())
	        name = recording_->Info()->Title();
        */
	if (recording_->Info()->ShortText() && strcmp(recording_->Info()->ShortText(), recording_->Name())!=0) {
		name += " - ";
		name += recording_->Info()->ShortText();
	}

	if (level == recording_->HierarchyLevels()) {
		int recSize = DirSizeMB(recording_->FileName()) + 50; // add 50 for correct rounding
		string::size_type pos;
		for ( int l = 0; l < level && ((pos = name.find('~')) != string::npos); ++l )
			name.erase(0, pos + 1);
		ostringstream archivePath;
		archivePath << recording_->FileName() << "/dvd.vdr";
		if (access(archivePath.str().c_str(), F_OK) == 0)
			result << '\x81';
		else
		{
			if (recording_->IsNew())
				result << '\x84';
		}
		if (recording_->IsEdited())
			result << '\x85';
		result << '\t';
		result << get_recording_date(recording_)<< '\t';
		result << recSize / 1000 << '.';
		result << (recSize % 1000) / 100 << " GB " << '\t';
		if (recording_->IsHD())
			result << '\x86' << ' ';
		result << name;
		return result.str();
	}

	string::size_type lastOffset = 0;
	string::size_type offset;
	while ((offset = name.find('~', lastOffset)) != string::npos) {
		if (level-- == 0)
			break;
		lastOffset = offset + 1;
	}
	result << "\t\t" << name.substr(lastOffset, offset - lastOffset);
	return result.str();
}

std::string get_recording_description(const cRecording* recording_)
{
#if VDRVERSNUM < 10300
	if (recording_->Summary() != 0)
		return recording_->Summary();
#else
	if (recording_->Info()->Description() != 0)
		return recording_->Info()->Description();
	else if (recording_->Info()->ShortText() != 0)
		return recording_->Info()->ShortText();
#endif

	return "";
}

string string_replace( const string& text, char from, char to )
{
	string result( text );
	replace( result.begin(), result.end(), from, to );
	return result;
}

bool elapsed_since(time_t& timestamp, time_t difference)
{
	time_t now = time(NULL);
	if (timestamp + difference <= now) {
		timestamp = now;
		return true;
	}
	return false;
}

string int_to_string(int value, int base, bool prefix)
{
	stringstream conv;
	if (prefix) {
		switch (base) {
		case 2: conv << "0b"; break;
		case 8: conv << "0"; break;
		case 16: conv << "0x"; break;
		}
	}
	conv << setbase(base) << value;

	string result;
	conv >> result;
	return result;
}

string get_recording_name(const cRecording* recording)
{
	string title( recording->Title() );
	string::size_type pos( title.rfind('~') );
	if (pos != string::npos)
		title.erase( pos );
	return title;
}

	inline char clean_path_character( char ch )
	{
		//! Table with clean characters for path names.
		static const char cleanPathCharacters[] =
		{
		//	  0 ,  1 ,  2 ,  3 ,  4 ,  5 ,  6 ,  7 ,  8 ,  9 ,  A ,  B ,  C ,  D ,  E ,  F ,
			 '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_',	//00
			 '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_',	//10
			 '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', 0x2D,0x2E,0x2F,	//20
			 0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,'_', '_', '_', '_', '_', '_',	//30
			 '_', 0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,	//40
			 0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,'_', '_', '_', '_', '_',	//50
			 '_', 0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,	//60
			 0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,'_', '_', '_', '_', '_' 	//70
			 // everything above 127 (for signed char, below zero) is replaced with '_'
		};

		if ( static_cast<signed char>( ch ) <= 0 ) // since not all implementations consider char _arrays_ to be signed, make sure
			return '_';
		return cleanPathCharacters[ int( ch ) ];
	}

	///////////////////////////////////////////////////////////////////////////////
	//! Clean path name, so it just contains save characters.
	/*! Use this function to ensure functionality in various charset environments.
	 All "bad" characters are replaced with '_'.
	 CAUTION: for UTF16, we need to adjust the table or add a upper limit.
	 \param text Path name to be cleaned up
	 \return Cleaned path named
	*/
	string clean_path_name( const string& text )
	{
		string result;
		result.resize( text.length() );
		// I like the functional approach ;)
		transform( text.begin(), text.end(), result.begin(), clean_path_character );
		return result;
	}

	string convert_to_utf8( const string& text )
	{
		if ( plugin::get_character_encoding() == "utf-8" )
			return text;

		iconvwrapper::iconv iso_to_utf( plugin::get_character_encoding(), "utf-8" );
		string result = iso_to_utf( text );
		if ( !iso_to_utf ) {
			logger::error( "couldn't convert characters from iso-8859-15 to utf-8" );
			throw std::string( tr("Couldn't convert character sets!") );
		}
		return result;
	}

}
