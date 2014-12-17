/*
   provide compatibility between S2API (DVB API V5) and older Reel S2 API
   RC, 24.08.2010
*/

#ifdef DVBAPI_V5
//FIXME: really ugly hack to extend enum fe_type
#define FE_DVBS2 (FE_ATSC+1)

//DVB V5: PSK_8+4 = DQPSK+1 = one more than last modulation
#define QPSK_S2 (PSK_8+4)

#else // for API V3 + Reel S2 extensions

//      API V5     Reel
#define ROLLOFF_20 FE_ROLLOFF_20
#define ROLLOFF_25 FE_ROLLOFF_25
#define ROLLOFF_35 FE_ROLLOFF_35

#define PSK_8 PSK8

#endif
