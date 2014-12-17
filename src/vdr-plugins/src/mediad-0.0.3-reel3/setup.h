#ifndef VDR_VDRCD_SETUP_H
#define VDR_VDRCD_SETUP_H

#include <vdr/plugin.h>
#include "mediaplugin.h"
#include <vector>

struct cVdrcdSetup {
    cVdrcdSetup(void);
    cVdrcdSetup(const cVdrcdSetup &setup);
    cVdrcdSetup& operator=(const cVdrcdSetup &setup);
    bool SetupParse(const char *Name, const char *Value);
    cList<mediaplugin> mediaplugins;
    int HideMenu;
};

extern cVdrcdSetup VdrcdSetup;
extern int AutoStartMode;
extern int RequestDvdAction;
extern int RequestCdAction;

class cVdrcdMenuSetupPage: public cMenuSetupPage {
private:
    cVdrcdSetup _setup;
    int _AutoStartMode;
//        int RequestDvdAction_;
    
protected:
    virtual void Store(void);

public:
    cVdrcdMenuSetupPage(void);
    virtual ~cVdrcdMenuSetupPage();
};

#endif // VDR_VDRCD_SETUP_H
