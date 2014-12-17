/***************************************************************************
 *   Copyright (C) 2008 by Reel Multimedia Vertriebs GmbH                  *
 *                         E-Mail  : info @ reel-multimedia.com            *
 *                         Internet: www.reel-multimedia.com               *
 *                                                                         *
 *   This code is free software; you can redistribute it and/or modify     *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#define LOG_MODULE "hde-cmds"
#define LOG_VERBOSE
#define LOG

#define LOG_FUNC 1
#define LOG_ERR  1

#define HDE_PTS_DELAY 45000

#include <xine_internal.h>
#include "hde_cmds.h"

#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <execinfo.h>
#include <hdchannel.h>

#include "hde_base.h"
#include "hde_tool.h"

#include <hdshmlib.c>
#include <hdchannel.c>

static pthread_mutex_t     mInitLock;
static int                 mInit   = 0;
static hdshm_area_t       *mHsa    = 0;
static hd_data_t volatile *mCtrl   = 0;
static hd_channel_t       *mTxChan = 0;
static hd_channel_t       *mRxChan = 0;
static hd_channel_t       *mOsdChan = 0;
static int                 mSeqNr  = 0;
static int                 mDeinit = 0;

static void safe_memcpy ( void *to, void const *from, size_t size ) {
#if 1
	size_t const buffer_size = 4096;
	unsigned char buffer[buffer_size];
	unsigned char *t = ( unsigned char * ) to;
	unsigned char const *f = ( unsigned char const * ) from;
	if(((size_t)to)&3) {
		printf("safe_memcpy to unaligned address %p\n", to);
		return;
	} // if
	while ( size ) {
		size_t cpy_size = size;
		if (cpy_size > buffer_size)
			cpy_size = buffer_size;
		if ((((size_t)f) | cpy_size ) & 3) {
			size_t align_size = ( cpy_size + 3 ) & 0x7FFFFFFC;
			xine_fast_memcpy(buffer, f, cpy_size);
			xine_fast_memcpy(t, buffer, align_size); // cp_size rounded up, will copy up to 3 bytes more than requested.
		} else {
			xine_fast_memcpy (t, f, cpy_size); // The simple case: from and cpy_size are aligned. Copy directly.
		} // if
		t += cpy_size;
		f += cpy_size;
		size -= cpy_size;
	} // while
#else
	size_t const buffer_size = 4096;
	uint32_t buffer[buffer_size>>2];
	uint32_t volatile *t = (uint32_t volatile *) to;
	uint32_t const *f = ( uint32_t const * ) from;
	int i;
	if(((size_t)to)&3) {
		printf("safe_memcpy to unaligned address %p\n", to);
		return;
	} // if
	while ( size ) {
		size_t cpy_size = size;
		if (cpy_size > buffer_size)
			cpy_size = buffer_size;
		size_t align_size = ( cpy_size + 3 ) >> 2;
		if ((((size_t)f) | cpy_size ) & 3) {
			memcpy(buffer, f, cpy_size);
			f = (uint32_t const *)(((unsigned char const *)f)+cpy_size);
			for(i=0;i<align_size;i++,t++)
				*t = buffer[i];
		} else {
			for(i=0;i<align_size;i++,t++,f++)
				*t = *f;
		} // if
		size -= cpy_size;
	} // while
#endif
} // safe_memcpy

static int hde_send_osd ( void *fData, unsigned int fDataSize, int *fErr ) {
	void *lBuffer = 0;
	if(fErr) *fErr = EINVAL;
	if (!mOsdChan)
		return -1;
	if(fErr) *fErr = EIO;
	hd_packet_es_data_t lCmdPacket;
	memset ( &lCmdPacket, 0, sizeof(lCmdPacket));
	lCmdPacket.header.magic       = HD_PACKET_MAGIC;
	lCmdPacket.header.seq_nr      = mSeqNr++;
	lCmdPacket.header.type        = HD_PACKET_OSD;
	lCmdPacket.header.packet_size = sizeof(lCmdPacket)+fDataSize;
	int ch_packet_size = (lCmdPacket.header.packet_size + 3) & 0x7FFFFFFC; // Align 4
	int lSize = hd_channel_write_start(mOsdChan, &lBuffer, ch_packet_size, 0);
	if (!lSize)
		return -1;
	if(fErr) *fErr = 0;
	safe_memcpy(lBuffer, &lCmdPacket, sizeof(lCmdPacket));
	if(fData && fDataSize)
		safe_memcpy(((unsigned char *)lBuffer)+sizeof(lCmdPacket), fData, fDataSize);
	hd_channel_write_finish(mOsdChan, ch_packet_size);
	return 0;
} // hde_send_osd

static int hde_send ( hd_packet_header_t *fPacket, int fType, int fSize, int *fErr ) {
	void *lBuffer = 0;
	if(fErr) *fErr = EINVAL;
	if ( !mTxChan )
		return -1;
	if(fErr) *fErr = EIO;
	fPacket->magic       = HD_PACKET_MAGIC;
	fPacket->seq_nr      = mSeqNr++;
	fPacket->type        = fType;
	fPacket->packet_size = fSize;
	int ch_packet_size = (fPacket->packet_size + 3) & 0x7FFFFFFC; // Align 4
	int lSize = hd_channel_write_start(mTxChan, &lBuffer, ch_packet_size, 0);
	if(!lSize)
		return -1;
	if(fErr) *fErr = 0;
	safe_memcpy(lBuffer, fPacket, fSize);
	hd_channel_write_finish(mTxChan, ch_packet_size);
	return 0;
} // hde_send

static int hde_send_cmd ( unsigned int fCmd, unsigned int fParam1, unsigned int fParam2, void *fData, unsigned int fDataSize, long fTimeout, int *fErr) {
	void *lTxBuffer = 0;
	void *lRxBuffer = 0;
	int lSize = 0;
	if(fErr) *fErr = EINVAL;
	if(!mTxChan || !mRxChan)
		return -1;
	if(fErr) *fErr = EIO;
	while(hd_channel_read_start(mRxChan, ( void * ) &lRxBuffer, &lSize, 0))
		hd_channel_read_finish(mRxChan);
	hd_packet_rpc_cmd_t lCmdPacket;
	memset ( &lCmdPacket, 0, sizeof(lCmdPacket));
	lCmdPacket.header.magic       = HD_PACKET_MAGIC;
	lCmdPacket.header.seq_nr      = mSeqNr++;
	lCmdPacket.header.type        = HD_PACKET_RPC_CMD;
	lCmdPacket.header.packet_size = sizeof (lCmdPacket) +fDataSize;
	lCmdPacket.cmd       = fCmd;
	lCmdPacket.param1    = fParam1;
	lCmdPacket.param2    = fParam2;
	lCmdPacket.data_size = fDataSize;
	int ch_packet_size = (lCmdPacket.header.packet_size + 3) & 0x7FFFFFFC; // Align 4
	lSize = hd_channel_write_start(mTxChan, &lTxBuffer, ch_packet_size, 0);
	if (!lSize)
		return -1;
	safe_memcpy(lTxBuffer, &lCmdPacket, sizeof(lCmdPacket));
	if ( fData && fDataSize )
		safe_memcpy(((unsigned char *)lTxBuffer)+sizeof(lCmdPacket), fData, fDataSize);
	hd_channel_write_finish(mTxChan, ch_packet_size);
	int64_t last_action = GetCurrentTime();
	while (!hd_channel_read_start(mRxChan, ( void * ) &lRxBuffer, &lSize, 0)) {
		if (last_action+fTimeout <= GetCurrentTime()) {
			hd_packet_rpc_init_t lInitPacket;
			memset ( &lInitPacket, 0, sizeof ( lInitPacket ) );
			if(hde_send(&lInitPacket.header, HD_PACKET_RPC_INIT, sizeof(lInitPacket), 0) < 0)
				hde_done();
			if(fErr) *fErr = EIO;
			return -1;
		} // if
		usleep(10000);
	} // while
	int lRet = -1;
	if(fErr) *fErr = 0;
	hd_packet_rpc_cmd_t lRcvPacket;
	safe_memcpy(&lRcvPacket, lRxBuffer, sizeof(lRcvPacket));
	if((lCmdPacket.header.magic  == lRcvPacket.header.magic) &&
	   (lCmdPacket.header.seq_nr == lRcvPacket.header.seq_nr) &&
	   (lCmdPacket.header.type   == lRcvPacket.header.type)) {
		lRet   = lRcvPacket.cmd;
		if(fErr) *fErr = lRcvPacket.param1;
		unsigned int lSize = fDataSize;
		if(lRcvPacket.data_size < lSize)
			lSize = lRcvPacket.data_size;
		if (fData && lSize)
			safe_memcpy(fData, ((unsigned char *)lRxBuffer)+sizeof(lRcvPacket), lSize);
	} // if
	hd_channel_read_finish(mRxChan);
	return lRet;
} // hde_send_cmd

static int hde_send_data ( unsigned int fCmd, unsigned int fParam1, unsigned int fParam2, void *fData, unsigned int fDataSize, long fTimeout, int *fErr) {
	void *lTxBuffer = 0;
	int lSize = 0;
	if(fErr) *fErr = EINVAL;
	if(!mTxChan)
		return -1;
	if(fErr) *fErr = EIO;
	int64_t last_action = GetCurrentTime();
	hd_packet_rpc_cmd_t lCmdPacket;
	memset ( &lCmdPacket, 0, sizeof(lCmdPacket));
	lCmdPacket.header.magic       = HD_PACKET_MAGIC;
	lCmdPacket.header.seq_nr      = mSeqNr++;
	lCmdPacket.header.type        = HD_PACKET_RPC_CMD;
	lCmdPacket.header.packet_size = sizeof(lCmdPacket) +fDataSize;
	lCmdPacket.cmd       = fCmd;
	lCmdPacket.param1    = fParam1;
	lCmdPacket.param2    = fParam2;
	lCmdPacket.data_size = fDataSize;
	int ch_packet_size = (lCmdPacket.header.packet_size + 3) & 0x7FFFFFFC; // Align 4
	while(!(lSize = hd_channel_write_start(mTxChan, &lTxBuffer, ch_packet_size, 0))) {
		if(last_action+fTimeout <= GetCurrentTime()) {
			if(fTimeout)
				printf("hd_channel_write_start timeout! %ld %d\n", fTimeout, sizeof(hd_packet_rpc_cmd_t) +fDataSize);
			return -1;
		} // if
		usleep ( 10 );
	} // while
	if(!lSize) {
		printf("hd_channel_write_start timeout! %lld %d\n", WAIT_TIMEOUT_US, sizeof(hd_packet_rpc_cmd_t) +fDataSize);
		return -1;
	} // if
	if(fErr) fErr = 0;
	safe_memcpy(lTxBuffer, &lCmdPacket, sizeof(lCmdPacket));
	if (fData && fDataSize)
		safe_memcpy(((unsigned char *)lTxBuffer)+sizeof(lCmdPacket), fData, fDataSize);
	hd_channel_write_finish(mTxChan, ch_packet_size);
	return 0;
} // hde_send_data

int hde_clear_tx_queue(void) {
	if (!mTxChan)
		return -1;
	if(!hd_channel_invalidate(mTxChan, 100))
		return 0;
	return -1;
} // hde_clear_tx_queue

void hde_done() {
	hd_packet_rpc_done_t lDonePacket;
	memset ( &lDonePacket, 0, sizeof ( lDonePacket ) );
	hde_osd_xine_clear();
	pthread_mutex_lock (&mInitLock);
	if ( mDeinit ) {
		hde_send ( &lDonePacket.header, HD_PACKET_RPC_DONE, sizeof ( lDonePacket ), 0 );
		printf("HD_PACKET_RPC_DONE\n");
	} // if
	mDeinit = 0;
// Unable to hd_channel_open after hd_channel_close????
//    if ( mTxChan )
//        hd_channel_close ( mTxChan );
//    mTxChan = 0;
// Unable to hd_channel_open after hd_channel_close????
//    if ( mRxChan )
//        hd_channel_close ( mRxChan );
//    mRxChan = 0;
// Unable to hd_channel_open after hd_channel_close????
//    if ( mOsdChan )
//        hd_channel_close ( mOsdChan );
//    mOsdChan = 0;
// Vdr does not start the player again...
//    if(mCtrl)
//        mCtrl->hdp_enable = 0;
	mCtrl = 0;
// Unable to hd_get_area after hd_free_area????
//    if(mHsa)
//        hd_free_area(mHsa);
//    mHsa = 0;
	pthread_mutex_unlock (&mInitLock);
} // hde_done

int hde_is_init () {
//    pthread_mutex_lock (&mInitLock);
	int ret = mTxChan != 0 && mRxChan != 0 && mOsdChan != 0 && mCtrl != 0 && mCtrl->hdp_running && mCtrl->active_player[HDE_PLAYER] == HD_PLAYER_MODE_RPC;
//    pthread_mutex_unlock (&mInitLock);
	return ret;
} // hde_is_init

int hde_init () {
	hd_packet_rpc_init_t lInitPacket;
	if(!mInit) {
		if ( hd_init ( 0 ) != 0 )
			return 0;
		pthread_mutex_init ( &mInitLock, NULL );
		mInit = 1;
	} // if
	memset ( &lInitPacket, 0, sizeof ( lInitPacket ) );
	pthread_mutex_lock (&mInitLock);
	mDeinit = 0;
//    if(!mHsa)
//        mHsa = hd_get_area(HDID_HDA);
	int64_t last_action = GetCurrentTime();
	while (!mHsa && (last_action+WAIT_TIMEOUT_US > GetCurrentTime())) {
		mHsa = hd_get_area(HDID_HDA);
		if ( !mHsa )
			usleep ( WAIT_BUSY_US );
	} // while
	if(!mHsa) {
		mCtrl = 0;
		llprintf(LOG_ERR, "Failed to get hde aerea\n");
		goto error;
	} // if
	mCtrl = (hd_data_t *)mHsa->mapped;
	if(!mCtrl) {
		llprintf(LOG_ERR, "Failed to get ctrl\n");
		goto error;
	} // if
	last_action = GetCurrentTime();
	while (!mCtrl->hdp_running && (last_action+WAIT_TIMEOUT_US > GetCurrentTime())) {
		mCtrl->hdp_enable = 1;
		usleep ( WAIT_BUSY_US );
	} // while
	if(!mCtrl->hdp_running) {
		llprintf(LOG_ERR, "Failed to start hdplayer\n");
		goto error;
	} // if
	last_action = GetCurrentTime();
	while (!mTxChan && (last_action+WAIT_TIMEOUT_US > GetCurrentTime())) {
#if HDE_PLAYER==0
		mTxChan = hd_channel_open ( HDCH_STREAM1 );
#else
		mTxChan = hd_channel_open ( HDCH_STREAM2 );
#endif
		if ( !mTxChan )
			usleep ( WAIT_BUSY_US );
	} // while
	if ( !mTxChan ) {
		llprintf(LOG_ERR, "Failed to get tx channel\n");
		goto error;
	} // if
	last_action = GetCurrentTime();
	while (!mRxChan && (last_action+WAIT_TIMEOUT_US > GetCurrentTime())) {
#if HDE_PLAYER==0
		mRxChan = hd_channel_open ( HDCH_STREAM1_INFO );
#else
		mRxChan = hd_channel_open ( HDCH_STREAM2_INFO );
#endif		
		if ( !mRxChan )
			usleep ( WAIT_BUSY_US );
	} // for
	if ( !mRxChan ) {
		llprintf(LOG_ERR, "Failed to get rx channel\n");
		goto error;
	} // if
	last_action = GetCurrentTime();
	while (!mOsdChan && (last_action+WAIT_TIMEOUT_US > GetCurrentTime())) {
		mOsdChan = hd_channel_open ( HDCH_XINE_OSD );
		if ( !mOsdChan )
			usleep ( WAIT_BUSY_US );
	} // for
	if ( !mOsdChan ) {
		llprintf(LOG_ERR, "Failed to get osd channel\n");
		goto error;
	} // if
	if ( hde_send ( &lInitPacket.header, HD_PACKET_RPC_INIT, sizeof ( lInitPacket ), 0 ) < 0)
		goto error;
	printf("HD_PACKET_RPC_INIT\n");
	mDeinit = 1;
	pthread_mutex_unlock (&mInitLock);
	hde_osd_xine_clear();
	hd_vdec_config_t vcfg;
	memset(&vcfg, 0, sizeof(vcfg));
	hde_write_video_cfg(&vcfg);
	hd_adec_config_t acfg;
	memset(&acfg, 0, sizeof(acfg));
	hde_write_audio_cfg(&acfg);
	return 1;
error:
	pthread_mutex_unlock (&mInitLock);
	hde_done();
	return 0;
} // hde_init

void hde_osd_scale(int *x, int *y, int *w, int *h, int sw, int sh, int sax, int say) {
	pthread_mutex_lock (&mInitLock);
	if(!mCtrl) {
		pthread_mutex_unlock (&mInitLock);
		return;
	} // if
	hd_data_t lCtrl = *mCtrl;
	pthread_mutex_unlock (&mInitLock);
#if 0 // GA: Quick hack to make it compile FIXME!!!	
	switch(lCtrl.aspect.scale & HD_VM_SCALE_FB) {
		case HD_VM_SCALE_FB_DEF:
			*w = 720;//800; Old default overwritten?
			*h = 576;//600; Old default overwritten?
			break;
		case HD_VM_SCALE_FB_FIT:
			*w = 720;
			*h = 576;
			break;
		case HD_VM_SCALE_FB_DBD:
			return;
		default:
			return;
	} // switch
#else
	*w = 720;
	*h = 576;	                        
#endif
	*h = (sh * sh) / *h;
	*w = (sw * sw) / *w;
	switch(lCtrl.aspect.scale & HD_VM_SCALE_VID) {
		case HD_VM_SCALE_F2S: {// Fill to screen
			if(sw <= 720) {
				if(sw > 480)
					*w -= 32;
				else 
					*w -= 16;
				*x += (*w - sw) / 2;
			} // if
			break;
		} // HD_VM_SCALE_F2S
		case HD_VM_SCALE_F2A: { // Fill to aspect
			if(lCtrl.video_mode.current_w <= 720) {
				*w += 24;//Don't ask why...
				*x += (*w - sw) / 2;
			} // if
			double o = ((double)lCtrl.aspect.w)/((double)lCtrl.aspect.h);
			double s = ((double)sax)/((double)say);
			int ow = *w;
			*w = o* *w/s;
			*x += (*w - ow) / 2;
			break;
		} // HD_VM_SCALE_F2A
		case HD_VM_SCALE_C2F: { // Crop to fill
			if(sw <= 720) {
				if(sw > 480)
					*w -= 32;
				else 
					*w -= 16;
				*x += (*w - sw) / 2;
			} // if
			*h -= 32;
			*y += (*h - sh) /2;
			double o = ((double)lCtrl.aspect.w)/((double)lCtrl.aspect.h);
			double s = ((double)sax)/((double)say);
			int oh = *h;
			*h = s* *h/o;
			*y += (*h - oh) / 2;
			break;
		} // HD_VM_SCALE_C2F
		case HD_VM_SCALE_DBD: // Dot by dot
			break; // Not used??
	} // switch
} // hde_fb_scale

int hde_osd_clear(void) {
	int err = 0;
	hdcmd_osd_clear_t lCmd;
	memset(&lCmd, 0, sizeof(lCmd));
	lCmd.cmd = HDCMD_OSD_CLEAR;
	pthread_mutex_lock (&mInitLock);
	int ret = hde_send_osd(&lCmd, sizeof(lCmd), &err);
	pthread_mutex_unlock (&mInitLock);
	if(ret)
		printf ( "hde_osd_xine_start FAILED %d\n", err );
	return ret;
} // hde_osd_clear

int hde_osd_xine_clear() {
	if(!hde_osd_xine_start())
		return hde_osd_xine_end();
	return -1;
} // hde_osd_xine_clear

int hde_osd_xine_start() {
	int err = 0;
	hdcmd_osd_xine_start lCmd;
	lCmd.cmd = HDCMD_OSD_XINE_START;
	pthread_mutex_lock (&mInitLock);
	int ret = hde_send_osd(&lCmd, sizeof(lCmd), &err);
	pthread_mutex_unlock (&mInitLock);
	if(ret)
		printf ( "hde_osd_xine_start FAILED %d\n", err );
	return ret;
} // hde_osd_xine_start

int hde_osd_xine_tile(hdcmd_osd_overlay *overlay, int img_width, int img_height, int offset_x, int offset_y, int dst_width, int dst_height) {
	int err = 0;
	int rle_size   = overlay->num_rle*sizeof(hdcmd_osd_rle_elem);
	int total_size = sizeof(hdcmd_osd_xine_tile)+rle_size;
	hdcmd_osd_xine_tile *lCmd = (hdcmd_osd_xine_tile *)malloc(total_size);
	if(!lCmd)
		return -1;
	lCmd->cmd = HDCMD_OSD_XINE_TILE;
	lCmd->tile.img_width  = img_width;
	lCmd->tile.img_height = img_height;
	lCmd->tile.offset_x   = offset_x;
	lCmd->tile.offset_y   = offset_y;
	lCmd->tile.dst_width  = dst_width;
	lCmd->tile.dst_height = dst_height;
	memcpy(&lCmd->tile.overlay, overlay, sizeof(hdcmd_osd_overlay));
	lCmd->tile.overlay.data_size = rle_size;
	lCmd->tile.overlay.rle       = 0;
	memcpy(((uint8_t *)lCmd)+sizeof(hdcmd_osd_xine_tile), overlay->rle, rle_size);
	pthread_mutex_lock (&mInitLock);
	int ret = hde_send_osd(lCmd, total_size, &err);
	pthread_mutex_unlock (&mInitLock);
	if(ret)
		printf ( "hde_osd_xine_tile FAILED %d\n", err );
	free(lCmd);
	return ret;
} // hde_osd_xine_tile

int hde_osd_xine_end() {
	int err = 0;
    mCtrl->plane[2].changed++;

	hdcmd_osd_xine_end lCmd;
	lCmd.cmd = HDCMD_OSD_XINE_END;
	pthread_mutex_lock (&mInitLock);
	int ret = hde_send_osd(&lCmd, sizeof(lCmd), &err);
	pthread_mutex_unlock (&mInitLock);
	if(ret)
		printf ( "hde_osd_xine_end FAILED %d\n", err );
	return ret;
} // hde_osd_xine_end

int hde_get_vol(int *vol) {
	int ret = 0;
	pthread_mutex_lock (&mInitLock);
	if(!mCtrl) {
		ret = -1;
	} else {
		hd_data_t lCtrl = *mCtrl;
		*vol = lCtrl.audio_control.volume1;
	} // if
	pthread_mutex_unlock (&mInitLock);
	return ret;
} // hde_get_vol

int hde_set_vol(int vol) {
	int ret = 0;
	pthread_mutex_lock (&mInitLock);
	if(!mCtrl) {
		ret = -1;
	} else {
		hd_audio_control_t lCtrl = mCtrl->audio_control;
		lCtrl.volume1 = vol;
		lCtrl.changed++;
		mCtrl->audio_control = lCtrl;
	} // if
	pthread_mutex_unlock (&mInitLock);
	return ret;
} // hde_set_vol

hd_vdec_config_t gLastVdecCfg = { 0,};
int hde_write_video_cfg(hd_vdec_config_t *cfg) {
	int lErr = 0;
	pthread_mutex_lock (&mInitLock);
	gLastVdecCfg = *cfg;
	int lRet = hde_send_cmd ( RPC_CMD_VCFG, 0, 0, cfg, sizeof(hd_vdec_config_t), WAIT_TIMEOUT_US, &lErr);
	pthread_mutex_unlock (&mInitLock);
	printf("hde_write_video_cfg \n");
	if(lRet)
		printf("hde_write_video_cfg failed: %d (%d)\n", lRet, lErr);
	return lRet;
} // hde_write_video_cfg

#define MAX_VIDEO_BLOCK_SIZE (0x8000 - sizeof(hd_packet_header_t))
int hde_write_video_data(uint8_t *data, int32_t size, int64_t pts) {
	int lErr = 0;
	int lRet = -1;
	hd_vdec_config_t lCurVdecCfg = mCtrl->player[HDE_PLAYER].vdec_cfg;
	if(memcmp(&gLastVdecCfg, &lCurVdecCfg, sizeof(gLastVdecCfg)))
		lRet = hde_write_video_cfg(&gLastVdecCfg);
	pthread_mutex_lock (&mInitLock);
	int64_t last_action = GetCurrentTime();
	int i = 0;
	int block_count = size/MAX_VIDEO_BLOCK_SIZE;
	long lTimeout = WAIT_TIMEOUT_US;
	pts |= HDE_FLAG_FRAME_START_PTS;
	while (i < block_count) {
		unsigned char *data_pos = data+i*MAX_VIDEO_BLOCK_SIZE;
		if(hde_send_data( RPC_CMD_VDATA, pts>>32, pts&0xFFFFFFFF, data_pos, MAX_VIDEO_BLOCK_SIZE, 0, &lErr)) {
			if(last_action+lTimeout <= GetCurrentTime())
				goto cleanup;	
			usleep ( 1000 );
		} else {
			i++;
			last_action = GetCurrentTime();
			pts &= ~HDE_FLAG_FRAME_START_PTS;
		} // if
	} // while
	pts |= HDE_FLAG_FRAME_END_PTS;
	while(hde_send_data( RPC_CMD_VDATA, pts>>32, pts&0xFFFFFFFF, data+block_count*MAX_VIDEO_BLOCK_SIZE, size-block_count*MAX_VIDEO_BLOCK_SIZE, 0, &lErr)) {
		if(last_action+lTimeout <= GetCurrentTime())
			goto cleanup;	
		usleep ( 1000 );
	} // while
	lRet = 0;
cleanup:
	pthread_mutex_unlock (&mInitLock);
	if(lRet)
		printf("hde_write_video_data failed: %d (%d)\n", lRet, lErr);
	return lRet;
} // hde_write_video_data

int hde_video_reset() {
	printf("hde_video_reset\n");
//	hde_clear_tx_queue();
	gLastVdecCfg.generation++;
	hde_write_video_cfg(&gLastVdecCfg);
	return 0;
} // hde_video_reset

int hde_video_resync() {
	printf("hde_video_resync\n");
//	hde_clear_tx_queue();
	gLastVdecCfg.av_sync++;
	hde_write_video_cfg(&gLastVdecCfg);
	return 0;
} // hde_video_resync

hd_adec_config_t gLastAdecCfg = { 0,};
int hde_write_audio_cfg(hd_adec_config_t *cfg) {
	int lErr = 0;
	pthread_mutex_lock (&mInitLock);
	gLastAdecCfg = *cfg;
	int lRet = hde_send_cmd ( RPC_CMD_ACFG, 0, 0, cfg, sizeof(hd_adec_config_t), WAIT_TIMEOUT_US, &lErr);
	pthread_mutex_unlock (&mInitLock);
	printf("hde_write_audio_cfg \n");
	if(lRet)
		printf("hde_write_audio_cfg failed: %d (%d)\n", lRet, lErr);
	return lRet;
} // hde_write_audio_cfg

int hde_write_audio_data(uint8_t *data, int32_t size, int64_t pts) {
	int lErr = 0;
	int lRet = 0;
	hd_adec_config_t lCurAdecCfg = mCtrl->player[HDE_PLAYER].adec_cfg;
	if(memcmp(&gLastAdecCfg, &lCurAdecCfg, sizeof(gLastAdecCfg)))
		lRet = hde_write_audio_cfg(&gLastAdecCfg);
	pthread_mutex_lock (&mInitLock);
	if(!lRet)
		lRet = hde_send_data ( RPC_CMD_ADATA, pts>>32, pts&0xFFFFFFFF, data, size, WAIT_TIMEOUT_US, &lErr);
	pthread_mutex_unlock (&mInitLock);
	if(lRet)
		printf("hde_write_audio_data failed: %d (%d)\n", lRet, lErr);
	return lRet;
} // hde_write_audio_data

int hde_audio_reset() {
//	hde_clear_tx_queue();
	printf("hde_audio_reset\n");
	gLastAdecCfg.generation++;
	hde_write_audio_cfg(&gLastAdecCfg);
	return 0;
} // hde_audio_reset

int hde_set_speed(int64_t speed) {
	int ret = 0;
	pthread_mutex_lock (&mInitLock);
	if(!mCtrl) {
		ret = -1;
	} else {
		int trickmode, trickspeed, pause = (speed == 0);
		if(pause) {
			trickmode = mCtrl->player[HDE_PLAYER].trickmode;
			trickspeed = mCtrl->player[HDE_PLAYER].trickspeed;
		} else if (speed == 90000) { // Normal
			trickmode = 1;
			trickspeed = 0;
		} else if (speed > 90000) { // FF
			trickmode = 1;
			trickspeed = 2;
			if (speed > 180000)
				trickspeed = 1; // what the hell are correct values??
		} else { //SF
			trickmode = 0;
			trickspeed = 2;
			if (speed < 45000)
				trickspeed = 4;
		}
		mCtrl->player[HDE_PLAYER].pause      = pause;
		mCtrl->player[HDE_PLAYER].trickmode  = trickmode;
		mCtrl->player[HDE_PLAYER].trickspeed = trickspeed;
		printf("New speed: %lld -> %d %d %d\n", speed, pause, trickmode, trickspeed);
	} // if
	pthread_mutex_unlock (&mInitLock);
	hde_get_speed(&speed);
	return ret;
} // hde_set_speed

int hde_get_speed(int64_t *speed) {
	int ret = 0;
	pthread_mutex_lock (&mInitLock);
	if(!mCtrl) {
		ret = -1;
	} else {
		if(mCtrl->player[HDE_PLAYER].pause) {
			*speed = 0;
		} else {
			int trickmode  = mCtrl->player[HDE_PLAYER].trickmode;
			int trickspeed = mCtrl->player[HDE_PLAYER].trickspeed;
			if(/*trickmode && */!trickspeed) {
				*speed = 90000;
			} else if (trickmode) {
				*speed=180000;
				if(trickspeed == 1)
					*speed=360000;
			} else {
				*speed = 45000;
				if (trickspeed == 4)
					*speed = 22500;
			} // if
		} // if
	} // if
	pthread_mutex_unlock (&mInitLock);
	return ret;
} // hde_io_get_speed

int hde_get_stc(int64_t *stc) {
	int ret = 0;
	pthread_mutex_lock (&mInitLock);
	if(!mCtrl) {
		ret = -1;
	} else {
		*stc = mCtrl->player[HDE_PLAYER].hde_stc_base_low&0xFFFFFFFF;
		*stc += (((int64_t)mCtrl->player[HDE_PLAYER].hde_stc_base_high&0xFFFFFFFF)<<32);
//		*stc += HDE_PTS_DELAY;
	} // if
	pthread_mutex_unlock (&mInitLock);
	return ret;
} // hde_get_stc

int hde_adjust_stc(int64_t pts, int64_t *stc) {
	int ret = 0;
	pthread_mutex_lock (&mInitLock);
	if(!mCtrl) {
		ret = -1;
	} else {
		*stc = mCtrl->player[HDE_PLAYER].hde_stc_base_low&0xFFFFFFFF;
		*stc += (((int64_t)mCtrl->player[HDE_PLAYER].hde_stc_base_high&0xFFFFFFFF)<<32);
//		*stc += HDE_PTS_DELAY;
		int64_t diff = *stc-pts+HDE_PTS_DELAY;
		mCtrl->player[HDE_PLAYER].hde_stc_offset_low  = diff&0xFFFFFFFF;
		mCtrl->player[HDE_PLAYER].hde_stc_offset_high = (diff>>32)&0xFFFFFFFF;
	} // if
	pthread_mutex_unlock (&mInitLock);
	return ret;
} // hde_adjust_stc
