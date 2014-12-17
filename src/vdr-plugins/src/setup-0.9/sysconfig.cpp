/****************************************************************************
 * DESCRIPTION:
 *             Handles sysconfig File
 *
 * $Id: sysconfig.cpp,v 1.4 2005/10/03 14:05:20 ralf Exp $
 *
 * Contact:    ranga@teddycats.de
 *
 * Copyright (C) 2004 by Ralf Dotzert
 ****************************************************************************/

#include <map>
#include <string>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "vdr/debug.h"
#include "vdr/plugin.h"

#include <vdr/sysconfig_vdr.h>
#include "util.h"


using std::pair;
using std::map;
using std::string;
using std::vector;

#define MAXLENGTH 256

// ----- class cSysConfig ------------------------------------------------------


cSysConfig::~cSysConfig()
{
    for (mapConstIter_t iter = sysMap_.begin(); iter != sysMap_.end(); ++iter)
    {
        delete[]iter->second;
    }
}


bool cSysConfig::Load(const char *fname)
{
    int count = 0;
    const char *line;
    fileName_ = Util::Strdupnew(fname);
    int fp = open(fname, O_RDONLY);
    if (fp != -1)
    {
        while ((line = ReadLine(fp)) != NULL)
        {
            AddLine(line);
            delete[]line;
            count++;
        }
        close(fp);
    }
    else
    {
        //printf(DBG  "   .............. cSysConfig -- LoadFile  %s Failed !!! \n", fname);
        DLOG("can`t open: %s, errno=%d\n", fileName_.c_str(), errno);
        sysMap_.clear();
    }
    return true;
}


/**
 * Save cSysConfig file
 * @return true on success
 */
bool cSysConfig::Save()
{
    FILE *fp = fopen(fileName_.c_str(), "w");
    if (!fp)
    {
        DLOG("Could not write file: %s, errno=%d\n",
             fileName_.c_str(), errno);
        return false;
    }

    fprintf(fp, "#\n"
            "# Generated by Setup-Plugin, \n"
            "# (c) 2005 by Ralf Dotzert and MiniVDR.de\n"
            "# (c) 2006-2010 Reel Multimedia (www.reel-multimedia.com)\n" 
            "#\n\n");

    for (mapConstIter_t iter = sysMap_.begin(); iter != sysMap_.end(); ++iter)
    {
        fprintf(fp, "%s=\"%s\"\n", iter->first.c_str(), iter->second);
    }
    fclose(fp);

    CopyToTftpRoot(fileName_.c_str());

    return true;
}

/**
 * read one line from opened file
 * @param fp opened filepointer
 * @return null if EOF or allocated character String holding one line
 */
const char *cSysConfig::ReadLine(int fd)
{
    char c;
    char buf[1024];
    int i = 0;
    int maxLen = static_cast < int >(sizeof(buf)) - 1;
    char *line = NULL;
    while (read(fd, &c, 1))
    {
        if (c == '\n' || i == maxLen)
        {
            line = new char[i + 1];
            strncpy(line, buf, i);
            line[i] = '\0';
            //dsyslog (DBG " cSysConfig readline %s ",line);
            return line;
        }
        else
            buf[i++] = c;
    }
    return line;
}

/**
 * Add the given line from sysconfig file an split it in Name an Variable
 * @param line allocated buffer holding one line
 */

void cSysConfig::AddLine(const char *line)
{
    //dsyslog(DBG  "   AddLine   %s", line);
    vector < char >v(strlen(line) + 1);
    char *l = &v[0];
    strcpy(l, line);
    char *key = NULL;
    char *val = new char[MAXLENGTH];    //XXX!

    if (strlen(l) > 0 && l[0] != '#')   // comment line
    {
        char *tmp = NULL;
        if ((key = strtok(l, "=")) != NULL && (tmp = strtok(NULL, "\"")) != NULL)
        {
            strncpy(val, tmp, MAXLENGTH - 1);   //XXX!
            //dsyslog(DBG " SysConf Insert key \"%s\"-> \"%s\" ",key, val);
            sysMap_.insert(map < string, char *>::value_type(key, val));
        }
    }
}
