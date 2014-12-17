/*
 * checkerOsd.c: Internationalization
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: checkerOsd.c,v 1.24 2006/06/18 11:46:31 schmitzj Exp $
 *
 */

#include <vdr/device.h>
#include "checkerOsd.h"

#define HEADINFO	"(c) JS"

#ifndef clrGrey
#define clrGrey 0xFF5F5F5F
#endif

class blinkThread : public cThread
{
	class checkerOsd *parent;
	struct recordingdata *recordingtimer;
	int doexit;
	cTimer *showThisTimer;
	int timerNumber;
	int timerState;
	int timerTimer;
	
	virtual void Action(void)
	{
		int b=0;
		do
		{
			rectsem.Lock();
			if (recordingtimer)
			{
				for(int i=0;i<MAXRECORDS;i++)
				{
					if (recordingtimer[i].timer && recordingtimer[i].timer->Recording())
					{
						osdsem.Lock();
						tColor col = (b&1)?clrGrey:clrYellow;
						parent->osd->DrawRectangle(recordingtimer[i].x1,recordingtimer[i].y1,recordingtimer[i].x2,recordingtimer[i].y2,col);
						osdsem.Unlock();
					}
				}
			}
			rectsem.Unlock();
			cTimer *ct=NULL;
			sem.Lock();
			if (--timerTimer<0)
			{
				ct=showThisTimer;
				timerTimer=2;
			}
			int tnr=timerNumber;
			int ts=timerState;
			timerState++;
			if (timerState>1) 
				timerState=0;
			sem.Unlock();
			if (ct)
			{
				char buf[2000];
				osdsem.Lock();
				parent->osd->DrawRectangle(parent->x0_graph,parent->y0_graph+5+30+cFont::GetFont(fontOsd)->Height()*3-5,parent->x0_graph+parent->width_graph,parent->y0_graph+5+30+cFont::GetFont(fontOsd)->Height()*4,clrGray50);
				switch (ts)
				{
					case 2:
						sprintf(buf,"%d: ",(tnr>9?0:tnr));
						break;
					case 1:
						sprintf(buf,"%d: %02d:%02d - %02d:%02d, %s",(tnr>9?0:tnr),ct->Start()/100,ct->Start()%100,ct->Stop()/100,ct->Stop()%100,ct->Channel()->Name());
						break;
					case 0:
					default:
						sprintf(buf,"%d: %s, (%s %d)",(tnr>9?0:tnr),ct->File(),tr("Prio"),ct->Priority());
						break;
				}
				parent->osd->DrawText(parent->x0_graph+5,parent->y0_graph+5+30+cFont::GetFont(fontOsd)->Height()*3-5,buf,clrYellow,clrGray50,cFont::GetFont(fontSml));
				osdsem.Unlock();
			}
			osdsem.Lock();
			parent->osd->Flush();
			osdsem.Unlock();
			sleep(1);
			b=1-b;
		}
		while (!doexit);
	}
	cMutex sem;
	cMutex osdsem;
	cMutex rectsem;
public:
	blinkThread(checkerOsd *o) : cThread(), sem(), osdsem(), rectsem()
	{
		recordingtimer=NULL;
		parent=o;
		doexit=0;
		timerState=0;
		showThisTimer=NULL;
		timerTimer=0;
	}
	void finish(void)
	{
		doexit=1;
		Cancel(3);
	}
	void setTimers(struct recordingdata *r)
	{
		rectsem.Lock();
		recordingtimer=r;
		rectsem.Unlock();
	}
	void setShow(int n,cTimer *t)
	{
		sem.Lock();
		showThisTimer=t;
		timerNumber=n;
		timerState=0;
		timerTimer=0;
		sem.Unlock();
	}
	void LockOSD(void)
	{
		osdsem.Lock();
	}
	void UnlockOSD(void)
	{
		osdsem.Unlock();
	}
};

checkerOsd::checkerOsd(class cPlugin *p)
{
	osd=NULL;
	parent=p;
	startLine=0;
	lines=NULL;
	lines_c=0;
	blinker=NULL;
	if (fontFix==1)
		smallFont=fontOsd;
	else
		smallFont=(enum eDvbFont)1;
}
checkerOsd::checkerOsd(void)
{
	osd=NULL;
	parent=NULL;
	startLine=0;
	lines=NULL;
	lines_c=0;
	blinker=NULL;
	if (fontFix==1)
		smallFont=fontOsd;
	else
		smallFont=(enum eDvbFont)1;
}
checkerOsd::~checkerOsd(void)
{
	if (blinker)
	{
		blinker->finish();
		delete blinker;
	}
	for(int i=0;i<lines_c;i++)
	{
		delete [] lines[i];
	}
	if (lines)
		delete [] lines;
	if (osd)
		delete osd;
}
void checkerOsd::genLines(int day,time_t dayt)
{
	if (lines && !day && !dayt)
		return;

	lines_c=Timers.Count()*3;
	if (lines_c>0)
	{
		char buf[2000];
		int si=0;

		if (!lines)
		{
			lines=new char*[lines_c];
			for(int i=0;i<lines_c;i++)
			{
				lines[i]=NULL;
			}
		}

		cTimer *ct1,*ct2;

		Timers.Sort();
		ct1=Timers.First();
		while (ct1)
		{
			cChannel const *ch1=ct1->Channel();
			char line1[200],line2[200],line3[200];

			if (ct1->Flags() & tfActive)
			{
				if (showDay<0 && ct1->IsSingleEvent())
				{
					showDay=cTimer::GetMDay(ct1->Day());
				}

				for(int i=0;i<MAXDEVICES;i++)
					devices[i]=NULL;

				ct2=Timers.First();
				while (ct2)
				{
					cChannel const *ch2=ct2->Channel();

					if (ct1!=ct2 && ct2->Flags() & tfActive)

					{
						bool hasmatched=false;
						time_t start1t,start2t;
						time_t stop1t; //,stop2t;
						struct tm day1;

						if (ct1->IsSingleEvent() && !ct2->IsSingleEvent())
						{
							start1t=ct1->StartTime();
							stop1t=ct1->StopTime();

							if (ct2->DayMatches(start1t))
							{ // Start-Tag �berschneidung
								start2t=ct2->SetTime(start1t,ct2->TimeToInt(ct2->Start()));
								hasmatched=(start1t<=start2t && start2t<stop1t);
							}
							if (ct2->DayMatches(stop1t))
							{ // Stop-Tag �berschneidung
								start2t=ct2->SetTime(stop1t,ct2->TimeToInt(ct2->Start()));
								hasmatched|=(start1t<=start2t && start2t<stop1t);
							}

							if (hasmatched)
							{
								localtime_r(&start1t,&day1);
								sprintf(line1,"%s %04d-%02d-%02d (%s):",tr("Conflict on"),day1.tm_year+1900,day1.tm_mon+1,day1.tm_mday,tr("same input device"));
								sprintf(line2,"%02d:%02d-%02d:%02d, (P%d) %s: %s",ct1->Start()/100,ct1->Start()%100,ct1->Stop()/100,ct1->Stop()%100,ct1->Priority(),ch1->Name(),ct1->File());
								sprintf(line3,"%02d:%02d-%02d:%02d (%s), (P%d) %s: %s",ct2->Start()/100,ct2->Start()%100,ct2->Stop()/100,ct2->Stop()%100,(const char *)ct2->PrintDay(ct2->Day(),ct2->WeekDays(),true),ct2->Priority(),ch2->Name(),ct2->File());
							}
						}
						else if (!ct1->IsSingleEvent() && ct2->IsSingleEvent())
						{
							start2t=ct2->StartTime();

							if (ct1->DayMatches(start2t))
							{
								start1t=ct1->SetTime(start2t,ct1->TimeToInt(ct1->Start()));
								if (ct1->Stop()<ct1->Start())
									stop1t =ct1->SetTime(start2t+SECSINDAY,ct1->TimeToInt(ct1->Stop()));
								else
									stop1t =ct1->SetTime(start2t,ct1->TimeToInt(ct1->Stop()));
								hasmatched=(start1t<=start2t && start2t<stop1t);
							}
							if (ct1->DayMatches(ct2->StopTime()))
							{
								start1t=ct1->SetTime(ct2->StopTime(),ct1->TimeToInt(ct1->Start()));
								if (ct1->Stop()<ct1->Start())
									stop1t =ct1->SetTime(ct2->StopTime()+SECSINDAY,ct1->TimeToInt(ct1->Stop()));
								else
									stop1t =ct1->SetTime(ct2->StopTime(),ct1->TimeToInt(ct1->Stop()));
								hasmatched=(start1t<=start2t && start2t<stop1t);
							}

							if (hasmatched)
							{
								localtime_r(&start1t,&day1);
								sprintf(line1,"%s %04d-%02d-%02d (%s):",tr("Conflict on"),day1.tm_year+1900,day1.tm_mon+1,day1.tm_mday,tr("same input device"));
								sprintf(line2,"%02d:%02d-%02d:%02d (%s), (P%d) %s: %s",ct1->Start()/100,ct1->Start()%100,ct1->Stop()/100,ct1->Stop()%100,(const char *)ct1->PrintDay(ct1->Day(),ct1->WeekDays(),true),ct1->Priority(),ch1->Name(),ct1->File());
								sprintf(line3,"%02d:%02d-%02d:%02d, (P%d) %s: %s",ct2->Start()/100,ct2->Start()%100,ct2->Stop()/100,ct2->Stop()%100,ct2->Priority(),ch2->Name(),ct2->File());
							}
						}
						else if (!ct1->IsSingleEvent() && !ct2->IsSingleEvent())
						{
							int mdays=0;
							bool hassubmatch;
							for(int i=0;i<6;i++)
							{
								hassubmatch=false;
								if (ct1->WeekDays() & (1<<i))
								{
									int nn=(4+i)*SECSINDAY; // 4: first Sunday after Unix time 0
									start1t=ct1->SetTime(nn,ct1->TimeToInt(ct1->Start()));
									if (ct1->Stop()<ct1->Start())
										stop1t =ct1->SetTime(nn+SECSINDAY,ct1->TimeToInt(ct1->Stop()));
									else
										stop1t =ct1->SetTime(nn,ct1->TimeToInt(ct1->Stop()));

									if (ct2->DayMatches(start1t))
									{ // Start-Tag �berschneidung
										start2t=ct2->SetTime(start1t,ct2->TimeToInt(ct2->Start()));
										hassubmatch|=(start1t<=start2t && start2t<stop1t);
									}
									if (ct2->DayMatches(stop1t))
									{ // Stop-Tag �berschneidung
										start2t=ct2->SetTime(stop1t,ct2->TimeToInt(ct2->Start()));
										hassubmatch|=(start1t<=start2t && start2t<stop1t);
									}
									if (hassubmatch)
										mdays|=(1<<i);
								}
								hasmatched|=hassubmatch;
							}
							if (hasmatched)
							{
								sprintf(line1,"%s %s (%s):",tr("Repeating conflict on"),(const char *)ct1->PrintDay(0,mdays,true),tr("same input device"));
								sprintf(line2,"%02d:%02d-%02d:%02d (%s), (P%d) %s: %s",ct1->Start()/100,ct1->Start()%100,ct1->Stop()/100,ct1->Stop()%100,(const char *)ct1->PrintDay(ct1->Day(),ct1->WeekDays(),true),ct1->Priority(),ch1->Name(),ct1->File());
								sprintf(line3,"%02d:%02d-%02d:%02d (%s), (P%d) %s: %s",ct2->Start()/100,ct2->Start()%100,ct2->Stop()/100,ct2->Stop()%100,(const char *)ct2->PrintDay(ct2->Day(),ct2->WeekDays(),true),ct2->Priority(),ch2->Name(),ct2->File());
							}
						}
						else // ct1->IsSingleEvent() && ct2->IsSingleEvent()
						{
							start1t=ct1->StartTime();
							stop1t=ct1->StopTime();
							start2t=ct2->StartTime();
							hasmatched=(start1t<=start2t && start2t<stop1t);

							if (hasmatched)
							{
								localtime_r(&start1t,&day1);
								sprintf(line1,"%s %04d-%02d-%02d (%s):",tr("Conflict on"),day1.tm_year+1900,day1.tm_mon+1,day1.tm_mday,tr("same input device"));
								sprintf(line2,"%02d:%02d-%02d:%02d, (P%d) %s: %s",ct1->Start()/100,ct1->Start()%100,ct1->Stop()/100,ct1->Stop()%100,ct1->Priority(),ch1->Name(),ct1->File());
								sprintf(line3,"%02d:%02d-%02d:%02d, (P%d) %s: %s",ct2->Start()/100,ct2->Start()%100,ct2->Stop()/100,ct2->Stop()%100,ct2->Priority(),ch2->Name(),ct2->File());
							}
						}
						if (hasmatched) // stop1<=stop2 && start2<stop1) // start1<=start2 && stop1>start2)
						{ // gleiche Zeit, pa�t Sender?
							if (ch1->Frequency()!=ch2->Frequency())
							{ // verschiedene Kan�le, genug Devices?
								bool nofree=true;
								
								cDevice *cd1=cDevice::GetDevice(ch1);
								cDevice *cd2=cDevice::GetDevice(ch2),*cdt;
								if (cd1->DeviceNumber()!=cd2->DeviceNumber())
								{ // Sonderfall: ch2 kann gar nicht auf Device von ch1 empfangen werden
									nofree=false;
								}
								else
								{
									for(int i=cDevice::NumDevices()-1;i>=0 && nofree;i--)
									{
										cdt=cDevice::GetDevice(i);
										if (cdt->IsPrimaryDevice() && !timelineCfg.ignorePrimary)
										{
											if (Setup.PrimaryLimit>ct2->Priority())
												continue;
										}
										
										if (cdt->ProvidesChannel(ch2) && cd1->DeviceNumber()!=cdt->DeviceNumber())
										{
											for (int j=0;j<MAXDEVICES;j++)
											{
												if (devices[j]==NULL)
												{
													devices[j]=cdt;
													frequencies[j]=ch2->Frequency();
													nofree=false;
													break;
												}
												else if (devices[j]->DeviceNumber()==cdt->DeviceNumber())
												{
													if (frequencies[j]==ch2->Frequency())
														nofree=false;
													break;
												}
											}
										}
									}
								}

								if (nofree)
								{ // gleiches Device - schlecht!
									if (!day && !dayt)
										addText(line1,line2,line3,si);
									else
										paintOver(ct1,ct2,day,dayt);
								}
							}
						}
					}
					ct2=Timers.Next(ct2);
				}
			}
			ct1=Timers.Next(ct1);
		}
		if (si==0 && !day && !dayt)
		{
			sprintf(buf,"%s",tr("No conflicts"));
			lines[si]=new char[strlen(buf)+1];
			strcpy(lines[si],buf);
			si++;
		}
		if (!day && !dayt)
			totalLines=si;
	}
}
void checkerOsd::addText(char *line1,char *line2,char *line3,int &si)
{
	lines[si]=new char[strlen(line1)+1];
	strcpy(lines[si],line1);
	si++;

	lines[si]=new char[strlen(line2)+1];
	strcpy(lines[si],line2);
	si++;

	lines[si]=new char[strlen(line3)+1];
	strcpy(lines[si],line3);
	si++;
}
void checkerOsd::paintOver(cTimer *ct1,cTimer *ct2,int day,time_t dayt)
{
	int r1=-1,r2=-1;
	for(int i=0;i<MAXRECORDS;i++)
	{
		if (recordingtimer[i].timer==ct1) r1=i;
		if (recordingtimer[i].timer==ct2) r2=i;
	}
	if (r1>=0 && r2>=0)
	{
		int minx2=recordingtimer[r1].x2;
		int maxx1=recordingtimer[r2].x1;
		if (recordingtimer[r2].x1>maxx1) maxx1=recordingtimer[r2].x1;
		if (recordingtimer[r1].x2<minx2) minx2=recordingtimer[r1].x2;
		if (maxx1>minx2)
		{
			int z=maxx1;
			maxx1=minx2;
			minx2=z;
		}
		osd->DrawRectangle(maxx1,recordingtimer[r1].y1,minx2,recordingtimer[r1].y2,clrRed);
		osd->DrawRectangle(maxx1,recordingtimer[r2].y1,minx2,recordingtimer[r2].y2,clrRed);
	}
}

void checkerOsd::genGraphs(int day)
{
	char const *navtxt;
	if (blinker)
	{
		blinker->setTimers(NULL);
		blinker->setShow(0,NULL);
		blinker->LockOSD();
	}
	navtxt=tr("Cursor up/down/left/right+Nums");

	osd->DrawRectangle(areas[0].x1, areas[0].y1, areas[0].x2, areas[0].y2, clrGray50);
	osd->DrawText(x0_graph+width_graph-cFont::GetFont(fontSml)->Width(navtxt),y0_graph,navtxt,clrWhite,clrBlue,cFont::GetFont(fontSml));

	for(int i=0;i<10;i++)  // crash fix
		quicktimer[i]=NULL; // crash fix

	if (Timers.Count()>0)
	{
		int graph_border=15;
		int x,y,x2,y2;
		char buf[100];
		int m;
		struct tm day_tm;
		time_t dayt;

		y=today.tm_year+1900;
		m=(day<today.tm_mday?today.tm_mon+1:today.tm_mon)+1;
		if (m>12)
		{
			m=1;
			y++;
		}
		day_tm.tm_year=y-1900;
		day_tm.tm_mon=m-1;
		day_tm.tm_mday=day;
		day_tm.tm_hour=0;
		day_tm.tm_min=0;
		day_tm.tm_sec=0;
		day_tm.tm_isdst=0;
		dayt=mktime(&day_tm);
		localtime_r(&dayt,&day_tm);
		day=day_tm.tm_mday;
		m=day_tm.tm_mon+1;
		y=day_tm.tm_year+1900;
		
		sprintf(buf,"%04d-%02d-%02d %s",y,m,day,(day==today.tm_mday?tr("(today)"):""));
		osd->DrawText(x0_graph+10,y0_graph+5,buf,clrCyan,clrGray50,cFont::GetFont(fontOsd));

		for(int i=0;i<25;i+=3)
		{
			x=x0_graph+graph_border+(i*(width_graph-graph_border*2)/24);
			sprintf(buf,"%d",i);
			osd->DrawText(x-cFont::GetFont(fontOsd)->Width(buf)/2,y0_graph+5+30+cFont::GetFont(fontOsd)->Height(),buf,clrCyan,clrGray50,cFont::GetFont(fontOsd));
			osd->DrawRectangle(x,y0_graph+5+20+cFont::GetFont(fontOsd)->Height(),x,y0_graph+5+30+cFont::GetFont(fontOsd)->Height(),clrBlack);
		}
		osd->DrawRectangle(x0_graph+graph_border,y0_graph+5+cFont::GetFont(fontOsd)->Height(),x0_graph+width_graph-graph_border,y0_graph+5+20+cFont::GetFont(fontOsd)->Height(),clrBlack);

		cTimer *ct1;
		struct paintdata *pd=new paintdata[2401];
		for(int i=0;i<MAXRECORDS;i++)
			recordingtimer[i].timer=NULL;
		for(int i=0;i<2401;i++)
		{
			pd[i].count=0;
			pd[i].filled=0;
		}

		int cnt=1,reccnt=0;
		Timers.Sort();
		for(int run=0;run<2;run++)
		{
			ct1=Timers.First();
			while (ct1)
			{
				int day1=cTimer::GetMDay(ct1->Day());
				if (!ct1->IsSingleEvent())
				{
					if (ct1->DayMatches(dayt))
					{
						day1=day;
					}
					else
					{
						day1=0;
					}
				}
				int start1=ct1->Start();
				int stop1=ct1->Stop();
				int day11=(stop1<start1?day1+1:day1);

				if (day1==day)
				{
					if (day11!=day1)
						stop1=2400;

					if (run==0)
					{
						for(int i=start1;i<stop1;i++)
							pd[i].count++;
					}
					else if (run==1)
					{
						x=x0_graph+graph_border+(start1*(width_graph-graph_border*2)/24/100);
						x2=x0_graph+graph_border+(stop1*(width_graph-graph_border*2)/24/100);

						int max=0;
						int mask=0;
						for(int i=start1;i<stop1;i++)
						{
							if (pd[i].count>max) max=pd[i].count;
							mask|=pd[i].filled;
						}

						int h=(20-(max-1))/max;
						int p=0;
						for(int i=0;i<max;i++)
						{
							if (!(mask & (1<<i)))
							{
								for(int j=start1;j<stop1;j++)
								{
									pd[j].filled|=1<<i;
								}
								if (i&1)
								{ // ungerade: von unten
									p=20-((i+1)/2*h)-(i/2);
								}
								else
								{ // gerade: von oben
									p=(h+1)*(i/2);
								}
								break;
							}
						}

						y=y0_graph+5+1+p+cFont::GetFont(fontOsd)->Height();
						y2=y0_graph+5+p+h-1+cFont::GetFont(fontOsd)->Height();
						tColor col=ct1->Recording()?clrGrey:clrYellow;
						osd->DrawRectangle(x,y,x2,y2,col);

						if (cnt<11)
						{
							sprintf(buf,"%d",(cnt>9?0:cnt));
							osd->DrawText(x2-cFont::GetFont(fontSml)->Width(buf),y0_graph+5+30+cFont::GetFont(fontOsd)->Height()*2-(cnt&1?0:19),buf,clrYellow,clrGray50,cFont::GetFont(fontSml));
							quicktimer[cnt-1]=ct1;
							cnt++;
						}
						if (reccnt<MAXRECORDS)
						{
							recordingtimer[reccnt].timer=ct1;
							recordingtimer[reccnt].x1=x;
							recordingtimer[reccnt].y1=y;
							recordingtimer[reccnt].x2=x2;
							recordingtimer[reccnt].y2=y2;
							reccnt++;
						}
					}
				}
				else if (day11==day)
				{
					if (day11!=day1)
						start1=0;

					if (run==0)
					{
						for(int i=start1;i<stop1;i++)
							pd[i].count++;
					}
					else if (run==1)
					{
						x=x0_graph+graph_border+(start1*(width_graph-graph_border*2)/24/100);
						x2=x0_graph+graph_border+(stop1*(width_graph-graph_border*2)/24/100);

						int max=0;
						int mask=0;
						for(int i=start1;i<stop1;i++)
						{
							if (pd[i].count>max) max=pd[i].count;
							mask|=pd[i].filled;
						}
                                                
                                                int h = 0;
                                                if(max)  //hot fix: avoid division by zero
                                                {
						    h=(20-(max-1))/max;
                                                }
						int p=0;
						for(int i=0;i<max;i++)
						{
							if (!(mask & (1<<i)))
							{
								for(int j=start1;j<stop1;j++)
								{
									pd[j].filled|=1<<i;
								}
								if (i&1)
								{ // ungerade: von unten
									p=((i+1)/2*h)-(i/2);
								}
								else
								{ // gerade: von oben
									p=(h+1)*(i/2);
								}
								break;
							}
						}

						y=y0_graph+5+1+p+cFont::GetFont(fontOsd)->Height();
						y2=y0_graph+5+p+h-1+cFont::GetFont(fontOsd)->Height();
						tColor col=ct1->Recording()?clrGrey:clrYellow;
						osd->DrawRectangle(x,y,x2,y2,col);

						if (cnt<11)
						{
							sprintf(buf,"%d",(cnt>9?0:cnt));
							osd->DrawText(x2-cFont::GetFont(fontSml)->Width(buf),y0_graph+5+30+cFont::GetFont(fontOsd)->Height()*2-(cnt&1?0:19),buf,clrYellow,clrGray50,cFont::GetFont(fontSml));
							quicktimer[cnt-1]=ct1;
							cnt++;
						}
						if (reccnt<MAXRECORDS)
						{
							recordingtimer[reccnt].timer=ct1;
							recordingtimer[reccnt].x1=x;
							recordingtimer[reccnt].y1=y;
							recordingtimer[reccnt].x2=x2;
							recordingtimer[reccnt].y2=y2;
							reccnt++;
						}
					}
				}
				ct1=Timers.Next(ct1);
			}
		}

		genLines(day,dayt);

		osd->DrawRectangle(x0_graph+graph_border,y0_graph+5+cFont::GetFont(fontOsd)->Height(),x0_graph+width_graph-graph_border,y0_graph+5+cFont::GetFont(fontOsd)->Height(),clrBlue);
		osd->DrawRectangle(x0_graph+graph_border,y0_graph+5+20+cFont::GetFont(fontOsd)->Height(),x0_graph+width_graph-graph_border,y0_graph+5+20+cFont::GetFont(fontOsd)->Height(),clrBlue);
		osd->DrawRectangle(x0_graph+graph_border,y0_graph+5+cFont::GetFont(fontOsd)->Height(),x0_graph+graph_border,y0_graph+5+20+cFont::GetFont(fontOsd)->Height(),clrBlue);
		osd->DrawRectangle(x0_graph+width_graph-graph_border,y0_graph+5+cFont::GetFont(fontOsd)->Height(),x0_graph+width_graph-graph_border,y0_graph+5+20+cFont::GetFont(fontOsd)->Height(),clrBlue);
		osd->Flush();
		showTimer(1);
		if (blinker && reccnt>0)
			blinker->setTimers(recordingtimer);
	}
	else
		osd->Flush();
	if (blinker)
		blinker->UnlockOSD();
}
void checkerOsd::showTimer(int timer)
{
	if (timer<11 && timer>0)
	{
		if (quicktimer[timer-1])
		{
			if (blinker)
			{
				blinker->setShow(timer,quicktimer[timer-1]);
				blinker->LockOSD();
			}
			char buf[2000];
			sprintf(buf,"%d: %s",(timer>9?0:timer),quicktimer[timer-1]->File());
			osd->DrawRectangle(x0_graph,y0_graph+5+30+cFont::GetFont(fontOsd)->Height()*3-5,x0_graph+width_graph,y0_graph+5+30+cFont::GetFont(fontOsd)->Height()*4,clrGray50);
			osd->DrawText(x0_graph+5,y0_graph+5+30+cFont::GetFont(fontOsd)->Height()*3-5,buf,clrYellow,clrGray50,cFont::GetFont(fontSml));
			osd->Flush();
			if (blinker)
				blinker->UnlockOSD();
		}
	}
}
void checkerOsd::genText(int sline)
{
	if (blinker)
		blinker->LockOSD();
	osd->DrawRectangle(areas[1].x1, areas[1].y1, areas[1].x2, areas[1].y2, clrGray50);
	int y=y0_text;
	for(int i=sline;i<totalLines;i++)
	{
		char *t=lines[i];

		if (t)
		{
			tColor col=clrWhite;
			osd->DrawText(x0_text+5, y, t,col,clrGray50,cFont::GetFont(fontOsd));
			if (i % 3==0)
				osd->DrawRectangle(x0_text,y+cFont::GetFont(fontOsd)->Height()-2,x0_text+90,y+cFont::GetFont(fontOsd)->Height(),clrBlack);
			if ((i+1) % 3==0)
			{
				osd->DrawRectangle(x0_text,y+cFont::GetFont(fontOsd)->Height()+3,x0_text+width_text,y+cFont::GetFont(fontOsd)->Height()+6,clrTransparent);
				y+=10;
			}
			y+=cFont::GetFont(fontOsd)->Height()+1;
		}
	}
	osd->Flush();
	if (blinker)
		blinker->UnlockOSD();
}
void checkerOsd::Show(void)
{
	osd = cOsdProvider::NewTrueColorOsd(50, 50, 0, 0);
  	if (osd)
	{
		time_t tmt;
		time(&tmt);
		localtime_r(&tmt,&today);
		
		startLine=0;
		showDay=-1;
		totalLines=0;
		textLines=9; //9;

		height_graph=cFont::GetFont(fontOsd)->Height()*6;
		height_text=(cFont::GetFont(fontOsd)->Height()+1)*textLines;
		width_graph=590;
		width_text=590; //cOsd::CellWidth()*40;
		x0_graph=x0_text=10;
		y0_graph=20+cFont::GetFont(fontOsd)->Height()+5+8;
		y0_text=20+cFont::GetFont(fontOsd)->Height()+5+8+height_graph+8;
		int width_title=cFont::GetFont(fontOsd)->Width(tr("Timeline"))+20;

		genLines();

//		osd->Create(0, 240, 612, 250, 4);

		tArea a[sizeof(areas) / sizeof(tArea)] = {
				{ x0_graph, y0_graph, x0_graph+width_graph-1, y0_graph+height_graph-1,  32},
				{ x0_text, y0_text, x0_text+width_text-1, y0_text+height_text-1, 32 },
				{ x0_graph, 20, x0_graph+width_title-1, 20+cFont::GetFont(fontOsd)->Height()+5-1, 32 }
		};
		memcpy(areas, a, sizeof(areas));
#if 0
		for (uint i = 0; i < sizeof(areas) / sizeof(tArea); ++i) {
			while ((areas[i].x2 - areas[i].x1 + 1) % (8 / areas[i].bpp) != 0)
				++areas[i].x2;
			while ((areas[i].y2 - areas[i].y1 + 1) % (8 / areas[i].bpp) != 0)
				++areas[i].y2;
		}
#endif
		osd->SetAreas(areas, sizeof(areas) / sizeof(tArea));

		for (uint i = 0; i < sizeof(areas) / sizeof(tArea); ++i)
			osd->DrawRectangle(areas[i].x1, areas[i].y1, areas[i].x2, areas[i].y2, clrGray50);
		
		osd->DrawText(20,20, tr("Timeline"),clrWhite,clrGray50,cFont::GetFont(fontOsd));
		osd->DrawRectangle(10,20+cFont::GetFont(fontOsd)->Height(),50,20+cFont::GetFont(fontOsd)->Height()+1,clrBlue);
		osd->DrawRectangle(20+cFont::GetFont(fontOsd)->Width(tr("Timeline"))+10-40,21,20+cFont::GetFont(fontOsd)->Width(tr("Timeline"))+10,22,clrBlue);
		osd->Flush();

		blinker=new blinkThread(this);
		blinker->Start();

		genText(startLine);
		genGraphs(showDay);
	}
}
eOSState checkerOsd::ProcessKey(eKeys Key)
{
	eOSState state = cOsdObject::ProcessKey(Key);

	if (state == osUnknown)
	{
		switch (Key & ~k_Repeat)
		{
			case kOk:
				return osEnd;
            case kBack:
				return osEnd;
			case kLeft:
				showDay--;
				if (showDay<1)
					showDay=31;
				genGraphs(showDay);
				break;
			case kRight:
				showDay++;
				if (showDay>31)
					showDay=1;
				genGraphs(showDay);
				break;
			case kUp:
				startLine-=3;
				if (startLine<0)
					startLine=0;
				genText(startLine);
				break;
			case kDown:
				startLine+=3;
				if (startLine>=totalLines-3)
					startLine=totalLines-3;
				if (startLine<0)
					startLine=0;
				genText(startLine);
				break; 
			case kRed:
				break;
			case k0:
				showTimer(10);
				break;			
			case k1:
				showTimer(1);
				break;			
			case k2:
				showTimer(2);
				break;			
			case k3:
				showTimer(3);
				break;			
			case k4:
				showTimer(4);
				break;			
			case k5:
				showTimer(5);
				break;			
			case k6:
				showTimer(6);
				break;			
			case k7:
				showTimer(7);
				break;			
			case k8:
				showTimer(8);
				break;			
			case k9:
				showTimer(9);
				break;			
			default:
				return state;
		}
		state = osContinue;
	}
	return state;
}

bool checkerOsd::hasConflicts(void)
{
	bool ret=false;
	checkerOsd *c=new checkerOsd();
	c->genLines();
	ret=c->totalLines>2;
	delete c;
	return ret;
}

#include <stdarg.h>

void tllog(int level, const char *fmt, ...)
{
	auto char    t[BUFSIZ];
	auto va_list ap;
	int minlevel=0;

	va_start(ap, fmt);

	if (level>=minlevel)
	{
		vsnprintf(t + 10, sizeof t - 10, fmt, ap);
		memcpy(t, "timeline: ", 10);
		syslog(LOG_DEBUG, "%s", t);
	}
	va_end(ap);
}
