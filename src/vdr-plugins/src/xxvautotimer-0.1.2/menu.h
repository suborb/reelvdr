/****************************************************************************
 * DESCRIPTION:
 *            
 *
 * $Id: menu.h,v 1.5 2005/12/03 13:12:42 ralf Exp $
 *
 * Contact:    ranga@teddycats.de
 *
 * Copyright (C) 2005 by Ralf Dotzert
 ****************************************************************************/
#ifndef MENU_H
#define MENU_H

#include <mysql/mysql.h>
#include <vdr/menu.h>
#include <vdr/interface.h>
#include "autotimer.h"
#include "libmysqlwrapped.h"

/**
@author Ralf Dotzert
*/

struct XXV_Command
{
  char * description;
  char * command;
};

class cXxvAutotimerCommand : public cOsdMenu
{
  private:
    enum          Mode { UNDEF, COMMAND};
    cXxvAutotimerCommand::Mode mode;
    void SendCommand(char *command);
    void CallSystem();

  public:
    cXxvAutotimerCommand();
    ~cXxvAutotimerCommand();
    void  Set();
    eOSState ProcessKey(eKeys Key);
};


class cXxvAutotimerEdit : public cOsdMenu
{
private:
   cXxvAutoTimer *_timer;

public:
    cXxvAutotimerEdit(cXxvAutoTimer *timer);
    ~cXxvAutotimerEdit();
    void  Set();
    eOSState ProcessKey(eKeys Key);
};


class cXxvAutotimerMenu : public cOsdMenu
{
private:
    enum          State { UNDEF, EDIT, NEW, DELETE, COMMAND};
    State         _state;

    Database      *_db;
    SysLog        *_syslog;
    cXxvAutoTimers _timers;
    bool           getTimers();
    cXxvAutoTimer  _currTimer;
    void           updateTimer();
    void           insertTimer();
    void           deleteTimer();


public:
    cXxvAutotimerMenu();
    ~cXxvAutotimerMenu();
    void  Set();
    eOSState ProcessKey(eKeys Key);
};

#endif
