#include "setup.h"
#include "menu.h"

#include <vdr/menuitems.h>
#include <vdr/osdbase.h>
#include <vdr/plugin.h>

#define KEY_FILE "/etc/vdr/plugins/shoutcast.devid"

cInternetRadioSetupParams::cInternetRadioSetupParams(): lastRead(0)
{

}

bool cInternetRadioSetupParams::KeyFileChanged() const
{
    time_t t = LastModifiedTime(KEY_FILE);
    return t > lastRead;
}

bool cInternetRadioSetupParams::ReadKey()
{
    FILE *f = fopen(KEY_FILE, "r");
    if (!f) return false;

    char tmpkey[17] = {0};
    fscanf(f, " %s", tmpkey);
    fclose(f);
    lastRead= time(NULL);

    tmpkey[16] = 0;

    printf("read key: '%s'\n", tmpkey);
    // check if key is "valid" before copying
    if (strlen(tmpkey) == 16)
    {
        strcpy(shoutcastKey, tmpkey);
        return true;
    }
    printf("key not valid len('%s'):= %d\n", tmpkey, strlen(tmpkey));
    return false;
}

bool cInternetRadioSetupParams::IsValidKey() const
{
    // for the time being check just the length
    return strlen(shoutcastKey) == 16;
}

bool cInternetRadioSetupParams::ShouldAskForKey() const
{
    return !IsValidKey() && shoutcastKeyEnabled;
}

cInternetRadioSetupPage::cInternetRadioSetupPage(cInternetRadioSetupParams & param, bool isSetupPage):param_(param),
    tmpParam(param), isSetupPage_ (isSetupPage)
{
    // if key not valid, show edit page
    showEditPage_ = !param_.IsValidKey();

    PrepareOsd();
}

cInternetRadioSetupPage::~cInternetRadioSetupPage()
{
}

void cInternetRadioSetupPage::PrepareOsd()
{
    Clear();
    SetCols(25);
    SetTitle(cString::sprintf("%s: %s", trVDR("Setup"), "Internet Radio"));

    if (showEditPage_)          // Key is incorrect
        EditDevIdPage();
    else                        // key is possibly correct
        ShowDevIdPage();
}

void cInternetRadioSetupPage::ShowDevIdPage()
{
    SetTitle(cString::sprintf("%s: %s", trVDR("Setup"), "Internet Radio"));
    Add(new cOsdItem(tr("Your SHOUTcast is active."), osUnknown, false));
    Add(new cOsdItem("", osUnknown, false));    //blank line

    //lets make this bright
    Add(new
        cOsdItem(cString::
        sprintf("%s: %s", tr("Your SHOUTcast DevID is"), param_.shoutcastKey),
                 osUnknown, false), false);

    SetHelp(tr("Reset DevID"), NULL, NULL, NULL);
    Display();

}

void cInternetRadioSetupPage::EditDevIdPage()
{
    #define MAX_LETTER_PER_LINE 50

    SetTitle(cString::sprintf("%s: %s", trVDR("Setup"), "Internet Radio"));
    Add(new cOsdItem(tr("Welcome to Internetradio setup"), osUnknown, false));
    Add(new cOsdItem("", osUnknown, false));    //blank line

    AddFloatingText(tr
                    ("To use the internet radio service from SHOUTcast you need an access-key since their switch to the new Interface (API 2.0). "
                    ), MAX_LETTER_PER_LINE);
    //Add(new cOsdItem("", osUnknown, false));//blank line

    AddFloatingText(tr
    ("You can register for this DevID directly on the Website of the AOL Developer Network http://dev.aol.com/keys ."),
                    MAX_LETTER_PER_LINE);
    Add(new cOsdItem("", osUnknown, false));    //blank line

    //AddFloatingText(tr("Press \"Cancel\" if you do not have a Shoutcast-ID. You can then only listen to your own radio stations."), LINE_LEN);
    Add(new cOsdItem("", osUnknown, false));    //blank line

    //Add(new cOsdItem(tr("Please enter Key"), osUnknown, false));

    cMenuEditStrItem *keyItem =
        new cMenuEditStrItem(tr("Enter DevID"), tmpParam.shoutcastKey, 17,
                             "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-_+#");
    Add(keyItem, true);         // Set as current item

    if (isSetupPage_)
    {
        Add(new cOsdItem("", osUnknown, false));        //blank line
        Add(new cMenuEditBoolItem(tr("Show this page again"), &tmpParam.shoutcastKeyEnabled,
                              trVDR("no"), trVDR("yes")));
    }

    SetHelp(tr("Cancel"), NULL, NULL, NULL);
    Display();
}



eOSState cInternetRadioSetupPage::ProcessKey(eKeys key)
{

    eOSState state = cMenuSetupPage::ProcessKey(key);

    printf("State(%d) == osBack (%d )?\n", state, osBack);
    /* When in edit page, showEditPage_
     ** After editing, state == osBack for key == kOk
     ** show the Dev-Id page
     */
    if (showEditPage_ && state == osBack && kOk == key)
    {
        if (tmpParam.IsValidKey())
        {
            showEditPage_ = false;
            PrepareOsd();
            return osContinue;
        }
        else
        {
            // warn, not the complete key
            Skins.Message(mtInfo,
                          tr("DevID is to short, it should be 16 characters long"));
            return osContinue;
        }
    }                           // if

    /* check if edit mode has been exited,
     ** if so display Help keys again
     */
    if (state == osContinue)
    {
        switch (key)
        {
        case kOk:
        case kBack:            // out of edit mode; set the help keys

            // key is valid => Save it
            if (tmpParam.IsValidKey())
                SetHelp(tr("Cancel"), NULL, NULL, tr("Save"));
            else
                SetHelp(tr("Cancel"), NULL, NULL, NULL);

            break;

        default:
            break;
        }                       // switch
    }                           // if

    if (state == osUnknown)
    {
        switch (key)
        {
        case kRed:             // Cancel
            //param_.shoutcastKeyEnabled = 1;
            //SetupStore("shoutcastKeyEnabled", param_.shoutcastKeyEnabled);
            if (showEditPage_)
                return osBack;

            // not in edit page
            showEditPage_ = true;
            PrepareOsd();
            return osContinue;

        case kBlue:            // Save
        case kOk:
            printf("Calling Store()\n");
            Store();
            return osBack;

        default:
            return osContinue;  // donot allow other keys to steal the focus; like '<' key (Audio stream selection)
            break;
        }                       // switch
    }                           // if

    return state;
}



void cInternetRadioSetupPage::Store()
{
    //printf("\033[0;41m%s\033[0m\n", __PRETTY_FUNCTION__);
    // 'cMenuSetupPage::plugin' is not initialized when
    // this osd is not called from Setup Menu
    if (!isSetupPage_)
    {
        cMenuSetupPage::SetPlugin(cPluginManager::GetPlugin("shoutcast"));
    }

    strncpy(param_.shoutcastKey, tmpParam.shoutcastKey, 17);
    param_.shoutcastKeyEnabled = tmpParam.shoutcastKeyEnabled;

    SetupStore("shoutcastkey", tmpParam.shoutcastKey);
    SetupStore("shoutcastKeyEnabled", param_.shoutcastKeyEnabled);
    Setup.Save();
    //Skins.Message(mtInfo, tr("Settings stored"));

    // redraws shoutcast Menu
    cShoutcastMenu::triggerRead = true;
}
