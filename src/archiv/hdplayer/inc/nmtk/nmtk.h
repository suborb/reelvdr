/*
 *  Copyright (C) 2004-2007 Nathan Lutchansky <lutchann@litech.org>
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

struct ref_lock {
	nmtk_mutex_t lock;
	struct ref_counter *ref_counter;
};

struct timeline;

void random_bytes(unsigned char *dest, int len);
void random_id(unsigned char *dest, int len);
unsigned int random_range(unsigned int base, unsigned int count);
void reduce_factors(int *a, int *b);
int base64_to_bin(unsigned char *dest, unsigned char *src, int len);
int bin_to_base64(char *dest, unsigned char *src, int len);
int hex_to_bin(unsigned char *dest, char *src, int len);
int parse_skip(char **start, char *skip_set);
int parse_num(char **ptr, char *skip_set, int *value);
int parse_range(struct timeline *timeline, char *range);
int measure_word_length(char **start, char *skip_set, char *punc_set);
int copy_words_or_quoted(char *c, int spaces,
		char *dest, int dest_size, char *punc_set);
int lang_normalize(char *dest, int max_size, char *src);
int lang_test(char *desired, char *candidate);
void nmtk_hexdump(unsigned char *d, int len);

typedef unsigned long track_id_t;
typedef unsigned long long media_clock_t;

typedef enum {
	FORMAT_NONE,
	FORMAT_UNSUPPORTED,
	FORMAT_METADATA,
	FORMAT_TYPE_MUX			= 0x01000000,
	FORMAT_AVI,
	FORMAT_WAV,
	FORMAT_MOV,
	FORMAT_M2TS,
	FORMAT_MPG,
	FORMAT_RTP,
	FORMAT_TYPE_VIDEO		= 0x02000000,
	FORMAT_RAWVIDEO,
	FORMAT_MPV,
	FORMAT_M4V,
	FORMAT_H264,
	FORMAT_H263,
	FORMAT_JPEG,
	FORMAT_TYPE_AUDIO		= 0x04000000,
	FORMAT_RAWAUDIO,
	FORMAT_MPA,
	FORMAT_M4A,
	FORMAT_AC3,
	FORMAT_AMR,
	FORMAT_TYPE_MASK		= 0xff000000
} media_format_t;

#define IS_FORMAT_MUX(x)	((x) & FORMAT_TYPE_MUX)
#define IS_FORMAT_VIDEO(x)	((x) & FORMAT_TYPE_VIDEO)
#define IS_FORMAT_AUDIO(x)	((x) & FORMAT_TYPE_AUDIO)

typedef enum {
	PARAM_SUBFORMAT,
	PARAM_RESOLUTION,
	PARAM_CROP_AREA,
	PARAM_PIXEL_ASPECT_RATIO,
	PARAM_OVERSCAN_ENCODED,
	PARAM_FRAMEPERIOD,
	PARAM_STREAM_CLOCK,
	PARAM_RTP_CLOCK,
	PARAM_CHANNELS,
	PARAM_PALI,
	PARAM_CONFIG_SET,
	PARAM_FMTP,
	PARAM_CNAME,
	PARAM_FILE_PROPS,
	PARAM_MIME_TYPE,
	PARAM_LAST_TIMESTAMP,
} media_param_t;

typedef enum {
	RAWAUDIO_S16LE,
	RAWAUDIO_S16BE,
	RAWAUDIO_ALAW,
	RAWAUDIO_ULAW,
	RAWAUDIO_S8,
	RAWAUDIO_U8,
	RAWAUDIO_U16LE,
	RAWAUDIO_U16BE,
} rawaudio_subformat_t;

typedef enum {
	MPV_MPEG1,
	MPV_MPEG2,
} mpv_subformat_t;

struct resolution {
	unsigned int width;
	unsigned int height;
};

struct crop_area {
	unsigned int x_off;
	unsigned int y_off;
	unsigned int width;
	unsigned int height;
};

struct pixel_aspect_ratio {
	unsigned short pixel_width;
	unsigned short pixel_height;
};

struct frameperiod {
	unsigned int numerator;
	unsigned int denominator;
};

typedef enum {
	RA_UNSUPPORTED		= 0x00,
	RA_OVERHEAD_LOW		= 0x01,
	RA_OVERHEAD_MEDIUM	= 0x02,
	RA_OVERHEAD_HIGH	= 0x04,
	RA_SUPPORTED		= 0x07,
	RA_CANON_TRANS_TS	= 0x08,
	RA_KEY_FRAME_MODE	= 0x10,
} ra_flags_t;

struct file_props {
	off_t file_length;
	ra_flags_t seek_overhead;
};

struct timeline {
	media_clock_t timescale;
	media_clock_t start;
	media_clock_t end;
};

struct mpeg4_generic_params {
	int constantSize;
	int sizeLength;
	int constantDuration;
	int indexLength;
	int indexDeltaLength;
	int CTSDeltaLength;
	int DTSDeltaLength;
	int randomAccessIndication;
};

static media_clock_t inline extend_counter(media_clock_t clock,
		media_clock_t last, int valid_bits)
{
	clock |= last & ~((1ULL << valid_bits) - 1);
	if (clock + (1ULL << valid_bits) +
				(1ULL << (valid_bits - 2)) > last &&
			clock + (1ULL << valid_bits) -
				(1ULL << (valid_bits - 2)) < last)
		return clock + (1ULL << valid_bits);
	if (clock - (1ULL << valid_bits) +
				(1ULL << (valid_bits - 2)) > last &&
			clock - (1ULL << valid_bits) -
				(1ULL << (valid_bits - 2)) < last)
		return clock - (1ULL << valid_bits);
	return clock;
}

static media_clock_t inline timeval_to_clock(struct timeval *tv)
{
	return (media_clock_t)tv->tv_sec * 1000000ULL +
		(media_clock_t)tv->tv_usec;
}

static void inline timeval_add(struct timeval *tv1, struct timeval *tv2)
{
	tv1->tv_sec += tv2->tv_sec;
	tv1->tv_usec += tv2->tv_usec;
	if (tv1->tv_usec >= 1000000) {
		tv1->tv_usec -= 1000000;
		++tv1->tv_sec;
	}
}

static void inline timeval_subtract(struct timeval *tv1, struct timeval *tv2)
{
	tv1->tv_sec -= tv2->tv_sec;
	if (tv1->tv_usec < tv2->tv_usec) {
		--tv1->tv_sec;
		tv1->tv_usec = 1000000 - tv2->tv_usec + tv1->tv_usec;
	} else
		tv1->tv_usec -= tv2->tv_usec;
}

static int inline timeval_diff(struct timeval *second, struct timeval *first)
{
	return (second->tv_sec - first->tv_sec) * 1000 +
			(second->tv_usec - first->tv_usec) / 1000;
}

struct media_timestamp {
	enum {
		TS_STREAM = 0x10,
		TS_TRANSPORT = 0x20,
		TS_RECEIVED = 0x40,
	} valid;
	media_clock_t stream;
	media_clock_t transport;
	media_clock_t received;
};

#define MAX_SDP_TRACKS	128
#define MAX_SDP_FORMATS	128

struct sdp;
struct media_track;
struct media_parser;

struct media_program {
	track_id_t id;
	media_format_t format;
	struct timeline timeline;
	struct sdp *sdp;
	struct media_track *metadata_track;
	int ended;
	media_clock_t start_time;
	media_clock_t end_time;
	int track_count;
	struct media_track *tracks[MAX_SDP_TRACKS];
};

struct media_track_slot {
	struct media_track_slot *next;
	struct media_track_slot *prev;
	struct media_track_slot *last_meta;
	int ref_count;
	enum {
		TRACK_REFRESH,
		TRACK_DESCR,
		TRACK_FRAME,
		TRACK_EOS_MARKER,
	} type;
	union {
		struct {
			media_format_t format;
		} refresh;
		struct {
			media_param_t param;
			void *value;
			int length;
		} descr;
		struct {
			struct media_parser *parser;
			unsigned long long seqno;
			struct media_timestamp ts;
			unsigned int format_data;
			unsigned int buffer_fraction; /* millionths */
		} frame;
	};
};

struct rtp_header {
	int payload;
	int marker;
	unsigned int seqno;
	unsigned int ssrc;
	media_clock_t timestamp;
};

struct read_position {
	long long offset;
	unsigned char *dest;
	int bytes_min;
	int bytes_max;
	int bytes_complete;
	enum {
		FLAG_FRAME_START = 1 << 0,
		FLAG_FRAME_END = 1 << 1,
		FLAG_STREAM_END = 1 << 2,
		FLAG_TIMESTAMP_VALID = 1 << 3,
	} flags;
	int ring_size;
	int ring_offset;
	media_clock_t timestamp;
};

#define FF_INC_CONFIG		(1 << 24)

struct media_parser_func {
	/* Input functions */
	void (*set_param)(media_param_t param, void *value, int value_len,
			void *data);
	int (*rtp_recv)(struct rtp_header *rh, struct media_timestamp *ts,
			unsigned char *d, int len, void *data);
	void (*parse)(struct read_position **pos, struct media_timestamp *ts,
			void *data);
	void (*resync)(struct read_position **pos, void *data);

	/* Output functions */
	int (*frame_length)(struct media_track_slot *slot, int format);
	int (*frame_copyv)(struct iovec *iov, int iov_count,
			struct media_track_slot *slot, int format);
	int (*frame_write)(int fd, struct media_track_slot *slot, int format);
	//int (*rtp_send)(struct media_frame_info *fi, struct rtp_endpoint *ep);
};

struct media_parser {
	struct ref_counter *ref_counter;
	struct media_parser_func *func;
	void *data;
};

struct frame_ring_entry {
	struct media_track_slot slot;
	struct frame_ring_entry *next;
	int offset;
	int length;
	int track_slot;
};

struct frame_ring {
	struct media_track *track;
	int buffer_size;
	void *entry_block_ptr;
	struct frame_ring_entry *next;
	struct frame_ring_entry *oldest;
	unsigned char *d;
};

void copy_to_ring(struct frame_ring *ring, int off, void *d, int len);
void copy_from_ring(struct frame_ring *ring, int off, void *d, int len);
void move_backward_on_ring(struct frame_ring *ring, int dst_off,
		int src_off, int len);
void move_forward_on_ring(struct frame_ring *ring, int dst_off,
		int src_off, int len);
int iov_to_iov(struct iovec *dst, int dst_count,
		struct iovec *src, int src_count);
int iov_create(struct iovec *iov, struct frame_ring *ring, int off, int len);
int read_from_mem(struct read_position *pos, int offset, void *d, int len);
int read_from_fd(struct read_position *pos, int fd);
int alloc_frame_ring(struct frame_ring *ring, int finfo_size, int frame_count);
void free_frame_ring(struct frame_ring *ring);
void prepare_ring_read(struct frame_ring *ring, struct read_position *pos,
		int max_bytes);
struct frame_ring_entry *get_next_ring_entry(struct frame_ring *ring);
void set_buffer_fraction(struct frame_ring *ring, struct frame_ring_entry *f);
unsigned int get_word(struct frame_ring *ring, int off);

static inline int bytes_in_ring(struct frame_ring *ring, int start, int end)
{
	if (start <= end)
		return end - start;
	else
		return ring->buffer_size - start + end;
}

static inline int offset_in_range(int start, int offset, int end)
{
	if (start <= end)
		return start <= offset && offset < end;
	else
		return start <= offset || offset < end;
}

int nmtk_rand_init(void);

int nmtk_init(int minlevel);

#define EXTENSION_MATCH_CONFIDENCE	2
#define MIME_TYPE_MATCH_CONFIDENCE	3

struct nmtk_client_context;
struct nmtk_file_read_context;

typedef struct media_parser *(*parser_new_t)(struct media_track *track);
typedef void (*reader_new_t)(struct nmtk_client_context *client,
		struct nmtk_file_read_context *file);
typedef int (*file_test_t)(char *filename, char *mime_type,
		unsigned char *start, int start_len, reader_new_t *new_func);

void nmtk_register_builtins(void);
void file_test_register(char *name, file_test_t test_func);
void mime_type_register(char *type, media_format_t format,
		parser_new_t new_func);
int file_test_match(char *filename, char *mime_type, unsigned char *start,
		int start_len, reader_new_t *new_func);
media_format_t mime_type_match(char *mime_type, parser_new_t *new_func);

/* Group owner functions */
struct media_track *media_track_new(track_id_t track_id,
		media_clock_t timescale, struct notifier *restart_notifier);
void media_track_refresh(struct media_track *track, media_format_t format,
		struct media_parser *parser);
void media_track_flush(struct media_track *track);
void media_track_del(struct media_track *track);

static inline void media_parser_set_param(struct media_parser *parser,
		media_param_t param, void *value, int value_len)
{
	parser->func->set_param(param, value, value_len, parser->data);
}

static inline void media_parser_set_param_num(struct media_parser *parser,
		media_param_t param, int value)
{
	parser->func->set_param(param, &value, sizeof(value), parser->data);
}

/* Parser functions */
void media_track_deliver_frame(struct media_track *track,
		struct media_track_slot *s);
int media_track_reclaim_frame(struct media_track *track,
		struct media_track_slot *s);
void media_track_set_eos(struct media_track *track);
void media_track_set_param(struct media_track *track, media_param_t param,
		void *value, int value_len);
static void inline media_track_set_param_num(struct media_track *track,
		media_param_t param, int value)
{
	media_track_set_param(track, param, &value, sizeof(value));
}

/* Destination functions */
int media_frame_length(struct media_track_slot *slot, int format);
int media_frame_copy(unsigned char *d, int len,
		struct media_track_slot *slot, int format);
int media_frame_copyv(struct iovec *iov, int iov_count,
		struct media_track_slot *slot, int format);
int media_frame_write(int fd, struct media_track_slot *slot, int format);

typedef enum {
	TRACK_READ_EMPTY = 0,
	TRACK_READ_DATA,
	TRACK_READ_OVERRUN,
} track_read_status_t;

track_id_t media_track_get_id(struct media_track *track);
media_clock_t media_track_get_timescale(struct media_track *track);
struct notifier *media_track_get_notifier(struct media_track *track);
void media_track_set_active(struct media_track *track, int active);
void media_track_set_min_fullness(struct media_track *track, int minimum);
track_read_status_t media_track_read(struct media_track *track,
		struct media_track_slot **data);
struct media_track_slot *media_track_find_refresh(struct media_track_slot *s);
struct media_track_slot *media_track_find_descr(struct media_track_slot *s,
		media_param_t param);
void media_track_unget_last(struct media_track *track);
void media_track_release_last(struct media_track *track);

struct raw_reader *raw_reader_new(struct nmtk_client_context *ctx,
		struct nmtk_file_read_context *file, ra_flags_t ra_flags,
		media_clock_t timescale, media_format_t format,
		struct media_track **track);
void raw_reader_start(struct raw_reader *raw, struct media_parser *parser);
void raw_reader_set_eos(struct raw_reader *raw);
void raw_reader_close(void *data);

struct media_parser *h264_parser_new(struct media_track *track,
		int size_length);
struct media_parser *m4v_parser_new(struct media_track *track,
		int reorder_subgop_timestamps);
struct media_parser *mpv_parser_new(struct media_track *track);
struct media_parser *jpeg_parser_new(struct media_track *track);
struct media_parser *h263_parser_new(struct media_track *track);
int mpeg4_parse_fmtp(struct media_parser *parser, char *fmtp,
		struct mpeg4_generic_params *mpeg4);
int mpeg4_generic_rtp_recv(struct media_parser *parser,
		struct mpeg4_generic_params *mpeg4, struct read_position *pos,
		struct rtp_header *rh, struct media_timestamp *ts,
		unsigned char *d, int len);
struct media_parser *m4a_parser_new(struct media_track *track);
struct media_parser *adts_parser_new(struct media_track *track);
struct media_parser *mpa_parser_new(struct media_track *track,
		struct frameperiod *ts_rate);
struct media_parser *mp3_file_parser_new(struct media_track *track);
struct media_parser *ac3_parser_new(struct media_track *track,
		struct frameperiod *ts_rate);
struct media_parser *amr_parser_new(struct media_track *track);
struct media_parser *rawaudio_parser_new(struct media_track *track,
		struct frameperiod *ts_rate);
int rawaudio_get_sample_size(rawaudio_subformat_t subformat);
media_format_t waveformat_parse(unsigned char *hdr, int len,
		struct media_track *track, struct frameperiod *ts_rate,
		struct media_parser **parser);

struct v4l2_capture *v4l2_capture_new(int fd, struct media_track **track,
		struct dispatcher *dispatcher);
int v4l2_capture_prepare(struct v4l2_capture *cap);
int v4l2_capture_start(struct v4l2_capture *cap);
int v4l2_capture_stop(struct v4l2_capture *cap);
void v4l2_capture_del(struct v4l2_capture *cap);

struct oss_device *oss_device_open(char *device, track_id_t track_id,
		struct dispatcher *dispatcher);
int oss_device_configure(struct oss_device *oss, rawaudio_subformat_t subformat,
		int samplerate, int channels);
struct media_track *oss_device_start(struct oss_device *oss,
		struct media_track *sync_track);
void oss_device_close(struct oss_device *oss);

typedef enum {
	NMTK_ADDR_UNSET = 0,
	NMTK_IPV4,
	NMTK_IPV6,
} nmtk_protocol_t;

struct nmtk_addr {
	nmtk_protocol_t proto;
	unsigned char addr[128];
	unsigned short port;
};

/*
 * These are normally defined when <sys/socket.h> is included, but with these
 * declarations we can avoid forcing all applications using NMTK to include it.
 */
struct sockaddr;
struct sockaddr_storage;

void nmtk_addr_to_sockaddr(struct nmtk_addr *addr,
		struct sockaddr_storage *sockaddr, socklen_t *addrlen, int *pf);
void sockaddr_to_nmtk_addr(struct nmtk_addr *addr,
		struct sockaddr *sockaddr);
int nmtk_addr_to_str(struct nmtk_addr *addr, char *dest, int dest_size);
int nmtk_addr_cmp(struct nmtk_addr *a, struct nmtk_addr *b);

struct sdp {
	struct ref_counter *ref_counter;
	char *hdr;
	int hdr_len;
	int track_count;
	struct sdp_track *tracks;
};

#define LANGUAGE_SIZE 64

struct sdp_track {
	track_id_t id;
	media_format_t common_format;
	char *hdr;
	int hdr_len;
	char language[LANGUAGE_SIZE];
	struct nmtk_addr addr;
	char *control_uri;
	int format_count;
	struct sdp_format *formats;
};

#define MIME_TYPE_SIZE 64

struct sdp_format {
	char mime_type[MIME_TYPE_SIZE];
	media_format_t format;
	parser_new_t parser_new;
	int rtp_payload;
	int rtp_clock;
	int channels;
	char *fmtp;
};

typedef enum {
	/* Uninitialized */
	NMTK_STATE_UNKNOWN		= 0,

	/* Normal states */
	NMTK_STATE_TYPE_NORMAL		= 0x01000000,
	NMTK_STATE_IDLE,
	NMTK_STATE_RUNNING,

	/* In-progress/pending states */
	NMTK_STATE_TYPE_PROGRESS	= 0x02000000,
	NMTK_STATE_RESOLVING,
	NMTK_STATE_CONNECTING,
	NMTK_STATE_REQUESTING,
	NMTK_STATE_DESCRIBE,
	NMTK_STATE_SETUP,
	NMTK_STATE_PLAY,
	NMTK_STATE_LOADING,

	/* General errors */
	NMTK_STATE_TYPE_GEN_ERROR	= 0x10000000,
	NMTK_STATE_HOST_UNKNOWN,
	NMTK_STATE_BAD_URI,
	NMTK_STATE_PROTO_ERROR,
	NMTK_STATE_NOT_2XX,
	NMTK_STATE_UNKNOWN_FORMAT,
	NMTK_STATE_INVALID_SDP,
	NMTK_STATE_NO_MEDIA,
	NMTK_STATE_FORMAT_ERROR,
	NMTK_STATE_TRUNCATED,

	/* System errors, where errno is valid */
	NMTK_STATE_TYPE_SYS_ERROR	= 0x20000000,
	NMTK_STATE_RESOLVER_ERROR,
	NMTK_STATE_SOCKET_ERROR,
	NMTK_STATE_RTP_SOCKET_ERROR,
	NMTK_STATE_FILE_OPEN_ERROR,
	NMTK_STATE_FILE_READ_ERROR,

	/* All failures */
	NMTK_STATE_TYPE_FAILURE		= 0xf0000000,
} nmtk_state_t;

struct nmtk_status {
	nmtk_state_t state;
	int posix_errno;
};

int nmtk_status_text(struct nmtk_status status, char *dest, int len);

struct resolver_func {
	void (*complete)(int addr_count, struct nmtk_addr *addrs, void *data);
	void (*error)(int status, void *data);
};

struct resolver *resolver_new(struct ref_lock *rlock,
		struct dispatcher *dispatcher);
void resolver_new_query(struct resolver *res, char *name,
		nmtk_protocol_t proto, struct resolver_func *func, void *data);
void resolver_del(struct resolver *res);
int nmtk_resolver_init(void);

struct sdp_track *sdp_get_track_by_tid(struct sdp *sdp, track_id_t tid);
struct sdp_format *sdp_get_format_by_payload(struct sdp_track *t, int payload);
char *sdp_get_session_field(struct sdp *sdp, char *prefix);
char *sdp_get_track_field(struct sdp_track *t, char *prefix);
struct sdp *sdp_new(int track_count, int format_count, int data_size,
		struct sdp_format **format_base, void **data);
struct sdp *sdp_parse(char *sdp_text, int sdp_text_len, char *base_uri);

struct nmtk_file_source_func {
	void (*start_read)(struct read_position *pos, void *data);
	void (*close)(void *data);
};

struct nmtk_file_reader_func {
	void (*read_complete)(void *data);
};

struct nmtk_file_read_context {
	int can_block;
	struct file_props props;
	struct {
		struct nmtk_file_source_func *func;
		void *data;
	} source;
	struct {
		struct nmtk_file_reader_func *func;
		void *data;
	} reader;
};

struct nmtk_source_func {
	int (*set_playback)(media_clock_t pos, int rate, void *data);
	void (*close)(void *data);
};

struct nmtk_source {
	struct nmtk_source_func *func;
	void *data;
};

struct nmtk_demux_func {
	int (*get_progs)(struct media_program **progs, int max_progs,
			void *data);
	int (*set_prog_act)(struct media_program *prog, int active,
			void *data);
	int (*set_track_act)(struct media_track *track, int active,
			void *data);
	void (*release_progs)(void *data);
};

struct nmtk_demux {
	struct nmtk_demux_func *func;
	void *data;
	struct list_head list;
};

struct nmtk_client_func {
	void (*status_update)(nmtk_state_t state, int last_errno, void *data);
	void (*file_opened)(char *filename, char *mime_type,
			unsigned char *start, int start_len,
			struct nmtk_file_read_context *file, void *data);
	void (*set_ra_info)(ra_flags_t ra_flags, struct timeline *timeline,
			void *data);
	void (*set_sdp)(struct sdp *sdp, void *data);
	void (*set_seek_result)(media_clock_t start, void *data);
	void (*add_demux)(struct nmtk_demux *demux, void *data);
	void (*new_track_list)(void *data);
};

struct nmtk_client_context {
	struct ref_lock *rlock;
	struct dispatcher *dispatcher;
	struct nmtk_source *source;
	struct nmtk_client_func *func;
	void *data;
};

void rtp_init(void);
void rtsp_client_init(void);

struct rtsp_client *rtsp_client_new(struct nmtk_client_context *client);
int rtsp_client_add_header(struct rtsp_client *rtsp, char *name, char *value);
void rtsp_client_start(struct rtsp_client *rtsp, char *uri);
void rtsp_client_close(struct rtsp_client *rtsp);

struct shm_client *shm_client_new(struct nmtk_client_context *client, char *uri);
void shm_client_start(struct shm_client *rtsp);
void shm_client_close(struct shm_client *shm);
int  shm_init( void *tx, void *rx);
void shm_done();
int  shm_send(void *pkt);

struct local_file *local_file_open(struct nmtk_client_context *client,
		char *path);
void local_file_start(struct local_file *lf);
void local_file_close(struct local_file *local_file);

struct http_client *http_client_new(struct nmtk_client_context *client);
int http_client_add_header(struct http_client *hc, char *name, char *value);
void http_client_start(struct http_client *hc, char *uri);
void http_client_close(struct http_client *hc);

struct udp_client *udp_client_new(struct nmtk_client_context *client,
		char *host, int port, char *path);
void udp_client_close(struct udp_client *udp);

struct nmtk_client *nmtk_client_new(char *uri, struct dispatcher *dispatcher);
int nmtk_client_add_header(struct nmtk_client *client, char *name, char *value);
void nmtk_client_start(struct nmtk_client *client);
struct notifier *nmtk_client_get_notifier(struct nmtk_client *client);
struct nmtk_status nmtk_client_get_status(struct nmtk_client *client);
int nmtk_client_get_ra_info(struct nmtk_client *client, ra_flags_t *ra_flags,
		struct timeline *timeline);
int nmtk_client_set_playback(struct nmtk_client *client,
		media_clock_t pos, int rate);
int nmtk_client_get_seek_result(struct nmtk_client *client, media_clock_t *pos);
int nmtk_client_get_progs(struct nmtk_client *client, struct sdp **sdp,
		struct media_program **progs, int max_progs);
int nmtk_client_set_prog_act(struct nmtk_client *client,
		struct media_program *prog, int active);
int nmtk_client_set_track_act(struct nmtk_client *client,
		struct media_track *track, int active);
void nmtk_client_release_progs(struct nmtk_client *client);
void nmtk_client_close(struct nmtk_client *client);

struct file_avi_writer *file_avi_writer_new(char *filename, int track_count);
int file_avi_writer_write(struct file_avi_writer *aw, int track,
		struct media_track_slot *slot);
void file_avi_writer_close(struct file_avi_writer *aw);

struct file_wav_writer *file_wav_writer_new(char *filename);
int file_wav_writer_write(struct file_wav_writer *ww,
		struct media_track_slot *slot);
void file_wav_writer_close(struct file_wav_writer *ww);

typedef enum {
	MOV_QT,		/* Apple QuickTime */
	MOV_MP4,	/* ISO/IEC 14496-14 */
	MOV_MSNV,	/* Sony PSP-compatible (unregistered brand) */
	MOV_3GP,	/* 3GPP Release 6 Basic Profile */
} mov_flavor_t;

struct file_mov_writer *file_mov_writer_new(char *filename, mov_flavor_t flavor,
		int track_count);
int file_mov_writer_write(struct file_mov_writer *mv, int track,
		struct media_track_slot *slot, unsigned int duration,
		media_clock_t timescale);
void file_mov_writer_close(struct file_mov_writer *mv);

int pcm_configuration_set(int pcmSampleRate, int pcmFormat, int pcmChannels); 
void pcm_configuration_reset(); 

/* There's probably a better place to put these... */
#define PUT_16(p,v) (((unsigned char *)(p))[0]=((v)>>8)&0xff,((unsigned char *)(p))[1]=(v)&0xff)
#define PUT_32(p,v) (((unsigned char *)(p))[0]=((v)>>24)&0xff,((unsigned char *)(p))[1]=((v)>>16)&0xff,((unsigned char *)(p))[2]=((v)>>8)&0xff,((unsigned char *)(p))[3]=(v)&0xff)
#define GET_16(p) (unsigned short)((((unsigned char *)(p))[0]<<8)|((unsigned char *)(p))[1])
#define GET_32(p) (unsigned int)((((unsigned char *)(p))[0]<<24)|(((unsigned char *)(p))[1]<<16)|(((unsigned char *)(p))[2]<<8)|((unsigned char *)(p))[3])
#define GET_16_LE(p) (unsigned short)((((unsigned char *)(p))[1]<<8)|((unsigned char *)(p))[0])
#define GET_32_LE(p) (unsigned int)((((unsigned char *)(p))[3]<<24)|(((unsigned char *)(p))[2]<<16)|(((unsigned char *)(p))[1]<<8)|((unsigned char *)(p))[0])
#define FOURCC(c) (unsigned int)(((c)[0]<<24)|((c)[1]<<16)|((c)[2]<<8)|(c)[3])
#define CONST_FOURCC(a,b,c,d) (unsigned int)(((a)<<24)|((b)<<16)|((c)<<8)|(d))
