
/*
 * Some general functions are define here
 * str2time_t ()
 * crc16 ()
 *
 * declared in channelData.h
 */


#include <time.h>
#include <stdio.h>



/********* str2time_t *************************************/
time_t str2time_t (const char *parmTime, const char *parmDate = NULL, int jumpDays = 0)
{
/*
 *   parmTime is "1130" not "11:30"
 *   parmDate is "dd/mm/yyyy" for eg. "17/11/2007"
 *
 */
  time_t t1;
  char trailer;
  t1 = time (NULL);
  struct tm when;               /* = *localtime(&t1); */

  if (parmDate != NULL)
  {
    sscanf (parmDate, " %d/%d/%d%c", &(when.tm_mday), &(when.tm_mon), &(when.tm_year), &trailer);
    if (when.tm_year >= 1900)
      when.tm_year -= 1900;
    when.tm_mon--;
    when.tm_isdst = 0;
    when.tm_yday = 0;
  }
  else
  {
    when = *localtime (&t1);
  }
  sscanf (parmTime, " %d%c", &(when.tm_hour), &trailer);
  when.tm_sec = 0;
  when.tm_min = when.tm_hour % 100;
  when.tm_hour /= 100;
//    showTM(when);

  return mktime (&when) + (time_t) (24 * 60 * 60 * jumpDays);
}



/*******CRC16 ****************************************/

#define POLY 0xA001             // CRC16

unsigned int crc16 (unsigned int crc, unsigned char const *p, int len)
{
  while (len--)
  {
    crc ^= *p++;
    for (int i = 0; i < 8; i++)
      crc = (crc & 1) ? (crc >> 1) ^ POLY : (crc >> 1);
  }
  return crc & 0xFFFF;
}
