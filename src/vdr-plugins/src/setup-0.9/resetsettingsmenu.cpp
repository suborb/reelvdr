
#include <string>

#include "resetsettingsmenu.h"

#define RESETSCRIPT "/root/bin/factorysettings.sh "

#ifdef RBMINI
#define RESETSCRIPT_OPTS "--netclient"
#else
#define RESETSCRIPT_OPTS "--factory"
#endif

#define RESETSCRIPT_OPTS_END " 1>/dev/null 2>/dev/null"
#define MAX_LETTER_PER_LINE 48

cMenuSetupResetSettings::cMenuSetupResetSettings(void)
:cOsdMenu(tr("Factory defaults"), 25)
{
    Set();
}

eOSState cMenuSetupResetSettings::ProcessKey(eKeys Key)
{
    if (Key == kRed) {
        Key = Skins.Message(mtWarning, tr("Really reset?"), 20);
        if(kOk != Key) {
            ; //Skins.Message(mtWarning, tr("Cancelling..."), 10);
        } else {
            ResetSettings();
            return osContinue;
        }
    }

    eOSState state = cOsdMenu::ProcessKey(Key);
    return state;
}

void cMenuSetupResetSettings::Set(void)
{
    SetHelp(tr("Reset"), NULL, NULL, NULL);

    Add(new cOsdItem("", osUnknown, false), false, NULL);  //blank line
    AddFloatingText(tr("Use the red button to reset to factory defaults."), MAX_LETTER_PER_LINE);
    Add(new cOsdItem("", osUnknown, false), false, NULL);  //blank line
    Add(new cOsdItem("", osUnknown, false), false, NULL);  //blank line
    Add(new cOsdItem("", osUnknown, false), false, NULL);  //blank line
    Add(new cOsdItem("", osUnknown, false), false, NULL);  //blank line
    Add(new cOsdItem("", osUnknown, false), false, NULL);  //blank line
    Add(new cOsdItem("", osUnknown, false), false, NULL);  //blank line
    Add(new cOsdItem("", osUnknown, false), false, NULL);  //blank line
    Add(new cOsdItem("", osUnknown, false), false, NULL);  //blank line
    Add(new cOsdItem("", osUnknown, false), false, NULL);  //blank line
    Add(new cOsdItem(tr("Info:"), osUnknown, false), false, NULL);
    AddFloatingText(tr("All settings will be lost!"), MAX_LETTER_PER_LINE);
}

void cMenuSetupResetSettings::ResetSettings(void)
{
    Skins.Message(mtStatus, tr("Resetting ReelBox to factory defaults..."));
    std::string command = std::string(RESETSCRIPT) + " " + RESETSCRIPT_OPTS + " " + RESETSCRIPT_OPTS_END;
    SystemExec(command.c_str());

    Skins.Message(mtStatus, tr("Rebooting..."));
    SystemExec("reboot now");
    SystemExec("killall -9 vdr");
}


