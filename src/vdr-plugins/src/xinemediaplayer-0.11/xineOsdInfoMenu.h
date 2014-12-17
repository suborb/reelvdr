#ifndef __XINE_FILEINFO_H__
#define __XINE_FILEINFO_H__

#include <vdr/osdbase.h>
#include <vdr/status.h>
#include <vdr/debugmacros.h>

#include <string>
#include <vector>
#include <time.h>

namespace Reel{
    namespace XineMediaplayer{



        ////////////////////////////////////////////////////////////////////////
        // File Info Menu
        ////////////////////////////////////////////////////////////////////////
        class cFileInfoMenu: public cOsdMenu
        {
            private:
                // absolute path of the file
                std::string 			mrl_; 
                
                // vector that is displayed on the OSD
                // got from GetFileInfo()
                std::vector<std::string>	fileInfoVec_; 


            public: 
                cFileInfoMenu(std::string mrl);
                ~cFileInfoMenu();

                virtual eOSState	ProcessKey(eKeys Key);
                void		ShowInfo();
        };

    } //namespace
} //namespace

#endif
