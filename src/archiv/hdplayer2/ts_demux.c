/*-------------------------------------------------------------------------*/
/*
  TS demuxer for MPEG and H.264

  (c) 2007 Georg Acher (acher at baycom dot de)

  #include <GPL_v2.h>
*/
/*-------------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ts_demux.h"

//#define DEBUG

#define ADMXBUFLEN (1024*1024)

static void vput(ts_demux_t *td, u08_t const *data, int len, int pes_start);
static void aput(ts_demux_t *td, u08_t const *data, int len, int pes_start);
static void store_vdata(ts_demux_t *td, uint8_t const *x, int len, int flush);

void hexdump(uint8_t const *x, int len)
{
        int n;
        for(n=0;n<len;n+=16) {
                printf("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                       x[0],x[1],x[2],x[3],x[4],x[5],x[6],x[7],
		       x[8],x[9],x[10],x[11],x[12],x[13],x[14],x[15]
			);
                x+=8;
        }
}

/*-------------------------------------------------------------------------*/
ts_demux_t *new_ts_demux(uint8_t *vdmxbuf)
{
    ts_demux_t *td;

    td = (ts_demux_t *)malloc(sizeof(ts_demux_t));

    td->player = NULL;
    td->video_flush = NULL;
    td->audio_flush = NULL;
    td->get_video_out_buffer = NULL;

    td->apid = -1;
    td->vpid = -1;
    td->synced=0;

    td->vdmxbuf = vdmxbuf;
    td->vdmxbuflen = 0;
    td->vdmxbuflen_max = VDMXBUFLEN;

    td->frame_start = 1;

    td->tmp_aformat=FORMAT_NONE;
    td->admxbuf = (uint8_t *)malloc(ADMXBUFLEN);
    td->admxbuflen=0;
    td->admxbuflen_max = ADMXBUFLEN;

    memset(&td->vfi, 0, sizeof(video_frame_info_t));

    return td;
}
/*-------------------------------------------------------------------------*/
void delete_ts_demux(ts_demux_t *td)    
{
    free(td->admxbuf);
    free(td);
}
/*-------------------------------------------------------------------------*/
void ts_demux_set_player(ts_demux_t *self,
                         void *player,
                         video_flush_func video_flush,
                         audio_flush_func audio_flush,
                         get_video_out_buffer_func get_video_out_buffer)
{
    self->player = player;
    self->video_flush = video_flush;
    self->audio_flush = audio_flush;
    self->get_video_out_buffer = get_video_out_buffer;
}
/*-------------------------------------------------------------------------*/
void ts_demux_set_pids(ts_demux_t *td, int vpid, int apid)
{
    td->apid = apid;
    td->vpid = vpid;
    td->synced=0;
    td->vdmxbuflen = 0;
//    td->admxbuflen=0;
    td->vfi.vformat=0;
}
/*-------------------------------------------------------------------------*/
void ts_reset(ts_demux_t *td)
{
	// ?
}
/*-------------------------------------------------------------------------*/
int cc=0;  // continuity checker for VPID
int ccnum=0;
int ts_parse(ts_demux_t *td, u08_t const *data, int len)
{
    int adapfield;
    int i=0,offset;
    int pes_start,pid;
    int skew=0;
    int length_mode=0; // 0: 188, 1: 192

    if (!td->vfi.data)
    {
        td->vfi.vformat = FORMAT_NONE;
        td->vfi.data = td->get_video_out_buffer(td->player);
    }
    if (*(data+4)==0x47 && *(data+4+192)==0x47 ) {
	    length_mode=1;
	    data+=4;
    }

        while(*data!=0x47 && *(data+188+4*length_mode)!=0x47 && len>188) {
                data++;
                len--;
                skew++;

        }
        if (len<188){
                printf("ts_demux.c: error: too small packet\n");
                return 0;
        }

        for ( i = 0; i < len; i += 188+4*length_mode, data+=188+4*length_mode) {
#if 1
                if (data[1]&0x80) {// Error 
                        printf("ERROR\n");
                        continue;
                }
                if (data[3]&0xc0) // Scrambled 
                        continue;
#endif
                adapfield=data[3]&0x30;
                
                if (adapfield==0 || adapfield==0x20)
                        continue;

                if (adapfield==0x30) {
                        offset=5+data[4];
                        if (offset>188)
                                continue;
                }
                else
                        offset=4;
                        
                pes_start=data[1]&0x40;

                pid=((data[1]&0x1f)<<8)|data[2];

                if (pid==td->vpid && (data[3]&0x10)) {
#if 1//def DEBUG			
                        ccnum++;
			if ((data[3]&15)!=((cc+1)&15)) {
				printf("%i PID %x, cc is %i , should be %i\n",ccnum,td->vpid,data[3]&15,(cc+1)&15);
				ccnum=0;
				continue;
			}
			cc=data[3]&15;
#endif
                        vput(td, data+offset, 188-offset, pes_start);
                }
                else if (pid==td->apid) {
                        aput(td, data+offset, 188-offset, pes_start);
                }
                else {
                   //  printf("unknown pid = %X\n", pid);
                }		
        }
        return skew;
}
/*-------------------------------------------------------------------------*/
static void vput(ts_demux_t *td, uint8_t const *data, int len, int pes_start)
{
        if (pes_start) {
                parsevpes(td, td->vdmxbuf,td->vdmxbuflen);
                td->vdmxbuflen=0;
                td->audio_synced=1;
        }

        if (td->frame_synced && td->vdmxbuflen+len<td->vdmxbuflen_max) {
                memcpy(td->vdmxbuf+td->vdmxbuflen,data,len);
                td->vdmxbuflen+=len;
        }
        else {
                td->vdmxbuflen=0;
        }
}
/*-------------------------------------------------------------------------*/

static void aput(ts_demux_t *td, uint8_t const *data, int len, int pes_start)
{
        if (pes_start) {
                parseapes(td, td->admxbuf,td->admxbuflen);
                td->admxbuflen=0;
        }
        if (td->admxbuflen+len<=td->admxbuflen_max) {
                memcpy(td->admxbuf+td->admxbuflen,data,len);
                td->admxbuflen+=len;
        }
        else {
                td->admxbuflen=0;
        }
}
/*-------------------------------------------------------------------------*/
int parsevpes(ts_demux_t *td, u08_t const *data, int len)
{
        u08_t const *x, *last; // 
        int pos=0,l,ll;
        u08_t pt;
        u08_t z;
        int last_marker=0;
        int last_pt=-1;
	u08_t const *pp=data;

//	printf("PARSEPES %i @ %p\n",len,data);

        last=data;

        while(1) {

//              printf("Search %i\n",pos);
                x=findMarker(data,len,pos, &pt);
                if (!x) {
                        ll=data+len-last;
                        if (td->synced) {
                                store_vdata(td, last, ll,0);
                                return 0;
                        }
                        return 0;
                }

                l=data+len-x;
                z=x[3];
#if 0
                printf("XXX @%p, offset %i pos: %i marker: %02x last %x @%p\n",
		       x,x-data,pos,z,last_marker,last);
                hexdump(x,32);
#endif          
//		td->vfi.vformat=FORMAT_H264;
//		td->synced=1;
                if (z==0xb3) {
                        td->vfi.width=(x[4]<<4)|(x[5]>>4);
                        td->vfi.height=((x[5]&0xf)<<8) | x[6];
                }

                if (!td->synced) {

                        if (z==0x00) {
                                printf("############################## VIDEO SYNC MPEG2\n");
                                td->vfi.vformat=FORMAT_MPV;
                                last_pt=pt;
//				td->synced=1;

                        }
                        else if ((z==0xb3)&&(td->vfi.vformat==FORMAT_MPV)) {
                                last=x;
                                td->synced=1;
                        }
#if 1
//                        else if (z==0x67 || (z==0x09 && x[5]==0)) {
                        if ((z==0x67 || z==0x09)&&td->vfi.vformat!=FORMAT_MPV) {

                                last=x;
                                td->synced=1;
                                printf("############################## SYNC H.264\n");
                                printf("td->vfi.vformat[1] = %d\n", td->vfi.vformat);
                                td->vfi.vformat=FORMAT_H264;
                                printf("FORMAT_H264 = %d\n", td->vfi.vformat);
                                printf("td->vfi.vformat[2] = %d\n", td->vfi.vformat);
                        }
#endif
                }
                else {
                        if (z==0x00) {
                                ll=x-last;
//                                 printf("PICT TYPE %i\n",pt);
                                td->pic_type=pt;
                                if (last_marker!=0)
                                        store_vdata(td, last, ll,0);
                                last=x;
                        }
#if 0
                        if (z==0x09 && td->vfi.vformat==FORMAT_H264) {
                                
                                ll=x-last;
                                td->pic_type=pt;
				if (ll>1) 
				{
					store_vdata(td, last, ll, 0);
					store_vdata(td, last, 0, 1);
				}
                                last=x+4+2;
                                
                        }
#endif
                        if ((z&0xf0)==0xe0) {
                                ll=x-last;
                                if (td->vfi.vformat==FORMAT_H264)
                                        store_vdata(td, last, ll, 0);
                                else
                                        store_vdata(td, last, ll, 1);
                                store_pts(td, x, 0);
                                last=x+4+5+get_e0_length(x);
                        }

                }
                last_marker=z;
                pos=x-data+4;       
        }
}
/*-------------------------------------------------------------------------*/
int parseapes(ts_demux_t *td, u08_t const *data, size_t len)
{
        u08_t const *x=data,*lx;
        u08_t pt,z;
        int pos=0,l,ll;
	media_format_t fr=0;
	int start;

        lx=data;

        while(1) {
                x=findMarker(data,len,pos, &pt);
                if (!x) {
                        ll=data+len-lx;
                        store_adata(td, lx, ll);
                        return 0;
                }

                l=data+len-x;
                z=x[3];
		start=0;
		if (z == 0xbd && (x[7]&0xc0)==0x80) {
			fr = FORMAT_AC3;
			start=1;
		}
		if ((z&0xf0)==0xc0 && (x[7]&0xc0)==0x80) {
			fr = FORMAT_MPA;
			start=1;
		}
		if (start) {
//			printf("%i  ",len);hexdump(x,16);
			if (fr==td->tmp_aformat)
				td->ai.format = fr;

			td->tmp_aformat = fr;

                        ll=x-lx;
                        store_adata(td, lx, ll);
                        store_pts(td, x, 1);
                        lx=x+4+5+get_e0_length(x);
                }
                pos=x-data+4;       
        }
        return 0;
}
/*-------------------------------------------------------------------------*/
/*int parseapes_ger(ts_demux_t *td, uint8_t const *data, size_t len)
{
    while (len)
    {
        int stream_id;
        int substream_id = 0;
        uint8_t const *adata;
        size_t         adata_len;

        if (!parse_pes(&data,
                       &len,
                       &td->audio_pts,
                       &stream_id,
                       &substream_id,
                       &adata,
                       &adata_len)
            )
        {
            break;
        }
        printf("stream_id = %02X\n", stream_id);
        printf("substream_id = %02X\n", substream_id);

        store_adata(td, adata, adata_len);
    }
    return 0;
}*/
/*-------------------------------------------------------------------------*/
int xpts;
void store_pts(ts_demux_t *td, u08_t const *x, int type)
{
        int pdf=x[7]&0xf0;
        int pts;
        if (pdf==0x80 || pdf==0xc0) {
                pts= ((x[9+0] & 0x06) << 29) |
                   ( x[9+1]         << 22) |
                   ((x[9+2] & 0xFE) << 14) |
                   ( x[9+3]         <<  7) |
                   ( x[9+4]         >>  1);

                if (type==0) {
                        td->vfi.pts = pts;
//                        printf("VPTS %i      %u\n",pts-xpts,pts);
			xpts=pts;
                }
                else {
                        td->ai.pts = pts;
//                        printf("APTS %u\n",pts);
                }
        }
}
/*-------------------------------------------------------------------------*/
void store_adata(ts_demux_t *td, uint8_t const *x, size_t len)
{
    if ((ssize_t)len > 0 && x)
    {
        td->ai.data = x;
        td->ai.len = len;
        td->audio_flush(td->player, &td->ai);
        td->ai.pts = 0;
    }
}
/*-------------------------------------------------------------------------*/
// Store individual frames
static void store_vdata(ts_demux_t *td, uint8_t const *x, int len, int flush)
{
	if (len<0) {
//		printf("BUGBUG: store_vdata, len<0: %i, flush %i!\n",len,flush);
		return;
	}
	if (len) {
//		printf("store_vdata, %i, flush %i!\n",len,flush);
		td->vfi.data_len=len;
		td->vfi.data = x;
		td->vfi.frame_start=td->frame_start;
                td->vfi.still = false;
		if (td->frame_synced)
		td->video_flush(td->player, &td->vfi);
        }
//        td->vfi.pts = 0;
        if (flush)
	{
		td->video_synced = 1;
		td->frame_start=1;
	}
	else {
		td->frame_start=0;
	}
}
/*-------------------------------------------------------------------------*/
#define CMP_CORE(offset) \
        x=Data[i+1+offset]; \
        if (x<2) { \
                if (x==0) { \
                        if ( Data[i+offset]==0 && Data[i+2+offset]==1) \
                                return i+offset; \
                        else if (Data[i+2+offset]==0 && Data[i+3+offset]==1) \
                                return i+1+offset; \
                } \
                else if (x==1 && Data[i-1+offset]==0 && Data[i+offset]==0 && (i+offset)>0) \
                         return i-1+offset; \
         }
/*-------------------------------------------------------------------------*/

int FindPacketHeader(const u08_t *Data, int s, int l)
{
        int i;
        u08_t x;
        
        if (l>12) {
                for(i=s;i<l-12;i+=12) { 
                        CMP_CORE(0);
                        CMP_CORE(3);
                        CMP_CORE(6);
                        CMP_CORE(9);
                }
                for(;i<l-3;i+=3) {
                        CMP_CORE(0);
                }
        }
        else {
                for(i=s; i<l-3; i+=3) {
                        CMP_CORE(0);
                }
        }
        return -1;
}


#define NO_PICTURE 0
#define I_FRAME    1
#define P_FRAME    2
#define B_FRAME    3
/*-------------------------------------------------------------------------*/
uint8_t const *findMarker(uint8_t const *Data, int Count, int Offset, unsigned char *PictureType)
{
        int Length = Count;

        *PictureType = NO_PICTURE;
        if (Length > 0) {
                 uint8_t const *p = Data + Offset;
                 uint8_t const *pLimit = Data + Offset + Length - 3;
                
                while (p < pLimit) {
                        int x;
                        x=FindPacketHeader(p, 0, pLimit - p);
                        if (x!=-1) {           // found 0x000001
                                p+=x+2;
//                              printf("FOUND %02x\n",p[1]);
                                switch (p[1]) {
                                case 0: 
                                        *PictureType = (p[3] >> 3) & 0x07;
//                                      printf("############ %i\n",*PictureType);
                                        return p-2;

                                case 9: // Access Unit Delimiter AUD h.264
                                        if (p[2]==0x10)
                                                *PictureType=I_FRAME;
                                        else
                                                *PictureType=B_FRAME;
                                        return p-2; // 
                                case 0xe0 ... 0xef:                              
                                case 0x67:
                                case 0xb3:
                                case 0xb5:
                                case 0xbd:
                                case 0xc0:
                                case 0xc8: //needed for BBC HD
                                        return p-2;
                                }
                        }
                        else
                                break;
                }
        }       
        return NULL;
}
/*-------------------------------------------------------------------------*/

// 00 00 01 e0 LL LL FF FF HL
int get_e0_length(u08_t const *p)
{
        int add=p[8];
//      printf("ADD %i\n",add);
        return add;
}
/*-------------------------------------------------------------------------*/
static int read_pts(uint8_t const **pes_packet, size_t *pes_length, int *pts)
{
    if (*pes_length < 5)
    {
        return 0;
    }

    // We ignore the highest pts bit.
    *pts = (((*pes_packet)[0] & 0x06) << 29) |
           ( (*pes_packet)[1]         << 22) |
           (((*pes_packet)[2] & 0xFE) << 14) |
           ( (*pes_packet)[3]         <<  7) |
           ( (*pes_packet)[4]         >>  1);
    *pes_packet += 5;
    *pes_length -= 5;
    return 1;
}
/*-------------------------------------------------------------------------*/
int parse_pes(uint8_t const **pes_packet,        // in/out (will be advanced)
              size_t         *pes_packet_length, // in/out (will be advanced)
              int            *pts,               // out pts (0 = no pts)
              int            *stream_id,         // out
              int            *substream_id,      // out (valid if stream_id == 0xBD)
              uint8_t const **es_ptr,            // out
              size_t         *es_length          // out
             )
{
    uint8_t const *pes_ptr        = *pes_packet;
    size_t         pes_len        = *pes_packet_length;
    uint8_t const *pes_packet_end =  pes_ptr + pes_len;

    *pts = 0;
    {
        uint8_t const *d = *pes_packet;
        int n;
        printf("> %08X\n", (uint32_t)d);
        for (n = 0; n < 3; ++n)
        {
            printf("  %04X : %02X %02X %02X %02X %02X %02X %02X %02X | ", n * 16,
                        d[0], d[1], d[2], d[3],
                        d[4], d[5], d[6], d[7]);
            printf("%02X %02X %02X %02X %02X %02X %02X %02X\n",
                        d[8], d[9], d[10], d[11],
                        d[12], d[13], d[14], d[15]);
            d += 16;
        }
    }

    if (pes_len >= 4 &&
        pes_ptr[0] == 0 &&
        pes_ptr[1] == 0 &&
        pes_ptr[2] == 1)
    {
        /* stream id */
        *stream_id = pes_ptr[3];

        /* pes packet length */
        ssize_t pes_length;
        if (*stream_id == 0xB9)
        {
            pes_length = -2;
        }
        else
        {
            if (pes_len < 8)
            {
                return 0;
            }
            if (*stream_id == 0xBA)
            {
                // Pack header.
                if ((pes_ptr[4]>>4) == 4)
                {
                    pes_length = (pes_ptr[13] & 7) + 8;
                }
                else
                {
                    pes_length = 6;
                }
            }
            else
            {
                pes_length = pes_ptr[4] << 8 | pes_ptr[5];
            }
        }

        pes_ptr += 6;

        *pes_packet = pes_ptr + pes_length;
        *pes_packet_length -= pes_length + 6;

        if (!(*stream_id >= 0xC0 && *stream_id <= 0xEF) &&
              *stream_id != 0xBD)
        {
            return 0;
        }

        /* skip stuffing bytes */
        while (*pes_ptr == 0xFF && ++pes_ptr != pes_packet_end)
        {
            -- pes_length;
        }

        if (pes_length)
        {
            int8_t b = *pes_ptr++;
            -- pes_length;
            if ((b & 0xC0) == 0x80)
            {
                // MPEG-2
                if (pes_length >= 2)
                {
                    b = *pes_ptr++;
                    int pes_header_length = *pes_ptr++;
                    pes_length -= 2;
                    if (b & 0x80)
                    {
                        // pts is present
                        if (!read_pts(&pes_ptr, &pes_length, pts))
                        {
                            return 0;
                        }
                        pes_header_length -= 5;
                    }
                    if (pes_header_length >= 0 && pes_length >= pes_header_length)
                    {
                        pes_ptr += pes_header_length;
                        pes_length -= pes_header_length;

                        if (*stream_id == 0xBD)
                        {
                            if (!pes_length)
                            {
                                return 0;
                            }
                            *substream_id = *pes_ptr++;
                            -- pes_length;
                        }
                        *es_ptr = pes_ptr;
                        *es_length = pes_length;
                        return 1;
                    }
                }
            }
            else
            {
                // MPEG-1
                if ((b & 0xC0) == 0x40)
                {
                    if (pes_length < 2)
                    {
                        return 0;
                    }
                    b = pes_ptr[1];
                    pes_ptr += 2;
                    pes_length -= 2;
                }
                b &= 0xF0;
                if (b == 0x20 || b == 0x30)
                {
                    // pts is present
                    -- pes_ptr;
                    ++ pes_length;
                    if (!read_pts(&pes_ptr, &pes_length, pts))
                    {
                        return 0;
                    }
                    if (b == 0x30)
                    {
                        // skip dts
                        if (pes_length < 5)
                        {
                            return 0;
                        }
                        pes_ptr += 5;
                        pes_length -= 5;
                    }
                }
                *es_ptr = pes_ptr;
                *es_length = pes_length;

                return 1;
            }
        }
    }
    return 0;
}

