#ifndef __FILEINFOMENU_H__
#define __FILEINFOMENU_H__

#include <vdr/osdbase.h>
#include <vdr/status.h>
#include <vdr/menu.h>

#ifdef REELVDR
#include <vdr/debugmacros.h>
#endif

#include <string>
#include <vector>
#include <time.h>

////////////////////////////////////////////////////////////////////////
// File Info Menu
////////////////////////////////////////////////////////////////////////
class cFileInfoMenu: public cOsdMenu
{
	private:
		// absolute path of the file
		std::string             mrl_;

		// vector that is displayed on the OSD
		// got from GetFileInfo()
		std::vector<std::string>    fileInfoVec_;

	public:
		cFileInfoMenu(std::string mrl);
		~cFileInfoMenu();

		virtual eOSState    ProcessKey(eKeys Key);
		void        ShowInfo();
};


#endif
