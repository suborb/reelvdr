#include <stdio.h>
#include <vdr/tools.h> //uchar 
int AddESAudioHeader(uchar *Data, size_t len)
{
    if (!Data || len < 4) return -1;

    /*ES header for Audio*/

    /*byte #0; sync word*/
    Data[0] = 0xFF;


    /*byte #1; sync word 0xF 
     *          ID      0:mpeg2   1:mpeg1
     *          Layer  11:Layer1 10:Layer2 01:Layer3 
     *          no protection 1:not protected
     *
     * 0xF0 + binary 1 01 1 = 0xFB*/  
    Data[1] = 0xFB;


    /*byte #2; bit rate index (4bits)
     *         sampling freq KHz 00:44.1 01:48 10:32 (XXX getinfo)
     *         padding 1bit
     *         private 1bit
     *
     */

    // 0x8  00 0 0
    Data[2] = 0x14;


    /*byte #3; mode 00:stereo 01:joint stereo 10:dual channel 11:single channel (XXX get info)
     *         mode ext 2bits
     *         copyright 0:none
     *         original  0:copy
     *         empahsis  2bits
     */

    // 11 00 0 0 00
    Data[3] = 0x44;

    return 4;
}

#define pts32_30  ((pts>>30)&0x07)
#define pts29_15  ((pts>>15)&0x7fff)
#define pts14_0   (pts&0x7FFF)

int AddPesAudioHeader(uchar* Data, size_t len, uint64_t pts)
{
    if (!Data || len < 8) return -1;

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
    Data[7] = pts>0?0x80:0x00;

    /*byte 8: PES header*/
    Data[8] = pts>0?0x05:0x00; //XXX always 8? can ignore optional fields?

    if(pts==0) 
        return 9;

    // set pts
    Data[9]    = ((pts32_30<<1)|0x01|0x20) & 0xff;
    Data[10]   = (pts29_15>>7) &0xff;
    Data[11]   = ((pts29_15<<1) | 0x01) &0xff;
    Data[12]   = (pts14_0 >>7) & 0xff;
    Data[13]   = ((pts14_0<<1) | 0x01) & 0xff;

    return 14; //14 bytes changed 
}
