/* Include file for libsaa7114 */

typedef unsigned int __u32;
typedef unsigned short __u16;
typedef unsigned char __u8,u8;
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#define __user
#include <sys/ioctl.h>
#include "videodev.h"
#include "video_decoder.h"

struct saa7114_handle;

struct saa7114_handle* SAA7114_init(char *dev, int dont_reset);
void SAA7114_close(struct saa7114_handle* handle);
void SAA7114_write(struct saa7114_handle* handle, u8 reg, u8 val);
int SAA7114_read (struct saa7114_handle* handle, u8 reg);
int SAA7114_command (struct saa7114_handle* handle,
		     unsigned int       cmd,
		     void              *arg);

// Inputs for Reelbox
#define REEL_CVBS_IN_SCART 1
#define REEL_CVBS_IN_FRONT 4
#define REEL_YC_IN_FRONT 6
#define REEL_CVBS_IN_TUNER  5
#define REEL_YC_IN_SCART 7
