
#include "channelData.h"
#include <vdr/channels.h>       // for Channels
#include <vdr/epg.h>
#include <iostream>
//#include "xmlepg2vdrthread.h"


cChannelData::cChannelData (const char *epgFileName)
{

  int len = strlen (epgFileName);
  EpgDataFileName = new char[len + 1];
  strcpy (EpgDataFileName, epgFileName);
  isParsed = false;
}                               //end cChannelData(char*)

bool cChannelData::setEpgFileName (const char *epgFileName)
{

  if (EpgDataFileName != NULL)
    delete
      EpgDataFileName;          // free memory

  int len = strlen (epgFileName);
  EpgDataFileName = new char[len + 1];

  if (EpgDataFileName == NULL)
    return false;               // TODO log this error

  strcpy (EpgDataFileName, epgFileName);
  isParsed = false;
  return true;
}                               // end setEpgFileName(char*)


int cChannelData::loadEpgData (const char *channelName)
{
  /*
   *  loads EPG data into Channel "channelName"
   */

  if (!parseEpgDataFile ())
    return -1;                  // parsing failed

  int prgCount = 0;             // programme count
  // prgCount is returned
  for (cChannel * c = Channels.First (); c; c = Channels.Next (c))
  {
    if (strcasecmp (c->Name (), channelName) == 0)
    {

      tChannelID id = c->GetChannelID ();
      cSchedulesLock tmp_schedulesLock (true, 10);
      cSchedules *tmp_schedules = (cSchedules *) cSchedules::Schedules (tmp_schedulesLock);
      cSchedule *tmp_schedule = (cSchedule *) tmp_schedules->GetSchedule (id); // channelID id

      /*
       * if no schedule exists for the channel
       * create a new one 
       */
      if (!tmp_schedule)
      {
        tmp_schedule = new cSchedule (id);
        tmp_schedules->Add (tmp_schedule);
      }
      //tmp_schedules->ClearAll();

      int duration = 0, daysForward = 0;
      time_t now = 0, lastEndTime = 0;
      time_t StartTime = 0, EndTime = 0;
      char crc_str[256];
      tEventID tmp_eventID;

      std::vector < programmeData >::iterator iter = ProgrammeList.begin ();

      for (; iter != ProgrammeList.end (); iter++)
      {
        int emptyEndTime = 0;
        now = time (NULL);

        if (strcmp (iter->end_time.c_str (), "") == 0)
        {
          emptyEndTime = 1;     //TODO log error
        }
        StartTime = str2time_t (iter->start_time.c_str (), date.c_str (), daysForward);
        if (!emptyEndTime)
          EndTime = str2time_t (iter->end_time.c_str (), date.c_str (), daysForward);

        /* 
         * Correct for "improper" Times
         * like change of day, indicated just by time and not date
         *
         * eg. 2330-0015 programme Title 
         */
        if ((double) lastEndTime > (double) StartTime)
        {
          // assume the schedule points to next day's event
          // add 24*60*60 to both EndTime and StartTime
          EndTime += 24 * 60 * 60;
          StartTime += 24 * 60 * 60;
          daysForward++;
        }
        if (!emptyEndTime)
        {
          if ((double) lastEndTime > (double) EndTime)
          {
            // StartTime before midnight and EndTime after midnight
            EndTime += 24 * 60 * 60;
            daysForward++;      // assumes date.c_str() doesnot change
          }
          duration = (int) difftime (EndTime, StartTime);
          lastEndTime = EndTime; // to take care when schedule spills into the following day
        }

        /* CORRECT FOR TIME ZONE 
         * assume that programme times are in GMT+0
         *
         * use tzset() to find your TimeZone
         * add timezone (in sec) to startTime and endTime
         *
         * TODO check if StartTime and EndTime are either side of DST/timezone change.
         */
        tzset ();
        StartTime -= timezone;
        EndTime -= timezone;

	// GA-Hack: DST for UK/Sky EPG
	// Since the C-calls for timezone-stuff are quite quirky (environment fiddling),
	// it's hard to get the current DST-info for UK. So we do a rough check if we are in the
	// EU with the same DST timing and DST is active. Then we subtract 3600s.

	if ( daylight && (
		     !strcmp(tzname[1],"CEST") || !strcmp(tzname[1],"MEST") || !strcmp(tzname[1],"WEST") ||
		     !strcmp(tzname[1],"EEST"))) {
		StartTime-=3600;
		EndTime-=3600;
	}

        /*
         *  calculate a unique eventID
         *  from title and startTime
         */
        sprintf (crc_str, "%s%32lf", iter->title.c_str (), (double) StartTime);
        tmp_eventID = (((('P' << 8) | 'W') << 16) | (crc16 (0, (unsigned char *) crc_str, sizeof (crc_str)))) & 0xFFFF;
        // 32 bit unique number

        const cEvent *tmp_evt;

       /** check if event (eventID) has already been entered **/
        tmp_evt = tmp_schedule->GetEvent (tmp_eventID);
        if (tmp_evt)
        {
          esyslog ("xepg: eventID %x already taken. Skipping event.", tmp_eventID);
          esyslog ("xepg: current title %s starttime %lf", iter->title.c_str (), (double) StartTime);
          esyslog ("xepg: earlier title %s and time %lf", tmp_evt->Title (), (double) tmp_evt->StartTime ());
          continue;
        }

        /** Add event to schedule **/
        cEvent *tmp_event = new cEvent (tmp_eventID);

        /*
         * set Title, Short Text, Description, start and end time etc...
         */

        tmp_event->SetTitle (iter->title.c_str ());
        tmp_event->SetShortText (iter->description.c_str ());
        tmp_event->SetDescription (iter->description.c_str ());

        tmp_event->SetStartTime (StartTime);
        if (!emptyEndTime)
          tmp_event->SetDuration (duration);

        int runningStatus = 0;
        if (!emptyEndTime)
          runningStatus = (StartTime < now &&
                           now < EndTime) ? SI::RunningStatusRunning :
            ((StartTime - 30 < now && now < StartTime) ? SI::RunningStatusStartsInAFewSeconds : SI::RunningStatusNotRunning);

        tmp_event->SetRunningStatus (runningStatus, c);
        tmp_event->SetVersion (0xFF);
        tmp_event->SetTableID (0x2); //0x2E   // when 0x0 it is not modified by the EPG got from DVB broadcasted

        tmp_event->SetComponents (NULL);
        tmp_event->FixEpgBugs ();

        const cEvent *tmp_delEvent = tmp_schedule->GetEventAround (StartTime);
        if (tmp_delEvent)       // remove this event
        {
          //time_t eT = tmp_delEvent->EndTime (), sT = tmp_delEvent->StartTime ();
          tmp_schedule->DropOutdated (StartTime, EndTime + 60, tmp_event->TableID (), !tmp_delEvent->Version ());
        }

        tmp_schedule->AddEvent (tmp_event);
        tmp_schedule->SetRunningStatus (tmp_event, runningStatus, c);
        tmp_schedule->Sort ();
        tmp_schedule->SetModified ();

        tmp_schedules->SetModified (tmp_schedule);
        prgCount++;
      }                         // end  programmeList loop

    }                           // endif comparing c->Name()
  }                             // end channel loop

  return prgCount;
}                               // end LoadEpgData(char*)

bool cChannelData::parseEpgDataFile ()
{

  if (isParsed == true)
    return true;                // avoid parsing again

  TiXmlDocument *doc = new TiXmlDocument;
  bool loadOkay;

  loadOkay = doc->LoadFile (EpgDataFileName);
  isParsed = false;

  if (!loadOkay) {
    return false;               // TODO add error msg and handling
  }

  if (getChannelData (doc, this) == -1) {
    return false;
  }

  isParsed = true;
  return true;

}                               // end parseEpgDataFile()

int getChannelData (TiXmlNode * pParent, cChannelData * ch_data)
{

  if (!pParent)
    return -1;                  // return an error flag

  TiXmlNode *pChild;
  TiXmlText *pText;
  int t = pParent->Type ();

  switch (t)
  {
  case TiXmlNode::DOCUMENT:
    break;

  case TiXmlNode::ELEMENT:
    if (strcmp (pParent->Value (), "channel") == 0) {
      if (pParent->ToElement () == NULL) {
        esyslog ("xepg: getChannelData() Error: No channel data");
      } else {
        TiXmlAttribute *pAttrib = pParent->ToElement ()->FirstAttribute ();
        while (pAttrib) {
          if (strcmp (pAttrib->Name (), "id") == 0) {
            ch_data->id = pAttrib->Value ();
          } else if (strcmp (pAttrib->Name (), "source") == 0) {
            ch_data->source = pAttrib->Value ();
          } else if (strcmp (pAttrib->Name (), "date") == 0) {
            ch_data->date = pAttrib->Value ();
          }
          pAttrib = pAttrib->Next ();
        }
      }
    } else if (strcmp (pParent->Value (), "programme") == 0) {
      // get programme data
      programmeData prog_data;
      for (pChild = pParent->FirstChild (); pChild != 0; pChild = pChild->NextSibling ()) {
        // get programme data
        if (pChild->Type () == TiXmlNode::ELEMENT) {
          /* TODO
           * for (iterate over a string list)
           *   if (strcmp(pChild->Value(), iter[i].cstring)==0)
           *      if ( pChild->FirstChild()!=NULL && (pText = pChild->FirstChild()->ToText())!=NULL)
           *                tmp_str =pText->Value(); else tmp_str=""
           *                switch(i) { case 1: prog_date.title = tmp_str; break; case 2:... }
           */
          if (strcmp (pChild->Value (), "title") == 0) {
            if (pChild->FirstChild () != NULL && (pText = pChild->FirstChild ()->ToText ()) != NULL) {
              prog_data.title = pText->Value ();
            } else {
              prog_data.title = "";
            }
          } else if (strcmp (pChild->Value (), "desc") == 0) {
            if (pChild->FirstChild () != NULL && (pText = pChild->FirstChild ()->ToText ()) != NULL) {
              prog_data.description = pText->Value ();
            } else {
              prog_data.description = "";
            }
          } else if (strcmp (pChild->Value (), "end") == 0) {
            if (pChild->FirstChild () != NULL && (pText = pChild->FirstChild ()->ToText ()) != NULL) {
              prog_data.end_time = pText->Value ();
            } else {
              prog_data.end_time = "";
            }
          } else if (strcmp (pChild->Value (), "start") == 0) {
            if (pChild->FirstChild () != NULL && (pText = pChild->FirstChild ()->ToText ()) != NULL) {
              prog_data.start_time = pText->Value ();
            } else {
              prog_data.start_time = "";
            }
          } else if (strcmp (pChild->Value (), "infourl") == 0) {
            if (pChild->FirstChild () != NULL && (pText = pChild->FirstChild ()->ToText ()) != NULL) {
              prog_data.infourl = pText->Value ();
            } else {
              prog_data.infourl = "";
            }
          }
        }
      }
      ch_data->ProgrammeList.push_back (prog_data);
      pParent = (TiXmlNode *) 0; // avoid this subtree
    }
    break;

  case TiXmlNode::COMMENT:
    break;

  case TiXmlNode::UNKNOWN:
    break;

  case TiXmlNode::TEXT:
    break;

  case TiXmlNode::DECLARATION:
    break;

  default:
    break;
  }
  if (pParent != 0)
    for (pChild = pParent->FirstChild (); pChild != 0; pChild = pChild->NextSibling ()) {
      if (getChannelData (pChild, ch_data) == -1)
        return -1;
    }

  return 1;
}
