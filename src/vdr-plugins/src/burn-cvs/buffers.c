#include <sys/select.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#define KILOBYTE(x) ((x) * 1024)
#define MEGABYTE(x) (KILOBYTE(x) * 1024)

struct chain_buffer
{
	char *data;
	int used, size;

	struct chain_buffer *next;
};

int main()
{
	struct chain_buffer *first = NULL, *last = NULL;
	int size = 0;
	fd_set rfds, wfds;
	int eof = 0;

	fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
	fcntl(STDOUT_FILENO, F_SETFL, O_NONBLOCK);

	while (!eof) {
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);

		if (size < MEGABYTE(32))
			FD_SET(STDIN_FILENO, &rfds);

		if (first != NULL)
			FD_SET(STDOUT_FILENO, &wfds);

		select(STDOUT_FILENO + 1, &rfds, &wfds, NULL, NULL);

		if (FD_ISSET(STDIN_FILENO, &rfds)) {
			struct chain_buffer *next =
				(struct chain_buffer*)malloc(sizeof(struct chain_buffer));
			int r;

			memset(next, 0, sizeof(struct chain_buffer));
			next->data = (char*)malloc(sizeof(char) * BUFSIZ);
			if ((r = read(STDIN_FILENO, next->data, BUFSIZ)) > 0) {
				next->size = r;
				size += r;
				next->data = (char*)realloc(next->data, sizeof(char) * r);

				if (first == NULL)
					first = last = next;
				else {
					last->next = next;
					last = next;
				}
			} else if (r < 0) {
				fprintf(stderr, "Read error: %m\n");
				exit(1);
			} else
				eof = 1;
		}

		if (FD_ISSET(STDOUT_FILENO, &wfds)) {
			int w;

			if ((w = write(STDOUT_FILENO, first->data + first->used,
			               first->size - first->used)) >= 0) {
				first->used += w;

				if (first->used == first->size) {
					free(first->data);
					size -= first->size;

					if (first->next == NULL) {
						free(first);
						first = last = NULL;
					} else {
						struct chain_buffer *next = first->next;
						free(first);
						first = next;
					}
				}
			} else if (w < 0) {
				fprintf(stderr, "Write error: %m\n");
				exit(1);
			}
		}
	}
	exit(0);
}
