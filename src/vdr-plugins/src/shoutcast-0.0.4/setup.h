#ifndef SHOUTCAST_SETUP__H
#define SHOUTCAST_SETUP__H
#include <vdr/menuitems.h>

struct cInternetRadioSetupParams 
{
private:
	time_t lastRead; // time when the key was read from file last

public:
	cInternetRadioSetupParams();
	char shoutcastKey[17];
	int shoutcastKeyEnabled;

	bool ReadKey();
	bool KeyFileChanged() const;

	bool IsValidKey() const;
	bool ShouldAskForKey() const;
}; // struct


class cInternetRadioSetupPage: public cMenuSetupPage
{
private:
	cInternetRadioSetupParams& param_; // reference to real param
        cInternetRadioSetupParams tmpParam;
        // this osd can be called from Internet radio Menu
        // in this case do not offer to disable dev-Id
        bool isSetupPage_;
        bool showEditPage_;
public:
	cInternetRadioSetupPage(cInternetRadioSetupParams&, bool isSetupPage=true);
	virtual ~cInternetRadioSetupPage();
	virtual void Store();

	// Set all the items here and call Display()
	void PrepareOsd(); 
	void ShowDevIdPage();
	void EditDevIdPage();

	eOSState ProcessKey(eKeys key);

}; // class

extern cInternetRadioSetupParams internetRadioParams;
#endif

