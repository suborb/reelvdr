#include "reelcamlink.h"

void PrepareReLink(struct ReLink *link, const cDevice *dev, short op)
{
  memcpy(link->magic,magic,sizeof(link->magic));
  link->op  = op; 
  link->dev = dev;
  link->num = -1;
}

int DoReLinkOp(struct ReLink *link)
{
  cPlugin *p=cPluginManager::GetPlugin(RE_NAME);
  if(p) {
    p->SetupParse((const char *)link,"");
    return link->num;
    }
  return -1;
}

int GetSlotOnDev(const cDevice *dev)
{
  struct ReLink link;
  cPlugin *p = cPluginManager::GetPlugin(RE_NAME);
  if(p){
    PrepareReLink(&link, dev, OP_GETSLOT);
    DoReLinkOp(&link);
    return link.slotOnDev;
  }
  return -1;
}

int ResetSlot(int Slot)
{
  struct ReLink link;
  cPlugin *p = cPluginManager::GetPlugin(RE_NAME);
  if(p){
    memcpy(link.magic,magic,sizeof(link.magic));
    link.op  = OP_RESET;
    link.num = -1;
    link.slot = Slot;
    DoReLinkOp(&link);
  } 
  return -1;
}

