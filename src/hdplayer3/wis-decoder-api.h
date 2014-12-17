/*****************************************************************************
*
* Copyright (c) 2003-2007 Micronas USA, Inc. All Rights Reserved.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
******************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev.h>
#include <errno.h>

#if 0
#include <linux/go8xxx.h>
#include <linux/go8xxx-adec.h>
#endif

typedef enum {
	WIS_FMT_MPEG1 = GO8XXX_PIX_FMT_MPEG1,
	WIS_FMT_MPEG2 = GO8XXX_PIX_FMT_MPEG2,
	WIS_FMT_MPEG4 = GO8XXX_PIX_FMT_MPEG4,
	WIS_FMT_H264 = GO8XXX_PIX_FMT_H264,
	WIS_FMT_WMV3 = GO8XXX_PIX_FMT_WMV3,
	WIS_FMT_WVC1 = GO8XXX_PIX_FMT_WVC1,
} wis_fmt_id_t;

typedef enum {
	WIS_FIELD_NONE = V4L2_FIELD_NONE,                            // 1
	WIS_FIELD_TB = V4L2_FIELD_INTERLACED_TB,                     // 8
	WIS_FIELD_BT = V4L2_FIELD_INTERLACED_BT,                     // 9
	WIS_FIELD_TOP_ONLY = V4L2_FIELD_TOP,                         // 2
	WIS_FIELD_BOTTOM_ONLY = V4L2_FIELD_BOTTOM,                   // 3
} wis_field_t;

typedef enum {
	WIS_SPKR_FRONT_LEFT			= 0x1,
	WIS_SPKR_FRONT_RIGHT			= 0x2,
	WIS_SPKR_FRONT_CENTER			= 0x4,
	WIS_SPKR_LOW_FREQUENCY			= 0x8,
	WIS_SPKR_BACK_LEFT			= 0x10,
	WIS_SPKR_BACK_RIGHT			= 0x20,
	WIS_SPKR_FRONT_LEFT_OF_CENTER		= 0x40,
	WIS_SPKR_FRONT_RIGHT_OF_CENTER		= 0x80,
	WIS_SPKR_BACK_CENTER			= 0x100,
	WIS_SPKR_SIDE_LEFT			= 0x200,
	WIS_SPKR_SIDE_RIGHT			= 0x400,
	WIS_SPKR_TOP_CENTER			= 0x800,
	WIS_SPKR_TOP_FRONT_LEFT			= 0x1000,
	WIS_SPKR_TOP_FRONT_CENTER		= 0x2000,
	WIS_SPKR_TOP_FRONT_RIGHT		= 0x4000,
	WIS_SPKR_TOP_BACK_LEFT			= 0x8000,
	WIS_SPKR_TOP_BACK_CENTER		= 0x10000,
	WIS_SPKR_TOP_BACK_RIGHT			= 0x20000,
} wis_channel_mask_t;

typedef enum adec_format wis_pcm_fmt_id_t;

static inline int
wis_dec_map_au_buffers(int video_fd, int buffer_count, void **buf_addrs)
{
	struct v4l2_requestbuffers req;
	struct v4l2_buffer buf;
	int i;

	/* Request that buffers be allocated for memory mapping */
	memset(&req, 0, sizeof(req));
	req.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	req.memory = V4L2_MEMORY_MMAP;
	req.count = buffer_count;
	if (ioctl(video_fd, VIDIOC_REQBUFS, &req) < 0) {
		perror("Error requesting video buffers");
		return -1;
	}
	if (req.count != buffer_count) {
		fprintf(stderr, "Unable to allocate %d buffers\n",
				buffer_count);
		return -1;
	}

	/* Map each of the buffers into this process's memory */
	for (i = 0; i < req.count; ++i) {
		memset(&buf, 0, sizeof(buf));
		buf.index = i;
		buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		buf.memory = V4L2_MEMORY_MMAP;
		if (ioctl(video_fd, VIDIOC_QUERYBUF, &buf) < 0) {
			perror("Error requesting buffer address");
			return -1;
		}
		buf_addrs[buf.index] = mmap(NULL, buf.length,
				PROT_READ | PROT_WRITE, MAP_SHARED,
				video_fd, buf.m.offset);
	}
	return buf.length;
}

static inline int
wis_dec_map_ring_buffer(int video_fd, void **buf_addr)
{
	struct v4l2_requestbuffers req;
	struct v4l2_buffer buf;
	unsigned int ring_size;

	/* Request that buffers be allocated for memory mapping */
	memset(&req, 0, sizeof(req));
	req.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	req.memory = GO8XXX_MEMORY_MMAP_RING;
	if (ioctl(video_fd, VIDIOC_REQBUFS, &req) < 0) {
		perror("Error requesting video ring buffer");
		return -1;
	}

	/* query a buffer to calculate ring size */
	memset(&buf, 0, sizeof(buf));
	buf.index = 0;
	buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	buf.memory = GO8XXX_MEMORY_MMAP_RING;
	if (ioctl(video_fd, VIDIOC_QUERYBUF, &buf) < 0) {
		perror("Error requesting buffer address");
		return -1;
	}
	
	ring_size = req.count*buf.length;

	/* Map the ring buffer into this process's memory */
	*buf_addr = mmap(NULL, ring_size, PROT_READ | PROT_WRITE,
			MAP_SHARED, video_fd, 0);
	return ring_size;
}

static inline int
wis_dec_start(int video_fd)
{
	int arg = V4L2_BUF_TYPE_VIDEO_OUTPUT;

	if (ioctl(video_fd, VIDIOC_STREAMON, &arg) < 0) {
		perror("Error starting video stream");
		return -1;
	}
	return 0;
}

static inline int
wis_dec_get_finished_au_buffer(int video_fd)
{
	struct v4l2_buffer buf;

	/* Retrieve the displayed buffer from the kernel */
	memset(&buf, 0, sizeof(buf));
	buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	buf.memory = V4L2_MEMORY_MMAP;
	if (ioctl(video_fd, VIDIOC_DQBUF, &buf) < 0) {
		perror("Error retrieving finished frame buffer");
		return -1;
	}
	return buf.index;
}

static inline int
wis_dec_get_finished_ring_offset(int video_fd)
{
	struct v4l2_buffer buf;

	/* Retrieve the displayed buffer from the kernel */
	memset(&buf, 0, sizeof(buf));
	buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	buf.memory = GO8XXX_MEMORY_MMAP_RING;
	if (ioctl(video_fd, VIDIOC_DQBUF, &buf) < 0) {
		perror("Error retrieving finished frame buffer");
		return -1;
	}
	return buf.m.offset;
}

static inline int
wis_dec_au_play(int video_fd, int index, int frame_length,
		struct timeval *presentation_time)
{
	struct v4l2_buffer buf;

	memset(&buf, 0, sizeof(buf));
	buf.index = index;
	buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	buf.memory = V4L2_MEMORY_MMAP;
	if (presentation_time)
		buf.timestamp = *presentation_time;
	buf.bytesused = frame_length;
	if (ioctl(video_fd, VIDIOC_QBUF, &buf) < 0) {
		perror("Error queueing buffer for display");
		return -1;
	}
	return 0;
}

static inline int
wis_dec_ring_play(int video_fd, int ring_offset, int frame_length,
		struct timeval *presentation_time)
{
	struct v4l2_buffer buf;
	long flags;
	int retval;

	memset(&buf, 0, sizeof(buf));
	buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	buf.memory = GO8XXX_MEMORY_MMAP_RING;
	buf.m.offset = ring_offset;
	if (presentation_time)
		buf.timestamp = *presentation_time;
	buf.bytesused = frame_length;
	flags = fcntl(video_fd, F_GETFL);
	fcntl(video_fd, F_SETFL, flags | O_NONBLOCK);
	retval = ioctl(video_fd, VIDIOC_QBUF, &buf);
	if (retval < 0) {
		if (errno == EAGAIN)
			retval = 1;
		else
			perror("Error queueing buffer for display");
	}
	fcntl(video_fd, F_SETFL, flags);
	return retval;
}

static inline int
wis_dec_au_play_pts(int video_fd, int index, int frame_length, __u64 pts)
{
	struct go8xxx_buffer_pts buf;

	memset(&buf, 0, sizeof(buf));
	buf.index = index;
	buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.pts = pts;
	buf.bytesused = frame_length;
	if (ioctl(video_fd, GO8XXXIOC_QBUF_PTS, &buf) < 0) {
		perror("Error queueing buffer for display");
		return -1;
	}
	return 0;
}

static inline int
wis_dec_ring_play_pts(int video_fd, int ring_offset, int frame_length,
		__u64 pts)
{
	struct go8xxx_buffer_pts buf;
	long flags;
	int retval;

	memset(&buf, 0, sizeof(buf));
	buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	buf.memory = GO8XXX_MEMORY_MMAP_RING;
	buf.m.offset = ring_offset;
	buf.pts = pts;
	buf.bytesused = frame_length;
	flags = fcntl(video_fd, F_GETFL);
	fcntl(video_fd, F_SETFL, flags | O_NONBLOCK);
	retval = ioctl(video_fd, GO8XXXIOC_QBUF_PTS, &buf);
	if (retval < 0) {
		if (errno == EAGAIN)
			retval = 1;
		else
			perror("Error queueing buffer for display");
	}
	fcntl(video_fd, F_SETFL, flags);
	return retval;
}

static inline int 
wis_dec_set_nodelay(int video_fd, int enable)
{ 
	unsigned int flags;

	if (ioctl(video_fd, GO8XXXIOC_G_DEC_MODE, &flags) < 0) {
		perror("Error querying decoder mode");
		return -1;
	}
	if (enable)
		flags |= GO8XXX_DEC_MODE_NODELAY; 
	else
		flags &= ~GO8XXX_DEC_MODE_NODELAY; 
	if (ioctl(video_fd, GO8XXXIOC_S_DEC_MODE, &flags) < 0) {
		perror("Error setting decoder mode");
		return -1;
	}
	return 0;
} 

static inline int
wis_dec_stop(int video_fd)
{
	int arg = V4L2_BUF_TYPE_VIDEO_OUTPUT;

	if (ioctl(video_fd, VIDIOC_STREAMOFF, &arg) < 0) {
		perror("Error stopping the video stream");
		return -1;
	}
	return 0;
}

static inline int
wis_dec_unmap_au_buffers(int video_fd, void **buf_addrs, int buffer_count)
{
	struct v4l2_buffer buf;
	int i;

	for (i = 0; i < buffer_count; ++i) {
		memset(&buf, 0, sizeof(buf));
		buf.index = i;
		buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		buf.memory = V4L2_MEMORY_MMAP;
		if (ioctl(video_fd, VIDIOC_QUERYBUF, &buf) < 0) {
			perror("Error querying buffer address");
			return -1;
		}
		munmap(buf_addrs[buf.index], buf.length);
	}
	return 0;
}

static inline void
wis_dec_unmap_ring_buffer(int video_fd, void *buf_addr, int buffer_size)
{
	munmap(buf_addr, buffer_size);
}

static inline int
wis_dec_set_format(int video_fd, wis_fmt_id_t format)
{
	struct v4l2_format fmt;

	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	fmt.fmt.pix.pixelformat = format;
	if (ioctl(video_fd, VIDIOC_S_FMT, &fmt) < 0) {
		fprintf(stderr, "Error setting video format %s\n",strerror(errno));
		return -1;
	}
	return 0;
}

static inline int
wis_dec_set_crop_area(int video_fd, int left, int top, int width, int height)
{
	struct v4l2_crop crop;

	memset(&crop, 0, sizeof(crop));
	crop.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	crop.c.left = left;
	crop.c.top = top;
	crop.c.width = width;
	crop.c.height = height;
	if (ioctl(video_fd, VIDIOC_S_CROP, &crop) < 0) {
		fprintf(stderr, "Error setting crop area\n");
		return -1;
	}
	return 0;
}

static inline int
wis_dec_set_pic_params(int video_fd, int par_width, int par_height,
		int overscan)
{
	struct go8xxx_pic_params pic = {
		.type = V4L2_BUF_TYPE_VIDEO_OUTPUT,
		.par.width = par_width,
		.par.height = par_height,
		.flags = overscan ? GO8XXX_PIC_FLAG_OVERSCAN : 0,
	};

	if (ioctl(video_fd, GO8XXXIOC_S_PIC_PARAMS, &pic) < 0) {
		fprintf(stderr, "Error setting picture parameters\n");
		return -1;
	}
	return 0;
}

static inline int
wis_dec_set_default_fps(int video_fd, int incr, int base)
{
	struct v4l2_streamparm parm;

	memset(&parm, 0, sizeof(parm));
	parm.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	parm.parm.output.timeperframe.numerator = incr;
	parm.parm.output.timeperframe.denominator = base;
	if (ioctl(video_fd, VIDIOC_S_PARM, &parm) < 0) {
		fprintf(stderr, "Error setting the new frame rate\n");
		return -1;
	}
	return 0;
}

static inline int
wis_dec_reset_stc(int video_fd, __u64 initial_stc_value)
{
	if (ioctl(video_fd, GO8XXXIOC_S_STC, &initial_stc_value) < 0) {
		fprintf(stderr, "Error initializing the STC\n");
		return -1;
	}
	return 0;
}

static inline int
wis_dec_get_stc(int video_fd, __u64 *current_stc)
{
	if (ioctl(video_fd, GO8XXXIOC_G_STC, current_stc) < 0) {
//		fprintf(stderr, "Error reading the STC\n");
		return -1;
	}
	return 0;
}

static inline int
wis_dec_map_yuv_buffers(int yuv_fd, int *buffer_count, void **buf_addrs,
		int max_yuv_width, int max_yuv_height)
{
	struct v4l2_format fmt;
	struct v4l2_requestbuffers req;
	struct v4l2_buffer buf;
	int i;

	/* Inform the driver of the maximum size YUV frames which need
	 * to fit in our mapped buffers */
	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.pixelformat = GO8XXX_PIX_FMT_YUV420;
	fmt.fmt.pix.width = max_yuv_width;
	fmt.fmt.pix.height = max_yuv_height;
	if (ioctl(yuv_fd, VIDIOC_S_FMT, &fmt) < 0) {
		perror("Error setting maximum YUV buffer dimensions");
		return -1;
	}

	/* Request that buffers be allocated for memory mapping */
	memset(&req, 0, sizeof(req));
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	req.count = *buffer_count;
	if (ioctl(yuv_fd, VIDIOC_REQBUFS, &req) < 0) {
		perror("Error requesting YUV video buffers");
		return -1;
	}
	*buffer_count = req.count;
	memset(buf_addrs, *buffer_count, sizeof(void*));

	/* Map each of the buffers into this process's memory */
	for (i = 0; i < req.count; ++i) {
		memset(&buf, 0, sizeof(buf));
		buf.index = i;
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		if (ioctl(yuv_fd, VIDIOC_QUERYBUF, &buf) < 0) {
			perror("Error requesting YUV buffer address");
			return -1;
		}
		buf_addrs[buf.index] = mmap(NULL, buf.length,
				PROT_READ | PROT_WRITE, MAP_SHARED,
				yuv_fd, buf.m.offset);
	}
	return buf.length;
}

static inline int
wis_dec_yuv_capture_start(int yuv_fd)
{
	int arg = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (ioctl(yuv_fd, VIDIOC_STREAMON, &arg) < 0) {
		perror("Error starting YUV capture");
		return -1;
	}
	return 0;
}

static inline int
wis_dec_queue_yuv_buffer(int yuv_fd, unsigned int index)
{
	struct v4l2_buffer buf;

	memset(&buf, 0, sizeof(buf));
	buf.index = index;
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	if (ioctl(yuv_fd, VIDIOC_QBUF, &buf) < 0) {
		perror("Error queueing YUV buffer for capture");
		return -1;
	}
	return 0;
}

static inline int
wis_dec_dequeue_yuv_buffer(int yuv_fd, unsigned int *index,
		struct timeval *presentation_time, wis_field_t *field,
		unsigned int *display_handle)
{
	struct v4l2_buffer buf;

	memset(&buf, 0, sizeof(buf));
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	if (ioctl(yuv_fd, VIDIOC_DQBUF, &buf) < 0) {
		if (errno == EAGAIN)
			return 1;
		perror("Error dequeueing captured YUV buffer");
		return -1;
	}
	*index = buf.index;
	if (presentation_time)
		*presentation_time = buf.timestamp;
	if (field)
		*field = buf.field;
	if (display_handle)
		*display_handle = buf.reserved;
	return 0;
}

static inline int
wis_dec_dequeue_yuv_buffer_pts(int yuv_fd, unsigned int *index,
		__u64 *pts, wis_field_t *field, unsigned int *display_handle)
{
	struct go8xxx_buffer_pts buf;

	memset(&buf, 0, sizeof(buf));
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	if (ioctl(yuv_fd, GO8XXXIOC_DQBUF_PTS, &buf) < 0) {
		if (errno == EAGAIN)
			return 1;
		perror("Error dequeueing captured YUV buffer");
		return -1;
	}
	*index = buf.index;
	if (pts)
		*pts = buf.pts;
	if (field)
		*field = buf.field;
	if (display_handle)
		*display_handle = buf.reserved;
	return 0;
}

static inline int
wis_dec_yuv_capture_stop(int yuv_fd)
{
	int arg = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (ioctl(yuv_fd, VIDIOC_STREAMOFF, &arg) < 0) {
		perror("Error stopping YUV capture");
		return -1;
	}
	return 0;
}

static inline int
wis_dec_unmap_yuv_buffers(int yuv_fd, void **buf_addrs, int buffer_count, int buf_size)
{
#if 0
	struct v4l2_buffer buf;
#endif
	int i;

	for (i = 0; i < buffer_count; ++i) {
#if 0
		memset(&buf, 0, sizeof(buf));
		buf.index = i;
		buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		buf.memory = V4L2_MEMORY_MMAP;
		if (ioctl(yuv_fd, VIDIOC_QUERYBUF, &buf) < 0) {
			perror("Error querying YUV buffer address");
			return -1;
		}
		munmap(buf_addrs[buf.index], buf.length);
#else
		if (buf_addrs[i])
			munmap(buf_addrs[i], buf_size);
#endif

	}
	return 0;
}

static inline int
wis_dec_yuv_display_start(int disp_fd)
{
	struct v4l2_requestbuffers req;
	int arg = V4L2_BUF_TYPE_VIDEO_OUTPUT;

	/* Reserve the device */
	memset(&req, 0, sizeof(req));
	req.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	req.memory = GO8XXX_MEMORY_DEC_HANDLE;
	if (ioctl(disp_fd, VIDIOC_REQBUFS, &req) < 0) {
		perror("Error acquiring display device");
		return -1;
	}

	/* Start display */
	if (ioctl(disp_fd, VIDIOC_STREAMON, &arg) < 0) {
		perror("Error starting YUV capture");
		return -1;
	}
	return 0;
}

static inline int
wis_dec_yuv_play(int disp_fd, unsigned int display_handle,
		struct timeval presentation_time, wis_field_t field,
		unsigned int frame_id)
{
	struct v4l2_buffer buf;

	memset(&buf, 0, sizeof(buf));
	buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	buf.memory = GO8XXX_MEMORY_DEC_HANDLE;
	buf.m.offset = display_handle;
	buf.timestamp = presentation_time;
	buf.sequence = frame_id;
	buf.field = field;
	if (ioctl(disp_fd, VIDIOC_QBUF, &buf) < 0) {
		perror("Error queueing YUV buffer for display");
		return -1;
	}
	return 0;
}

static inline int
wis_dec_yuv_play_pts(int disp_fd, unsigned int display_handle,
		__u64 pts, wis_field_t field, unsigned int frame_id)
{
	struct go8xxx_buffer_pts buf;

	memset(&buf, 0, sizeof(buf));
	buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	buf.memory = GO8XXX_MEMORY_DEC_HANDLE;
	buf.m.offset = display_handle;
	buf.pts = pts;
	buf.sequence = frame_id;
	buf.field = field;
	if (ioctl(disp_fd, GO8XXXIOC_QBUF_PTS, &buf) < 0) {
		perror("Error queueing YUV buffer for display");
		return -1;
	}
	return 0;
}

static inline int
wis_dec_get_displayed_yuv(int disp_fd, unsigned int *frame_id)
{
	struct v4l2_buffer buf;

	/* Retrieve the displayed buffer from the kernel */
	memset(&buf, 0, sizeof(buf));
	buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	buf.memory = GO8XXX_MEMORY_DEC_HANDLE;
	if (ioctl(disp_fd, VIDIOC_DQBUF, &buf) < 0) {
		perror("Error retrieving finished frame buffer");
		return -1;
	}
	if (frame_id)
		*frame_id = buf.sequence;
	return 0;
}

static inline int
wis_dec_config_display(int disp_fd, int yuv_fd)
{
	struct v4l2_format fmt;
	struct v4l2_crop crop;
	struct go8xxx_pic_params pic;
	printf("######################################################\n");
	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(yuv_fd, VIDIOC_G_FMT, &fmt) < 0) {
		perror("VIDIOC_G_FMT");
		return -1;
	}
	memset(&crop, 0, sizeof(crop));
	crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(yuv_fd, VIDIOC_G_CROP, &crop) < 0) {
		perror("VIDIOC_G_CROP");
		return -1;
	}
	memset(&pic, 0, sizeof(pic));
	pic.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(yuv_fd, GO8XXXIOC_G_PIC_PARAMS, &pic) < 0) {
		perror("GO8XXXIOC_G_PIC_PARAMS");
		return -1;
	}
	fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	if (ioctl(disp_fd, VIDIOC_S_FMT, &fmt) < 0) {
		perror("VIDIOC_S_FMT");
		return -1;
	}
	crop.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	if (ioctl(disp_fd, VIDIOC_S_CROP, &crop) < 0) {
		perror("VIDIOC_S_CROP");
		return -1;
	}
	pic.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	printf("########## PIC %i %i, FMT %i %i\n",pic.par.width,pic.par.height,fmt.fmt.pix.width,fmt.fmt.pix.height);
//	pic.par.width=fmt.fmt.pix.height*16;
//	pic.par.height=fmt.fmt.pix.width*9;
	if (ioctl(disp_fd, GO8XXXIOC_S_PIC_PARAMS, &pic) < 0) {
		perror("GO8XXXIOC_S_PIC_PARAMS");
		return -1;
	}
	return 0;
}

static inline int
wis_dec_yuv_display_stop(int disp_fd)
{
	struct v4l2_requestbuffers req;
	int arg = V4L2_BUF_TYPE_VIDEO_OUTPUT;

	/* Stop display */
	if (ioctl(disp_fd, VIDIOC_STREAMOFF, &arg) < 0) {
		perror("Error stopping YUV capture");
		return -1;
	}

	/* Un-reserve the device */
	memset(&req, 0, sizeof(req));
	req.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	req.memory = V4L2_MEMORY_MMAP;
	if (ioctl(disp_fd, VIDIOC_REQBUFS, &req) < 0) {
		perror("Error releasing display device");
		return -1;
	}
	return 0;
}

static inline int
wis_adec_output_config(int audio_fd, int use_stc,int audio_port)
{
	struct adec_output_config output;

	if (use_stc)
		output.pts_clock = ADEC_PTS_STC_TSI0;
	else
		output.pts_clock = ADEC_PTS_AUDIO;
        if ((audio_port < ADEC_AUDIO_PORT_INVALID) && (audio_port >= 0))
		output.audio_port = audio_port;
        else
		output.audio_port = -1;
	if (ioctl(audio_fd, ADEC_OUTPUT_CONFIG, &output) < 0) {
		fprintf(stderr, "Error setting audio output configuration\n");
		return -1;
	}
	return 0;
}

static inline int
wis_adec_config_mpeg(int audio_fd, int sample_rate, int out_sample_rate, 
		     int channels, int out_channels)
{
	struct adec_config adec_config = {
		.format			= ADEC_FMT_MPEG,
		.samplerate		= sample_rate,
		.out_samplerate		= out_sample_rate,
		.channels		= channels,
                .out_channels           = out_channels,
		.codec_config_size	= 0,
	};

	if (ioctl(audio_fd, ADEC_STREAM_CONFIG, &adec_config) < 0) {
		perror("Error setting audio stream configuration");
		return -1;
	}
	return 0;
}

static inline int
wis_adec_config_aac(int audio_fd, int sample_rate, int out_sample_rate, int channels,
		    int out_channels, void *decoder_config, int config_len)
{
	struct adec_config adec_config = {
		.format			= ADEC_FMT_AAC,
		.samplerate		= sample_rate,
		.out_samplerate		= out_sample_rate,
		.channels		= channels,
                .out_channels           = out_channels,
		.codec_config		= NULL,
		.codec_config_size	= 0,
		.downmix_mode		= channels > 2 ? 1 : 0,
	};
        adec_config.codec_config = decoder_config;
        adec_config.codec_config_size = config_len;
	if (ioctl(audio_fd, ADEC_STREAM_CONFIG, &adec_config) < 0) {
		perror("Error setting audio stream configuration");
		return -1;
	}
	return 0;
}

static inline int
wis_adec_config_wma(int audio_fd, int sample_rate, int out_sample_rate, 
	int channels, int out_channels,
	int format_tag, int bitrate, int block_align, int bits_per_sample,
	void *waveformatex_extra, int waveformatex_extra_length)
{
	unsigned char *cfg = waveformatex_extra;
	int dec_cfg[6] = { 0, format_tag, 0, bitrate, block_align, bits_per_sample };
	struct adec_config adec_config = {
		.format			= ADEC_FMT_WMA,
		.samplerate		= sample_rate,
		.out_samplerate		= out_sample_rate,
		.channels		= channels,
                .out_channels           = out_channels,
		.codec_config		= dec_cfg,
		.codec_config_size	= 6 * sizeof(int),
	};

	switch (format_tag) {
	case 0x0160:
		if (waveformatex_extra_length != 4) {
			fprintf(stderr, "Error: expected 4 bytes of codec "
					"configuration data for WMAv1 but "
					"got %d\n", waveformatex_extra_length);
			return -1;
		}
		printf("WMA1\n");
		switch (channels) {
		case 1:
			dec_cfg[0] = WIS_SPKR_FRONT_CENTER;
			break;
		case 2:
			dec_cfg[0] = WIS_SPKR_FRONT_LEFT | WIS_SPKR_FRONT_RIGHT;
			break;
		default:
			fprintf(stderr, "Invalid channel selection\n");
			return -1;
		}
		dec_cfg[2] = (cfg[3] << 8) | cfg[2];
		break;
	case 0x0161:
		if (waveformatex_extra_length != 10) {
			fprintf(stderr, "Error: expected 10 bytes of codec "
					"configuration data for WMAv2 but "
					"got %d\n", waveformatex_extra_length);
			return -1;
		}
		switch (channels) {
		case 1:
			dec_cfg[0] = WIS_SPKR_FRONT_CENTER;
			break;
		case 2:
			dec_cfg[0] = WIS_SPKR_FRONT_LEFT | WIS_SPKR_FRONT_RIGHT;
			break;
		case 6:
			dec_cfg[0] = WIS_SPKR_FRONT_LEFT |
				WIS_SPKR_FRONT_RIGHT | WIS_SPKR_FRONT_CENTER |
				WIS_SPKR_BACK_LEFT | WIS_SPKR_BACK_RIGHT |
				WIS_SPKR_LOW_FREQUENCY;
			break;
		default:
			fprintf(stderr, "Invalid channel selection\n");
			return -1;
		}
		dec_cfg[2] = (cfg[5] << 8) | cfg[4];
		break;
	case 0x0162:
		if (waveformatex_extra_length != 18) {
			fprintf(stderr, "Error: expected 18 bytes of codec "
					"configuration data for WMAv3 but "
					"got %d\n", waveformatex_extra_length);
			return -1;
		}
		dec_cfg[0] = (cfg[5] << 24) | (cfg[4] << 16) |
			(cfg[3] << 8) | cfg[2];
		dec_cfg[2] = (cfg[15] << 8) | cfg[14];
		break;
	default:
		fprintf(stderr, "Invalid WMA format tag\n");
		return -1;
	}

	if (ioctl(audio_fd, ADEC_STREAM_CONFIG, &adec_config) < 0) {
		perror("Error setting audio stream configuration");
		return -1;
	}
	return 0;
}

static inline int
wis_adec_config_ac3(int audio_fd, int sample_rate, int out_sample_rate, int channels, 
		    int out_channels, void *decoder_config, int config_len)
{
        int dec_cfg[15] = { 2, 0, 0, 3, 1, 7, 0x7fffffff, 0x7fffffff, 6, 0, 1, 2, 3, 4, 5 };
        struct adec_config adec_config = {
            .format			= ADEC_FMT_AC3,
            .samplerate		= sample_rate,
            .out_samplerate	= out_sample_rate,
            .channels		= channels,
            .out_channels           = out_channels,
            .codec_config		= dec_cfg,
            .codec_config_size	= 15*sizeof(int),
        };
        if (decoder_config&&(config_len==15*sizeof(int))) {
	    memcpy(dec_cfg,decoder_config,config_len);
        } else {
	    dec_cfg[8] = out_channels;
            if (out_channels == 2) {
                dec_cfg[1] = 0;
                if (out_channels < channels) {
                    dec_cfg[1] = 1;
                    dec_cfg[4] = 0;
                } 
                dec_cfg[5] = 2;         
                dec_cfg[9] = 0;
                dec_cfg[10] = 2;    
            }        
        }  
        if (ioctl(audio_fd, ADEC_STREAM_CONFIG, &adec_config) < 0) {
                perror("Error setting audio stream configuration");
		return -1;
        }
        return 0;
}

static inline int
wis_adec_config_dts(int audio_fd, int sample_rate, int out_sample_rate, int channels,
                    int out_channels, void *dts_dec_cfg, int dts_dec_cfg_size)
{
        struct adec_config adec_config = {
                .format                 = ADEC_FMT_DTS,
                .samplerate             = sample_rate,
                .out_samplerate         = out_sample_rate,
                .channels               = channels,
                .out_channels           = out_channels,
                .codec_config           = NULL,
                .codec_config_size      = 0,
        };
        adec_config.codec_config = dts_dec_cfg;
        adec_config.codec_config_size = dts_dec_cfg_size;
        if (ioctl(audio_fd, ADEC_STREAM_CONFIG, &adec_config) < 0) {
                perror("Error setting audio stream configuration");
                return -1;
        }
        return 0;
}

static inline int
wis_adec_config_pcm(int audio_fd, int sample_rate, int out_sample_rate, int channels, 
		    int out_channels, wis_pcm_fmt_id_t pcm_format)
{
	struct adec_config adec_config = {
		.format			= pcm_format,
		.samplerate		= sample_rate,
		.out_samplerate		= out_sample_rate,
		.channels		= channels,
                .out_channels           = out_channels,
		.codec_config		= NULL,
		.codec_config_size	= 0,
	};
	if (ioctl(audio_fd, ADEC_STREAM_CONFIG, &adec_config) < 0) {
		perror("Error setting audio stream configuration");
		return -1;
	}
	return 0;
}

static inline int
wis_adec_play_frame(int audio_fd, void *audio_data, int audio_data_len,
		unsigned long long presentation_time)
{
	unsigned char buffer[32*1024];

	memcpy(buffer + 8, audio_data, audio_data_len);
	*((unsigned long long *)buffer) = presentation_time;
	if (write(audio_fd, buffer, audio_data_len + 8) < 0) {
		perror("Error queueing audio frame for playback");
		return -1;
	}
	return 0;
}

static inline unsigned long long
wis_adec_get_play_position(int audio_fd, struct timeval *timestamp)
{
	struct adec_position pos;

	if (ioctl(audio_fd, ADEC_GET_POSITION, &pos) < 0) {
		perror("Error retrieving audio playback position");
		return 0;
	}
	if (pos.position == 0) {
		fd_set set;
		FD_ZERO(&set);
		FD_SET(audio_fd, &set);
		select(audio_fd + 1, NULL, &set, NULL, NULL);
		return wis_adec_get_play_position(audio_fd, timestamp);
	}
	if (timestamp)
		*timestamp = pos.timestamp;
	return pos.position;
}
