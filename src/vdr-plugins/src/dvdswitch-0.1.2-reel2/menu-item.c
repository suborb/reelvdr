#include "menu.h"
#include "menu-item.h"
#include "setup.h"
#include "imagelist.h"

// --- cMainMenuItem -----------------------------------------------------

cMainMenuItem::cMainMenuItem(eMainMenuItem itype, const char *file)
{
  MYDEBUG("Neues MainMenuItem: %i, %s", (int)itype, file);

  iType = itype;
  File = file ? strdup(file) : NULL;

  OSD = cMainMenu::CreateOSDName(iType, File);

  SetText(OSD, true);
  if(iType == iCat)
    SetSelectable(false);

  MYDEBUG("OSDName: %s", OSD);
}

cMainMenuItem::~ cMainMenuItem()
{
  free(File);
  free(OSD);
}

