/*
 * directorosd.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include "directorosd.h"
#include <time.h>
#include <vdr/themes.h>
#include <vdr/skins.h>

cDirectorOsd::cDirectorOsd(int i_portalmode, int i_swapKeys, int i_displayChannelInfo, int i_autostart, int i_themelike):cOsdObject(true)
{	
	portalMode = i_portalmode;
	swapKeys = i_swapKeys;
	displayChannelInfo = i_displayChannelInfo;
	themelike = i_themelike;
	autostart = i_autostart;
	
	//init colors
	if(themelike){
	  bgColor = Skins.Current()->Theme()->Color((char*)"clrBackground"); 
	  if(!bgColor) bgColor = toColor(52, 80, 97, 200);
	  bgClear = toColor(52, 80, 97, 0);
	  txtColor = Skins.Current()->Theme()->Color((char*)"clrWhiteText");
	  if(!txtColor) txtColor = toColor(170, 170, 170, 255);
	  activeTxtColor = clrBlack;
	  borderColor = clrWhite;
	} else {
	  bgColor = toColor(52, 80, 97, 200);
	  bgClear = toColor(52, 80, 97, 0);
	  txtColor = toColor(170, 170, 170, 255);
	  activeTxtColor = clrBlack;
	  borderColor = clrWhite;
	}

	font = cFont::GetFont(fontOsd);
	osd = NULL;
	channelname = NULL;
	asprintf(&channelname," ");
	
	Channel = Channels.GetByNumber(cDevice::PrimaryDevice()->CurrentChannel());
	const cChannel *initChannel = Channel;

	if(!Channel->LinkChannels())
		Channel = Channel->RefChannel();

	if(!Channel)
		Channel = initChannel;
		
	number = 1;
	current = 0;
	lastChannel = 0;	
	
	//try to find the right channel number
	if(Channel)
	{
		const cLinkChannels* linkChannels = Channel->LinkChannels();
		if(linkChannels)
			for(int i = 0; i < getNumLinks() + 1; i++)
			{
				if(initChannel == getChannel(i))
				{
					current = i;
				}
			}
	}
	
	lastkeytime = time(NULL);
}

void cDirectorOsd::Show(void)
{
	drawOsd();
}
 
cDirectorOsd::~cDirectorOsd(void)
{
  delete osd;
  free(channelname);
}
 
eOSState cDirectorOsd::ProcessKey(eKeys Key)
{
	eOSState state = cOsdObject::ProcessKey(Key);
	if(state == osUser1)
	{
		SwitchtoChannel(0);
		state = osContinue;
	}  
  else if (state == osUnknown) 
  {
    switch (Key)
    { 
      case kUp:
      case kUp|k_Repeat:
        CursorUp();
        break; 
      case kDown:
      case kDown|k_Repeat:
        CursorDown();
        break; 
      case kLeft:
      case kLeft|k_Repeat:
        CursorLeft();
        break; 
      case kRight:
      case kRight|k_Repeat:
        CursorRight();
        break;       
      case kOk:
		    if(isOsdShowing)
					clearOsd();
				else
					drawOsd();
        break;   
      case k0 ... k9:
      	int number;
      	number = Key;
      	
      	if(difftime(time(NULL), lastkeytime) < 2)
      		number = 10 * current + Key;
      	else if(number == 11) //catch a switch with 0
      	{
      		SwitchtoChannel(lastChannel);
        	break;
        }
      	if(portalMode == 0)
				{
					SwitchtoChannel(number - k0 - 1);
				}
				else
					SwitchtoChannel(number - k0);        
        lastkeytime =  time(NULL);
        break;
        number = 0;
      case kBack:
      	//if(autostart == 0)
      	//     SwitchtoChannel(0);
        return osEnd;
	break;
        default:
        break;
    }
    state = osContinue;
  }
  return state;
}

void cDirectorOsd::Action(void){}

void cDirectorOsd::CursorUp()
{
	if(swapKeys == 0)
		ChannelUp();
	else
		ChannelDown();
} 

void cDirectorOsd::CursorDown()
{
	if(swapKeys == 0)
		ChannelDown();
	else
		ChannelUp();
}

void cDirectorOsd::CursorLeft()
{	
	SwitchtoChannel(0);
}

void cDirectorOsd::CursorRight()
{
	SwitchtoChannel(getNumLinks());
}

void cDirectorOsd::ChannelDown()
{	
	number = getNumLinks() + 1;
	if(current > number - 1)
		current = number - 1;	
	
	if(lastChannel == current)
		return;		
	lastChannel = current;	
	if(current > 0)
		current--;
	
	const cChannel *newChannel =  getChannel(current);
	free(channelname);
	asprintf(&channelname, "%s", newChannel->Name());
	
	cDevice::PrimaryDevice()->SwitchChannel(newChannel, true);
	if(isOsdShowing)
		drawOsd();
	else if(displayChannelInfo == 1)
		Skins.Message(mtInfo, channelname);
}

void cDirectorOsd::ChannelUp()
{
	number = getNumLinks() + 1;
	
	if(current == number - 1)
		return;
	lastChannel = current;
	
	current++;
	if(current >= number - 1)
		current = number - 1;	
	if(lastChannel == current)
		return;
	
	
	const cChannel *newChannel = getChannel(current);
	
	free(channelname);	
	asprintf(&channelname, "%s", newChannel->Name());
	
	cDevice::PrimaryDevice()->SwitchChannel(newChannel, true);
	if(isOsdShowing)
		drawOsd();
	else if(displayChannelInfo == 1)
		Skins.Message(mtInfo, channelname);
}

void cDirectorOsd::SwitchtoChannel(int channelnumber)
{
	number = getNumLinks() + 1;	
	if(channelnumber > number - 1)
		channelnumber = number - 1;	
	
	if(current == channelnumber)
		return;
	lastChannel = current;
	current = channelnumber;
	
	//fprintf(stderr, "lastChannel %d\n", lastChannel);
	//fprintf(stderr, "current %d\n", current);
	
	const cChannel *newChannel = getChannel(current);
	
	free(channelname);
	asprintf(&channelname, "%s", newChannel->Name());
	
	cDevice::PrimaryDevice()->SwitchChannel(newChannel, true);
	if(isOsdShowing)
		drawOsd();
	else if(displayChannelInfo == 1)
		Skins.Message(mtInfo, channelname);
}

void cDirectorOsd::CursorOK(){}

const cChannel* cDirectorOsd::getChannel(int channelnumber)
{	
	const cChannel *retChannel = NULL;
	number = getNumLinks() + 1;
	if(channelnumber > number -1)
		channelnumber = number -1;
		
	if(channelnumber < 1)
		return Channel;
	else
	{
		const cLinkChannels* linkChannels = Channel->LinkChannels();
		cLinkChannel *lca = linkChannels->First();
		for(int i = 1; i < channelnumber; i++)
			lca = linkChannels->Next(lca);


		retChannel = (const cChannel*)lca->Channel();
		return retChannel;
	}
}

int cDirectorOsd::getNumLinks()
{
	const cLinkChannels* linkChannels = Channel->LinkChannels();
	if(linkChannels)
		return linkChannels->Count();
	else
		return 0;
}

tColor cDirectorOsd::toColor(int r, int g, int b, int t)
{
  int temp = t;
  temp <<= 8;
  temp += r;
  temp <<= 8;
  temp += g;
  temp <<= 8;
  temp += b;
  return (tColor) temp;
}

void cDirectorOsd::clearOsd(void)
{
	delete osd;
	osd = NULL;
	isOsdShowing = false;
}

void cDirectorOsd::drawOsd(void)
{	
	isOsdShowing = true;
	
	if(Channel)
	{
		const cLinkChannels* linkChannels = Channel->LinkChannels();
		if(linkChannels)
			number = getNumLinks() + 1;
		else
			number = 1;
		
		char *buffer=0;
		asprintf(&buffer,"1 %s %d", Channel->Name(), number);
				
		int theHeigth = font->Height(); //font->Height('A');
		int m_height = (number * theHeigth) + 10;
		
		//get the width		
		int gr_width = font->Width(buffer);
		for(int i = 0; i < number; i++)
		{
			const cChannel *newChannel =  getChannel(i);

			cSchedulesLock SchedulesLock(true,1000);
			cSchedules *Schedules=(cSchedules *)cSchedules::Schedules(SchedulesLock);
			const cEvent *present = NULL;
			if(Schedules)
			{
				cSchedule *pSchedule = (cSchedule *)Schedules->GetSchedule(newChannel->GetChannelID());			
				if(pSchedule)
					present = pSchedule->GetPresentEvent();
			}
			
			if(present && i != 0)
			{
				asprintf(&buffer,"%d %s (%s)", i + 1, newChannel->Name(), present->Title());
			}
			else
			{
				asprintf(&buffer,"%d %s", i + 1, newChannel->Name());
			}

			int width = font->Width(buffer);
			if(width > gr_width)
				gr_width = width;
		}
		
		gr_width += 20;
		if(gr_width%2 == 0)
			gr_width++;
		
		if(gr_width > 599) // um fehler mit dem OSD zu vermeiden, sollte man aber auch weglassen können
			gr_width = 599;
  
  		if(!osd)
  		{
  			osd = cOsdProvider::NewOsd(50, 50);
  			tArea Area = { 0, 0, gr_width, m_height,  4 };
				osd->SetAreas(&Area, 1);
  		}
		if (osd) 
		{
			bool reinitOsd = false;
			
			if(gr_width != last_gr_width)
			{
				delete osd;
				osd = cOsdProvider::NewOsd(50, 50);
				reinitOsd = true;
			}
			
			last_gr_width = gr_width;
			
			int y = 5;
			if(reinitOsd)
			{
				tArea Area = { 0, 0, gr_width, m_height,  4 };
				osd->SetAreas(&Area, 1);
			}
			
			osd->DrawRectangle(0, 0, gr_width, m_height, bgColor);
			
			for(int i = 0; i < number; i++)
			{				
				const cChannel *newChannel =  getChannel(i);
			
				cSchedulesLock SchedulesLock(true,1000);
				cSchedules *Schedules=(cSchedules *)cSchedules::Schedules(SchedulesLock);
				const cEvent *present = NULL;
				if(Schedules)
				{
					cSchedule *pSchedule = (cSchedule *)Schedules->GetSchedule(newChannel->GetChannelID());			
					if(pSchedule)
						present = pSchedule->GetPresentEvent();
				}
				
				if(portalMode == 0)
				{
					if(i < 9)
						asprintf(&buffer,"%d   %s", i + 1, newChannel->Name());
					else
						asprintf(&buffer,"%d %s", i + 1, newChannel->Name());
				}
				else
				{
					if(i == 0)
					 asprintf(&buffer,"     %s", newChannel->Name());
					else if(i < 10)
					{
						if(present)
							asprintf(&buffer,"%d   %s (%s)", i, newChannel->Name(), present->Title());
						else
							asprintf(&buffer,"%d   %s", i, newChannel->Name());
					}
					else
					{
						if(present)
							asprintf(&buffer,"%d %s (%s)", i, newChannel->Name(), present->Title());
						else
							asprintf(&buffer,"%d %s", i, newChannel->Name());
						
					}
				}
				if(current == i)
					osd->DrawText(7, y, buffer, activeTxtColor, bgColor, font);
				else
					osd->DrawText(7, y, buffer, txtColor, bgColor, font);
				y += theHeigth;
			}
			drawRect(0, 0, gr_width, m_height, borderColor);
			osd->Flush();
		}
		
		free(buffer);
	}
}

void cDirectorOsd::drawRect(int x, int y, int width, int height, tColor colorvalue)
{
	if(osd)
	{
		osd->DrawRectangle(x, y, 1, height, colorvalue);//-
		osd->DrawRectangle(x, y, width, 1, colorvalue); //|
		osd->DrawRectangle(width - 1, y, width, height, colorvalue);
		osd->DrawRectangle(x, height - 1, width, height, colorvalue);
	}
}

