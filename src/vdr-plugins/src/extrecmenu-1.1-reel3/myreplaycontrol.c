/*
 * See the README file for copyright information and how to reach the author.
 */

#include <string>
#include <vdr/interface.h>
#include <vdr/status.h>
#include <vdr/menu.h>
#include <vdr/cutter.h>
#include <vdr/videodir.h>
#include <vdr/config.h>
#include "myreplaycontrol.h"
#include "mymenusetup.h"
#include "tools.h"

using namespace std;
bool myReplayControl::active=false;

myReplayControl::myReplayControl()
{
  timesearchactive=false;
  myReplayControl::active=true;
  exitType_ = 0;
}

myReplayControl::~myReplayControl()
{
  myReplayControl::active=false;
  if(mysetup.ReturnToPlugin)
  {
    if(exitType_ == 1 || (!Active() && exitType_ != 2)) //do not open menu when another control has been started or kStop has been received
    {
      cRemote::CallPlugin("extrecmenu");
    }
  }
}

eOSState myReplayControl::ProcessKey(eKeys Key)
{
  if(Key!=kNone)
  {
    if(timesearchactive && mysetup.UseCutterQueue)
    {
      if(Key<k0 || Key>k9)
        timesearchactive=false;
    }
    else
    {
#ifdef RBMINI
        /* NetClient cannot handle the huge load of cutting a recording,
         * so delegate this task to the MR-server that is on the network
         * assuming the relative path of the recording to VideoDirectory
         * on both NetClient and the Avantgarde to be the same
         * send a SVDRP request (CUTR) to the extrecmenu of Avg
         */

        if(Key==kEditCut && mysetup.ClientMayCut != 1)
        {
        // ignore mysetup.UseCutterQueue value and send the recording to Avg for cutting
        const char *filename=cReplayControl::NowReplaying();
        if (filename)
        {
            dsyslog("extrecmenu: cut '%s' (VideoDir:'%s')", filename, VideoDirectory);

            Hide();
            const char* f = strstr(filename,VideoDirectory);

            // relative path (to VideoDir) of recording
            const char *rfilename = NULL;
            if (f == filename) //filename (rec. path) should start with video directory as base
            {
                rfilename = f + strlen(VideoDirectory);
            }
            else
            {
                // error, recording not under VideoDirectory
                Skins.Message(mtError,tr("Cannot cut recording not under VideoDirectory"));
                esyslog("ERROR: attempting to cut rec:'%s' NOT under VideoDir:'%s'", filename, VideoDirectory);
                return osContinue;
            }

            char cut_rec_cmd[512];
            sprintf(cut_rec_cmd, "/usr/sbin/cut_rec_remotely.sh -d %s plug extrecmenu CUTR %s\n",
                    Setup.NetServerIP, rfilename );
            // cut_rec_remotely.sh calls svdrpsend.sh with the same parameters,
            // it reports back errors by sending a message via svdrpsend.sh

            bool hasAvgServer = Setup.NetServerIP && strlen(Setup.NetServerIP);
            if (!hasAvgServer)
                esyslog("Setup.NetServerIP is Null or empty!");

            if (Setup.ReelboxModeTemp == 1 // client mode
                    && hasAvgServer )
            {
                dsyslog("executing: %s", cut_rec_cmd);
                //svdrp
                SystemExec(cut_rec_cmd);
                // error messages from svdrpsend.sh is sent back by the script
                // to vdr on Netclient by svdrpmesg.sh

                Skins.Message(mtInfo, tr("Added recording to cutter queue on Avantgarde"));
            }
            else
            {
                Skins.Message(mtError, tr("Cannot cut recordings on NetClient without an Avantgarde server"));
                esyslog("hasAvgServer: %i, ReelboxModeTemp: %i", hasAvgServer, Setup.ReelboxModeTemp);
            }
        }
        else
            esyslog("Current recording's filename == NULL!");

        return osContinue;
        }
        else if(mysetup.ClientMayCut)
#endif //RBMINI

      if(mysetup.UseCutterQueue)
      {
        if(Key==kEditCut)
        {
          const char *filename=cReplayControl::NowReplaying();
/*#if VDRVERSNUM >= 10728
	  const char *title=cReplayControl::NowReplayingTitle();
          if(!title || !strlen(title))
		title = filename;
#endif*/

          if(filename)
          {
            Hide();
            if(MoveCutterThread->IsCutting(filename))
              Skins.Message(mtError,tr("Recording already in cutter queue!"));
            else
            {
              cMarks marks;
#if APIVERSNUM >= 10703
              cRecording Recording(filename);
              marks.Load(filename, Recording.FramesPerSecond(), Recording.IsPesRecording());
#else
              marks.Load(filename);
#endif

              if(!marks.Count())
                Skins.Message(mtError,tr("No editing marks defined!"));
              else
              {
                if(CheckRemainingHDDSpace())
                {
//                    MoveCutterThread->AddToCutterQueue(filename, title);
                    MoveCutterThread->AddToCutterQueue(filename, NULL);
                    Skins.Message(mtInfo,tr("Added recording to cutter queue"));
                }
                else
                    Skins.Message(mtError,tr("Not enough space on harddisk!"));
              }
            }
          }
          else
              esyslog("Current recording's filename == NULL!");

          return osContinue;
        }

      }
      if(Key==kRed)
        timesearchactive=true;
      if(Key==kBack)
      {
        exitType_ = 1;
#if 0
        return osEnd;
#endif
      }
      if(Key==kStop)
      {
        exitType_ = 2;
      }
    }
  }

  return cReplayControl::ProcessKey(Key);
}

bool myReplayControl::CheckRemainingHDDSpace()
{
    int FreeMB = 0;
    VideoDiskSpace(&FreeMB);
    const char *filename=cReplayControl::NowReplaying();
    int FullSizeMB=DirSizeMB(filename);
    cMarks marks;
#if APIVERSNUM >= 10703
    cRecording rec(filename);
    marks.Load(filename, rec.FramesPerSecond(), rec.IsPesRecording());
#else
    marks.Load(filename);
#endif


#ifdef DEBUG
    printf("\033[0;93m %s(%i): File:%s FreeMB:%i FullSizeMB:%i \033[0m\n", __FILE__, __LINE__, filename, FreeMB, FullSizeMB);
#endif
    if(FreeMB > FullSizeMB)
        return true;
    else
    {
        int Position1, Position2, Range=0, RangeMB, CutSizeMB;
        cMark *mark = NULL;

        if(marks.Count()%2)
        { // Uneven
            mark = marks.First();
            Position1=0;
#if APIVERSNUM >= 10721
            Position2=mark->Position();
#else
            Position2=mark->position;
#endif
            Range = Position2-Position1;
        }
        else
        { // Even
            mark = marks.First();
#if APIVERSNUM >= 10721
            Position1=mark->Position();
            mark=marks.GetNext(Position1);
            Position2=mark->Position();
#else
            Position1=mark->position;
            mark=marks.GetNext(Position1);
            Position2=mark->position;
#endif
            Range = Position2-Position1;
        }
        mark = marks.GetNext(Position2);
        while(mark)
        {
#if APIVERSNUM >= 10721
            Position1=mark->Position();
            mark = marks.GetNext(Position1);
            Position2=mark->Position();
#else
            Position1=mark->position;
            mark = marks.GetNext(Position1);
            Position2=mark->position;
#endif
            mark = marks.GetNext(Position2);
            Range += Position2-Position1;
        }

        RangeMB = Range*MAXFRAMESIZE/(1024*1024);
#ifdef DEBUG
        printf("\033[0;93m %s(%i): RangeMB:%i \033[0m\n", __FILE__, __LINE__, RangeMB);
#endif

        if(marks.Count()%2)
            CutSizeMB = FullSizeMB-RangeMB;
        else
            CutSizeMB = RangeMB;

#ifdef DEBUG
        printf("\033[0;93m %s(%i): CutSizeMB:%i \033[0m\n", __FILE__, __LINE__, CutSizeMB);
#endif
        if(FreeMB > CutSizeMB)
            return true;
        else
            return false;
    }
}

