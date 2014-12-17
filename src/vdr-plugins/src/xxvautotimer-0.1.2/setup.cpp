/****************************************************************************
 * DESCRIPTION:
 *             Setup Dialog
 *
 * $Id: setup.cpp,v 1.6 2005/12/03 13:12:42 ralf Exp $
 *
 * Contact:    ranga@vdrtools.de
 *
 * Copyright (C) 2005 by Ralf Dotzert
 ****************************************************************************/

#include "setup.h"




cXxvAutotimerSetup::cXxvAutotimerSetup()
{
  allowed           =  " abcdefghijklmnopqrstuvwxyz0123456789-_/";
  strHost           = "Host";
  strTelnetPort     = "Telnet-Port";
  strDatabase       = "Database";
  strDatabasePort   = "Database-Port";
  strDatabaseUser   = "Database-User";
  strDatabasePasswd = "Database-Password";
  strLifetime       = "Default-Lifetime";
  strPriority       = "Default-Priority";
  strPrevMinutes    = "Default-prevminutes";
  strAfterMinutes   = "Default-afterminutes";

  strXXV020         = "XXV Version < 0.30";
  strDefaultDone    = "Ignore recurrences";

  
  strcpy(host, "localhost");
  strcpy(database, "xxv");
  strcpy(user,"root");
  strcpy(passwd,"root");
  //dbPort          = 80;
  dbPort          = 3304;
  telnetPort      = 8081;
  defaultDone     = false;
  defaultLifetime = 50;
  defaultPriority = 50;
  xxv020          = false;
  useXXVConfigFile = false;
}

bool cXxvAutotimerSetup::SetupParse( const char * Name, const char * Value )
{
    if      (strcmp(Name, strDatabase )       == 0) snprintf(database, sizeof(database), Value);
    else if (strcmp(Name, strHost )           == 0) snprintf(host, sizeof(host), Value);
    else if (strcmp(Name, strTelnetPort )     == 0) telnetPort = atoi(Value);
    else if (strcmp(Name, strDatabasePort )   == 0) dbPort   = atoi(Value);
    else if (strcmp(Name, strDatabaseUser )   == 0) snprintf(user, sizeof(user), Value);
    else if (strcmp(Name, strDatabasePasswd ) == 0) snprintf(passwd, sizeof(passwd), Value);
    else if (strcmp(Name, strLifetime )       == 0) defaultLifetime   = atoi(Value);
    else if (strcmp(Name, strPriority )       == 0) defaultPriority   = atoi(Value);
    else if (strcmp(Name, strXXV020 )         == 0) xxv020      = atoi(Value);
    else if (strcmp(Name, strDefaultDone )    == 0) defaultDone = atoi(Value);
    else return false;

    return true;
}
//--------------------------------------------------------------


cXxvAutotimerSetupPage::cXxvAutotimerSetupPage( void )
{
  //Add(new cMenuEditBoolItem(tr(autotimerSetup.strXXV020),       &autotimerSetup.xxv020));
  Add(new cMenuEditBoolItem(tr(autotimerSetup.strDefaultDone),  &autotimerSetup.defaultDone));
   if( autotimerSetup.useXXVConfigFile)
   {
     char *tmp = NULL;
     Add( new cOsdItem(""));
     Add( new cOsdItem(tr("XXVD Configuration is used:")));
     
     asprintf(&tmp, "%s:\t%s", tr(autotimerSetup.strHost), autotimerSetup.host);
     Add( new cOsdItem(tmp));
     free(tmp);

     asprintf(&tmp, "%s:\t%d", tr(autotimerSetup.strTelnetPort), autotimerSetup.telnetPort);
     Add( new cOsdItem(tmp));
     free(tmp);

     asprintf(&tmp, "%s:\t%s", tr(autotimerSetup.strDatabase), autotimerSetup.database);
     Add( new cOsdItem(tmp));
     free(tmp);
     
     asprintf(&tmp, "%s:\t%d", tr(autotimerSetup.strDatabasePort), autotimerSetup.dbPort);
     Add( new cOsdItem(tmp));
     free(tmp);
     
     asprintf(&tmp, "%s:\t%s", tr(autotimerSetup.strDatabaseUser), autotimerSetup.user);
     Add( new cOsdItem(tmp));
     free(tmp);
     
     asprintf(&tmp, "%s:\t%s", tr(autotimerSetup.strDatabasePasswd), autotimerSetup.passwd);
     Add( new cOsdItem(tmp));
     free(tmp);
     
     asprintf(&tmp, "%s:\t%d", tr(autotimerSetup.strLifetime), autotimerSetup.defaultLifetime);
     Add( new cOsdItem(tmp));
     free(tmp);
     
     asprintf(&tmp, "%s:\t%d", tr(autotimerSetup.strPriority), autotimerSetup.defaultPriority);
     Add( new cOsdItem(tmp));
     free(tmp);
     
     asprintf(&tmp, "%s:\t%d", tr(autotimerSetup.strPrevMinutes), autotimerSetup.defaultPrevMinutes);
     Add( new cOsdItem(tmp));
     free(tmp);
     
     asprintf(&tmp, "%s:\t%d", tr(autotimerSetup.strAfterMinutes), autotimerSetup.defaultAfterMinutes);
     Add( new cOsdItem(tmp));
     free(tmp);
      
   }
   else
   {
   
   /*
     Add(new cMenuEditStrItem(tr(autotimerSetup.strHost),          autotimerSetup.host, sizeof(autotimerSetup.host) + 1, autotimerSetup.allowed));
     Add(new cMenuEditIntItem(tr(autotimerSetup.strTelnetPort),  &autotimerSetup.telnetPort,1,65000));
     Add(new cMenuEditStrItem(tr(autotimerSetup.strDatabase),      autotimerSetup.database, sizeof(autotimerSetup.database) + 1, autotimerSetup.allowed));
    Add(new cMenuEditIntItem(tr(autotimerSetup.strDatabasePort),  &autotimerSetup.dbPort,1,65000));
    Add(new cMenuEditStrItem(tr(autotimerSetup.strDatabaseUser),  autotimerSetup.user, sizeof(autotimerSetup.user) + 1, autotimerSetup.allowed));
    Add(new cMenuEditStrItem(tr(autotimerSetup.strDatabasePasswd),autotimerSetup.passwd, sizeof(autotimerSetup.passwd) + 1, autotimerSetup.allowed));
    Add(new cMenuEditIntItem(tr(autotimerSetup.strLifetime),      &autotimerSetup.defaultLifetime,1,MAXLIFETIME));
    Add(new cMenuEditIntItem(tr(autotimerSetup.strPriority),      &autotimerSetup.defaultPriority,1,MAXPRIORITY));    
    Add(new cMenuEditIntItem(tr(autotimerSetup.strPrevMinutes),      &autotimerSetup.defaultPrevMinutes,0, 90));
    Add(new cMenuEditIntItem(tr(autotimerSetup.strAfterMinutes),      &autotimerSetup.defaultAfterMinutes,0,90));
*/     
   }
  
}


void cXxvAutotimerSetupPage::Store( void )
{
  SetupStore(autotimerSetup.strHost,           autotimerSetup.host);
  SetupStore(autotimerSetup.strTelnetPort,     autotimerSetup.telnetPort);
  SetupStore(autotimerSetup.strDatabase,       autotimerSetup.database);
  SetupStore(autotimerSetup.strDatabasePort,   autotimerSetup.dbPort);
  SetupStore(autotimerSetup.strDatabaseUser,   autotimerSetup.user);
  SetupStore(autotimerSetup.strDatabasePasswd, autotimerSetup.passwd);
  SetupStore(autotimerSetup.strLifetime,       autotimerSetup.defaultLifetime);
  SetupStore(autotimerSetup.strPriority,       autotimerSetup.defaultPriority);
  SetupStore(autotimerSetup.strPrevMinutes,    autotimerSetup.defaultPrevMinutes);
  SetupStore(autotimerSetup.strAfterMinutes,   autotimerSetup.defaultAfterMinutes);
  SetupStore(autotimerSetup.strXXV020,         autotimerSetup.xxv020);
  SetupStore(autotimerSetup.strDefaultDone,    autotimerSetup.defaultDone);
}

