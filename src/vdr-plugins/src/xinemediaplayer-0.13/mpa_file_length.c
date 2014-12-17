#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include "PesMpaTools.h"


/*
   finds if there is a valid mpa frame header at CURR_POS of fd
   assume 0xff has been found already
   */
int find_mpa_header_at(int fd, float* d)
{
    uchar p[2];
    int flag = read(fd, p, 2);

    if (flag != 2) // read error
    {
        printf("Read error! %i returned\n", flag);
        return 0;
    }

    if ((p[0]&0xe0)!=0xe0) // sync word not found
    {		
        //leave the file handle at the byte after the 0xff
        lseek(fd, -1, SEEK_CUR); // -1 since p[0] != 0xff, and we are looking for 0xff
//        printf("Sync word not found\n");
        return 0;
    }

#define id           		 	((p[0]>>3)&0x03)
#define layer          		 	(4-(p[0]>>1)&0x03)

#define bit_rate_idx   		 	((p[1]>>4)&0x0f)
#define sample_rate_idx      	((p[1]>>2)&0x03)

#define padding             	((p[1]>>1) &0x01)
#define lsf                 	((id==0x0) || (id==0x02)) // low sampling freq
#define mp25                	(id==0x0)

    if (id == 1 || sample_rate_idx > 2 || layer > 3 || bit_rate_idx == 0 || bit_rate_idx > 14) // not a valid header
    {
        lseek(fd, -2, SEEK_CUR);
//        printf("invalid header  id:%i  sri:%i  layer:%i  bri:%i\n", id, sample_rate_idx, layer, bit_rate_idx);
        return 0;
    }

#define bitrate  		(mpa_bitrate_table[lsf][layer-1][bit_rate_idx]*1000)
#define samplerate		(mpa_samplerate_table[sample_rate_idx]>>(lsf+mp25))

    int samples_per_frame = 0;
    switch(layer)
    {
        case 1:
            samples_per_frame = 384; break;
        case 2:
            samples_per_frame = 1152; break;
        default:
            samples_per_frame = lsf?576:1152; break;
    }

    int len = (samples_per_frame/8 * bitrate)/samplerate + padding;
    float frame_duration = (float)(samples_per_frame)/samplerate; // in secs

    //check if the next sync word is found after this frame
    if (lseek(fd, len-3, SEEK_CUR) == (off_t)-1) // seek error
    {
    //    printf("seek error: tried jumping %i bytes\n", len-3);
        lseek(fd, -2, SEEK_CUR);
        return 0;
    }
    flag = read(fd, p, 2);

    if(p[0]==0xff && (p[1]&0xe0)==0xe0) // next sync word found
    {
        *d += frame_duration;

        //leave the file handle at the beginning
        //of the frame for next  frame detection
        lseek(fd, -2, SEEK_CUR);
        //printf("got header : %i bytes\n", len);
        return 1;
    }
    else
        return 0;
}


int mpa_file_length(int fd, int *len)
{

    if(fd <0 || !len) 
        return 0;

    float duration = 0.0;

    // start from the beginning of the file
    lseek(fd, 0, SEEK_SET);
    unsigned char c = 0;
    int flag = 0;

    //read through the file
    while (1)
    {
        flag = read(fd, &c, 1);
        if(flag != 1)
        {
    //        printf("ERROR: read %i bytes instead of 1\n", flag);

            // eof
            if(!flag) 
            {
     //           printf("EOF reached\n");
                *len = duration;
            }

            break;	
        }

        if (c == 0xff) 
        {
            //find mpa frame here
            flag = find_mpa_header_at(fd, &duration);
        }
    }// while(1)


    return 1;
} // mpa_file_length


