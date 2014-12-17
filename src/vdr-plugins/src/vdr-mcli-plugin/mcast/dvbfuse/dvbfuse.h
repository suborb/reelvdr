#ifndef _INCLUDE_DVBFUSE_H
#define _INCLUDE_DVBFUSE_H

#ifdef WIN32
#define inline __inline
typedef unsigned long long uint64_t;
#endif

#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 29
#define _FILE_OFFSET_BITS 64
#endif

#include <fuse/fuse.h>

#include "headers.h"

#define MAXAPIDS 32
#define MAXDPIDS 4
#define MAXCAIDS 2

typedef struct
{
	int number;
	char *name;
	char *provider;
	char *shortName;
	int type;
	unsigned int frequency;
	int srate;
	int coderateH;
	int coderateL;
	int guard;
	int polarization;
	int inversion;
	int modulation;
	int source;
	int transmission;
	int bandwidth;
	int hierarchy;
	int vpid;
	int ppid;
	int sid;
	int rid;
	int tid;
	int tpid;
	int nid;
	int caids[MAXCAIDS];
	int NumCaids;
	int apids[MAXAPIDS];
	int NumApids;
	int dpids[MAXDPIDS];
	int NumDpids;
	int eitpids[1];
	int NumEitpids;
	int sdtpids[1];
	int NumSdtpids;
} channel_t;


typedef struct
{
	char *name;
	channel_t **channels;
	int total;
} channel_group_t;

typedef struct
{
	recv_info_t *r;
	char *buffer;
	int head, tail, curr;
	int called;
	int si_state;
	psi_buf_t psi;
	channel_t *cdata;
	int pmt_pid;
	int es_pids[32];
	int es_pidnum;
	int fta;
	pthread_t t;
	int stop;
	int ts_cnt;
	pthread_mutex_t lock_ts;
	pthread_mutex_t lock_bf;
} stream_info_t;

extern int channel_groups;
extern channel_group_t *groups;

#define get_channel_num(g) groups[g].total
#define get_group_num() channel_groups
#define get_channel_name(g,n) groups[g].channels[n]->name
#define get_group_name(n) groups[n].name
#define get_channel_data(g,n) groups[g].channels[n]

// dvbfuse.c
int start_fuse (int argc, char *argv[], int debug);

// mcilink.c
void mcli_startup (int debug);
void* mcli_stream_setup (int group, int channel);
size_t mcli_stream_read (void* handle, char *buf, size_t len, off_t offset);
int mcli_stream_stop (void* handle);

// parse.c
int ParseLine (const char *s, channel_t * ch,char *charset, int idx, char *ext);
int ParseGroupLine (const char *s, channel_group_t * gr,char *charset);

#endif
