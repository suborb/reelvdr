#ifndef _POLL_H_H_
#define _POLL_H_H_

struct pollfd {
	int fd;
	short events;
	short revents;
};

#define POLLIN 001
#define POLLPRI 002
#define POLLOUT 004
#define POLLNORM POLLIN
#define POLLERR 010
#define POLLHUP 020
#define POLLNVAL 040

DLL_SYMBOL int poll(struct pollfd *, int, int);

#endif /* _POLL_H_H_ */
