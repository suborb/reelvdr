/*
 * See the README file for copyright information and how to reach the author.
 */

#include <string>
#include <vdr/plugin.h>
#include <vdr/videodir.h>
#include "service.h"
#include "mymenusetup.h"
#include "mymenurecordings.h"

#include "extrecmenu.h"
#include "mymenuundelete.h"
#include "tools.h"
#include <sstream>

using namespace std;
bool cPluginExtrecmenu::replayArchiveDVD = false;

cPluginExtrecmenu::cPluginExtrecmenu(void)
{
}

cPluginExtrecmenu::~cPluginExtrecmenu()
{
}

const char *cPluginExtrecmenu::CommandLineHelp(void)
{
  return NULL;
}

bool cPluginExtrecmenu::ProcessArgs(int argc,char *argv[])
{
  return true;
}

bool cPluginExtrecmenu::Initialize(void)
{
  return true;
}

bool cPluginExtrecmenu::Start(void)
{
  mySortList=new SortList;
  mySortList->ReadConfigFile();

  MoveCutterThread=new WorkerThread();

  return true;
}

void cPluginExtrecmenu::Stop(void)
{
  delete mySortList;
  delete MoveCutterThread;
}

void cPluginExtrecmenu::Housekeeping(void)
{
}

cString cPluginExtrecmenu::Active(void)
{
  return MoveCutterThread->Working();
}

cOsdObject *cPluginExtrecmenu::MainMenuAction(void)
{
    if (replayArchiveDVD)
    {
        return new myMenuRecordings("dvd",NULL,1);
    }

  return new myMenuRecordings();
}

cMenuSetupPage *cPluginExtrecmenu::SetupMenu(void)
{
  return new myMenuSetup();
}

bool cPluginExtrecmenu::SetupParse(const char *Name,const char *Value)
{
  if(!strcasecmp(Name,"IsOrgRecMenu"))
    return (mysetup.ReplaceOrgRecMenu==false); // vdr-replace patch

  if(!strcasecmp(Name,"ShowRecDate"))
    mysetup.ShowRecDate=atoi(Value);
  else if(!strcasecmp(Name,"ShowRecTime"))
    mysetup.ShowRecTime=atoi(Value);
  else if(!strcasecmp(Name,"ShowRecLength"))
    mysetup.ShowRecLength=atoi(Value);
  else if(!strcasecmp(Name,"HideMainMenuEntry"))
    mysetup.HideMainMenuEntry=atoi(Value);
  else if(!strcasecmp(Name,"ReplaceOrgRecMenu"))
    mysetup.ReplaceOrgRecMenu=atoi(Value);
  else if(!strcasecmp(Name,"PatchNew"))
    mysetup.PatchNew=atoi(Value);
  else if(!strcasecmp(Name,"ShowNewRecs"))
    mysetup.ShowNewRecs=atoi(Value);
  else if(!strcasecmp(Name,"DescendSorting"))
    mysetup.DescendSorting=atoi(Value);
  else if(!strcasecmp(Name,"GoLastReplayed"))
    mysetup.GoLastReplayed=atoi(Value);
  else if(!strcasecmp(Name,"ReturnToPlugin"))
    mysetup.ReturnToPlugin=atoi(Value);
  else if(!strcasecmp(Name,"LimitBandwidth"))
    mysetup.LimitBandwidth=atoi(Value);
/* Force to use VDR's menu */
/*
  else if(!strcasecmp(Name,"UseVDRsRecInfoMenu"))
    mysetup.UseVDRsRecInfoMenu=atoi(Value);
*/
  else if(!strcasecmp(Name,"PatchFont"))
    mysetup.PatchFont=atoi(Value);
  else if(!strcasecmp(Name,"FileSystemFreeMB"))
    mysetup.FileSystemFreeMB=atoi(Value);
  else if(!strcasecmp(Name,"UseCutterQueue"))
    mysetup.UseCutterQueue=atoi(Value);
  else if(!strcasecmp(Name,"ClientMayCut"))
    mysetup.ClientMayCut=atoi(Value);
  else
    return false;
  return true;
}

bool cPluginExtrecmenu::Service(const char *Id,void *Data)
{
  if(!Data)
    //return true;
    return false;

  cOsdMenu **menu=(cOsdMenu**)Data;
  if(mysetup.ReplaceOrgRecMenu && strcmp(Id,"MainMenuHooksPatch-v1.0::osRecordings")==0)
  {
    if(menu)
      *menu=(cOsdMenu*)MainMenuAction();

    return true;
#if APIVERSNUM > 10700
  } else if(/*mysetup.ReplaceOrgRecMenu &&*/ Id && strcmp(Id,"MenuMainHook-V1.0")==0)
  {
    MenuMainHook_Data_V1_0 *data = (MenuMainHook_Data_V1_0*) Data;
    if(data->Function == osRecordings)
        data->pResultMenu = (cOsdMenu*)MainMenuAction();
    else { 
        printf("ExtrecMenu Service Function=%d not honoured\n", data->Function);
        return false;
    }
    return true;
#endif
  }
  
  if (Id && strcmp(Id,"archive")==0)
    {
        if (Data && strcmp(static_cast<const char *>(Data),"mount")==0)
        {
            cReplayControl::ClearLastReplayed(cReplayControl::LastReplayed());
            replayArchiveDVD = true;
            Recordings.Load();
        }
        return true;
    }
    if (strcmp(Id, "extrecmenu-undelete-v1.0") == 0) {
       printf (" Call Service ( extrecmenu-undelete-v1.0 ... ) \n");
        if (Data == NULL)
            return true;
        ExtrecMenuUndelete_v1_0 *serviceData = (ExtrecMenuUndelete_v1_0*) Data;
        printf (" Handle  undelete menu ) \n");
        serviceData->Menu = new cMenuUndelete(NULL, 0, true);

        return true;
    }


  
  
  return false;
}

const char **cPluginExtrecmenu::SVDRPHelpPages(void)
{
    static const char *HelpPages[] =
    {
        "CUTR <filename>\n    Start cut a recording.",
        "LSTR [ <number> ]\n"
        "    List recordings. Without option, all recordings are listed. Otherwise\n"
        "    the information for the given recording is listed.",
        "RENR <number> <new name>\n"
        "    Rename recording. Number must be the Number as returned by LSTR command.",
        "RENS <number> <new name>\n"
        "    Rename subtitle of recording. Number must be the Number as returned by LSTR command.", 
        NULL
    };
    return HelpPages;
}

cString cPluginExtrecmenu::SVDRPCommand(const char *Command,const char *Option,int &ReplyCode)
{
    std::stringstream sdat;
    std::string sout;

    if(strcasecmp(Command, "CUTR") == 0)
    {
        if(*Option)
        {
            char filename[512];
            sprintf(filename, "%s/%s", VideoDirectory, Option);

            if(MoveCutterThread->IsCutting(filename))
            {
                ReplyCode = 550;
                return "Extrecmenu: Recording already in cutter queue!";
            }
            else
            {
                cMarks marks;
#if APIVERSNUM >= 10703
                cRecording rec(filename);
                marks.Load(filename, rec.FramesPerSecond(), rec.IsPesRecording());
#else
                marks.Load(filename);
#endif

                if(!marks.Count())
                {
                    ReplyCode = 550;
                    return "Extrecmenu: No editing marks defined!";
                }
                else
                {
                    MoveCutterThread->AddToCutterQueue(filename, filename);
                    ReplyCode = 250;
                    return "Extrecmenu: Added recording to cutter queue";
                }
            }
        }
        else
        {
            ReplyCode = 501;
            return "Extrecmenu: Missing Filename";
        }
    }
    else if (strcasecmp(Command, "LSTR") == 0)
    { 
        bool recordings = Recordings.Update(true);
        if (*Option)
        {
            if (isnumber(Option))
            {
                cRecording *recording = Recordings.Get(strtol(Option, NULL, 10) - 1);
                if (recording)
                {
                    tChannelID channelID = recording->Info()->ChannelID();
	            if (channelID.Valid())
		        // 215-C
                        sdat.str(""); sdat << "C " << *channelID.ToString() << ( recording->Info()->ChannelName() ? " " : "" ) << ( recording->Info()->ChannelName() ? recording->Info()->ChannelName() : "") << "\n";
                    sout += sdat.str();

	            sdat.str("");
	            char *buffer = NULL;
		    // 215-
	            if ( asprintf( &buffer, "%sE %u %ld %d %X %X\n", "", recording->Info()->GetEvent()->EventID(), recording->Info()->GetEvent()->StartTime(), recording->Info()->GetEvent()->Duration(), recording->Info()->GetEvent()->TableID(), recording->Info()->GetEvent()->Version()) >= 0 )
	            {
	  	        sdat.str(""); sdat << buffer;
		        sout += sdat.str();
		        free(buffer);
                    }

		    // 215-
                    if ( (!isempty( recording->Info()->Title() )) && asprintf( &buffer, "%sT %s\n", "", recording->Info()->Title() ) >=0 )
	            {
		        sdat.str(""); sdat << buffer;
		        sout += sdat.str();
		        free(buffer);
	            }
		    // 215-
	            if ( (!isempty(recording->Info()->ShortText())) && asprintf( &buffer, "%sS %s\n", "", recording->Info()->ShortText() ) >=0 )
	            {
		        sdat.str(""); sdat << buffer;
		        sout += sdat.str();
		        free(buffer);
	            }

		    // 215-
	            if ( (!isempty(recording->Info()->Description())) && asprintf( &buffer,"%sD %s\n", "", recording->Info()->Description() ) >=0 )
	            {
		        sdat.str(""); sdat << buffer;
		        sout += sdat.str();
		        free(buffer); 
	            }

		    // 215-
		    if ( (!isempty(recording->FileName())) && asprintf( &buffer,"%sY %s\n", "", recording->FileName() ) >= 0 )
	            {
		        sdat.str(""); sdat << buffer;
		        sout += sdat.str();
		        free(buffer); 
	            }

	            if ( recording->Info()->GetEvent()->Contents(0) )
	            {
		        // 215-
		        sdat.str(""); sdat << "G";
		        sout += sdat.str();
		        for (int i = 0; recording->Info()->GetEvent()->Contents(i); i++)
		        {
			    if ( asprintf( &buffer," %02X", recording->Info()->GetEvent()->Contents(i)) >=0 )
			    {
			        sdat.str(""); sdat << buffer;
				sout += sdat.str();
				free(buffer);
			    }
		        }

                        sdat.str(""); sdat << "\n";
                        sout += sdat.str();
	            }

                    if ( !isempty(recording->Info()->GetEvent()->GetParentalRatingString()) )
	            {
		        int pr;
		        sscanf(recording->Info()->GetEvent()->GetParentalRatingString(),"%*s %d",&pr);
		        // 215-
		        if ( asprintf( &buffer,"%sR %d\n", "", pr ) >=0 )
	    	        {
			    sdat.str(""); sdat << buffer;
			    sout += sdat.str();
		            free(buffer); 
		        }
	            }

	            const cComponents *Components = recording->Info()->Components();
                    if (Components)
	            {
		        for (int i = 0; i < Components->NumComponents(); i++)
		        {
               		    tComponent *p = Components->Component(i);
			    // 215-
			    if ( asprintf( &buffer,"%sX %s\n", "", *p->ToString() ) >=0 )
			    {
                                sdat.str(""); sdat << buffer;
				sout += sdat.str();
			        free(buffer);
		            }
		        }
	            }

		    // 215-
	            if ( ( recording->Info()->GetEvent()->Vps() > 0 ) &&  asprintf( &buffer,"%sV %ld\n", "", recording->Info()->GetEvent()->Vps() ) >=0 )
	            {
		        sdat.str(""); sdat << buffer;
		        sout += sdat.str();
		        free(buffer);
	            }

		    // 215-
	            if ( ( recording->Info()->FramesPerSecond() > 0 ) &&  asprintf( &buffer, "%sF %.10g\n", "", recording->Info()->FramesPerSecond() ) >=0 )
	            {
		        sdat.str(""); sdat << buffer;
		        sout += sdat.str();
		        free(buffer);
	            }

		    // 215-
	            if ( ( recording->Priority() > 0 ) &&  asprintf( &buffer, "%sP %d\n", "", recording->Priority() ) >=0 )
	            {
		        sdat.str(""); sdat << buffer;
		        sout += sdat.str();
		        free(buffer);
	            }

		    // 215-
                    if ( ( recording->Lifetime() > 0 ) && ( asprintf( &buffer, "%sL %s\n", "", *itoa(recording->Lifetime() ) ) >=0 ) )
	            {
		        sdat.str(""); sdat << buffer;
		        sout += sdat.str();
		        free(buffer);
	            }

		    // 215-
	            if ( ( !isempty(recording->Info()->Aux()) ) &&  asprintf( &buffer, "%s@ %s\n", "", recording->Info()->Aux() ) >=0 )
	            {
		        sdat.str(""); sdat << buffer;
		        sout += sdat.str();
		        free(buffer);
	            }

		    // 215-
	            if ( ( recording->Info()->IsHD() ) &&  asprintf( &buffer, "%sH\n", "" ) >=0 )
	            {
		        sdat.str(""); sdat << buffer;
		        sout += sdat.str();
	                free(buffer);
	            }

	            sdat.str(""); sdat << "End of recording information";
	            sout += sdat.str();

                    ReplyCode = 215;
	            return cString( sout.c_str() );
	        }
                else
                {
                    ReplyCode = 550;
	            return cString::sprintf( "Recording \"%s\" not found", Option );
                }
            }
            else
            {
	        ReplyCode = 501;
                return cString::sprintf( "Error in recording number \"%s\"", Option );
            }
        }
        else if (recordings)
        {
            cRecording *recording = Recordings.First();
            while (recording)
            {
                if (recording->Info())
                {
                    if (recording->Info()->Title())
                    {
                        if (recording->Info()->ShortText())
                        {
			    // 250 250-
		            sdat.str(""); sdat <<  (recording == Recordings.Last() ? "" : "" ) << recording->Index() + 1 << " " << recording->Title(' ', true) << "{}" << recording->Info()->Title() << "{}" << recording->Info()->ShortText() << "\n";
		            sout += sdat.str();
                        }
	                else
                        {
			    // 250 250-
		            sdat.str(""); sdat <<  (recording == Recordings.Last() ? "" : "" ) << recording->Index() + 1 << " " << recording->Title(' ', true) << "{}" << recording->Info()->Title() << "{}" << "\n";
		            sout += sdat.str();
	                }
	            }
                }
                recording = Recordings.Next(recording);
            }
            ReplyCode = 250;
            return sout.c_str();
        }
        else
        {
            ReplyCode = 550;
            return cString( "No recordings available" );
        }
    }
    else if (strcasecmp(Command, "RENR") == 0)
    { 
        bool recordings = Recordings.Update(true);
        if (recordings)
        {
            if (*Option)
            {
                char *title;
                //char *shortText;
	        char *oldTitle=NULL;
	        char *oldShortText=NULL;
                int n = strtol(Option, &title, 10);
                cRecording *recording = Recordings.Get(n - 1);

                if (recording && title && title != Option)
                {
	            // default values from info.txt
	            if (recording && recording->Info())
                    {
	                if (recording->Info()->Title())
		            oldTitle = strdup( recording->Info()->Title() );
	                if(recording->Info()->ShortText())
		            oldShortText = strdup( recording->Info()->ShortText() );
	            }
	            // end get info from info.txt

	            recording->SetTitle(title,oldShortText);

                    if (recording->WriteInfo())
                    {
	                ReplyCode = 250;
                        Recordings.ChangeState();
                        Recordings.TouchUpdate();
	                sdat.str(""); sdat << "Renamed " << oldTitle << " to " << title;
                        sout = sdat.str();
                        free(oldTitle);
                        free(oldShortText);
	                return sout.c_str();;
                    }
                    else
                    {
	                ReplyCode = 501;
	                sdat.str(""); sdat << "Renaming " << oldTitle << " to " << title << " failed";
                        sout = sdat.str();
                        free(oldTitle);
                        free(oldShortText);
	                return sout.c_str();;
                    }
                }
                else
                {
	            ReplyCode = 501;
                    return cString( "Recording not found or wrong syntax" );
	        }
            }
            else
            {
	        ReplyCode = 501;
                return cString( "Missing Input settings" );
            }
        }
        else
        {
            ReplyCode = 550;
            return cString( "No recordings available" );
        }
    }
    else if (strcasecmp(Command, "RENS") == 0)
    { 
        bool recordings = Recordings.Update(true);
        if (recordings)
        {
            if (*Option)
            {
                //char *title;
	        char *shortText;
	        char *oldTitle=NULL;
	        char *oldShortText=NULL;
                int n = strtol(Option, &shortText, 10);
                cRecording *recording = Recordings.Get(n - 1);

                if (recording && shortText && shortText != Option)
                {
	            // default values from info.txt
	            if (recording && recording->Info())
                    {
	                if (recording->Info()->Title())
		            oldTitle = strdup( recording->Info()->Title() );
	                if(recording->Info()->ShortText())
		            oldShortText = strdup( recording->Info()->ShortText() );
	            }
	            // end get info from info.txt

	            recording->SetTitle(oldTitle,shortText);

                    if (recording->WriteInfo())
                    {
	                ReplyCode = 250;
                        Recordings.ChangeState();
                        Recordings.TouchUpdate();
	                sdat.str(""); sdat << "Renamed " << oldShortText << " to " << shortText;
                        sout = sdat.str();
                        free(oldTitle);
                        free(oldShortText);
	                return sout.c_str();;
                    }
                    else
                    {
	                ReplyCode = 501;
	                sdat.str(""); sdat << "Renaming " << oldShortText << " to " << shortText << " failed";
                        sout = sdat.str();
                        free(oldTitle);
                        free(oldShortText);
	                return sout.c_str();;
                    }
                }
                else
                {
	            ReplyCode = 501;
                    return cString( "Recording not found or wrong syntax" );
	        }
            }
            else
            {
	        ReplyCode = 501;
                return cString( "Missing Input settings" );
            }
        }
        else
        {
            ReplyCode = 550;
            return cString( "No recordings available" );
        }
    }
    else
    {
        ReplyCode = 550;
        return cString::sprintf( "Command unrecognized: \"%s\"", Command );
    }
    return NULL;
}

VDRPLUGINCREATOR(cPluginExtrecmenu); // Don't touch this!

