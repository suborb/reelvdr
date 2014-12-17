#include "diseqc.h"
#include "menu.h"
#include "menuitems.h"
#include <sys/ioctl.h>
#include <linux/dvb/frontend.h>
#include <math.h>
#include <ctype.h>
#include <vdr/interface.h>
#include <vdr/plugin.h>


// --- cMainMenuRotor --------------------------------------------------

#define Col 24
#define EDW cSkinDisplay::Current()->EditableWidth()+12*Col

cMainMenuRotor::cMainMenuRotor(cPluginRotor *Plugin):cOsdMenu(tr("Rotor"), Col)
{
#if VDRVERSNUM >= 10330
  int tmp=false;
  cPlugin *p = cPluginManager::GetPlugin("text2skin");
  if (p)
    p->Service("Text2Skin-TTF",&tmp);
#endif
  plugin=Plugin;
  edw=cSkinDisplay::Current()->EditableWidth();
  first_time=true;
  oldupdate=Setup.UpdateChannels;
  Setup.UpdateChannels=0;
  Setup_DiSEqC = ::Setup.DiSEqC;
  Tuner=1;
  while (!(cDevice::GetDevice(Tuner-1) && cDevice::GetDevice(Tuner-1)->ProvidesSource(cSource::stSat)) && Tuner < MAXTUNERS)
    Tuner++;
  while (!(((Setup_DiSEqC & (TUNERMASK << (TUNERBITS * (Tuner-1)))) >> (TUNERBITS * (Tuner-1))) & ROTORMASK) && Tuner < MAXTUNERS)
    Tuner++;
  while (!(cDevice::GetDevice(Tuner-1) && cDevice::GetDevice(Tuner-1)->ProvidesSource(cSource::stSat)) && Tuner > 0)
    Tuner--;
  mode=mUser;
  cmdnr=0;
  OldChannel=Channels.GetByNumber(cDevice::GetDevice(Tuner-1)->CurrentChannel());
  HasSwitched=false;
  if (cDevice::GetDevice(Tuner-1)!=cDevice::ActualDevice())
  {
    data.sw=1;
    cDevice::GetDevice(Tuner-1)->SwitchChannel(OldChannel,false);
    data.sw=0;
  }
  RotorPos=RotorPositions.GetfromSource(OldChannel->Source());
  Position=RotorPos->Pos(Tuner);
  SatAngle=SatAngles.GetfromSource(OldChannel->Source());
  Frequenz=OldChannel->Frequency();
  if (OldChannel->Polarization() == 'v' || OldChannel->Polarization() == 'V')
    Pol='V';
  else
    Pol='H';
  Symbolrate=OldChannel->Srate();

  enum { NUMCHARS = cFont::NUMCHARS } ;
  const cFont::tCharData *data;

  for (int f=0; f<eDvbFontSize; f++)
  {
    for (int i = 32, k=0; i < NUMCHARS; i++) {
      data = cFont::GetFont((eDvbFont)f)->CharData(i);
      CharData[f][k++]=data->width;
      CharData[f][k++]=data->height;
      for (ulong j=0; j<data->height; j++) {
        CharData[f][k++]=data->lines[j];
      }
    }
    CharData[f][(127-32)*(cFont::GetFont((eDvbFont)f)->Height()+2)]=1;
    CharData[f][(128-32)*(cFont::GetFont((eDvbFont)f)->Height()+2)]=1;
    for (int i=6; i<=cFont::GetFont((eDvbFont)f)->Height()-3 ;i++)
      CharData[f][(128-32)*(cFont::GetFont((eDvbFont)f)->Height()+2)+i]=0x00000001;
    cFont::SetFont((eDvbFont)f, CharData[f]);
  }

  static char buffer[PATH_MAX];
  snprintf(buffer, sizeof(buffer), "%s%d/%s%d", "/dev/dvb/adapter",Tuner-1,"frontend",0);
  Fd_Frontend = open(buffer,0);

  SS[0]='\0';
  SNR[0]='\0';
  Status[0]='\0';
  AddMenuItems();
  CursorDown();
  UpdateStatus();
  Display();
}

cMainMenuRotor::~cMainMenuRotor()
{
  Setup.UpdateChannels=oldupdate;
  if (HasSwitched)
    cDevice::ActualDevice()->SwitchChannel(OldChannel,true);
 
  if (Fd_Frontend>0)
    close(Fd_Frontend); 
 
  for (int f=0; f<eDvbFontSize; f++)
    cFont::SetFont((eDvbFont)f);

  plugin->SetupStore("Latitude", data.Lat);
  plugin->SetupStore("Longitude", data.Long);
  RotorPositions.Save();
  SatAngles.Save();

#if VDRVERSNUM >= 10330
  int tmp=true;
  cPluginManager::CallFirstService("Text2Skin-TTF",&tmp);
#endif

}

void cMainMenuRotor::Set()
{
  int current=Current();
  Clear();
  AddMenuItems();
  SetCurrent(Get(current));
  Display();
}

void cMainMenuRotor::AddMenuItems()
{
  texts[0]=tr("no");  
  texts[1]="DiSEqC1.2";
  texts[2]="GotoX";
  texts[3]=tr("shared LNB");
  Add(new cOsdItem(SNR,osUnknown,false));
  Add(new cOsdItem(SS,osUnknown,false));
  Add(new cOsdItem(Status,osUnknown,false));
  Add(new cOsdItem("",osUnknown,false));
  if ((Setup_DiSEqC & DIFFSETUPS) == DIFFSETUPS) {
     int DiSEqC=(Setup_DiSEqC & (TUNERMASK << (TUNERBITS * (Tuner-1)))) >> (TUNERBITS * (Tuner-1));
     if ((DiSEqC & ROTORMASK) == DISEQC12)
        achsw=1;
     else if ((DiSEqC & ROTORMASK) == GOTOX)
        achsw=2;
     else if ((DiSEqC & ROTORMASK) == ROTORLNB)
        achsw=3;
     else achsw=0; 
     }
  else {
     if ((Setup_DiSEqC & ROTORMASK) == DISEQC12)
        achsw=1;
     else if ((Setup_DiSEqC & ROTORMASK) == GOTOX)
        achsw=2;
     else if ((Setup_DiSEqC & ROTORMASK) == ROTORLNB)
        achsw=3;
     else achsw=0;
     }
  a_source = achsw==2 ? SatAngle->R_Code() : RotorPos->Code();
  if (Setup_DiSEqC & DIFFSETUPS)
     Add(new cMenuEditSatTunerItem (tr("Tuner"), &Tuner,tr("none"), Setup_DiSEqC));
  snprintf(Type, sizeof(Type), "%s:\t%s", tr("Type of Control"), texts[achsw]); 
  Add(new cOsdItem(Type, osUnknown, false)); // Unveränderlich machen 
  SetHelp(NULL);
  if (achsw==2)  // GotoX
  {
    SetHelp(NULL,NULL,NULL,tr("Scan"));
    Add(new cMenuEditSatItem(tr("Satellite"),&a_source));  
    Angle = SatAngle->Code(Tuner) & ~0xC800;
    Angle*= (SatAngle->Code(Tuner) & cSource::st_Neg) ? -1 : 1;
    satActive = SatAngle->Code(Tuner)!=0;
    Add(new cMenuEditAngleItem(tr("Satellite Angle"),&Angle,0,1800,tr("West"),tr("East"),&satActive));
    Add(new cMenuEditIntpItem(tr("My Latitude"), &data.Lat,0,900,tr("North"),tr("South")));
    Add(new cMenuEditIntpItem(tr("My Longitude"), &data.Long,0,1800,tr("West"),tr("East")));
    commands[0]=tr("Goto Angular Position");
    commands[1]=tr("Goto Reference Position");
    Add(new cMenuEditCmdsItem(tr("Motor Control"), &cmdnr, 2 , commands, osUser3));
  }
  if (achsw==1)  // GotoPosition
  {
    SetHelp(tr("User"),tr("Installer"),NULL,tr("Scan"));
    Add(new cMenuEditSatItem(tr("Satellite"),&a_source));
    Add(new cMenuEditintItem(tr("Satellite number"),&Position,0,60,tr("none")));
    if (mode == mUser)
    {
      commands[0]=tr("Goto");
      commands[1]=tr("Store");
      if (cmdnr>=2)
         cmdnr=0;
      Add(new cMenuEditStepsEWItem(tr("Move"),Tuner));
      Add(new cMenuEditCmdsItem(tr("Motor Control"), &cmdnr, 2 , commands, osUser4)); 
    }
    else
    {
      commands[0]=tr("Goto");
      commands[1]=tr("Store");
      commands[2]=tr("Recalc");
      commands[3]=tr("Set East Limit");
      commands[4]=tr("Set West Limit");
      commands[5]=tr("Enable Limits");
      commands[6]=tr("Disable Limits");
      commands[7]=tr("Goto Reference Position");
      if (cmdnr>=8)
         cmdnr=0;
      Add(new cMenuEditEWItem(tr("Move"),Tuner));
      Add(new cMenuEditCmdsItem(tr("Motor Control"), &cmdnr, 8 , commands, osUser4));
    }
  }
  if (achsw==1 || achsw==2) 
  {
    Add(new cMenuEditFreqItem(tr("Frequency"),&Frequenz,&Pol,osUser1));
    Add(new cMenuEditSymbItem(tr("Symbolrate"),&Symbolrate,0,50000,osUser1));
  } 
}

void cMainMenuRotor::UpdateStatus(void)
{
  unsigned int SNRChip=0,SSChip=0;
  int i;
  char buf[700];
  if (!HasSubMenu())
  {
    if (edw!=EDW && !first_time)
      edw=EDW;
    first_time=false;

    // SNR 

    if (Fd_Frontend>0) 
    {     
      CHECK(ioctl(Fd_Frontend, FE_READ_SNR, &SNRChip));
    }
    else
      SNRChip=0;
    usleep(15);
    for (i=0; i<=(signed long)(SNRChip*(edw-Width("SNR:  99%   ",edw))/65536);i++) 
      buf[i]=edw>100 ? '\x80' : '|';
    for ( ; i<=(signed long)(edw-Width("SNR:  99%   ",edw)); i++)
      buf[i]=edw>100 ? '\x7F' : ' ';
    buf[i]='\0';
    sprintf(SNR,"SNR: %s %d%%",buf,SNRChip/655);

    // SS

    if (Fd_Frontend>0)
    {
      CHECK(ioctl(Fd_Frontend, FE_READ_SIGNAL_STRENGTH, &SSChip));
    }
    else
      SSChip=0;
    usleep(15);
    for (i=0; i<=(signed long)(SSChip*(edw-Width("SNR:  99%   ",edw))/65535);i++)
      buf[i]=edw>100 ? '\x80' : '|';
    for ( ; i<=(signed long)(edw-Width("SNR:  99%   ",edw)); i++)
      buf[i]=edw>100 ? '\x7F' : ' ';
    buf[i]='\0';
    char buf2[100];
    sprintf(buf2,"SS: ");
    for (i=0; i<(Width("SNR:",edw)-Width("SS:",edw)); i++)
      buf2[i+4]=edw>100 ? '\x7F' : ' ';
    buf2[i+4]='\0';
    sprintf(SS,"%s%s %d%%",buf2,buf,SSChip/655);
     // STATUS
    fe_status_t status = fe_status_t(0);
    if (Fd_Frontend>0)
    {
      CHECK(ioctl(Fd_Frontend, FE_READ_STATUS, &status));
    }
    usleep(15);
    buf[0]='\0';
    char h[]=" ";
    h[0]=edw>100 ? '\x7F' : ' ';
    if (status & FE_HAS_SIGNAL)
      strcat(buf,"SIGNAL");
    while(Width(buf,edw) < edw/4)
      strcat(buf,h);
    if (status & FE_HAS_CARRIER)
      strcat(buf,"CARRIER");
    while(Width(buf,edw) < edw/2)
      strcat(buf,h);
    if (status & FE_HAS_VITERBI)
      strcat(buf,"VITERBI");
    while(Width(buf,edw) < (3*edw)/4)
      strcat(buf,h);
    if (status & FE_HAS_SYNC)
      strcat(buf,"SYNC");
    Get(0)->SetText(SNR);;
    Get(1)->SetText(SS);
    strcpy(Status,buf);
    Get(2)->SetText(Status);
  }
}

eOSState cMainMenuRotor::ProcessKey(eKeys Key)
{
  int oldPosition=Position;
  int olda_source=a_source;
  int oldAngle = Angle;
  int oldsatActive = satActive;
  int oldTuner = Tuner;
  eOSState state = cOsdMenu::ProcessKey(Key);
  if (state == osUser1)
  {
    cChannel SChannel = *OldChannel; 
    SChannel.cChannel::SetSatTransponderData(a_source,Frequenz,Pol,Symbolrate,FEC_AUTO);
    if (cDevice::GetDevice(Tuner-1)==cDevice::ActualDevice())
      HasSwitched=true;
    data.sw=1;
    cDevice::GetDevice(Tuner-1)->SwitchChannel(&SChannel,HasSwitched);
    data.sw=0;
    state=osContinue;
  }
  if (state == osUser3)
  {
    switch (cmdnr) {
      case 0: if (SatAngle->Code(Tuner)) 
                GotoX(Tuner,SatAngle->Code(Tuner));
              break;
      case 1: DiseqcCommand(Tuner,Goto,0);
              break;
    }
    state=osContinue;
  }
  if (state == osUser4)
  {
    switch (cmdnr) {
      case 0: if (Position)
                DiseqcCommand(Tuner,Goto,Position);
              break;
      case 1: if (Position)
                DiseqcCommand(Tuner,Store,Position);
              break;
      case 2: DiseqcCommand(Tuner,Recalc,Position);
              break;
      case 3: DiseqcCommand(Tuner,SetEastLimit);
              break;
      case 4: DiseqcCommand(Tuner,SetWestLimit);
              break;
      case 5: DiseqcCommand(Tuner,LimitsOn);
              break;
      case 6: DiseqcCommand(Tuner,LimitsOff);
              break;
      case 7: DiseqcCommand(Tuner,Goto, 0);
              break;
    }
    state=osContinue;
  }
  if (state == osUnknown)
  {
    Key = NORMALKEY(Key);
    switch (Key) {
      case kRed   : if (achsw == 1 && mode!=mUser)
                    {
                      mode=mUser;
                      Set();
                    }
                    break;
      case kGreen : if (achsw == 1 && mode!=mInstaller)
                    {
                      mode=mInstaller; 
                      Set();
                    }
                    break;
      case kBlue  : {
                    cChannel *SChannel = new cChannel;
                    *SChannel = *OldChannel;
                    SChannel->cChannel::SetSatTransponderData(a_source,Frequenz,Pol,Symbolrate,FEC_AUTO);
                    if (cDevice::GetDevice(Tuner-1)==cDevice::ActualDevice())
                      HasSwitched=true;
                    data.ActualSource[Tuner-1]=achsw==2 ? SatAngle->R_Code() : RotorPos->Code();
                    data.sw=1;
                    cDevice::GetDevice(Tuner-1)->SwitchChannel(SChannel,HasSwitched);
                    data.sw=0;
                    AddSubMenu(new cMenuScan(Tuner));
                    break;
                    }
      default     : return state;
                 }
    state=osContinue;
  }
  if (a_source != olda_source)
  {
    RotorPos=RotorPositions.GetfromSource(a_source);
    Position=RotorPos->Pos(Tuner);
    SatAngle=SatAngles.GetfromSource(a_source);
    Set();
  }
  if (Position != oldPosition)
    RotorPos->SetPos(Position, Tuner);
  if (achsw == 2 && Current()==(Setup_DiSEqC & DIFFSETUPS ? 7 : 6))
    SetHelp(tr("inactive"),NULL,NULL,tr("Scan"));
  else if (achsw == 2)
    SetHelp(NULL,NULL,NULL,tr("Scan"));
  if (oldAngle != Angle || oldsatActive != satActive)
  {
    if (satActive)
      SatAngle->SetCode(cSource::stSat | abs(Angle) | (Angle<0 ? cSource::st_Neg : 0),Tuner);
    else
      SatAngle->SetCode(0,Tuner);
  }
  if (oldTuner != Tuner)
  {
    static char buffer[PATH_MAX];
    snprintf(buffer, sizeof(buffer), "%s%d/%s%d", "/dev/dvb/adapter",Tuner-1,"frontend",0);
    if (Fd_Frontend > 0)
      close(Fd_Frontend);
    if (Tuner)
      Fd_Frontend = open(buffer,0);
    else
      Fd_Frontend = 0;
    cChannel SChannel = *OldChannel;
    SChannel.cChannel::SetSatTransponderData(a_source,Frequenz,Pol,Symbolrate,FEC_AUTO);
    if (cDevice::GetDevice(Tuner-1)==cDevice::ActualDevice())
      HasSwitched=true;
    data.sw=1;
    cDevice::GetDevice(Tuner-1)->SwitchChannel(&SChannel,HasSwitched);
    data.sw=0;
    Position=RotorPos->Pos(Tuner);
    Set();
  }

  UpdateStatus();
  Display();
  return state;
}

//--- cMenuScan ----------------------------------------------------------------

cMenuScan::cMenuScan(int tuner):cOsdMenu("Transponder-Scan")
{
  Tuner = tuner;
  num=0;
  PFilter = new PatFilter(this);
  SFilter = new SdtFilter(PFilter,this);
  PFilter->SetSdtFilter(SFilter);
  cDevice::GetDevice(Tuner-1)->AttachFilter(SFilter);
  cDevice::GetDevice(Tuner-1)->AttachFilter(PFilter);
  SetHelp(tr("Add Channel"),tr("Add All"),NULL,NULL);
}

cMenuScan::~cMenuScan()
{
  if (PFilter)
    cDevice::GetDevice(Tuner-1)->Detach(PFilter);
  if (SFilter)
    cDevice::GetDevice(Tuner-1)->Detach(SFilter);
}

eOSState cMenuScan::ProcessKey(eKeys Key)
{
  eOSState state = cOsdMenu::ProcessKey(Key);
  if (state == osUnknown)
  {
    Key = NORMALKEY(Key);
    switch (Key) {
      case kRed:   AddChannel(Current());
                   break;
      case kGreen: for (int i=0; i<num; i++)
                     AddChannel(i);
                   break;
/*      case kOk: if (PFilter && PFilter->EndOfScan())
                {
                  cDevice::GetDevice(Tuner-1)->Detach(PFilter);
                  PFilter=NULL;
                  cDevice::GetDevice(Tuner-1)->Detach(SFilter);
                  SFilter=NULL;
                }     
                cDevice::GetDevice(Tuner-1)->SwitchChannel(&Channel[Current()],true);
                break; */
      default: return state;
      }
    state=osContinue;
  }
  return state;
}

void cMenuScan::AddChannel(int Num)
{
  cChannel *channel=Channels.GetByServiceID(Channel[Num].Source(),Channel[Num].Transponder(), Channel[Num].Sid());
  if (channel)
  {
    channel->SetName(Channel[Num].Name(),Channel[Num].ShortName(),Channel[Num].Provider());
    channel->SetId(Channel[Num].Nid(),Channel[Num].Tid(),Channel[Num].Sid(),channel->Rid());
    int Apids[MAXAPIDS + 1] = { 0 };
    int Dpids[MAXDPIDS + 1] = { 0 };
#if VDRVERSNUM>=10332
    char ALangs[MAXAPIDS + 1][MAXLANGCODE2] = { "" };
    char DLangs[MAXDPIDS + 1][MAXLANGCODE2] = { "" };
#else
    char ALangs[MAXAPIDS + 1][4] = { "" };
    char DLangs[MAXDPIDS + 1][4] = { "" };
#endif
#if VDRVERSNUM>=10509 || defined(REELVDR)
    int Spids[MAXSPIDS + 1] = { 0 }; 
    char SLangs[MAXSPIDS][MAXLANGCODE2] = { "" };
#endif
    int CaIds[MAXCAIDS+1] = { 0 };
    for (int i=0; i<=MAXAPIDS; i++)
    {
      Apids[i]=Channel[Num].Apid(i);
      strcpy(ALangs[i],Channel[Num].Alang(i));
    }
    for (int i=0; i<=MAXDPIDS; i++)
    {
      Dpids[i]=Channel[Num].Dpid(i);
      strcpy(DLangs[i],Channel[Num].Dlang(i));
    }
#if VDRVERSNUM>=10509 || defined(REELVDR)
    for (int i=0; i<=MAXSPIDS; i++)
    {
      Spids[i]=Channel[Num].Spid(i);
      strcpy(SLangs[i],Channel[Num].Slang(i));
    }
#endif
    for (int i=0; i<=MAXCAIDS; i++)
      CaIds[i]=Channel[Num].Ca(i);
#if VDRVERSNUM>=10509 || defined(REELVDR)
    channel->SetPids(Channel[Num].Vpid(),Channel[Num].Ppid(),Apids,ALangs,Dpids,DLangs,Spids,SLangs,Channel[Num].Tpid());
#else
    channel->SetPids(Channel[Num].Vpid(),Channel[Num].Ppid(),Apids,ALangs,Dpids,DLangs,Channel[Num].Tpid());
#endif
    channel->SetCaIds(CaIds);
  }
  else
  {
    for (int i=0; i<1000; i++)
      if (!Channels.GetByChannelID(tChannelID(Channel[num].Source(), Channel[num].Nid(), Channel[num].Tid(), Channel[num].Sid(),i))) 
      {
        channel=Channels.NewChannel(&Channel[Num], Channel[Num].Name(), Channel[Num].ShortName(), Channel[Num].Provider(), Channel[Num].Nid(), Channel[Num].Tid(), Channel[Num].Sid(),i);
        break;
      }
  }
}

void cMenuScan::NewChannel(const cChannel *Transponder, const char *Name, const char *ShortName, const char *Provider, int Nid, int Tid, int Sid)
{
  for (int i=0; i<num; i++)
    if (Sid==Channel[i].Sid())
      return;
  Channel[num]=*Transponder;
  Channel[num].SetNumber(0);
  Channel[num].SetName(Name,ShortName,Provider);
  Channel[num].SetId(Nid,Tid,Sid);
  n[num]=new cOsdItem("no Info");
  Add(n[num]);
  display(num);
  num++;
}

#if VDRVERSNUM>=10332 && VDRVERSNUM<10509 && !defined(REELVDR)
void cMenuScan::SetPids(int Sid,int Vpid, int Ppid, int *Apids, char ALangs[][MAXLANGCODE2], int *Dpids, char DLangs[][MAXLANGCODE2], int Tpid)
#elif VDRVERSNUM>=10509 || defined(REELVDR)
void cMenuScan::SetPids(int Sid,int Vpid, int Ppid, int *Apids, char ALangs[][MAXLANGCODE2], int *Dpids, char DLangs[][MAXLANGCODE2], int *Spids, char SLangs[][MAXLANGCODE2], int Tpid)
#else
void cMenuScan::SetPids(int Sid,int Vpid, int Ppid, int *Apids, char ALangs[][4], int *Dpids, char DLangs[][4], int Tpid)
#endif
{
  for (int i=0; i<num; i++)
    if (Sid==Channel[i].Sid())
    {
      Channel[i].SetPids(Vpid,Ppid,Apids,ALangs,Dpids,DLangs,Spids,SLangs,Tpid);
      display(i);
    }
}

void cMenuScan::SetCaIds(int Sid,const int *CaIds)
{
  for (int i=0; i<num; i++)
    if (Sid==Channel[i].Sid())
    {
      Channel[i].SetCaIds(CaIds);
      display(i);
    }
}

void cMenuScan::SetCaDescriptors(int Sid,int Level)
{
  for (int i=0; i<num; i++)
    if (Sid==Channel[i].Sid())
    {
      Channel[i].SetCaDescriptors(Level);
      display(i);
    }
}

cChannel* cMenuScan::GetChannel(int Sid)
{
  for (int i=0; i<num; i++)
    if (Sid==Channel[i].Sid())
      return &Channel[i];
  return NULL;
}


void cMenuScan::display(int num)
{
  char buf[200]="";
  sprintf(buf,"%s - %d - %d - %d (%s) %c",Channel[num].Name(),Channel[num].Sid(),Channel[num].Vpid(),Channel[num].Apid(0),*cSource::ToString(Channel[num].Source()),Channel[num].Ca(0) ? '*' : ' ');
  n[num]->SetText(buf);
  cOsdMenu::Display();
}

config data;

// --- cRotorPos ---------------------------------------

cRotorPos::cRotorPos(int Code, int Pos0, int Pos1, int Pos2, int Pos3)
{
  code = Code;
  pos[0] = Pos0;
  pos[1] = Pos1;
  pos[2] = Pos2;
  pos[3] = Pos3;
}

bool cRotorPos::Parse(const char *s)
{
  char *buf1 = NULL;
  char *buf2 = NULL;
  if (2 == sscanf(s, "%a[^=]=%a[^\n]",&buf1,&buf2))
  {
    char *b1=buf1;
    char *b2=buf2;
    int i;
    while (isspace(*b1)) b1++;
    i=0;
    while (b1[i] && !isspace(b1[i])) i++;
    b1[i]='\0';
    while (isspace(*b2)) b2++;
    code = cSource::FromString(b1);
    int temp = sscanf(b2, "%d %d %d %d",&pos[0],&pos[1],&pos[2],&pos[3]);
    for (int i=temp; i<4; i++)
      pos[i]=pos[temp-1];
  }
  free(buf1);
  free(buf2);
  return code!=0;
}

bool cRotorPos::Save(FILE *f)
{
  cSource *source = Sources.Get(code);
  if (!source)
    return true;
  return fprintf(f, "%s = %d %d %d %d\n",*cSource::ToString(source->Code()),pos[0],pos[1],pos[2],pos[3])>0;
}

// --- cRotorPositions ----------------------------------------------

cRotorPositions RotorPositions;

cRotorPos *cRotorPositions::GetfromSource(int Code)
{
  for (cRotorPos *p = First(); p; p = Next(p)) 
  {
    if (p->Code() == Code)
      return p;
  }
  return First();
}

cRotorPos *cRotorPositions::GetfromPos(int Pos, int Tuner)
{
  for (cRotorPos *p = First(); p; p = Next(p))
  {
    if (p->Pos(Tuner) == Pos)
      return p;
  }
  return First();
}

// --- cSatAngle ---------------------------------------------

cSatAngle::cSatAngle(int Code)
{
  code = Code;
  for (int i=0; i<4; i++)
    e_code[i] = Code;
}

bool cSatAngle::Parse(const char *s)
{
  char *buf1 = NULL;
  char *buf2 = NULL;
  if (2 == sscanf(s, "%a[^=]=%a[^\n]",&buf1,&buf2))
  {
    char *b1=buf1;
    char *b2=buf2;
    int i;
    while (isspace(*b1)) b1++;
    i=0;
    while (b1[i] && !isspace(b1[i])) i++;
    b1[i]='\0';
    code = cSource::FromString(b1);
    for (int i=0; b2[i]; i++)
      b2[i]=toupper(b2[i]);
    int last = false;
    for (int k=0; k<4 && !last; k++)
    {
      while (isspace(*b2)) b2++;
      i=0;
      while (b2[i] && !isspace(b2[i])) i++;
      if (!b2[i]) last=true;
      b2[i]='\0';
      if (strcmp(b2,"OFF")==0)
        e_code[k]=0;
      else
      {
        if (*b2!='S')
          e_code[k]=code;
        else
        {
          int e = cSource::FromString(b2);
          e_code[k] = e ? e : code;
        }
      }
      b2+=i+1;
    }

  }
  free(buf1);
  free(buf2);
  return code!=0;
}

bool cSatAngle::Save(FILE *f)
{
  cSource *source = Sources.Get(code);
  bool save=false; 
  for (int i=0; i<4; i++)
    if (code!=e_code[i])
       save=true;
  if (!source)
    save=false;
  if (save)
  {
    char buf[4][200];
    for (int i=0; i<4; i++)
      sprintf(buf[i],"%s",e_code[i] ? *cSource::ToString(e_code[i]) : "OFF");
    return fprintf(f, "%s = %s %s %s %s\n",*cSource::ToString(source->Code()),buf[0],buf[1],buf[2],buf[3])>0;
  }
  return true;
}

// --- cSatAngles ---------------------------------------------------

cSatAngles SatAngles;

cSatAngle *cSatAngles::GetfromSource(int Code)
{
  for (cSatAngle *p = First(); p; p = Next(p))
  {
    if (p->R_Code() == Code)
      return p;
  }
  return First();
}

