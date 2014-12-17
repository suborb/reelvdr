
//***************************************************************************
// Includes
//***************************************************************************

#include <vdr/menuitems.h>
#include "setupmenu.hpp"

//***************************************************************************
// Object
//***************************************************************************

PodSetupMenu::PodSetupMenu()
{
   strcpy(mountScript, cPluginIpod::mountScript);
   strcpy(mountPoint, cPluginIpod::mountPoint);
   strcpy(playlistPath, cPluginIpod::playlistPath);

   Add(new cMenuEditStrItem(tr("Mount Script"),  mountScript, 500, trVDR(FileNameChars)));
   Add(new cMenuEditStrItem(tr("Mount Point"), mountPoint, 500, trVDR(FileNameChars)));
   Add(new cMenuEditStrItem(tr("mp3 Plugin's Playlist Path"), playlistPath, 500, trVDR(FileNameChars)));
}

//***************************************************************************
// Store
//***************************************************************************

void PodSetupMenu::Store(void)
{
  SetupStore("mountScript", mountScript);
  SetupStore("mountPoint", mountPoint);
  SetupStore("playlistPath", playlistPath);
}

