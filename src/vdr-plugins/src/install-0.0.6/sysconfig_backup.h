#ifndef SYSCONFIG_BACKUP_H
#define SYSCONFIG_BACKUP_H

#include <string>
#include <vector>
#include <map>

typedef std::map<std::string, std::string> SysconfigMap_t;
typedef std::vector<std::string> SysconfigKeys_t;

/**
 * @brief The cSysConfigBackup class
 * store a few sysconfig (key,value) pairs so that they can be 
 * restored if necessary later
 */

class cSysConfigBackup {
private:
    // key -> value mapping of sysconfig
    SysconfigMap_t sysConfigBackup; 
    
     // variables to be backed up
    SysconfigKeys_t keysToBackup;
    
public:
    cSysConfigBackup();
    
    /**
     * @brief SetBackupKeys
     *  keys that need to be backed up from sysconfig
     *
     * @param a vector of keys that needs to be backed up
     */
    void SetBackupKeys(const std::vector<std::string>&);

    /**
     * @brief BackupSysConfig
     * save the given keys and their corresponding values 
     */
    void BackupSysConfig();
    
    /**
     * @brief RestoreSysConfig
     * copy the saved key,value pairs back to sysconfig
     */
    void RestoreSysConfig();
};


#endif // SYSCONFIG_BACKUP_H
