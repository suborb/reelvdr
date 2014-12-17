/****************************************************************************
 * DESCRIPTION:
 *             Utitility Methods
 *
 * $Id: util.cpp,v 1.4 2005/10/03 14:05:20 ralf Exp $
 *
 * Contact:    ranga@teddycats.de
 *
 * Copyright (C) 2004 by Ralf Dotzert
 ****************************************************************************/


#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <iostream>

#include <vdr/debug.h>
#include <vdr/submenu.h>

#include "util.h"
#include <vdr/sysconfig_vdr.h>
#include <vdr/channels.h>
#include <vdr/timers.h>
#include <vdr/thread.h>

using std::string;
using std::cout;
using std::endl;

#define DBG ""

//#define PATH_MAX 255

namespace setup
{
    namespace
    {
        void LoadDummy()
        {
            //cout << " ++  LoadDummy() ++ " << endl;
            const char * tmpFileName = "/tmp/.tmpChannels.conf";
            FILE * f = fopen(tmpFileName, "w+");

            fprintf(f, "VDR:1:H:0:27500:0:0:0:0:1:0:0:0\n");
            fclose(f);

            //bool Load(const char *FileName, bool AllowComments = false, bool MustExist = false);
            Channels.Load(tmpFileName, false, true);     // Read Channel-List
            if (Channels.Count())
                Channels.SwitchTo(1);

            cout << " ++  LoadDummy() ++ END " << endl;
        }
    }


    const char * ExecuteCommand(const char *cmd)
    {
        //printf ("  --------------  ExecuteCommand  (%s) ------------------- \n", cmd);
        char * commandResult = NULL;

        //dsyslog("executing command '%s'", cmd);
        //
        cPipe p;
        if (p.Open(cmd, "r"))
        {
            int
                l = 0;
            int
                c;
            while ((c = fgetc(p)) != EOF)
            {
                if (l % 20 == 0)
                    commandResult = (char *)realloc(commandResult, l + 21);
                commandResult[l++] = c;
            }
            if (commandResult)
                commandResult[l] = 0;

            p.Close();
        }
        else
            esyslog("ERROR: can't open pipe for command '%s'", cmd);

        return commandResult;
        //return strdup (_commandResult);
    }


    bool MakeBool(string str, bool defaults)
    {
        if (str.compare("true") == 0)
            return true;
        if (str.compare("yes") == 0)
            return true;
        if (str.compare("1") == 0)
            return true;
        if (str.compare("on") == 0)
            return true;
        if (str.compare("false") == 0)
            return false;
        if (str.compare("no") == 0)
            return false;
        if (str.compare("0") == 0)
            return false;
        if (str.compare("off") == 0)
            return false;

        return defaults;
    }

    // -----------  setup::SwitchChannelList(const char *SelectedChannelList)            ---------------------

    void SwitchChannelList(const char *SelectedChannelList)
    {
        //cout << " ++ SwitchChannelList ++ " << SelectedChannelList << endl;
        if (SelectedChannelList == NULL)
            return;

        //link "/etc/video/channels.conf" dir  "/etc/video/channels" file "/etc/video/channels.conf"
        ::Setup.UpdateChannels = 0;
        string channelLink = setup::FileNameFactory("link");
        string newChannelFile = setup::FileNameFactory(SelectedChannelList);

        //strip plugins
        char resolvedPath[PATH_MAX];
        if (realpath(channelLink.c_str(), resolvedPath) == NULL)
        {
            DLOG("Can not resolve realpath of %s  errno=%d\n", 
                 channelLink.c_str(), errno);
        } else {
            // Test bevore enter this  funktion ( setupmenu keyhandling )
            if (strcmp(resolvedPath, newChannelFile.c_str()) != 0)      // channels changed !!
            {
                //cout << " old List: " << resolvedPath << endl;

                Channels.SwitchTo(1);

                if (unlink(channelLink.c_str()) == 0)
                {
                    if (symlink(newChannelFile.c_str(), channelLink.c_str()) != 0)
                    {
                        DLOG("%s Can not link File %s  to %s errno=%d\n", DBG,
                             newChannelFile.c_str(), channelLink.c_str(), errno);
                    }
                    TimerList timers;
                    timers.SaveTimer();
                    LoadDummy();
                    //cout << " ++ SwitchChannelList LoadDummy() Count : " << Channels.Count() << "  ++ " << endl;
                    Channels.Load(channelLink.c_str(), false, true);    // Read Channel-List
                    Channels.Count() > 0 ? Channels.SwitchTo(1) : Channels.SwitchTo(0);
                    timers.RestoreTimer();
                } else {
                    DLOG("%s Can not unlink File %s  errno=%d\n", DBG, channelLink.c_str(), errno);
                }
            }
        }
        //cout << " ++ SwitchChannelList return ++ " << endl;
    }
}

Util::Util()
{
}

Util::~Util()
{
}

static const char *undefStr = "undefined";
static const char *boolStr = "bool";
static const char *textStr = "text";
static const char *constantTextStr = "constant text";
static const char *numTextStr = "numtext";
static const char *hexStr = "hex";
static const char *ipStr = "ip";
static const char *pinStr = "pincode";
static const char *numberStr = "number";
static const char *selectionStr = "selection";
static const char *spaceStr = "space";


static const char *trueStr = "true";
static const char *falseStr = "false";
static const char *yesStr = "yes";
static const char *noStr = "no";


char * Util::Strdupnew(const char *str)
{
    return str ? strcpy(new char[strlen(str) + 1], str) : 0;
}

char * Util::Strdupnew(const char *str, int size)
{
    char *result = NULL;
    if (str != NULL)
    {
        result = strncpy(new char[size + 1], str, size);
        result[size] = '\0';
    }

    return (result);
}


char * Util::Strdupnew(const char *prefix, const char *str)
{
    char *newStr = NULL;

    if (str != NULL && prefix != NULL)
    {
        int len = strlen(prefix) + strlen(str);
        newStr = new char[(len + 1)];
        sprintf(newStr, "%s%s", prefix, str);
    }
    return (newStr);
}


/**
 * check id the given string represenst a bool value
 * @param string string to compare for bool representation
 * @param flag corresponding bool value
 * @return false if error was detected
 */
bool Util::isBool(const char *string, bool & flag)
{
    bool ok = true;

    if (string != NULL)
    {
        if (strcmp(string, trueStr) == 0 || strcmp(string, yesStr) == 0)
            flag = true;
        else if (strcmp(string, falseStr) == 0 || strcmp(string, noStr) == 0)
            flag = false;
        else
            ok = false;
    }
    else
        ok = false;

    if (!ok)
        DLOG("Illegal Bool value '%s' found", string);

    return (ok);
}

bool Util::isBool(const char *string, int &flag)
{
    bool ok = true;
    bool boolVal;

    if (isBool(string, boolVal))
    {
        if (boolVal)
            flag = true;
        else
            flag = false;
    }
    return (ok);
}


/**
 * check id the given string represenst a Type value
 *
 * known types are "text", "bool", "number", "numtext" "ip"
 * @param string string to compare for type representation
 * @param typ corresponding type value
 * @return false if error was detected
 */
bool Util::isType(const char *string, Util::Type & type)
{
    bool ok = true;

    if (string != NULL)
    {
        if (strcmp(string, boolStr) == 0)
            type = Util::BOOL;
        else if (strcmp(string, textStr) == 0)
            type = Util::TEXT;
        else if (strcmp(string, constantTextStr) == 0)
            type = Util::CONSTANT_TEXT;
        else if (strcmp(string, numTextStr) == 0)
            type = Util::NUMBER_TEXT;
        else if (strcmp(string, hexStr) == 0)
            type = Util::HEX;
        else if (strcmp(string, numberStr) == 0)
            type = Util::NUMBER;
        else if (strcmp(string, ipStr) == 0)
            type = Util::IP;
        else if (strcmp(string, selectionStr) == 0)
            type = Util::SELECTION;
        else if (strcmp(string, spaceStr) == 0) 
           type = Util::SPACE;
        else if (strcmp(string, pinStr) == 0)
            type = Util::PIN;
        else
            ok = false;
    }
    else
        ok = false;

    if (!ok)
        DLOG("Illegal Type value '%s' found", string);

    return (ok);
}


/**
 *
 * @param string
 * @param number
 * @return
 */
bool Util::isNumber(const char *string, int &number)
{
    bool ok = true;
    number = 0;

    if (string != NULL)
    {
        int len = strlen(string);
        for (int i = 0; i < len && ok == true; i++)
            if (string[i] < '0' || string[i] > '9')
                ok = false;
            else
                number = number * 10 + (string[i] - '0');
    }
    else
        ok = false;

    return (ok);

}


/**
 * Returns the strung representation of given type
 * @param type
 * @return
 */
const char * Util::boolToStr(bool val)
{
    const char *result;

    if (val == true)
        result = yesStr;
    else
        result = noStr;

    return (result);
}



/**
 * Returns the strung representation of given type
 * @param type
 * @return
 */
const char * Util::typeToStr(Type type)
{
    switch (type)
    {
    case BOOL:
        return boolStr;
    case TEXT:
        return textStr;
    case CONSTANT_TEXT:
        return constantTextStr;
    case NUMBER_TEXT:
        return numTextStr;
    case HEX:
        return hexStr;
    case NUMBER:
        return numberStr;
    case IP:
        return ipStr;
    case PIN:
        return pinStr;
    case SELECTION:
        return selectionStr;
    case SPACE:
        return spaceStr;
    case UNDEFINED:
        return undefStr;
    }
    return undefStr;
}

// ---- Class cChildLock ------------------------------------------


cChildLock::cChildLock():childLock_(false)
{

}

void cChildLock::Init()
{
    if (cSysConfig_vdr::GetInstance().GetVariable("CHILDLOCK")) 
    {
       childLockSysConf_ = cSysConfig_vdr::GetInstance().GetVariable("CHILDLOCK");
    } else {
         childLockSysConf_.clear();
    }

    if (childLockSysConf_ == "0000" || childLockSysConf_.empty())
        childLock_ = false;
    else
        childLock_ = false;     // <- (ar) childlock over pin-pl (true) 
}

// ---- Class cExecuteCommand ------------------------------------------

cExecuteCommand::cExecuteCommand():comState_(eCSCommandNoOp)
{
}

eComState cExecuteCommand::Execute(string Command)
{
    command_ = Command;
    Start();
    comState_ = eCSCommandRunning;
    return comState_;
}

void cExecuteCommand::Action()
{

#ifdef DEBUG_SETUP
    dsyslog(DBG " Exec Action");
#endif
    string pathName;

    string::size_type pos = command_.find(' ');
    if (pos != string::npos)
    {
        //string::size_type pos = line.find(' ');
        pathName = command_.substr(pos);
#ifdef DEBUG_SETUP
        dsyslog("Out String %s ", pathName.c_str());
#endif
    }
    else
    {
        pathName = command_;
#ifdef DEBUG_SETUP
        dsyslog("Out String %s ", pathName.c_str());
#endif
    }



//dsyslog (DBG " execute %s!", command_.c_str());
    if ((SystemExec(command_.c_str())) != 0)
    {
#ifdef DEBUG_SETUP
        dsyslog(DBG "  %s command returns error!", command_.c_str());
#endif
        comState_ = eCSCommandError;
    }
    else
    {
#ifdef DEBUG_SETUP
        dsyslog(DBG "  %s command is executed!", command_.c_str());
#endif
        comState_ = eCSCommandSuccess;
    }
}

// ---- Class TimerList  ------------------------------------------

// Timer Save and Restore Methods
void TimerList::SaveTimer()
{
    //cout << " ++  TimerList::SaveTimer() ++ " << endl;
    myTimers.Clear();
    for (cTimer * t = Timers.First(); t; t = Timers.Next(t))
    {
        myTimers.Add(new TimerString(t->ToText(true)));
    }
    Timers.cList < cTimer >::Clear();
    //cout << " ++  TimerList::SaveTimer()  End ++ " << endl;
}

// restore Timers, delete Timers with unknown channel
void TimerList::RestoreTimer()
{
    //cout << " ++  TimerList::RestoreTimer()  Restore ++ " << endl;
    Timers.cList < cTimer >::Clear();

    for (TimerString * timer = myTimers.First(); timer;
         timer = myTimers.Next(timer))
    {
        cString t = timer->GetTimerString();
        cTimer *tim = new cTimer();
        tim->Parse(t);
        if (tim->Channel() != NULL)
            Timers.Add(tim);
    }
    myTimers.Clear();
    //cout << " ++  TimerList::RestoreTimer()   END ++ " << endl;
}
