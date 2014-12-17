#include "PesMpaTools.h"

#define pts32_30  ((pts>>30)&0x07)
#define pts29_15  ((pts>>15)&0x7fff)
#define pts14_0   (pts&0x7FFF)

int AddPesAudioHeader(uchar* Data, size_t len, uint64_t pts)
{
    if (!Data || len < 9) return -1;

    /* start code: bytes #0 #1 #2  0x00 00 01 */
    Data[0] = Data[1] = 0x00;
    Data[2] = 0x01;

    /*byte #3; Stream ID*/
    Data[3] = 0xC0; // audio stream ID 0xC0-0xDF

    /*byte #4 #5 Packet length: set later correctly*/
    Data[4] = (len >> 8); //MSB
    Data[5] = len & 0xFF; //LSB

    /*byte #6 marker 2bits :10
     *        scrambled? 2bits :00
     *        priority 1bit
     *        Data Align. 1bit :1
     *        Copyright :0
     *        Original :1
     */
//static int c = 0;
    // 10 00 0 1 0 1 : 0x85
    Data[6] = 0x84;//0x85;
#if 0
    if(c == 1)
    {
        Data[6] = 0x81;
    }
    c = 1;
#endif
    Data[7] = Data[8] = 0;
    if(pts==0 || len < 14) 
        return 9;
    /*byte #7 PTS       1bit
     *        DTS       1bit
     *        ESCR flag 1bit
     *        ES rate           1bit
     *        DSM trick             1bit
     *        additional copy info  1bit
     *        CRC flag  :0
     *        Extension flag 1bit
     */
    //  1 0 0 0 0 0 0 0
    Data[7] = 0x80;

    /*byte 8: PES header*/
    Data[8] = 0x05; //XXX always 8? can ignore optional fields?


    // set pts
    Data[9]    = ((pts32_30<<1)|0x01|0x20) & 0xff;
    Data[10]   = (pts29_15>>7) &0xff;
    Data[11]   = ((pts29_15<<1) | 0x01) &0xff;
    Data[12]   = (pts14_0 >>7) & 0xff;
    Data[13]   = ((pts14_0<<1) | 0x01) & 0xff;

    return 14; //14 bytes changed 
}



int find_valid_mpa_frame(const uchar* data, size_t len, mpa_frame_t* frame)
{
    if (len < 4)
        return -1;

    int layer, id, bit_rate_idx, sample_rate_idx, padding;
    int lsf, mp25;

    const uchar*p = NULL;
    int i = 0;

    while(((size_t)i+3)<len)
    {
        p = data + i;

        // not the sync word, jump one or two bytes
        if (p[0] != 0xff || (p[1] & 0xe0) != 0xe0) 
        {
            if (p[1] == 0xff)
                i++;
            else 
                i +=2;

            continue;
        }

        id                  = (p[1]>>3)&0x03;
        layer               = 4-(p[1]>>1)&0x03;

        bit_rate_idx        = (p[2]>>4)&0x0f;
        sample_rate_idx     = (p[2]>>2)&0x03;
        padding             = (p[2]>>1) &0x01;
        lsf                 = (id==0x0) || (id==0x02); // low sampling freq
        mp25                = (id==0x0);

        if(p[0] != 0xff || (p[1] &0xe0) != 0xe0 || id == 1 || 
                sample_rate_idx > 2 || layer > 3 || bit_rate_idx > 14) 
        {
            // not a frame || does not have a valid header 
            // continue searching
            ++i;
        }
        else
        {
            //found valid frame header

            //bitrate
            //sample rate

            if(frame)
            {

                frame->id               = id;
                frame->layer            = layer;
                frame->bri              = bit_rate_idx;
                frame->sri              = sample_rate_idx;
                frame->padding          = padding;
                frame->lsf              = lsf;
                frame->mp25             = mp25;

                frame->bitrate          = mpa_bitrate_table[lsf][layer-1][bit_rate_idx]*1000;
                frame->samplerate       = mpa_samplerate_table[sample_rate_idx]>>(lsf+mp25);

                switch(layer)
                {
                    case 1:
                        frame->samples_per_frame = 384; break;
                    case 2:
                        frame->samples_per_frame = 1152; break;
                    default:
                        frame->samples_per_frame = lsf?576:1152; break;
                }

//DL                frame->duration = (float)(frame->samples_per_frame)/frame->samplerate; // in secs
                //     frame->len      = (int)((frame->duration*frame->bitrate)/8); // in bytes
                frame->len      = (frame->samples_per_frame/8 * frame->bitrate)/frame->samplerate + frame->padding;


                if( i + frame->len +1 < len)
                {
                    int k = frame->len + 0;
                    if(p[k] != 0xff || (p[k+1]&0xe0)!= 0xe0)
                    {

                //    printf("FALSE +ve: bitrate: %i [%i, %i, %i(%0#x)]\t samplerate: %i\n", frame->bitrate, lsf, layer, bit_rate_idx,p[2],  frame->samplerate);
                  //  printf("%0#x %0#x [%0#x] %0#x\n", p[k-2], p[k-1],p[k],p[k+1]);
                        i++;   continue; //next sync word not found. Therefore, assume not a frame beginning.
                    }
//                    printf("bitrate: %i [%i, %i, %i(%0#x)]\t samplerate: %i\n", frame->bitrate, lsf, layer, bit_rate_idx,p[2],  frame->samplerate);
                }

            }

            return i;
        }

    }// while: loop thro' data

    return -1;
}
