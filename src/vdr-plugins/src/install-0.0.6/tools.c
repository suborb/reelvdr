#include "tools.h"
#include <vdr/plugin.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <vdr/s2reel_compat.h>
#include "../mcli/mcli_service.h"

bool FileExists(cString file)
{
    return (*file && access(file, F_OK) == 0);
}

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
    if (HasFrontPanel()) // front panel is only available in Avg
        return false;

    if(cPluginManager::GetPlugin("rbmini")) // Netclient 1
        return true;
    if (cPluginManager::GetPlugin("reelice")) // Netclient 2
        return true;

    return false;
}

// check if the system is a server/Avantgarde hardware
bool IsAvantgarde()
{
    if (cPluginManager::GetPlugin("reelbox")) // avantgarde
        return true;

    if (HasFrontPanel())
        return true;

    return false;
}

bool IsNetworkDevicePresent(const char* dev)
{
    // check if '/sys/class/net/'dev exists
    cString dev_in_sys = cString::sprintf("/sys/class/net/%s", dev);
    return FileExists(dev_in_sys);
}


void UseAllTuners()
{
    LimitTunerCount(-1, -1, -1, -1);
}


bool LimitTunerCount(int dvb_c, int dvb_t, int dvb_s, int dvb_s2)
{
    mcli_tuner_count_t tuner_count;
    tuner_count.dvb_c = dvb_c;
    tuner_count.dvb_t = dvb_t;
    tuner_count.dvb_s = dvb_s;
    tuner_count.dvb_s2 = dvb_s2;

    WriteTunerCountToMcliFile(dvb_c, dvb_t, dvb_s, dvb_s2);
    return cPluginManager::CallAllServices("Set tuner count", &tuner_count);
}


std::string TUNER_COUNT(std::string type, int count)
{
    std::string line = "DVB_" + type + "_DEVICES";
    std::string number;

    if (count < 0)
        number = "\"\"" ;
    else {
        char num[8]; snprintf(num, 7, "\"%d\"", count);
        number = num;
    }

    line += "=" + number;
    return line;
}


bool WriteTunerCountToMcliFile(int dvb_c, int dvb_t, int dvb_s, int dvb_s2)
{
#define MCLI_FILE "/etc/default/mcli"

    std::vector <std::string> lines;
    std::ifstream ifile(MCLI_FILE, std::ifstream::in);
    std::string line;

    /* read the file and replace all _DEVICES lines with given
       values
     */
    while(!ifile.eof()) {
        //std::getline(ifile, line);
        ifile >> line;

        if (!ifile.good()) break;

        if (line.find("DVB_C_DEVICES") != std::string::npos)
            continue;
        else if (line.find("DVB_T_DEVICES") != std::string::npos)
            continue;
        else if (line.find("DVB_S_DEVICES") != std::string::npos)
            continue;
        else if (line.find("DVB_S2_DEVICES") != std::string::npos)
            continue;

        lines.push_back(line);
    } // while

    ifile.close();

    lines.push_back(TUNER_COUNT("C", dvb_c));
    lines.push_back(TUNER_COUNT("T", dvb_t));
    lines.push_back(TUNER_COUNT("S", dvb_s));
    lines.push_back(TUNER_COUNT("S2", dvb_s2));



    std::vector<std::string>::iterator it = lines.begin();

    /* write the modified file */
    std::ofstream ofile(MCLI_FILE);
    if (ofile.is_open()) {
        for ( ; it != lines.end() && ofile.good(); ++it) {
            ofile << *it << std::endl;
            std::cout<< *it << std::endl;
            isyslog("writing %s into '%s'", it->c_str(), MCLI_FILE);
        }

        ofile.close();
    }

    return it == lines.end(); // all lines were written successfully
}


bool TotalNumberOfTuners(int &dvb_c, int &dvb_t, int &dvb_s, int &dvb_s2)
{
    mclituner_info_t info;
    for (int i = 0; i < MAX_TUNERS_IN_MENU; i++)
        info.name[i][0] = '\0';
    bool result = cPluginManager::CallAllServices("GetTunerInfo", &info);

    // No plugin responded
    if (!result) return false;

    dvb_c = dvb_t = dvb_s = dvb_s2 = 0;

    for (int i = 0; i < MAX_TUNERS_IN_MENU; i++) {
        if(info.preference[i] == -1 || strlen(info.name[i]) == 0)
            break;

        switch(info.type[i]) {
        case FE_QPSK : dvb_s++; break; // DVB-S
        case FE_DVBS2: dvb_s2++; break; // DVB-S2
        case FE_OFDM : dvb_t++; break; // DVB-T
        case FE_QAM  : dvb_c++; break; // DVB-C
        } // switch
    } // for

    return true;
}

/**
 * @brief AllocateTunersForServer : given the number of clients, set the number of tuners to be used for the server
 * @param noOfClients
 *
 * For each client allocate one tuner and the rest is left for the server
 *
 * Mixed configuration of tuners not supported
 *   (ie. box has _only_ sat or _only_ cable or _only_ terrestrial tuners)
 */

void AllocateTunersForServer(int noOfClients)
{
    int dvb_t, dvb_c, dvb_s, dvb_s2;
    bool result = TotalNumberOfTuners(dvb_c, dvb_t, dvb_s, dvb_s2);

    int tunersCount = dvb_c + dvb_t + dvb_s + dvb_s2;
    if (!result || tunersCount <= 0) {
        esyslog("Failed? %d Total number of tuners found : %d. Not proceeding with allocating tuners for server",
                !result, tunersCount);
        return ;
    }

    printf("Result %d Tuners cable:%d, Terr:%d Sat:%d, Sat2:%d\n",
           result, dvb_c, dvb_t, dvb_s, dvb_s2);
    isyslog("Got from mcli: Tuners cable:%d, Terr:%d Sat:%d, Sat2:%d\n",
            dvb_c, dvb_t, dvb_s, dvb_s2);

    // find the most number of tuners in the box
    // only allocate this type of tuner to the client, ie. free them from the server
    int tuners_for_client = max( max(dvb_c, dvb_t), max(dvb_s, dvb_s2) );

    // max_tuners_of_of_one_type points to the number of tuners that the server has the most
    // this is the type of tuner that the clients will be given
    int *max_tuners_of_one_type = NULL;

    if (tuners_for_client == dvb_s2) max_tuners_of_one_type = &dvb_s2;
    else if (tuners_for_client == dvb_s) max_tuners_of_one_type = &dvb_s;
    else if (tuners_for_client == dvb_t) max_tuners_of_one_type = &dvb_t;
    else  max_tuners_of_one_type = &dvb_c;

    // allocate tuners to client, ie. server does not use it
    // one per client (tuner of the type the server has the most)
    if (max_tuners_of_one_type) {
        *max_tuners_of_one_type -= noOfClients;

        if (*max_tuners_of_one_type < 0)
            *max_tuners_of_one_type = 0;
    } else {
        esyslog("Install/%s:%d max_tuners_of_one_type is NULL! Not allocating tuners to clients", __FILE__, __LINE__);
        return;
    }
    
    // MultiRoom server must have atleast one tuner, it needs it to record programmes!
    int tuners_for_server = dvb_c + dvb_t + dvb_s + dvb_s2;
    if (tuners_for_server <= 0) {
        isyslog("%s:%d Not allocating tuners for %d clients, no tuner is left for MultiRoom Server",
                __FILE__, __LINE__ , noOfClients);
        return ;
    }
    

    isyslog("Limit tuners to: Tuners cable:%d, Terr:%d Sat:%d, Sat2:%d\n",
            dvb_c, dvb_t, dvb_s, dvb_s2);
    LimitTunerCount(dvb_c, dvb_t, dvb_s, dvb_s2);
}
