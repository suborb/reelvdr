#ifndef __RADIO_TOOLS_H
#define __RADIO_TOOLS_H


unsigned short crc16_ccitt(unsigned char *daten, int len, bool skipfirst);

char *rtrim(char *text);

#endif //__RADIO_TOOLS_H
