/*
 * Remote Plugin for the Video Disk Recorder
 *
 * remotetcp.c: tcp/telnet remote control
 *
 * See the README file for copyright information and how to reach the author.
 */


#include "remote.h"
#include "remotetcp.h"
#include "ttystatus.h"


cTcpRemote::cTcpRemote(const char *name, int f, char *d)
    : cRemoteDevTty(name, f, d)
{
    cstatus = NULL;
    csock = NULL;
    Start();
}


cTcpRemote::~cTcpRemote()
{
    Cancel();

    if (cstatus)
        delete cstatus;

    if (fh != -1)
        close(fh);

    if (csock)
        delete csock;
}		  


uint64_t cTcpRemote::getKey(void)
{
    if (csock == NULL)
    {
        int port;
        sscanf (device, "tcp:%d", &port);
        csock = new cSocket(port);
        if (! csock)
        {
            esyslog("error creating socket");
            Cancel();
        }

        if (! csock->Open())
        {
            esyslog("error opening socket");
            Cancel();
        }
    }


    while (fh < 0)
    {
        usleep(100000);
        fh = csock->Accept();
        if (fh >= 0)
        {
            char str[80];
            // hack for telnet
            sprintf(str, "%c%c%c", 255,251,1); // WILL ECHO
            write(fh, str, strlen(str));
            sprintf(str, "%c%c%c", 255,251,3); // WILL SGA
            write(fh, str, strlen(str));

            sprintf(str, "VDR Remote Plugin - %s\n\r", device);
            write(fh, str, strlen(str));
            // skip telnet stuff
            read(fh, str, sizeof str);

            cstatus = new cTtyStatus(fh);
        }
    }


    uint64_t key = cRemoteDevTty::getKey();

    if (key == INVALID_KEY)
    {
        delete cstatus;
        cstatus = NULL;
        close(fh);
        fh = -1;
    }

    return key;
}
