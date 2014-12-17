#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "dvbfuse.h"

#ifdef __MINGW32__
#include <getopt.h>
#endif

struct dvbfs_config
{
	char *iface;
	char *chnfile;
	char *chset;
	char *ext;
	int debug;
	int epgsdt;
};

static struct dvbfs_config conf=
{
	NULL,
	"channels.conf",
	NULL,
	"ts",
	0,
};

#ifndef WIN32

#ifndef offsetof
#include <stddef.h>
#endif

#define DVBFS_OPT(t, p, v) { t, offsetof(struct dvbfs_config, p), v }

enum
{
	KEY_HELP,
	KEY_VERSION,
	KEY_KEEP_OPT,
};

static struct fuse_opt dvbfs_opts[] =
{
	DVBFS_OPT("-i %s",		iface, 0),
	DVBFS_OPT("-c %s",		chnfile, 0),
	DVBFS_OPT("-x %s",		chset, 0),
	DVBFS_OPT("-e %s",		ext, 0),
	DVBFS_OPT("-p",			epgsdt, 1),
	FUSE_OPT_KEY("-h",		KEY_HELP),
	FUSE_OPT_KEY("--help",		KEY_HELP),
	FUSE_OPT_KEY("-V",		KEY_VERSION),
	FUSE_OPT_KEY("--version",	KEY_VERSION),
	FUSE_OPT_KEY("-d",		KEY_KEEP_OPT),
	FUSE_OPT_KEY("debug",		KEY_KEEP_OPT),
	FUSE_OPT_END
};

#endif

channel_t *channels=NULL;
channel_group_t *groups=NULL;
int channel_num=0;
int channel_max_num=0;
int channel_groups=0;
int channel_groups_max=0;

extern cmdline_t cmd;
/*-------------------------------------------------------------------------*/
channel_t * read_channel_list(char *filename,char *chset,char *ext)
{
	int i;
	int j;
	FILE *f;
	char buf[512];

	f=fopen(filename,"r");
	if (!f) {
		printf("Can't read %s: %s\n",filename,strerror(errno));
		return NULL;
	}

	while(fgets(buf,512,f)) {
		if (channel_num==channel_max_num) {
			channel_max_num+=200;
			channels=(channel_t*)realloc(channels,
				channel_max_num*sizeof(channel_t));
			if (!channels) {
				printf("Out of memory\n");
				exit(1);
			}
		}
		if (channel_groups+1>=channel_groups_max) {
			channel_groups_max+=20;
			groups=(channel_group_t*)realloc(groups,
				channel_groups_max*sizeof(channel_group_t));
			if (!groups) {
				printf("Out of memory\n");
				exit(1);
			}
			for (i=channel_groups_max-20;i<channel_groups_max;i++) {
				groups[i].name=NULL;
				groups[i].channels=NULL;
				groups[i].total=0;
			}
		}
		if (ParseLine(buf,&channels[channel_num],chset,
			groups[channel_groups].total+1,ext)) {
			if(conf.epgsdt) {
				channels[channel_num].NumEitpids = 1;
				channels[channel_num].eitpids[0] = 0x12;
				channels[channel_num].NumSdtpids = 1;
				channels[channel_num].sdtpids[0] = 0x11;
			} else {
				channels[channel_num].NumEitpids = 0;
				channels[channel_num].NumSdtpids = 0;
			}
			if (!(groups[channel_groups].total&0xf)) {
				groups[channel_groups].channels=(channel_t **)
					realloc(groups[channel_groups].channels,
					sizeof(channel_t *)*
					(groups[channel_groups].total+0x10));
				if (!groups[channel_groups].channels) {
					printf("Out of memory\n");
					exit(1);
				}
			}
			groups[channel_groups].
				channels[groups[channel_groups].total++]=
					(void *)((long)channel_num);
			channel_num++;
		}
		else if (ParseGroupLine(buf,&groups[channel_groups+1],chset)) {
			if (!groups[channel_groups].total) {
				if(groups[channel_groups].name)
					free(groups[channel_groups].name);
				groups[channel_groups].name=
					groups[channel_groups+1].name;
				groups[channel_groups+1].name=NULL;
			} else
				channel_groups++;
		}
	}
	if (!groups[channel_groups].total) {
		if(groups[channel_groups].name)
			free(groups[channel_groups].name);
		groups[channel_groups].name=groups[channel_groups+1].name;
		groups[channel_groups+1].name=NULL;
	} else
		channel_groups++;
	for (i=0;i<channel_groups;i++) {
		for (j=0;j<groups[i].total;j++) {
			groups[i].channels[j]=
				&channels[(long)groups[i].channels[j]];
		}
	}
	for (i=0;i<channel_groups;i++) {
		if (!groups[i].name)
			continue;
		for (j=i+1;j<channel_groups;j++) {
			if (!strcmp(groups[i].name,groups[j].name)) {
				printf("duplicate group %s\n",groups[i].name);
				exit(1);
			}
		}
	}
	if(conf.debug)
		printf("Read %i channels and %i groups\n",channel_num,
			channel_groups);
	fclose(f);
	return channels;
}
/*-------------------------------------------------------------------------*/
void usage (void)
{
	printf("\nUsage: dvbfuse [options] mountpoint\n\n");
	printf("General options:\n\n");
	printf("    -h, --help             display this help and exit\n");
#ifndef WIN32
	printf("    -V, --version          output version information and "
		"exit\n\n");
#endif
	printf("DVBFS options:\n\n");
	printf("    -i                     Netceiver network interface\n");
	printf("    -c                     configuration file\n");
	printf("    -e                     stream file extension\n");
	printf("    -x                     filesystem charset\n");
	printf("    -p                     include program guide\n\n");
}
/*-------------------------------------------------------------------------*/
#ifndef WIN32
static int dvbfs_opt_proc(void *data, const char *arg, int key,
	struct fuse_args *outargs)
{
	switch(key) {
		case KEY_HELP:
			usage();
			fuse_opt_add_arg(outargs, "-ho");
			start_fuse(outargs->argc, outargs->argv, conf.debug);
			exit(1);

		case KEY_VERSION:
			printf("DVBFS version 1.0.0\n");
			fuse_opt_add_arg(outargs, "--version");
			start_fuse(outargs->argc, outargs->argv, conf.debug);
			exit(0);

		case KEY_KEEP_OPT:
			if(!strcmp(arg,"-d")||!strcmp(arg,"debug"))
				conf.debug=1;
			break;
	}
	return 1;
}
#endif
/*-------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
#ifndef WIN32
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	if (fuse_opt_parse(&args, &conf, dvbfs_opts, dvbfs_opt_proc)) {
		fprintf(stderr, "Error parsing options.\n");
		return 1;
	}
#else
	int argn=0;
	int c;

	while ((c=getopt(argc,argv,"i:c:e:x:p"))!=-1) {
		switch (c) {
		case 'i':
			conf.iface=optarg;
			argn+=2;
			break;
		case 'c':
			conf.chnfile=optarg;
			argn+=2;
			break;
		case 'e':
			conf.ext=optarg;
			argn+=2;
			break;
		case 'x':
			conf.chset=optarg;
			argn+=2;
			break;
		case 'p':
			conf.epgsdt=1;
			argn+=1;
			break;
		default:
			goto done;
		}
	}

done:	argc-=argn;
	argv+=argn;
#endif
	if (conf.iface) {
		strncpy(cmd.iface, conf.iface, IFNAMSIZ);
		cmd.iface[IFNAMSIZ-1]=0;
	}

	if (!read_channel_list(conf.chnfile,conf.chset,conf.ext))
		return 1;

	mcli_startup(conf.debug);
	
#ifndef WIN32
	start_fuse(args.argc, args.argv, conf.debug);

	fuse_opt_free_args(&args);
#else
	start_fuse(argc, argv, conf.debug);
#endif

	return 0;
}
/*-------------------------------------------------------------------------*/
