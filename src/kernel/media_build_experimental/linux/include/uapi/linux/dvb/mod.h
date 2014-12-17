#ifndef _UAPI_DVBMOD_H_
#define _UAPI_DVBMOD_H_

#include <linux/types.h>
#include "frontend.h"

struct dvb_mod_params {
	__u32 base_frequency;
	__u32 attenuator; 
};

struct dvb_mod_channel_params {
	enum fe_modulation modulation;

	__u32 rate_increment;
	
};


#define DVB_MOD_SET              _IOW('o', 208, struct dvb_mod_params)
#define DVB_MOD_CHANNEL_SET      _IOW('o', 209, struct dvb_mod_channel_params)

#endif /*_UAPI_DVBMOD_H_*/
