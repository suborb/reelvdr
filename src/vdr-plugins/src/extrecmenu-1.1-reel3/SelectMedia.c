#include "SelectMedia.h"
#include "SelectFile.h"
#include "ExternalMedia.h"

#include <vdr/videodir.h>

#define INTERNAL_HDD tr("Internal HDD")
#define EXTERNAL_MEDIA tr("External Media")
#define INFO_TEXT trNOOP("Please select the drive where to move the recording:")

cSelectMediaMenu::cSelectMediaMenu(std::string path): cOsdMenu(tr("Select Media"))
{
    srcDir = path;
    Set();
}


void cSelectMediaMenu::Set()
{
    int count = ExtMediaCount(); // needed to Call ExtMediaCount()
    const std::vector<std::string> MediaList = ExtMediaMountPointList();
    char buffer[128];
    int fsSize = 0;

    AddFloatingText(tr(INFO_TEXT), 50); 
    Add(new cOsdItem("", osUnknown, false));

    Add(new cOsdItem( INTERNAL_HDD, osUnknown, true));
    Add(new cOsdItem("", osUnknown, false));

    for (int i = 0; i < count ; i++)
    {
        fsSize = FileSystemSizeMB( MediaList.at(i) );
        if(fsSize >= 1024)
            snprintf(buffer, 127, "%s #%i (%.1f GB)", EXTERNAL_MEDIA, i+1, ((float)fsSize/1024));
        else
            snprintf(buffer, 127, "%s #%i (%i MB)", EXTERNAL_MEDIA, i+1, fsSize );
        Add(new cOsdItem(buffer, osUnknown, true));
    }

    SetHelp(tr("Cancel"));
    Display();
}

eOSState cSelectMediaMenu::ProcessKey(eKeys key)
{
    eOSState state = cOsdMenu::ProcessKey(key);

    if (HasSubMenu())
    {
        if (state == osUser3)
        {
            CloseSubMenu();
            return osBack;
        }

        return state;
    }

    if (state == osUnknown)

    switch(key)
    {
        case kOk:
            {
                cOsdItem *item = Get(Current());
                if (item && item->Text())
                {
                    const char *p = item->Text();
                    if ( strcmp(p, INTERNAL_HDD) == 0)
                        return AddSubMenu(new cSelectfileMenu(srcDir, VideoDirectory ));

                    // point to the start of numbers
                    p += strlen( EXTERNAL_MEDIA )  + 2 + 0; // '+2' for ' #'
                    int num;
                    sscanf(p, " %i", &num); //
                    const std::vector<std::string> MediaList = ExtMediaMountPointList();
                    --num;
                    //printf("'%s' \033[1;91m %s : '%i'\033[0m\n", p, item->Text(), num);

                    if (num >= 0 && (int)MediaList.size() > num)
                        return AddSubMenu(new cSelectfileMenu(srcDir, MediaList.at(num) ));
                    else
                        Skins.Message(mtError, tr("Could not open media"));

                }
                return osContinue;
            }
            break;

        case kRed: // cancel
            return osBack;

        default:
            break;
    }

    return state;
}
