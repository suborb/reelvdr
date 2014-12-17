#ifndef INSTALLWIZ_TOOLS_H
#define INSTALLWIZ_TOOLS_H

#include <vdr/tools.h>

bool FileExists(cString file);
bool HasFrontPanel();
bool IsClient(); 
bool IsAvantgarde();
bool IsNetworkDevicePresent(const char* dev);

/**
 * @brief UseAllTuners: use all tuners for all types
 *
 *  calls LimitTunerCount(-1, -1, -1, -1)
 */
void UseAllTuners();


/**
 * @brief LimitTunerCount : limits the number of tuners for each type of tuner
 *   informs mcli plugin and write to "/etc/default/mcli"
 *
 * @param dvb_c : max number of dvb_c tuners to be used
 * @param dvb_t : max number of dvb_t tuners to be used
 * @param dvb_s : max number of dvb_s tuners to be used
 * @param dvb_s2 : max number of dvb_s2 tuners to be used
 *  if any of the params is -1, then all the tuners of that type is to be used
 *
 * @return true if it was able to call "Set tuner count" service of mcli plugin
 */
bool LimitTunerCount(int dvb_c, int dvb_t, int dvb_s, int dvb_s2);

/**
 * @brief WriteTunerCountToMcliFile : writes max. number of tuners to be used into '/etc/default/mcli'
 * where -1 is written as empty string to signify :" use all the tuners of this type"
 *
 * @param dvb_c : number of dvb_c tuners
 * @param dvb_t : number of dvb_t tuners
 * @param dvb_s : number of dvb_s tuners
 * @param dvb_s2 : number of dvb_s2 tuners
 * @return true if file could be read and written
 */
bool WriteTunerCountToMcliFile(int dvb_c, int dvb_t, int dvb_s, int dvb_s2);

/**
 * @brief TotalNumberOfTuners
 * @param dvb_c
 * @param dvb_t
 * @param dvb_s
 * @param dvb_s2
 * @return
 */
bool TotalNumberOfTuners(int& dvb_c, int& dvb_t, int& dvb_s, int& dvb_s2);

/**
 * @brief AllocateTunersForServer : given the number of clients, set the number of tuners to be used for the server
 * @param noOfClients
 *
 * For each client allocate one tuner and the rest is left for the server
 *
 * Mixed configuration of tuners not supported
 *   (ie. box has _only_ sat or _only_ cable or _only_ terrestrial tuners)
 */
void AllocateTunersForServer(int noOfClients);

#endif
