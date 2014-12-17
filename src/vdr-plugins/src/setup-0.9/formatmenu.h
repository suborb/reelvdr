#ifndef SETUP_FORMATMENU_H
#define SETUP_FORMATMENU_H


#include <stdio.h>
#include <stdint.h>
#include <string>

#include <vdr/osdbase.h>
#include <vdr/tools.h>

// TB: jfs works now on the Client
//#ifdef RBMINI
//#define NO_JFS 1 // no jfs on NetClient
//#endif

enum ePageNums { eFormatStartPage, eFormatConfigPage, eFormatProgressPage, eFormatFinishedPage};

enum eFsType { 
    eStandardFs,
    eRecFs,
    eWinCompatFs,
    /*holds the count of fs types, ie. always the last entry in enum*/
    eLastFs
};


struct cFsDevice
{
    // eg. /dev/sdb2
    std::string dev_; 
    std::string logFileName_;
    std::string diskmodel_; 

    bool partitionMustBeCreated;

    // size and available space of partition
    long long int size_; /** size in GigaBytes */
    long long int availableSpace_; /** free space in GigaBytes */

    // type of filesystem
    eFsType type_;

    std::string Type()const;

    void SetProp(std::string dev, long long int size, long long int free);

    /* let each device have its own logfile
     * reading which formatting status can be found
     *
     * returns
     * formatting not started
     * started
     * complete success
     * complete failure
     */
    int IsFormatting();
    std::string LogFile();
};



class cFormatMenu : public cOsdMenu
{
  private:
    struct cFsDevice fsDevice_;

    // holds strings that are shown to user
    cStringList PartitionTypes_;

    eFsType newPartitionType_;

    // initial page -- kOk --> confirm page --kRed--> Format (--> Finished page ?)
    int pageNum_;

  public:
    cFormatMenu(const char*, long long int, long long int);
    ~cFormatMenu();

    void        Set();
    void        ShowStartPage       ();
    void        ShowConfirmPage     ();
    void        ShowFinishedPage    ();

    eOSState    ProcessKey      (eKeys key);
    eOSState    ProcessKeyStartPage (eKeys key);
    eOSState    ProcessKeyConfirmPage   (eKeys key);
    eOSState    ProcessKeyFinishPage    (eKeys key);

};


#endif

