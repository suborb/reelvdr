#include "parser.h"

int hde_ff_mpeg4_find_frame_end(ParseContext *pc, const uint8_t *buf, int buf_size){
	int vop_found, i;
	uint32_t state;

	vop_found= pc->frame_start_found;
	state= pc->state;

	i=0;
	if(!vop_found){
		for(i=0; i<buf_size; i++){
			state= (state<<8) | buf[i];
			if(state == 0x1B6){
				i++;
				vop_found=1;
				break;
			}
		}
	}

	if(vop_found){
		/* EOF considered as end of frame */
		if (buf_size == 0)
			return 0;
		for(; i<buf_size; i++){
			state= (state<<8) | buf[i];
			if((state&0xFFFFFF00) == 0x100){
				pc->frame_start_found=0;
				pc->state=-1;
				return i-3;
			}
		}
	}
	pc->frame_start_found= vop_found;
	pc->state= state;
	return END_NOT_FOUND;
}

static int hde_decode_vop_header(MpegEncContext *s, GetBitContext *gb){
	int time_incr, time_increment;

	s->pict_type = get_bits(gb, 2) + I_TYPE;        /* pict type: I = 0 , P = 1 */
	if(s->pict_type==B_TYPE && s->low_delay && s->vol_control_parameters==0 && !(s->flags & CODEC_FLAG_LOW_DELAY)){
		av_log(s->avctx, AV_LOG_ERROR, "low_delay flag incorrectly, clearing it\n");
		s->low_delay=0;
	}

	s->partitioned_frame= s->data_partitioning && s->pict_type!=B_TYPE;
//	if(s->partitioned_frame)
//		s->decode_mb= mpeg4_decode_partitioned_mb;
//	else
//		s->decode_mb= ff_mpeg4_decode_mb;

	time_incr=0;
	while (get_bits1(gb) != 0)
		time_incr++;

	check_marker(gb, "before time_increment");

	if(s->time_increment_bits==0 || !(show_bits(gb, s->time_increment_bits+1)&1)){
		av_log(s->avctx, AV_LOG_ERROR, "hmm, seems the headers are not complete, trying to guess time_increment_bits\n");

		for(s->time_increment_bits=1 ;s->time_increment_bits<16; s->time_increment_bits++){
			if(show_bits(gb, s->time_increment_bits+1)&1) break;
		}

		av_log(s->avctx, AV_LOG_ERROR, "my guess is %d bits ;)\n",s->time_increment_bits);
	}

	if(IS_3IV1) time_increment= get_bits1(gb); //FIXME investigate further
	else time_increment= get_bits(gb, s->time_increment_bits);

//    printf("%d %X\n", s->time_increment_bits, time_increment);
//av_log(s->avctx, AV_LOG_DEBUG, " type:%d modulo_time_base:%d increment:%d t_frame %d\n", s->pict_type, time_incr, time_increment, s->t_frame);
	if(s->pict_type!=B_TYPE){
		s->last_time_base= s->time_base;
		s->time_base+= time_incr;
		s->time= s->time_base*s->avctx->time_base.den + time_increment;
		if(s->workaround_bugs&FF_BUG_UMP4){
			if(s->time < s->last_non_b_time){
//                fprintf(stderr, "header is not mpeg4 compatible, broken encoder, trying to workaround\n");
				s->time_base++;
				s->time+= s->avctx->time_base.den;
			}
		}
		s->pp_time= s->time - s->last_non_b_time;
		s->last_non_b_time= s->time;
	}else{
		s->time= (s->last_time_base + time_incr)*s->avctx->time_base.den + time_increment;
		s->pb_time= s->pp_time - (s->last_non_b_time - s->time);
		if(s->pp_time <=s->pb_time || s->pp_time <= s->pp_time - s->pb_time || s->pp_time<=0){
//            printf("messed up order, maybe after seeking? skipping current b frame\n");
			return FRAME_SKIPPED;
		}
//DL		ff_mpeg4_init_direct_mv(s);

		if(s->t_frame==0) s->t_frame= s->pb_time;
		if(s->t_frame==0) s->t_frame=1; // 1/0 protection
		s->pp_field_time= (  ROUNDED_DIV(s->last_non_b_time, s->t_frame)
				- ROUNDED_DIV(s->last_non_b_time - s->pp_time, s->t_frame))*2;
		s->pb_field_time= (  ROUNDED_DIV(s->time, s->t_frame)
				- ROUNDED_DIV(s->last_non_b_time - s->pp_time, s->t_frame))*2;
		if(!s->progressive_sequence){
			if(s->pp_field_time <= s->pb_field_time || s->pb_field_time <= 1)
				return FRAME_SKIPPED;
		}
	}
//av_log(s->avctx, AV_LOG_DEBUG, "last nonb %"PRId64" last_base %d time %"PRId64" pp %d pb %d t %d ppf %d pbf %d\n", s->last_non_b_time, s->last_time_base, s->time, s->pp_time, s->pb_time, s->t_frame, s->pp_field_time, s->pb_field_time);

	if(s->avctx->time_base.num)
		s->current_picture_ptr->pts= (s->time + s->avctx->time_base.num/2) / s->avctx->time_base.num;
	else
		s->current_picture_ptr->pts= AV_NOPTS_VALUE;
	if(s->avctx->debug&FF_DEBUG_PTS)
		av_log(s->avctx, AV_LOG_DEBUG, "MPEG4 PTS: %"PRId64"\n", s->current_picture_ptr->pts);

	check_marker(gb, "before vop_coded");

	/* vop coded */
	if (get_bits1(gb) != 1){
		if(s->avctx->debug&FF_DEBUG_PICT_INFO)
			av_log(s->avctx, AV_LOG_ERROR, "vop not coded\n");
		return FRAME_SKIPPED;
	}
//printf("time %d %d %d || %"PRId64" %"PRId64" %"PRId64"\n", s->time_increment_bits, s->avctx->time_base.den, s->time_base,
//s->time, s->last_non_b_time, s->last_non_b_time - s->pp_time);
	if (s->shape != BIN_ONLY_SHAPE && ( s->pict_type == P_TYPE
		   || (s->pict_type == S_TYPE && s->vol_sprite_usage==GMC_SPRITE))) {
		/* rounding type for motion estimation */
		s->no_rounding = get_bits1(gb);
		   } else {
			   s->no_rounding = 0;
		   }
//FIXME reduced res stuff

		   if (s->shape != RECT_SHAPE) {
			   if (s->vol_sprite_usage != 1 || s->pict_type != I_TYPE) {
				   int width, height, hor_spat_ref, ver_spat_ref;

				   width = get_bits(gb, 13);
				   skip_bits1(gb);   /* marker */
				   height = get_bits(gb, 13);
				   skip_bits1(gb);   /* marker */
				   hor_spat_ref = get_bits(gb, 13); /* hor_spat_ref */
				   skip_bits1(gb);   /* marker */
				   ver_spat_ref = get_bits(gb, 13); /* ver_spat_ref */
			   }
			   skip_bits1(gb); /* change_CR_disable */

			   if (get_bits1(gb) != 0) {
				   skip_bits(gb, 8); /* constant_alpha_value */
			   }
		   }
//FIXME complexity estimation stuff

		   if (s->shape != BIN_ONLY_SHAPE) {
			   s->intra_dc_threshold= mpeg4_dc_threshold[ get_bits(gb, 3) ];
			   if(!s->progressive_sequence){
				   s->top_field_first= get_bits1(gb);
				   s->alternate_scan= get_bits1(gb);
			   }else
				   s->alternate_scan= 0;
		   }
/*DL
		   if(s->alternate_scan){
		   ff_init_scantable(s->dsp.idct_permutation, &s->inter_scantable  , ff_alternate_vertical_scan);
		   ff_init_scantable(s->dsp.idct_permutation, &s->intra_scantable  , ff_alternate_vertical_scan);
		   ff_init_scantable(s->dsp.idct_permutation, &s->intra_h_scantable, ff_alternate_vertical_scan);
		   ff_init_scantable(s->dsp.idct_permutation, &s->intra_v_scantable, ff_alternate_vertical_scan);
} else{
		   ff_init_scantable(s->dsp.idct_permutation, &s->inter_scantable  , ff_zigzag_direct);
		   ff_init_scantable(s->dsp.idct_permutation, &s->intra_scantable  , ff_zigzag_direct);
		   ff_init_scantable(s->dsp.idct_permutation, &s->intra_h_scantable, ff_alternate_horizontal_scan);
		   ff_init_scantable(s->dsp.idct_permutation, &s->intra_v_scantable, ff_alternate_vertical_scan);
}
		   DL*/
		   if(s->pict_type == S_TYPE && (s->vol_sprite_usage==STATIC_SPRITE || s->vol_sprite_usage==GMC_SPRITE)){
//DL			   mpeg4_decode_sprite_trajectory(s, gb);
			   if(s->sprite_brightness_change) av_log(s->avctx, AV_LOG_ERROR, "sprite_brightness_change not supported\n");
			   if(s->vol_sprite_usage==STATIC_SPRITE) av_log(s->avctx, AV_LOG_ERROR, "static sprite not supported\n");
		   }

		   if (s->shape != BIN_ONLY_SHAPE) {
			   s->chroma_qscale= s->qscale = get_bits(gb, s->quant_precision);
			   if(s->qscale==0){
				   av_log(s->avctx, AV_LOG_ERROR, "Error, header damaged or not MPEG4 header (qscale=0)\n");
				   return -1; // makes no sense to continue, as there is nothing left from the image then
			   }

			   if (s->pict_type != I_TYPE) {
				   s->f_code = get_bits(gb, 3);       /* fcode_for */
				   if(s->f_code==0){
					   av_log(s->avctx, AV_LOG_ERROR, "Error, header damaged or not MPEG4 header (f_code=0)\n");
					   return -1; // makes no sense to continue, as the MV decoding will break very quickly
				   }
			   }else
				   s->f_code=1;

				   if (s->pict_type == B_TYPE) {
					   s->b_code = get_bits(gb, 3);
				   }else
					   s->b_code=1;

					   if(s->avctx->debug&FF_DEBUG_PICT_INFO){
						   av_log(s->avctx, AV_LOG_DEBUG, "qp:%d fc:%d,%d %s size:%d pro:%d alt:%d top:%d %spel part:%d resync:%d w:%d a:%d rnd:%d vot:%d%s dc:%d\n",
								  s->qscale, s->f_code, s->b_code,
		  s->pict_type == I_TYPE ? "I" : (s->pict_type == P_TYPE ? "P" : (s->pict_type == B_TYPE ? "B" : "S")),
										  gb->size_in_bits,s->progressive_sequence, s->alternate_scan, s->top_field_first,
			s->quarter_sample ? "q" : "h", s->data_partitioning, s->resync_marker, s->num_sprite_warping_points,
   s->sprite_warping_accuracy, 1-s->no_rounding, s->vo_type, s->vol_control_parameters ? " VOLC" : " ", s->intra_dc_threshold);
					   }

					   if(!s->scalability){
						   if (s->shape!=RECT_SHAPE && s->pict_type!=I_TYPE) {
							   skip_bits1(gb); // vop shape coding type
						   }
					   }else{
						   if(s->enhancement_type){
							   int load_backward_shape= get_bits1(gb);
							   if(load_backward_shape){
								   av_log(s->avctx, AV_LOG_ERROR, "load backward shape isn't supported\n");
							   }
						   }
						   skip_bits(gb, 2); //ref_select_code
					   }
		   }
		   /* detect buggy encoders which don't set the low_delay flag (divx4/xvid/opendivx)*/
     // note we cannot detect divx5 without b-frames easily (although it's buggy too)
		   if(s->vo_type==0 && s->vol_control_parameters==0 && s->divx_version==0 && s->picture_number==0){
			   av_log(s->avctx, AV_LOG_ERROR, "looks like this file was encoded with (divx4/(old)xvid/opendivx) -> forcing low_delay flag\n");
			   s->low_delay=1;
		   }

		   s->picture_number++; // better than pic number==0 always ;)

//DL		   s->y_dc_scale_table= ff_mpeg4_y_dc_scale_table; //FIXME add short header support
//DL		   s->c_dc_scale_table= ff_mpeg4_c_dc_scale_table;

		   if(s->workaround_bugs&FF_BUG_EDGE){
			   s->h_edge_pos= s->width;
			   s->v_edge_pos= s->height;
		   }
		   return 0;
}

static int hde_decode_vol_header(MpegEncContext *s, GetBitContext *gb){
	int width, height, vo_ver_id;

	/* vol header */
	skip_bits(gb, 1); /* random access */
	s->vo_type= get_bits(gb, 8);
	if (get_bits1(gb) != 0) { /* is_ol_id */
		vo_ver_id = get_bits(gb, 4); /* vo_ver_id */
		skip_bits(gb, 3); /* vo_priority */
	} else {
		vo_ver_id = 1;
	}
//printf("vo type:%d\n",s->vo_type);
	s->aspect_ratio_info= get_bits(gb, 4);
	if(s->aspect_ratio_info == FF_ASPECT_EXTENDED){
		s->avctx->sample_aspect_ratio.num= get_bits(gb, 8); // par_width
		s->avctx->sample_aspect_ratio.den= get_bits(gb, 8); // par_height
	}else{
		s->avctx->sample_aspect_ratio= pixel_aspect[s->aspect_ratio_info];
	}

	if ((s->vol_control_parameters=get_bits1(gb))) { /* vol control parameter */
		int chroma_format= get_bits(gb, 2);
		if(chroma_format!=1){
			av_log(s->avctx, AV_LOG_ERROR, "illegal chroma format\n");
		}
		s->low_delay= get_bits1(gb);
		if(get_bits1(gb)){ /* vbv parameters */
			get_bits(gb, 15);   /* first_half_bitrate */
			skip_bits1(gb);     /* marker */
			get_bits(gb, 15);   /* latter_half_bitrate */
			skip_bits1(gb);     /* marker */
			get_bits(gb, 15);   /* first_half_vbv_buffer_size */
			skip_bits1(gb);     /* marker */
			get_bits(gb, 3);    /* latter_half_vbv_buffer_size */
			get_bits(gb, 11);   /* first_half_vbv_occupancy */
			skip_bits1(gb);     /* marker */
			get_bits(gb, 15);   /* latter_half_vbv_occupancy */
			skip_bits1(gb);     /* marker */
		}
	}else{
        // set low delay flag only once the smartest? low delay detection won't be overriden
		if(s->picture_number==0)
			s->low_delay=0;
	}

	s->shape = get_bits(gb, 2); /* vol shape */
	if(s->shape != RECT_SHAPE) av_log(s->avctx, AV_LOG_ERROR, "only rectangular vol supported\n");
	if(s->shape == GRAY_SHAPE && vo_ver_id != 1){
		av_log(s->avctx, AV_LOG_ERROR, "Gray shape not supported\n");
		skip_bits(gb, 4);  //video_object_layer_shape_extension
	}

	check_marker(gb, "before time_increment_resolution");

	s->avctx->time_base.den = get_bits(gb, 16);
	if(!s->avctx->time_base.den){
		av_log(s->avctx, AV_LOG_ERROR, "time_base.den==0\n");
		return -1;
	}

	s->time_increment_bits = av_log2(s->avctx->time_base.den - 1) + 1;
	if (s->time_increment_bits < 1)
		s->time_increment_bits = 1;

	check_marker(gb, "before fixed_vop_rate");

	if (get_bits1(gb) != 0) {   /* fixed_vop_rate  */
		s->avctx->time_base.num = get_bits(gb, s->time_increment_bits);
	}else
		s->avctx->time_base.num = 1;

		s->t_frame=0;

		if (s->shape != BIN_ONLY_SHAPE) {
			if (s->shape == RECT_SHAPE) {
				skip_bits1(gb);   /* marker */
				width = get_bits(gb, 13);
				skip_bits1(gb);   /* marker */
				height = get_bits(gb, 13);
				skip_bits1(gb);   /* marker */
				if(width && height && !(s->width && s->codec_tag == ff_get_fourcc("MP4S"))){ /* they should be non zero but who knows ... */
					s->width = width;
					s->height = height;
//                printf("width/height: %d %d\n", width, height);
				}
			}

			s->progressive_sequence=
					s->progressive_frame= get_bits1(gb)^1;
			s->interlaced_dct=0;
			if(!get_bits1(gb) && (s->avctx->debug & FF_DEBUG_PICT_INFO))
				av_log(s->avctx, AV_LOG_INFO, "MPEG4 OBMC not supported (very likely buggy encoder)\n");   /* OBMC Disable */
			if (vo_ver_id == 1) {
				s->vol_sprite_usage = get_bits1(gb); /* vol_sprite_usage */
			} else {
				s->vol_sprite_usage = get_bits(gb, 2); /* vol_sprite_usage */
			}
			if(s->vol_sprite_usage==STATIC_SPRITE) av_log(s->avctx, AV_LOG_ERROR, "Static Sprites not supported\n");
			if(s->vol_sprite_usage==STATIC_SPRITE || s->vol_sprite_usage==GMC_SPRITE){
				if(s->vol_sprite_usage==STATIC_SPRITE){
					s->sprite_width = get_bits(gb, 13);
					skip_bits1(gb); /* marker */
					s->sprite_height= get_bits(gb, 13);
					skip_bits1(gb); /* marker */
					s->sprite_left  = get_bits(gb, 13);
					skip_bits1(gb); /* marker */
					s->sprite_top   = get_bits(gb, 13);
					skip_bits1(gb); /* marker */
				}
				s->num_sprite_warping_points= get_bits(gb, 6);
				s->sprite_warping_accuracy = get_bits(gb, 2);
				s->sprite_brightness_change= get_bits1(gb);
				if(s->vol_sprite_usage==STATIC_SPRITE)
					s->low_latency_sprite= get_bits1(gb);
			}
        // FIXME sadct disable bit if verid!=1 && shape not rect

			if (get_bits1(gb) == 1) {   /* not_8_bit */
				s->quant_precision = get_bits(gb, 4); /* quant_precision */
				if(get_bits(gb, 4)!=8) av_log(s->avctx, AV_LOG_ERROR, "N-bit not supported\n"); /* bits_per_pixel */
				if(s->quant_precision!=5) av_log(s->avctx, AV_LOG_ERROR, "quant precision %d\n", s->quant_precision);
			} else {
				s->quant_precision = 5;
			}

        // FIXME a bunch of grayscale shape things

			if((s->mpeg_quant=get_bits1(gb))){ /* vol_quant_type */
				int i, v;
				/* load default matrixes */
/*DL
				for(i=0; i<64; i++){
				int j= s->dsp.idct_permutation[i];
				v= ff_mpeg4_default_intra_matrix[i];
				s->intra_matrix[j]= v;
				s->chroma_intra_matrix[j]= v;

				v= ff_mpeg4_default_non_intra_matrix[i];
				s->inter_matrix[j]= v;
				s->chroma_inter_matrix[j]= v;
			}
				DL*/
				/* load custom intra matrix */
				if(get_bits1(gb)){
					int last=0;
					for(i=0; i<64; i++){
//DL						int j;
						v= get_bits(gb, 8);
						if(v==0) break;

						last= v;
/*DL
						j= s->dsp.idct_permutation[ ff_zigzag_direct[i] ];
						s->intra_matrix[j]= v;
						s->chroma_intra_matrix[j]= v;
						DL*/
					}
					/* replicate last value */
/*DL
					for(; i<64; i++){
					int j= s->dsp.idct_permutation[ ff_zigzag_direct[i] ];
					s->intra_matrix[j]= last;
					s->chroma_intra_matrix[j]= last;
				}
					DL*/
				}

				/* load custom non intra matrix */
				if(get_bits1(gb)){
					int last=0;
					for(i=0; i<64; i++){
//DL						int j;
						v= get_bits(gb, 8);
						if(v==0) break;

						last= v;
/*DL
						j= s->dsp.idct_permutation[ ff_zigzag_direct[i] ];
						s->inter_matrix[j]= v;
						s->chroma_inter_matrix[j]= v;
						DL*/
					}

					/* replicate last value */
/*DL
					for(; i<64; i++){
					int j= s->dsp.idct_permutation[ ff_zigzag_direct[i] ];
					s->inter_matrix[j]= last;
					s->chroma_inter_matrix[j]= last;
				}
					DL*/
				}

            // FIXME a bunch of grayscale shape things
			}

			if(vo_ver_id != 1)
				s->quarter_sample= get_bits1(gb);
			else s->quarter_sample=0;

			if(!get_bits1(gb)) av_log(s->avctx, AV_LOG_ERROR, "Complexity estimation not supported\n");

			s->resync_marker= !get_bits1(gb); /* resync_marker_disabled */

			s->data_partitioning= get_bits1(gb);
			if(s->data_partitioning){
				s->rvlc= get_bits1(gb);
			}

			if(vo_ver_id != 1) {
				s->new_pred= get_bits1(gb);
				if(s->new_pred){
					av_log(s->avctx, AV_LOG_ERROR, "new pred not supported\n");
					skip_bits(gb, 2); /* requested upstream message type */
					skip_bits1(gb); /* newpred segment type */
				}
				s->reduced_res_vop= get_bits1(gb);
				if(s->reduced_res_vop) av_log(s->avctx, AV_LOG_ERROR, "reduced resolution VOP not supported\n");
			}
			else{
				s->new_pred=0;
				s->reduced_res_vop= 0;
			}

			s->scalability= get_bits1(gb);

			if (s->scalability) {
				GetBitContext bak= *gb;
				int ref_layer_id;
				int ref_layer_sampling_dir;
				int h_sampling_factor_n;
				int h_sampling_factor_m;
				int v_sampling_factor_n;
				int v_sampling_factor_m;

				s->hierachy_type= get_bits1(gb);
				ref_layer_id= get_bits(gb, 4);
				ref_layer_sampling_dir= get_bits1(gb);
				h_sampling_factor_n= get_bits(gb, 5);
				h_sampling_factor_m= get_bits(gb, 5);
				v_sampling_factor_n= get_bits(gb, 5);
				v_sampling_factor_m= get_bits(gb, 5);
				s->enhancement_type= get_bits1(gb);

				if(   h_sampling_factor_n==0 || h_sampling_factor_m==0
								  || v_sampling_factor_n==0 || v_sampling_factor_m==0){

//                fprintf(stderr, "illegal scalability header (VERY broken encoder), trying to workaround\n");
					s->scalability=0;

					*gb= bak;
								  }else
									  av_log(s->avctx, AV_LOG_ERROR, "scalability not supported\n");

            // bin shape stuff FIXME
			}
		}
		return 0;
}

static int hde_decode_user_data(MpegEncContext *s, GetBitContext *gb){
	char buf[256];
	int i;
	int e;
	int ver = 0, build = 0, ver2 = 0, ver3 = 0;
	char last;

	for(i=0; i<255 && get_bits_count(gb) < gb->size_in_bits; i++){
		if(show_bits(gb, 23) == 0) break;
		buf[i]= get_bits(gb, 8);
	}
	buf[i]=0;

	/* divx detection */
	e=sscanf(buf, "DivX%dBuild%d%c", &ver, &build, &last);
	if(e<2)
		e=sscanf(buf, "DivX%db%d%c", &ver, &build, &last);
	if(e>=2){
		s->divx_version= ver;
		s->divx_build= build;
		s->divx_packed= e==3 && last=='p';
	}

	/* ffmpeg detection */
	e=sscanf(buf, "FFmpe%*[^b]b%d", &build)+3;
	if(e!=4)
		e=sscanf(buf, "FFmpeg v%d.%d.%d / libavcodec build: %d", &ver, &ver2, &ver3, &build);
	if(e!=4){
		e=sscanf(buf, "Lavc%d.%d.%d", &ver, &ver2, &ver3)+1;
		if (e>1)
			build= (ver<<16) + (ver2<<8) + ver3;
	}
	if(e!=4){
		if(strcmp(buf, "ffmpeg")==0){
			s->lavc_build= 4600;
		}
	}
	if(e==4){
		s->lavc_build= build;
	}

	/* xvid detection */
	e=sscanf(buf, "XviD%d", &build);
	if(e==1){
		s->xvid_build= build;
	}

//printf("User Data: %s\n", buf);
	return 0;
}

static int hde_mpeg4_decode_gop_header(MpegEncContext * s, GetBitContext *gb){
	int hours, minutes, seconds;

	hours= get_bits(gb, 5);
	minutes= get_bits(gb, 6);
	skip_bits1(gb);
	seconds= get_bits(gb, 6);

	s->time_base= seconds + 60*(minutes + 60*hours);

	skip_bits1(gb);
	skip_bits1(gb);

	return 0;
}

int hde_ff_mpeg4_decode_picture_header(MpegEncContext * s, GetBitContext *gb)
{
	int startcode, v;

	/* search next start code */
	align_get_bits(gb);

	if(s->codec_tag == ff_get_fourcc("WV1F") && show_bits(gb, 24) == 0x575630){
		skip_bits(gb, 24);
		if(get_bits(gb, 8) == 0xF0)
			return hde_decode_vop_header(s, gb);
	}

	startcode = 0xff;
	for(;;) {
		if(get_bits_count(gb) >= gb->size_in_bits){
			if(gb->size_in_bits==8 && (s->divx_version || s->xvid_build)){
//				av_log(s->avctx, AV_LOG_ERROR, "frame skip %d\n", gb->size_in_bits);
				return FRAME_SKIPPED; //divx bug
			}else
				return -1; //end of stream
		}

		/* use the bits after the test */
		v = get_bits(gb, 8);
		startcode = ((startcode << 8) | v) & 0xffffffff;

		if((startcode&0xFFFFFF00) != 0x100)
			continue; //no startcode

		if(s->avctx->debug&FF_DEBUG_STARTCODE){
			av_log(s->avctx, AV_LOG_DEBUG, "startcode: %3X ", startcode);
			if     (startcode<=0x11F) av_log(s->avctx, AV_LOG_DEBUG, "Video Object Start");
			else if(startcode<=0x12F) av_log(s->avctx, AV_LOG_DEBUG, "Video Object Layer Start");
			else if(startcode<=0x13F) av_log(s->avctx, AV_LOG_DEBUG, "Reserved");
			else if(startcode<=0x15F) av_log(s->avctx, AV_LOG_DEBUG, "FGS bp start");
			else if(startcode<=0x1AF) av_log(s->avctx, AV_LOG_DEBUG, "Reserved");
			else if(startcode==0x1B0) av_log(s->avctx, AV_LOG_DEBUG, "Visual Object Seq Start");
			else if(startcode==0x1B1) av_log(s->avctx, AV_LOG_DEBUG, "Visual Object Seq End");
			else if(startcode==0x1B2) av_log(s->avctx, AV_LOG_DEBUG, "User Data");
			else if(startcode==0x1B3) av_log(s->avctx, AV_LOG_DEBUG, "Group of VOP start");
			else if(startcode==0x1B4) av_log(s->avctx, AV_LOG_DEBUG, "Video Session Error");
			else if(startcode==0x1B5) av_log(s->avctx, AV_LOG_DEBUG, "Visual Object Start");
			else if(startcode==0x1B6) av_log(s->avctx, AV_LOG_DEBUG, "Video Object Plane start");
			else if(startcode==0x1B7) av_log(s->avctx, AV_LOG_DEBUG, "slice start");
			else if(startcode==0x1B8) av_log(s->avctx, AV_LOG_DEBUG, "extension start");
			else if(startcode==0x1B9) av_log(s->avctx, AV_LOG_DEBUG, "fgs start");
			else if(startcode==0x1BA) av_log(s->avctx, AV_LOG_DEBUG, "FBA Object start");
			else if(startcode==0x1BB) av_log(s->avctx, AV_LOG_DEBUG, "FBA Object Plane start");
			else if(startcode==0x1BC) av_log(s->avctx, AV_LOG_DEBUG, "Mesh Object start");
			else if(startcode==0x1BD) av_log(s->avctx, AV_LOG_DEBUG, "Mesh Object Plane start");
			else if(startcode==0x1BE) av_log(s->avctx, AV_LOG_DEBUG, "Still Texture Object start");
			else if(startcode==0x1BF) av_log(s->avctx, AV_LOG_DEBUG, "Texture Spatial Layer start");
			else if(startcode==0x1C0) av_log(s->avctx, AV_LOG_DEBUG, "Texture SNR Layer start");
			else if(startcode==0x1C1) av_log(s->avctx, AV_LOG_DEBUG, "Texture Tile start");
			else if(startcode==0x1C2) av_log(s->avctx, AV_LOG_DEBUG, "Texture Shape Layer start");
			else if(startcode==0x1C3) av_log(s->avctx, AV_LOG_DEBUG, "stuffing start");
			else if(startcode<=0x1C5) av_log(s->avctx, AV_LOG_DEBUG, "reserved");
			else if(startcode<=0x1FF) av_log(s->avctx, AV_LOG_DEBUG, "System start");
			av_log(s->avctx, AV_LOG_DEBUG, " at %d\n", get_bits_count(gb));
		}

		if(startcode >= 0x120 && startcode <= 0x12F){
			if(hde_decode_vol_header(s, gb) < 0)
				return -1;
		}
		else if(startcode == USER_DATA_STARTCODE){
			hde_decode_user_data(s, gb);
		}
		else if(startcode == GOP_STARTCODE){
			hde_mpeg4_decode_gop_header(s, gb);
		}
		else if(startcode == VOP_STARTCODE){
			return hde_decode_vop_header(s, gb);
		}

		align_get_bits(gb);
		startcode = 0xff;
	}
}

#ifdef CONFIG_MPEG4VIDEO_PARSER
/* used by parser */
/* XXX: make it use less memory */
static int hde_av_mpeg4_decode_header(AVCodecParserContext *s1,
								  AVCodecContext *avctx,
		  const uint8_t *buf, int buf_size)
{
	ParseContext1 *pc = s1->priv_data;
	MpegEncContext *s = pc->enc;
	GetBitContext gb1, *gb = &gb1;
	int ret;

	s->avctx = avctx;
	s->current_picture_ptr = &s->current_picture;
	
	if (avctx->extradata_size && pc->first_picture){
		init_get_bits(gb, avctx->extradata, avctx->extradata_size*8);
		ret = hde_ff_mpeg4_decode_picture_header(s, gb);
	}

	init_get_bits(gb, buf, 8 * buf_size);
	ret = hde_ff_mpeg4_decode_picture_header(s, gb);
	if (s->width) {
		myavcodec_set_dimensions(avctx, s->width, s->height);
	}
	s1->pict_type= s->pict_type;
	pc->first_picture = 0;
	return ret;
}

static int hde_mpeg4video_parse_init(AVCodecParserContext *s)
{
	ParseContext1 *pc = s->priv_data;

	pc->enc = av_mallocz(sizeof(MpegEncContext));
	if (!pc->enc)
		return -1;
	pc->first_picture = 1;
	return 0;
}

static int hde_mpeg4video_parse(AVCodecParserContext *s,
							AVCodecContext *avctx,
							uint8_t **poutbuf, int *poutbuf_size,
							const uint8_t *buf, int buf_size)
{
	ParseContext *pc = s->priv_data;
	int next;

	if(s->flags & PARSER_FLAG_COMPLETE_FRAMES){
		next= buf_size;
	}else{
		next= hde_ff_mpeg4_find_frame_end(pc, buf, buf_size);

		if (hde_ff_combine_frame(pc, next, (uint8_t **)&buf, &buf_size) < 0) {
			*poutbuf = NULL;
			*poutbuf_size = 0;
			return buf_size;
		}
	}
	hde_av_mpeg4_decode_header(s, avctx, buf, buf_size);

	*poutbuf = (uint8_t *)buf;
	*poutbuf_size = buf_size;
	return next;
}
#endif

static int hde_ff_mpeg4video_split(AVCodecContext *avctx,
						const uint8_t *buf, int buf_size)
{
	int i;
	uint32_t state= -1;

	for(i=0; i<buf_size; i++){
		state= (state<<8) | buf[i];
		if(state == 0x1B3 || state == 0x1B6)
			return i-3;
	}
	return 0;
}

#ifdef CONFIG_MPEG4VIDEO_PARSER
AVCodecParser hde_mpeg4video_parser = {
	{ CODEC_ID_MPEG4 },
	sizeof(ParseContext1),
	hde_mpeg4video_parse_init,
	hde_mpeg4video_parse,
	hde_ff_parse1_close,
	hde_ff_mpeg4video_split,
};
#endif
