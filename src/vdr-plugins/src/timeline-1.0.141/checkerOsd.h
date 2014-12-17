/*
 * checkerOsd.h: Internationalization
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: checkerOsd.h,v 1.9 2006/06/18 11:46:31 schmitzj Exp $
 *
 */

#ifndef CHECKEROSD_H
#define CHECKEROSD_H

#include <vdr/plugin.h>
#include <time.h>
#include "config.h"

void tllog(int level, const char *fmt, ...);

struct recordingdata
{
	cTimer	*timer;
	int		x1;
	int		y1;
	int		x2;
	int		y2;
};
struct paintdata
{
	unsigned char	count;
	unsigned char	filled;
};

#define MAXRECORDS	20
class checkerOsd : public cOsdObject
{
	friend class blinkThread;
	cPlugin *parent;
	cOsd *osd;

	enum eDvbFont smallFont;
	class cDevice *devices[MAXDEVICES];
	int           frequencies[MAXDEVICES];

	struct tm today;
	int startLine;
	int showDay;
	int lines_c;
	int totalLines;
	char **lines;
	int width_graph;
	int width_text;
	int height_graph;
	int height_text;
	int x0_graph;
	int x0_text;
	int y0_graph;
	int y0_text;
	tArea areas[3];
	int textLines;
	cTimer *quicktimer[10];
	struct recordingdata recordingtimer[MAXRECORDS];
	class blinkThread *blinker;

	void genLines(int day=0,time_t dayt=0);
	void genGraphs(int day);
	void genText(int sline);
	void showTimer(int);
	void addText(char *l1,char *l2,char *l3,int &si);
	void paintOver(cTimer *ct1,cTimer *ct2,int day,time_t dayt);

public:
	checkerOsd(class cPlugin *);
	checkerOsd(void);
	virtual ~checkerOsd();
	virtual void Show(void);
	virtual eOSState ProcessKey(eKeys Key);

	static bool hasConflicts(void);
};

#endif
