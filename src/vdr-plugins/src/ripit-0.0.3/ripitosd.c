#include <fstream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <vdr/osdbase.h>
#include <vdr/config.h>
#include "ripitosd.h"
#include "ripitstatus.h"
#include "tools.h"

#include "setup.h"

/* try openning the dvd device '/dev/dvd' 
   and look for error 
*/
bool IsDvdInDrive() {

    int fd = open("/dev/dvd", O_RDONLY);

    // error NO medium in drive
    if (fd < 0)
        return false;
    else {
        // DVD is in drive
        close(fd);
        return true;
    } // else

} // dvd_in_drive()



#define LINES_IN_DISPLAY 16
#define CHARS_PER_LINE 50

const cFont *font;
class cRipitStatus;

string percentString(float percent)
{
    const int len = 100;

    // string of 100 ' ' spaces
    string pString(len, ' ');

    int i;
    for (i = 1; i < len && i < percent; i++)
        pString[i] = '|';

// string already initialized with ' '
//    for (; i < len; i++)
//        pString += " ";

    pString[0] = '[';
    pString[len-1] = ']';
    return pString;
}

#define OSDTITLE "Archive Audio-CD"
cRipitOsd::cRipitOsd():cOsdMenu(tr(OSDTITLE), 8, 10)
{
    font = cFont::GetFont(fontSml);
    offset = 0;
    old_mtime = 0;
    filechanged = true;

//----------------------------taken from setup
    bitrateStrings[0] = "32";
    bitrateStrings[1] = "64";
    bitrateStrings[2] = "96";
    bitrateStrings[3] = "112";
    bitrateStrings[4] = "128";
    bitrateStrings[5] = "160";
    bitrateStrings[6] = "192";
    bitrateStrings[7] = "224";
    bitrateStrings[8] = "256";
    bitrateStrings[9] = "320";

    encoders[0] = "OGG";
    encoders[1] = "FLAC";
    encoders[2] = "WAV";
    encoders[3] = "MP3";

    preset[0] = tr("user defined");
    preset[1] = tr("low");
    preset[2] = tr("standard");
    preset[3] = tr("great");
    preset[4] = tr("best");
//----------------------------taken from setup

    ShowStatus();
}

cRipitOsd::~cRipitOsd()
{
}

eOSState cRipitOsd::ProcessKey(eKeys Key)
{
    switch (Key)
    {
    case kDown:
        offset++;
        break;
    case kUp:
        if (offset > 0)
            offset--;
        break;
    default:
        break;
    }
    eOSState state = cOsdMenu::ProcessKey(Key);
    if (state == osUnknown)
    {
        switch (Key)
        {
        case kRed: 
            // no current jobs running and DVD has medium, start a new encoding task
            if (!IsRipRunning() && IsDvdInDrive())
            {
                old_mtime = 0;
                StartEncoding(NULL);
            }
            state = osContinue;
            break;
        case kBlue:
            if (IsRipRunning())
            {
                old_mtime = 0;
                AbortEncoding();
            }
            state = osContinue;
            break;

            /* make sure unhandled color button do not trigger macrokeys*/
        case kGreen:
        case kYellow:
            state = osContinue;
            break;

        default:
            break;
        }
    }
    ShowStatus();
    return state;
}

void cRipitOsd::ShowStatus()
{
    static bool isRipRunning = false;

    // check for running process  
    if (IsRipRunning())
    {
        isRipRunning = true;
        Clear();
        SetCols(8, 18);
        Add(new cOsdItem(totalTracksStr.c_str(), osUnknown, false));
        Add(new cOsdItem("", osUnknown, false));
        Add(new cOsdItem(ripTitle.c_str(), osUnknown, false));
        Add(new cOsdItem(encodeTitle.c_str(), osUnknown, false));
        SetHelp(NULL, NULL, NULL, tr("Abort"));
    }
    else if (!IsRipRunning())
    {
        int current = Current();
        Clear();
        int nrEncoders = 3;
        if (IsLameInstalled())
            nrEncoders = 4;

        SetCols(20);

        if (!IsDvdInDrive()) {
            AddFloatingText(tr("No CD found in drive."), CHARS_PER_LINE);
            Add(new cOsdItem((""),osUnknown, false) );
            Add(new cOsdItem((""),osUnknown, false));
            Add(new cOsdItem((""),osUnknown, false));
            Add(new cOsdItem((""),osUnknown, false));
            Add(new cOsdItem((""),osUnknown, false));
            Add(new cOsdItem((""),osUnknown, false));
            Add(new cOsdItem((""),osUnknown, false));
            Add(new cOsdItem((""),osUnknown, false));
            Add(new cOsdItem((""),osUnknown, false));
            Add(new cOsdItem((""),osUnknown, false));
            Add(new cOsdItem(tr("Info:"),osUnknown, false));
            AddFloatingText(tr("Please insert an audio CD to archive it."), CHARS_PER_LINE);
            SetHelp(NULL, NULL, NULL, NULL);
            cOsdMenu::Display();
            return;
        }

        // DVD in drive
        AddFloatingText(tr("CD found in drive."), CHARS_PER_LINE);
        Add(new cOsdItem("", osUnknown, false), false);

        AddFloatingText(tr("Please select archive format and quality and press RED to start archiving your audio CD."), CHARS_PER_LINE);

        Add(new cOsdItem("", osUnknown, false), false);
        Add(new cOsdItem("", osUnknown, false), false);

        Add(new cMenuEditStraItem(tr("Destination format"), &RipitSetup.Ripit_encoder, nrEncoders, encoders));


        if (RipitSetup.Ripit_encoder == RIPIT_LAME || RipitSetup.Ripit_encoder == RIPIT_OGG)
        {
            Add(new cMenuEditStraItem(tr("Quality"), &RipitSetup.Ripit_preset, 5, preset));

            if (RipitSetup.Ripit_preset == 0 /*&& RipitSetup.Ripit_encoder == RIPIT_LAME */ )
            {
                Add(new cMenuEditStraItem(tr("Min. bitrate"), &RipitSetup.Ripit_lowbitrate, 10, bitrateStrings));
                Add(new cMenuEditStraItem(tr("Max. bitrate"), &RipitSetup.Ripit_maxbitrate, 10, bitrateStrings));
            }
        }
        SetHelp(tr("Copy"), NULL, NULL, NULL);
        SetCurrent(Get(current));
        cOsdMenu::Display();
        return;
    }

    if (stat(RIPIT_LOG, &fstats) == 0)
    {
        if (fstats.st_mtime != old_mtime)
        {                       // only re-read file if it has changed 
            cout << "reading file" << endl;
            old_mtime = fstats.st_mtime;
            filechanged = true;
            std::ifstream logfile(RIPIT_LOG);
            std::string line;
            encodeTitle.clear();
            ripTitle.clear();
            ripCount = encodeCount = totalTracks = 0;
            int tmp;
            // read logfile
            if (logfile.is_open())
            {
                while (!logfile.eof())
                {
                    getline(logfile, line);
                    cout << "LINE: " << line << endl;
                    if (line.find("Audio-CD rip process started...") == 0)
                    {
                        ;
                        cout << "started" << endl;
                    }
                    else if (line.find("Tracks") == 0)
                    {           // Tracks 1 2 3 4 5 6 7 8 will be ripped
                        std::stringstream oss;
                        oss << line.substr(7);
                        /* get the last int in the line */
                        while (oss.good())
                            oss >> tmp;
                        totalTracks = tmp;
                        oss.clear();    // clear error flags on oss
                    }
                    else if (line.find("Ripper : ") == 0)
                    {           // Ripper : 01 ...
                        ripCount++;
                        if (ripCount == totalTracks)
                            ripTitle = tr("Ripped: \t") + percentString(ripCount * 100.0 / totalTracks) + "\t" + tr("completed");
                        else
                            ripTitle = tr("Ripping: \t") + percentString(ripCount * 100.0 / totalTracks) + "\t" + line.substr(9);
                    }
                    else if (line.find("Encoder: finished -> ") == 0)
                    {
                        encodeCount++;
                        encodeTitle = tr("Encoded: \t") + percentString(encodeCount * 100.0 / totalTracks) + "\t" + line.substr(20);
                    }
                    else if (line.find("Encoder: ") == 0)
                    {
                        encodeTitle = tr("Encoding: \t") + percentString(encodeCount * 100.0 / totalTracks) + "\t" + line.substr(9);
                    }
                    else if (line.find("!! DONE") == 0)
                    {
                        encodeTitle = tr("Completed");
                        ripTitle = "";
                    }
                    else if (line.find("cdparanoia failed on") == 0)
                    {
                        encodeTitle = tr("Aborted");
                        old_mtime = 0;
                        ripTitle = "";
                    }
                    else if (line.find("No medium found") == 0)
                    {
                        encodeTitle = tr("No medium found");
                        old_mtime = 0;
                        ripTitle = "";
                    }
                    else if (line.find("PROCESS MANUALLY ABORTED") == 0)
                    {
                        encodeTitle = tr("Aborted");
                        old_mtime = 0;
                        ripTitle = "";
                    }
                    else if (line.find("Searching CDDB entry") == 0)
                    {
                        ripTitle = tr("Searching CDDB entry, please wait");
                    }
                    else if (line.find("CDTITLE") == 0 && line.length() > 8)
                    {           //make sure the Title of the CD was found
                        //printf("%s\n", line.c_str());
                        std::string title_str = tr(OSDTITLE);
                        title_str += std::string(":") + line.substr(7);
                        SetTitle(title_str.c_str());
                    }
                }
                logfile.close();
                std::stringstream oss;
                if (ripCount == 0)
                {
                    oss << tr("Searching CDDB entry, please wait");
                }
                else
                {
                    oss << tr("Total Tracks: ") << totalTracks;
                }               //if
                totalTracksStr = oss.str();
                Clear();
                //cout << "1: " << ripTitle << " 2: " << encodeTitle.c_str() << std::endl;
                Add(new cOsdItem(totalTracksStr.c_str(), osUnknown, false));
                Add(new cOsdItem("", osUnknown, false));
                Add(new cOsdItem(ripTitle.c_str(), osUnknown, false));
                Add(new cOsdItem(encodeTitle.c_str(), osUnknown, false));
            }
        }
        else
        {
            filechanged = false;
            //cout << "file didn't change!" << endl;
        }
    }
    else
        cerr << "fstat failed!" << endl;
    cOsdMenu::Display();
}
