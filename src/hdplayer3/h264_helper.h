//#include "go_dvb.h"


struct bits {
	u8 *buf;
	int len;
	int p;
	u8 b;
	u32 lz;
	u32 eof;
	u32 nulls;
};

void bits_init(struct bits *b, u8 *buf, u32 len)
{
	b->buf=buf;
	b->p=0;
	b->b=0;
	b->len=len*8;
	b->eof=0;
	b->nulls=0;
}

u32 bits_get_bit(struct bits *b)
{
	u32 v;

	if (b->p==b->len) {
		b->eof=1;
		return 1;
	}
	if (!(b->p&7)) {
		b->b=b->buf[b->p>>3];
		if (b->b==0x03 && b->nulls==2) {
			b->p+=8;
			if (b->p==b->len) {
				b->eof=1;
				return 1;
			}
			b->b=b->buf[b->p>>3];
		}
		if (!b->b) {
			b->nulls++;
			if (b->nulls>2) {
				b->eof=1;
				return 1;
			}
		} else 
			b->nulls=0;
	}
	b->p++;
	v=b->b>>7;
	b->b<<=1;
	return v;
}

u32 bits_get_expg(struct bits *b)
{
	u32 lz=0, code=0, v;

	while (!bits_get_bit(b)) 
		lz++;
	b->lz=lz;
	while (lz) {
		v=bits_get_bit(b);
		if (b->eof)
			return 0;
		code=(code<<1)|v;
		lz--;
	}
	return code;

}

u32 bits_get_ue_v(struct bits *b)
{
	u32 code=bits_get_expg(b);
	
	return (1<<b->lz)-1+code;
}

s32 bits_get_se_v(struct bits *b)
{
	u32 code=bits_get_expg(b);
	
	code=(1<<b->lz)-1+code;
	if (code&1)
		return  (s32)((code+1)/2);
	else
		return -(s32)((code+1)/2);
}

u32 bits_get_u_n(struct bits *b, u32 n)
{
	u32 v=0;

	while(n) {
		v=(v<<1)|bits_get_bit(b);
		if (b->eof)
			return 0;
		n--;
	}
	return v;
}


#if 0
struct go_h264 {
	struct bits;

	u8 frame_num;
	u8 frame_mbs_only;
	u8 field_pic;
	u8 idr_flag;
	u8 idr_pic_id;
	u8 pic_order_cnt_type;
	u8 pic_order_cnt_lsb;
	u8 pic_order_cnt_bottom;
	
};


static int h264_SPS(struct go_h264 *h)
{
	struct bits *b=&h->b;
	u8 profile_idc, flags;

	profile_idc=bits_get_u_n(b, 8);
	flags=bits_get_u_n(b, 8);
	
}

#endif
