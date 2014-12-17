/****************************************************************************
 * DESCRIPTION:
 *            
 *
 * $Id: setup.h,v 1.6 2005/12/03 13:12:42 ralf Exp $
 *
 * Contact:    ranga@vdrtools.de
 *
 * Copyright (C) 2005 by Ralf Dotzert
 ****************************************************************************/
#ifndef SETUP_H
#define SETUP_H

#include <vdr/plugin.h>

/**
@author Ralf Dotzert
*/


// holds Configuration
class cXxvAutotimerSetup
{
public:
    char *allowed;
    char *strHost;
    char *strTelnetPort;
    char *strDatabase;
    char *strDatabasePort;
    char *strDatabaseUser;
    char *strDatabasePasswd;
    char *strLifetime;
    char *strPriority;
    char *strPrevMinutes;
    char *strAfterMinutes;
    char *strXXV020;
    char *strDefaultDone;

    char host[50];
    int  telnetPort;
    char database[50];
    int  dbPort;
    char user[50];
    char passwd[50];
    int  defaultLifetime;
    int  defaultPriority;
    int  defaultPrevMinutes;
    int  defaultAfterMinutes;
    int  defaultDone;
    int  xxv020; // TRUE if we are using an old version
    bool useXXVConfigFile;

    cXxvAutotimerSetup();
    bool SetupParse(const char *Name, const char *Value);

};
extern cXxvAutotimerSetup autotimerSetup;



class cXxvAutotimerSetupPage : public cMenuSetupPage
{
private:

protected:
  virtual void Store(void);
public:
  cXxvAutotimerSetupPage(void);
};


#endif
