#include "CheckServer.h"


//---------------------------------------------------------------------
//------------------------cCheckServer---------------------------------
//---------------------------------------------------------------------

cCheckServer::cCheckServer(std::string Server)
    : cThread("checkserver"),
    server(Server), online(false)
{
}

cCheckServer::~cCheckServer()
{
    Stop();
}

void cCheckServer::Stop()
{
    Cancel(1);
}

bool cCheckServer::IsOnline() const
{
    return online;
}

void cCheckServer::Action()
{
    int r = 0;
    int i;
    const std::string ping_cmd = "ping -c 1 -w 5 ";
    const std::string silent = " 2>&1 >/dev/null";
    std::string cmd;

    online = false;

    while (Running() && !server.empty())
    {
        // Check IP address first
        cmd = ping_cmd + "8.8.8.8" + silent; // google's DNS
        r = SystemExec(cmd.c_str());

        // couldn't reach IP address, no hope of reaching given 'server'
        // DNS lookup may take ~20s, so avoid pinging 'server'
        if (r != 0)
            online = false;
        else
        {
            // check Server
            cmd = ping_cmd + server + silent;
            r = SystemExec(cmd.c_str());
            online = (r == 0);
        }

        // sleep for a couple of seconds
        // but if thread was cancelled quit immediately
        for (i = 0; Running() && i<40; ++i)
            cCondWait::SleepMs(50);
    } // while
}

cCheckServer checkServer("download.reelbox.org");
