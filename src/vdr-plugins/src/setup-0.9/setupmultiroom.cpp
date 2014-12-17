
#include <stdio.h>
#include <unistd.h>

#include "setup.h"
#include "setupmultiroom.h"
#include "menueditrefreshitems.h"
#include <vdr/sysconfig_vdr.h>

#include <vdr/debug.h>
#include <vdr/menuitems.h>
#include <vdr/plugin.h>
#include <vdr/s2reel_compat.h>

#include "../mcli/mcli_service.h"

#define DPRINT(x...) printf(x)

// Titles
#define OSD_TITLE trNOOP("MultiRoom Setup")
#define OSD_TITLE_EXPERT trNOOP("MultiRoom Setup (Expert)")
#define OSD_TITLE_TUNER trNOOP("MultiRoom Setup (Tuner)")

// Column widths
#if APIVERSNUM < 10700
    #define COL_WIDTH 22
    #define COL_WIDTH_CLIENT 22
#else
    #define COL_WIDTH 28
    #define COL_WIDTH_CLIENT 28
#endif


#define MULTI_ROOM_STR trNOOP("MultiRoom-System")
#define MR_MODE_STR trNOOP("MultiRoom Mode")

#define MR_MODE_INFO trNOOP("In Server Mode your recordings and NetCeiver/tuners are available to all clients on the network")

// Server
#define MSG1 trNOOP("MultiRoom option not available. Plugin is missing.")
#define MSG2 trNOOP("Please press OK to continue")
#define LIVETV_WITH_AVG_STR trNOOP("TV-Output on Avantgarde")
#define MAX_CLIENTS_STR trNOOP("Number of NetClients")
#define CONNECTION_TYPE_STR trNOOP("Connection via")

// Server Expert
#define NC_BRIDGE_STR trNOOP("Enable NetCeiver bridge")
#define BRIDGE_STR trNOOP("Bridge NetCeiver to")
#define VLAN_STR trNOOP("Use VLAN")
#define VLAN_ID_STR trNOOP("VLAN ID")

#define TIMERS_PATH "/etc/vdr/timers.conf"
#define MAX_LETTER_PER_LINE 48
#define NUMBERS_OF_ETHERNET 2
const char *EthernetStrings[NUMBERS_OF_ETHERNET] = { "Ethernet 1", "Ethernet 2" };
#define NUMBERS_OF_MODE 2
const char *MultiRoomModeStrings[NUMBERS_OF_MODE] = { "Server", "Client" };
enum EnumMultiRoomMode { eServer, eClient };

//#define OSD_TITLE trNOOP("MultiRoom Setup")
//#define MSG1 trNOOP("MultiRoom option not available. Plugin is missing.")
//#define MSG2 trNOOP("Please press OK to continue")

#ifdef RBMINI
  #define HDDEVICE "sdf"
#else
  #define HDDEVICE "sda4"
#endif

#define NETCLIENT_MODE_TEXT trNOOP("NetClient-Mode")
#define RECEIVER_TYPE_TEXT trNOOP("Receiver-Type")
#define STREAM_SERVER_TEXT trNOOP("Stream-Server")
#define RECORDING_SERVER_TEXT trNOOP("Recording-Server")

//const char* ClientModeStrings[] = { trNOOP("Stand-alone"), "MultiRoom", trNOOP("Hotel-Mode") };
//const char* ClientModeInactiveStrings[] = { trNOOP("Stand-alone"), trNOOP("MultiRoom (inactive)") };
const char* ReceiverTypeStringsAVG[] = { "Streamdev", "NetCeiver (mcli)" };

#define MAX_TUNERS_IN_MENU 16
typedef struct {
    int type[MAX_TUNERS_IN_MENU];
        char name[MAX_TUNERS_IN_MENU][128];
} tuner_info_t;


#define MULTI_ROOM_STR trNOOP("MultiRoom-System")
#define LIVETV_WITH_AVG_STR trNOOP("TV-Output on Avantgarde")
#define MAX_CLIENTS_STR trNOOP("Number of NetClients")
#define CONNECTION_TYPE_STR trNOOP("Connection via")

#define MAX_LETTER_PER_LINE 48

#ifdef USEMYSQL
#include <vdr/vdrmysql.h>

#define eMcli eModeMcli
#define eStreamdev eModeStreamdev

void StoreMcliConf(nrTuners_t *nrTuners, eReceiverType ReceiverType_)
{
  cSetupNetClientMenu::StoreMcliConf(nrTuners, ReceiverType_);
}

void parseAVGsSysconfig(bool *hasNasAsMediaDir)
{
  cSetupNetClientMenu::parseAVGsSysconfig(hasNasAsMediaDir);
}

void LoadMcliConf(nrTuners_t *nrTuners) {
    char *buffer;
    std::string strBuff;
    FILE *file = NULL;

    file = fopen("/etc/default/mcli", "r");
    if(file) {
        cReadLine readline;
        buffer = readline.Read(file);
        while(buffer) {
            if(strstr(buffer, "DVB_C_DEVICES=\"") && !strstr(buffer, "\"\""))
                nrTuners->cable = atoi(buffer+15);
            if(strstr(buffer, "DVB_S_DEVICES=\"") && !strstr(buffer, "\"\""))
                nrTuners->sat   = atoi(buffer+15);
            if(strstr(buffer, "DVB_S2_DEVICES=\"") && !strstr(buffer, "\"\""))
                nrTuners->satS2 = atoi(buffer+16);
            if(strstr(buffer, "DVB_T_DEVICES=\"") && !strstr(buffer, "\"\""))
                nrTuners->terr  = atoi(buffer+15);

            buffer = readline.Read(file);
        }
        fclose(file);
    }
}

void GetPhysTuners(nrTuners_t *nrTunersPhys) {
    cPlugin *mcliPlugin = cPluginManager::GetPlugin("mcli");
    if (mcliPlugin) {
        mclituner_info_t info;
        for (int i = 0; i < MAX_TUNERS_IN_MENU; i++)
            info.name[i][0] = '\0';
        mcliPlugin->Service("GetTunerInfo", &info);
        for (int i = 0; i < MAX_TUNERS_IN_MENU; i++) {
            if(info.preference[i] == -1 || strlen(info.name[i]) == 0)
                break;
            else {
                switch(info.type[i]) {
                    case FE_QPSK: // DVB-S
                        nrTunersPhys->sat++;
                        break;
                    case FE_DVBS2: // DVB-S2
                        nrTunersPhys->satS2++;
                        break;
                    case FE_OFDM: // DVB-T
                        nrTunersPhys->terr++;
                        break;
                    case FE_QAM: // DVB-C
                        nrTunersPhys->cable++;
                        break;
                }
            }
        }
    }
}


std::string GetUUID()
{
    std::string UUID;
    char command[64];
    FILE *file;
    char *buffer;

    sprintf(command, "ls -l /dev/disk/by-uuid/ | grep %s", HDDEVICE);
    file = popen(command, "r");
    if(file)
    {
        cReadLine readline;
        buffer = readline.Read(file);
        if(buffer)
        {
            char *ptr = strrchr(buffer, '>');
            if(ptr)
                *(ptr-2) = '\0';
            UUID = strrchr(buffer, ' ')+1;
        }
        else
            UUID = "";
        pclose(file);
    }

    return UUID;
}


bool FileExists(cString);

/* When no frontpanel is available,
 * '/dev/frontpanel' points to '/dev/null'
 * else to the appropriate device. eg.'/dev/ttyS2'
 *
 * easier : check for '/dev/.have_frontpanel'
 */
bool HasFrontPanel()
{
    if (FileExists("/dev/.have_frontpanel"))
        return true;

    // '/dev/frontpanel' points to '/dev/null' if reelbox does NOT
    // have a frontpanel
    char *realPath = ReadLink("/dev/frontpanel");
    if (realPath) {
        bool result = true;
        if (strcmp(realPath, "/dev/null")==0) // points to /dev/null ?
            result = false; // no front panel
        free(realPath);
        return result;
    }
    return false;
}

// check if the system is a client-hardware
bool IsClient()
{
    if (!HasFrontPanel()) // front panel is only available in Avg
        return true;

    if(cPluginManager::GetPlugin("rbmini")) // Netclient 1
        return true;

    return false;
}


int AvgDatabaseCheck(bool DisplayMessage)
{
    // mysql-server installed?
    if(SystemExec("mysqld --version 1>/dev/null 2>/dev/null"))
    {
        if(DisplayMessage)
            Skins.QueueMessage(mtError, tr("Error: Database Server is not installed!"));
        esyslog("%s(%i) ERROR (#1): mysql-server is not installed!", __FILE__, __LINE__);
        return E_DATABASE_NOT_INSTALLED;
    }

    // Does database "vdr" exist?
    if(access("/var/lib/mysql/vdr", R_OK))
    {
        if(DisplayMessage)
            Skins.QueueMessage(mtError, tr("Error: Database vdr doesn't exist!"));
        esyslog("%s(%i) ERROR (#2): Database vdr doesn't exist!", __FILE__, __LINE__);
        return E_DATABASE_VDR_NOT_EXISTENT;
    }

    // Is my.cnf set correctly?
    if(SystemExec("grep ^bind-address /etc/mysql/my.cnf | grep 0.0.0.0 1>/dev/null 2>/dev/null"))
    {
        if(DisplayMessage)
            Skins.QueueMessage(mtError, tr("Error: my.cnf is not set properly! (bind-address wrong)"));
        esyslog("%s(%i) ERROR (#3): my.cnf is not set properly (bind-address must be 0.0.0.0)!", __FILE__, __LINE__);
        return E_DATABASE_NOT_PROPERLY_CONFIGURED;
    }

    // Test Connection to DB
    cVdrMysqlAdmin *mysql = new cVdrMysqlAdmin(MYSQLADMINUSER, MYSQLADMINPWD, "vdr");
    if(!mysql->SetServer("localhost"))
    {
        if(DisplayMessage)
            Skins.QueueMessage(mtError, tr("Error: Connection to database failed!"));
        esyslog("%s(%i) ERROR (#4): Connection to database failed! (Maybe wrong permission/password?)", __FILE__, __LINE__);
        if(mysql)
            delete mysql;
        return E_DATABASE_CONNECTION_FAILED;
    }
    if(mysql)
        delete mysql;

    // Test reeluser Account
    mysql = new cVdrMysqlAdmin(MYSQLREELUSER, MYSQLREELPWD, "vdr");
    if(!mysql->SetServer("localhost"))
    {
        if(DisplayMessage)
            Skins.QueueMessage(mtError, tr("Error: Cannot login with reeluser!"));
        esyslog("%s(%i) ERROR (#5): Cannot login with reeluser!", __FILE__, __LINE__);
        if(mysql)
            delete mysql;
        return E_DATABASE_LOGIN_FAILED;
    }

    // Test Tables
    if(!mysql->TestTables()) {
        if(DisplayMessage)
            Skins.QueueMessage(mtError, "Error: some database-table doesn't exist!");
        esyslog("%s(%i) ERROR (#6): some database-table doesn't exist!", __FILE__, __LINE__);

        if(mysql)
            delete mysql;
        return E_DATABASE_TABLE_NOT_EXISTENT;
    }

//    // Test Permission for Reeluser
//    if(!mysql->TestPermission())
//    {
//        if(DisplayMessage)
//            Skins.QueueMessage(mtError, "Error: Missing DB-permission for reeluser!");
//        esyslog("%s(%i) ERROR (#7): Missing DB-permission for reeluser!", __FILE__, __LINE__);
//        if(mysql)
//            delete mysql;
//        return E_DATABASE_REELUSER_PERMISSION_MISSING;
//    }

    if(mysql)
        delete mysql;

#ifdef MYSQL_DEBUG
    std::cout << "\033[0;093m " << __FILE__ << '(' << __LINE__ << "): Database: OK! \033[0m\n";
#endif
    return 0;
}

//##################################################################################
// class cDatabaseChecker
//##################################################################################
cDatabaseChecker::cDatabaseChecker()
{
}

cDatabaseChecker::~cDatabaseChecker()
{
}

int cDatabaseChecker::Check(bool DisplayMessage)
{
    // mysql-server installed?
    if(SystemExec("mysqld --version 1>/dev/null 2>/dev/null"))
    {
        if(DisplayMessage)
            Skins.QueueMessage(mtError, tr("Error: Database Server is not installed!"));
        esyslog("%s(%i) ERROR (#1): mysql-server is not installed!", __FILE__, __LINE__);
        return E_DATABASE_NOT_INSTALLED;
    }

    // Does database "vdr" exist?
    if(access("/var/lib/mysql/vdr", R_OK))
    {
        if(DisplayMessage)
            Skins.QueueMessage(mtError, tr("Error: Database vdr doesn't exist!"));
        esyslog("%s(%i) ERROR (#2): Database vdr doesn't exist!", __FILE__, __LINE__);
        return E_DATABASE_VDR_NOT_EXISTENT;
    }

    // Is my.cnf set correctly?
    if(SystemExec("grep ^bind-address /etc/mysql/my.cnf | grep 0.0.0.0 1>/dev/null 2>/dev/null"))
    {
        if(DisplayMessage)
            Skins.QueueMessage(mtError, tr("Error: my.cnf is not set properly! (bind-address wrong)"));
        esyslog("%s(%i) ERROR (#3): my.cnf is not set properly (bind-address must be 0.0.0.0)!", __FILE__, __LINE__);
        return E_DATABASE_NOT_PROPERLY_CONFIGURED;
    }

    // Test Connection to DB
    cVdrMysqlAdmin *mysql = new cVdrMysqlAdmin(MYSQLADMINUSER, MYSQLADMINPWD, "vdr");
    if(!mysql->SetServer("localhost"))
    {
        if(DisplayMessage)
            Skins.QueueMessage(mtError, tr("Error: Connection to database failed!"));
        esyslog("%s(%i) ERROR (#4): Connection to database failed! (Maybe wrong permission/password?)", __FILE__, __LINE__);
        if(mysql)
            delete mysql;
        return E_DATABASE_CONNECTION_FAILED;
    }
    if(mysql)
        delete mysql;

    // Test reeluser Account
    mysql = new cVdrMysqlAdmin(MYSQLREELUSER, MYSQLREELPWD, "vdr");
    if(!mysql->SetServer("localhost"))
    {
        if(DisplayMessage)
            Skins.QueueMessage(mtError, tr("Error: Cannot login with reeluser!"));
        esyslog("%s(%i) ERROR (#5): Cannot login with reeluser!", __FILE__, __LINE__);
        if(mysql)
            delete mysql;
        return E_DATABASE_LOGIN_FAILED;
    }

    // Test Tables
    if(!mysql->TestTables()) {
        if(DisplayMessage)
            Skins.QueueMessage(mtError, "Error: some database-table doesn't exist!");
        esyslog("%s(%i) ERROR (#6): some database-table doesn't exist!", __FILE__, __LINE__);

        if(mysql)
            delete mysql;
        return E_DATABASE_TABLE_NOT_EXISTENT;
    }

//    // Test Permission for Reeluser
//    if(!mysql->TestPermission())
//    {
//        if(DisplayMessage)
//            Skins.QueueMessage(mtError, "Error: Missing DB-permission for reeluser!");
//        esyslog("%s(%i) ERROR (#7): Missing DB-permission for reeluser!", __FILE__, __LINE__);
//        if(mysql)
//            delete mysql;
//        return E_DATABASE_REELUSER_PERMISSION_MISSING;
//    }

    if(mysql)
        delete mysql;

#ifdef MYSQL_DEBUG
    std::cout << "\033[0;093m " << __FILE__ << '(' << __LINE__ << "): Database: OK! \033[0m\n";
#endif
    return 0;
}

//##################################################################################
// class cDatabaseDoctorMenu
//##################################################################################
cDatabaseDoctorMenu::cDatabaseDoctorMenu(int ErrNumber) : cOsdMenu(tr(OSD_TITLE),26), ErrNumber_(ErrNumber)
{
    if(!ErrNumber)
        ErrNumber_ = cDatabaseChecker::Check(false);
    Set();
}

cDatabaseDoctorMenu::~cDatabaseDoctorMenu()
{
}

void cDatabaseDoctorMenu::Set()
{
    bool EnableSetHelp = false;
    switch(ErrNumber_)
    {
        case E_DATABASE_NOT_INSTALLED:
            AddFloatingText(tr("Error: MySQL-Server is not installed.\n\n"
                        "Please press \"Fix it\" to install MySQL-Server. It may take some minutes."), MAX_LETTER_PER_LINE);
            EnableSetHelp = true;
            break;
        case E_DATABASE_VDR_NOT_EXISTENT:
            AddFloatingText(tr("Error: Database is not installed properly.\n\n"
                        "Please press \"Fix it\" to initialize the database."), MAX_LETTER_PER_LINE);
            EnableSetHelp = true;
            break;
        case E_DATABASE_NOT_PROPERLY_CONFIGURED:
            AddFloatingText(tr("Error: Permissions to access the database are not set properly.\n"
                        "Access will be possible from this box but not from others in the network. "
                        "\n\nPlease press \"Fix it\" to make database accessible from network."), MAX_LETTER_PER_LINE);
            EnableSetHelp = true;
            break;
        case E_DATABASE_CONNECTION_FAILED:
            AddFloatingText(tr("Error: Connection to DB failed. Check if mysqld is running"), MAX_LETTER_PER_LINE);
            break;
        case E_DATABASE_LOGIN_FAILED:
            AddFloatingText(tr("Error: Cannot login with reeluser to mysql.\n\n"
                        "Please press \"Fix it\" to create account reeluser."), MAX_LETTER_PER_LINE);
            EnableSetHelp = true;
            break;
        case E_DATABASE_TABLE_NOT_EXISTENT:
            AddFloatingText(tr("Error: Database is not installed properly.\n\n"
                        "Please press \"Fix it\" to create missing tables."), MAX_LETTER_PER_LINE);
            EnableSetHelp = true;
            break;
        case E_DATABASE_REELUSER_PERMISSION_MISSING:
            AddFloatingText(tr("Error: Permissions to access the database are not set properly.\n\n"
                        "Please press \"Fix it\" to set permissions for reeluser."), MAX_LETTER_PER_LINE);
            break;
        default:
            // Unknown Error Number
            AddFloatingText(tr("Error: An unknown error occured.\n"
                        "Try to reset Database."), MAX_LETTER_PER_LINE);
            break;
    }

    if(EnableSetHelp)
        SetHelp(tr("Fix it"));

    Display();
}

eOSState cDatabaseDoctorMenu::ProcessKey(eKeys Key)
{
    // If Installation of MySQL is running, do nothing else
    if(cPluginSetup::MysqlInstallThread_)
    {
        if(cPluginSetup::MysqlInstallThread_->Active())
        {
            if((cPluginSetup::MysqlInstallThread_->State_ > 0) && (cPluginSetup::MysqlInstallThread_->State_ < 100))
                Skins.Message(mtStatus, tr("Please wait..."));
            return osContinue;
        }
        else
        {
            if(cPluginSetup::MysqlInstallThread_->State_ == 100)
            {
                cPluginSetup::MysqlInstallThread_->State_ = 0; // Inactive
                // TODO: Reload Timer & Timersearch!
                Skins.Message(mtInfo, tr("MySQL-Server successfully installed."));
                return osBack;
            }
            if(cPluginSetup::MysqlInstallThread_->State_ < 0) // Error!
            {
                cPluginSetup::MysqlInstallThread_->State_ = 0; // Inactive
                Skins.Message(mtError, tr("Couldn't install MySQL-Server!"));
                return osContinue;
            }
        }
    }

    eOSState state = cOsdMenu::ProcessKey(Key);

    if(!HasSubMenu())
    {
        if(Key == kRed)
        {
            switch(ErrNumber_)
            {
                case 0:
                    // Reset();
                    // FullReset();
                    break;
                case E_DATABASE_NOT_INSTALLED:
                    Skins.Message(mtStatus, tr("Please wait..."));
                    if(!cDatabaseTools::InstallMysql())
                        Skins.Message(mtError, tr("Couldn't install MySQL-Server!"));
                    break;
                case E_DATABASE_VDR_NOT_EXISTENT:
                    Skins.Message(mtStatus, tr("Please wait..."));
                    if(cDatabaseTools::CreateDatabase())
                    {
                        Skins.Message(mtInfo, tr("Database successfully created."));
                        state = osBack;
                    }
                    else
                        Skins.Message(mtError, tr("Couldn't create Database!"));
                    break;
                case E_DATABASE_NOT_PROPERLY_CONFIGURED:
                    Skins.Message(mtStatus, tr("Please wait..."));
                    if(cDatabaseTools::ConfigureMysql())
                    {
                        Skins.Message(mtInfo, tr("Database successfully configured."));
                        state = osBack;
                    }
                    else
                        Skins.Message(mtError, tr("Couldn't configure Database!"));
                    break;
                case E_DATABASE_CONNECTION_FAILED:
                    // TODO: How to fix here?
                    break;
                case E_DATABASE_LOGIN_FAILED:
                    if(cDatabaseTools::CreateAccount())
                    {
                        Skins.Message(mtInfo, tr("Account reeluser successfully created."));
                        state = osBack;
                    }
                    else
                        Skins.Message(mtError, tr("Couldn't create reeluser account!"));
                    break;
                case E_DATABASE_TABLE_NOT_EXISTENT:
                    if(cDatabaseTools::CreateTables())
                    {
                        Skins.Message(mtInfo, tr("Missing tables successfully created."));
                        state = osBack;
                    }
                    else
                        Skins.Message(mtError, tr("Couldn't create missing tables!"));
                    break;
                case E_DATABASE_REELUSER_PERMISSION_MISSING:
                    cDatabaseTools::SetPermissions();
                    break;
                default:
                    // Unknown Error Number
                    // Reset();
                    // FullReset();
                    break;
            }
        }
    }

    return state;
}

//##################################################################################
// class cDatabaseTools
//##################################################################################
bool cDatabaseTools::Reset()
{
    //printf("\033[0;104m %s(%i): %s \033[0m\n",__FILE__, __LINE__, __PRETTY_FUNCTION__);
    return false;
}

bool cDatabaseTools::FullReset()
{
    //printf("\033[0;104m %s(%i): %s \033[0m\n",__FILE__, __LINE__, __PRETTY_FUNCTION__);
    return false;
}

bool cDatabaseTools::InstallMysql()
{
    //printf("\033[0;104m %s(%i): %s \033[0m\n",__FILE__, __LINE__, __PRETTY_FUNCTION__);
    return cPluginSetup::MysqlInstallThread_->Start(false);
}

bool cDatabaseTools::CreateDatabase()
{
    //printf("\033[0;104m %s(%i): %s \033[0m\n",__FILE__, __LINE__, __PRETTY_FUNCTION__);
    bool retValue = false;
    cVdrMysqlAdmin *mysql = NULL;
    mysql = new cVdrMysqlAdmin(MYSQLADMINUSER, NULL, NULL);
    if(mysql->SetServer("localhost"))
    {
        // without root-password
        if(mysql->CreateDatabase("vdr"))
            retValue = true;
        else
            Skins.QueueMessage(mtError, tr("Error: Initializing database failed!"));
    }
    else
    {
        // with root-password
        delete mysql;
        mysql = new cVdrMysqlAdmin(MYSQLADMINUSER, MYSQLADMINPWD, NULL);
        if(mysql->SetServer("localhost"))
        {
            if(mysql->CreateDatabase("vdr"))
                retValue = true;
            else
                Skins.QueueMessage(mtError, tr("Error: Initializing database failed!"));
        }
        else
            Skins.QueueMessage(mtError, tr("Error: Connecting database failed!"));
    }
    delete mysql;
    return retValue;
}

bool cDatabaseTools::ClearDatabase()
{
    //printf("\033[0;104m %s(%i): %s \033[0m\n",__FILE__, __LINE__, __PRETTY_FUNCTION__);
    bool retValue = false;
    cVdrMysqlAdmin *mysql = NULL;
    mysql = new cVdrMysqlAdmin(MYSQLADMINUSER, MYSQLADMINPWD, "vdr");
    if(mysql->SetServer("localhost"))
    {
        if(mysql->ClearDatabase())
            retValue = true;
        else
            Skins.QueueMessage(mtError, tr("Error: Clearing database failed!"));
    }
    else
        Skins.QueueMessage(mtError, tr("Error: Connecting database failed!"));
    delete mysql;
    return retValue;
}

bool cDatabaseTools::ConfigureMysql()
{
    //printf("\033[0;104m %s(%i): %s \033[0m\n",__FILE__, __LINE__, __PRETTY_FUNCTION__);
    // /etc/mysql/my.cnf settings
    if(SystemExec("grep ^bind-address /etc/mysql/my.cnf | grep 0.0.0.0 1>/dev/null 2>/dev/null"))
    {
        SystemExec("sed s/^bind-address.*/\"bind-address          = 0.0.0.0\"/ /etc/mysql/my.cnf > /tmp/my.cnf");
        if(!access("/tmp/my.cnf", R_OK) && !access("/etc/mysql/my.cnf", R_OK))
        {
            SystemExec("cp /etc/mysql/my.cnf /etc/mysql/my.cnf.bak");
            SystemExec("cp /tmp/my.cnf /etc/mysql/my.cnf");
            SystemExec("/etc/init.d/mysql restart");
            sleep(2); // Wait till mysqld is fully ready
            return true;
        }
    }

    // check if mysqld is running
    bool isMysqlRunning = (SystemExec("pidof mysqld")==0);

    // if not, restart mysql
    if (!isMysqlRunning) {
        SystemExec("/etc/init.d/mysql restart");
        sleep(2); // Wait till mysqld is fully ready
    } // if isMysqlRunning

    return false;
}

bool cDatabaseTools::CreateAccount()
{
    //printf("\033[0;104m %s(%i): %s \033[0m\n",__FILE__, __LINE__, __PRETTY_FUNCTION__);
    bool retValue = false;
    cVdrMysqlAdmin *mysql = new cVdrMysqlAdmin(MYSQLADMINUSER, MYSQLADMINPWD, NULL);
    if(mysql->SetServer("localhost"))
        retValue = mysql->CreateAccount();
    delete mysql;
    return retValue;
}

bool cDatabaseTools::CreateTables()
{
    //printf("\033[0;104m %s(%i): %s \033[0m\n",__FILE__, __LINE__, __PRETTY_FUNCTION__);
    bool retValue = false;
    cVdrMysqlAdmin *mysql = new cVdrMysqlAdmin(MYSQLADMINUSER, MYSQLADMINPWD, "vdr");
    if(mysql->SetServer("localhost"))
        retValue = mysql->CreateTables();
    delete mysql;
    return retValue;
}

bool cDatabaseTools::SetPermissions()
{
    //printf("\033[0;104m %s(%i): %s \033[0m\n",__FILE__, __LINE__, __PRETTY_FUNCTION__);
    return false;
}

//##################################################################################
// class cInstallMysqlThread
//##################################################################################
cInstallMysqlThread::cInstallMysqlThread() : cThread("MySQL Installation")
{
    State_ = 0; // Inactive
}

cInstallMysqlThread::~cInstallMysqlThread()
{
}

bool cInstallMysqlThread::Start(bool FullInstall)
{
    FullInstall_ = FullInstall;
    return cThread::Start();
}

void cInstallMysqlThread::Action()
{
    State_ = 1;
    if(FullInstall_)
        FullInstall();
    else
        SmallInstall();
}

void cInstallMysqlThread::FullInstall()
{
    // Install MySQL & transfer datas to MySQL
    int DB_State = 0;
    std::string strBuff;
    // Preparations
    Timers.IncBeingEdited();

    // MySQL-Server Installation
    bool installMysql = SystemExec("mysqld --version 1>/dev/null 2>/dev/null");
    if(installMysql)
    {
        SystemExec("export DEBIAN_FRONTEND=noninteractive; aptitude update");
        State_ = 20;
        SystemExec("export DEBIAN_FRONTEND=noninteractive; aptitude -o Aptitude::Cmdline::ignore-trust-violations=true -y install mysql-server");
        State_ = 40;
        sleep(2); // Wait till mysqld is fully ready
    }

    State_ = 45;
    cDatabaseTools::ConfigureMysql();

    State_ = 50;
    // Clear/Create Database
    if(access("/var/lib/mysql/vdr", R_OK))
        cDatabaseTools::CreateDatabase();
    else
        cDatabaseTools::ClearDatabase();
    State_ = 60;

    DB_State = cDatabaseChecker::Check(true);
    if(!DB_State)
    {
        // Transfer existing vdr-datas to DB
        TransferDataToMysqlTimers();
        cPluginManager::CallAllServices("MultiRoom enable", NULL); // epgsearch plugin transfers its data in DB
        State_ = 70;
        // Finish
#if 0 // why restart network when install & configuring database ?
        // Restart Network
        SystemExec("setup-net restart");
        sleep(25); // Wait because of Network-Restart (else AVAHI wouldn't work)
#endif

        // Clear DB-Events
        cVdrMysqlAdmin *mysql = new cVdrMysqlAdmin(MYSQLADMINUSER, MYSQLADMINPWD, "vdr");
        if(mysql->SetServer("localhost"))
        {
#ifdef MYSQL_DEBUG
            printf("\033[0;93m %s(%i): mysql->ClearEvents(); \033[0m\n", __FILE__, __LINE__);
#endif
            mysql->ClearEvents();
        }
        delete mysql;

        Timers.DecBeingEdited();
        State_ = 100;

        strBuff = std::string(tr("Changes saved")) + " (100%)";
        Skins.QueueMessage(mtInfo, strBuff.c_str());
    }
    else
    {
        Timers.DecBeingEdited();
        State_ = -DB_State; // State must have negative number if somewhat went wrong!
    }
}

void cInstallMysqlThread::SmallInstall()
{
    // Just install MySQL-Server
    SystemExec("export DEBIAN_FRONTEND=noninteractive; aptitude update");
    SystemExec("export DEBIAN_FRONTEND=noninteractive; aptitude -o Aptitude::Cmdline::ignore-trust-violations=true -y install mysql-server");
    sleep(2); // Wait till mysqld is fully ready
    State_ = 100;
}

void cInstallMysqlThread::TransferDataToMysqlTimers()
{
    cTimersMysql *mysql = new cTimersMysql(MYSQLREELUSER, MYSQLREELPWD, "vdr");
    if(mysql->SetServer("localhost"))
    {
        for (cTimer *Timer = Timers.First(); Timer; Timer = Timers.Next(Timer))
            mysql->InsertTimer(Timer);
    }
    else
        Skins.Message(mtError, tr("Error: Connecting database failed!"));
    delete mysql;
}
#endif

//##################################################################################
// class cMultiRoomSetupMenu
//##################################################################################
const char *ConnectionTypeStrings[2] = { trNOOP("direct"), trNOOP("network") };

bool IsBridgeRequired()
{
    // if 'eth1' network device is present... then bridge is required
    return IsNetworkDevicePresent("eth1");
}

cMultiRoomSetupMenu::cMultiRoomSetupMenu() : cOsdMenu(tr(OSD_TITLE))
{
    ExpertSettings_ = false;
    TunerSettings_ = false;

    DatabaseState_ = 0;
    canStream = true;

    // check for StreamDev server plugin
    cPlugin *p = cPluginManager::GetPlugin("streamdev-server");
    if (p)
    {
        struct A
        {
            int maxClients;
            int startServer;
        } a;
        p->Service("Streamdev Server Setup data", &a);

        maxClientNum = a.maxClients;

        canStream = true;
    }
    else canStream = false;
    Eth2Available_ = SystemExec("ifconfig eth2 2>/dev/null 1>/dev/null") ? false : true;

    maxClientNum = Setup.MaxMultiRoomClients;

    MultiRoom_old_ = MultiRoom_ = (bool)Setup.ReelboxMode; // bool because 1!=2
#ifdef RBMINI
    MultiRoomMode_ = eClient;
#else
    if(Setup.ReelboxMode == eModeClient)
        MultiRoomMode_ = eClient;
    else
        MultiRoomMode_ = eServer;
#endif
    MultiRoomMode_old_ = MultiRoomMode_;

    if(!MultiRoom_)
        maxClientNum = 1;
    oldMaxClientNum = maxClientNum;

    if(MultiRoomMode_ == eServer)
        nrTuners.sat = nrTuners.satS2 = nrTuners.terr = nrTuners.cable = 6;
    else
        nrTuners.sat = nrTuners.satS2 = nrTuners.terr = nrTuners.cable = 1;

    nrTunersPhys_.sat = nrTunersPhys_.satS2 = nrTunersPhys_.terr = nrTunersPhys_.cable = 0;
    GetPhysTuners(&nrTunersPhys_);
    LoadMcliConf(&nrTuners);
    memcpy(&nrTuners_old_, &nrTuners, sizeof(struct nrTuners));

    bool onlySatS2 = !nrTunersPhys_.terr && !nrTunersPhys_.sat && !nrTunersPhys_.cable && nrTunersPhys_.satS2;
    bool onlySat = !nrTunersPhys_.terr && nrTunersPhys_.sat && !nrTunersPhys_.cable && !nrTunersPhys_.satS2;
    bool onlyTerr = nrTunersPhys_.terr && !nrTunersPhys_.sat && !nrTunersPhys_.cable && !nrTunersPhys_.satS2;
    bool onlyCable = !nrTunersPhys_.terr && !nrTunersPhys_.sat && nrTunersPhys_.cable && !nrTunersPhys_.satS2;
    MixedTuners = !(onlySatS2 || onlySat || onlyTerr || onlyCable) && (nrTunersPhys_.satS2 + nrTunersPhys_.cable + nrTunersPhys_.terr + nrTunersPhys_.sat > 0);


    //so that maxitem is set correctly
    Clear();
    Display();
    currText = tr(MULTI_ROOM_STR);

    // live tv in avantgarde
    AvgLiveTv = Setup.LiveTvOnAvg;
    cSysConfig_vdr &sysconfig = cSysConfig_vdr::GetInstance();
    sysconfig.Load("/etc/default/sysconfig");

    // Get Connection Type
    ConnectionType_ = 0; // 0=direct
    if(sysconfig.GetVariable("NC_CONNECTION_TYPE") && !strcmp(sysconfig.GetVariable("NC_CONNECTION_TYPE"), "network"))
        ConnectionType_ = 1;

    // Get Variable for Bridge-Setting
    NC_Bridge_ = true; // Default: Enabled
    if(MultiRoom_old_)
    {
        if(sysconfig.GetVariable("NC_BRIDGE") && !strcmp(sysconfig.GetVariable("NC_BRIDGE"), "no"))
            NC_Bridge_ = false;
    }

    // if bridge is not required, lets not show the option for bridge settings
    if (!IsBridgeRequired())
        NC_Bridge_ = false;

    NC_Bridge_old_ = NC_Bridge_;

    Bridge_ = 0; // Default: Ethernet 1
    if(MultiRoom_old_ && sysconfig.GetVariable("ETH0_BRIDGE"))
    {
        if(!strcmp(sysconfig.GetVariable("ETH0_BRIDGE"), "Ethernet 1"))
            Bridge_=0; // Ethernet 1
        else if(!strcmp(sysconfig.GetVariable("ETH0_BRIDGE"), "Ethernet 2"))
            Bridge_=1; // Ethernet 2
    }
    Bridge_old_ = Bridge_;

    // Get Variable for VLAN-Setting
    Vlan_ = Vlan_old_ = 1; // Default: Enabled
    if(MultiRoom_old_)
    {
        if(sysconfig.GetVariable("VLAN") && !strcmp(sysconfig.GetVariable("VLAN"), "no"))
            Vlan_ = Vlan_old_ = 0;
    }

    VlanID_ = 2; // Default: 2
    if(sysconfig.GetVariable("VLAN_ID"))
        VlanID_ = atoi(sysconfig.GetVariable("VLAN_ID"));
    VlanID_old_ = VlanID_;

    // Client Settings
    // Check if DB is available
    if(Setup.ReelboxMode && !Setup.ReelboxModeTemp)
        DatabaseAvailable_ = DatabaseAvailableOld_= false;
    else
        DatabaseAvailable_ = DatabaseAvailableOld_= true;

    AVGChanged_ = false;

    if(GetUUID()=="")
        HasHarddisk_ = false;
    else
        HasHarddisk_ = true;

    // Get ReelBox-List from avahi-plugin
    p=cPluginManager::GetPlugin("avahi");
    if(p) {
        p->Service("Avahi MultiRoom-List", &ReelBoxes);
    } else {
        printf("ERROR: could not get the avahi-Plugin, menu will be broken\n");
        esyslog("ERROR: could not get the avahi-Plugin, menu will be broken\n");
    }

    ReceiverType_ = ReceiverTypeOld_ = eMcli;
    if (cPluginManager::GetPlugin("streamdev-client"))
    {
        if(Setup.Get("StartClient", "streamdev-client"))
            if(!strcmp(Setup.Get("StartClient", "streamdev-client")->Value(), "1"))
                ReceiverType_ = ReceiverTypeOld_ = eStreamdev;
    }

    // Mode: Avantgarde (=streamdev)
    NetServer.IP = Setup.NetServerIP;
    NetServer.Name = Setup.NetServerName;
    NetServer.MAC = Setup.NetServerMAC;

    // display
    Set();
}

cMultiRoomSetupMenu::~cMultiRoomSetupMenu()
{
}

void cMultiRoomSetupMenu::Set()
{
    if(MultiRoomMode_ == eServer)
    {
        if(ExpertSettings_ || TunerSettings_)
            ServerExpert_Set();
        else
            Server_Set();
    }
    else if(MultiRoomMode_ == eClient)
        Client_Set();
}

void cMultiRoomSetupMenu::Server_Set()
{
    //printf("%s\n",__PRETTY_FUNCTION__);
    char strBuff[256];
    // clear OSD
    Clear();
    SetTitle(tr(OSD_TITLE));
    SetCols(COL_WIDTH);



    // add items to OSD
    Add(new cMenuEditBoolItem(tr(MULTI_ROOM_STR), &MultiRoom_, tr("inactive"),tr("active")), IsCurrent(tr(MULTI_ROOM_STR)));
    if (MultiRoom_)
    Add(new cMenuEditStraItem(tr(MR_MODE_STR), &MultiRoomMode_, NUMBERS_OF_MODE, MultiRoomModeStrings),
        IsCurrent(tr(MR_MODE_STR)));

    if(MultiRoom_)
    {
        Add(new cMenuEditIntItem(tr(MAX_CLIENTS_STR), &maxClientNum, 1, 6  ), IsCurrent(tr(MAX_CLIENTS_STR)));
        // Connection Type
	/*
        if(maxClientNum>1)
        {
            snprintf(strBuff, 256, "%s:\t%s", tr(CONNECTION_TYPE_STR), tr(ConnectionTypeStrings[1]));
            Add(new cOsdItem(strBuff, osUnknown, true), IsCurrent(tr(CONNECTION_TYPE_STR)));
        }
        else
            Add(new cMenuEditStraRefreshItem(tr(CONNECTION_TYPE_STR), &ConnectionType_, 2, ConnectionTypeStrings), IsCurrent(tr(CONNECTION_TYPE_STR)));
	*/
        // Watch TV with AVG
        Add(new cMenuEditBoolItem(tr(LIVETV_WITH_AVG_STR), &AvgLiveTv, tr("inactive"), tr("active")), IsCurrent(tr(LIVETV_WITH_AVG_STR)));

    }

    if(MultiRoom_old_)
        DatabaseState_ = AvgDatabaseCheck(false);
    if (currText.size()) Server_AddInfo();
    // show on screen
    SetHelp(NULL,NULL,tr("Tuner"),MultiRoom_?tr("Expert"):NULL);
    Display();
}

std::string cMultiRoomSetupMenu::TunersString(nrTuners_t *nrTuners) {
    std::stringstream str;
    if (nrTuners->sat + nrTuners->satS2 > 0) {
        str << nrTuners->sat + nrTuners->satS2 << tr("x Satellite ");
        if (nrTuners->terr > 0 || nrTuners->cable > 0)
            str << "- ";
    }
    if (nrTuners->terr > 0) {
        str << nrTuners->terr << tr("x Terrestrial ");
        if (nrTuners->cable > 0)
            str << "- ";
    }
    if (nrTuners->cable > 0)
        str << nrTuners->cable << tr("x Cable ");
    if (nrTuners->cable <=0 && nrTuners->terr <= 0 && nrTuners->sat + nrTuners->satS2 <= 0 )
        str << "0";
    return str.str();
}

void cMultiRoomSetupMenu::Server_AddInfo()
{
    //Adds info depending on the current item
    //printf("%s %s\n",__PRETTY_FUNCTION__, currText.c_str());

    char strBuff[1024];
    int count = Count();
    int max_items = displayMenu->MaxItems();
    int num_blank_lines = max_items - count - 10; // 6 lines at the bottom contain info
    while (num_blank_lines-- > 0)
        Add(new cOsdItem("",osUnknown, false));
    int liveTvPossible;
    int minRecPossible;
    int requestedTV = maxClientNum + (int)AvgLiveTv;

    std::string text (currText);

    // number of tuners, type of tuners etc
    char buffer[128];
    snprintf(buffer, 127, "%s:\t%s", tr("Available tuners in MultiRoom Network"), TunersString(&nrTunersPhys_).c_str());
    Add(new cOsdItem(buffer,osUnknown, false));

    Add(new cOsdItem("____________________________________________________________",osUnknown, false));
    Add(new cOsdItem("",osUnknown, false));

    if(MultiRoom_)
    {
        switch(DatabaseState_)
        {
            case 0:
                // Everything ok
                break;
                // TODO: try to fix these errors automatically
            case E_DATABASE_NOT_INSTALLED:
                AddFloatingText(tr("Error: MySQL-Server is not installed.\n"
                        "Disable MultiRoom, then enable it again to try the install again. "
                        "Or try installing the package 'mysql-server' manually"), MAX_LETTER_PER_LINE);
                break;
            case E_DATABASE_VDR_NOT_EXISTENT:
                AddFloatingText(tr("Error: Database is not installed properly.\n"
                           "Disable MultiRoom, then enable it again to try to initialize the database again. "
                        ), MAX_LETTER_PER_LINE);
                break;
            case E_DATABASE_NOT_PROPERLY_CONFIGURED:
                AddFloatingText(tr("Error: Permissions to access the database are not set properly.\n"
                                   "Access will be possible from this box but not from others in the network. "
                                   "Change 'bind-address' in /etc/mysql/my.cnf."), MAX_LETTER_PER_LINE);
                break;
            case E_DATABASE_CONNECTION_FAILED:
                AddFloatingText(tr("Error: Connection to DB failed. Check if mysqld is running"), MAX_LETTER_PER_LINE);
                break;
            case E_DATABASE_LOGIN_FAILED:
                AddFloatingText(tr("Error: Cannot login with reeluser to mysql. Try re-installing mysql-server."), MAX_LETTER_PER_LINE);
                break;
            case E_DATABASE_TABLE_NOT_EXISTENT:
                AddFloatingText(tr("Error: Database is not installed properly.\n"
                           "Disable MultiRoom, then enable it again to try to initialize the database again. "
                        ), MAX_LETTER_PER_LINE);
                break;
            case E_DATABASE_REELUSER_PERMISSION_MISSING:
                // TODO: Error-Text
                break;
            default:
                // Undefined errNumber
                AddFloatingText(tr("Error: An unknown error occured.\n"
                                   "Disable MultiRoom, manually deinstall mysql-server, then "
                                   "enable MultiRoom again."), MAX_LETTER_PER_LINE);
                break;
        }
    }

    if(!DatabaseState_ || !MultiRoom_)
    {
        if (MixedTuners)
        {
            Add(new cOsdItem(tr("Using mixed tuner types with NetClients is not"), osUnknown,false));
            Add(new cOsdItem(tr("yet supported."), osUnknown,false));
            Add(new cOsdItem("",osUnknown, false));
        }

        if( text.find(tr(MULTI_ROOM_STR))==0)
        {
            if(!MultiRoom_ && !MixedTuners)
                AddFloatingText(tr("Enable the MultiRoom-System to connect NetClients in "
                            "the network with your ReelBox Avantgarde and to provide "
                            "TV-signals to the NetClients."), MAX_LETTER_PER_LINE);
            else if(MultiRoom_)
            {
                if(MultiRoom_old_)
                    AddFloatingText(tr("TV-Programs are now available in your network. Please configure your network components according to the installation guide."), MAX_LETTER_PER_LINE);
                else
                    AddFloatingText(tr("MultiRoom setup will now be completed if you confirm the settings with 'Ok'. "
                                "You can then continue with the installation of the NetClients."
                                ), MAX_LETTER_PER_LINE);
            }
        }
        else if(text.find(tr(LIVETV_WITH_AVG_STR))==0)
        {
            if (AvgLiveTv)
            {
                if (!MixedTuners && (requestedTV > nrTunersPhys_.sat + nrTunersPhys_.satS2 + nrTunersPhys_.terr + nrTunersPhys_.cable))
                {
                    Add(new cOsdItem(tr("Caution: For this configuration there are not"),osUnknown, false));
                    Add(new cOsdItem(tr("enough tuners available for LiveTV and recording.") ,osUnknown, false));
                    Add(new cOsdItem(tr("simultaneously."),osUnknown, false));
                }
                else
                {
                    AddFloatingText(tr("MultiRoom setup will now be completed if you confirm the settings with 'Ok'. "
                                "You can then continue with the installation of the NetClients."
                                ), MAX_LETTER_PER_LINE);
                }
            }
            else
            {
                AddFloatingText(tr("TV-Output will be disabled after 5 minutes. "
                            "All tuners are exclusively available to NetClients."
                            ), MAX_LETTER_PER_LINE);
                if (!MixedTuners && (requestedTV > nrTunersPhys_.sat + nrTunersPhys_.satS2 + nrTunersPhys_.terr + nrTunersPhys_.cable))
                {
                    Add(new cOsdItem("", osUnknown, false));
                    Add(new cOsdItem(tr("Caution: For this configuration there are not"),osUnknown, false));
                    Add(new cOsdItem(tr("enough tuners for providing"),osUnknown, false));
                    Add(new cOsdItem(tr("TV-Signals for all NetClients simultaneously."),osUnknown, false));
                }
            }
        }
        else if (text.find(tr(MAX_CLIENTS_STR))==0)
        {
            if(!MixedTuners)
            {
                if (requestedTV > nrTunersPhys_.sat + nrTunersPhys_.satS2 + nrTunersPhys_.terr + nrTunersPhys_.cable)
                {
                    //Warn
                    Add(new cOsdItem(tr("Caution: In the selected configuration there are"),osUnknown, false));
                    Add(new cOsdItem(tr("not enough tuners available to be able to provide"),osUnknown, false));
                    Add(new cOsdItem(tr("TV-signals to all NetClients at the same time."), osUnknown, false));
                    if(nrTunersPhys_.sat + nrTunersPhys_.satS2 + nrTunersPhys_.terr + nrTunersPhys_.cable < 6) // if NetCeiver is fully upgraded there is no space left for more tuners
                    {
                        Add(new cOsdItem("", osUnknown, false));
                        Add(new cOsdItem(tr("Please extend the number of tuners to be able"),osUnknown, false));
                        Add(new cOsdItem(tr("to use all NetClients simultaneously."),osUnknown, false));
                    }
                }
                else if(nrTunersPhys_.sat + nrTunersPhys_.satS2 + nrTunersPhys_.terr + nrTunersPhys_.cable == 1)
                {
                    Add(new cOsdItem(tr("Using this configuration i.e. one of"),osUnknown, false));
                    Add(new cOsdItem(tr("these actions is possible:"),osUnknown, false));
                    Add(new cOsdItem("", osUnknown, false));
                    Add(new cOsdItem(tr("1x TV-Recording through NetClient or"),osUnknown, false));
                    Add(new cOsdItem(tr("1x Live-TV through NetClient"),osUnknown, false));
                }
                else
                {
                    Add(new cOsdItem(tr("Using this configuration these simultanous"),osUnknown, false));
                    Add(new cOsdItem(tr("actions are possible:"),osUnknown, false));
                    Add(new cOsdItem("", osUnknown, false));
                    liveTvPossible = std::min((nrTunersPhys_.sat + nrTunersPhys_.satS2 + nrTunersPhys_.terr + nrTunersPhys_.cable)/2, requestedTV);
                    minRecPossible = (nrTunersPhys_.sat + nrTunersPhys_.satS2 + nrTunersPhys_.terr + nrTunersPhys_.cable) - liveTvPossible;
                    char buffer[128];
                    snprintf(buffer, 127, "%ix %s%s",
                            minRecPossible, tr("TV-Recordings through NetClients"), AvgLiveTv ? tr(" or Avantgarde") : "");
                    Add(new cOsdItem(buffer,osUnknown, false));
                    snprintf(buffer, 127, "%ix %s%s",
                            liveTvPossible, tr("Live-TV through NetClients"), AvgLiveTv ? tr(" or Avantgarde") : "");
                    Add(new cOsdItem(buffer,osUnknown, false));
                }
            }
        }
        else if (text.find(tr(CONNECTION_TYPE_STR))==0)
        {
            if(maxClientNum>1)
            {
                AddFloatingText(tr("NetworkConnectPlur$Please connect your NetClients with the network switch."), MAX_LETTER_PER_LINE);
            }
            else
            {
                if(ConnectionType_)
                    AddFloatingText(tr("NetworkConnectSing$Please connect your NetClient with the network switch."), MAX_LETTER_PER_LINE);
                else
                {
                    snprintf(strBuff, 1024, tr("DirectConnect$Please connect your NetClient to the port labeled \"Ethernet %i\"."), Bridge_+1);
                    AddFloatingText(strBuff, MAX_LETTER_PER_LINE);
                }
            }
        } else if (text.find(tr(MR_MODE_STR))==0) {
            AddFloatingText(tr(MR_MODE_INFO), MAX_LETTER_PER_LINE);
        }
    }
}

void cMultiRoomSetupMenu::ServerExpert_Set()
{
    int current = Current();
    //printf("\033[0;41m current: %i \033[0m\n", current);
    Clear();
    SetCols(COL_WIDTH);

    if (!TunerSettings_)
    {
        SetTitle(tr(OSD_TITLE_EXPERT));
            Add(new cMenuEditBoolItem(tr(MULTI_ROOM_STR), &MultiRoom_, tr("inactive"),tr("active")), IsCurrent(tr(MULTI_ROOM_STR)));
        Add(new cMenuEditStraItem(tr(MR_MODE_STR), &MultiRoomMode_, NUMBERS_OF_MODE, MultiRoomModeStrings));
        // Bridge settings
        if (IsBridgeRequired()) {
            Add(new cMenuEditBoolItem(tr(NC_BRIDGE_STR), (int*) &NC_Bridge_, tr("no"), tr("yes")));
            if(NC_Bridge_) {
                if(Eth2Available_)
                    Add(new cMenuEditStraItem(tr(BRIDGE_STR), &Bridge_, NUMBERS_OF_ETHERNET, EthernetStrings ));
                Add(new cOsdItem("", osUnknown, false));
                // VLAN settings
                Add(new cMenuEditBoolItem(tr(VLAN_STR), &Vlan_, tr("no"), tr("yes")));
                if(Vlan_)
                    Add(new cMenuEditIntItem(tr(VLAN_ID_STR), &VlanID_, 1, 9));
            }
        }

        // If bridge is not required, still vlan maybe necessary
        if (!IsBridgeRequired())
        {
            // VLAN settings
            Add(new cMenuEditBoolItem(tr(VLAN_STR), &Vlan_, tr("no"), tr("yes")));
            if(Vlan_)
                Add(new cMenuEditIntItem(tr(VLAN_ID_STR), &VlanID_, 1, 9));
        }

        if(current==-1)
            SetCurrent(Get(0));
        else
            SetCurrent(Get(current));
    }
    else  // Tuner settings
    {
        SetTitle(tr(OSD_TITLE_TUNER));
        Add(new cOsdItem(tr("Number of tuners assigned locally:"), osUnknown, false));
        Add(new cOsdItem("", osUnknown, false));
        if (nrTunersPhys_.sat)
            Add(new cMenuEditIntItem(tr("Satellite (DVB-S)"), &nrTuners.sat, 0, 6));
        if (nrTunersPhys_.satS2)
            Add(new cMenuEditIntItem(tr("Satellite (DVB-S2)"), &nrTuners.satS2, 0, 6));
        if (nrTunersPhys_.cable)
            Add(new cMenuEditIntItem(tr("Cable"), &nrTuners.cable, 0, 6));
        if (nrTunersPhys_.terr)
            Add(new cMenuEditIntItem(tr("Terrestrial"), &nrTuners.terr, 0, 6));

        Add(new cOsdItem("", osUnknown, false));
        Add(new cOsdItem("____________________________________________________________",osUnknown, false));
        Add(new cOsdItem("", osUnknown, false));
        Add(new cOsdItem(tr("Tuners available for the whole MultiRoom-System:"), osUnknown, false));
        Add(new cOsdItem("", osUnknown, false));
        std::stringstream buf;
        if (nrTunersPhys_.sat)
        {
            buf << tr("Satellite (DVB-S)") << ":\t " << nrTunersPhys_.sat;
            Add(new cOsdItem(buf.str().c_str(), osUnknown, false));
        }
        buf.str("");
        if (nrTunersPhys_.satS2)
        {
            buf << tr("Satellite (DVB-S2)") << ":\t " << nrTunersPhys_.satS2;
            Add(new cOsdItem(buf.str().c_str(), osUnknown, false));
        }
        buf.str("");
        if (nrTunersPhys_.cable)
        {
            buf << tr("Cable") << ":\t " << nrTunersPhys_.cable;
            Add(new cOsdItem(buf.str().c_str(), osUnknown, false));
        }
        buf.str("");
        if (nrTunersPhys_.terr)
        {
            buf << tr("Terrestrial") << ":\t " << nrTunersPhys_.terr;
            Add(new cOsdItem(buf.str().c_str(), osUnknown, false));
        }

        if(current == -1 || current == 0)
            SetCurrent(Get(2));
        else
            SetCurrent(Get(current));
    }

    if (!TunerSettings_ && Get(Current()) && Get(Current())->Text())
        ServerExpert_AddInfo(Get(Current())->Text());
    SetHelp(NULL, NULL, TunerSettings_?tr("Normal"):tr("Tuner"),MultiRoom_?(ExpertSettings_?tr("Normal"):tr("Expert")):NULL);
    Display();
}

void cMultiRoomSetupMenu::ServerExpert_AddInfo(std::string currText)
{
    //Adds info depending on the current item
    //printf("%s %s\n",__PRETTY_FUNCTION__, currText.c_str());

    int num_blank_lines = displayMenu->MaxItems() - Count() - 10;
    while (--num_blank_lines > 0) Add(new cOsdItem("",osUnknown, false));

    Add(new cOsdItem("____________________________________________________________",osUnknown, false));
    Add(new cOsdItem("",osUnknown, false));

    if( currText.find(tr(NC_BRIDGE_STR))==0)
    {
        if(NC_Bridge_) // NC_Bridge enabled?
            AddFloatingText(tr("bridge_on$TV-data is now available in the network for the NetClients.\n"
                "Please note that you also have to enable VLAN and configure the "
                "switches inside your network if you experience problems with "
                "your network."
                  ), MAX_LETTER_PER_LINE);
        else
            AddFloatingText(tr("bridge_off$Activate the network bridge to provide TV-signals to your home "
                               "network.\n"
                               "Warning: As soon as you activate the bridge, network components "
                   "like WLAN routers will possibly not work correct any longer. "
                   "Thus also enable VLAN and configure your switches according to "
                   "the installation guide."
                  ), MAX_LETTER_PER_LINE);
    }
    else if(currText.find(tr(BRIDGE_STR))==0)
    {
        AddFloatingText(tr("Dummytext$"), MAX_LETTER_PER_LINE);
    }
    else if (currText.find(tr(VLAN_STR))==0)
    {
        if(Vlan_) // VLAN enabled?
            AddFloatingText(tr("vlan_on$VLAN ist now active. You can now set up your network switches."
                              ), MAX_LETTER_PER_LINE);
        else
            AddFloatingText(tr("vlan_off$VLANs are used to separate the network traffic into multiple "
                "virtual Networks. With this option you can limit the traffic of "
                "the NetCeiver to one VLAN.\n"
                "To reasonably use VLANs you also need VLAN capable switches. "
                "You can receive preconfigured switches "
                "for the Reel MultiRoom System at your local dealer."
                ), MAX_LETTER_PER_LINE);
    }
    else if (currText.find(tr(VLAN_ID_STR))==0)
    {
        AddFloatingText(tr("bridge_id$Each VLAN is referenced by its own unique ID. It is "
               "recommended to use the ID '2' for NetCeiver and NetClients."
              ), MAX_LETTER_PER_LINE);
    } else if (currText.find(tr(MR_MODE_STR))==0) {
        AddFloatingText(tr(MR_MODE_INFO), MAX_LETTER_PER_LINE);
    }
}

void cMultiRoomSetupMenu::Client_Set()
{
    DatabaseAvailableOld_ = DatabaseAvailable_;
    std::string strBuff;
    int current = Current();
    Clear();

    if(TunerSettings_)
    {
        SetTitle(tr(OSD_TITLE_TUNER));
        SetCols(COL_WIDTH);
        Add(new cOsdItem(tr("Number of tuners to use on your Avantgarde:"), osUnknown, false));
        Add(new cOsdItem("", osUnknown, false));
        Add(new cMenuEditIntItem(tr("Satellite (DVB-S)"), &nrTuners.sat, 0, 6));
        Add(new cMenuEditIntItem(tr("Satellite (DVB-S2)"), &nrTuners.satS2, 0, 6));
        Add(new cMenuEditIntItem(tr("Cable"), &nrTuners.cable, 0, 6));
        Add(new cMenuEditIntItem(tr("Terrestrial"), &nrTuners.terr, 0, 6));
        Add(new cOsdItem("____________________________________________________________",osUnknown, false));
        Add(new cOsdItem("", osUnknown, false));
        Add(new cOsdItem(tr("Tuners available for the whole MultiRoom-System:"), osUnknown, false));
        Add(new cOsdItem("", osUnknown, false));
        std::stringstream buf;
        buf << tr("Satellite (DVB-S)") << ":\t " << nrTunersPhys_.sat;
        Add(new cOsdItem(buf.str().c_str(), osUnknown, false));
        buf.str("");
        buf << tr("Satellite (DVB-S2)") << ":\t " << nrTunersPhys_.satS2;
        Add(new cOsdItem(buf.str().c_str(), osUnknown, false));
        buf.str("");
        buf << tr("Cable") << ":\t " << nrTunersPhys_.cable;
        Add(new cOsdItem(buf.str().c_str(), osUnknown, false));
        buf.str("");
        buf << tr("Terrestrial") << ":\t " << nrTunersPhys_.terr;
        Add(new cOsdItem(buf.str().c_str(), osUnknown, false));

        if(current == -1 || current == 0)
            SetCurrent(Get(2));
        else
            SetCurrent(Get(current));

        SetHelp(NULL, NULL, tr("Normal"), tr("Expert"));
    }
    else
    {
        SetCols(COL_WIDTH_CLIENT);
        if(ExpertSettings_)
            SetTitle(tr(OSD_TITLE_EXPERT));
        else
            SetTitle(tr(OSD_TITLE));


        Add(new cMenuEditBoolRefreshItem(tr(MULTI_ROOM_STR), &MultiRoom_, tr("inactive"),tr("active")), IsCurrent(tr(MULTI_ROOM_STR)));

        if(MultiRoom_) {
            cOsdItem *item = NULL;
            Add(item=new cMenuEditStraRefreshItem(tr(MR_MODE_STR), &MultiRoomMode_, NUMBERS_OF_MODE, MultiRoomModeStrings));

            /* NetClients cannot act as servers, so donot offer the option to change MultiRoom Mode */
            if (IsClient())
                item->SetSelectable(false);
        }

        if(MultiRoom_ || ExpertSettings_)
        {
            if(ExpertSettings_)
                Add(new cMenuEditStraRefreshItem(tr(RECEIVER_TYPE_TEXT), (int*)&ReceiverType_, 2, ReceiverTypeStringsAVG), IsCurrent(tr(RECEIVER_TYPE_TEXT)));
            switch(ReceiverType_)
            {
                case eStreamdev:
                    strBuff = std::string(STREAM_SERVER_TEXT) + ":\t";
                    if(ReelBoxes.size() == 0)
                        strBuff += tr("Not available!");
                    else if(NetServer.IP.size()) {
                        if(NetServer.Name.size())
                            strBuff += NetServer.Name;
                        else
                            strBuff += NetServer.IP;
                    }
                    else
                        strBuff += tr("No Avantgarde");
                    Add(new cOsdItem(strBuff.c_str()), IsCurrent(tr(STREAM_SERVER_TEXT)));

                    break;
                case eMcli:
                    if(MultiRoom_)
                    {
                        strBuff = std::string(tr(RECORDING_SERVER_TEXT)) + ":\t";
                        if(ReelBoxes.size() == 0)
                            strBuff += tr("Not available!");
                        else if(NetServer.IP.size())
                        {
                            if(NetServer.Name.size())
                            {
                                if(DatabaseAvailable_)
                                    strBuff += NetServer.Name;
                                else
                                    strBuff += std::string("(") + NetServer.Name + ")";
                            }
                            else
                                strBuff += NetServer.IP;
                        }
                        else
                            strBuff += tr("No Avantgarde");
                        Add(new cOsdItem(strBuff.c_str()), IsCurrent(tr(RECORDING_SERVER_TEXT)));
                    }

                    break;
            }
        }

        if(Get(current))
            SetCurrent(Get(current));
        else
            SetCurrent(Get(0));

        currText = Get(Current()) && Get(Current())->Text()?std::string(Get(Current())->Text()):"";

        if (currText.size())
            Client_AddInfo();

        SetHelp(MultiRoom_?tr("Search"):NULL, NULL, TunerSettings_?tr("Normal"):tr("Tuner"), ExpertSettings_?tr("Normal"):tr("Expert"));
    }

    Display();
}

void cMultiRoomSetupMenu::Client_AddInfo()
{
    //Adds info depending on the current item
    //printf("%s %s\n",__PRETTY_FUNCTION__, currText.c_str());

    char strBuff[512];
    int count = Count();
    int max_items = displayMenu->MaxItems();
    int num_blank_lines = max_items - count - 7;
    while (--num_blank_lines > 0) Add(new cOsdItem("",osUnknown, false));

    Add(new cOsdItem("____________________________________________________________",osUnknown, false));
    Add(new cOsdItem("",osUnknown, false));

    if(IsCurrent(tr(MULTI_ROOM_STR)))
    {
        if(MultiRoom_ == MultiRoom_old_)
        { // ClientMode unchanged
            if(!MultiRoom_) { // MultiRoom is disabled
                if(HasHarddisk_) {
                    Add(new cOsdItem(tr("Info:"),osUnknown, false));
                    AddFloatingText(tr("MultiRoom is inactive."), MAX_LETTER_PER_LINE);
                    AddFloatingText(tr("Connection to ReelBox Avantgarde is disabled. Recording will be done locally."), MAX_LETTER_PER_LINE);
                } else {
                    Add(new cOsdItem(tr("Warning:"),osUnknown, false));
                    AddFloatingText(tr("MultiRoom is disabled and no harddisk was found - only LiveTV is available!"), MAX_LETTER_PER_LINE);
                    AddFloatingText(tr("Recording and timeshift are disabled."), MAX_LETTER_PER_LINE);
                }
            } else { // MultiRoom is enabled
                if(DatabaseAvailable_) {
                    Add(new cOsdItem(tr("Info:"),osUnknown, false));
                    AddFloatingText(tr("MultiRoom is active. Recording will be done by the ReelBox Avantgarde."), MAX_LETTER_PER_LINE);
                } else {
                    //Add(new cOsdItem(tr("Error!"),osUnknown, false));
                    AddFloatingText(tr("Error: The ReelBox Avantgarde is not accessible now.\n"
                                "Possible reasons are: network hardware error, wrong network settings, Avantgarde is off.\n"
                                "Please check the system settings and the network or select a different recording server."), MAX_LETTER_PER_LINE);
                }
            }
        }
        else
        { // ClientMode modified
            if(!MultiRoom_) { // MultiRoom will be disabled
                if(HasHarddisk_) {
                    Add(new cOsdItem(tr("Info:"),osUnknown, false));
                    AddFloatingText(tr("If confirm now, MultiRoom will be deactivated."), MAX_LETTER_PER_LINE);
                    AddFloatingText(tr("Connection to the ReelBox Avantgarde will be disabled. Recording will be done locally."), MAX_LETTER_PER_LINE);
                } else {
                    Add(new cOsdItem(tr("Warning:"),osUnknown, false));
                    AddFloatingText(tr("If confirm now, MultiRoom will be disabled. No harddisk was found - only LiveTV will be available."), MAX_LETTER_PER_LINE);
                    AddFloatingText(tr("Recording and timeshift will be disabled."), MAX_LETTER_PER_LINE);
                }
            } else { // MultiRoom will be enabled
                if(DatabaseAvailable_) {
                    Add(new cOsdItem(tr("Info:"),osUnknown, false));
                    AddFloatingText(tr("If confirm now, MultiRoom will be activated. Recording will be done by the ReelBox Avantgarde."), MAX_LETTER_PER_LINE);
                } else {
                    //Add(new cOsdItem(tr("Error!"),osUnknown, false));
                    AddFloatingText(tr("Error: The ReelBox Avantgarde is not accessible now.\n"
                                "Possible reasons are: network hardware error, wrong network settings, Avantgarde is off.\n"
                                "Please check the system settings and the network or select a different recording server."), MAX_LETTER_PER_LINE);
                }
            }
        }
    }
    else if(IsCurrent(tr(MR_MODE_STR)))
    {
        if(NetServer.IP.size()) {
            Add(new cOsdItem(tr("Info:"),osUnknown, false));
            snprintf(strBuff, 512, tr("The ReelBox Avantgarde with the IP address \"%s\" will be used for MultiRoom services."), NetServer.IP.c_str());
            AddFloatingText(strBuff, MAX_LETTER_PER_LINE);
        } else
            AddFloatingText(tr("Please select a ReelBox Avantgarde."), MAX_LETTER_PER_LINE);
    } else if(IsCurrent(tr(RECEIVER_TYPE_TEXT))) {
        if(ReceiverType_ == eMcli)
            AddFloatingText(tr("DummyText RECEIVER_TYPE_TEXT MCLI$"), MAX_LETTER_PER_LINE);
        else if(ReceiverType_ == eStreamdev)
            AddFloatingText(tr("DummyText RECEIVER_TYPE_TEXT Streamdev$"), MAX_LETTER_PER_LINE);
    } else if(IsCurrent(tr(STREAM_SERVER_TEXT)) || IsCurrent(tr(RECORDING_SERVER_TEXT))) {
        if(ReelBoxes.size() && !DatabaseAvailable_) {
            //Add(new cOsdItem(tr("Error!"),osUnknown, false));
            AddFloatingText(tr("Error: The ReelBox Avantgarde is not accessible now.\n"
                                   "Possible reasons are: network hardware error, wrong network settings, Avantgarde is off.\n"
                                   "Please check the system settings and the network or select a different recording server."), MAX_LETTER_PER_LINE);
            Add(new cOsdItem("",osUnknown, false));
        }
        if(ReelBoxes.size()==0) {
            Add(new cOsdItem(tr("Error!"),osUnknown, false));
            AddFloatingText(tr("Please check the network settings and the network connection else please search in network."), MAX_LETTER_PER_LINE);
        } else {
            if(NetServer.IP.size()) {
                Add(new cOsdItem(tr("Info:"),osUnknown, false));
                snprintf(strBuff, 512, tr("The ReelBox Avantgarde with the IP address \"%s\" will be used for MultiRoom services."), NetServer.IP.c_str());
                AddFloatingText(strBuff, MAX_LETTER_PER_LINE);
            } else
                AddFloatingText(tr("Please select a ReelBox Avantgarde."), MAX_LETTER_PER_LINE);
        }
    }
}

bool cMultiRoomSetupMenu::AreSettingsModified()
{
    // MultiRoom
    if((MultiRoom_!=MultiRoom_old_) || (MultiRoomMode_!=MultiRoomMode_old_) || (ReceiverType_!=ReceiverTypeOld_))
        return true;

    // Network
    if((NC_Bridge_!=NC_Bridge_old_) || (Bridge_!=Bridge_old_) || (Vlan_!=Vlan_old_) || (VlanID_!=VlanID_old_))
        return true;

    // Tuners
    if((nrTuners.sat!=nrTuners_old_.sat) || (nrTuners.satS2!=nrTuners_old_.satS2) || (nrTuners.terr!=nrTuners_old_.terr) || (nrTuners.cable!=nrTuners_old_.cable))
        return true;

    return false;
}

bool cMultiRoomSetupMenu::IsCurrent(std::string text)
{
    if (currText.size())
        return currText.find(text) == 0;
    else
        return text.find(tr(MULTI_ROOM_STR)) == 0;
}

eOSState cMultiRoomSetupMenu::ProcessKey(eKeys key)
{
    switch(MultiRoomMode_)
    {
        case eClient:
            return Client_ProcessKey(key);
            break;
        case eServer:
            if(ExpertSettings_ || TunerSettings_)
                return ServerExpert_ProcessKey(key);
            else
                return Server_ProcessKey(key);
            break;
    }
    return osUnknown; // This must not happen!
}

eOSState cMultiRoomSetupMenu::Server_ProcessKey(eKeys key)
{
#ifdef USEMYSQL
    // If Installation of MySQL is running, do nothing else
    if(!HasSubMenu() && cPluginSetup::MysqlInstallThread_)
    {
        if(cPluginSetup::MysqlInstallThread_->Active())
        {
            if((cPluginSetup::MysqlInstallThread_->State_ > 0) && (cPluginSetup::MysqlInstallThread_->State_ < 100))
            {
                // Display progress
                char strBuff[256];
                snprintf(strBuff, 256, "%s (%i%%)", tr("Initializing database..."), cPluginSetup::MysqlInstallThread_->State_);
                Skins.Message(mtStatus, strBuff);
            }
            return osContinue;
        }
        else
        {
            if(cPluginSetup::MysqlInstallThread_->State_ == 100)
            {
                cPluginSetup::MysqlInstallThread_->State_ = 0; // Inactive

                // Jump to first line
                SetCurrent(Get(0));
                currText = Get(Current()) && Get(Current())->Text()?std::string(Get(Current())->Text()):"";
                Set(); // Redraw Menu

                return osContinue;
            }
            if(cPluginSetup::MysqlInstallThread_->State_ < 0) // Error!
            {
                cPluginSetup::MysqlInstallThread_->State_ = 0; // Inactive
                // Disable Multiroom
                MultiRoom_old_ = MultiRoom_ = 0;
                Setup.ReelboxMode = Setup.ReelboxModeTemp = eModeStandalone;
                Setup.Save();

                // Disable Bridge
                cSysConfig_vdr &sysconfig = cSysConfig_vdr::GetInstance();
                sysconfig.Load("/etc/default/sysconfig");
                sysconfig.SetVariable("NC_BRIDGE", "no");
                sysconfig.SetVariable("VLAN", "no");
                sysconfig.Save();

                // Redraw Menu
                Set();
                return osContinue;
            }
        }
    }
#endif


    bool hadSubMenu = HasSubMenu();
    int MultiRoom_prev = MultiRoom_;
    int AvgLiveTv_prev = AvgLiveTv;
    int maxClientNum_prev = maxClientNum;
    int ConnectionType_prev = ConnectionType_;
    if(!HasSubMenu() && IsCurrent(tr(CONNECTION_TYPE_STR)) && (maxClientNum>1) && ((key==kRight) || (key==kLeft)))
        return osContinue;

    eOSState state = cOsdMenu::ProcessKey(key);

    if(!hadSubMenu)
    {
        if(state == osUnknown)
        {
            switch (key)
            {
//            case kYellow: // Fix Database
//                if(DatabaseState_)
//                    return AddSubMenu(new cDatabaseDoctorMenu(DatabaseState_));
//                break;

                case kYellow:
                    TunerSettings_ = true;
                    ServerExpert_Set();
                    return osContinue;
                    break;

                case kBlue: // expert
                    if(MultiRoom_)
                    {
                        ExpertSettings_ = true;
                        ServerExpert_Set();
                        return osContinue;
                    }
                    else
                        return state;
                    break;

                case kOk: // Save
                    Save();
                    break;

                default:
                    break;
            }
        }
        else if(state == osBack)
        {
            if(AreSettingsModified() && (Interface->Confirm(tr("Save changes?"))))
            {
                Skins.Message(mtStatus, tr("Storing settings - please wait..."));
                Save();
                Skins.Message(mtInfo, tr("Settings stored"));
            }
            return state;
        }
    }

    if(hadSubMenu == !HasSubMenu())
        SetCurrent(Get(0)); // Jump to first line

    currText = Get(Current()) && Get(Current())->Text()?std::string(Get(Current())->Text()):"";
        if (!HasSubMenu())
    if ((hadSubMenu&&!HasSubMenu()) // If submenu is closed update this menu
            || MultiRoom_ != MultiRoom_prev
            || AvgLiveTv_prev != AvgLiveTv || maxClientNum != maxClientNum_prev
            || ConnectionType_prev != ConnectionType_
            || NORMALKEY(key) == kUp || NORMALKEY(key) == kDown
            || NORMALKEY(key) == kRight || NORMALKEY(key) == kLeft)
        Set(); // changed

//  printf("Multiroom settings: %i %i %i\n", actAsServer, maxClientNum, AvgLiveTv);

    return state;
}

eOSState cMultiRoomSetupMenu::ServerExpert_ProcessKey(eKeys Key)
{
    int MultiRoomMode_prev = MultiRoomMode_;
    int NC_Bridge_prev = NC_Bridge_;
    int Vlan_prev = Vlan_;

    eOSState state = cOsdMenu::ProcessKey(Key);

    if(!HasSubMenu())
    {
        if(state==osBack)
        {
            ExpertSettings_ = false;
            TunerSettings_ = false;
            Server_Set();
            return osContinue;
        }
        switch(Key)
        {
            case kOk:
                Save();
                return osContinue;
            case kLeft:
            case kRight:
                if((NC_Bridge_ != NC_Bridge_prev) || (Vlan_ != Vlan_prev))
                    ServerExpert_Set();
                else if(MultiRoomMode_ != MultiRoomMode_prev)
                {
                    Set();
                    return osContinue;
                }
                break;
            case kUp:
            case kUp|k_Repeat:
            case kDown:
            case kDown|k_Repeat:
                ServerExpert_Set();
                break;
            case kBlue:
                if(MultiRoom_)
                {
                    if(ExpertSettings_)
                        ExpertSettings_ = false;
                    else
                    {
                        ExpertSettings_ = true;
                        TunerSettings_ = false;
                    }
                    Set();
                }
                return osContinue;
            case kYellow:
                if(TunerSettings_)
                    TunerSettings_ = false;
                else
                {
                    TunerSettings_ = true;
                    ExpertSettings_ = false;
                }
                Set();
                return osContinue;
            default:
                return state;
        }
    }
    return state;
}

eOSState cMultiRoomSetupMenu::Client_ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);
    if(!HasSubMenu())
    {
        // Check if DB is available
        if(Setup.ReelboxMode && !Setup.ReelboxModeTemp)
            DatabaseAvailable_ = false;
        else
            DatabaseAvailable_ = true;

        currText = Get(Current()) && Get(Current())->Text()?std::string(Get(Current())->Text()):"";
        if(state == osUnknown) {
            if(Key == kRed)
            {
                if(MultiRoom_)
                    return AddSubMenu(new cSetupNetClientServerMenu(GetTitle(), &NetServer));
            }
            else if(Key == kBlue)
            {
                if(ExpertSettings_)
                    ExpertSettings_ = false;
                else
                {
                    ExpertSettings_ = true;
                    TunerSettings_ = false;
                }
                Set();
                currText = Get(Current()) && Get(Current())->Text()?std::string(Get(Current())->Text()):"";
                state = osContinue;
            }
            else if(Key == kYellow)
            {
                if(TunerSettings_)
                    TunerSettings_ = false;
                else
                {
                    TunerSettings_ = true;
                    ExpertSettings_ = false;
                }
                Set();
                return osContinue;
            }
            else if(Key == kOk)
            {
                if(NetServer.IP.size() || !MultiRoom_)
                {
                    Skins.Message(mtStatus, tr("Storing settings - please wait..."));
                    Client_Save();
                    Skins.Message(mtInfo, tr("Settings stored"));
                    Set();
                }
                else
                    Skins.Message(mtError, tr("Invalid settings: Client without Server!"));
                return osContinue;
            }

        }
        else if(state == os_User)
        { // Refresh menu
            Set();
            return osContinue;
        }
        else if(state == osBack)
        {
            if(ExpertSettings_ || TunerSettings_)
            {
                ExpertSettings_ = false;
                TunerSettings_ = false;
                Set();
                return osContinue;
            }

            if(AreSettingsModified())
            {
                if(NetServer.IP.size())
                {
                    if(Interface->Confirm(tr("Save changes?")))
                    {
                        Skins.Message(mtStatus, tr("Storing settings - please wait..."));
                        Client_Save();
                        Skins.Message(mtInfo, tr("Settings stored"));
                    }
                }
            }
        }

        if((Key == kUp) || (Key == kDown) || (Key == (kUp|k_Repeat)) || (Key == (kDown|k_Repeat) || (DatabaseAvailable_ != DatabaseAvailableOld_)))
            Set(); // Call AddInfo()
    }
    else if(state == osUser1)
    { // Close Submenu and refresh menu
        if(NetServer.IP.size())
        {
            Skins.Message(mtStatus, tr("Storing settings - please wait..."));
            Client_Save();
            Skins.Message(mtInfo, tr("Settings stored"));
        }
        CloseSubMenu();
        Set();
        Display();
        AVGChanged_ = true;
        return osContinue;
    }

    return state;
}

eOSState cMultiRoomSetupMenu::Save()
{
    DPRINT("\033[0;41m %s(%i): %s \033[0m\n", __FILE__, __LINE__, __PRETTY_FUNCTION__);
    if(MultiRoomMode_old_ == eClient)
    {
        // if somebody want to disable his "Client" within server-menu then use Client_Save() for disabling Client-Mode
        Client_Save();
        return osContinue;
    }

    if(Timers.BeingEdited())
        Skins.Message(mtWarning, tr("Some Timer is being edited, cannot save settings."));
    else
    {
        if(!OnlineCheck())
        {
            Skins.Message(mtError, tr("Internet connection not available!"));
            return osContinue;
        }
        if (Server_Save())
        {
#ifdef USEMYSQL
            // is DB being installed, then don't display "Changes saved"
            if(!(cPluginSetup::MysqlInstallThread_ && cPluginSetup::MysqlInstallThread_->Active()))
#endif
            {
                Skins.Message(mtInfo, tr("Changes saved"));
                // Jump to first line
                SetCurrent(Get(0));
                currText = Get(Current()) && Get(Current())->Text()?std::string(Get(Current())->Text()):"";
                Set(); // Redraw
            }
        }
    }
    return osContinue;
}

int cMultiRoomSetupMenu::Server_Save()
{
    DPRINT("\033[0;41m %s(%i): %s \033[0m\n", __FILE__, __LINE__, __PRETTY_FUNCTION__);
    cSysConfig_vdr &sysconfig = cSysConfig_vdr::GetInstance();
    sysconfig.Load("/etc/default/sysconfig");

    bool keepMysqlServer = false;
    if(MultiRoom_ && !MultiRoom_old_)
        keepMysqlServer = !SystemExec("pidof mysqld");
    else if(!MultiRoom_ && MultiRoom_old_)
    {
        if(sysconfig.GetVariable("MYSQLD_KEEP_RUNNING"))
            keepMysqlServer = !strcmp(sysconfig.GetVariable("MYSQLD_KEEP_RUNNING"), "yes");
        else
            keepMysqlServer = false; // default
    }

    Skins.Message(mtStatus, tr("Saving Settings..."));

    if(MultiRoom_ && MultiRoom_old_)
    {
        // Check if Database is ok
        if(AvgDatabaseCheck(true))
            return 0;
    }

    cPlugin *p = cPluginManager::GetPlugin("streamdev-server");

    if (p)
    {
        // send data to streamdev server
        p->SetupStore("MaxClients", maxClientNum);
        p->SetupStore("StartServer", MultiRoom_);

        std::stringstream buffer;
        buffer << maxClientNum;
        p->SetupParse("MaxClients", buffer.str().c_str());

        buffer.str("");
        buffer << MultiRoom_;
        p->SetupParse("StartServer", buffer.str().c_str());

        // restart streamdev-server
        p->Service("Streamdev Server Reinit", 0);
    }

    Setup.MaxMultiRoomClients = maxClientNum;

    bool onlySatS2 = !nrTunersPhys_.terr && !nrTunersPhys_.sat && !nrTunersPhys_.cable && nrTunersPhys_.satS2;
    bool onlySat = !nrTunersPhys_.terr && nrTunersPhys_.sat && !nrTunersPhys_.cable && !nrTunersPhys_.satS2;
    bool onlyTerr = nrTunersPhys_.terr && !nrTunersPhys_.sat && !nrTunersPhys_.cable && !nrTunersPhys_.satS2;
    bool onlyCable = !nrTunersPhys_.terr && !nrTunersPhys_.sat && nrTunersPhys_.cable && !nrTunersPhys_.satS2;
    int bNoToSK = -1;

    bool TunerSettingsModified = true;
    if(nrTuners_old_.satS2 == nrTuners.satS2 && nrTuners_old_.sat == nrTuners.sat && nrTuners_old_.terr == nrTuners.terr && nrTuners_old_.cable == nrTuners.cable)
        TunerSettingsModified = false;

    if(MultiRoom_ != MultiRoom_old_ || maxClientNum != oldMaxClientNum || TunerSettingsModified)
    {
        if(MultiRoom_)
        {
            // if something was changed, then don't autoset tunersettings
            if(!TunerSettingsModified)
            {
                if (!MixedTuners)
                { // simple case: no mixed tuners
                    if (onlySatS2)  // reserve up to the half of the tuners to the clients
                        nrTuners.satS2 = std::max(std::max(nrTunersPhys_.satS2-maxClientNum, nrTunersPhys_.satS2/2),1);
                    else if (onlySat)
                        nrTuners.sat = std::max(std::max(nrTunersPhys_.sat-maxClientNum, nrTunersPhys_.sat/2),1);
                    else if (onlyCable)
                        nrTuners.cable = std::max(std::max(nrTunersPhys_.cable-maxClientNum, nrTunersPhys_.cable/2),1);
                    else if (onlyTerr)
                        nrTuners.terr = std::max(std::max(nrTunersPhys_.terr-maxClientNum, nrTunersPhys_.terr/2),1);
                }
                else
                { // mixed tuners
                    nrTuners.sat = nrTunersPhys_.sat;
                    nrTuners.satS2 = nrTunersPhys_.satS2;
                    nrTuners.terr = nrTunersPhys_.terr;
                    nrTuners.cable = nrTunersPhys_.cable;
                    /*int*/ bNoToSK = std::max(std::max(std::max(nrTuners.terr, nrTuners.cable), nrTuners.sat), nrTuners.satS2); //biggestNumberOfTunersOfSameKind;

                    int *ptrMax = &nrTuners.terr;
                    if (nrTuners.cable > *ptrMax) ptrMax = &nrTuners.cable;
                    if (nrTuners.sat > *ptrMax) ptrMax = &nrTuners.sat;
                    if (nrTuners.satS2 > *ptrMax) ptrMax = &nrTuners.satS2;
                    if (*ptrMax-maxClientNum > 1) // the max. number of tuners of one kind is big enough for the clients
                        *ptrMax = *ptrMax - maxClientNum;
                    else {
                        int numToDec = maxClientNum;
                        while (numToDec > 0 && bNoToSK > 1) { // as long as there are >=2 tuners of the same kind, reserve 1
                            bNoToSK = std::max(std::max(std::max(nrTuners.terr, nrTuners.cable), nrTuners.sat), nrTuners.satS2); //biggestNumberOfTunersOfSameKind;
                            ptrMax = &nrTuners.terr;
                            if (nrTuners.cable > *ptrMax) ptrMax = &nrTuners.cable;
                            if (nrTuners.sat > *ptrMax) ptrMax = &nrTuners.sat;
                            if (nrTuners.satS2 > *ptrMax) ptrMax = &nrTuners.satS2;
                            *ptrMax = *ptrMax - 1;
                            numToDec--;
                        }
                        //printf("XXXXX: s: %i s2: %i, c: %i, t: %i, mcn: %i\n", nrTuners.sat, nrTuners.satS2, nrTuners.cable, nrTuners.terr, maxClientNum);
                    }
                }
            }
            StoreMcliConf(&nrTuners, eMcli);
        }
        else
        {
            if(TunerSettingsModified)
                StoreMcliConf(&nrTuners, eMcli);
            else
            {
                nrTuners_t tmpnrTuners;
                tmpnrTuners.terr = 6;
                tmpnrTuners.sat = 6;
                tmpnrTuners.satS2 = 6;
                tmpnrTuners.cable = 6;
                StoreMcliConf(&tmpnrTuners, eMcli);
            }
        }
    }

    // In tuner settings menu, do not save any of the multiroom settings
    // only save the tuners settings
    if (TunerSettings_)
    {
        printf("%s:%d In Tunersettings menu. change only the tuner configs\n", __FILE__, __LINE__);
	//if (AreSettingsModified)
        return 1;
    }

    //printf("XXXXXXXXXX: maxClientNum: %i bNOTOSK: %i ", maxClientNum, bNoToSK);
    //printf("T: %i C: %i S: %i S2: %i \n", nrTuners.terr, nrTuners.cable, nrTuners.sat, nrTuners.satS2);

    //liveTV on Avg if  multiroom off OR multiroom on and livetv
    Setup.LiveTvOnAvg = !MultiRoom_ || (MultiRoom_ && AvgLiveTv);
    if(MultiRoom_ && !AvgLiveTv)
        Setup.LiveBuffer = 0;

    if(MultiRoom_)
    {
        Setup.StandbyOrQuickshutdown = 1; // Set Shutdownmode to QuickShutdown
        Setup.ReelboxMode = Setup.ReelboxModeTemp = eModeServer; // Server-Mode
    }
    else
    {
        Setup.StandbyOrQuickshutdown = 0; // Set Shutdownmode to normal Standby
        Setup.ReelboxMode = Setup.ReelboxModeTemp = eModeStandalone; // StandAlone-Mode
        SystemExec("avahi-publish-avg-mysql.sh -2"); // -2 means kill avahi-publish
    }

    Setup.Save();
    if(MultiRoom_)
    {
        if(!MultiRoom_old_)
            sysconfig.SetVariable("MYSQLD_KEEP_RUNNING", keepMysqlServer?"yes":"no");
        // Store Network settings
        char strBuff[16];
        sysconfig.SetVariable("WOL_ENABLE", "yes");
        sysconfig.SetVariable("NC_CONNECTION_TYPE", ConnectionType_?"network":"direct");
        if(NC_Bridge_)
        {
            sysconfig.SetVariable("NC_BRIDGE", "yes");
            snprintf(strBuff, 16, "Ethernet %i", Bridge_+1);
            sysconfig.SetVariable("ETH0_BRIDGE", strBuff);

            if(Vlan_)
            {
                sysconfig.SetVariable("VLAN", "yes");
                sysconfig.SetVariable("VLAN_ID", itoa(VlanID_));
            }
            else
                sysconfig.SetVariable("VLAN", "no");

        }
        else
        {
            sysconfig.SetVariable("NC_BRIDGE", "no");
            sysconfig.SetVariable("VLAN", "no");
        }

        if (!IsBridgeRequired())
        {
            // do not use bridge, but create vlan on eth1?
            if (IsNetworkDevicePresent("eth1"))
                sysconfig.SetVariable("ETH0_BRIDGE", "Ethernet 1");
            else
                sysconfig.SetVariable("ETH0_BRIDGE", "-");

            if(Vlan_)
            {
                sysconfig.SetVariable("VLAN", "yes");
                sysconfig.SetVariable("VLAN_ID", itoa(VlanID_));
            }
            else
                sysconfig.SetVariable("VLAN", "no");
        }
    }
    else
    {
        // Disable NC-Bridge & VLAN
        sysconfig.SetVariable("NC_BRIDGE", "no");
        sysconfig.SetVariable("VLAN", "no");
    }
    sysconfig.Save();
#ifdef USEMYSQL
    //Start/Stop/Install MySQL if needed
    if(MultiRoom_ && !MultiRoom_old_)
    {
        if(!SystemExec("mysqld --version"))
        {
            //printf("\033[0;41m %s(%i): %s \033[0m\n", __FILE__, __LINE__, "enabling MySQL");
            // Create symlinks in /etc/rcX.d/ and start MySQL-Server
            SystemExec("update-rc.d mysql start 19 2 3 4 5 . stop 21 0 1 6 .");
            SystemExec("/etc/init.d/mysql start");
        }

        cPluginSetup::MysqlInstallThread_->Start();
    }
    else if(!MultiRoom_ && MultiRoom_old_)
    {
        // Save Timers to file
        Timers.Save();
        // Disable MultiRoom-features in other plugins
        cPluginManager::CallAllServices("MultiRoom disable", NULL);
        sleep(3); // TODO: Test if we really need this (test if NetClients takes note about disabling MultiRoom of AVG *before*(important!) restarting network)
        // Restart Network
        Skins.Message(mtStatus, tr("Restarting network..."));
        SystemExec("setup-net restart");

        if(!keepMysqlServer)
        {
            //printf("\033[0;41m %s(%i): %s \033[0m\n", __FILE__, __LINE__, "disabling MySQL");
            // Remove symlinks in /etc/rcX.d/ and stop MySQL-Server
            SystemExec("/etc/init.d/mysql stop");
            SystemExec("update-rc.d -f mysql remove");
        }
    }
#else
    if(MultiRoom_ != MultiRoom_old_)
    {
        Skins.Message(mtStatus, tr("Restarting network..."));
        SystemExec("setup-net restart");
    }
#endif

    MultiRoom_old_ = MultiRoom_;
    MultiRoomMode_old_ = MultiRoomMode_;
    return 1;
}

bool cMultiRoomSetupMenu::Client_Save()
{
    DPRINT("\033[0;41m %s(%i): %s \033[0m\n", __FILE__, __LINE__, __PRETTY_FUNCTION__);
    cSysConfig_vdr &sysconfig = cSysConfig_vdr::GetInstance();
    sysconfig.Load("/etc/default/sysconfig");
    bool save=true;

    if(!MultiRoom_)
    {
        Setup.ReelboxMode = Setup.ReelboxModeTemp = eModeStandalone; // 0=Standalone => use local Harddisk

        if(HasHarddisk_) {
            // Set UUID for Recording Media
            std::string UUID = GetUUID();
            if(UUID.size()) {
                // Use local harddisk
                sysconfig.SetVariable("MEDIADEVICE", UUID.c_str());
                sysconfig.SetVariable("MEDIADEVICE_ONLY_RECORDING", "no");
            } else {
                Skins.Message(mtError, "Storing failed! (missing UUID)");
                save = false;
            }
        } else {
            sysconfig.SetVariable("MEDIADEVICE", "");
            sysconfig.SetVariable("MEDIADEVICE_ONLY_RECORDING", "no");
        }
    }
    else
    {
        Setup.ReelboxMode = eModeClient; // 1=Client => use Database

        //NetServerIP
        if(NetServer.IP.size())
            strcpy(Setup.NetServerIP, NetServer.IP.c_str());
        else
            strcpy(Setup.NetServerIP, "");
        //NetServerName
        if(NetServer.Name.size())
            strcpy(Setup.NetServerName, NetServer.Name.c_str());
        else
            strcpy(Setup.NetServerName, "");

        //NetServerMAC
        if(GetNetserverMAC(&NetServer))
            strcpy(Setup.NetServerMAC, NetServer.MAC.c_str());
        else
            strcpy(Setup.NetServerMAC, "");

        // not NetServerIP must be an error
        if (NetServer.IP.empty())
        {
            esyslog("ERROR (%s:%d) Could not save settings. NetServerIP is empty!", __FILE__, __LINE__);
            return false;
        }

        std::string cmd = std::string("getConfigsFromAVGServer.sh ") + Setup.NetServerIP + std::string(" sysconfig ; ");
        SystemExec(cmd.c_str());
        bool hasNasAsMediaDir = false;
        parseAVGsSysconfig(&hasNasAsMediaDir);

        DPRINT("\033[0;92m Storing recordings on the AVG! \033[0;m\n");
        char strBuff[512];
        //int Number = GetFreeMountNumber();
        int Number = 0; // use #0 (reserved for NetClient-AVG-Mount)

        if (!hasNasAsMediaDir) {
            if((Number != -1) && NetServer.IP.size()) {
                // Store the NFS-Shares
                sprintf(strBuff, "MOUNT%i_NFS_STARTCLIENT", Number);
                sysconfig.SetVariable(strBuff, "yes");
                sprintf(strBuff, "MOUNT%i_NFS_HOST", Number);
                sysconfig.SetVariable(strBuff, NetServer.IP.c_str());
                sprintf(strBuff, "MOUNT%i_NFS_SHARENAME", Number);
                sysconfig.SetVariable(strBuff, "/media/hd");
                sprintf(strBuff, "MOUNT%i_NFS_DISPLAYNAME", Number);
                sysconfig.SetVariable(strBuff, "Avantgarde");
                sprintf(strBuff, "MOUNT%i_NFS_PICTURES", Number);
                sysconfig.SetVariable(strBuff, "yes");
                sprintf(strBuff, "MOUNT%i_NFS_MUSIC", Number);
                sysconfig.SetVariable(strBuff, "yes");
                sprintf(strBuff, "MOUNT%i_NFS_RECORDINGS", Number);
                sysconfig.SetVariable(strBuff, "yes");
                sprintf(strBuff, "MOUNT%i_NFS_VIDEOS", Number);
                sysconfig.SetVariable(strBuff, "yes");

                // NFS-specified options
                sprintf(strBuff, "MOUNT%i_NFS_NETWORKPROTOCOL", Number);
                sysconfig.SetVariable(strBuff, "tcp");
                sprintf(strBuff, "MOUNT%i_NFS_BLOCKSIZE", Number);
                sysconfig.SetVariable(strBuff, "8192");
                sprintf(strBuff, "MOUNT%i_NFS_OPTIONHS", Number);
                sysconfig.SetVariable(strBuff, "soft");
                sprintf(strBuff, "MOUNT%i_NFS_OPTIONLOCK", Number);
                sysconfig.SetVariable(strBuff, "yes");
                sprintf(strBuff, "MOUNT%i_NFS_OPTIONRW", Number);
                sysconfig.SetVariable(strBuff, "yes");
                sprintf(strBuff, "MOUNT%i_NFS_NFSVERSION", Number);
                sysconfig.SetVariable(strBuff, "3");

                // Use NFS-Mount as MediaDevice
                sprintf(strBuff, "/media/.mount%i", Number);
                sysconfig.SetVariable("MEDIADEVICE", strBuff);
                sysconfig.SetVariable("MEDIADEVICE_ONLY_RECORDING", "no");

                sysconfig.Save();

                sprintf(strBuff, "setup-shares start %i &", Number);
                SystemExec(strBuff);
            } else {
                // Use NFS-Mount as MediaDevice
                sprintf(strBuff, "/media/.mount%i", Number);
                sysconfig.SetVariable("MEDIADEVICE", strBuff);
                sysconfig.SetVariable("MEDIADEVICE_ONLY_RECORDING", "no");
                //snprintf(strBuff, 512, "/media/.mount0/recordings");
                //VideoDirectory = strdup(strBuff);
                sysconfig.Save();
            }
        }
        SystemExec("setup-mediadir");
        // Recordings.Update(false);
    }

    //NetServerIP
    if(NetServer.IP.size())
        strcpy(Setup.NetServerIP, NetServer.IP.c_str());
    else
        strcpy(Setup.NetServerIP, "");
    //NetServerName
    if(NetServer.Name.size())
        strcpy(Setup.NetServerName, NetServer.Name.c_str());
    else
        strcpy(Setup.NetServerName, "");

    //NetServerMAC
    if(GetNetserverMAC(&NetServer))
        strcpy(Setup.NetServerMAC, NetServer.MAC.c_str());
    else
        strcpy(Setup.NetServerMAC, "");

    Setup.Save();

    if(MultiRoom_)
    {
        //check and set ReelboxModeTemp
        bool DatabaseAvailable = false;
        cPlugin *p = cPluginManager::GetPlugin("avahi");
        if(p)
            p->Service("Avahi check NetServer", &DatabaseAvailable);
        Setup.ReelboxModeTemp = DatabaseAvailable?eModeClient:eModeStandalone;
    }

    // Load Timers if necessary
    if(MultiRoom_)
    {
        Timers.Load(); // Clear Timers
#ifdef USEMYSQL
        cPluginManager::CallAllServices("MultiRoom disable");
        cPluginManager::CallAllServices("MultiRoom enable");
        Timers.LoadDB();
#endif
    } else if(!MultiRoom_)
    {
        if(MultiRoom_old_ != MultiRoom_)
            Timers.Load(TIMERS_PATH);
    }

    if(ReceiverType_ == eStreamdev) {
        //Streamdev-Client settings
        if(NetServer.IP.size()) {
            DPRINT("\033[0;92m Use NetServer! \033[0;m\n");
            // Setup Streamdev-Client
            if (cPlugin *StreamdevPlugin = cPluginManager::GetPlugin("streamdev-client")) {
                StreamdevPlugin->SetupStore("RemoteIp", NetServer.IP.c_str());
                StreamdevPlugin->SetupParse("RemoteIp", NetServer.IP.c_str());
                StreamdevPlugin->SetupStore("StartClient", 1);
                StreamdevPlugin->SetupParse("StartClient", "1");
                StreamdevPlugin->SetupStore("SyncEPG", 1);
                StreamdevPlugin->SetupParse("SyncEPG", "1");
                StreamdevPlugin->Service("Streamdev Client Reinit", NULL);
            }
        } else {
            DPRINT("\033[0;92m No NetServer! \033[0;m\n");
            strcpy(Setup.NetServerIP, "");
            strcpy(Setup.NetServerName, "");
            strcpy(Setup.NetServerMAC, "");

            // Disable Streamdev-Client
            if (cPlugin *StreamdevPlugin = cPluginManager::GetPlugin("streamdev-client")) {
                StreamdevPlugin->SetupStore("RemoteIp", "");
                StreamdevPlugin->SetupParse("RemoteIp", "");
                StreamdevPlugin->SetupStore("StartClient", 0);
                StreamdevPlugin->SetupParse("StartClient", "0");
                StreamdevPlugin->Service("Streamdev Client Reinit", NULL);
            }
        }
    } else if(ReceiverType_ == eMcli) {
        // Disable Streamdev-Client
        if (cPlugin *StreamdevPlugin = cPluginManager::GetPlugin("streamdev-client")) {
            StreamdevPlugin->SetupStore("RemoteIp", "");
            StreamdevPlugin->SetupParse("RemoteIp", "");
            StreamdevPlugin->SetupStore("StartClient", 0);
            StreamdevPlugin->SetupParse("StartClient", "0");
            StreamdevPlugin->Service("Streamdev Client Reinit", NULL);
        }
    }

    StoreMcliConf(&nrTuners, ReceiverType_); // Write settings to /etc/default/mcli

    if(save)
    {
        AVGChanged_ = false;
        MultiRoom_old_ = MultiRoom_;
        MultiRoomMode_old_ = MultiRoomMode_;
        ReceiverTypeOld_ = ReceiverType_;
        nrTuners_old_.sat   = nrTuners.sat;
        nrTuners_old_.satS2 = nrTuners.satS2;
        nrTuners_old_.terr  = nrTuners.terr;
        nrTuners_old_.cable = nrTuners.cable;
        sysconfig.Save();
        if(!MultiRoom_)
        {
            // Hint: do this *after* sysconfig.Save()!
            // Stop & Remove NetClient-AVG-Mount
            SystemExec("setup-shares stop 0");
            SystemExec("sed -i \"/^MOUNT0_NFS/d\" /etc/default/sysconfig");
            // TODO: If we use vdr-sysconfig, then do cSysConfig_vdr::Load() here!
            //VideoDirectory = strdup("/media/reel/recordings");
            Recordings.Update(false);
        }
        Setup.Save();
    }
    return save;
}


bool cMultiRoomSetupMenu::OnlineCheck()
{
    if(MultiRoom_ && !MultiRoom_old_ && SystemExec("mysqld --version"))
    {
        cNetworkCheckerThread OnlineChecker;
        OnlineChecker.Start();
        int timeout=40;
        while(!OnlineChecker.HasFinished())
        {
            if(!--timeout)
                return false; // Timeout
            cCondWait::SleepMs(250);
        }
        if(OnlineChecker.IsDnsOK() && OnlineChecker.IsRoutingOK())
            return true; // Connection OK
        else
            return false; // Connection failed
    }
    return true; // No installation needed
}

//##################################################################################
// class cMultiRoomSetupExpertMenu
//##################################################################################
#define NUMBERS_OF_ETHERNET 2

#define NC_BRIDGE_STR trNOOP("Enable NetCeiver bridge")
#define BRIDGE_STR trNOOP("Bridge NetCeiver to")
#define VLAN_STR trNOOP("Use VLAN")
#define VLAN_ID_STR trNOOP("VLAN ID")

//const char *EthernetStrings[NUMBERS_OF_ETHERNET] = { "Ethernet 1", "Ethernet 2" };

cMultiRoomSetupExpertMenu::cMultiRoomSetupExpertMenu(bool *NC_Bridge, int *Bridge,
              int *Vlan, int *VlanID, bool Eth2Available, nrTuners_t *tuners,
              nrTuners_t *tunersPhys, int *nrClients, bool TunerMenu) : cOsdMenu(tr(OSD_TITLE),26)
{
    inTunerMenu = TunerMenu;
    nrTuners = tuners;
    memcpy(&nrTunersOrig, nrTuners, sizeof(struct nrTuners));
    nrTunersPhys = tunersPhys;
    Eth2Available_ = Eth2Available;
    NC_Bridge_ = NC_Bridge;
    NC_Bridge_old_ = NC_Bridge_last_ = *NC_Bridge;
    nrClients_ = nrClients;
    Bridge_ = Bridge;
    Bridge_old_ = *Bridge;
    Vlan_ = Vlan;
    Vlan_old_ = Vlan_last_ = *Vlan;
    VlanID_ = VlanID;
    VlanID_old_ = *VlanID;

    Set();
}

cMultiRoomSetupExpertMenu::~cMultiRoomSetupExpertMenu()
{
}

void cMultiRoomSetupExpertMenu::Set()
{
    int current = Current();
    //printf("\033[0;41m current: %i \033[0m\n", current);
    Clear();

    if (!inTunerMenu) {
        Add(new cMenuEditBoolItem(tr(NC_BRIDGE_STR), (int*)NC_Bridge_, tr("no"), tr("yes")));
        if(*NC_Bridge_) {
            if(Eth2Available_)
                Add(new cMenuEditStraItem(tr(BRIDGE_STR), Bridge_, NUMBERS_OF_ETHERNET, EthernetStrings ));
            Add(new cOsdItem("", osUnknown, false));
            Add(new cMenuEditBoolItem(tr(VLAN_STR), Vlan_, tr("no"), tr("yes")));
            if(*Vlan_)
                Add(new cMenuEditIntItem(tr(VLAN_ID_STR), VlanID_, 1, 9));
        }
        if(current==-1)
            SetCurrent(Get(0));
        else
            SetCurrent(Get(current));
    } else { // inTunerMenu
        Add(new cOsdItem(tr("Number of tuners to use on your Avantgarde:"), osUnknown, false));
        Add(new cOsdItem("", osUnknown, false));
        Add(new cMenuEditIntItem(tr("Satellite (DVB-S)"), &nrTuners->sat, 0, 6));
        Add(new cMenuEditIntItem(tr("Satellite (DVB-S2)"), &nrTuners->satS2, 0, 6));
        Add(new cMenuEditIntItem(tr("Cable"), &nrTuners->cable, 0, 6));
        Add(new cMenuEditIntItem(tr("Terrestrial"), &nrTuners->terr, 0, 6));
        Add(new cOsdItem("____________________________________________________________",osUnknown, false));
        Add(new cOsdItem("", osUnknown, false));
        Add(new cOsdItem(tr("Tuners available for the whole MultiRoom-System:"), osUnknown, false));
        Add(new cOsdItem("", osUnknown, false));
        std::stringstream buf;
        buf << tr("Satellite (DVB-S)") << ":\t " << nrTunersPhys->sat;
        Add(new cOsdItem(buf.str().c_str(), osUnknown, false));
        buf.str("");
        buf << tr("Satellite (DVB-S2)") << ":\t " << nrTunersPhys->satS2;
        Add(new cOsdItem(buf.str().c_str(), osUnknown, false));
        buf.str("");
        buf << tr("Cable") << ":\t " << nrTunersPhys->cable;
        Add(new cOsdItem(buf.str().c_str(), osUnknown, false));
        buf.str("");
        buf << tr("Terrestrial") << ":\t " << nrTunersPhys->terr;
        Add(new cOsdItem(buf.str().c_str(), osUnknown, false));

        if(current == -1 || current == 0)
            SetCurrent(Get(2));
        else
            SetCurrent(Get(current));
    }

    if (!inTunerMenu && Get(Current()) && Get(Current())->Text())
        AddInfo(Get(Current())->Text());
    if (inTunerMenu)
        SetHelp(NULL, NULL, NULL, tr("Normal"));
    else
        SetHelp(tr("Tuner"), NULL, NULL, tr("Normal"));
    Display();
}

void cMultiRoomSetupExpertMenu::AddInfo(std::string currText)
{
    //Adds info depending on the current item
    //printf("%s %s\n",__PRETTY_FUNCTION__, currText.c_str());

#if VDRVERSNUM < 10716
    int num_blank_lines = displayMenu->MaxItems() - Count() - 10;
#else
    int num_blank_lines = dynamic_cast<cSkinDisplayMenu *>(cSkinDisplay::Current())->MaxItems() - Count() - 10;
#endif
    while (--num_blank_lines > 0) Add(new cOsdItem("",osUnknown, false));

    Add(new cOsdItem("____________________________________________________________",osUnknown, false));
    Add(new cOsdItem("",osUnknown, false));

    if( currText.find(tr(NC_BRIDGE_STR))==0)
    {
        if(*NC_Bridge_) // NC_Bridge enabled?
            AddFloatingText(tr("bridge_on$TV-data is now available in the network for the NetClients.\n"
                "Please note that you also have to enable VLAN and configure the "
                "switches inside your network if you experience problems with "
                "your network."
                  ), MAX_LETTER_PER_LINE);
        else
            AddFloatingText(tr("bridge_off$Activate the network bridge to provide TV-signals to your home "
                               "network.\n"
                               "Warning: As soon as you activate the bridge, network components "
                   "like WLAN routers will possibly not work correct any longer. "
                   "Thus also enable VLAN and configure your switches according to "
                   "the installation guide."
                  ), MAX_LETTER_PER_LINE);
    }
    else if(currText.find(tr(BRIDGE_STR))==0)
    {
        AddFloatingText(tr("Dummytext$"), MAX_LETTER_PER_LINE);
    }
    else if (currText.find(tr(VLAN_STR))==0)
    {
        if(*Vlan_) // VLAN enabled?
            AddFloatingText(tr("vlan_on$VLAN ist now active. You can now set up your network switches."
                              ), MAX_LETTER_PER_LINE);
        else
            AddFloatingText(tr("vlan_off$VLANs are used to separate the network traffic into multiple "
                "virtual Networks. With this option you can limit the traffic of "
                "the NetCeiver to one VLAN.\n"
                "To reasonably use VLANs you also need VLAN capable switches. "
                "You can receive preconfigured switches "
                "for the Reel MultiRoom System at your local dealer."
                ), MAX_LETTER_PER_LINE);
    }
    else if (currText.find(tr(VLAN_ID_STR))==0)
    {
        AddFloatingText(tr("bridge_id$Each VLAN is referenced by its own unique ID. It is "
               "recommended to use the ID '2' for NetCeiver and NetClients."
              ), MAX_LETTER_PER_LINE);
    }
}

eOSState cMultiRoomSetupExpertMenu::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);
    if(!HasSubMenu())
    {
        switch(Key)
        {
            case kOk:
                if (inTunerMenu) {
                    if(nrTunersOrig.satS2 != nrTuners->satS2 || nrTunersOrig.sat != nrTuners->sat || nrTunersOrig.terr != nrTuners->terr || nrTunersOrig.cable != nrTuners->cable) // if something was changed
                        *nrClients_ = 0; // set number of clients to 0 to make the multiroom-menu not to adjust the tuners
                    Skins.Message(mtStatus, tr("Saving Settings..."));
                    cSetupNetClientMenu::StoreMcliConf(nrTuners, eModeMcli);
                } else if((*NC_Bridge_!=NC_Bridge_old_) ||
                        (*Bridge_!=Bridge_old_) ||
                        (*Vlan_!=Vlan_old_) ||
                        (*VlanID_!=VlanID_old_) )
                    Save();
                return osBack;
            case kBack:
                // undo changes
                *NC_Bridge_ = NC_Bridge_old_;
                *Bridge_ = Bridge_old_;
                *Vlan_ = Vlan_old_;
                *VlanID_ = VlanID_old_;
                return osBack;
                break;
            case kLeft:
            case kRight:
                if((*NC_Bridge_ != NC_Bridge_last_) || (*Vlan_ != Vlan_last_)) {
                    Set();
                    NC_Bridge_last_ = *NC_Bridge_;
                    Vlan_last_ = *Vlan_;
                }
                break;
            case kUp:
            case kUp|k_Repeat:
            case kDown:
            case kDown|k_Repeat:
                Set();
                break;
            case kBlue:
                return osBack;
            case kRed:
                if (!inTunerMenu)
                {
                    inTunerMenu = true;
                    Set();
                }
                return osContinue;
                break;
            default:
                return osContinue;
        }
    }
    return state;
}

void cMultiRoomSetupExpertMenu::Save() {
    Skins.Message(mtStatus, tr("Saving Settings..."));
    std::stringstream strBuff;
    cSysConfig_vdr &sysconfig = cSysConfig_vdr::GetInstance();
    // Store Network settings
    sysconfig.Load("/etc/default/sysconfig");
    if(*NC_Bridge_) {
        sysconfig.SetVariable("NC_BRIDGE", "yes");
        strBuff << "Ethernet " << *Bridge_+1;
        sysconfig.SetVariable("ETH0_BRIDGE", strBuff.str().c_str());

        if(*Vlan_) {
            sysconfig.SetVariable("VLAN", "yes");
            sysconfig.SetVariable("VLAN_ID", itoa(*VlanID_));
        } else
            sysconfig.SetVariable("VLAN", "no");

    } else {
        sysconfig.SetVariable("NC_BRIDGE", "no");
        sysconfig.SetVariable("VLAN", "no");
    }
    sysconfig.Save();
    Skins.Message(mtStatus, tr("Restarting network..."));
    SystemExec("setup-net restart");
    Skins.Message(mtInfo, tr("Changes saved"));
}

