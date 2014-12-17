#ifndef __RIPITOSD_H
#define __RIPITOSD_H

#include <string>
#include <vdr/plugin.h>
#include <vdr/status.h>
#include "setup.h"
#include <vector>
#include <sys/stat.h>

class cUpdateOsd : public cOsdMenu {
private:
  unsigned int offset;
  std::vector<std::string> lines;
  struct stat fstats; 
  time_t old_time;
  bool filechanged;
public:
  cUpdateOsd();
  ~cUpdateOsd();
  void Start_Update(void);
  void Abort_Update(void);
  virtual void Display(void);
  virtual eOSState ProcessKey(eKeys Key);
  bool Update_On(void);
  };

extern cUpdateOsd *updateosd;

#endif //__RIPITOSD_H
