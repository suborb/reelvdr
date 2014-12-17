/*
 * rotor.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include "menu.h"
#include "rotor.h"

// --- cPluginRotor ---------------------------------------------------------

cPluginRotor::cPluginRotor(void)
{
  statusMonitor = NULL;
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
}

cPluginRotor::~cPluginRotor()
{
  // Clean up after yourself!
  delete statusMonitor;
}

#if VDRVERSNUM >= 10330

bool cPluginRotor::Service(const char *Id, void *Data)
{
  struct switchdata
  {
    cDevice* device;
    cChannel* channel;
  } data;
  if (strcmp(Id,"Rotor-SwitchChannel") == 0) 
  {
     if (Data == NULL)
        return true;
     data = *((switchdata*) Data); 
     cStatusMonitor::ChannelSwitchAction(data.device,data.channel);
     return true;
  }
  return false;
}

#endif

const char *cPluginRotor::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  return NULL;
}

bool cPluginRotor::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
  return true;
}

bool cPluginRotor::Initialize(void)
{
  // Initialize any background activities the plugin shall perform.
  return true;
}

bool cPluginRotor::Start(void)
{
  // Start any background activities the plugin shall perform.
  statusMonitor = new cStatusMonitor;
  RegisterI18n(Phrases);
  RotorPositions.Load(AddDirectory(ConfigDirectory(), "rotor-positions.conf"), true);
  SatAngles.Load(AddDirectory(ConfigDirectory(), "rotor-angles.conf"), true);
  cRotorPos *t = new cRotorPos;
  RotorPositions.Ins(t);

  for(const cSource *source=Sources.First();source;source=(cSource *)source->Next())
  {
    if ((source->Code() & 0xC000) != 0x8000)
      continue;
    cRotorPos *p = RotorPositions.GetfromSource(source->Code());
    if (p==RotorPositions.First())
      RotorPositions.Add(p = new cRotorPos(source->Code(),0));
    char buf[200];
    sprintf(buf,"%s - %s",*cSource::ToString(source->Code()),source->Description());
    if (SatAngles.GetfromSource(source->Code())==SatAngles.First())
      SatAngles.Add(new cSatAngle(source->Code()));
  }
  RotorPositions.Save();
  return true;
}

void cPluginRotor::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

bool cPluginRotor::ProvidesSource(int Source, int Tuner)
{
  int Type = ((::Setup.DiSEqC & (TUNERMASK << (TUNERBITS * (Tuner-1)))) >> (TUNERBITS * (Tuner-1))) & ROTORMASK; 
  if (!(::Setup.DiSEqC & DIFFSETUPS)) 
     Type = ::Setup.DiSEqC & ROTORMASK;
  else if ((Type & ROTORLNB) == ROTORLNB) {
     Tuner = (Type & 0x30) >> 4;
     Type = ((::Setup.DiSEqC & (TUNERMASK << (TUNERBITS * (Tuner-1)))) >> (TUNERBITS * (Tuner-1))) & ROTORMASK;
     }
  return ((Type == DISEQC12 && RotorPositions.GetfromSource(Source)->Pos(Tuner)) || (Type == GOTOX && SatAngles.GetfromSource(Source)->Code(::Setup.DiSEqC & DIFFSETUPS ? Tuner : 1)));
}

cOsdObject *cPluginRotor::MainMenuAction(void)
{
  // Perform the action when selected from the main VDR menu.
  return new cMainMenuRotor(this);
}

cMenuSetupPage *cPluginRotor::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
  return NULL;
}

bool cPluginRotor::SetupParse(const char *Name, const char *Value)
{
  // Parse your own setup parameters and store their values.
  if      (!strcasecmp(Name, "Latitude"))       data.Lat = atoi(Value);
  else if (!strcasecmp(Name, "Longitude"))      data.Long = atoi(Value);
  else
    return false;
  return true;
}

#if 0 /*VDRVERSNUM >= 10331*/

const char **cPluginRotor::SVDRPHelpPages(void)
{
  static const char *HelpPages[] = {
    "DRIVE east | west\n"
    "    Drives the rotor east or west.",
    "HALT\n"
    "    Halts the rotor.",
    "STEPS east | west [ <steps> ]"
    "    Drives the rotor one (or the given) steps east or west.",
    "GOTO <position>"
    "    Lets the motor drive to the position.",
    "STORE <position>"
    "    Stores the current position.",
    "RECALC [ <satellite position> ]\n"
    "    Recalculates the satellite positions.",
    "LIMITS on | off\n"
    "    Enable / Disable Softlimits."
    "SETL east | west\n"
    "    Sets east / west softlimit.",
    "GOTOX <source>\n"
    "    Drives the rotor to the given source (like S13E)",
    NULL
    };
  return HelpPages;
}

cString cPluginRotor::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
  if (strcasecmp(Command, "DRIVE") == 0) {
    if (*Option) {
      if (strcasecmp(Option, "east") == 0) {
         DiseqcCommand(DriveEast);
         return cString::sprintf("Started to drive the rotor eastward");
         }
      else if (strcasecmp(Option, "west") == 0) {
         DiseqcCommand(DriveWest);
         return cString::sprintf("Started to drive the rotor westward");
         }
      else {
         return cString::sprintf("Unknown option: \"%s\"", Option);
         }
      }
    else {
      return cString::sprintf("No argument given for command DRIVE");
      }
    }  
  else if (strcasecmp(Command, "HALT") == 0) {
    DiseqcCommand(Halt);
    return cString::sprintf("Rotor was halted");
    }
  else if (strcasecmp(Command, "STEPS") == 0) {
    if (*Option) {
      char buf[strlen(Option) + 1];
      strcpy(buf, Option);
      const char *delim = " \t";
      char *strtok_next;
      char *p = strtok_r(buf, delim, &strtok_next);
      if (strcasecmp(p, "east") == 0 || strcasecmp(p, "west") == 0) {
          char *b = strtok_r(NULL, delim, &strtok_next);
          int steps=1;
          if (b)
            steps=atoi(b);
          if (strcasecmp(p, "east") == 0) {
             DiseqcCommand(DriveStepsEast,steps);
             return cString::sprintf("Drove %d steps eastward.",steps);
             }
          else {
             DiseqcCommand(DriveStepsWest,steps);
             return cString::sprintf("Drove %d steps westward.",steps);
             }
         }
      else {
         return cString::sprintf("Unknown option: \"%s\"", Option);
         }
      }
    else {
      return cString::sprintf("No argument given for command STEPS");
      }
    }
  else if (strcasecmp(Command, "GOTO") == 0) {
    if (*Option) {
      DiseqcCommand(Goto,atoi(Option));
      return cString::sprintf("Goto position %d",atoi(Option));
      }
    else
      return cString::sprintf("No argument given for command GOTO");
    }
  else if (strcasecmp(Command, "STORE") == 0) {
    if (*Option) {
      DiseqcCommand(Store,atoi(Option));
      return cString::sprintf("Stored position %d",atoi(Option));
      }
    else
      return cString::sprintf("No argument given for command STORE");
    }
  else if (strcasecmp(Command, "RECALC") == 0) {
    int number = 0;
    if (*Option) 
      number = atoi(Option);
    DiseqcCommand(Recalc,number);
    return cString::sprintf("Recalculated positions");
    }
  else if (strcasecmp(Command, "LIMITS") == 0) {
    if (*Option) {
      if (strcasecmp(Option, "on") == 0) {
         DiseqcCommand(LimitsOn);
         return cString::sprintf("Turned limits on");
         }
      else if (strcasecmp(Option, "off") == 0) {
         DiseqcCommand(LimitsOff);
         return cString::sprintf("Turned limits off");
         }
      else {
         return cString::sprintf("Unknown option: \"%s\"", Option);
         }
      }
    else {
      return cString::sprintf("No argument given for command LIMITS");
      }
    }
  else if (strcasecmp(Command, "SETL") == 0) {
    if (*Option) {
      if (strcasecmp(Option, "east") == 0) {
         DiseqcCommand(SetEastLimit);
         return cString::sprintf("Set east limit");
         }
      else if (strcasecmp(Option, "west") == 0) {
         DiseqcCommand(SetWestLimit);
         return cString::sprintf("Set west limit");
         }
      else {
         return cString::sprintf("Unknown option: \"%s\"", Option);
         }
      }
    else {
      return cString::sprintf("No argument given for command SETL");
      }
    }
  else if (strcasecmp(Command, "GOTOX") == 0) {
    if (*Option) {
      int i=cSource::FromString(Option);
      if (!i)
        return cString::sprintf("Unknown source");
      GotoX(i);
      return cString::sprintf("Goto angular position %s",Option);
      }
    else
      return cString::sprintf("No argument given for command GOTOX");
    }
  return NULL;  
}

#endif

VDRPLUGINCREATOR(cPluginRotor); // Don't touch this!

