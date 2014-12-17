#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <vdr/osdbase.h>
#include <vdr/config.h>
#include "updateosd.h"
#include "i18n.h"

#define LINES_IN_DISPLAY 16
#define CHARS_PER_LINE 60

const cFont *font;

cUpdateOsd::cUpdateOsd() :cOsdMenu("")
{
  font = cFont::GetFont(fontSml);
  offset = 0;
  old_time = 0;
  filechanged = false;
}

cUpdateOsd::~cUpdateOsd()
{
  DisplayMenu()->SetTitle(tr("Online Update"));
}

bool cUpdateOsd::Update_On(void)
{
  return access("/tmp/update.process",F_OK) == 0;
}

eOSState cUpdateOsd::ProcessKey(eKeys Key)
{
     switch(Key) {
          case kDown:
                offset++;
                break;
          case kUp:
                if(offset>0) offset--;
                break;
          default: break;
     }

     eOSState state = cOsdMenu::ProcessKey(Key);
     if (state == osUnknown) {
    	switch (Key) {
          case kBlue:
	        if (!Update_On()) {
		  Start_Update();
	        } else {
	          Abort_Update();
	        }	
                break;
          default: break;
	}	
     } 
  Display();
  return state;
}


void cUpdateOsd::Start_Update()
{
  char *buffer;
  old_time = 0;

  asprintf(&buffer,"touch /tmp/update.process; /usr/bin/sudo -s /usr/sbin/reel-update.sh inet_update >/var/log/update.log 2>&1 &");

  dsyslog("Update: Executing '%s'",buffer);
  SystemExec(buffer);
  free(buffer);
}

void cUpdateOsd::Abort_Update(void)
{
  char *buffer;
  asprintf(&buffer, "killall aptitude; rm /tmp/update.process");   
  esyslog("Update: Executing '%s'\nProcess manually aborted ...!!!\n",buffer);
  old_time = 0;
  SystemExec(buffer);
  free(buffer);
}

void cUpdateOsd::Display(void)
{

  cOsdMenu::Display();

  // check for running process  
  if (Update_On()) {
    DisplayMenu()->SetTitle(tr("Process started..."));
    DisplayMenu()->SetText(tr("Ready To Rumble"), 0);
    SetHelp(NULL, NULL, NULL, tr("Abort"));
  } else {    
    DisplayMenu()->SetTitle(tr("No process started"));
    DisplayMenu()->SetText(tr("No process started"), 0);
    SetHelp(NULL, NULL, NULL, tr("Start"));
  }    

  if(stat("/var/log/apt/term.log", &fstats)==0){
    if(fstats.st_mtime != old_time) { // only re-read file if it has changed 
      //printf("reading file\n");
      old_time = fstats.st_mtime;
      filechanged = true;
      std::ifstream logfile("/var/log/apt/term.log");
      std::string line;
      lines.clear();
      // read logfile
      if(logfile.is_open()) {
        while(!logfile.eof()) {
          getline(logfile, line);
          //printf("getline: %s\n", line.c_str());
          if(line.size()<CHARS_PER_LINE)
            lines.push_back(line);
          else
            lines.push_back(line.substr(0,CHARS_PER_LINE-3)+="..."); // truncate string to avoid line wrapping
        }
        logfile.close();
      }
    } else {
      filechanged = false;
      //printf("file didn't change!\n");
    }
  } else
    printf("fstat failed!\n"); 

  //printf("offset: %i\n", offset);
  // scroll to end of logfile if it has changed
  if(filechanged && (lines.size() > LINES_IN_DISPLAY)) {
    //printf("filechanged: %i - lines.size(): %i\n", filechanged, lines.size());
    offset = lines.size() - LINES_IN_DISPLAY;
  }

  // display logfile
  std::string buffer;
  for(unsigned int i=offset; i<lines.size(); i++){
    buffer+=lines.at(i).c_str();
    buffer+="\n";
    //printf("line %i: %s\n", i, lines.at(i).c_str());
  }

  //printf("text: %s\n", buffer.c_str());
  DisplayMenu()->SetText(buffer.c_str(), 0);
}

