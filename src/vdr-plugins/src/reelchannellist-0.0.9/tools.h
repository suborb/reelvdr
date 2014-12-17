#ifndef TOOLS_H
#define TOOLS_H

#include <vdr/tools.h>
#include <vdr/channels.h>
#include <vdr/epg.h>
#include <vdr/keys.h>

int CompareBySourceName(const void *a, const void *b);


/** *********************** cMySourceList***************************************
*** cMySourceList should hold list of sources, represented by numbers
*** (see cSource in vdr ), without any duplicates
*******************************************************************************/
class cMySourceList : public cVector<int> {
public:
  cMySourceList (int Allocated = 10): cVector<int>(Allocated) {}
  virtual ~cMySourceList ();
  int Find(int code) const;
  void Sort(void) { cVector<int>::Sort(CompareBySourceName); }
  virtual void Clear(void);
  };


/** group/bouquet name of channel, given channel */
const char* ChannelInBouquet(const cChannel*, cChannels&);

const cEvent* GetPresentEvent(const cChannel* channel);


char NumpadChar(eKeys Key);
char NumpadToChar(unsigned int k, unsigned int multiples=0);

#endif // TOOLS_H
