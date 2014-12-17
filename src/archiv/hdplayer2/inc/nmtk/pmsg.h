/*
 *  Copyright (C) 2004-2005 Nathan Lutchansky <lutchann@litech.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <nmtk/list.h>

struct hdrf {
	char *name;
	char *value;
};

#define MAX_FIELDS	32
#define TOKEN_PUNC_SET		"-.!%*_+`'~"
#define UNRESERVED_PUNC_SET	"-_.!~*'()"

struct pmsg {
	unsigned char *msg;
	int max_len;
	int msg_len;
	struct hdrf fields[MAX_FIELDS];
	int header_count;
	enum { PMSG_REQ, PMSG_RESP } type;
	char *proto_id;
	union {
		struct {
			char *method;
			char *uri;
			char *branch;
		} req;
		struct {
			int code;
			char *reason;
		} stat;
	} sl;
	unsigned char *body;
	int body_len;
	struct list_head list;
	int send_result;
	void (*send_done_func)(struct pmsg *pmsg);
	void *app_data;
};

typedef enum {
	URI_UNKNOWN,
	URI_ERROR,
	URI_FILE,
	URI_HTTP,
	URI_FTP,
	URI_RTSP,
	URI_SIP,
	URI_RAWUDP,
	URI_SHM,
} uri_proto_t;

struct digest_auth_info {
	char *algorithm;
	char *realm;
	char *username;
	char *method;
	char *uri;
	char *nonce;
	char *opaque;
	char *response;
};

uri_proto_t uri_parse(char *uri, char *username, int username_size,
		char *password, int password_size, char *hostname,
		int hostname_size, int *port, char *path, int path_size);
int uri_construct(char *dest, int dest_size, char *base, char *sub);
char *add_pmsg_string(struct pmsg *msg, const char *s);
int parse_pmsg(struct pmsg *msg);
char *get_header(struct pmsg *msg, char *name);
int digest_auth_parse(char *c, struct digest_auth_info *auth,
		char *buf, int size);
int add_header(struct pmsg *msg, char *name, char *value);
int add_header_printf(struct pmsg *msg, char *name, char *fmt, ...);
int replace_header(struct pmsg *msg, char *name, char *value);
int copy_headers(struct pmsg *dest, struct pmsg *src, char *name);
int digest_authorize(struct digest_auth_info *auth,
		char *password, char *response);
int get_param(char *value, char *tag, char *dest, int size);
int get_param_num(char *value, char *tag, int *arg);
void init_pmsg(struct pmsg *pmsg, unsigned char *msg, int len);
struct pmsg *new_pmsg(int size);
int copy_pmsg(struct pmsg *dst, struct pmsg *src);
void free_pmsg(struct pmsg *msg);
void print_pmsg(struct pmsg *msg, FILE *fp);
