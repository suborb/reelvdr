/*
 * See the files COPYING and README for copyright information and how to reach
 * the author.
 *
 *  $Id: etsi-const.h,v 1.2 2006/06/25 18:20:27 lordjaxom Exp $
 */

#ifndef VDR_ETSI_CONST_H
#define VDR_ETSI_CONST_H

#include <string>

/* Defines some constants from DVB standards */

namespace etsi
{
   /* ETSI EN 300 468 */

   /* Possible values of the stream content descriptor
    */
   enum stream_content {
      sc_reserved = 0x00,
      sc_video    = 0x01,
      sc_audio    = 0x02,
      sc_subtitle = 0x03
   };

   /* Possible values of the component type descriptor
    */

   enum component_type_video {
      ctv_reserved      = 0x00,

      // video types
      ctv_hz25_4_3      = 0x01,
      ctv_hz25_16_9_pan = 0x02,
      ctv_hz25_16_9     = 0x03,
      ctv_hz25_21_9     = 0x04,
      ctv_hz30_4_3      = 0x05,
      ctv_hz30_16_9_pan = 0x06,
      ctv_hz30_16_9     = 0x07,
      ctv_hz30_21_9     = 0x08
   };

   enum component_type_audio {
      cta_reserved      = 0x00,

      // audio types
      cta_mono           = 0x01,
      cta_dualchannel    = 0x02,
      cta_stereo         = 0x03,
      cta_multilang      = 0x04,
      cta_surround       = 0x05
   };

}

#endif // VDR_ETSI_CONST_H
