#include <defs.h>

int poll(struct pollfd *p,int n,int tmo)
{
    struct timeval timeout, *tptr;
    fd_set in, out, err, *pin, *pout;
    int i, m;

    FD_ZERO(&err);
    for (i = 0, m = -1, pout = pin = NULL; i < n; i++) {
	p[i].revents = 0;
	if (p[i].fd < 0)
		continue;
	if (p[i].fd > m)
		m = p[i].fd;
	if (p[i].events & (POLLIN|POLLPRI)) {
		if (!pin) {
			FD_ZERO(&in);
			pin = &in;
		}
		FD_SET(p[i].fd, pin);
	}
	if (p[i].events & POLLOUT) {
		if (!pout) {
    			FD_ZERO(&out);
			pout = &out;
		}
		FD_SET(p[i].fd, pout);
	}
	FD_SET(p[i].fd, &err);
    }
    if (tmo < 0)
	tptr = NULL;
    else {
	tptr = &timeout;
	timeout.tv_sec = tmo / 1000;
	timeout.tv_usec = (tmo % 1000) * 1000;
    }

    i = select(m+1, pin, pout, &err, tptr);
    if (i <= 0)
	return i;

    for (i = 0, m = 0; i < n; i++) {
	if (p[i].fd < 0)
		continue;
	if ((p[i].events & (POLLIN|POLLPRI)) && FD_ISSET(p[i].fd, pin))
		p[i].revents |= POLLIN;
	if ((p[i].events & POLLOUT) && FD_ISSET(p[i].fd, pout))
		p[i].revents |= POLLOUT;
	if (FD_ISSET(p[i].fd, &err))
		p[i].revents |= POLLHUP;
	if (p[i].revents)
		m++;
    }
    return m;
}
