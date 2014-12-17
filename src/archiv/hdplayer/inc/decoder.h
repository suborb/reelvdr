enum {
	FORMAT_ASF = FORMAT_TYPE_MUX + 0x8080,
	FORMAT_WMV = FORMAT_TYPE_VIDEO + 0x8080,
	FORMAT_WMA = FORMAT_TYPE_AUDIO + 0x8080,
};

typedef enum {
	DECYPHER_PLAYING		= NMTK_STATE_TYPE_NORMAL + 0x8000,
	DECYPHER_FLUSHED,
	DECYPHER_END_OF_STREAM,
	DECYPHER_SYNCING		= NMTK_STATE_TYPE_PROGRESS + 0x8000,
	DECYPHER_FORMAT_ERROR		= NMTK_STATE_TYPE_GEN_ERROR + 0x8000,
	DECYPHER_VIDEO_DEV_ERROR	= NMTK_STATE_TYPE_SYS_ERROR + 0x8000,
	DECYPHER_AUDIO_DEV_ERROR,
} decypher_state_t;

typedef enum {
	DECYPHER_MM_AV = 0,
	DECYPHER_MM_A_ONLY,
	DECYPHER_MM_V_ONLY,
} decypher_media_mode_t;

struct decypher *decypher_open(struct dispatcher *dispatcher, int vplane);
struct notifier *decypher_get_notifier(struct decypher *dec);
struct nmtk_status decypher_get_status(struct decypher *dec);
int decypher_status_text(struct nmtk_status status, char *dest, int len);
void decypher_reset_position(struct decypher *dec, media_clock_t media_start,
		media_clock_t display_start, int playback_rate);
int decypher_get_position(struct decypher *dec, media_clock_t *position);

int decypher_set_debug_print_times(struct decypher *dec, int print_enable);
int decypher_set_initial_audio_skew(struct decypher *dec, int skew);
int decypher_set_audio_clock_offset(struct decypher *dec, int samples);
int decypher_set_audio_device_index(struct decypher *dec, int index);
int decypher_set_audio_output_port(struct decypher *dec, int port);
int decypher_set_downmix(struct decypher *dec, int downmix_enable);
void decypher_set_buffer_delay(struct decypher *dec, int msec_delay);
void decypher_set_offset_check(struct decypher *dec, int measure_offset);
int decypher_set_init_buffer_level(struct decypher *dec, int min_fullness);

int decypher_play(struct decypher *dec);
int decypher_set_playback(struct decypher *dec, struct nmtk_client *client,
		media_clock_t pos, int rate);
void decypher_stop(struct decypher *dec);
int decypher_set_video_track(struct decypher *dec,
		struct media_track *track, int use_es_clock);
int decypher_set_audio_track(struct decypher *dec,
		struct media_track *track, int use_es_clock);
int decypher_set_metadata_track(struct decypher *dec,
		struct media_track *track);
