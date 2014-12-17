#include "dvbfuse.h"

#define PREFETCH 131072

#if ULONG_MAX > 0xffffffffUL
#define conv_t uint64_t
#define FSIZE 0x7fffffffffffffffLL
#else
#define conv_t uint32_t
#define FSIZE 0x7fffffffL
#endif

struct dvbfile
{
	int group;
	int channel;
	int error;
	off_t offset;
	void *handle;
};

/*-------------------------------------------------------------------------*/

static int dvbfuse_getattr (const char *path, struct stat *stbuf)
{
	int num, g, cnum, n;
	char *p;

	memset (stbuf, 0, sizeof (struct stat));

	if (!strcmp (path, "/")) {
		stbuf->st_mode = S_IFDIR | 0555;
		stbuf->st_nlink = 2;
		return 0;
	}

	path++;
	num=get_group_num ();

	for (g=0;g<num;g++) {
		p=get_group_name (g);
		if(!p) {
			cnum=get_channel_num(g);

			for (n=0;n<cnum;n++) {
				p=get_channel_name (g,n);
				if (!strcmp(path,p)) {
					stbuf->st_mode = S_IFREG | 0444;
					stbuf->st_nlink = 1;
					stbuf->st_size = FSIZE;
					return 0;
				}
			}
			continue;
		}
		n=strlen(p);
		if (!strncmp(path,p,n)) {
			if(!path[n]) {
				stbuf->st_mode = S_IFDIR | 0555;
				stbuf->st_nlink = 2;
				return 0;
			}
			if (path[n]!='/')
				continue;

			path+=n+1;
			num=get_channel_num (g);

			for (n=0;n<num;n++) {
				p=get_channel_name (g,n);
				if (!strcmp(path,p)) {
					stbuf->st_mode = S_IFREG | 0444;
					stbuf->st_nlink = 1;
					stbuf->st_size = FSIZE;
					return 0;
				}
			}

			return -ENOENT;
		}
	}

	return -ENOENT;
}

/*-------------------------------------------------------------------------*/

static int dvbfuse_readdir (const char *path, void *buf, fuse_fill_dir_t filler,
	off_t offset, struct fuse_file_info *fi)
{
	int gnum, g, cnum, n;
	char *p;

	if (!strcmp(path,"/")) {
		filler (buf, ".", NULL, 0);
		filler (buf, "..", NULL, 0);
		gnum=get_group_num();
		for (g=0;g<gnum;g++) {
			p=get_group_name (g);
			if (p)
				filler (buf,p,NULL,0);
			else {
				cnum=get_channel_num(g);
				for (n=0;n<cnum;n++)
					filler (buf,get_channel_name(g,n),
						NULL,0);
			}
		}
		return 0;
	}

	path++;
	gnum=get_group_num();
	for (g=0;g<gnum;g++) {
		p=get_group_name (g);
		if(!p)
			continue;
		if(!strcmp(path,p)) {
			filler (buf, ".", NULL, 0);
			filler (buf, "..", NULL, 0);
			cnum=get_channel_num (g);
			for (n=0;n<cnum;n++)
				filler (buf,get_channel_name(g,n),NULL,0);

			return 0;
		}
	}

	return -ENOENT;
}

/*-------------------------------------------------------------------------*/

static int dvbfuse_open (const char *path, struct fuse_file_info *fi)
{
	int num, g, cnum, n;
	struct dvbfile *f;
	char *p;

	path++;
	num=get_group_num ();

	for (g=0;g<num;g++) {
		p=get_group_name (g);
		if(!p) {
			cnum=get_channel_num(g);

			for (n=0;n<cnum;n++) {
				p=get_channel_name (g,n);
				if (!strcmp(path,p))
					goto match;
			}
			continue;
		}
		n=strlen(p);
		if (!strncmp(path,p,n)) {
			if(!path[n])
				return -EISDIR;
			if (path[n]!='/')
				continue;

			path+=n+1;
			num=get_channel_num (g);

			for (n=0;n<num;n++) {
				p=get_channel_name (g,n);
				if (!strcmp(path,p))
					goto match;
			}

			return -ENOENT;
		}
	}

	return -ENOENT;

match:	if (fi->flags&(O_APPEND|O_WRONLY|O_RDWR|O_CREAT|O_EXCL|O_TRUNC))
		return -EACCES;

	if(!(f=malloc(sizeof(struct dvbfile))))
		return -ENOMEM;

	memset(f,0,sizeof(struct dvbfile));

	f->group=g;
	f->channel=n;

	fi->fh=(conv_t)f;
#ifndef __MINGW32__
	fi->nonseekable=1;
#endif
	fi->keep_cache=0;
	fi->direct_io=0;

	return 0;
}

/*-------------------------------------------------------------------------*/
static int dvbfuse_release (const char *path, struct fuse_file_info *fi)
{
	struct dvbfile *f;

	if((f=(struct dvbfile *)(conv_t)fi->fh)) {
		if (f->handle)
			mcli_stream_stop (f->handle);
		free(f);
	}

	return 0;
}
/*-------------------------------------------------------------------------*/

static int dvbfuse_read (const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	size_t len;
	struct dvbfile *f;

	if (!(f=(struct dvbfile *)(conv_t)fi->fh))
		return -ENOENT;

	if (f->error)
		return f->error;

	if (!f->handle) {
		f->handle = mcli_stream_setup (f->group,f->channel);
		if (!f->handle) {
			f->error=-ENOMEM;
			return -ENOMEM;
		}
	}

	if (offset>f->offset)
		goto fail;

	len = mcli_stream_read (f->handle,buf,size,f->offset-offset);

	if (len==-1) {
fail:		mcli_stream_stop (f->handle);
		f->handle=NULL;
		f->error=-EIO;
		return -EIO;
	}

	f->offset+=len;

	return len;
}

/*-------------------------------------------------------------------------*/

static void *dvbfuse_init(struct fuse_conn_info *conn) {
	conn->async_read=0;
#ifndef __MINGW32__
	conn->want=0;
#endif
	conn->max_readahead=PREFETCH;
	return NULL;
}

/*-------------------------------------------------------------------------*/

static struct fuse_operations dvbfuse_oper = {
	.getattr = dvbfuse_getattr,
	.readdir = dvbfuse_readdir,
	.open    = dvbfuse_open,
	.release = dvbfuse_release,
	.read    = dvbfuse_read,
	.init    = dvbfuse_init,
#ifndef __MINGW32__
	.flag_nullpath_ok=1,
#endif
};

/*-------------------------------------------------------------------------*/
int start_fuse (int argc, char *argv[], int debug)
{
	if(debug)
		printf ("Starting fuse\n");
	return fuse_main (argc, argv, &dvbfuse_oper, NULL);
}
