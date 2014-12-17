#ifndef CHECKSERVER_H
#define CHECKSERVER_H

#include <vdr/thread.h>
#include <string>

class cCheckServer : public cThread
{
    std::string server;
    bool online;
public:
    cCheckServer(std::string Server);
    ~cCheckServer();

    void Action();

    void Stop();
    bool IsOnline() const;
};

extern cCheckServer checkServer;

#endif // CHECKSERVER_H
