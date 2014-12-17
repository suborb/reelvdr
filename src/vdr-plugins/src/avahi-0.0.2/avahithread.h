/***************************************************************************
 *   Copyright (C) 2009 by Reel Multimedia;  Author:  Florian Erfurth      *
 *                                                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                         *
 ***************************************************************************
 *
 * avahithread.h
 *
 ***************************************************************************/

#ifndef AVAHITHREAD_H
#define AVAHITHREAD_H

#include <avahi-client/client.h>
#include <avahi-client/lookup.h>

#include <avahi-common/simple-watch.h>
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>

#include <vector>

#include <vdr/thread.h>

#include "avahiservice.h"

class cAvahiThread : public cThread
{
    private:
        static cMutex mutex;
        static std::vector<struct ReelBox_t> ReelBoxes;
        static std::vector<struct ReelBox_t> Databases;
        static int localLastEventID;
        static AvahiSimplePoll *simple_poll;
        static AvahiStringList *browsed_types;
        static void browse_service_type(AvahiClient *client, const char *stype, const char *domain);
        static void client_callback(AvahiClient *c, AvahiClientState state, AVAHI_GCC_UNUSED void * userdata);
        static void service_browser_callback(AvahiServiceBrowser *b, AvahiIfIndex interface, AvahiProtocol protocol, AvahiBrowserEvent event, const char *name, const char *type, const char *domain, AVAHI_GCC_UNUSED AvahiLookupResultFlags flags, void* userdata);
        static void service_type_browser_callback(AvahiServiceTypeBrowser *b, AvahiIfIndex interface, AvahiProtocol protocol, AvahiBrowserEvent event, const char *type, const char *domain, AVAHI_GCC_UNUSED AvahiLookupResultFlags flags, void* userdata);
        static void resolve_callback(AvahiServiceResolver *resolver, AVAHI_GCC_UNUSED AvahiIfIndex interface, AVAHI_GCC_UNUSED AvahiProtocol protocol, AvahiResolverEvent event, const char *name, const char *type, const char *domain, const char *host_name, const AvahiAddress *address, uint16_t port, AvahiStringList *txt, AvahiLookupResultFlags flags, AVAHI_GCC_UNUSED void* userdata);
        static void AddHost(ReelBox_t ReelBox, std::vector<ReelBox_t> *Hosts);
        static void RemoveHost(const char *name, std::vector<ReelBox_t> *Hosts);
        static void RemoveHostCheckMysql(const char *name); // A Host is being removed, check if this was a host with MySQL
#ifdef USEMYSQL
        static void LoadDB();
        static void SyncDB();
#endif
        static std::string GetMac(const char *ip);

    protected:
        void Cancel(int WaitSeconds=0);

    public:
        cAvahiThread();
        ~cAvahiThread();

        static eReelboxMode ReelboxMode_;
        void Action();
        static void GetReelBoxList(std::vector<ReelBox_t> *reelboxes) { cMutexLock MutexLock(&mutex);  *reelboxes = ReelBoxes; };
        static void GetDatabaseList(std::vector<ReelBox_t> *databases) { cMutexLock MutexLock(&mutex); *databases = Databases; };
        static bool CheckNetServer();
        void Restart();
};

#endif
