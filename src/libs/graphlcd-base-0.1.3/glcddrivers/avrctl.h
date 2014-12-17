/*
 * GraphLCD driver library
 *
 * avrctl.h  -  AVR controlled LCD driver class
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2005 Andreas Regel <andreas.regel AT powarman.de>
 */

#ifndef _GLCDDRIVERS_AVRCTL_H_
#define _GLCDDRIVERS_AVRCTL_H_

#include "driver.h"

namespace GLCD
{

class cDriverConfig;
class cSerialPort;

class cDriverAvrCtl : public cDriver
{
private:
  cSerialPort * port;
	unsigned char ** newLCD; // wanted state
	unsigned char ** oldLCD; // current state
	cDriverConfig * config;
	cDriverConfig * oldConfig;
	int refreshCounter;

  int WaitForAck(void);
  void CmdSysSync(void);
  void CmdDispClearScreen(void);
  void CmdDispSwitchScreen(void);
  void CmdDispSetBrightness(unsigned char percent);
  void CmdDispSetRowData(unsigned char column, unsigned char offset, unsigned short length, unsigned char * data);

	int CheckSetup();

public:
	cDriverAvrCtl(cDriverConfig * config);
	virtual ~cDriverAvrCtl();

	virtual int Init();
	virtual int DeInit();

	virtual void Clear();
	virtual void Set8Pixels(int x, int y, unsigned char data);
	virtual void Refresh(bool refreshAll = false);
};

} // end of namespace

#endif
