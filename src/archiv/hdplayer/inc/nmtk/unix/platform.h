#ifdef NEED_NMTK_CONFIG_H
#include <nmtk/unix/config.h>
#endif

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <limits.h>
#include <byteswap.h>

#define INLINE			inline
#define _O_BINARY		0

typedef pthread_mutex_t nmtk_mutex_t;

static INLINE void nmtk_mutex_init(nmtk_mutex_t *mutex)
{
	pthread_mutex_init(mutex, NULL);
}

static INLINE void nmtk_mutex_destroy(nmtk_mutex_t *mutex)
{
	pthread_mutex_destroy(mutex);
}

static INLINE void nmtk_mutex_lock(nmtk_mutex_t *mutex)
{
	pthread_mutex_lock(mutex);
}

static INLINE void nmtk_mutex_unlock(nmtk_mutex_t *mutex)
{
	pthread_mutex_unlock(mutex);
}

static INLINE struct tm *nmtk_localtime(const time_t *timep, struct tm *result)
{
	return localtime_r(timep, result);
}

static INLINE int nmtk_set_nonblock(int fd, int nonblock)
{
	long flags;

	flags = fcntl(fd, F_GETFL);
	if (flags < 0)
		return -1;
	if (nonblock)
		flags |= O_NONBLOCK;
	else
		flags &= ~O_NONBLOCK;
	return fcntl(fd, F_SETFL, flags);
}
