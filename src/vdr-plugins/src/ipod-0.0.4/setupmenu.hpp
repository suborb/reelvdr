
#ifndef __IPODSETUTMENU_H_
#define __IPODSETUTMENU_H_

//***************************************************************************
// Includes
//***************************************************************************

#include <vdr/plugin.h>
#include "ipod.hpp"

//***************************************************************************
// iPod Setup Menu
//***************************************************************************

class PodSetupMenu : public cMenuSetupPage
{

   public:
      
      PodSetupMenu();
      
   protected:
      
      void Store(void);

      // data

      char mountPoint[sizePath+TB];
      char mountScript[sizePath+TB];
      char playlistPath[sizePath+TB];

};

//***************************************************************************
#endif // __IPODSETUTMENU_H_
