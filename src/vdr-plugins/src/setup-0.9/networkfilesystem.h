/***************************************************************************
 *   Copyright (C) 2008 by Reel Multimedia;  Author:  Florian Erfurth      *
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
 * networkfilesystem.h
 *
 ***************************************************************************/

#ifndef NETWORKFILESYSTEM_H
#define NETWORKFILESYSTEM_H

#include <vector>

#define DISPLAYNAME_LENGTH 64
#define USERNAME_LENGTH 64
#define PASSWORD_LENGTH 64
#define MOUNTPOINT_LENGTH 256

class cNetworkFilesystem
{
        friend class cShareSmbSettingsMenu;
        friend class cShareNfsSettingsMenu;
        friend class cShareAfpSettingsMenu;
    protected:
        char *Host_;
        char *Sharename_;

    public:
        cNetworkFilesystem(const char *Host, const char *Sharename);
        virtual ~cNetworkFilesystem();

        int Number_;
        char *Displayname_;
        int Pictures_; // 0=no; 1=yes
        int Music_; // 0=no; 1=yes
        int Recordings_; // 0=no; 1=yes
        int Videos_; // 0=no; 1=yes
        bool StartClient_;

        const char* GetHost() const { return Host_; };
        const char* GetSharename() const { return Sharename_; };
        void GetFreeNumber();
        int GetNumber() const { return Number_; };
        bool MountActive();
        void SetRecordingMedia();
};

class cNetworkFSSMB : public cNetworkFilesystem
{
    public:
        cNetworkFSSMB(const char *Host, const char *Sharename);
        cNetworkFSSMB(const cNetworkFSSMB &NetworkFSSMB); // copy-constructor
        ~cNetworkFSSMB();

        char *Username_;
        char *Password_;
};

class cNetworkFSAFP : public cNetworkFilesystem
{
    public:
        cNetworkFSAFP(const char *Host, const char *Sharename);
        cNetworkFSAFP(const cNetworkFSAFP &NetworkFSAFP); // copy-constructor
        ~cNetworkFSAFP();

        char *Username_;
        char *Password_;
};

class cNetworkFSNFS : public cNetworkFilesystem
{
    public:
        cNetworkFSNFS(const char *Host, const char *Sharename);
        cNetworkFSNFS(const cNetworkFSNFS &NetworkFSNFS); // copy-constructor
        ~cNetworkFSNFS();

        int NetworkProtocol_;
        int BlockSize_;
        int OptionHS_; // 0=soft; 1=hard
        int OptionLock_; // 0=no; 1=yes
        int OptionRW_; // 0=no; 1=yes
        int NFSVersion_;
};

class cNetworkFSHost
{
    protected:
        char *Host_;

    private:
        std::vector<cNetworkFSSMB*> vNetworkFSSMB_;
        std::vector<cNetworkFSNFS*> vNetworkFSNFS_;

    public:
        cNetworkFSHost(const char *Host);
        virtual ~cNetworkFSHost();

        bool ShowShares_;

        const char* GetHost() { return Host_; };
        unsigned int CountShareSMB() { return vNetworkFSSMB_.size(); };
        void AddShareSMB(const char *Sharename);
        cNetworkFSSMB* GetShareSMB(int i) { return vNetworkFSSMB_.at(i); };
        unsigned int CountShareNFS() { return vNetworkFSNFS_.size(); };
        void AddShareNFS(const char *Sharename);
        cNetworkFSNFS* GetShareNFS(int i) { return vNetworkFSNFS_.at(i); };
};

class cNetworkFSMacHost : public cNetworkFSHost
{
    private:
        std::vector<cNetworkFSAFP*> vNetworkFSAFP_;

    public:
        cNetworkFSMacHost(const char *Host);
        ~cNetworkFSMacHost();

        char *Username_;
        char *Password_;

        unsigned int CountShareAFP() { return vNetworkFSAFP_.size(); };
        void AddShareAFP(const char *Sharename);
        cNetworkFSAFP* GetShareAFP(int i) { return vNetworkFSAFP_.at(i); };
};

class cNetworkFSVistaHost : public cNetworkFSHost
{
    private:
        std::vector<cNetworkFSSMB*> vNetworkFSSMB_;

    public:
        cNetworkFSVistaHost(const char *Host);
        ~cNetworkFSVistaHost();

        char *Username_;
        char *Password_;
};

#endif

