#include <vdr/sysconfig_vdr.h>
#include "sysconfig_backup.h"

cSysConfigBackup::cSysConfigBackup()
{
    sysConfigBackup.clear();
    keysToBackup.clear();
}

void cSysConfigBackup::SetBackupKeys(const SysconfigKeys_t &keys)
{
    keysToBackup = keys;
}

void cSysConfigBackup::BackupSysConfig()
{
    cSysConfig_vdr& sysconfig = cSysConfig_vdr::GetInstance();    
    
    // clear previous key,value pairs
    sysConfigBackup.clear();
    
    // loop through the list of keys that need to be backed up
    SysconfigKeys_t::iterator it = keysToBackup.begin();
    
    const char* val = NULL;
    
    for ( ; it != keysToBackup.end(); ++it)
    {
        val = sysconfig.GetVariable(it->c_str());
        sysConfigBackup[*it] = val?val:""; // handle NULL values
    }
}

void cSysConfigBackup::RestoreSysConfig()
{
    cSysConfig_vdr& sysconfig = cSysConfig_vdr::GetInstance();
    SysconfigMap_t::iterator it = sysConfigBackup.begin();
    
    int changed = 0;
    for(; it!=sysConfigBackup.end(); ++it)
    {
        sysconfig.SetVariable(it->first.c_str(), it->second.c_str(), changed);
    }
    
    if (changed)
        sysconfig.Save();
}
