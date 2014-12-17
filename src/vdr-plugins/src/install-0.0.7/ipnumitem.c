/****************************************************************************
 * DESCRIPTION: 
 *             Creates VDR Menus
 *
 * $Id: setupmenu.cpp,v 1.16 2005/10/03 14:05:20 ralf Exp $
 *
 * Contact:    ranga@teddycats.de        
 *
 * Copyright (C) 2004 by Ralf Dotzert 
 ****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <sys/utsname.h>
#include <netinet/in.h>
#include <net/if.h>
#include <netdb.h>
#include <unistd.h>

#include <vdr/menuitems.h>
#include <vdr/config.h>
#include <vdr/menu.h>
#include <string>
#include <vector>

#include "util.h"
#include "ipnumitem.h"

using std::vector;
using std::string;

#define DBG_PREFIX "DEBUG: install"
#define VALUEIPMAXLEN   16

// --- Class cMenuEditIpNumItem ------------------------------------------------------

cMenuEditIpNumItem::cMenuEditIpNumItem(const char *Name, char *Value):cMenuEditItem(Name)
{
    //value = strdup(mEntry.GetValueIp());
    value = Value;
    dsyslog(DBG_PREFIX " >>>> Ip-Adress  %s ", value);
    val_type = Util::IP;
    pos = 0;
    digit = 0;
    Init();                     // split ipAdress into segments
    Set();
}

cMenuEditIpNumItem::~cMenuEditIpNumItem()
{
    pos = 0;
    digit = 0;
}

void cMenuEditIpNumItem::Set(void)
{

    // pos 0 indicates edit mode
    char cursor[6];
    sprintf(cursor, "[%3d]", val[pos]);
    char *buffer = (char *)"";
#if 0
    dsyslog(DBG_PREFIX " +++ cursor %s in feld %d ", cursor, pos);
    dsyslog(DBG_PREFIX " +++ segments  %d.%d.%d.%d\n", val[1], val[2], val[3], val[4]);
    dsyslog(DBG_PREFIX " +++ build String! +++ buf %s", buffer);
#endif

    //free(value);
    snprintf(value, VALUEIPMAXLEN, "%d.%d.%d.%d", val[1], val[2], val[3], val[4]);

    if (pos != 0)
    {
        for (int i = 1; i < 5; i++)
        {
            if (pos == i)
                asprintf(&buffer, "%s%s%s", buffer, i == 1 ? "" : ".", cursor);
            else
            {
                asprintf(&buffer, "%s%s%3d", buffer, i == 1 ? "" : ".", val[i]);
                //dsyslog(DBG_PREFIX " +++ -------------build buf :::: %s ", buffer);
            }
        }

        //dsyslog(DBG_PREFIX " +++ SetValue(buffer) : %s",  buffer);
        SetValue(buffer);
        free(buffer);
    }
    else
    {
        SetValue(value);
        //dsyslog(DBG_PREFIX " >>>>>>>> +++ mEntry.SetValue(value) : %s", value);
        //mEntry.SetValue(val_type, value);
    }
    //dsyslog(DBG_PREFIX " +++ SetValue(string)  value %s", value);       
}

unsigned char cMenuEditIpNumItem::Validate(char *Val)
{
    // dsyslog(DBG_PREFIX "Val %s ValAtoi %d",Val, atoi(Val));
    if (atoi(Val) <= 255)
        return atoi(Val);
    else
    {
        Val[2] = '\0';
        //dsyslog(DBG_PREFIX "Val ret \"\\0 ");
        return atoi(Val);
    }
    //dsyslog(DBG_PREFIX "Val ret \"1\" ");
    return 1;
}

// this methode extracts the string into the u_int fields 
void cMenuEditIpNumItem::Init()
{
    // XXX  dirty hack !!! 
    //dsyslog(DBG_PREFIX  " >>>> ++ INIT Ip-Adress  %s; AdripAdr %p", value, &value);
    pos = 0;
    char *s, *tmp = NULL, *tmp_cp;
    int i;
    if (strchr(value, '.'))
        tmp = strdup(value);
    else
        tmp = strdup("0.0.0.0");
    tmp_cp = tmp;

    val[0] = 0;
    for (i = 1; i < 5; i++)
    {
        if (i == 4)
        {
            val[i] = atoi(tmp);
            continue;
        }
        s = strchr(tmp, '.');
        *s = '\0';
        val[i] = atoi(tmp);
        *s = ' ';
        s++;
        tmp = s;
    }
    free(tmp_cp);
}

eOSState cMenuEditIpNumItem::ProcessKey(eKeys Key)
{
    eOSState state = cMenuEditItem::ProcessKey(Key);

    Key = NORMALKEY(Key);

    if (state == osUnknown)
    {
        // hey markus denk daran, felder von 1 bis 3 stellig
        // das in Set mit if digits
        // mit kLeft ins nächste Feld springen  10 -> . x -> x  
        switch (NORMALKEY(Key))
        {
        case kOk:
        case kBack:
            if (pos != 0)
                pos = 0;
            else
                return cMenuEditItem::ProcessKey(Key);
            break;

        case kLeft:
            {
                if (pos > 1)
                    pos--;
                if (pos == 0)
                    pos = 4;
                digit = 0;
            }
            break;
        case kRight:
            {
                if (pos < 4)
                    pos++;
                digit = 0;
            }
            break;
        case kUp:
            if (pos != 0)
            {
                val[pos]++;
                digit = 0;
            }
            else
                return cMenuEditItem::ProcessKey(Key);
            break;
        case kDown:
            if (pos != 0)
            {
                val[pos]--;
                digit = 0;
            }
            else
                return cMenuEditItem::ProcessKey(Key);
            break;
        case k0...k9:
            {
                //printf("k0 ... k9");
                if (pos != 0)
                {
                    if (digit > 2)
                    {
                        digit = 0;
                        if (pos < 4)
                            pos++;
                    }
                    if (digit == 0)
                        val[pos] = 0;

                    char tmp[4];
                    sprintf(tmp, "%d", val[pos]);
                    //int len = strlen(tmp);
                    tmp[digit] = Key - k0 + '0';
                    digit++;
                    val[pos] = Validate(tmp);

#if 0
                    dsyslog(DBG_PREFIX "key: k%d, val[pos]:%d, digit: %d pos %d, len:%d \n", Key - k0, val[pos], digit, pos, len);
                    dsyslog(DBG_PREFIX ">>> tmp %s \n", tmp);
                    printf("key: k%d, val[pos]:%d, digit: %d pos %d, len:%d \n", Key - k0, val[pos], digit, pos, len);

                    printf(">>> tmp %s \n", tmp);
#endif
                    Set();
                }
            }
            break;

        default:
            return state;
        }
        Set();
        state = osContinue;
    }
    return state;

}
