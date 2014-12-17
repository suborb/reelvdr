/*
 * H.26L/H.264/AVC/JVT/14496-10/... encoder/decoder
 * Copyright (c) 2003 Michael Niedermayer <michaelni@gmx.at>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

/**
 * @file h264.c
 * stripped down version of H.264 / AVC / MPEG4 part10 codec.
 * @author Michael Niedermayer <michaelni@gmx.at>
 */

#include "parser.h"
#include "golomb.h"

//#undef NDEBUG
#include <assert.h>
#include <syslog.h>

const uint8_t start_code[] = {0,0,1};

typedef struct H264Buffer {
	unsigned int pos;
	unsigned int size;
	uint8_t *data;
} H264Buffer;

typedef struct SliceHeader {
	int defined;
	int isFirstSliceOfCurrentAccessUnit;
	uint32_t picOrderCntType;
	uint32_t nal_ref_idc;
	uint32_t nal_unit_type;
	uint32_t slice_type;
	uint32_t pic_parameter_set_id;
	uint32_t frame_num;
	uint32_t field_pic_flag;
	uint32_t bottom_field_flag;
	uint32_t idr_pic_id;
	uint32_t pic_order_cnt_lsb;
	int32_t delta_pic_order_cnt_bottom;
	int32_t delta_pic_order_cnt[2];
} SliceHeader;

typedef struct PictureTiming {
	int defined;
	uint32_t pic_struct;
} PictureTiming;

typedef struct SequenceParameterSet {
	int defined;
	uint32_t log2_max_frame_num_minus4;
	uint32_t log2_max_pic_order_cnt_lsb_minus4;
	uint32_t cpb_removal_delay_length_minus1;
	uint32_t dpb_output_delay_length_minus1;
	uint32_t seq_parameter_set_id;
	uint32_t pic_order_cnt_type;
	uint32_t delta_pic_order_always_zero_flag;
	uint32_t frame_mbs_only_flag;
	uint32_t timing_info_present_flag;
	uint32_t num_units_in_tick;
	uint32_t time_scale;
	uint32_t fixed_frame_rate_flag;
	uint32_t nal_hrd_parameters_present_flag;
	uint32_t vcl_hrd_parameters_present_flag;
	uint32_t pic_width_in_mbs_minus1;
	uint32_t pic_height_in_map_units_minus1;
	uint32_t pic_struct_present_flag;
	uint32_t aspect_ratio_idc;
	uint32_t sar_width;
	uint32_t sar_height;
	PictureTiming pic_timing_sei;
} SequenceParameterSet;

typedef struct PictureParameterSet {
	int defined;
	uint32_t pic_parameter_set_id;
	uint32_t seq_parameter_set_id;
	uint32_t pic_order_present_flag;
} PictureParameterSet;

typedef struct H264Context {
	H264Buffer in;
	H264Buffer out;
	int nal_length_size;
	int is_avc;
	int slice_count;
	int slice_types;
	int got_frame;
	int avc_header_pos;
	int avc_header_sent;
	SequenceParameterSet spsStore[32];
	PictureParameterSet ppsStore[256];
	SequenceParameterSet *sps; // active Sequence Parameter Set
	PictureParameterSet *pps; // active Picture Parameter Set
	SliceHeader sh;
} H264Context;

void put_buffer(H264Buffer *buffer, const uint8_t *data, int size) {
	buffer->data = av_fast_realloc ( buffer->data, &buffer->size, buffer->pos+size);
	memcpy(&buffer->data[buffer->pos], data, size);
	buffer->pos += size;
} // put_buffer

void del_buffer(H264Buffer *buffer, int size) {
	if(!size) return;
	buffer->pos -= size;
	if(buffer->pos > 0)
		memcpy(&buffer->data[0], &buffer->data[size], buffer->pos);
	else
		buffer->pos = 0;
} // del_buffer

int find_start_code(unsigned char *data, int size) {
	unsigned char *start=data;
	while(size>=3) {
		if ( !data[0] && !data[1] && data[2] == 1 )
			return data-start;
		data++; size--;
	} // while
	return -1;
} // find_start_code

int get_nal(AVCodecParserContext *s, AVCodecContext *avctx, int *pos, int *size) {
	H264Context *h = s->priv_data;
	if(h->is_avc) {
		*pos = h->nal_length_size;
		if (h->in.pos < h->nal_length_size)
			return 0;
		int i;
		for ( i=0, *size = 0; i < h->nal_length_size; i++ )
			*size = ( *size << 8 ) | h->in.data[i];
		*size += *pos;
		if (h->in.pos < *size)
			return 0;
		return *size;
	} else {
		int start = find_start_code(h->in.data, h->in.pos);
		if(start == -1) 
			return 0;
		int end = find_start_code(&h->in.data[start+3], h->in.pos-start-3);
		if(end == -1) 
			return 0;
		*pos = start+3;
		*size = end+*pos;
		return *size;
	} // if
	return 0;
} // get_nal

int ActivateSPS(H264Context *h, uint32_t ID) {
	if (ID >= (sizeof (h->spsStore) / sizeof (*h->spsStore)))
		return 0; // ERROR: H264::cContext::ActivateSPS(): id out of range
	if (!h->spsStore[ID].defined)
		return 0; // ERROR: H264::cContext::ActivateSPS(): requested SPS is undefined
	h->sps = &h->spsStore[ID];
	return 1;
} // ActivateSPS

int ActivatePPS(H264Context *h, uint32_t ID) {
	if (ID >= (sizeof (h->ppsStore) / sizeof (*h->ppsStore)))
		return 0; // ERROR: H264::cContext::ActivatePPS(): id out of range
	if (!h->ppsStore[ID].defined)
		return 0; // ERROR: H264::cContext::ActivatePPS(): requested PPS is undefined
	h->pps = &h->ppsStore[ID];
	if(!ActivateSPS(h, h->pps->seq_parameter_set_id))
		return 0;
	return 1;
} // ActivatePPS

void DefineSliceHeader(H264Context *h, SliceHeader *SH) {
	SH->defined = 1;
	SH->picOrderCntType = h->sps->pic_order_cnt_type;
	// ITU-T Rec. H.264 (03/2005): 7.4.1.2.4
	SH->isFirstSliceOfCurrentAccessUnit = !h->sh.defined
		|| (h->sh.frame_num                  != SH->frame_num)
		|| (h->sh.pic_parameter_set_id       != SH->pic_parameter_set_id)
		|| (h->sh.field_pic_flag             != SH->field_pic_flag)
		|| (h->sh.bottom_field_flag          != SH->bottom_field_flag)
		|| (h->sh.nal_ref_idc                != SH->nal_ref_idc
		&& (h->sh.nal_ref_idc == 0           || SH->nal_ref_idc == 0))
		|| (h->sh.picOrderCntType == 0       && SH->picOrderCntType == 0
		&& (h->sh.pic_order_cnt_lsb          != SH->pic_order_cnt_lsb
		||  h->sh.delta_pic_order_cnt_bottom != SH->delta_pic_order_cnt_bottom))
		|| (h->sh.picOrderCntType == 1       && SH->picOrderCntType == 1
		&& (h->sh.delta_pic_order_cnt[0]     != SH->delta_pic_order_cnt[0]
		||  h->sh.delta_pic_order_cnt[1]     != SH->delta_pic_order_cnt[1]))
		|| (h->sh.nal_unit_type              != SH->nal_unit_type
		&& (h->sh.nal_unit_type == 5         || SH->nal_unit_type == 5))
		|| (h->sh.nal_unit_type == 5         && SH->nal_unit_type == 5
		&&  h->sh.idr_pic_id                 != SH->idr_pic_id);
	h->sh = *SH;
} // DefineSliceHeader

void DefineSequenceParameterSet(H264Context *h, SequenceParameterSet *SPS) {
	if (SPS->seq_parameter_set_id >= (sizeof (h->spsStore) / sizeof (*h->spsStore)))
		return; // ERROR: H264::cContext::DefineSPS(): id out of range
	SPS->defined = 1;
	h->spsStore[SPS->seq_parameter_set_id] = *SPS;
} // DefineSequenceParameterSet

void DefinePictureParameterSet(H264Context *h, PictureParameterSet *PPS) {
	if (PPS->pic_parameter_set_id >= (sizeof (h->ppsStore) / sizeof (*h->ppsStore)))
		return; // ERROR: H264::cContext::DefinePPS(): id out of range
	PPS->defined = 1;
	h->ppsStore[PPS->pic_parameter_set_id] = *PPS;
} // DefinePictureParameterSet

int ParseSlice(H264Context *h, const uint8_t *Data, int Count) {
	SliceHeader SH;
	memset(&SH, 0, sizeof(SH));
	SH.nal_ref_idc = Data[0] >> 5;
	GetBitContext gb;
	init_get_bits(&gb, Data+1, (Count-1)*8 );
	get_ue_golomb(&gb); // first_mb_in_slice
	SH.slice_type = get_ue_golomb(&gb);
	SH.pic_parameter_set_id = get_ue_golomb(&gb);
	if(!ActivatePPS(h, SH.pic_parameter_set_id))
		return 0;
	const SequenceParameterSet *SPS = h->sps;
	SH.frame_num = get_bits(&gb, SPS->log2_max_frame_num_minus4+4);
	if (!SPS->frame_mbs_only_flag) {
		SH.field_pic_flag = get_bits(&gb, 1);
		if (SH.field_pic_flag)
			SH.bottom_field_flag = get_bits(&gb, 1);
	} // if
	if (SH.nal_unit_type == 5)
		SH.idr_pic_id = get_ue_golomb(&gb);
	if (SPS->pic_order_cnt_type == 0) {
		SH.pic_order_cnt_lsb = get_bits(&gb, SPS->log2_max_pic_order_cnt_lsb_minus4+4);
		const PictureParameterSet *PPS = h->pps;
		if (PPS->pic_order_present_flag && !SH.field_pic_flag)
			SH.delta_pic_order_cnt_bottom = get_se_golomb(&gb);
	} // if
	if (SPS->pic_order_cnt_type == 1 && !SPS->delta_pic_order_always_zero_flag) {
		SH.delta_pic_order_cnt[0] = get_se_golomb(&gb);
		const PictureParameterSet *PPS = h->pps;
		if (PPS->pic_order_present_flag && !SH.field_pic_flag)
			SH.delta_pic_order_cnt[1] = get_se_golomb(&gb);
	} // if
	DefineSliceHeader(h, &SH);
	return 1;
} // ParseSlice

void parseHeaderParameters(SequenceParameterSet *SPS, GetBitContext *gb) {
	uint32_t cpb_cnt_minus1 = get_ue_golomb(gb);
	/* uint32_t bit_rate_scale = */ get_bits(gb, 4);
	/* uint32_t cpb_size_scale = */ get_bits(gb, 4);
	uint32_t i;
	for (i = 0; i <= cpb_cnt_minus1; i++) {
		/* uint32_t bit_rate_value_minus1 = */ get_ue_golomb(gb);
		/* uint32_t cpb_size_value_minus1 = */ get_ue_golomb(gb);
		/* uint32_t cbr_flag = */ get_bits(gb, 1);
	} // for
	/* uint32_t initial_cpb_removal_delay_length_minus1 = */ get_bits(gb, 5);
	SPS->cpb_removal_delay_length_minus1 = get_bits(gb, 5);
	SPS->dpb_output_delay_length_minus1 = get_bits(gb, 5);
	/* uint32_t time_offset_length = */ get_bits(gb, 5);
} // parseHeaderParameters

void ParseSequenceParameterSet(H264Context *h, const uint8_t *Data, int Count) {
	SequenceParameterSet SPS;
	memset(&SPS, 0, sizeof(SPS));
	GetBitContext gb;
	init_get_bits(&gb, Data+1, (Count-1)*8 );
	uint32_t i, j;

	uint32_t profile_idc = get_bits(&gb, 8);
	/* uint32_t constraint_set0_flag = */ get_bits(&gb, 1);
	/* uint32_t constraint_set1_flag = */ get_bits(&gb, 1);
	/* uint32_t constraint_set2_flag = */ get_bits(&gb, 1);
	/* uint32_t constraint_set3_flag = */ get_bits(&gb, 1);
	/* uint32_t reserved_zero_4bits = */ get_bits(&gb, 4);
	/* uint32_t level_idc = */ get_bits(&gb, 8);
	SPS.seq_parameter_set_id = get_ue_golomb(&gb);
	if (profile_idc == 100 || profile_idc == 110 || profile_idc == 122 || profile_idc == 144) {
		uint32_t chroma_format_idc = get_ue_golomb(&gb);
		if (chroma_format_idc == 3) {
			/* uint32_t residual_colour_transform_flag = */ get_bits(&gb, 1);
		} // if
		/* uint32_t bit_depth_luma_minus8 = */ get_ue_golomb(&gb);
		/* uint32_t bit_depth_chroma_minus8 = */ get_ue_golomb(&gb);
		/* uint32_t qpprime_y_zero_transform_bypass_flag = */ get_bits(&gb, 1);
		uint32_t seq_scaling_matrix_present_flag = get_bits(&gb, 1);
		if (seq_scaling_matrix_present_flag) {
			for (i = 0; i < 8; i++) {
				uint32_t seq_scaling_list_present_flag = get_bits(&gb, 1);
				if (seq_scaling_list_present_flag) {
					int sizeOfScalingList = (i < 6) ? 16 : 64;
					int lastScale = 8;
					int nextScale = 8;
					for (j = 0; j < sizeOfScalingList; j++) {
						if (nextScale != 0) {
							int32_t delta_scale = get_se_golomb(&gb);
							nextScale = (lastScale + delta_scale + 256) % 256;
						} // if
						lastScale = (nextScale == 0) ? lastScale : nextScale;
					} // for
				} // if
			} // for
		} // if
	} // if
	SPS.log2_max_frame_num_minus4 = get_ue_golomb(&gb);
	SPS.pic_order_cnt_type = get_ue_golomb(&gb);
	if (SPS.pic_order_cnt_type == 0)
		SPS.log2_max_pic_order_cnt_lsb_minus4 = get_ue_golomb(&gb);
	else if (SPS.pic_order_cnt_type == 1) {
		SPS.delta_pic_order_always_zero_flag = get_bits(&gb, 1);
		/* int32_t offset_for_non_ref_pic = */ get_se_golomb(&gb);
		/* int32_t offset_for_top_to_bottom_field = */ get_se_golomb(&gb);
		uint32_t num_ref_frames_in_pic_order_cnt_cycle = get_ue_golomb(&gb);
		for (i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++) {
			/* int32_t offset_for_ref_frame = */ get_se_golomb(&gb);
		} // for
	} // if
	/* uint32_t num_ref_frames = */ get_ue_golomb(&gb);
	/* uint32_t gaps_in_frame_num_value_allowed_flag = */ get_bits(&gb, 1);
	SPS.pic_width_in_mbs_minus1 = get_ue_golomb(&gb);
	SPS.pic_height_in_map_units_minus1 = get_ue_golomb(&gb);
	SPS.frame_mbs_only_flag = get_bits(&gb, 1);
	if (!SPS.frame_mbs_only_flag) {
		/* uint32_t mb_adaptive_frame_field_flag = */ get_bits(&gb, 1);
	} // if
	/* uint32_t direct_8x8_inference_flag = */ get_bits(&gb, 1);
	uint32_t frame_cropping_flag = get_bits(&gb, 1);
	if (frame_cropping_flag) {
		/* uint32_t frame_crop_left_offset = */ get_ue_golomb(&gb);
		/* uint32_t frame_crop_right_offset = */ get_ue_golomb(&gb);
		/* uint32_t frame_crop_top_offset = */ get_ue_golomb(&gb);
		/* uint32_t frame_crop_bottom_offset = */ get_ue_golomb(&gb);
	} // if
	uint32_t vui_parameters_present_flag = get_bits(&gb, 1);
	if (vui_parameters_present_flag) {
		uint32_t aspect_ratio_info_present_flag = get_bits(&gb, 1);
		if (aspect_ratio_info_present_flag) {
			SPS.aspect_ratio_idc = get_bits(&gb, 8);
			const uint32_t Extended_SAR = 255;
			if (SPS.aspect_ratio_idc == Extended_SAR) {
				 SPS.sar_width =  get_bits(&gb, 16);
				 SPS.sar_height = get_bits(&gb, 16);
			} // if
		} // if
		uint32_t overscan_info_present_flag = get_bits(&gb, 1);
		if (overscan_info_present_flag) {
			/* uint32_t overscan_appropriate_flag = */ get_bits(&gb, 1);
		} // if
		uint32_t video_signal_type_present_flag = get_bits(&gb, 1);
		if (video_signal_type_present_flag) {
			/* uint32_t video_format = */ get_bits(&gb, 3);
			/* uint32_t video_full_range_flag = */ get_bits(&gb, 1);
			uint32_t colour_description_present_flag = get_bits(&gb, 1);
			if (colour_description_present_flag) {
				/* uint32_t colour_primaries = */ get_bits(&gb, 8);
				/* uint32_t transfer_characteristics = */ get_bits(&gb, 8);
				/* uint32_t matrix_coefficients = */ get_bits(&gb, 8);
			} // if
		} // if
		uint32_t chroma_loc_info_present_flag = get_bits(&gb, 1);
		if (chroma_loc_info_present_flag) {
			/* uint32_t chroma_sample_loc_type_top_field = */ get_ue_golomb(&gb);
			/* uint32_t chroma_sample_loc_type_bottom_field = */ get_ue_golomb(&gb);
		} // if
		SPS.timing_info_present_flag = get_bits(&gb, 1);
		if (SPS.timing_info_present_flag) {
			/*SPS.num_units_in_tick =*/ get_bits(&gb, 32);
			SPS.num_units_in_tick = get_bits(&gb, 8);
			/*SPS.time_scale =*/ get_bits(&gb, 32);
			SPS.time_scale = get_bits(&gb, 8);
			SPS.fixed_frame_rate_flag = get_bits(&gb, 1);
		} // if
		SPS.nal_hrd_parameters_present_flag = get_bits(&gb, 1);
		if (SPS.nal_hrd_parameters_present_flag)
			parseHeaderParameters(&SPS, &gb);
		SPS.vcl_hrd_parameters_present_flag = get_bits(&gb, 1);
		if (SPS.vcl_hrd_parameters_present_flag)
			parseHeaderParameters(&SPS, &gb);
		if (SPS.nal_hrd_parameters_present_flag || SPS.vcl_hrd_parameters_present_flag) {
			/* uint32_t low_delay_hrd_flag = */ get_bits(&gb, 1);
		} // if
		SPS.pic_struct_present_flag = get_bits(&gb, 1);
	} // if
	DefineSequenceParameterSet(h, &SPS);
} // ParseSequenceParameterSet

void ParsePictureParameterSet(H264Context *h, const uint8_t *Data, int Count) {
	PictureParameterSet PPS;
	memset(&PPS, 0, sizeof(PPS));
	GetBitContext gb;
	init_get_bits(&gb, Data+1, (Count-1)*8 );

	PPS.pic_parameter_set_id = get_ue_golomb(&gb);
	PPS.seq_parameter_set_id = get_ue_golomb(&gb);
	/* uint32_t entropy_coding_mode_flag = */ get_bits(&gb, 1);
	PPS.pic_order_present_flag = get_bits(&gb, 1);
	DefinePictureParameterSet(h, &PPS);
} // ParsePictureParameterSet

#define FRAME_NO_TYPE 0
#define FRAME_I_TYPE  1
#define FRAME_P_TYPE  2
#define FRAME_B_TYPE  3
#define FRAME_SP_TYPE 4
#define FRAME_SI_TYPE 5
static const uint8_t slice_type_map[5] = {FRAME_P_TYPE, FRAME_B_TYPE, FRAME_I_TYPE, FRAME_SP_TYPE, FRAME_SI_TYPE};

int parse_nal(H264Context *h, AVCodecContext *avctx, const uint8_t *data, int size) {
	int nal_unit_type = data[0] & 0x1F;
	switch(nal_unit_type) {
		case 1: // coded slice of a non-IDR picture
		case 2: // coded slice data partition A
		case 5: {// coded slice of an IDR picture
			SliceHeader SH = h->sh;
			if(ParseSlice(h, data, size)) {
				if(h->sps && h->sps->timing_info_present_flag && h->sps->fixed_frame_rate_flag) {
					avctx->time_base.num = h->sps->num_units_in_tick *2;
					avctx->time_base.den = h->sps->time_scale;
//					av_reduce(&avctx->time_base.num, &avctx->time_base.den, avctx->time_base.num, avctx->time_base.den, 1<<30);
				} // if
				if(h->sh.isFirstSliceOfCurrentAccessUnit) {
					if(h->slice_count) {
						h->sh = SH; // Restore last slice
						h->slice_count = 0;
						return 1;
					} // if
					h->slice_count++;
				} // if
				if ( h->sh.slice_type <= 9 )
					h->slice_types |= ( h->sh.slice_type > 4 ) ? (1<<(h->sh.slice_type-5)) : (1<<(h->sh.slice_type));
			} // if
			break;
		} // Slice
		case 6: // supplemental enhancement information
			break;
		case 7: {// sequence parameter set
			ParseSequenceParameterSet(h, data, size);
			if(h->sps) {
				avctx->width  = (h->sps->pic_width_in_mbs_minus1 + 1) * 16;
				avctx->height = (h->sps->pic_height_in_map_units_minus1 + 1) * 16;
				if (!h->sps->frame_mbs_only_flag) avctx->height *= 2;
				avctx->sample_aspect_ratio.num  = 0;
				avctx->sample_aspect_ratio.den  = 1;
				if (h->sps->aspect_ratio_idc == 255) {
				    avctx->sample_aspect_ratio.num = h->sps->sar_width;
				    avctx->sample_aspect_ratio.den = h->sps->sar_height;
				} else if (h->sps->aspect_ratio_idc < 17) {
				    avctx->sample_aspect_ratio = pixel_aspect[h->sps->aspect_ratio_idc];
				}
				if(h->sps->timing_info_present_flag && h->sps->fixed_frame_rate_flag) {
					avctx->time_base.num = h->sps->num_units_in_tick *2;
					avctx->time_base.den = h->sps->time_scale;
				}
			}
			}
			break;
		case 8: // picture parameter set
			ParsePictureParameterSet(h, data, size);
			break;
		case 9: // access unit delimiter
			break;
	} // switch
	return 0;
} // parse_nal

void add_avc_header(AVCodecParserContext *s, AVCodecContext *avctx) {
	H264Context *h = s->priv_data;
	if(h->out.pos)
		return;
	if ( avctx->extradata_size > 6 ) {
		uint8_t *p = &avctx->extradata[5];
		int i, count = *(p++) & 0x1f;
		for ( i=0; i < count; i++ ) {
			int len = ((*p << 8) | * ( p+1 ));
			p += 2;
			parse_nal(h, avctx, p, len);
			put_buffer(&h->out, start_code, sizeof(start_code));
			put_buffer(&h->out, p, len);
			p += len;
		} // if
		count = *(p++) & 0x1f;
		for ( i=0; i < count; i++ ) {
			int len = ((*p << 8) | * ( p+1 ));
			p += 2;
			parse_nal(h, avctx, p, len);
			put_buffer(&h->out, start_code, sizeof(start_code));
			put_buffer(&h->out, p, len);
			p += len;
		} // if
	} // if
	h->avc_header_pos = h->out.pos;
} // add_avc_header

void got_frame(AVCodecParserContext *s) {
	H264Context *h = s->priv_data;
	h->got_frame = 1;
	s->delay_pts = h->sh.isFirstSliceOfCurrentAccessUnit;
	int slice_type = h->sh.slice_type;
	if ( slice_type <= 9 ) {
		if ( slice_type > 4 )
			slice_type -= 5;
		s->pict_type = slice_type_map[slice_type];
	} // if
	switch(h->sh.field_pic_flag + h->sh.bottom_field_flag) {
		case 0 : s->repeat_pict = 0; break; // Frame
		case 1 : s->repeat_pict = -1; break; // TopField
		case 2 : s->repeat_pict = -2; break; // BottomField
		default: s->repeat_pict = 0;
	} // switch
#if 0
	if(h->is_avc) {
		put_buffer(&h->out, start_code, sizeof(start_code));
		uint8_t aud[] = {9, 0, 0};
		if(h->slice_types & (1<<1)) // Has B-Slices
			aud[1] = (h->slice_types & 0x18) ? 7 : 2;
		else if(h->slice_types & (1<<0)) // Has P-Slices
			aud[1] = (h->slice_types & 0x18) ? 6 : 1;
		else if(h->slice_types & (1<<3)) // Has SP-Slices
			aud[1] = (h->slice_types & 0x5) ? 6 : 4;
		else if(h->slice_types & (1<<4)) // Has SI-Slices
			aud[1] = (h->slice_types & 0x4) ? 5 : 3;
aud[1]=2;
		aud[1] = aud[1] << 5;
		aud[1] |= 0x1F;
/* 0 I | 1 I, P | 2 I, P, B | 3 SI | 4 SI, SP | 5 I, SI | 6 I, SI, P, SP | 7 I, SI, P, SP, B | */
/* 0,5 P | 1,6 B | 2,7 I | 3, 8 SP | 4,9 SI */
		put_buffer(&h->out, start_code, sizeof(start_code));
		put_buffer(&h->out, aud, sizeof(aud));
	} // if
#endif
	h->slice_types = 0;
} // got_frame

static int hde_h264_parse_init( AVCodecParserContext *s) {
	H264Context *h = s->priv_data;
	memset(h, 0, sizeof(H264Context));
	return 0;
} // hde_h264_parse_init

static void hde_h264_parse_close( AVCodecParserContext *s) {
	H264Context *h = s->priv_data;
	if(h->in.data) av_free(h->in.data);
	if(h->out.data) av_free(h->out.data);
	memset(h, 0, sizeof(H264Context));
} // hde_h264_parse_close

int AVC1_FOURCC = 'a' | ( 'v'<<8 ) | ( 'c'<<16 ) | ( '1'<<24 );
//int FLV1_FOURCC = 'f' | ( 'l'<<8 ) | ( 'v'<<16 ) | ( '1'<<24 );
static int hde_h264_parse( AVCodecParserContext *s, AVCodecContext *avctx, uint8_t **poutbuf, int *poutbuf_size, const uint8_t *buf, int buf_size ) {
	H264Context *h = s->priv_data;
//printf("%x %x\n",avctx->codec_tag,AVC1_FOURCC);
	if ( !h->nal_length_size && ( avctx->codec_tag == AVC1_FOURCC)) {
		h->is_avc = 1;
		h->nal_length_size = 4;
		if ( avctx->extradata_size >= 7 )
			h->nal_length_size = ( ( * ( ( ( char* ) ( avctx->extradata ) ) +4 ) ) &0x03 ) +1;
	} // if
	if(h->got_frame)
		del_buffer(&h->out, h->out.pos);
	h->got_frame = 0;
	s->pict_type = 0;
	if(h->is_avc)
		add_avc_header(s, avctx);
	put_buffer(&h->in, buf, buf_size);
//printf("buf %x %x %x %x %x %x len %d\n",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf_size);
	int pos, size;
	while(get_nal(s, avctx, &pos, &size)) {
//printf("get nal\n");
		if(parse_nal(h, avctx, &h->in.data[pos], size-pos)) {
//printf("parse nal\n");
			got_frame(s);
			if((s->pict_type==FRAME_I_TYPE) || !h->avc_header_sent) {
				h->avc_header_pos=0; // only include header with i-Frames
				h->avc_header_sent=1;
			} // if
			*poutbuf = &h->out.data[h->avc_header_pos];
			*poutbuf_size = h->out.pos-h->avc_header_pos;
			h->avc_header_pos=0;
			return buf_size;
		} else {
			put_buffer(&h->out, start_code, sizeof(start_code));
			put_buffer(&h->out, &h->in.data[pos], size-pos);
			del_buffer(&h->in, size);
		} // if
	} // while
	return buf_size;
} // hde_h264_parse

static int hde_h264_split( AVCodecContext *avctx, const uint8_t *buf, int buf_size ) {
	printf("hde_h264_split\n");
	return 0;
} // hde_h264_split

AVCodecParser hde_h264_parser = {
	{ CODEC_ID_H264 },
	sizeof ( H264Context ),
	hde_h264_parse_init,
	hde_h264_parse,
	hde_h264_parse_close,
	hde_h264_split,
}; // hde_h264_parser
