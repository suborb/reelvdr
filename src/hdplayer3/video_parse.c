/*****************************************************************************
*
* Copyright (C) 2008 Georg Acher (acher (at) baycom dot de)
* Copyright (C) Reel Multimedia
*
* #include <gpl_v2.h>
*
******************************************************************************/
#include <stdlib.h>
#include "types.h"
#include "video_parse.h"
#include <syslog.h>
/*-------------------------------------------------------------------------*/
inline u8 *find_next_start_code(u8 *d, u8 *e)
{
	for ( ; d != e; ++d)
	{
		if (d[0] == 0 && d[1] == 0 && d[2] == 1)
		{
			return d;
		}
	}
	return NULL;
}
/*-------------------------------------------------------------------------*/
u8 *parse_user_data(u8 *d, u8 *e)
{

	return find_next_start_code(d + 4, e);
}
/*-------------------------------------------------------------------------*/
u8 *parse_group_start(video_attr_t *vat, u8 *d, u8 *e)
{
	return find_next_start_code(d + 4, e);
}
/*-------------------------------------------------------------------------*/
u8 *parse_seq_ext(u8 *d, u8 *e)
{
//	printf("%02x %02x %02x %02x\n",*(d+3),*(d+4),*(d+5),*(d+6));
	return find_next_start_code(d + 4, e);
}
/*-------------------------------------------------------------------------*/
u8 *parse_seq_header(video_attr_t *vat, u8 *d, u8 *e)
{
	int aspect_info;

	vat->width = (d[4] << 4) | (d[5] >> 4);
	vat->height = ((d[5] << 8) & 0xF00) + d[6];
	aspect_info = d[7] >> 4;

	switch(aspect_info)
	{
	case 3:
		vat->aspect_w = 16;
		vat->aspect_h = 9;
		break;
	case 4:
		vat->aspect_w = 221;  // 2.21 : 1
		vat->aspect_h = 100;
		break;
	default:
		vat->aspect_w = 4;
		vat->aspect_h = 3;
	}

	switch(d[7]&0xf)
	{
	case 1:
		vat->framerate_nom=1001; // 23.976
		vat->framerate_div=24000;
		break;
	case 2:
		vat->framerate_nom=3750;  // 24
		vat->framerate_div=90000; 
		break;
	case 4:
		vat->framerate_nom=1001;  // 29.97
		vat->framerate_div=30000; 
		break;
	case 5:
		vat->framerate_nom=3000; // 30
		vat->framerate_div=90000; 
		break;
	case 6:
		vat->framerate_nom=1800;  // 50
		vat->framerate_div=90000; 
		break;
	case 7:
		vat->framerate_nom=1001;  // 59.94
		vat->framerate_div=60000; 
		break;
	case 8:
		vat->framerate_nom=1500;  // 60
		vat->framerate_div=90000; 	    
		break;
	default:
		vat->framerate_nom=3600; // 25
		vat->framerate_div=90000; 
	}
	// find here also: frame_rate_code
	return find_next_start_code(d + 12, e);
}
/*-------------------------------------------------------------------------*/
int video_parse(video_attr_t *vat, u8 *data, int length)
{
	u8 *end;
	if (length < 4 || data[0] != 0 || data[1] != 0 || data[2] != 1
	    || (data[3] != 0 && data[3] != 9 && data[3] < 0xB0))
	{
		return video_h264_parse(vat, data, length);
	}

	end =  data + (length - 4);

	while (data + 64 <= end)
	{
		switch (data[3])
		{
		case 0:
		{
			char type;
			switch ((data[5] >> 3) & 7)
			{
			case 1:
				type = 'I';
				break;
			case 2:
				type = 'P';
				break;
			case 3:
				type = 'B';
				break;
			default:
				type = 'U';
			}
			vat->pic_type = type;
//                vat->temp_ref = pl->gop_temp_ref + (data[4] << 2) + (data[5] >> 6);
		}
		return 1;
		case 9:
		{
			int prim_pic_type = data[4] >> 5;
			char type = 'U';
			switch (prim_pic_type) {
			case 0:
			case 3:
			case 5:
				type = 'I';
				break;
			case 1:
			case 4:
			case 6:
				type = 'P';
				break;
			case 2:
			case 7:
				type = 'B';
				break;
			}
			vat->pic_type = type;
		}
		return 1;

		case 0xB3:
			data = parse_seq_header(vat, data, end);
			break;
		case 0xB5:
			data = parse_seq_ext(data, end);
			break;
		case 0xB8:
			data = parse_group_start(vat, data, end);
			break;
		case 0xB2:
			data = parse_user_data(data, end);
			break;
		default:
			return 0;
		}

		if (!data)
		{
			return 0;
		}
	}
	return 1;
}

/* ==============  h.264 parse block, used ffmpeg code ======================*/

int offset=0;


static const u16 h264_aspect[17][2] = {
 {0, 1},
 {1, 1},
 {12, 11},
 {10, 11},
 {16, 11},
 {40, 33},
 {24, 11},
 {20, 11},
 {32, 11},
 {80, 33},
 {18, 11},
 {15, 11},
 {64, 33},
 {160,99},
 {4, 3},
 {3, 2},
 {2, 1},
};


unsigned int
read_bits(u8 *data, int num)
{
//int n=num;
  int r = 0;
  while(num > 0) {
        num--;
        if(data[offset / 8] & (1 << (7 - (offset & 7))))
            r |= 1 << num;
            offset++;
        }
//syslog(LOG_WARNING,"bits %d val %d off %d \n",n,r,offset);
    return r;
}

u32 read_bits_long(u8 *data, int n)
{
    int ret = read_bits(data, 16) << (n-16);
    return ret | read_bits(data, n-16);
}

unsigned int
read_bits1(u8 *data)
{
  return read_bits(data, 1);
}

unsigned int
read_golomb_ue(u8 *data)
{
  int b, lzb = -1;
    for(b = 0; !b; lzb++)
        b = read_bits1(data);
  return (1 << lzb) - 1 + read_bits(data, lzb);
}

signed int
read_golomb_se(u8 *data)
{
  int v, pos;
    v = read_golomb_ue(data);
    if(v == 0)
        return 0;
    pos = v & 1;
    v = (v + 1) >> 1;
    return pos ? v : -v;
}


static const int h264_nal_deescape(u8 *d, const u8 *data, int size)
{
  int rbsp_size, i;
  /* Escape 0x000003 into 0x0000 */
  rbsp_size = 0;
  for(i = 1; i < size; i++) {
        if(i + 2 < size && data[i] == 0 && data[i + 1] == 0 && data[i + 2] == 3) {
            d[rbsp_size++] = 0;
            d[rbsp_size++] = 0;
            i += 2;
        } else {
            d[rbsp_size++] = data[i];
        }
    }
int len = rbsp_size * 8;//len in bits
   return len;
}

static void decode_scaling_list(u8 *data, int size)
{
  int i, last = 8, next = 8;
    if(!read_bits1(data))
        return; /* matrix not written */

          for(i=0;i<size;i++){
              if(next)
                    next = (last + read_golomb_se(data)) & 0xff;
                    if(!i && !next)
                        break;
          last = next ? next : last;
    }
}

int video_h264_parse(video_attr_t *vat, u8 *data, int length)
{

    if (length < 40 || data[0] != 0 || data[1] != 0 || data[2] != 0 || data[3] != 1 || (data[4] != 0x9 && data[4] != 0x67)) return 0;

//if (length > 15)
//syslog(LOG_WARNING,"header %d %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x \n",length,data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14]);
//if (offset >length)return 0;

//syslog(LOG_WARNING,"data[5] %x\n",data[5]);
//        if (data[5] == 0x10){
//                switch (data[10] & 0x1f){
//                       case 7:
//                       {
if ((data[10]&0x1f) == 7){
//    syslog(LOG_WARNING,"header %d %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x \n",length,data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14]);
//u8 *nal = (u8*)malloc(sizeof((length-11)*sizeof(int)));
//memcpy(nal,data+11,length-11);
//int profile_idc = data[11];
//int level_idc = data[13];
//u8 *nal;
//int len;
//u8 *f = h264_nal_deescape(&nal, &len,data + 11 + 3, length - 11 - 3);
//free(f);
//u8 *nal = malloc(length);
    offset = 0;

    u8 *nal=malloc(40);
//    syslog(LOG_WARNING,"nal1 %x %x %x %x %x\n",nal[0],nal[1],nal[2],nal[3],nal[4]);
//  u8 *nal = malloc(length);
//u8 *nal=NULL;
//length-=11;
    int len = h264_nal_deescape(nal, data+10, 40);
//    syslog(LOG_WARNING,"nal2 %x %x %x %x %x\n",nal[0],nal[1],nal[2],nal[3],nal[4]);
//syslog(LOG_WARNING,"length %d len %d l %d \n",length,len,l);

    unsigned int sps_id, tmp, i, width, height;


    int profile_idc= read_bits(nal, 8);
    read_bits1(nal);   //constraint_set0_flag
    read_bits1(nal);   //constraint_set1_flag
    read_bits1(nal);   //constraint_set2_flag
    read_bits1(nal);   //constraint_set3_flag
    read_bits(nal, 4); // reserved
    int level_idc= read_bits(nal, 8);


    sps_id = read_golomb_ue(nal);
    if(sps_id > 255) goto err;
    if(profile_idc >= 100){ //high profile
	if(read_golomb_ue(nal) == 3) //chroma_format_idc
        read_bits1(nal);  //residual_color_transform_flag
        read_golomb_ue(nal);  //bit_depth_luma_minus8
        read_golomb_ue(nal);  //bit_depth_chroma_minus8
        read_bits1(nal);
	if(read_bits1(nal)) {
	/* Scaling matrices */
            decode_scaling_list(nal, 16);
            decode_scaling_list(nal, 16);
            decode_scaling_list(nal, 16);
            decode_scaling_list(nal, 16);
            decode_scaling_list(nal, 16);
            decode_scaling_list(nal, 16);
            decode_scaling_list(nal, 64);
            decode_scaling_list(nal, 64);
       }
    }
    int max_frame_num_bits = read_golomb_ue(nal) + 4;
    int poc_type= read_golomb_ue(nal);
    if(poc_type == 0){
	read_golomb_ue(nal);
    } else if(poc_type == 1){
	read_bits1(nal);
	read_golomb_se(nal);
	read_golomb_se(nal);
	tmp = read_golomb_ue(nal);
	for(i = 0; i < tmp; i++)
            read_golomb_se(nal);
    }else if(poc_type != 2){
        goto err;
    }
    int num_ref_frames = read_golomb_ue(nal);
    read_bits1(nal);
    width = read_golomb_ue(nal) + 1;
    height = read_golomb_ue(nal) + 1;
    width = width * 16;
    int mbs_only_flag = read_bits1(nal);
    height = height * 16 * (2 - mbs_only_flag);
    int aff;
    if(!mbs_only_flag)
	aff = read_bits1(nal);
    read_bits1(nal); //direct 8x8 interference flag
    if(read_bits1(nal)){ //frame cropping flag
	tmp = read_golomb_ue(nal);
	tmp = read_golomb_ue(nal);
	tmp = read_golomb_ue(nal);
	tmp = read_golomb_ue(nal);
    }

    vat->width =width;
    vat->height=height;

    if(read_bits1(nal)) { //vui parameter set
	int aspect_num = 0;
	int aspect_den = 1;

	if(read_bits1(nal)) {//aspect ratio present flag
	    int aspect = read_bits(nal, 8);
	    if(aspect == 255) {
    		u16 num = read_bits(nal, 16);
                u16 den = read_bits(nal, 16);
        	aspect_num = num;
        	aspect_den = den;
    	    } else if(aspect < 17) {
    		aspect_num =  h264_aspect[aspect][0];
    		aspect_den =  h264_aspect[aspect][1];
    	    }
	}
	if (offset >len)goto err;

	vat->aspect_w = aspect_num;
	vat->aspect_h = aspect_den;

	if(read_bits1(nal)){      /* overscan_info_present_flag */
	    read_bits1(nal);      /* overscan_appropriate_flag */
	}
	if(read_bits1(nal)){      /* video_signal_type_present_flag */
	    read_bits(nal, 3);    /* video_format */
	    read_bits1(nal);      /* video_full_range_flag */
	    if(read_bits1(nal)){  /* colour_description_present_flag */
        	read_bits(nal, 8); /* colour_primaries */
    		read_bits(nal, 8); /* transfer_characteristics */
    		read_bits(nal, 8); /* matrix_coefficients */
    	    }
	}
	if(read_bits1(nal)){      /* chroma_location_info_present_flag */
	    read_golomb_ue(nal);  /* chroma_sample_location_type_top_field */
    	    read_golomb_ue(nal);  /* chroma_sample_location_type_bottom_field */
	}

	if(!read_bits1(nal)) /* We need timing info */
	    goto ok;
	unsigned int units_in_tick = read_bits(nal, 32);
	unsigned int time_scale = read_bits(nal, 32);

	int fixed_rate    = read_bits1(nal);

	vat->framerate_nom = units_in_tick * 2;//(1 + fixed_rate);
	vat->framerate_div = time_scale;

	if (offset >len)goto err;

//	syslog(LOG_WARNING,"prof %d spsid %d w %d h %d asd %d asn %d ts %u ut %u off %d len %d -- %d -- %u\n",profile_idc,sps_id,width,height,aspect_num,aspect_den,time_scale,units_in_tick,offset, length,(1 + mbs_only_flag) * 2 * num_ref_frames, 180000. * (double) units_in_tick / (double) time_scale);
//	syslog(LOG_WARNING,"heade2 %d %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x \n",length,data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14]);
    }
    ok:
    free(nal);
    return 1;

err:
free(nal);
//off = 10;
//if (length > 15)
//syslog(LOG_WARNING,"header %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x \n",data[0+off],data[1+off],data[2+off],data[3+off],data[4+off],data[5+off],data[6+off],data[7+off],data[8+off],data[9+off],data[10+off],data[11+off],data[12+off],data[13+off],data[14+off]);
return 0;
}
//                             break;
//                       }
//                       case 5:
//                       case 1:
//                             break;
//                }
//        }


return 2;
}
