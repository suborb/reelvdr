/* Excerpt from new DVB-API dvb_frontend.h
   for cx24116 backport

#include <gpl.h>
*/


/*
 *      Delivery Systems
 *      needs to set/queried for multistandard frontends
 */
enum dvbfe_delsys {
        DVBFE_DELSYS_DVBS               = (1 <<  0),
        DVBFE_DELSYS_DSS                = (1 <<  1),
        DVBFE_DELSYS_DVBS2              = (1 <<  2),
        DVBFE_DELSYS_DVBC               = (1 <<  3),
        DVBFE_DELSYS_DVBT               = (1 <<  4),
        DVBFE_DELSYS_DVBH               = (1 <<  5),
        DVBFE_DELSYS_ATSC               = (1 <<  6),
        DVBFE_DELSYS_DUMMY              = (1 << 31)
};
#define DVBFE_GET_DELSYS                _IOR('o', 82, enum dvbfe_delsys)

/*
 *      Modulation types
 */
enum dvbfe_modulation {
        DVBFE_MOD_NONE                  = (0 <<  0),
        DVBFE_MOD_BPSK                  = (1 <<  0),
        DVBFE_MOD_QPSK                  = (1 <<  1),
        DVBFE_MOD_OQPSK                 = (1 <<  2),
        DVBFE_MOD_8PSK                  = (1 <<  3),
        DVBFE_MOD_16APSK                = (1 <<  4),
        DVBFE_MOD_32APSK                = (1 <<  5),
        DVBFE_MOD_QAM4                  = (1 <<  6),
        DVBFE_MOD_QAM16                 = (1 <<  7),
        DVBFE_MOD_QAM32                 = (1 <<  8),
        DVBFE_MOD_QAM64                 = (1 <<  9),
        DVBFE_MOD_QAM128                = (1 << 10),
        DVBFE_MOD_QAM256                = (1 << 11),
        DVBFE_MOD_QAM512                = (1 << 12),
        DVBFE_MOD_QAM1024               = (1 << 13),
        DVBFE_MOD_QAMAUTO               = (1 << 14),
        DVBFE_MOD_OFDM                  = (1 << 15),
        DVBFE_MOD_COFDM                 = (1 << 16),
        DVBFE_MOD_VSB8                  = (1 << 17),
        DVBFE_MOD_VSB16                 = (1 << 18),
        DVBFE_MOD_AUTO                  = (1 << 31)
};
/*
 *      Convolution Code Rate (Viterbi Inner Code Rate)
 *      DVB-S2 uses LDPC. Information on LDPC can be found at
 *      http://www.ldpc-codes.com
 */
enum dvbfe_fec {
        DVBFE_FEC_NONE                  = (0 <<  0),
        DVBFE_FEC_1_4                   = (1 <<  0),
        DVBFE_FEC_1_3                   = (1 <<  1),
        DVBFE_FEC_2_5                   = (1 <<  2),
        DVBFE_FEC_1_2                   = (1 <<  3),
        DVBFE_FEC_3_5                   = (1 <<  4),
        DVBFE_FEC_2_3                   = (1 <<  5),
        DVBFE_FEC_3_4                   = (1 <<  6),
        DVBFE_FEC_4_5                   = (1 <<  7),
        DVBFE_FEC_5_6                   = (1 <<  8),
        DVBFE_FEC_6_7                   = (1 <<  9),
        DVBFE_FEC_7_8                   = (1 << 10),
        DVBFE_FEC_8_9                   = (1 << 11),
        DVBFE_FEC_9_10                  = (1 << 12),
        DVBFE_FEC_AUTO                  = (1 << 31)
};

/*
 *      Frontend Inversion (I/Q Swap)
 */
enum dvbfe_inversion {
        DVBFE_INVERSION_OFF             = 0,
        DVBFE_INVERSION_ON,
        DVBFE_INVERSION_AUTO
};

/*
 *      Rolloff Rate (Nyquist Filter Rolloff)
 *      NOTE: DVB-S2 has rates of 0.20, 0.25, 0.35
 *      Values are x100
 *      Applies to DVB-S2
 */
enum dvbfe_rolloff {
        DVBFE_ROLLOFF_35                = (0 <<  0),
        DVBFE_ROLLOFF_25                = (1 <<  0),
        DVBFE_ROLLOFF_20                = (2 <<  0),
        DVBFE_ROLLOFF_UNKNOWN           = (3 <<  0)
};

/*
 *      DVB-S parameters
 */
struct dvbs_params {
        __u32                           symbol_rate;

        enum dvbfe_modulation           modulation;
        enum dvbfe_fec                  fec;
        enum dvbfe_rolloff              rolloff;
};
/*
 *      DSS parameters
 */
struct dss_params {
        __u32                           symbol_rate;

        enum dvbfe_modulation           modulation;
        enum dvbfe_fec                  fec;

        __u8                            pad[32];
};


/*
 *      DVB-S2 parameters
 */
struct dvbs2_params {
        __u32                           symbol_rate;

        enum dvbfe_modulation           modulation;
        enum dvbfe_fec                  fec;

        /*      Informational fields only       */
        __u8                            matype_1;
        __u8                            matype_2;

        __u8                            pad[32];
};
/*
 *      DVB Frontend Tuning Parameters
 */
struct dvbfe_params {
        __u32                           frequency;
        enum fe_spectral_inversion      inversion;
        enum dvbfe_delsys               delivery;

        __u8                            pad[32];

        union {
                struct dvbs_params      dvbs;
                struct dss_params       dss;
                struct dvbs2_params     dvbs2;

                __u8                    pad[128];
        } delsys;
};
                                                