#ifndef VDR_VDRCD_SETUP_H
#define VDR_VDRCD_SETUP_H

#include <vdr/plugin.h>

struct cVdrcdSetup {
	cVdrcdSetup(void);

	bool SetupParse(const char *Name, const char *Value);

	int AutoStartVCD;
	int AutoStartMP3;
};

extern cVdrcdSetup VdrcdSetup;

class cVdrcdMenuSetupPage: public cMenuSetupPage {
private:
	cVdrcdSetup m_NewVdrcdSetup;
	
protected:
	virtual void Store(void);

public:
	cVdrcdMenuSetupPage(void);
	virtual ~cVdrcdMenuSetupPage();
};

#endif // VDR_VDRCD_SETUP_H
