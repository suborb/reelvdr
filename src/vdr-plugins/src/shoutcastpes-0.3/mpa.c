
int find_valid_mpa_frame(const uchar* data, size_t len, mpa_frame_t* frame)
{
    if (len < 4)
        return -1;

    int layer, id, bit_rate_idx, sample_rate_idx, padding;
    int lsf, mp25;

    const uchar*p = NULL;
    int i = 0;

    while((size_t)i<len)
    {
        p = data + i;

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

                frame->duration = (float)(frame->samples_per_frame)/frame->samplerate; // in secs
                //     frame->len      = (int)((frame->duration*frame->bitrate)/8); // in bytes
                frame->len      = (frame->samples_per_frame/8 * frame->bitrate)/frame->samplerate + frame->padding;


                if( i + frame->len < len)
                {
                    int k = frame->len + 0;
                    if(p[k] != 0xff || (p[k+1]&0xe0)!= 0xe0)
                    {
                        i++;   continue;
                    }
//                    printf("bitrate: %i [%i, %i, %i(%0#x)]\t samplerate: %i\n", frame->bitrate, lsf, layer, bit_rate_idx,p[2],  frame->samplerate);
                }

            }

            return i;
        }

    }// while: loop thro' data

    return -1;
}
