/*
 * GraphLCD driver library
 *
 * avrctl.c  -  AVR controlled LCD driver class
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2005 Andreas Regel <andreas.regel AT powarman.de>
 */

#include <stdint.h>
#include <syslog.h>

#include "common.h"
#include "config.h"
#include "port.h"
#include "avrctl.h"


namespace GLCD
{

/* command header:
**  8 bits  sync byte (0xAA for sent commands, 0x55 for received commands)
**  8 bits  command id
** 16 bits  command length (excluding header)
*/
const unsigned char CMD_HDR_SYNC    = 0;
const unsigned char CMD_HDR_COMMAND = 1;
const unsigned char CMD_HDR_LENGTH  = 2;
const unsigned char CMD_DATA_START  = 4;

const unsigned char CMD_SYNC_SEND = 0xAA;
const unsigned char CMD_SYNC_RECV = 0x55;

const unsigned char CMD_SYS_SYNC = 0x00;
const unsigned char CMD_SYS_ACK  = 0x01;

const unsigned char CMD_DISP_CLEAR_SCREEN   = 0x10;
const unsigned char CMD_DISP_SWITCH_SCREEN  = 0x11;
const unsigned char CMD_DISP_SET_BRIGHTNESS = 0x12;
const unsigned char CMD_DISP_SET_ROW_DATA   = 0x13;


cDriverAvrCtl::cDriverAvrCtl(cDriverConfig * config)
:	config(config)
{
	oldConfig = new cDriverConfig(*config);

	port = new cSerialPort();
  
//	width = config->width;
//	height = config->height;
	refreshCounter = 0;
}

cDriverAvrCtl::~cDriverAvrCtl()
{
	delete port;
	delete oldConfig;
}

int cDriverAvrCtl::Init()
{
	int x;

	width = config->width;
	if (width <= 0)
		width = 256;
	height = config->height;
	if (height <= 0)
		height = 128;

	for (unsigned int i = 0; i < config->options.size(); i++)
	{
		if (config->options[i].name == "")
		{
		}
	}

	// setup lcd array (wanted state)
	newLCD = new unsigned char*[width];
	if (newLCD)
	{
		for (x = 0; x < width; x++)
		{
			newLCD[x] = new unsigned char[(height + 7) / 8];
			memset(newLCD[x], 0, (height + 7) / 8);
		}
	}
	// setup lcd array (current state)
	oldLCD = new unsigned char*[width];
	if (oldLCD)
	{
		for (x = 0; x < width; x++)
		{
			oldLCD[x] = new unsigned char[(height + 7) / 8];
			memset(oldLCD[x], 0, (height + 7) / 8);
		}
	}

  if (config->device == "")
  {
    return -1;
  }
  if (port->Open(config->device.c_str()) != 0)
    return -1;

	*oldConfig = *config;

	// clear display
	Clear();

	syslog(LOG_INFO, "%s: AvrCtl initialized.\n", config->name.c_str());
	return 0;
}

int cDriverAvrCtl::DeInit()
{
	int x;
	// free lcd array (wanted state)
	if (newLCD)
	{
		for (x = 0; x < width; x++)
		{
			delete[] newLCD[x];
		}
		delete[] newLCD;
	}
	// free lcd array (current state)
	if (oldLCD)
	{
		for (x = 0; x < width; x++)
		{
			delete[] oldLCD[x];
		}
		delete[] oldLCD;
	}

	if (port->Close() != 0)
		return -1;
	return 0;
}

int cDriverAvrCtl::CheckSetup()
{
	if (config->device != oldConfig->device ||
	    config->width != oldConfig->width ||
	    config->height != oldConfig->height)
	{
		DeInit();
		Init();
		return 0;
	}

	if (config->upsideDown != oldConfig->upsideDown ||
	    config->invert != oldConfig->invert)
	{
		oldConfig->upsideDown = config->upsideDown;
		oldConfig->invert = config->invert;
		return 1;
	}
	return 0;
}

void cDriverAvrCtl::Clear()
{
	for (int x = 0; x < width; x++)
		memset(newLCD[x], 0, (height + 7) / 8);
}

void cDriverAvrCtl::Set8Pixels(int x, int y, unsigned char data)
{
  if (x >= width || y >= height)
    return;

  if (!config->upsideDown)
  {
    int offset = 7 - (y % 8);
    for (int i = 0; i < 8; i++)
    {
      newLCD[x + i][y / 8] |= ((data >> (7 - i)) << offset) & (1 << offset);
    }
  }
  else
  {
    x = width - 1 - x;
    y = height - 1 - y;
    int offset = 7 - (y % 8);
    for (int i = 0; i < 8; i++)
    {
      newLCD[x - i][y / 8] |= ((data >> (7 - i)) << offset) & (1 << offset);
    }
  }
}

void cDriverAvrCtl::Refresh(bool refreshAll)
{
	int x;
  int y;
  int i;
  int num = 128;
  unsigned char data[16*num];

	if (CheckSetup() == 1)
		refreshAll = true;

	if (config->refreshDisplay > 0)
	{
		refreshCounter = (refreshCounter + 1) % config->refreshDisplay;
		if (!refreshAll && !refreshCounter)
		refreshAll = true;
	}

  refreshAll = true;
  if (refreshAll)
  {
    for (x = 0; x < width; x += num)
    {
      for (i = 0; i < num; i++)
      {
        for (y = 0; y < (height + 7) / 8; y++)
        {
          data[i * ((height + 7) / 8) + y] = (newLCD[x + i][y]) ^ (config->invert ? 0xff : 0x00);
        }
        memcpy(oldLCD[x + i], newLCD[x + i], (height + 7) / 8);
      }
      CmdDispSetRowData(x, 0, 16 * num, data);
    }
    CmdDispSwitchScreen();
    // and reset RefreshCounter
    refreshCounter = 0;
	}
	else
	{
		// draw only the changed bytes
	}
}

int cDriverAvrCtl::WaitForAck(void)
{
  uint8_t cmd[4];
  int len;
  int timeout = 10000;

  len = 0;
  while (len < 4 && timeout > 0)
  {
    len += port->ReadData(&cmd[len]);
    timeout--;
  }
  if (timeout == 0)
    return 0;
  return 1;
}

void cDriverAvrCtl::CmdSysSync(void)
{
  uint8_t cmd[4];

  cmd[CMD_HDR_SYNC] = CMD_SYNC_SEND;
  cmd[CMD_HDR_COMMAND] = CMD_SYS_SYNC;
  cmd[CMD_HDR_LENGTH] = 0;
  cmd[CMD_HDR_LENGTH+1] = 0;

  port->WriteData(cmd, 4);
  WaitForAck();
}

void cDriverAvrCtl::CmdDispClearScreen(void)
{
  uint8_t cmd[4];

  cmd[CMD_HDR_SYNC] = CMD_SYNC_SEND;
  cmd[CMD_HDR_COMMAND] = CMD_DISP_CLEAR_SCREEN;
  cmd[CMD_HDR_LENGTH] = 0;
  cmd[CMD_HDR_LENGTH+1] = 0;

  port->WriteData(cmd, 4);
  WaitForAck();
}

void cDriverAvrCtl::CmdDispSwitchScreen(void)
{
  uint8_t cmd[4];

  cmd[CMD_HDR_SYNC] = CMD_SYNC_SEND;
  cmd[CMD_HDR_COMMAND] = CMD_DISP_SWITCH_SCREEN;
  cmd[CMD_HDR_LENGTH] = 0;
  cmd[CMD_HDR_LENGTH+1] = 0;

  port->WriteData(cmd, 4);
  WaitForAck();
}

void cDriverAvrCtl::CmdDispSetBrightness(uint8_t percent)
{
  uint8_t cmd[5];

  cmd[CMD_HDR_SYNC] = CMD_SYNC_SEND;
  cmd[CMD_HDR_COMMAND] = CMD_DISP_SET_BRIGHTNESS;
  cmd[CMD_HDR_LENGTH] = 0;
  cmd[CMD_HDR_LENGTH+1] = 1;
  cmd[CMD_DATA_START] = percent;

  port->WriteData(cmd, 5);
  WaitForAck();
}

void cDriverAvrCtl::CmdDispSetRowData(uint8_t column, uint8_t offset, uint16_t length, uint8_t * data)
{
  uint8_t cmd[2560];

  cmd[CMD_HDR_SYNC] = CMD_SYNC_SEND;
  cmd[CMD_HDR_COMMAND] = CMD_DISP_SET_ROW_DATA;
  cmd[CMD_HDR_LENGTH] = (length + 4) >> 8;
  cmd[CMD_HDR_LENGTH+1] = (length + 4);
  cmd[CMD_DATA_START] = column;
  cmd[CMD_DATA_START+1] = offset;
  cmd[CMD_DATA_START+2] = length >> 8;
  cmd[CMD_DATA_START+3] = length;
  memcpy(&cmd[CMD_DATA_START+4], data, length);

  port->WriteData(cmd, length+8);
  WaitForAck();
}

}
