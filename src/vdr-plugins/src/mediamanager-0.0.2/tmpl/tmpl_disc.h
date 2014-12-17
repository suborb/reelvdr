#ifndef _MEDIA_#HANDLER#_DISC_H
#define _MEDIA_#HANDLER#_DISC_H

#include <vdr/i18n.h>
#include <vdr/tools.h>

class cMedia#HANDLERNAME#Track : public cListObject {
  private:
  public:
	cMedia#HANDLERNAME#Track();
	~cMedia#HANDLERNAME#Track();
};

class cMedia#HANDLERNAME#Disc : public cList<cMedia#HANDLERNAME#Track> {
  private:
  public:
	cMedia#HANDLERNAME#Disc();
	~cMedia#HANDLERNAME#Disc();
};

#endif
