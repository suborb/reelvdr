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
 * avahithread.c
 *
 ***************************************************************************/

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <string>
#include <sstream>
#include <avahi-common/domain.h>
#include <dbus/dbus.h>

#include <vdr/config.h>
#include <vdr/timers.h>
#include <vdr/plugin.h>

#include "avahithread.h"
#include "avahilog.h"

std::vector<struct ReelBox_t> cAvahiThread::ReelBoxes;
std::vector<struct ReelBox_t> cAvahiThread::Databases;
int cAvahiThread::localLastEventID = -1;
eReelboxMode cAvahiThread::ReelboxMode_ = eModeStandalone;
AvahiSimplePoll* cAvahiThread::simple_poll = NULL;
AvahiStringList *cAvahiThread::browsed_types = NULL;
cMutex cAvahiThread::mutex;

cAvahiThread::cAvahiThread()
{
}

cAvahiThread::~cAvahiThread()
{
    Cancel();
}

void cAvahiThread::Action()
{
    /* lock dbus data structures as other
     * plugins may also use them
     * See dbus documentation
     * http://dbus.freedesktop.org/doc/api/html/group__DBusThreads.html#ga33b6cf3b8f1e41bad5508f84758818a7
     *
     * avahi uses dbus internally...
     */
    dbus_threads_init_default();

    AvahiClient *client = NULL;
    AvahiServiceTypeBrowser *sb = NULL;
    int error;

    /* Allocate main loop object */
    if (!(simple_poll = avahi_simple_poll_new()))
    {
        fprintf(stderr, "Failed to create simple poll object.\n");
        goto fail;
    }

    /* Allocate a new client */
    client = (AvahiClient*)avahi_client_new(avahi_simple_poll_get(simple_poll), (AvahiClientFlags)0, client_callback, NULL, &error);

    /* Check wether creating the client object succeeded */
    if (!client)
    {
        fprintf(stderr, "Failed to create client: %s\n", avahi_strerror(error));
        goto fail;
    }

    /* Create the service browser */
    if (!(sb = avahi_service_type_browser_new(client, AVAHI_IF_UNSPEC/*all interfaces*/, AVAHI_PROTO_INET/*only ipv4*/, NULL, (AvahiLookupFlags)0, service_type_browser_callback, client)))
    {
        fprintf(stderr, "Failed to create service browser: %s\n", avahi_strerror(avahi_client_errno(client)));
        goto fail;
    }

    /* Run the main loop */
    avahi_simple_poll_loop(simple_poll);

fail:
    /* Cleanup things */
    if (sb)
        avahi_service_type_browser_free(sb);

    if (client)
        avahi_client_free(client);

    if (simple_poll)
        avahi_simple_poll_free(simple_poll);
    simple_poll = NULL;

    avahi_string_list_free(browsed_types);
    browsed_types = NULL;
}

void cAvahiThread::Cancel(int WaitSeconds)
{
    if(simple_poll)
        avahi_simple_poll_quit(simple_poll);
    cThread::Cancel(WaitSeconds);
}

void cAvahiThread::browse_service_type(AvahiClient *client, const char *stype, const char *domain)
{
#ifdef AVAHI_DEBUG
    printf("\033[0;41m %s(%i): %s \033[0m\n", __FILE__, __LINE__, __PRETTY_FUNCTION__);
#endif
    AvahiServiceBrowser *b;
    AvahiStringList *i;

    assert(client);
    assert(stype);

    for (i = browsed_types; i; i = i->next)
        if (avahi_domain_equal(stype, (char*) i->text))
            return;

    if (!(b = avahi_service_browser_new(client, AVAHI_IF_UNSPEC, AVAHI_PROTO_INET/*only ipv4*/, stype, domain, (AvahiLookupFlags)0, service_browser_callback, client)))
    {
#ifdef AVAHI_DEBUG
        printf("avahi_service_browser_new() failed: %s\n", avahi_strerror(avahi_client_errno(client)));
#endif
        char buffer[128];
        snprintf(buffer, 128, "avahi_service_browser_new() failed: %s", avahi_strerror(avahi_client_errno(client)));
        cAvahiLog::Log(buffer, __FILE__, __LINE__);
        avahi_simple_poll_quit(simple_poll);
    }

    browsed_types = avahi_string_list_add(browsed_types, stype);

//    n_all_for_now++;
//    n_cache_exhausted++;
}

void cAvahiThread::client_callback(AvahiClient *client, AvahiClientState state, AVAHI_GCC_UNUSED void * userdata)
{
#ifdef AVAHI_DEBUG
    printf("\033[0;41m %s(%i): %s \033[0m\n", __FILE__, __LINE__, __PRETTY_FUNCTION__);
#endif
	assert(client);

	/* Called whenever the client or server state changes */

	if (state == AVAHI_CLIENT_FAILURE)
	{
		fprintf(stderr, "Server connection failure: %s\n", avahi_strerror(avahi_client_errno(client)));
        if(avahi_client_errno(client) == AVAHI_ERR_DISCONNECTED)
        {
            int error;
            avahi_client_free(client);
            client = NULL;

            avahi_string_list_free(browsed_types);
            browsed_types = NULL;

            if(!(client = (AvahiClient*)avahi_client_new(avahi_simple_poll_get(simple_poll), AVAHI_CLIENT_NO_FAIL, client_callback, NULL, &error)))
            {
                fprintf(stderr, "Failed to create client object: %s\n", avahi_strerror(error));
                avahi_simple_poll_quit(simple_poll);
            }
        }
        else
        {
            fprintf(stderr, "Client failure, exiting: %s\n", avahi_strerror(avahi_client_errno(client)));
            avahi_simple_poll_quit(simple_poll);
        }
	}
}

void cAvahiThread::service_browser_callback(AvahiServiceBrowser *b, AvahiIfIndex interface, AvahiProtocol protocol, AvahiBrowserEvent event, const char *name, const char *type, const char *domain, AVAHI_GCC_UNUSED AvahiLookupResultFlags flags, void* userdata)
{
#ifdef AVAHI_DEBUG
    printf("\033[0;41m %s(%i): %s \033[0m\n", __FILE__, __LINE__, __PRETTY_FUNCTION__);
#endif

	AvahiClient *c = (AvahiClient*)userdata;
	assert(b);
	/* Called whenever a new services becomes available on the LAN or is removed from the LAN */
	switch (event)
	{
		case AVAHI_BROWSER_FAILURE:
			//fprintf(stderr, "(Browser) %s\n", avahi_strerror(avahi_client_errno(avahi_service_browser_get_client(b))));
			avahi_simple_poll_quit(simple_poll);
			return;

		case AVAHI_BROWSER_NEW:
			/* We ignore the returned resolver object. In the callback
			   function we free it. If the server is terminated before
			   the callback function is called the server will free
			   the resolver for us. */

#ifdef AVAHI_DEBUG
            printf("\033[0;92m %s(%i): NEW: service='%s' type='%s' domain='%s'\033[0m\n", __FILE__, __LINE__, name, type, domain);
#endif
            char buffer[128];
            snprintf(buffer, 128, "NEW: service='%s' type='%s' domain='%s'", name, type, domain);
            cAvahiLog::Log(buffer, __FILE__, __LINE__);

			if (!(avahi_service_resolver_new(c, interface, protocol, name, type, domain, AVAHI_PROTO_INET/*only ipv4*/ , (AvahiLookupFlags)0, resolve_callback, c)))
				fprintf(stderr, "Failed to resolve service '%s': %s\n", name, avahi_strerror(avahi_client_errno(c)));
			break;

		case AVAHI_BROWSER_REMOVE:
            {
#ifdef AVAHI_DEBUG
            printf("\033[0;91m %s(%i): service REMOVE: service='%s' type='%s' domain='%s'\033[0m\n", __FILE__, __LINE__, name, type, domain);
#endif
            char buffer[128];
            snprintf(buffer, 128, "service REMOVE: service='%s' type='%s' domain='%s'", name, type, domain);
            cAvahiLog::Log(buffer, __FILE__, __LINE__);
            cMutexLock MutexLock(&mutex);
            if(!strcmp(type, AVAHI_RESOLVER_AVG_TYPE))
                RemoveHost(name, &ReelBoxes);
            else if(!strcmp(type, AVAHI_RESOLVER_MYSQL_TYPE))
            {
                RemoveHostCheckMysql(name);
                RemoveHost(name, &Databases);
            }
			break;
            }
		case AVAHI_BROWSER_ALL_FOR_NOW:
		case AVAHI_BROWSER_CACHE_EXHAUSTED:
			//fprintf(stderr, "(Browser) %s\n", event == AVAHI_BROWSER_CACHE_EXHAUSTED ? "CACHE_EXHAUSTED" : "ALL_FOR_NOW");
			break;
	}
}

void cAvahiThread::service_type_browser_callback(AvahiServiceTypeBrowser *b, AvahiIfIndex interface, AvahiProtocol protocol, AvahiBrowserEvent event, const char *type, const char *domain, AVAHI_GCC_UNUSED AvahiLookupResultFlags flags, void* userdata)
{
#ifdef AVAHI_DEBUG
    printf("\033[0;41m %s(%i): %s \033[0m\n", __FILE__, __LINE__, __PRETTY_FUNCTION__);
#endif
	AvahiClient *client = (AvahiClient*)userdata;
	assert(b);

	/* Called whenever a new services becomes available on the LAN or is removed from the LAN */

	switch (event)
	{
		case AVAHI_BROWSER_FAILURE:
			//fprintf(stderr, "(Browser) %s\n", avahi_strerror(avahi_client_errno(avahi_service_browser_get_client(b))));
			avahi_simple_poll_quit(simple_poll);
			return;

		case AVAHI_BROWSER_NEW:
#ifdef AVAHI_DEBUG
            printf("\033[0;92m %s(%i): type=\"%s\"\033[0m\n", __FILE__, __LINE__, type);
#endif
            {
                char buffer[128];
                snprintf(buffer, 128, "type=\"%s\"", type);
                cAvahiLog::Log(buffer, __FILE__, __LINE__);
            }
            browse_service_type(client, type, domain);
			break;

		case AVAHI_BROWSER_REMOVE:
#ifdef AVAHI_DEBUG
            printf("\033[0;91m %s(%i): service type REMOVE: type='%s' domain='%s'\033[0m\n", __FILE__, __LINE__, type, domain);
#endif
            {
                char buffer[128];
                snprintf(buffer, 128, "service type REMOVE: type='%s' domain='%s'", type, domain);
                cAvahiLog::Log(buffer, __FILE__, __LINE__);
            }
			break;

		case AVAHI_BROWSER_ALL_FOR_NOW:
		case AVAHI_BROWSER_CACHE_EXHAUSTED:
			//fprintf(stderr, "(Browser) %s\n", event == AVAHI_BROWSER_CACHE_EXHAUSTED ? "CACHE_EXHAUSTED" : "ALL_FOR_NOW");
			break;
	}
}

void cAvahiThread::resolve_callback(AvahiServiceResolver *resolver, AVAHI_GCC_UNUSED AvahiIfIndex interface, AVAHI_GCC_UNUSED AvahiProtocol protocol, AvahiResolverEvent event, const char *name, const char *type, const char *domain, const char *host_name, const AvahiAddress *address, uint16_t port, AvahiStringList *txt, AvahiLookupResultFlags flags, AVAHI_GCC_UNUSED void* userdata)
{
#ifdef AVAHI_DEBUG
    printf("\033[0;41m %s(%i): %s \033[0m\n", __FILE__, __LINE__, __PRETTY_FUNCTION__);
#endif
    assert(resolver);
    bool free_resolver=true;

    /* Called whenever a service has been resolved successfully or timed out */

    switch (event)
    {
        case AVAHI_RESOLVER_FAILURE:
            fprintf(stderr, "(Resolver) Failed to resolve service '%s' of type '%s' in domain '%s': %s\n", name, type, domain, avahi_strerror(avahi_client_errno(avahi_service_resolver_get_client(resolver))));
            break;

        case AVAHI_RESOLVER_FOUND:
#ifdef AVAHI_DEBUG
            printf("\033[0;93m %s(%i): RESOLVER_FOUND - name=\"%s\" type=%s port=%i \033[0m\n", __FILE__, __LINE__, name, type, port);
#endif
            char buffer[128];
            snprintf(buffer, 128, "RESOLVER_FOUND - name=\"%s\" type=%s port=%i", name, type, port);
            cAvahiLog::Log(buffer, __FILE__, __LINE__);
            {
                char avahi_address[AVAHI_ADDRESS_STR_MAX], *text;

                avahi_address_snprint(avahi_address, sizeof(avahi_address), address);
                text = avahi_string_list_to_string(txt);

                cMutexLock MutexLock(&mutex);
                if(!strcmp(type, AVAHI_RESOLVER_AVG_TYPE))
                    ReelBoxes.push_back(ReelBox_t(name, avahi_address, GetMac(avahi_address), port));
                else if(!strcmp(type, AVAHI_RESOLVER_MYSQL_TYPE))
                {
                    bool NetserverFound = false;
                    std::string Mac = GetMac(avahi_address);
                    AddHost(ReelBox_t(name, avahi_address, Mac, port), &Databases);

                    if((Setup.NetServerIP) && (strlen(Setup.NetServerIP)))
                    {
                        if(!strcmp(Setup.NetServerIP, avahi_address))
                        {
#ifdef AVAHI_DEBUG
                            printf("%s(%i): avahi_address=\"%s\" name=\"%s\" port=%i text=%s\n", __FILE__, __LINE__, avahi_address, name, port, text);
#endif
                            char buffer[128];
                            snprintf(buffer, 128, "avahi_address=\"%s\" name=\"%s\" port=%i text=%s", avahi_address, name, port, text);
                            cAvahiLog::Log(buffer, __FILE__, __LINE__);
                            NetserverFound = true;
                        }
                    }
                    else
                    {
                        if(!strcmp(Setup.NetServerIP, avahi_address))
                            NetserverFound = true;
                    }

                    if(NetserverFound)
                    {
                        isyslog("MultiRoom server Found IP: '%s' '%s'", avahi_address, name);
#ifdef AVAHI_DEBUG
                        printf("%s(%i): avahi_address=\"%s\" name=\"%s\" port=%i text=%s\n", __FILE__, __LINE__, avahi_address, name, port, text);
#endif
                        char buffer[128];
                        snprintf(buffer, 128, "avahi_address=\"%s\" name=\"%s\" port=%i text=%s", avahi_address, name, port, text);
                        cAvahiLog::Log(buffer, __FILE__, __LINE__);
                        if(strstr(text, AVAHI_RESOLVER_MYSQL_TEXT))
                        {
                            char *offset = text+strlen(AVAHI_RESOLVER_MYSQL_TEXT)+1; // +1 because of first "
                            *(strchr(offset, '"')) = '\0'; // cut of the second "
                            int remoteLastEventID = atoi(offset);
                            if(remoteLastEventID != localLastEventID)
                            {
#ifdef AVAHI_DEBUG
                                printf("\033[0;41m %s(%i): localLastEventID = %i remoteLastEventID = %i \033[0m\n", __FILE__, __LINE__, localLastEventID, remoteLastEventID);
#endif
                                char buffer[128];
                                snprintf(buffer, 128, "localLastEventID = %i remoteLastEventID = %i", localLastEventID, remoteLastEventID);
                                cAvahiLog::Log(buffer, __FILE__, __LINE__);
#ifdef USEMYSQL
                                if(Setup.ReelboxMode == eModeClient)
                                {
                                    if((localLastEventID==-1) || (remoteLastEventID==0)) // -1 means Database wasn't "running"
                                        LoadDB();
                                    else
                                        SyncDB();
                                }
#endif
                                localLastEventID = remoteLastEventID;
                                free_resolver=false; // Don't free resolver since we want to listen to this service
                            }
                        }
                        
                        // mount the setup shares 0 and setup media directory
                        // should already be handled by automounter; TODO investigate why this doesn't happen
                        if(Setup.ReelboxMode == eModeClient)
                        {
                            printf("\033[7;95m (re)mount setup shares\033[0m");
                            isyslog("ClientMode: NetServer found. Remounting network shares and setting mediadir");
                            SystemExec("setup-shares start 0; setup-mediadir restart");
                        }
                    } // NetserverFound

                }
                avahi_free(text);
            }
            break;
    }
    if(free_resolver)
        avahi_service_resolver_free(resolver);
}

void cAvahiThread::AddHost(ReelBox_t ReelBox, std::vector<ReelBox_t> *Hosts)
{
#ifdef AVAHI_DEBUG
    printf("\033[0;104m %s(%i): %s(%s) - Mac: %s Port: %i \033[0m\n", __FILE__, __LINE__, ReelBox.Name.c_str(), ReelBox.Ip.c_str(), ReelBox.Mac.c_str(), ReelBox.Port);
#endif
    char buffer[128];
    snprintf(buffer, 128, "%s(%s) - Mac: %s Port: %i", ReelBox.Name.c_str(), ReelBox.Ip.c_str(), ReelBox.Mac.c_str(), ReelBox.Port);
    cAvahiLog::Log(buffer, __FILE__, __LINE__);
    bool found = false;
    std::vector<ReelBox_t>::iterator RB_Iter = Hosts->begin();
    while(!found && (RB_Iter != Hosts->end())) {
        if (RB_Iter->Port == ReelBox.Port && RB_Iter->Name.compare(ReelBox.Name) == 0 && RB_Iter->Ip.compare(ReelBox.Ip) == 0)
            found = true;
        ++RB_Iter;
    }
    if(!found)
        Hosts->push_back(ReelBox);
}

void cAvahiThread::RemoveHost(const char *name, std::vector<ReelBox_t> *Hosts)
{
    bool found = false;
    std::vector<ReelBox_t>::iterator RB_Iter = Hosts->begin();
    while(!found && (RB_Iter != Hosts->end()))
    {
        if(!strcmp(RB_Iter->Name.c_str(), name))
        {
            Hosts->erase(RB_Iter);
            found = true;
        }
        ++RB_Iter;
    }
}

void cAvahiThread::RemoveHostCheckMysql(const char *name)
{
    // A Host is being removed, check if this was a host with MySQL
    bool found = false;
    std::vector<ReelBox_t>::iterator RB_Iter = Databases.begin();
    while(!found && (RB_Iter != Databases.end()))
    {
        if(!strcmp(RB_Iter->Name.c_str(), name))
        {
            if(!strcmp(RB_Iter->Ip.c_str(), Setup.NetServerIP))
            {
//                Setup.ReelboxModeTemp = eModeStandalone;
                ReelboxMode_ = eModeStandalone;
                localLastEventID = -1;
            }
            found = true;
        }
        ++RB_Iter;
    }
}

#ifdef USEMYSQL
extern int ForceReloadDB;
extern int DBCounter;

void cAvahiThread::LoadDB()
{
#ifdef AVAHI_DEBUG
    printf("\033[0;41m %s(%i): %s \033[0m\n", __FILE__, __LINE__, __PRETTY_FUNCTION__);
#endif
    cAvahiLog::Log(__PRETTY_FUNCTION__, __FILE__, __LINE__);
//    Setup.ReelboxModeTemp = Setup.ReelboxMode;
    ReelboxMode_ = Setup.ReelboxMode;
    ForceReloadDB++;
}

void cAvahiThread::SyncDB()
{
#ifdef AVAHI_DEBUG
    printf("\033[0;41m %s(%i): %s \033[0m\n", __FILE__, __LINE__, __PRETTY_FUNCTION__);
#endif
    cAvahiLog::Log(__PRETTY_FUNCTION__, __FILE__, __LINE__);
//    Setup.ReelboxModeTemp = Setup.ReelboxMode;
    ReelboxMode_ = Setup.ReelboxMode;
    DBCounter++;
}
#endif

bool cAvahiThread::CheckNetServer()
{
    // This function checks if NetServer is available and its MultiRoom is enabled
    cMutexLock MutexLock(&mutex);
    std::vector<ReelBox_t>::iterator RB_Iter = Databases.begin();
    while(RB_Iter != Databases.end())
    {
        if(!strcmp(RB_Iter->Ip.c_str(), Setup.NetServerIP))
            return true;
        ++RB_Iter;
    }
    return false;
}

void cAvahiThread::Restart()
{
    if(simple_poll)
    {
        avahi_simple_poll_quit(simple_poll);
        simple_poll = NULL;
    }
    if(browsed_types)
    {
        avahi_string_list_free(browsed_types);
        browsed_types = NULL;
    }

    // Wait till thread is stopped
    unsigned int i=20;
    while(Active() && --i)
        cCondWait::SleepMs(250);

    // Clear Vectors
    {
        cMutexLock MutexLock(&mutex);
        ReelBoxes.clear();
        Databases.clear();
    }

    if(Active())
    {
        Skins.QueueMessage(mtError, tr("Couldn't restart thread!"));
        esyslog("Avahi-pl: Couldn't restart thread!");
    }
    else
        Start();
    // Wait because avahi takes some time till network is scanned
    cCondWait::SleepMs(2000);
}

std::string cAvahiThread::GetMac(const char *ip)
{
    std::stringstream command;
    char *buffer;
    char *index = NULL;
    FILE *process;

    // first ping, since arping is not available on NetClient now
    command << "ping -c 1 -q " << ip << " > /dev/null";
    SystemExec(command.str().c_str());

    command.str("");
    command << "LANG=C arp -an " << ip << " 2> /dev/null";
    process = popen(command.str().c_str(), "r");
    if(process)
    {
        cReadLine readline;
        buffer = readline.Read(process);
        if(buffer)
        {
            index = strchr(buffer, ':');
            if(index && (strlen(index) >= 14) && (*(index+3)==':') && (*(index+6)==':') && (*(index+9)==':') && (*(index+12)==':'))
            {
                *(index+15) = '\0';
                index -= 2;
            }
            else
                index = NULL;
        }
        pclose(process);
    }
    else
        return "";

    if(index)
        return std::string(index);
    else
        return "";
}
