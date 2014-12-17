#include "setup.h"

#define LEN 256

cMMfilesSetup::cMMfilesSetup()
{
    unsigned int i=0;
    char *path;
    NumDirs = cDirList::Instance().ListSize();
    for ( ; i< cDirList::Instance().ListSize(); ++i)
    {
        path = strdup(cDirList::Instance().PathAt(i).c_str());
        DirList.push_back( path );
    }
    Set();
}


void cMMfilesSetup::Set()
{
    unsigned int i=0;
    char itemText[256];
    Clear();

    SetCols(15);
    for (i=0; i<DirList.size(); ++i)
    {
        sprintf(itemText,"Path %i", i+1);
        Add(new cMenuEditStrItem(itemText, DirList.at(i), LEN-1, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz/. "));
    }
    SetHelp(tr("Delete"), tr("New"));
    SetHelp(tr("Delete"), tr("New"),NULL, tr("Rescan"));
    Display();
}

void cMMfilesSetup::Store()
{
    char setup_store_text[LEN+28];
    unsigned int i;

    // Save
    for (i=0; i<DirList.size(); ++i)
    {
        snprintf(setup_store_text, LEN+28-1,"ScanDirTree%i",i); 
        SetupStore(setup_store_text, DirList.at(i));
        printf("Stored %s : %s\n", setup_store_text, DirList.at(i));
    }

    // remove the rest of the dirs, if any
    for ( ; i<NumDirs; ++i)
    {
        snprintf(setup_store_text, LEN+28-1,"ScanDirTree%i",i); 
        SetupStore(setup_store_text);
    }

    NumDirs = DirList.size();
}

eOSState cMMfilesSetup::ProcessKey(eKeys key)
{
    // restart scan thread
    static bool RestartThread = false; 

    eOSState state = cMenuSetupPage::ProcessKey(key);

    if (state != osUnknown && (key == kOk || key == kBack) ) // exiting Edit Mode
    {
        if ( 1 || RestartThread)
            SetHelp(tr("Delete"), tr("New"),NULL, tr("Rescan"));
        else
            SetHelp(tr("Delete"), tr("New"));

    }

    if (state == osUnknown)
    {
        switch(key)
        {
            case kGreen:
                {
                    char * path = new char [LEN];
                    path[0] = 0;
                    DirList.push_back(path); 
                    Set();
                    return osContinue;
                }
                break;

            case kRed:
                {
                    int current = Current();
                    const char *text = Get(current)->Text();
                    if ( strncasecmp(text, "path",4) == 0)
                        DirList.erase(DirList.begin()+current);
                    Set();
                    return osContinue;
                }
            case kOk:
                RestartThread = true;
                LoadDirList();
                return osContinue;

            case kBlue:
                if (1 || RestartThread)
                {
                    // load new dir list to cDirList object
                    LoadDirList();

                    RestartThread = false;
                    // restart thread here
                    scandirthread->Stop();
                    SetStatus(tr("Starting ReScan.."));
                    usleep(200);
                    scandirthread->Start();
                    SetStatus(NULL);

                    return osContinue;
                }
                else
                    return state;

            default:
                break;
        }

    }

        return state;
}

void cMMfilesSetup::LoadDirList()
{
    cDirList::Instance().ClearList();
    for(unsigned int i=0; i < DirList.size(); ++i)
        cDirList::Instance().AddPath(DirList.at(i));
}

