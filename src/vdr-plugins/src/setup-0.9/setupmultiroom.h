#ifndef MULTIROOMSETUPMENU_H
#define MULTIROOMSETUPMENU_H

#include <vdr/osdbase.h>
#include <vector>
#include <string>

#ifdef USEMYSQL
#include <vdr/thread.h>

#include "setupnetclientmenu.h"

#define E_DATABASE_NOT_INSTALLED           -1
#define E_DATABASE_VDR_NOT_EXISTENT        -2
#define E_DATABASE_NOT_PROPERLY_CONFIGURED -3
#define E_DATABASE_CONNECTION_FAILED       -4
#define E_DATABASE_LOGIN_FAILED            -5
#define E_DATABASE_TABLE_NOT_EXISTENT      -6
#define E_DATABASE_REELUSER_PERMISSION_MISSING -7

class cDatabaseChecker
{
    public:
        cDatabaseChecker();
        ~cDatabaseChecker();
        static int Check(bool DisplayMessage=false); /** returns 0 if OK, else returns error-number */
};

class cInstallMysqlThread : public cThread
{
    private:
        bool FullInstall_;
        void FullInstall(); // Install MySQL & transfer datas to MySQL
        void SmallInstall(); // Just install MySQL
        void TransferDataToMysqlTimers();
    public:
        cInstallMysqlThread();
        ~cInstallMysqlThread();

        bool Start(bool FullInstall=true);
        void Action();
        int State_; // 0=inactive; >0=in progress; 100=finished; <0=error
};

class cDatabaseDoctorMenu : public cOsdMenu
{
    private:
        int ErrNumber_;
        void Set();
        eOSState ProcessKey(eKeys Key);
    public:
        cDatabaseDoctorMenu(int ErrNumber=0);
        ~cDatabaseDoctorMenu();
};

class cDatabaseTools
{
    public:
        static bool Reset(); // Try to keep data
        static bool FullReset(); // drop data
        static bool InstallMysql();
        static bool CreateDatabase();
        static bool ClearDatabase();
        static bool ConfigureMysql();
        // E_DATABASE_CONNECTION_FAILED How to fix???
        static bool CreateAccount();
        static bool CreateTables();
        static bool SetPermissions();
};
#endif

class cMultiRoomSetupMenu: public cOsdMenu
{
    private:
        bool OnlineCheck();
        int DatabaseState_;

        // Menu attributes
        bool ExpertSettings_;
        bool TunerSettings_;

        // Network settings
        bool Eth2Available_;
        bool NC_Bridge_; // enable/disable
        int Bridge_; // Bridge to Ethernet 1 (=0) or Ethernet 2(=1)
        int Vlan_; // enable/disable
        int VlanID_;

        // Remote Control / Automation
        int UseSerialRemote;
        int UseUdpRemote;
        int UseUdpRemotePort;

        // is stream-dev plugin available?
        bool canStream; 

        int MultiRoom_; // active/inactive
        int MultiRoom_old_;
        int MultiRoomMode_; // Client/Server
        int MultiRoomMode_old_;

        // max number of clients that can connect at a time
        int maxClientNum;
        int oldMaxClientNum;

        // Type of connection to the NetClients: direct or network
        int ConnectionType_;

        // quickshutdown this vdr after 5 mins of inactivity, giving clients all the tuners ?
        int AvgLiveTv;

        nrTuners_t nrTuners; // satellite/Terr/Cable Tuner counts
        nrTuners_t nrTuners_old_;
        nrTuners_t nrTunersPhys_; // copy, remains unchanged
        bool MixedTuners; // more than one tuner type

        // ExpertSettings
        bool NC_Bridge_old_;
        int Bridge_old_;
        int Vlan_old_;
        int VlanID_old_;

        // ClientSettings
        std::vector<struct ReelBox_t> ReelBoxes;
        bool DatabaseAvailable_;
        bool DatabaseAvailableOld_;
        bool HasHarddisk_;
        bool AVGChanged_; // If you selected AVG then AVGChanged is true
        eReceiverType ReceiverType_;
        eReceiverType ReceiverTypeOld_;
        server_t NetServer;
        std::vector<std::string> mclifile;

        // currently selected text
        std::string currText;

        eOSState Save();
        int Server_Save();
        bool Client_Save();

        bool AreSettingsModified();
        bool IsCurrent(std::string str);    
        std::string TunersString(nrTuners_t *nrTuners);
#if APIVERSNUM >= 10700
	const char* GetTitle() { return Title(); }
#endif

    public:
        cMultiRoomSetupMenu();
        ~cMultiRoomSetupMenu();

        //Display items
        void Set();
        void Server_Set();
        void Server_AddInfo();
        void ServerExpert_Set();
        void ServerExpert_AddInfo(std::string currText);
        void Client_Set();
        void Client_AddInfo();

        // Handle keys and possibly redraw OSD calling Set()
        eOSState ProcessKey(eKeys key);
        eOSState Server_ProcessKey(eKeys key);
        eOSState ServerExpert_ProcessKey(eKeys key);
        eOSState Client_ProcessKey(eKeys Key);
};

class cMultiRoomSetupExpertMenu : public cOsdMenu
{
    private:
        nrTuners_t *nrTuners;
        nrTuners_t nrTunersOrig;
        nrTuners_t *nrTunersPhys;
        bool inTunerMenu;
        bool Eth2Available_;
        bool *NC_Bridge_;
        bool NC_Bridge_last_;
        bool NC_Bridge_old_;
        int *Bridge_;
        int Bridge_old_;
        int *Vlan_;
        int Vlan_last_;
        int Vlan_old_;
        int *VlanID_;
        int VlanID_old_;
        void Set();
        int *nrClients_;
        void AddInfo(std::string currText);
        eOSState ProcessKey(eKeys Key);
        void Save();
    public:
        cMultiRoomSetupExpertMenu(bool *NC_Bridge, int *Bridge,
              int *Vlan, int *VlanID, bool Eth2Available, nrTuners_t *tuners,
              nrTuners_t *tunersPhys, int *nrClients, bool TunerMenu = false);
        ~cMultiRoomSetupExpertMenu();
};

#endif

