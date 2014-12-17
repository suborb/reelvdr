/*
 *
 *
 */

#include <fstream>
#include <iostream>
#include <sstream>              // zeicheketten streams
#include <string>
#include <vector>
#include "meminfo.h"

//using namespace std;
using std::cerr;
using std::cout;
using std::endl;
using std::fstream;
using std::string;
using std::vector;

inline int strToInt(const std::string & s)
{
    std::istringstream i(s);
    int x = -1;
    if (!(i >> x))
        cerr << " error in strToInt(): " << s << endl;
    return x;
}

cMemInfo::cMemInfo()
{
    //printf ("%s \n", __PRETTY_FUNCTION__);
    vector <string> meminfos;
    fstream memInfo(PROC_MEMINFO, fstream::in);
    string line;
    string nums("123456789");

    if (!memInfo)
    {
        esyslog("Unable to open File " PROC_MEMINFO "!");
    }
    else {
    // memInfo is opend in read mode, so it starts at start-of-file
    // while we haven`t hit an  error and are still reading the original
    // data and successfully read another line fron the file
        while (memInfo && (!memInfo.eof()) && getline(memInfo, line))
    {
            string::size_type pos = 0;
            if (line.find("MemTotal") == 0)
            {
                pos = line.find_first_of(nums);
                memory = cleanMemSize(line.substr(pos, line.size() - pos - 6)); // 6? 
                memory.append(" MB");
            }
        }

        memInfo.clear();            // clear error flags
        memInfo.close();            // close stream
    }
    GetDriveInfo();

}

#ifdef RBLITE

void cMemInfo::GetDriveInfo()
{
    driveInfo.clear();
    string line;
    //printf ("%s \n", __PRETTY_FUNCTION__);
    char driveLetter[4] = { 'a', 'b', 'c', 'd' };
    for (int i = 0; i < 3; i++)
    {
        char procStr[1024];
        snprintf(procStr, 1024, PROC_IDE_DEV, driveLetter[i]);
        fstream procFd(procStr, fstream::in);

        if (procFd)
        {
            getline(procFd, line);
            driveInfo.push_back(line);
        }
        procFd.close();
        //cout << "Drive " << driveLetter[i] << ": " << line << endl;
    }
}

#else  // avantgarde 

void cMemInfo::GetDriveInfo()
{
    driveInfo.clear();
    string line;
    string lastLine;
    fstream procScsi(PROC_SCSI_DEV);
    // device info for avg-2 
    char driveLetter[4] = { 'a', 'b', 'c', 'd' };    
    for (int i = 0; i <= 3; i++)
    {
        char procStr[1024];
        snprintf(procStr, 1024, PROC_IDE_DEV, driveLetter[i]);
        fstream procFd(procStr, fstream::in);

        if (procFd)
        {
            getline(procFd, line);
            driveInfo.push_back(line);
        }
        procFd.close();
        //cout << "Drive " << driveLetter[i] << ": \"" << line  << "\"" << endl;
    }
    // end device info for avg-2

#ifdef RBMINI
    string blacklist[3][3] = {
        {"Host: scsi0 Channel: 00 Id: 00 Lun: 00",
         "  Vendor: Generic  Model: Flash HS-CF      Rev: 5.39",
         "  Type:   Direct-Access                    ANSI SCSI revision: 02"},
        {"Host: scsi0 Channel: 00 Id: 00 Lun: 01",
         "  Vendor: Generic  Model: Flash HS-MS      Rev: 5.39",
         "  Type:   Direct-Access                    ANSI SCSI revision: 02"},
        {"Host: scsi0 Channel: 00 Id: 00 Lun: 02",
         "  Vendor: Generic  Model: Flash HS-SM      Rev: 5.39",
         "  Type:   Direct-Access                    ANSI SCSI revision: 02"}
    };
#endif

#ifdef RBMINI
    /** To be able to hide the usb-stick that contains the root-fs: *
     *  Read the symlink /dev/root: */
    char buf[16];
    int ret = readlink("/dev/root", buf, 16);
    if (ret == -1)
        printf("ERROR: could not read link: \"%s\"\n", strerror(errno));
    else
        buf[ret] = '\0';

    if(buf[3] >= '1' && buf[3] <= '9') buf[3] = '\0'; // strip off the '1' of "sdc1" e.g.

    /** find out details about the usb-stick to be able to hide it */
    FILE *process;
    std::string cmd = std::string("LANG=C lsscsi|grep /dev/") + buf;
    const char *strBuff;
    int h1 = -1, c1 = -1, t1 = -1, l1 = -1;
    int h2 = -2, c2 = -2, t2 = -2, l2 = -2;

    process = popen(cmd.c_str(), "r");
    if(process) {
        cReadLine readline;
        strBuff = readline.Read(process);
        while(strBuff) {
            int ret = sscanf(strBuff, "[%i:%i:%i:%i]", &h1, &c1, &t1, &l1);
            if(ret != 4)
                printf("ERROR: couldn't parse SCSI-info-line \"%s\"\n", strBuff);
            strBuff = readline.Read(process);
        }
        pclose(process);
    }
#endif

    while (!procScsi.eof() && procScsi.good()) {
       getline(procScsi, line);
       //cout << " get line: \"" << line << "\"" << endl;
#ifdef RBMINI
       if(line != blacklist[0][1] && line != blacklist[1][1] && line != blacklist[2][1])
         if(lastLine != blacklist[0][0] && lastLine != blacklist[1][0] && lastLine != blacklist[2][0]) {
            sscanf(line.c_str(), "Host: scsi%i Channel: 0%i Id: 0%i Lun: 0%i", &h2, &c2, &t2, &l2);
#endif
            //printf ("\033[0;41m %s \033[0m\n", __PRETTY_FUNCTION__);

            string vendor = SubString(line, "Vendor: ",  "Model");
            string model  = SubString(line, "Model: ", "Rev:");
            string scsiDevice = vendor;
            scsiDevice += " - ";
            scsiDevice += model;
            
#ifdef RBMINI
            if (!vendor.empty() && !(h1 == h2 && c1 == c2 && t1 == t2 && l1 == l2)) { // hide the root-fs-usb-stick
#else
            if (!vendor.empty()) {
#endif
        	    //printf ("Add SCSI device %s \n", scsiDevice.c_str());
                driveInfo.push_back(scsiDevice);
            }
#ifdef RBMINI
       }
#endif
       lastLine = line;
    }
    procScsi.close();
}
#endif 

// Avantgarde SCSI 
void cMemInfo::AddValidSCSI(const string& line)
{
    //printf ("\033[0;41m %s \033[0m\n", __PRETTY_FUNCTION__);

    string vendor = SubString(line, "Vendor: ",  "Model");
    string model  = SubString(line, "Model: ", "Rev:");
    string scsiDevice = vendor;
    scsiDevice += " - ";
    scsiDevice += model;

    if (!vendor.empty())
    {
	    //printf ("Add SCSI device %s \n", scsiDevice.c_str());
        driveInfo.push_back(scsiDevice);
    }
}

string cMemInfo::SubString(const string& line, const char *start, const char *end)
{
    string substr;

    if (!start || !end) return substr;

    string::size_type apos = line.find(start);
    if (apos != string::npos)
    {
        apos += string(start).size();
        string::size_type len  = line.find(end);
        len = len - apos;

        // skip trailing blanks  (but not at start):
        //len = line.substr(apos, len).find(" ",2);
        substr = line.substr(apos, len);
    }
    return substr;
}

const string cMemInfo::cleanMemSize(const string s)
{
    int mem = strToInt(s);

    if (mem == -1)
        return "NA";
    else if (mem > 3500)
        return s;
    else if (mem > 3000)
         return "4096";
    else if (mem > 1400)
	return "2048";
    else if (mem > 800)
	return "1024";
    else if (mem > 400)
        return "512";
    else if (mem < 129)
        return "128";
    else
        return "256";
}
