/*
 * radioaudio.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <vdr/remote.h>
#include <vdr/status.h>
#include <vdr/plugin.h>
#include "radioaudio.h"
#include "radioskin.h"
#include "radiotools.h"
#include "i18n.h"
#include <math.h>

// OSD-Symbols
#include "symbols/rds.xpm"
#include "symbols/arec.xpm"
#include "symbols/rass.xpm"
#include "symbols/index.xpm"
#include "symbols/marker.xpm"
#include "symbols/page1.xpm"
#include "symbols/pages2.xpm"
#include "symbols/pages3.xpm"
#include "symbols/pages4.xpm"
#include "symbols/no0.xpm"
#include "symbols/no1.xpm"
#include "symbols/no2.xpm"
#include "symbols/no3.xpm"
#include "symbols/no4.xpm"
#include "symbols/no5.xpm"
#include "symbols/no6.xpm"
#include "symbols/no7.xpm"
#include "symbols/no8.xpm"
#include "symbols/no9.xpm"

// Radiotext
int RTP_ItemToggle = 1, RTP_TToggle = 0;
bool RT_MsgShow = false, RT_PlusShow = false;
bool RT_Replay = false, RT_ReOpen = false;
char *RT_Titel, *RTp_Titel;
rtp_classes rtp_content;
// RDS rest
bool RDS_PSShow = false;
int RDS_PSIndex = 0;
char RDS_PSText[12][9];
char RDS_PTYN[9];
// plugin audiorecorder service
bool ARec_Receive = false, ARec_Record = false;
// QDAr
int QDar_Show = -1;		// -1=No, 0=Yes, 1=display
int QDar_Archiv = -1;		// -1=Off, 0=Index, 1000-9990=Slidenr.
bool QDar_Flags[10][4];		// Slides existent

cRadioAudio *RadioAudio;
cRadioTextOsd *RadioTextOsd;
cRDSReceiver *RDSReceiver;


// RDS-Chartranslation: 0x80..0xff
unsigned char rds_addchar[128] = {
    0xe1, 0xe0, 0xe9, 0xe8, 0xed, 0xec, 0xf3, 0xf2, 
    0xfa, 0xf9, 0xd1, 0xc7, 0x8c, 0xdf, 0x8e, 0x8f, 
    0xe2, 0xe4, 0xea, 0xeb, 0xee, 0xef, 0xf4, 0xf6, 
    0xfb, 0xfc, 0xf1, 0xe7, 0x9c, 0x9d, 0x9e, 0x9f, 
    0xaa, 0xa1, 0xa9, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 
    0xa8, 0xa9, 0xa3, 0xab, 0xac, 0xad, 0xae, 0xaf, 
    0xba, 0xb9, 0xb2, 0xb3, 0xb1, 0xa1, 0xb6, 0xb7, 
    0xb5, 0xbf, 0xf7, 0xb0, 0xbc, 0xbd, 0xbe, 0xa7, 
    0xc1, 0xc0, 0xc9, 0xc8, 0xcd, 0xcc, 0xd3, 0xd2, 
    0xda, 0xd9, 0xca, 0xcb, 0xcc, 0xcd, 0xd0, 0xcf, 
    0xc2, 0xc4, 0xca, 0xcb, 0xce, 0xcf, 0xd4, 0xd6, 
    0xdb, 0xdc, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 
    0xc3, 0xc5, 0xc6, 0xe3, 0xe4, 0xdd, 0xd5, 0xd8, 
    0xde, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xf0,
    0xe3, 0xe5, 0xe6, 0xf3, 0xf4, 0xfd, 0xf5, 0xf8, 
    0xfe, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};

const char *entitystr[5]  = { "&apos;", "&amp;", "&quote;", "&gt", "&lt" };
const char *entitychar[5] = { "'", "&", "\"", ">", "<" };
char *rds_entitychar(char *text)
{
    int i = 0, l, lof, lre, space;
    char *temp;
    
    while (i < 5) {
        if ((temp = strstr(text, entitystr[i])) != NULL) {
	    if (S_Verbose >= 2)
    		printf("RText-Entity: %s\n", text);
	    l = strlen(entitystr[i]);
	    lof = (temp-text);
	    if (strlen(text) < RT_MEL) {
		lre = strlen(text) - lof - l;
		space = 1;
		}
	    else {
		lre =  RT_MEL - 1 - lof - l;
		space = 0;
		}
	    memmove(text+lof, entitychar[i], 1);
	    memmove(text+lof+1, temp+l, lre);
	    if (space != 0)
		memmove(text+lof+1+lre, "         ", l-1);
	    }
	else i++;
	}

    return text;
}

const char* ptynr2string(int nr)
{
    switch (nr) {
        // Source: http://www.ebu.ch/trev_255-beale.pdf
        case  0: return tr("unknown program type");
	case  1: return tr("News"); 
	case  2: return tr("Current affairs"); 
	case  3: return tr("Information"); 
	case  4: return tr("Sport"); 
	case  5: return tr("Education"); 
	case  6: return tr("Drama"); 
	case  7: return tr("Culture"); 
	case  8: return tr("Science"); 
	case  9: return tr("Varied"); 
	case 10: return tr("Pop music"); 
	case 11: return tr("Rock music"); 
	case 12: return tr("M.O.R. music"); 
	case 13: return tr("Light classical"); 
	case 14: return tr("Serious classical"); 
	case 15: return tr("Other music"); 
	// 16-30 "Spares"
	case 31: return tr("Alarm");
	default: return "?";
	}
}


// --- cRDSReceiver ------------------------------------------------------------

cRDSReceiver::cRDSReceiver(int Pid)
:cReceiver(0, -1, Pid)
{
    if (S_Verbose >= 1)
        printf("vdr-radio: additional RDS-Receiver starts on Pid=%d\n", Pid);

    pid = Pid;
    rt_start = rt_bstuff = false;
}

cRDSReceiver::~cRDSReceiver()
{
    if (S_Verbose >= 1)
        printf("vdr-radio: additional RDS-Receiver stopped\n");
}

void cRDSReceiver::Receive(uchar *Data, int Length)
{
    const int mframel = 263;  // max. 255(MSG)+4(ADD/SQC/MFL)+2(CRC)+2(Start/Stop) of RDS-data
    static unsigned char mtext[mframel+1];
    static int index;
    static int mec = 0;
    
    // check TS-Size, -Sync, PID, Payload
    if (Length != TS_SIZE || Data[0] != 0x47 || pid != ((Data[1] & 0x1f)<<8)+Data[2] || !(Data[3] & 0x10))
	return;

    int offset;
    if (Data[1] & 0x40)	{		// 1.TS-Frame, payload-unit-start
	offset = (Data[3] & 0x20) ? Data[4] + 11 : 10;	// Header + ADFL + 6 byte: PES-Startcode, -StreamID, -PacketLength
	if (Data[offset-3] == 0xbd) {	// StreamID = Private stream 1 (for rds)
	    offset += 3;		// 3 byte: Extension + Headerlength
	    offset += Data[offset-1];
	    }
	else
	    return;
	}
    else
	offset = (Data[3] & 0x20) ? Data[4] + 5 : 4;	// Header + ADFL

    if (TS_SIZE-offset <= 0)
        return;

    // print TS-RawData with RDS
    if (S_Verbose >= 3) {
	printf("\n\nTS-Data(%d): ", Length);
	for (int a=0; a<Length; a++)
	    printf("%02x ", Data[a]);
	printf("(End)\n\n");
	}

    for (int i = 0, val = 0; i < (TS_SIZE-offset); i++) {
        val = Data[offset+i];

	if (val == 0xfe) {	// Start
	    index = -1;
	    rt_start = true;
	    rt_bstuff = false;
	    mec = 0;
	    if (S_Verbose >= 2)
		printf("\nRDS-Start: ");
	    }

	if (rt_start) {
	    if (S_Verbose >= 2)
		printf("%02x ", val);
	    // byte-stuffing reverse: 0xfd00->0xfd, 0xfd01->0xfe, 0xfd02->0xff
	    if (rt_bstuff) {
		switch (val) {
		    case 0x00: mtext[index] = 0xfd; break;
		    case 0x01: mtext[index] = 0xfe; break;
		    case 0x02: mtext[index] = 0xff; break;
		    default: mtext[++index] = val;	// should never be
		    }
		rt_bstuff = false;
		if (S_Verbose >= 2)
		    printf("(Bytestuffing -> %02x) ", mtext[index]);
		}
	    else
		mtext[++index] = val;
	    if (val == 0xfd && index > 0)	// stuffing found
		rt_bstuff = true;

	    // early check for used MEC
	    if (index == 5) {
		//mec = val;
		switch (val) {
		    case 0x0a:			// RT
		    case 0x46:			// RTplus-Tags
		    case 0x07:			// PTY
		    case 0x3e:			// PTYN
		    case 0x02:	mec = val;	// PS
				break;
		    default:	rt_start = false;
				if (S_Verbose >= 2)
				    printf("(RDS-MEC '%02x' not used -> End)\n", val);
		    }
		}
	    if (index >= mframel) {	// max. rdslength, garbage ?
		rt_start = false;
		if (S_Verbose >= 2)
		    printf("(RDS-Error: too long, garbage ?)\n");
		}
	    }

	if (rt_start && val == 0xff) {	// End
	    rt_start = false;
	    if (S_Verbose >= 2)
		printf("(RDS-End)\n");
	    if (index < 9) {		//  min. rdslength, garbage ?
		if (S_Verbose >= 1)
		    printf("RDS-Error: too short -> garbage ?\n");
		}
	    else {
		// crc16-check
		unsigned short crc16 = crc16_ccitt(mtext, index-3, true);
		if (crc16 != (mtext[index-2]<<8)+mtext[index-1]) {
		    if (S_Verbose >= 1)
			printf("RDS-Error: wrong CRC # calc = %04x <> transmit = %02x%02x\n", crc16, mtext[index-2], mtext[index-1]);
		    }
		else {
		    switch (mec) {
			case 0x0a:
			case 0x46:  RadioAudio->RadiotextDecode(mtext, index);		// RT, RTplus
				    break;
			case 0x07:  RT_PTY = mtext[8];					// PTY
				    if (S_Verbose >= 1)
					printf("RDS-PTY set to '%s'\n", ptynr2string(RT_PTY));
				    break;
			case 0x3e:  RadioAudio->RDS_PsPtynDecode(true, mtext, index);	// PTYN
				    break;
			case 0x02:  RadioAudio->RDS_PsPtynDecode(false, mtext, index);	// PS
			}
		    }
		}
    	    }
	
	}
}


// --- cRadioAudio -------------------------------------------------------------

cRadioAudio::cRadioAudio()
  : cAudio()
  , enabled(false)
  , imagepath(0)
  , imageShown(false)
  , imagedelay(100)
  , first_packets(0)
{
    RadioAudio = this;
    dsyslog("radio: new RadioAudio");
}

cRadioAudio::~cRadioAudio()
{
    dsyslog("radio: delete RadioAudio");
    free(imagepath);
}

void cRadioAudio::SetBackgroundImage(const char *Image)
{
    free(imagepath);
    imagepath = 0;

    if (Image) {
	imageShown = false;
	asprintf(&imagepath, "%s", Image);
    }
}

void cRadioAudio::Play(const uchar *Data, int Length, uchar Id)
{
    if (!enabled)
	return;

    if (imagepath && !imageShown && imagedelay <= 0) {
	imageShown = true;
        ShowImage(imagepath);
	imagedelay = 50;
        }
    else if (imagedelay > 0)
	imagedelay--;
  
    if (Id < 0xc0 || Id > 0xdf)
	return;

    // QDAr-Images Slideshow (only in Livemode, probs with dvbplayer @ replaying, filechaching/transfermode ???)
    if (S_QDarText > 0 && QDar_Archiv == -1 && QDar_Show == 1) {
	QDar_Show = 0;
	if (!RT_Replay) {
	    char *image;
	    asprintf(&image, "%s/QDar_show.mpg", DataDir);
	    SetBackgroundImage(image);
	    free(image);
	    }
	}

    // check Radiotext-PES
    if (S_RtFunc < 1)
	return;
    if (first_packets < 3) {
	first_packets++;
	return;
	}
    RadiotextCheckPES(Data, Length);

    // Radiotext-Autodisplay
    static int rtdelay = 100;
    if ((S_RtDispl == 2) && (RT_Info >= 0) && !RT_OsdTO && (RT_OsdTOTemp == 0) && RT_ReOpen && !Skins.IsOpen() && !cOsd::IsOpen() && (rtdelay <= 0)) {
        cRemote::CallPlugin("radio");
        rtdelay = 100;
	}
    else if (rtdelay >0)
        rtdelay--;
}

void cRadioAudio::ShowImage(const char *file)
{
    uchar *buffer;
    int fd;
    struct stat st;
    struct video_still_picture sp;
    if ((fd = open (file, O_RDONLY)) >= 0)
	{
		fstat (fd, &st);
		sp.iFrame = (char *) malloc (st.st_size);
		if (sp.iFrame)
		{
			sp.size = st.st_size;
			if (read (fd, sp.iFrame, sp.size) > 0)
			{
				buffer = (uchar *) sp.iFrame;  
#ifdef USE_STILLPICTURE
				cDevice::PrimaryDevice()->StillPicture(buffer, sp.size);
#else
				for (int i = 1; i <= 25; i++)
				{
					send_pes_packet (buffer, sp.size, i);
				}
#endif
			}
			free (sp.iFrame);
		}
		else
		{
		}
		close (fd);
	}
	else
	{
	}
}

void cRadioAudio::send_pes_packet(unsigned char *data, int len, int timestamp)
{
#define PES_MAX_SIZE 2048
    int ptslen = timestamp ? 5 : 1;
    static unsigned char pes_header[PES_MAX_SIZE];
    pes_header[0] = pes_header[1] = 0;
    pes_header[2] = 1;
    pes_header[3] = 0xe0;
    while(len > 0)
	{
		int payload_size = len;
		if(6 + ptslen + payload_size > PES_MAX_SIZE)
			payload_size = PES_MAX_SIZE - (6 + ptslen);
		pes_header[4] = (ptslen + payload_size) >> 8;
		pes_header[5] = (ptslen + payload_size) & 255;
		if(ptslen == 5)
		{
			int x;
			x = (0x02 << 4) | (((timestamp >> 30) & 0x07) << 1) | 1;
			pes_header[8] = x;
			x = ((((timestamp >> 15) & 0x7fff) << 1) | 1);
			pes_header[7] = x >> 8;
			pes_header[8] = x & 255;
			x = ((((timestamp) & 0x7fff) < 1) | 1);
			pes_header[9] = x >> 8;
			pes_header[10] = x & 255;
		} else
		{
			pes_header[6] = 0x0f;
		}
		memcpy(&pes_header[6 + ptslen], data, payload_size);
		cDevice::PrimaryDevice()->PlayPes(pes_header, 6 + ptslen + payload_size);
		len -= payload_size;
		data += payload_size;
		ptslen = 1;
	}
}

void cRadioAudio::RadiotextCheckPES(const unsigned char *data, int len)
{
    const int mframel = 263;  // max. 255(MSG)+4(ADD/SQC/MFL)+2(CRC)+2(Start/Stop) of RDS-data
    static unsigned char mtext[mframel+1];
    static bool rt_start = false, rt_bstuff=false;
    static int index;
    static int mec = 0;
  
    int offset = 0;
    while (true) {

	int pesl = (offset+5 < len) ? (data[offset+4] << 8) + data[offset+5] + 6 : -1;
	if (pesl <= 0 || offset+pesl > len)
	    return;

	offset += pesl;
	int rdsl = data[offset-2];	// RDS DataFieldLength
	// RDS DataSync = 0xfd @ end
	if (data[offset-1] == 0xfd && rdsl > 0) {
	    // print RawData with RDS-Info
	    if (S_Verbose >= 3) {
    		printf("\n\nPES-Data(%d/%d): ", pesl, len);
    		for (int a=offset-pesl; a<offset; a++)
		    printf("%02x ", data[a]);
		printf("(End)\n\n");
		}
    
	    for (int i = offset-3, val; i > offset-3-rdsl; i--) {	// <-- data reverse, from end to start
    		val = data[i];

		if (val == 0xfe) {	// Start
		    index = -1;
		    rt_start = true;
		    rt_bstuff = false;
		    if (S_Verbose >= 2)
			printf("\nRDS-Start: ");
		    }

		if (rt_start) {
		    if (S_Verbose >= 2)
			printf("%02x ", val);
		    // byte-stuffing reverse: 0xfd00->0xfd, 0xfd01->0xfe, 0xfd02->0xff
		    if (rt_bstuff) {
			switch (val) {
			    case 0x00: mtext[index] = 0xfd; break;
			    case 0x01: mtext[index] = 0xfe; break;
			    case 0x02: mtext[index] = 0xff; break;
			    default: mtext[++index] = val;	// should never be
			    }
			rt_bstuff = false;
			if (S_Verbose >= 2)
			    printf("(Bytestuffing -> %02x) ", mtext[index]);
			}
		    else
			mtext[++index] = val;
		    if (val == 0xfd && index > 0)	// stuffing found
		        rt_bstuff = true;
		    // early check for used MEC
		    if (index == 5) {
			//mec = val;
			switch (val) {
			    case 0x0a:			// RT
			    case 0x46:			// RTplus-Tags
			    case 0xda:			// QDar
			    case 0x07:			// PTY
			    case 0x3e:			// PTYN
			    case 0x02:	mec = val;	// PS
					break;
			    default:	rt_start = false;
					if (S_Verbose >= 2)
					    printf("(RDS-MEC '%02x' not used -> End)\n", val);
			    }
			}
		    if (index >= mframel) {		// max. rdslength, garbage ?
			if (S_Verbose >= 1)
			    printf("RDS-Error: too long, garbage ?\n");
			rt_start = false;
			}
		    }

		if (rt_start && val == 0xff) {	// End
		    if (S_Verbose >= 2)
			printf("(RDS-End)\n");
		    rt_start = false;
		    if (index < 9) {		//  min. rdslength, garbage ?
			if (S_Verbose >= 1)
			    printf("RDS-Error: too short -> garbage ?\n");
			}
		    else {
			// crc16-check
			unsigned short crc16 = crc16_ccitt(mtext, index-3, true);
			if (crc16 != (mtext[index-2]<<8)+mtext[index-1]) {
			    if (S_Verbose >= 1)
				printf("RDS-Error: wrong CRC # calc = %04x <> transmit = %02x%02x\n", crc16, mtext[index-2], mtext[index-1]);
			    }
			else {
			    switch (mec) {
				case 0x0a:
				case 0x46:  RadiotextDecode(mtext, index);		// Radiotext, RT+
					    break;
				case 0x07:  RT_PTY = mtext[8];				// PTY
					    if (S_Verbose >= 1)
						printf("RDS-PTY set to '%s'\n", ptynr2string(RT_PTY));
					    break;
				case 0x3e:  RDS_PsPtynDecode(true, mtext, index);	// PTYN
					    break;
				case 0x02:  RDS_PsPtynDecode(false, mtext, index);	// PS
					    break;
				case 0xda:  QDarDecode(mtext, index);			// QDar
				}
			    }
			}
		    }

		}
	    }
	}
}


void cRadioAudio::RadiotextDecode(unsigned char *mtext, int len)
{
    static bool rtp_itoggle = false;
    static int rtp_idiffs = 0;
    static cTimeMs rtp_itime;
    static char plustext[RT_MEL];

    // byte 1+2 = ADD (10bit SiteAdress + 6bit EncoderAdress)
    // byte 3   = SQC (Sequence Counter 0x00 = not used)
    int leninfo = mtext[4];	// byte 4 = MFL (Message Field Length)
    if (len >= leninfo+7) {	// check complete length

	// byte 5 = MEC (Message Element Code, 0x0a for RT, 0x46 for RTplus)
	if (mtext[5] == 0x0a) {
	    // byte 6+7 = DSN+PSN (DataSetNumber+ProgramServiceNumber, 
	    //		       	   ignore here, always 0x00 ?)
	    // byte 8   = MEL (MessageElementLength, max. 64+1 byte @ RT)
	    if (mtext[8] == 0 || mtext[8] > RT_MEL || mtext[8] > leninfo-4) {
		if (S_Verbose >= 1)
		    printf("RT-Error: Length = 0 or not correct !");
		return;
		}
	    // byte 9 = RT-Status bitcodet (0=AB-flagcontrol, 1-4=Transmission-Number, 5+6=Buffer-Config,
	    //				    ingnored, always 0x01 ?)
	    char temptext[RT_MEL];
	    memset(temptext, 0x20, RT_MEL-1);
	    for (int i = 1, ii = 0; i < mtext[8]; i++) {
	        if (mtext[9+i] <= 0xfe)
		    // additional rds-character, see RBDS-Standard, Annex E
	            temptext[ii++] = (mtext[9+i] >= 0x80) ? rds_addchar[mtext[9+i]-0x80] : mtext[9+i];
		}
	    memcpy(plustext, temptext, RT_MEL-1);
	    rds_entitychar(temptext);
	    // check repeats
	    bool repeat = false;
	    for (int ind = 0; ind < S_RtOsdRows; ind++) {
	    	if (memcmp(RT_Text[ind], temptext, RT_MEL-1) == 0) {
	    	    repeat = true;
		    if (S_Verbose >= 1)
		    	printf("RText-Rep[%d]: %s\n", ind, RT_Text[ind]);
		    }
		}
	    if (!repeat) {
	    	memcpy(RT_Text[RT_Index], temptext, RT_MEL-1);
		// +Memory
		char *temp;
		asprintf(&temp, "%s", RT_Text[RT_Index]);
		if (++rtp_content.rt_Index >= 2*MAX_RTPC)
		    rtp_content.rt_Index = 0;
		asprintf(&rtp_content.radiotext[rtp_content.rt_Index], "%s", rtrim(temp));
		free(temp);
	    	if (S_Verbose >= 1)
		    printf("Radiotext[%d]: %s\n", RT_Index, RT_Text[RT_Index]);
	    	RT_Index +=1; if (RT_Index >= S_RtOsdRows) RT_Index = 0;
		}
	    RTP_TToggle = 0x03;		// Bit 0/1 = Title/Artist
	    RT_MsgShow = true;
	    RT_Info = (RT_Info > 0) ? : 1;
	    RadioStatusMsg();
	    }

	else if (RTP_TToggle > 0 && mtext[5] == 0x46 && S_RtFunc >= 2) {	// RTplus tags V2.0, only if RT
	    if (mtext[6] > leninfo-2 || mtext[6] != 8) { // byte 6 = MEL, only 8 byte for 2 tags
		if (S_Verbose >= 1)
		    printf("RTp-Error: Length not correct !");
		return;
		}
	    uint rtp_typ[2], rtp_start[2], rtp_len[2];
	    // byte 7+8 = ApplicationID, always 0x4bd7
	    // byte 9   = Applicationgroup Typecode / PTY ?
	    // bit 10#4 = Item Togglebit
	    // bit 10#3 = Item Runningbit
	    // Tag1: bit 10#2..11#5 = Contenttype, 11#4..12#7 = Startmarker, 12#6..12#1 = Length
	    rtp_typ[0]   = (0x38 & mtext[10]<<3) | mtext[11]>>5;
	    rtp_start[0] = (0x3e & mtext[11]<<1) | mtext[12]>>7;
	    rtp_len[0]   = 0x3f & mtext[12]>>1;
	    // Tag2: bit 12#0..13#3 = Contenttype, 13#2..14#5 = Startmarker, 14#4..14#0 = Length(5bit)
	    rtp_typ[1]   = (0x20 & mtext[12]<<5) | mtext[13]>>3;
	    rtp_start[1] = (0x38 & mtext[13]<<3) | mtext[14]>>5;
	    rtp_len[1]   = 0x1f & mtext[14];
	    if (S_Verbose >= 2)
    	        printf("RTplus (tag=Typ/Start/Len):  Toggle/Run = %d/%d, tag#1 = %d/%d/%d, tag#2 = %d/%d/%d\n", 
	               (mtext[10]&0x10)>0, (mtext[10]&0x08)>0, rtp_typ[0], rtp_start[0], rtp_len[0], rtp_typ[1], rtp_start[1], rtp_len[1]);
	    // save info
	    for (int i = 0; i < 2; i++) {
		if (rtp_start[i]+rtp_len[i]+1 >= RT_MEL) {	// length-error
		    if (S_Verbose >= 1)
    		        printf("RTp-Error (tag#%d = Typ/Start/Len): %d/%d/%d (Start+Length > 'RT-MEL' !)\n",
		               i+1, rtp_typ[i], rtp_start[i], rtp_len[i]);
		    }
		else {
		    char temptext[RT_MEL];
		    memset(temptext, 0x20, RT_MEL-1);
		    memmove(temptext, plustext+rtp_start[i], rtp_len[i]+1);
		    rds_entitychar(temptext);
		    // +Memory
		    memset(rtp_content.temptext, 0x20, RT_MEL-1);
		    memcpy(rtp_content.temptext, temptext, RT_MEL-1);
		    switch (rtp_typ[i]) {
			case 1:		// Item-Title	
				if ((mtext[10] & 0x08) > 0 && (RTP_TToggle & 0x01) == 0x01) {
				    RTP_TToggle -= 0x01;
				    RT_Info = 2;
				    if (memcmp(RTP_Title, temptext, RT_MEL-1) != 0 || (mtext[10] & 0x10) != RTP_ItemToggle) {
	    				memcpy(RTP_Title, temptext, RT_MEL-1);
					if (RT_PlusShow && rtp_itime.Elapsed() > 1000)
					    rtp_idiffs = (int) rtp_itime.Elapsed()/1000;
					if (!rtp_content.item_New) {
					    RTP_Starttime = time(NULL);
					    rtp_itime.Set(0);
					    sprintf(RTP_Artist, "---");
					    if (++rtp_content.item_Index >= MAX_RTPC)
						rtp_content.item_Index = 0;
					    rtp_content.item_Start[rtp_content.item_Index] = time(NULL);	// todo: replay-mode
					    rtp_content.item_Artist[rtp_content.item_Index] = NULL;
					    }
					rtp_content.item_New = (!rtp_content.item_New) ? true : false;
	    				if (rtp_content.item_Index >= 0)
					    asprintf(&rtp_content.item_Title[rtp_content.item_Index], "%s", rtrim(rtp_content.temptext));
					RT_PlusShow = RT_MsgShow = rtp_itoggle = true;
					}
				    }
    				break;
			case 4:		// Item-Artist	
				if ((mtext[10] & 0x08) > 0 && (RTP_TToggle & 0x02) == 0x02) {
				    RTP_TToggle -= 0x02;
				    RT_Info = 2;
				    if (memcmp(RTP_Artist, temptext, RT_MEL-1) != 0 || (mtext[10] & 0x10) != RTP_ItemToggle) {
	    				memcpy(RTP_Artist, temptext, RT_MEL-1);
					if (RT_PlusShow && rtp_itime.Elapsed() > 1000)
					    rtp_idiffs = (int) rtp_itime.Elapsed()/1000;
					if (!rtp_content.item_New) {
					    RTP_Starttime = time(NULL);
					    rtp_itime.Set(0);
					    sprintf(RTP_Title, "---");
					    if (++rtp_content.item_Index >= MAX_RTPC)
						rtp_content.item_Index = 0;
					    rtp_content.item_Start[rtp_content.item_Index] = time(NULL);	// todo: replay-mode
					    rtp_content.item_Title[rtp_content.item_Index] = NULL;
					    }
					rtp_content.item_New = (!rtp_content.item_New) ? true : false;
	    				if (rtp_content.item_Index >= 0)
					    asprintf(&rtp_content.item_Artist[rtp_content.item_Index], "%s", rtrim(rtp_content.temptext));
					RT_PlusShow = RT_MsgShow = rtp_itoggle = true;
					}
				    }
				break;
			case 12:	// Info_News
    				asprintf(&rtp_content.info_News, "%s", rtrim(rtp_content.temptext));
				break;
			case 13:	// Info_NewsLocal
				asprintf(&rtp_content.info_NewsLocal, "%s", rtrim(rtp_content.temptext));
				break;
			case 14:	// Info_Stockmarket
	    			if (++rtp_content.info_StockIndex >= MAX_RTPC)
					    rtp_content.info_StockIndex = 0;
				asprintf(&rtp_content.info_Stock[rtp_content.info_StockIndex], "%s", rtrim(rtp_content.temptext));
				break;
			case 15:	// Info_Sport
	    			if (++rtp_content.info_SportIndex >= MAX_RTPC)
					    rtp_content.info_SportIndex = 0;
				asprintf(&rtp_content.info_Sport[rtp_content.info_SportIndex], "%s", rtrim(rtp_content.temptext));
				break;
			case 16:	// Info_Lottery
	    			if (++rtp_content.info_LotteryIndex >= MAX_RTPC)
					    rtp_content.info_LotteryIndex = 0;
				asprintf(&rtp_content.info_Lottery[rtp_content.info_LotteryIndex], "%s", rtrim(rtp_content.temptext));
				break;
			case 24:	// Info_DateTime
				asprintf(&rtp_content.info_DateTime, "%s", rtrim(rtp_content.temptext));
				break;
			case 25:	// Info_Weather
	    			if (++rtp_content.info_WeatherIndex >= MAX_RTPC)
					    rtp_content.info_WeatherIndex = 0;
				asprintf(&rtp_content.info_Weather[rtp_content.info_WeatherIndex], "%s", rtrim(rtp_content.temptext));
				break;
			case 26:	// Info_Traffic
				asprintf(&rtp_content.info_Traffic, "%s", rtrim(rtp_content.temptext));
				break;
			case 27:	// Info_Alarm
				asprintf(&rtp_content.info_Alarm, "%s", rtrim(rtp_content.temptext));
				break;
			case 28:	// Info_Advert
				asprintf(&rtp_content.info_Advert, "%s", rtrim(rtp_content.temptext));
				break;
			case 29:	// Info_Url
				asprintf(&rtp_content.info_Url, "%s", rtrim(rtp_content.temptext));
				break;
			case 30:	// Info_Other
	    			if (++rtp_content.info_OtherIndex >= MAX_RTPC)
					    rtp_content.info_OtherIndex = 0;
				asprintf(&rtp_content.info_Other[rtp_content.info_OtherIndex], "%s", rtrim(rtp_content.temptext));
				break;
			case 31:	// Programme_Stationname.Long
				asprintf(&rtp_content.prog_Station, "%s", rtrim(rtp_content.temptext));
				break;
			case 32:	// Programme_Now
				asprintf(&rtp_content.prog_Now, "%s", rtrim(rtp_content.temptext));
				break;
			case 33:	// Programme_Next
				asprintf(&rtp_content.prog_Next, "%s", rtrim(rtp_content.temptext));
				break;
			case 34:	// Programme_Part
				asprintf(&rtp_content.prog_Part, "%s", rtrim(rtp_content.temptext));
				break;
			case 35:	// Programme_Host
				asprintf(&rtp_content.prog_Host, "%s", rtrim(rtp_content.temptext));
				break;
			case 36:	// Programme_EditorialStaff
				asprintf(&rtp_content.prog_EditStaff, "%s", rtrim(rtp_content.temptext));
				break;
			case 38:	// Programme_Homepage
				asprintf(&rtp_content.prog_Homepage, "%s", rtrim(rtp_content.temptext));
				break;
			case 39:	// Phone_Hotline
				asprintf(&rtp_content.phone_Hotline, "%s", rtrim(rtp_content.temptext));
				break;
			case 40:	// Phone_Studio
				asprintf(&rtp_content.phone_Studio, "%s", rtrim(rtp_content.temptext));
				break;
			case 44:	// Email_Hotline
				asprintf(&rtp_content.email_Hotline, "%s", rtrim(rtp_content.temptext));
				break;
			case 45:	// Email_Studio
				asprintf(&rtp_content.email_Studio, "%s", rtrim(rtp_content.temptext));
				break;
			}
		    }
		}

	    // Title-end @ no Item-Running'
	    if ((mtext[10] & 0x08) == 0) {
		sprintf(RTP_Title, "---");
		sprintf(RTP_Artist, "---");
		if (RT_PlusShow) {
		    RT_PlusShow = false;
		    rtp_itoggle = true;
		    rtp_idiffs = (int) rtp_itime.Elapsed()/1000;
		    RTP_Starttime = time(NULL);
		    }
		RT_MsgShow = (RT_Info > 0);
		rtp_content.item_New = false;
		}

	    if (rtp_itoggle) {
		if (S_Verbose >= 1) {
		    struct tm tm_store;
		    struct tm *ts = localtime_r(&RTP_Starttime, &tm_store);
		    if (rtp_idiffs > 0)
			printf("  StartTime : %02d:%02d:%02d  (last Title elapsed = %d s)\n",
			       ts->tm_hour, ts->tm_min, ts->tm_sec, rtp_idiffs);
		    else
			printf("  StartTime : %02d:%02d:%02d\n", ts->tm_hour, ts->tm_min, ts->tm_sec);
	    	    printf("  RTp-Title : %s\n  RTp-Artist: %s\n", RTP_Title, RTP_Artist);
		    }
	        RTP_ItemToggle = mtext[10] & 0x10;
		rtp_itoggle = false;
		rtp_idiffs = 0;
		RadioStatusMsg();
		AudioRecorderService();
    		}
	    
	    RTP_TToggle = 0;
	    }
	}		

    else {
	if (S_Verbose >= 1)
	    printf("RDS-Error: [RTDecode] Length not correct !\n");
	}
}

void cRadioAudio::RDS_PsPtynDecode(bool ptyn, unsigned char *mtext, int len)
{
    if (len < 16) return;

    // decode Text
    for (int i = 8; i <= 15; i++) {
        if (mtext[i] <= 0xfe) {
	    // additional rds-character, see RBDS-Standard, Annex E
	    if (!ptyn)
        	RDS_PSText[RDS_PSIndex][i-8] = (mtext[i] >= 0x80) ? rds_addchar[mtext[i]-0x80] : mtext[i];
	    else
		RDS_PTYN[i-8] = (mtext[i] >= 0x80) ? rds_addchar[mtext[i]-0x80] : mtext[i];
	    }
	}
    
    if (S_Verbose >= 1) {
	if (!ptyn)
	    printf("RDS-PS  No= %d, Content[%d]= '%s'\n", mtext[7], RDS_PSIndex, RDS_PSText[RDS_PSIndex]);
	else
	    printf("RDS-PTYN  No= %d, Content= '%s'\n", mtext[7], RDS_PTYN);
	}

    if (!ptyn) {
	RDS_PSIndex += 1; if (RDS_PSIndex >= 12) RDS_PSIndex = 0;
	RT_MsgShow = RDS_PSShow = true;
	}
}

void cRadioAudio::RadioStatusMsg(void)
{
    /* announce text/items for lcdproc & other */

    if (!RT_MsgShow || S_RtMsgItems <= 0)
	return;
	
    if (S_RtMsgItems >= 2) {
	char temp[100];
        int ind = (RT_Index == 0) ? S_RtOsdRows - 1 : RT_Index - 1;
	strcpy(temp, RT_Text[ind]);
	cStatus::MsgOsdTextItem(rtrim(temp), false);
	}

    if ((S_RtMsgItems == 1 || S_RtMsgItems >= 3) && ((S_RtOsdTags == 1 && RT_PlusShow) || S_RtOsdTags >= 2)) {
	struct tm tm_store;
        struct tm *ts = localtime_r(&RTP_Starttime, &tm_store);
        cStatus::MsgOsdProgramme(mktime(ts), RTP_Title, RTP_Artist, 0, NULL, NULL);
	}
}

void cRadioAudio::AudioRecorderService(void)
{
    /* check plugin audiorecorder service */
    ARec_Receive = ARec_Record = false;

    if (!RT_PlusShow || RT_Replay)
	return;

    Audiorecorder_StatusRtpChannel_v1_0	arec_service;
    cPlugin *p;

    arec_service.channel = chan;
    p = cPluginManager::CallFirstService("Audiorecorder-StatusRtpChannel-v1.0", &arec_service);
    if (p) {
	ARec_Receive = (arec_service.status >= 2);
	ARec_Record = (arec_service.status == 3);
	}
}

// add <names> of DVB Radio Slides Specification 1.0, 20061228
#define qdarlog 0
void cRadioAudio::QDarDecode(unsigned char *mtext, int len)
{
    static uint splfd = 0, spmax = 0, index = 0;
    static uint afiles, slidenumr, slideelem, filemax, fileoffp;
    static int filetype, fileoffb;
    static bool slideshow = false, slidesave = false, slidecan = false, slidedel = false, start = false;    
    static uchar daten[65536];	// mpegs-stills defined <= 50kB
    FILE *fd;
        
    // byte 1+2 = ADD (10bit SiteAdress + 6bit EncoderAdress)
    // byte 3   = SQC (Sequence Counter 0x00 = not used)
    // byte 4   = MFL (Message Field Length), 
    if (len >= mtext[4]+7) {	// check complete length
        // byte 5   = MEC (0xda for QDAr)
        // byte 6   = MEL
        if (mtext[6] == 0 || mtext[6] > mtext[4]-2) {
		if (S_Verbose >= 1)
		    printf("QDar-Error: Length = 0 or not correct !\n");
		return;
		}
	// byte 7+8   = Service-ID zugehöriger Datenkanal
	// byte 9-11  = Nummer aktuelles Paket, <PNR>
	uint plfd = mtext[11] + mtext[10]*256 + mtext[9]*65536;
	// byte 12-14 = Anzahl Pakete, <NOP>
	uint pmax = mtext[14] + mtext[13]*256 + mtext[12]*65536;

	// byte 15+16 = QDar-Kennung = Header, <QDAr-STA>
        if (mtext[15] == 0x40 && mtext[16] == 0xda) {		// first
	    // byte 17+18 = Anzahl Dateien im Archiv, <NOI>
	    afiles = mtext[18] + mtext[17]*256;
	    // byte 19+20 = Slide-Nummer, <QDAr-ID>
	    slidenumr = mtext[20] + mtext[19]*256;
	    // byte 21+22 = Element-Nummer im Slide, <INR>
	    slideelem = mtext[22] + mtext[21]*256;
	    // byte 23 	  = Slide-Steuerbyte, <Cntrl-Byte>: bit0 = Anzeige, bit1 = Speichern, bit2 = DarfAnzeige bei Senderwechsel, bit3 = Löschen
	    slideshow = mtext[23] & 0x01;
	    slidesave = mtext[23] & 0x02;
	    slidecan = mtext[23] & 0x04;
	    slidedel  = mtext[23] & 0x08;
	    // byte 24 	  = Dateiart, <Item-Type>: 0=unbekannt/1=MPEG-Still/2=Definition
	    filetype = mtext[24];
	    if (filetype != 1 && filetype != 2) {
		if (S_Verbose >= 1)
		    printf("QDar-Error: Filetype unknown !\n");
		return;
		}
	    // byte 25-28 = Dateilänge, <Item-Length>
	    filemax  = mtext[28] + mtext[27]*256 + mtext[26]*65536 + mtext[25]*65536*256;
	    if (filemax >= 65536) {
		if (S_Verbose >= 1)
		    printf("QDar-Error: Filesize will be too big !\n");
		return;
		}	    
	    // byte 29-31 = Dateioffset Paketnr, <Rfu>
	    fileoffp  = mtext[31] + mtext[30]*256 + mtext[29]*65536;
	    // byte 32 = Dateioffset Bytenr, <Rfu>
	    fileoffb  = mtext[32];
	    if (S_Verbose >= 2 || qdarlog == 1)
		printf("QDar-Header: afiles= %d\n             slidenumr= %d, slideelem= %d\n             slideshow= %d, -save= %d, -canschow= %d, -delete= %d\n             filetype= %d, filemax= %d\n             fileoffp= %d, fileoffb= %d\n",
    		       afiles, slidenumr, slideelem, slideshow, slidesave, slidecan, slidedel, filetype, filemax, fileoffp, fileoffb);

	    if (fileoffp == 0) {	// First
		if (S_Verbose >= 2 || qdarlog == 1)
	    	    printf("QDar-Start@0 ...\n");
		start = true;
		index = 0;
    		for (int i=fileoffb; i < len-2; i++) {
	    	    if (index < filemax)
    			daten[index++] = mtext[i];
		    else
			start = false;
    		    }
		}
	    splfd = plfd;
	    }
	else if (plfd < pmax && plfd == splfd+1) {		// Between
	    splfd = plfd;
	    if (!start && fileoffp == plfd) {	// Data start, <with Rfu no more necesssary>
		if (S_Verbose >= 2 || qdarlog == 1)
	    	    printf("QDar-Start@%d ...\n", fileoffp);
		start = true;
		index = 0;
		}
	    else
		fileoffb = 15;
	    if (start) {
    		for (int i=fileoffb; i < len-2; i++) {
	    	    if (index < filemax)
    			daten[index++] = mtext[i];
		    else
			start = false;
		    }
		}
	    }
	else if (plfd == pmax && plfd == splfd+1) {		// Last
	    fileoffb = 15;
    	    if (start) {
		for (int i=fileoffb; i < len-4; i++) {
	    	    if (index <= filemax)
	    		daten[index++] = mtext[i];
		    else {
			start = false;
			return;
			}
		    }
		if (S_Verbose >= 2 || qdarlog == 1)
    		    printf("... QDar-End (%d bytes)\n", index);
		}

	    if (filemax > 0) {		// nothing todo, if 0 byte file
		// crc-check with bytes 'len-4/3'
		unsigned short crc16 = crc16_ccitt(daten, filemax, false);
		if (crc16 != (mtext[len-4]<<8)+mtext[len-3]) {
		    if (S_Verbose >= 1)
			printf("QDar-Error: wrong CRC # calc = %04x <> transmit = %02x%02x\n", crc16, mtext[len-4], mtext[len-3]);
		    start = false;
		    return;
		    }
		}
	    
    	    // show & save file ?
	    if (index == filemax) {
	        if (slideshow || (slidecan && QDar_Show == -1)) {
		    if (filetype == 1) {	// show only mpeg-still
			char *filepath;
			asprintf(&filepath, "%s/%s", DataDir, "QDar_show.mpg");
			if ((fd = fopen(filepath, "wb")) != NULL) {
			    fwrite(daten, 1, filemax, fd);
			    //fflush(fd);		// for test in replaymode
			    fclose(fd);
			    QDar_Show = 1;
			    if (qdarlog == 1)
    				printf("QDar-File: ready for displaying :-)\n");
			    }
			else
			    esyslog("ERROR vdr-radio: writing imagefile failed '%s'", filepath);
			free(filepath);
			}
		    }
		if (slidesave || slidedel) {
		    char *filepath;
		    (filetype == 2) ? asprintf(&filepath, "%s/QDar_%d.def", DataDir, slidenumr)
				    : asprintf(&filepath, "%s/QDar_%d.mpg", DataDir, slidenumr);
		    if ((fd = fopen(filepath, "wb")) != NULL) {
			fwrite(daten, 1, filemax, fd);
			fclose(fd);
			if (qdarlog == 1)
    			    printf("QDar-File: saving '%s'\n", filepath);
    			// mark mpeg-stills for existent
			if (filetype == 1) {
			    if (slidenumr == 0) {
			    	QDar_Flags[0][0] = !slidedel;	//true;
				RT_Info = (RT_Info > 0) ? : 0;	// open RadioTextOsd for ArchivTip
				}
			    else {
				int islide = (int) floor(slidenumr/1000);
			    	for (int i = 3; i > 0; i--) {
    			    	    if (fmod(slidenumr, pow(10, i)) == 0) {
				        QDar_Flags[islide][3-i] = !slidedel;	//true;
					break;
					}
				    }
			     	}
			    }
			}
		    else
		        esyslog("ERROR vdr-radio: writing image/data-file failed '%s'", filepath);
		    free(filepath);
		    }
		}
	    start = false;
	    splfd = spmax = 0;
	    }
	else {
	    start = false;
	    splfd = spmax = 0;
	    }
	}

    else {
	start = false;
        splfd = spmax = 0;
	if (S_Verbose >= 1)
	    printf("RDS-Error: [QDar] Length not correct !\n");
	}
}	

void cRadioAudio::EnableRadioTextProcessing(const char *Titel, bool replay)
{
    asprintf(&RT_Titel, "%s", Titel);
    RT_Replay = replay;
    ARec_Receive = ARec_Record = false;

    first_packets = 0;
    enabled = true;
    imagedelay = 0;

    // Radiotext init
    if (S_RtFunc >= 1) {
	RT_MsgShow = RT_PlusShow = false;
	RT_ReOpen = true;
	RT_OsdTO = false;
	RT_Index = RT_PTY = RTP_TToggle = 0;
	RTP_ItemToggle = 1;
	for (int i = 0; i < 5; i++)
	    memset(RT_Text[i], 0x20, RT_MEL-1);
	sprintf(RTP_Title, "---");
	sprintf(RTP_Artist, "---");
	RTP_Starttime = time(NULL);
	//
	RDS_PSShow = false;
        RDS_PSIndex = 0;
	for (int i = 0; i < 12; i++)
	    memset(RDS_PSText[i], 0x20, 8);
	}
    // ...Memory
    rtp_content.start = time(NULL);
    rtp_content.item_New = false;
    rtp_content.rt_Index = -1;
    rtp_content.item_Index = -1;
    rtp_content.info_StockIndex = -1;
    rtp_content.info_SportIndex = -1;
    rtp_content.info_LotteryIndex = -1;
    rtp_content.info_WeatherIndex = -1;
    rtp_content.info_OtherIndex = -1;
    for (int i = 0; i < MAX_RTPC; i++) {
	rtp_content.radiotext[i] = NULL;
	rtp_content.radiotext[MAX_RTPC+i] = NULL;
	rtp_content.item_Title[i] = NULL;
	rtp_content.item_Artist[i] = NULL;
	rtp_content.info_Stock[i] = NULL;
	rtp_content.info_Sport[i] = NULL;
	rtp_content.info_Lottery[i] = NULL;
	rtp_content.info_Weather[i] = NULL;
	rtp_content.info_Other[i] = NULL;
	}
    rtp_content.info_News = NULL;
    rtp_content.info_NewsLocal = NULL;
    rtp_content.info_DateTime = NULL;
    rtp_content.info_Traffic = NULL;
    rtp_content.info_Alarm = NULL;
    rtp_content.info_Advert = NULL;
    rtp_content.info_Url = NULL;
    rtp_content.prog_Station = NULL;
    rtp_content.prog_Now = NULL;
    rtp_content.prog_Next = NULL;
    rtp_content.prog_Part = NULL;
    rtp_content.prog_Host = NULL;
    rtp_content.prog_EditStaff = NULL;
    rtp_content.prog_Homepage = NULL;
    rtp_content.phone_Hotline = NULL;
    rtp_content.phone_Studio = NULL;
    rtp_content.email_Hotline = NULL;
    rtp_content.email_Studio = NULL;

    // QDAr init
    QDar_Show = QDar_Archiv = -1;
    for (int i = 0; i < 10; i++) {
	for (int ii = 0; ii < 4; ii++)
    	    QDar_Flags[i][ii] = false;
	}

    if (S_RtFunc < 1) return;
    
    // RDS-Receiver for seperate Data-PIDs, only Livemode, hardcoded Astra_19E + Hotbird 13E
    int pid = 0;
    if (!replay) {
        switch (chan->Tid()) {
	    case 1113:  switch (pid = chan->Apid(0)) {	// Astra_19.2E - 12633 GHz
	    		    case 0x161: pid = 0x229;	//  radio top40
				        break;
			    case 0x400:			//  Hitradio FFH
			    case 0x406:			//  planet radio
			    case 0x40c:	pid += 1;	//  harmony.ffm
					break;
			    default:	return;
			    }
			break;
	    case 1115:	switch (pid = chan->Apid(0)) {	// Astra_19.2E - 12663 GHz
			    case 0x1bA:	pid = 0x21e;	//  TruckRadio, only NaviData(0xbf) seen
					break;
			    default:	return;
			    }
			break;
	    /* unkown data, no RDS
	    case 15500:	switch (pid = chan->Apid(0)) {	// Hotbird_13E - 11604 GHz
			    case 0x7da:	pid = 0x1ff0;	//  DW01
					break;
			    case 0x7e4:			//  DW02
			    case 0x7f8:			//  DW04
			    case 0x802:			//  DW05
			    case 0x80c:			//  DW06
			    case 0x820:	pid = 0x080;	//  DW08
					break;
			    case 0x82a:			//  DW09
			    case 0x83e:			//  PF1
			    case 0x848:			//  PF2
			    case 0x852:			//  PF3
			    case 0x85c:			//  PF4
			    case 0x866:	pid = 0x081;	//  DW-M
					break;
			    default:	return;
			    }
			break;
	    */
	    case 5300:	switch (pid = chan->Apid(0)) {	// Hotbird_13E - 11747 GHz, no Radiotext @ moment, only TMC + MECs 25/26
			    case 0xdc3:			//  Radio 1
			    case 0xdd3:			//  Radio 3
			    case 0xddb:			//  Radio 5
			    case 0xde3:			//  Radio Exterior
			    case 0xdeb:	pid += 1;	//  Radio 4
					break;
			    default:	return;
			    }
			break;
	    default:	return;
	    }
	RDSReceiver = new cRDSReceiver(pid);
	rdsdevice = cDevice::ActualDevice();
	rdsdevice->AttachReceiver(RDSReceiver);
	}
}

void cRadioAudio::DisableRadioTextProcessing()
{
    RT_Replay = enabled = false;

    // Radiotext & QDAr
    RT_Info = -1;
    RT_ReOpen = false;
    QDar_Show = QDar_Archiv = -1;

    if (RadioTextOsd != NULL)
	RadioTextOsd->Hide();
    
    if (RDSReceiver != NULL) {
	rdsdevice->Detach(RDSReceiver);
	delete RDSReceiver;
	RDSReceiver = NULL;
	rdsdevice = NULL;
	}
}


// --- cRadioTextOsd ------------------------------------------------------

cBitmap cRadioTextOsd::rds(rds_xpm);
cBitmap cRadioTextOsd::arec(arec_xpm);
cBitmap cRadioTextOsd::rass(rass_xpm);
cBitmap cRadioTextOsd::index(index_xpm);
cBitmap cRadioTextOsd::marker(marker_xpm);
cBitmap cRadioTextOsd::page1(page1_xpm);
cBitmap cRadioTextOsd::pages2(pages2_xpm);
cBitmap cRadioTextOsd::pages3(pages3_xpm);
cBitmap cRadioTextOsd::pages4(pages4_xpm);
cBitmap cRadioTextOsd::no0(no0_xpm);
cBitmap cRadioTextOsd::no1(no1_xpm);
cBitmap cRadioTextOsd::no2(no2_xpm);
cBitmap cRadioTextOsd::no3(no3_xpm);
cBitmap cRadioTextOsd::no4(no4_xpm);
cBitmap cRadioTextOsd::no5(no5_xpm);
cBitmap cRadioTextOsd::no6(no6_xpm);
cBitmap cRadioTextOsd::no7(no7_xpm);
cBitmap cRadioTextOsd::no8(no8_xpm);
cBitmap cRadioTextOsd::no9(no9_xpm);

cRadioTextOsd::cRadioTextOsd()
{
    RadioTextOsd = this;
    osd = NULL;
    qosd = NULL;
    rtclosed = qdarclosed = false;
    RT_ReOpen = false;
}

cRadioTextOsd::~cRadioTextOsd()
{
    if (QDar_Archiv >= 0) {
	if (!RT_Replay)
    	    QDar_Archiv = QDarImage(-1, -1);
	else {
	    QDar_Archiv = -1;
	    RadioAudio->SetBackgroundImage(ReplayFile);
	    }
	}
	    	    
    if (osd != NULL) 
	delete osd;
    if (qosd != NULL) 
	delete qosd;
    RadioTextOsd = NULL;
    RT_ReOpen = !RT_OsdTO;

    cRemote::Put(LastKey);
}

void cRadioTextOsd::Show(void)
{
    LastKey = kNone;
    RT_OsdTO = false;
#ifndef VDRP_CLOSEMENU
    osdtimer.Set();
#endif

    ftitel = cFont::GetFont(fontOsd);
    ftext = cFont::GetFont(fontSml);
    fheight = ftext->Height() + 4;
    int rowoffset = (S_RtOsdTitle == 1) ? 1 : 0;
    bheight = (S_RtOsdTags >=1 ) ? fheight * (S_RtOsdRows+rowoffset+2) : fheight * (S_RtOsdRows+rowoffset);
    (S_RtOsdTitle == 1) ? bheight += 20 : bheight += 12;

    asprintf(&RTp_Titel, "%s - %s", tr("RTplus"), RT_Titel);

    if (S_RtDispl >= 1 && (QDar_Show == -1 || S_QDarText >= 2)) {
	RT_MsgShow = (RT_Info >= 1);
	ShowText();
	}
}

void cRadioTextOsd::Hide(void)
{
    RTOsdClose();
    QDarOsdClose();
}

void cRadioTextOsd::RTOsdClose(void)
{
    if (osd != NULL) {
	delete osd;
	osd = NULL;
	}
}

void cRadioTextOsd::ShowText(void)
{
    char stext[3][100];
    int yoff = 8, ii = 0;

    if (!osd && !qosd && !Skins.IsOpen() && !cOsd::IsOpen()) {
	if (S_RtOsdPos == 1)
	    osd = cOsdProvider::NewOsd(Setup.OSDLeft, Setup.OSDTop+Setup.OSDHeight-bheight);
   	else
	    osd = cOsdProvider::NewOsd(Setup.OSDLeft, Setup.OSDTop);
	tArea Area = {0, 0, Setup.OSDWidth-1, bheight-1, 4};
	osd->SetAreas(&Area, 1);
	}

    if (osd) {
	uint32_t bcolor, fcolor;
	int skin = theme_skin();
	if (S_RtOsdTitle == 1) {
	    // Title
	    bcolor = (S_RtSkinColor > 0) ? radioSkin[skin].clrTitleBack : (0x00FFFFFF | S_RtBgTra<<24) & rt_color[S_RtBgCol];
	    fcolor = (S_RtSkinColor > 0) ? radioSkin[skin].clrTitleText : rt_color[S_RtFgCol];
    	    osd->DrawRectangle(0, 0, Setup.OSDWidth-1, ftitel->Height()+9, bcolor);
	    sprintf(stext[0], RT_PTY == 0 ? "%s - %s %s%s" : "%s - %s (%s)%s",
	    	      RT_Titel, tr("Radiotext"), RT_PTY == 0 ? RDS_PTYN : ptynr2string(RT_PTY), RT_MsgShow ? ":" : tr("  [waiting ...]"));
    	    osd->DrawText(4, 5, stext[0], fcolor, clrTransparent, ftitel, Setup.OSDWidth-4, ftitel->Height());
	    yoff = 17;
	    ii = 1;
	    // RDS-, RaSS-, ARec-Symbol
	    if (QDar_Flags[0][0]) {
        	osd->DrawBitmap(Setup.OSDWidth-56, 8, rass, bcolor, fcolor);
		if (ARec_Record)
		    osd->DrawBitmap(Setup.OSDWidth-112, 8, arec, bcolor, 0xFFFC1414);	// FG=Red
		}
	    else {
	        osd->DrawBitmap(Setup.OSDWidth-84, 8, rds, bcolor, fcolor);
		if (ARec_Record)
		    osd->DrawBitmap(Setup.OSDWidth-140, 8, arec, bcolor, 0xFFFC1414);	// FG=Red
		}
	    }
	// Body
	bcolor = (S_RtSkinColor > 0) ? radioSkin[skin].clrBack : (0x00FFFFFF | S_RtBgTra<<24) & rt_color[S_RtBgCol];
	fcolor = (S_RtSkinColor > 0) ? radioSkin[skin].clrText : rt_color[S_RtFgCol];
    	osd->DrawRectangle(0, ftitel->Height()+10, Setup.OSDWidth-1, bheight-1, bcolor);
	if (RT_MsgShow) {
    	    // RT-Text roundloop
	    int ind = (RT_Index == 0) ? S_RtOsdRows - 1 : RT_Index - 1;
	    if (S_RtOsdLoop == 1) {	// latest bottom
	        for (int i = ind+1; i < S_RtOsdRows; i++)
		    osd->DrawText(5, yoff+fheight*(ii++), RT_Text[i], fcolor, clrTransparent, ftext, Setup.OSDWidth-4, ftext->Height());
    		for (int i = 0; i <= ind; i++)
		    osd->DrawText(5, yoff+fheight*(ii++), RT_Text[i], fcolor, clrTransparent, ftext, Setup.OSDWidth-4, ftext->Height());
		}
	    else {			// latest top
	        for (int i = ind; i >= 0; i--)
		    osd->DrawText(5, yoff+fheight*(ii++), RT_Text[i], fcolor, clrTransparent, ftext, Setup.OSDWidth-4, ftext->Height());
    		for (int i = S_RtOsdRows-1; i > ind; i--)
   		    osd->DrawText(5, yoff+fheight*(ii++), RT_Text[i], fcolor, clrTransparent, ftext, Setup.OSDWidth-4, ftext->Height());
		}
	    // + RT-Plus or PS-Text = 2 rows
	    if ((S_RtOsdTags == 1 && RT_PlusShow) || S_RtOsdTags >= 2) {
		if (!RDS_PSShow || !strstr(RTP_Title, "---") || !strstr(RTP_Artist, "---")) {
		    sprintf(stext[1], "> %s  %s", tr("Title  :"), RTP_Title);
		    sprintf(stext[2], "> %s  %s", tr("Artist :"), RTP_Artist);
		    osd->DrawText(4, 6+yoff+fheight*(ii++), stext[1], fcolor, clrTransparent, ftext, Setup.OSDWidth-4, ftext->Height());
   		    osd->DrawText(4, 3+yoff+fheight*(ii++), stext[2], fcolor, clrTransparent, ftext, Setup.OSDWidth-4, ftext->Height());
		    }
   		else {
		    char *temp = (char*)"";
		    int ind = (RDS_PSIndex == 0) ? 11 : RDS_PSIndex - 1;
	    	    for (int i = ind+1; i < 12; i++)
			asprintf(&temp, "%s%s ", temp, RDS_PSText[i]);
		    for (int i = 0; i <= ind; i++)
			asprintf(&temp, "%s%s ", temp, RDS_PSText[i]);
		    snprintf(stext[1], 6*9, "%s", temp);
		    snprintf(stext[2], 6*9, "%s", temp+(6*9));
		    free(temp);
    		    osd->DrawText(6, 6+yoff+fheight*ii, "[", fcolor, clrTransparent, ftext, 12, ftext->Height());
    		    osd->DrawText(Setup.OSDWidth-12, 6+yoff+fheight*ii, "]", fcolor, clrTransparent, ftext, Setup.OSDWidth-6, ftext->Height());
		    osd->DrawText(16, 6+yoff+fheight*(ii++), stext[1], fcolor, clrTransparent, ftext, Setup.OSDWidth-16, ftext->Height(), taCenter);
    		    osd->DrawText(6, 3+yoff+fheight*ii, "[", fcolor, clrTransparent, ftext, 12, ftext->Height());
    		    osd->DrawText(Setup.OSDWidth-12, 3+yoff+fheight*ii, "]", fcolor, clrTransparent, ftext, Setup.OSDWidth-6, ftext->Height());
   		    osd->DrawText(16, 3+yoff+fheight*(ii++), stext[2], fcolor, clrTransparent, ftext, Setup.OSDWidth-16, ftext->Height(), taCenter);
		    }	
		}
	    }
	osd->Flush();
	}

    RT_MsgShow = false;    
}

int cRadioTextOsd::QDarImage(int QArchiv, int QKey)
{
    int i;

    if (QKey >= 0) {
	if (QArchiv == 0)
	    (QDar_Flags[QKey][0]) ? QArchiv = QKey * 1000 : QArchiv = 0;
	else if (QArchiv > 0) {
	    if (floor(QArchiv/1000) == QKey) {
		for (i = 3; i > 0; i--) {
    		    if (fmod(QArchiv, pow(10, i)) == 0)
			break;
		    }
		(i > 1) ? QArchiv += QKey * (int) pow(10, --i) : QArchiv = QKey * 1000;
		(QDar_Flags[QKey][3-i]) ? : QArchiv = QKey * 1000;
		}
	    else
		(QDar_Flags[QKey][0]) ? QArchiv = QKey * 1000 : QArchiv = 0;
	    }
	}
  
    // show mpeg-still
    char *image;
    if (QArchiv >= 0)
        asprintf(&image, "%s/QDar_%d.mpg", DataDir, QArchiv);
    else
	asprintf(&image, "%s/QDar_show.mpg", DataDir);
    RadioAudio->SetBackgroundImage(image);
    free(image);
    
    return QArchiv;
}

void cRadioTextOsd::QDarOsd(void)
{
    if (!qosd && !osd && !Skins.IsOpen() && !cOsd::IsOpen()) {
        qosd = cOsdProvider::NewOsd(Setup.OSDLeft, Setup.OSDTop+Setup.OSDHeight - (29+240-6+36));
	tArea Area = {0, 0, 97, 29+240+5, 4};
	qosd->SetAreas(&Area, 1);
	}

    if (qosd) {
	uint32_t bcolor, fcolor;
	int skin = theme_skin();
	// Logo
	bcolor = radioSkin[skin].clrTitleBack;
	fcolor = radioSkin[skin].clrTitleText;
	qosd->DrawRectangle(0, 1, 97, 29, bcolor);
    	qosd->DrawBitmap(23, 4, rass, bcolor, fcolor);
	// Body
	bcolor = radioSkin[skin].clrBack;
	fcolor = radioSkin[skin].clrText;
	int offs = 29 + 2;
	qosd->DrawRectangle(0, offs, 97, 29+240+5, bcolor);
	// Keys+Index
	offs += 4;
    	qosd->DrawBitmap(4,     offs, no0, bcolor, fcolor);
    	qosd->DrawBitmap(4,  24+offs, no1, bcolor, fcolor);
    	qosd->DrawBitmap(4,  48+offs, no2, bcolor, fcolor);
    	qosd->DrawBitmap(4,  72+offs, no3, bcolor, fcolor);
    	qosd->DrawBitmap(4,  96+offs, no4, bcolor, fcolor);
    	qosd->DrawBitmap(4, 120+offs, no5, bcolor, fcolor);
    	qosd->DrawBitmap(4, 144+offs, no6, bcolor, fcolor);
    	qosd->DrawBitmap(4, 168+offs, no7, bcolor, fcolor);
    	qosd->DrawBitmap(4, 192+offs, no8, bcolor, fcolor);
    	qosd->DrawBitmap(4, 216+offs, no9, bcolor, fcolor);
    	qosd->DrawBitmap(44,    offs, index, bcolor, fcolor);
	// Content
	bool mark = false;
	for (int i = 1; i <= 9; i++) {
	    // Pages
	    if (QDar_Flags[i][0] && QDar_Flags[i][1] && QDar_Flags[i][2])
    		qosd->DrawBitmap(44, (i*24)+offs, pages3, bcolor, fcolor);
	    else if (QDar_Flags[i][0] && QDar_Flags[i][1])
    		qosd->DrawBitmap(44, (i*24)+offs, pages2, bcolor, fcolor);
	    else if (QDar_Flags[i][0])
    		qosd->DrawBitmap(44, (i*24)+offs, page1, bcolor, fcolor);
	    // Marker
	    if (floor(QDar_Archiv/1000) == i) {
    		qosd->DrawBitmap(26, (i*24)+offs, marker, bcolor, fcolor);
		mark = true;
		}
	    }
	if (!mark)
    	    qosd->DrawBitmap(26, offs, marker, bcolor, fcolor);
	qosd->Flush();
	}
}

void cRadioTextOsd::QDarOsdTip(void)
{
    int fh = ftext->Height();

    if (!qosd && !osd && !Skins.IsOpen() && !cOsd::IsOpen()) {
        qosd = cOsdProvider::NewOsd(Setup.OSDLeft, Setup.OSDTop+Setup.OSDHeight - (29+(2*fh)-6+36));
	tArea Area = {0, 0, 97, 29+(2*fh)+5, 4};
	qosd->SetAreas(&Area, 1);
	}

    if (qosd) {
	uint32_t bcolor, fcolor;
	int skin = theme_skin();
	// Title
	bcolor = radioSkin[skin].clrTitleBack;
	fcolor = radioSkin[skin].clrTitleText;
	qosd->DrawRectangle(0, 0, 97, 29, bcolor);
    	qosd->DrawBitmap(23, 4, rass, bcolor, fcolor);
	// Body
	bcolor = radioSkin[skin].clrBack;
	fcolor = radioSkin[skin].clrText;
	qosd->DrawRectangle(0, 29+2, 97, 29+(2*fh)+5, bcolor);
	qosd->DrawText(5, 29+4, tr("Records"), fcolor, clrTransparent, ftext, 97, fh);
	qosd->DrawText(5, 29+fh+4, tr("with <0>"), fcolor, clrTransparent, ftext, 97, fh);
	qosd->Flush();
	}
}

void cRadioTextOsd::QDarOsdClose(void)
{
    if (qosd != NULL) {
	delete qosd;
	qosd = NULL;
	}
}

void cRadioTextOsd::rtp_print(void)
{
    struct tm tm_store;
    time_t t = time(NULL);
    printf("\n>>> RTplus-Memoryclasses @ %s", asctime(localtime_r(&t, &tm_store)));
    printf("    on '%s' since %s", RT_Titel, asctime(localtime_r(&rtp_content.start, &tm_store)));

    printf("--- Programme ---\n");
    if (rtp_content.prog_Station != NULL)   printf("     Station: %s\n", rtp_content.prog_Station);
    if (rtp_content.prog_Now != NULL)	    printf("         Now: %s\n", rtp_content.prog_Now);
    if (rtp_content.prog_Next != NULL)	    printf("        Next: %s\n", rtp_content.prog_Next);
    if (rtp_content.prog_Part != NULL)	    printf("        Part: %s\n", rtp_content.prog_Part);
    if (rtp_content.prog_Host != NULL)	    printf("        Host: %s\n", rtp_content.prog_Host);
    if (rtp_content.prog_EditStaff != NULL) printf("    Ed.Staff: %s\n", rtp_content.prog_EditStaff);
    if (rtp_content.prog_Homepage != NULL)  printf("    Homepage: %s\n", rtp_content.prog_Homepage);

    printf("--- Interactivity ---\n");
    if (rtp_content.phone_Hotline != NULL)  printf("    Phone-Hotline: %s\n", rtp_content.phone_Hotline);
    if (rtp_content.phone_Studio != NULL)   printf("     Phone-Studio: %s\n", rtp_content.phone_Studio);
    if (rtp_content.email_Hotline != NULL)  printf("    Email-Hotline: %s\n", rtp_content.email_Hotline);
    if (rtp_content.email_Studio != NULL)   printf("     Email-Studio: %s\n", rtp_content.email_Studio);

    printf("--- Info ---\n");
    if (rtp_content.info_News != NULL) 	    printf("         News: %s\n", rtp_content.info_News);
    if (rtp_content.info_NewsLocal != NULL) printf("    NewsLocal: %s\n", rtp_content.info_NewsLocal);
    if (rtp_content.info_DateTime != NULL)  printf("     DateTime: %s\n", rtp_content.info_DateTime);
    if (rtp_content.info_Traffic != NULL)   printf("      Traffic: %s\n", rtp_content.info_Traffic);
    if (rtp_content.info_Alarm != NULL)     printf("        Alarm: %s\n", rtp_content.info_Alarm);
    if (rtp_content.info_Advert != NULL)    printf("    Advertisg: %s\n", rtp_content.info_Advert);
    if (rtp_content.info_Url != NULL)       printf("          Url: %s\n", rtp_content.info_Url);
    // no sorting
    for (int i = 0; i < MAX_RTPC; i++)
	if (rtp_content.info_Stock[i] != NULL)   printf("      Stock[%02d]: %s\n", i, rtp_content.info_Stock[i]);
    for (int i = 0; i < MAX_RTPC; i++)
	if (rtp_content.info_Sport[i] != NULL)   printf("      Sport[%02d]: %s\n", i, rtp_content.info_Sport[i]);
    for (int i = 0; i < MAX_RTPC; i++)
	if (rtp_content.info_Lottery[i] != NULL) printf("    Lottery[%02d]: %s\n", i, rtp_content.info_Lottery[i]);
    for (int i = 0; i < MAX_RTPC; i++)
	if (rtp_content.info_Weather[i] != NULL) printf("    Weather[%02d]: %s\n", i, rtp_content.info_Weather[i]);
    for (int i = 0; i < MAX_RTPC; i++)
	if (rtp_content.info_Other[i] != NULL)   printf("      Other[%02d]: %s\n", i, rtp_content.info_Other[i]);
/*
    printf("--- Item-Playlist ---\n");
    // no sorting
    if (rtp_content.item_Index >= 0) {
	for (int i = 0; i < MAX_RTPC; i++) {
	    if (rtp_content.item_Title[i] != NULL && rtp_content.item_Artist[i] != NULL) {
		struct tm tm_store;
    		struct tm *ts = localtime_r(&rtp_content.item_Start[i], &tm_store);
		printf("    [%02d]  %02d:%02d  Title: %s | Artist: %s\n",
	    	       i, ts->tm_hour, ts->tm_min, rtp_content.item_Title[i], rtp_content.item_Artist[i]);
		}
	    }
	}

    printf("--- Last seen Radiotext ---\n");
    // no sorting
    if (rtp_content.rt_Index >= 0) {
	for (int i = 0; i < 2*MAX_RTPC; i++)
	    if (rtp_content.radiotext[i] != NULL) printf("    [%03d]  %s\n", i, rtp_content.radiotext[i]);
	}
*/
    printf("<<<\n");
}

#define rtplog 0
eOSState cRadioTextOsd::ProcessKey(eKeys Key)
{
    // RTplus Infolog
    if (rtplog == 1 && S_Verbose >= 1) {
	static int ct = 0;
	if (++ct >= 60) {
	    ct = 0;
    	    rtp_print();
	    }
	}

    // check end @ replay
    if (RT_Replay) {
	int rplayCur, rplayTot;
        cControl::Control()->GetIndex(rplayCur, rplayTot, false);
        if (rplayCur >= rplayTot-1) {
    	    Hide();
	    return osEnd;
	    }
	}

    // Timeout or no Info/QDAr
    if (RT_OsdTO || (RT_OsdTOTemp > 0) || (RT_Info < 0)) {
	Hide();
	return osEnd;
	}

    eOSState state = cOsdObject::ProcessKey(Key);
    if (state != osUnknown)  return state;

    // Key pressed ...
    if (Key != kNone && Key < k_Release) {
	if (osd) {				// Radiotext, -plus Osd
	    switch (Key) {
		case kBack:	RTOsdClose();
				rtclosed = true;
				//qdarclosed = false;
				break;
		case k0:	RTOsdClose();
				RTplus_Osd = true;  
    				cRemote::CallPlugin("radio");
				return osEnd;
		default:    	Hide();
				LastKey = (Key == kChanUp || Key == kChanDn) ? kNone : Key;
				return osEnd;
		}
	    }
	else if (qosd && QDar_Archiv >= 0) {	// QDar-Archiv Osd
	    switch (Key) {
		// back to Slideshow
		case kBack: 	if (!RT_Replay)
				    QDar_Archiv = QDarImage(-1, 0);
				else {
				    QDar_Archiv = -1;
				    RadioAudio->SetBackgroundImage(ReplayFile);
				    }
				QDarOsdClose();
				qdarclosed = rtclosed = false;
				break;
		// Archiv-Sides
		case k0:
		case k1: 
		case k2: 
		case k3: 
		case k4: 
		case k5: 
		case k6: 
		case k7: 
		case k8: 
		case k9:    	QDar_Archiv = QDarImage(QDar_Archiv, Key-k0);
				QDarOsd();
		      		break;
		default:    	Hide();
				LastKey = (Key == kChanUp || Key == kChanDn) ? kNone : Key;
				return osEnd;
		}
	    }
	else if (qosd && QDar_Archiv == -1) {	// QDar-Slideshow Osd
	    switch (Key) {
		// close
		case kBack: 	QDarOsdClose();
				qdarclosed = true;
				//rtclosed = false;
				break;
		// Archiv-Index
		case k0:	if (QDar_Flags[0][0]) {
				    QDarOsdClose();
				    QDar_Archiv = QDarImage(0, 0);
				    QDarOsd();
				    }
    				break;
		default:    	Hide();
				LastKey = (Key == kChanUp || Key == kChanDn) ? kNone : Key;
				return osEnd;
		}
	
	    }
	else {					// no RT && no QDar
	    Hide();
	    LastKey = (Key == kChanUp || Key == kChanDn) ? kNone : Key;
	    return osEnd;
	    }
	}
    // no Key pressed ...
#ifndef VDRP_CLOSEMENU
    else if (S_RtOsdTO > 0 && osdtimer.Elapsed()/1000/60 >= (uint)S_RtOsdTO) {
	RT_OsdTO = true;
	Hide();
	return osEnd;
	}
#endif
    else if (QDar_Archiv >= 0)
        QDarOsd();
    else if (RT_MsgShow && !rtclosed && (QDar_Show == -1 || S_QDarText >= 2 || qdarclosed)) {
	QDarOsdClose();
	ShowText();
	}
    else if (QDar_Flags[0][0] && !qdarclosed && (S_QDarText < 2 || rtclosed)) {
	RTOsdClose();
	QDarOsdTip();
	}
  
    return osContinue;
}


// --- cRTplusOsd ------------------------------------------------------

cRTplusOsd::cRTplusOsd(void)
:cOsdMenu(RTp_Titel, 3, 12)
{
    RTplus_Osd = false;
    
    bcount = helpmode = 0;
    listtyp[0] = tr("Radiotext");
    listtyp[1] = tr("Playlist");
    listtyp[2] = tr("Sports");
    listtyp[3] = tr("Lottery");
    listtyp[4] = tr("Weather");
    listtyp[5] = tr("Stockmarket");
    listtyp[6] = tr("Other");

    Load();
    Display();
}

cRTplusOsd::~cRTplusOsd()
{
}

void cRTplusOsd::Load(void) 
{
    char text[80];

    struct tm tm_store;
    struct tm *ts = localtime_r(&rtp_content.start, &tm_store);
    snprintf(text, sizeof(text), "%s  %02d:%02d", tr("RTplus Memory  since"), ts->tm_hour, ts->tm_min);
    Add(new cOsdItem(hk(text)));
    snprintf(text, sizeof(text), "%s", " ");
    Add(new cOsdItem(hk(text)));

    snprintf(text, sizeof(text), "-- %s --", tr("Programme"));
    Add(new cOsdItem(hk(text)));
    if (rtp_content.prog_Station != NULL) {
	snprintf(text, sizeof(text), "\t%s:\t%s", tr("Station"), rtp_content.prog_Station);
	Add(new cOsdItem(hk(text)));
	}
    if (rtp_content.prog_Now != NULL) {
	snprintf(text, sizeof(text), "\t%s:\t%s", tr("Now"), rtp_content.prog_Now);
	Add(new cOsdItem(hk(text)));
	}
    if (rtp_content.prog_Part != NULL) {
	snprintf(text, sizeof(text), "\t%s:\t%s", tr("...Part"), rtp_content.prog_Part);
	Add(new cOsdItem(hk(text)));
	}
    if (rtp_content.prog_Next != NULL) {
	snprintf(text, sizeof(text), "\t%s:\t%s", tr("Next"), rtp_content.prog_Next);
        Add(new cOsdItem(hk(text)));
	}
    if (rtp_content.prog_Host != NULL) {
	snprintf(text, sizeof(text), "\t%s:\t%s", tr("Host"), rtp_content.prog_Host);
	Add(new cOsdItem(hk(text)));
	}
    if (rtp_content.prog_EditStaff != NULL) {
    	snprintf(text, sizeof(text), "\t%s:\t%s", tr("Edit.Staff"), rtp_content.prog_EditStaff);
	Add(new cOsdItem(hk(text)));
	}
    if (rtp_content.prog_Homepage != NULL) {
    	snprintf(text, sizeof(text), "\t%s:\t%s", tr("Homepage"), rtp_content.prog_Homepage);
	Add(new cOsdItem(hk(text)));
	}
    snprintf(text, sizeof(text), "%s", " ");
    Add(new cOsdItem(hk(text)));

    snprintf(text, sizeof(text), "-- %s --", tr("Interactivity"));
    Add(new cOsdItem(hk(text)));
    if (rtp_content.phone_Hotline != NULL) {
    	snprintf(text, sizeof(text), "\t%s:\t%s", tr("Phone-Hotline"), rtp_content.phone_Hotline);
	Add(new cOsdItem(hk(text)));
	}
    if (rtp_content.phone_Studio != NULL) {
    	snprintf(text, sizeof(text), "\t%s:\t%s", tr("Phone-Studio"), rtp_content.phone_Studio);
	Add(new cOsdItem(hk(text)));
	}
    if (rtp_content.email_Hotline != NULL) {
    	snprintf(text, sizeof(text), "\t%s:\t%s", tr("Email-Hotline"), rtp_content.email_Hotline);
	Add(new cOsdItem(hk(text)));
	}
    if (rtp_content.email_Studio != NULL) {
    	snprintf(text, sizeof(text), "\t%s:\t%s", tr("Email-Studio"), rtp_content.email_Studio);
	Add(new cOsdItem(hk(text)));
	}
    snprintf(text, sizeof(text), "%s", " ");
    Add(new cOsdItem(hk(text)));

    snprintf(text, sizeof(text), "-- %s --", tr("Info"));
    Add(new cOsdItem(hk(text)));
    if (rtp_content.info_News != NULL) {
    	snprintf(text, sizeof(text), "\t%s:\t%s", tr("News"), rtp_content.info_News);
	Add(new cOsdItem(hk(text)));
	}
    if (rtp_content.info_NewsLocal != NULL) {
    	snprintf(text, sizeof(text), "\t%s:\t%s", tr("NewsLocal"), rtp_content.info_NewsLocal);
	Add(new cOsdItem(hk(text)));
	}
    if (rtp_content.info_DateTime != NULL) {
    	snprintf(text, sizeof(text), "\t%s:\t%s", tr("DateTime"), rtp_content.info_DateTime);
	Add(new cOsdItem(hk(text)));
	}
    if (rtp_content.info_Traffic != NULL) {
    	snprintf(text, sizeof(text), "\t%s:\t%s", tr("Traffic"), rtp_content.info_Traffic);
	Add(new cOsdItem(hk(text)));
	}
    if (rtp_content.info_Alarm != NULL) {
    	snprintf(text, sizeof(text), "\t%s:\t%s", tr("Alarm"), rtp_content.info_Alarm);
	Add(new cOsdItem(hk(text)));
	}
    if (rtp_content.info_Advert != NULL) {
    	snprintf(text, sizeof(text), "\t%s:\t%s", tr("Advertising"), rtp_content.info_Advert);
	Add(new cOsdItem(hk(text)));
	}
    if (rtp_content.info_Url != NULL) {
    	snprintf(text, sizeof(text), "\t%s:\t%s", tr("Url"), rtp_content.info_Url);
	Add(new cOsdItem(hk(text)));
	}

    for (int i = 0; i <= 6; i++)
	btext[i] = NULL;	
    bcount = 0;
    asprintf(&btext[bcount++], "%s", listtyp[0]);
    if (rtp_content.item_Index >= 0)
	asprintf(&btext[bcount++], "%s", listtyp[1]);
    if (rtp_content.info_SportIndex >= 0)
	asprintf(&btext[bcount++], "%s", listtyp[2]);
    if (rtp_content.info_LotteryIndex >= 0)
	asprintf(&btext[bcount++], "%s", listtyp[3]);
    if (rtp_content.info_WeatherIndex >= 0)
	asprintf(&btext[bcount++], "%s", listtyp[4]);
    if (rtp_content.info_StockIndex >= 0)
	asprintf(&btext[bcount++], "%s", listtyp[5]);
    if (rtp_content.info_OtherIndex >= 0)
	asprintf(&btext[bcount++], "%s", listtyp[6]);
	
    switch (bcount) {
	case 4:	    if (helpmode == 0)
			SetHelp(btext[0], btext[1], btext[2], ">>");
		    else if (helpmode == 1)
			SetHelp("<<", btext[3], NULL, tr("Exit"));
		    break;
	case 5:	    if (helpmode == 0)
			SetHelp(btext[0], btext[1], btext[2], ">>");
		    else if (helpmode == 1)
			SetHelp("<<", btext[3], btext[4], tr("Exit"));
		    break;	    
	case 6:	    if (helpmode == 0)
			SetHelp(btext[0], btext[1], btext[2], ">>");
		    else if (helpmode == 1)
			SetHelp("<<", btext[3], btext[4], ">>");
		    else if (helpmode == 2)
			SetHelp("<<", btext[5], NULL, tr("Exit"));
		    break;
	case 7:	    if (helpmode == 0)
			SetHelp(btext[0], btext[1], btext[2], ">>");
		    else if (helpmode == 1)
			SetHelp("<<", btext[3], btext[4], ">>");
		    else if (helpmode == 2)
			SetHelp("<<", btext[5], btext[6], tr("Exit"));
		    break;
	default:    helpmode = 0;
		    SetHelp(btext[0], btext[1], btext[2], tr("Exit"));
	}
}

void cRTplusOsd::Update(void)
{
    Clear();
    Load();
    Display();
}

int cRTplusOsd::rtptyp(char *btext)
{
    for (int i = 0; i <= 6; i++) {
	if (strcmp(btext, listtyp[i]) == 0)
	    return i;
	}
	
    return -1;
}

void cRTplusOsd::rtp_fileprint(void)
{
    struct tm *ts, tm_store;
    char *fname, *fpath;
    FILE *fd;
    int ind, lfd = 0;

    time_t t = time(NULL);
    ts = localtime_r(&t, &tm_store);
    asprintf(&fname, "RTplus_%s_%04d-%02d-%02d.%02d.%02d", RT_Titel, ts->tm_year+1900, ts->tm_mon+1, ts->tm_mday, ts->tm_hour, ts->tm_min);
    asprintf(&fpath, "%s/%s", DataDir, fname);
    if ((fd = fopen(fpath, "w")) != NULL) {
	
	fprintf(fd, ">>> RTplus-Memoryclasses @ %s", asctime(localtime_r(&t, &tm_store)));
	fprintf(fd, "    on '%s' since %s", RT_Titel, asctime(localtime_r(&rtp_content.start, &tm_store)));

	fprintf(fd, "--- Programme ---\n");
        if (rtp_content.prog_Station != NULL)   fprintf(fd, "     Station: %s\n", rtp_content.prog_Station);
	if (rtp_content.prog_Now != NULL)	fprintf(fd, "         Now: %s\n", rtp_content.prog_Now);
	if (rtp_content.prog_Part != NULL)	fprintf(fd, "        Part: %s\n", rtp_content.prog_Part);
	if (rtp_content.prog_Next != NULL)	fprintf(fd, "        Next: %s\n", rtp_content.prog_Next);
	if (rtp_content.prog_Host != NULL)	fprintf(fd, "        Host: %s\n", rtp_content.prog_Host);
	if (rtp_content.prog_EditStaff != NULL) fprintf(fd, "    Ed.Staff: %s\n", rtp_content.prog_EditStaff);
	if (rtp_content.prog_Homepage != NULL)  fprintf(fd, "    Homepage: %s\n", rtp_content.prog_Homepage);

	fprintf(fd, "--- Interactivity ---\n");
	if (rtp_content.phone_Hotline != NULL)  fprintf(fd, "    Phone-Hotline: %s\n", rtp_content.phone_Hotline);
	if (rtp_content.phone_Studio != NULL)   fprintf(fd, "     Phone-Studio: %s\n", rtp_content.phone_Studio);
	if (rtp_content.email_Hotline != NULL)  fprintf(fd, "    Email-Hotline: %s\n", rtp_content.email_Hotline);
	if (rtp_content.email_Studio != NULL)   fprintf(fd, "     Email-Studio: %s\n", rtp_content.email_Studio);

	fprintf(fd, "--- Info ---\n");
	if (rtp_content.info_News != NULL) 	fprintf(fd, "         News: %s\n", rtp_content.info_News);
	if (rtp_content.info_NewsLocal != NULL) fprintf(fd, "    NewsLocal: %s\n", rtp_content.info_NewsLocal);
	if (rtp_content.info_DateTime != NULL)  fprintf(fd, "     DateTime: %s\n", rtp_content.info_DateTime);
	if (rtp_content.info_Traffic != NULL)   fprintf(fd, "      Traffic: %s\n", rtp_content.info_Traffic);
	if (rtp_content.info_Alarm != NULL)     fprintf(fd, "        Alarm: %s\n", rtp_content.info_Alarm);
	if (rtp_content.info_Advert != NULL)    fprintf(fd, "    Advertisg: %s\n", rtp_content.info_Advert);
	if (rtp_content.info_Url != NULL)       fprintf(fd, "          Url: %s\n", rtp_content.info_Url);


	if (rtp_content.item_Index >= 0) {
    	    fprintf(fd, "--- Item-Playlist ---\n");
	    ind = rtp_content.item_Index;
	    if (ind < (MAX_RTPC-1) && rtp_content.item_Title[ind+1] != NULL) {
		for (int i = ind+1; i < MAX_RTPC; i++) {
		    if (rtp_content.item_Title[i] != NULL && rtp_content.item_Artist[i] != NULL) {
    			ts = localtime_r(&rtp_content.item_Start[i], &tm_store);
			fprintf(fd, "    %02d:%02d  Title: '%s' | Artist: '%s'\n", ts->tm_hour, ts->tm_min, rtp_content.item_Title[i], rtp_content.item_Artist[i]);
			}
		    }
		}
	    for (int i = 0; i <= ind; i++) {
		if (rtp_content.item_Title[i] != NULL && rtp_content.item_Artist[i] != NULL) {
    	    	    ts = localtime_r(&rtp_content.item_Start[i], &tm_store);
	    	    fprintf(fd, "    %02d:%02d  Title: '%s' | Artist: '%s'\n", ts->tm_hour, ts->tm_min, rtp_content.item_Title[i], rtp_content.item_Artist[i]);
	    	    }
		}
	    }

	if (rtp_content.info_SportIndex >= 0) {
	    fprintf(fd, "--- Sports ---\n");
	    ind = rtp_content.info_SportIndex;
	    if (ind < (MAX_RTPC-1) && rtp_content.info_Sport[ind+1] != NULL) {
	        for (int i = ind+1; i < MAX_RTPC; i++) {
		    if (rtp_content.info_Sport[i] != NULL)
			fprintf(fd, "    %02d. %s\n", ++lfd, rtp_content.info_Sport[i]);
		    }
		}
	    for (int i = 0; i <= ind; i++) {
		if (rtp_content.info_Sport[i] != NULL)
		    fprintf(fd, "    %02d. %s\n", ++lfd, rtp_content.info_Sport[i]);
		}
	    }
	    
	if (rtp_content.info_LotteryIndex >= 0) {
	    fprintf(fd, "--- Lottery ---\n");
	    ind = rtp_content.info_LotteryIndex;
	    if (ind < (MAX_RTPC-1) && rtp_content.info_Lottery[ind+1] != NULL) {
	        for (int i = ind+1; i < MAX_RTPC; i++) {
		    if (rtp_content.info_Lottery[i] != NULL)
			fprintf(fd, "    %02d. %s\n", ++lfd, rtp_content.info_Lottery[i]);
		    }
		}
	    for (int i = 0; i <= ind; i++) {
		if (rtp_content.info_Lottery[i] != NULL)
		    fprintf(fd, "    %02d. %s\n", ++lfd, rtp_content.info_Lottery[i]);
		}
	    }

	if (rtp_content.info_WeatherIndex >= 0) {
	    fprintf(fd, "--- Weather ---\n");
    	    ind = rtp_content.info_WeatherIndex;
	    if (ind < (MAX_RTPC-1) && rtp_content.info_Weather[ind+1] != NULL) {
	        for (int i = ind+1; i < MAX_RTPC; i++) {
		    if (rtp_content.info_Weather[i] != NULL)
			fprintf(fd, "    %02d. %s\n", ++lfd, rtp_content.info_Weather[i]);
		    }
		}
	    for (int i = 0; i <= ind; i++) {
	        if (rtp_content.info_Weather[i] != NULL)
		    fprintf(fd, "    %02d. %s\n", ++lfd, rtp_content.info_Weather[i]);
	        }
	    }

	if (rtp_content.info_StockIndex >= 0) {
	    fprintf(fd, "--- Stockmarket ---\n");
	    ind = rtp_content.info_StockIndex;
	    if (ind < (MAX_RTPC-1) && rtp_content.info_Stock[ind+1] != NULL) {
	        for (int i = ind+1; i < MAX_RTPC; i++) {
		    if (rtp_content.info_Stock[i] != NULL)
			fprintf(fd, "    %02d. %s\n", ++lfd, rtp_content.info_Stock[i]);
		    }
		}
	    for (int i = 0; i <= ind; i++) {
		if (rtp_content.info_Stock[i] != NULL)
		    fprintf(fd, "    %02d. %s\n", ++lfd, rtp_content.info_Stock[i]);
		}
	    }
	    
	if (rtp_content.info_OtherIndex >= 0) {
	    fprintf(fd, "--- Other ---\n");
	    ind = rtp_content.info_OtherIndex;
	    if (ind < (MAX_RTPC-1) && rtp_content.info_Other[ind+1] != NULL) {
		for (int i = ind+1; i < MAX_RTPC; i++) {
		    if (rtp_content.info_Other[i] != NULL)
			fprintf(fd, "    %02d. %s\n", ++lfd, rtp_content.info_Other[i]);
		    }
		}
	    for (int i = 0; i <= ind; i++) {
	        if (rtp_content.info_Other[i] != NULL)
		    fprintf(fd, "    %02d. %s\n", ++lfd, rtp_content.info_Other[i]);
		}
	    }

	fprintf(fd, "--- Last seen Radiotext ---\n");
	ind = rtp_content.rt_Index;
	if (ind < (2*MAX_RTPC-1) && rtp_content.radiotext[ind+1] != NULL) {
	    for (int i = ind+1; i < 2*MAX_RTPC; i++) {
		if (rtp_content.radiotext[i] != NULL)
		    fprintf(fd, "    %03d. %s\n", ++lfd, rtp_content.radiotext[i]);
		}
	    }
	for (int i = 0; i <= ind; i++) {
	    if (rtp_content.radiotext[i] != NULL)
	        fprintf(fd, "    %03d. %s\n", ++lfd, rtp_content.radiotext[i]);
	    }

	fprintf(fd, "<<<\n");
	fclose(fd);

	char *infotext;
	asprintf(&infotext, "%s: %s", tr("RTplus-File saved"), fname);
	Skins.Message(mtInfo, infotext, Setup.OSDMessageTime);
	free(infotext);
	}
    else
        esyslog("ERROR vdr-radio: writing RTplus-File failed '%s'", fpath);

    free(fpath);
    free(fname);
}

eOSState cRTplusOsd::ProcessKey(eKeys Key)
{
    int typ, ind;
    eOSState state = cOsdMenu::ProcessKey(Key);

    if (HasSubMenu())
	return osContinue;

    if (state == osUnknown) {
	switch (Key) {
	    case kBack:
	    case kOk:	    return osEnd;
	    case kBlue:	    if (bcount >= 4 && helpmode == 0) {
				helpmode += 1;
				Update();
				}
			    else if (bcount >= 6 && helpmode == 1) {
				helpmode += 1;
				Update();
				}
			    else 
				return osEnd;
			    break;
	    case k0:	    Update();
			    break;
	    case k8:	    rtp_fileprint();
			    break;
    	    case kRed:	    if (helpmode == 0) {
				if (btext[0] != NULL)
				    if ((typ = rtptyp(btext[0])) >= 0)
					AddSubMenu(new cRTplusList(typ));
				}
			    else {
			    	helpmode -= 1;
				Update();
				}
			    break;
    	    case kGreen:    ind = (helpmode*2) + 1;
			    if (btext[ind] != NULL) {
				if ((typ = rtptyp(btext[ind])) >= 0)
				    AddSubMenu(new cRTplusList(typ));
				}
			    break;
    	    case kYellow:   ind = (helpmode*2) + 2;
			    if (btext[ind] != NULL) {
			        if ((typ = rtptyp(btext[ind])) >= 0)
					AddSubMenu(new cRTplusList(typ));
				}
			    break;
    	    default:	    state = osContinue;
	}
    }
    
    static int ct;
    if (++ct >= 60) {
	ct = 0;
	Update();
	}

    return state;
}


// --- cRTplusList ------------------------------------------------------

cRTplusList::cRTplusList(int Typ)
:cOsdMenu(RTp_Titel, 4)
{
    typ = Typ;
    refresh = false;
    
    Load();
    Display();
}

cRTplusList::~cRTplusList() 
{
    typ = 0;
}

void cRTplusList::Load(void) 
{
    char text[80];
    struct tm *ts, tm_store;
    int ind, lfd = 0;
    
    ts = localtime_r(&rtp_content.start, &tm_store);
    switch (typ) {
	case 0:	 snprintf(text, sizeof(text), "-- %s (max. %d) --", tr("last seen Radiotext"), 2*MAX_RTPC);
		 Add(new cOsdItem(hk(text)));
		 snprintf(text, sizeof(text), "%s", " ");
		 Add(new cOsdItem(hk(text)));
		 ind = rtp_content.rt_Index;
		 if (ind < (2*MAX_RTPC-1) && rtp_content.radiotext[ind+1] != NULL) {
		    for (int i = ind+1; i < 2*MAX_RTPC; i++) {
			if (rtp_content.radiotext[i] != NULL) {
			    snprintf(text, sizeof(text), "%d.\t%s", ++lfd, rtp_content.radiotext[i]);
			    Add(new cOsdItem(hk(text)));
			    }
			}
		    }
		 for (int i = 0; i <= ind; i++) {
		    if (rtp_content.radiotext[i] != NULL) {
		        snprintf(text, sizeof(text), "%d.\t%s", ++lfd, rtp_content.radiotext[i]);
		        Add(new cOsdItem(hk(text)), refresh);
		        }
		    }
		 break;
	case 1:	 SetCols(6, 19, 1);
		 snprintf(text, sizeof(text), "-- %s --", tr("Playlist"));
		 Add(new cOsdItem(hk(text)));
		 snprintf(text, sizeof(text), "%s\t%s\t\t%s", tr("Time"), tr("Title"), tr("Artist"));
		 Add(new cOsdItem(hk(text)));
		 snprintf(text, sizeof(text), "%s", " ");
		 Add(new cOsdItem(hk(text)));
		 ind = rtp_content.item_Index;
		 if (ind < (MAX_RTPC-1) && rtp_content.item_Title[ind+1] != NULL) {
		    for (int i = ind+1; i < MAX_RTPC; i++) {
			if (rtp_content.item_Title[i] != NULL && rtp_content.item_Artist[i] != NULL) {
    			    ts = localtime_r(&rtp_content.item_Start[i], &tm_store);
			    snprintf(text, sizeof(text), "%02d:%02d\t%s\t\t%s", ts->tm_hour, ts->tm_min, rtp_content.item_Title[i], rtp_content.item_Artist[i]);
			    Add(new cOsdItem(hk(text)));
			    }
			}
		    }
		 for (int i = 0; i <= ind; i++) {
		    if (rtp_content.item_Title[i] != NULL && rtp_content.item_Artist[i] != NULL) {
    		        ts = localtime_r(&rtp_content.item_Start[i], &tm_store);
		        snprintf(text, sizeof(text), "%02d:%02d\t%s\t\t%s", ts->tm_hour, ts->tm_min, rtp_content.item_Title[i], rtp_content.item_Artist[i]);
		        Add(new cOsdItem(hk(text)), refresh);
		        }
		    }
		 break;
	case 2:	 snprintf(text, sizeof(text), "-- %s --", tr("Sports"));
		 Add(new cOsdItem(hk(text)));
		 snprintf(text, sizeof(text), "%s", " ");
		 Add(new cOsdItem(hk(text)));
		 ind = rtp_content.info_SportIndex;
		 if (ind < (MAX_RTPC-1) && rtp_content.info_Sport[ind+1] != NULL) {
		    for (int i = ind+1; i < MAX_RTPC; i++) {
			if (rtp_content.info_Sport[i] != NULL) {
			    snprintf(text, sizeof(text), "%d.\t%s", ++lfd, rtp_content.info_Sport[i]);
			    Add(new cOsdItem(hk(text)));
			    }
			}
		    }
		 for (int i = 0; i <= ind; i++) {
		    if (rtp_content.info_Sport[i] != NULL) {
		        snprintf(text, sizeof(text), "%d.\t%s", ++lfd, rtp_content.info_Sport[i]);
		        Add(new cOsdItem(hk(text)), refresh);
		        }
		    }
		 break;
	case 3:	 snprintf(text, sizeof(text), "-- %s --", tr("Lottery"));
		 Add(new cOsdItem(hk(text)));
		 snprintf(text, sizeof(text), "%s", " ");
		 Add(new cOsdItem(hk(text)));
		 ind = rtp_content.info_LotteryIndex;
		 if (ind < (MAX_RTPC-1) && rtp_content.info_Lottery[ind+1] != NULL) {
		    for (int i = ind+1; i < MAX_RTPC; i++) {
			if (rtp_content.info_Lottery[i] != NULL) {
			    snprintf(text, sizeof(text), "%d.\t%s", ++lfd, rtp_content.info_Lottery[i]);
			    Add(new cOsdItem(hk(text)));
			    }
			}
		    }
		 for (int i = 0; i <= ind; i++) {
		    if (rtp_content.info_Lottery[i] != NULL) {
		        snprintf(text, sizeof(text), "%d.\t%s", ++lfd, rtp_content.info_Lottery[i]);
		        Add(new cOsdItem(hk(text)), refresh);
		        }
		    }
		 break;
	case 4:	 snprintf(text, sizeof(text), "-- %s --", tr("Weather"));
		 Add(new cOsdItem(hk(text)));
		 snprintf(text, sizeof(text), "%s", " ");
		 Add(new cOsdItem(hk(text)));
		 ind = rtp_content.info_WeatherIndex;
		 if (ind < (MAX_RTPC-1) && rtp_content.info_Weather[ind+1] != NULL) {
		    for (int i = ind+1; i < MAX_RTPC; i++) {
			if (rtp_content.info_Weather[i] != NULL) {
			    snprintf(text, sizeof(text), "%d.\t%s", ++lfd, rtp_content.info_Weather[i]);
			    Add(new cOsdItem(hk(text)));
			    }
			}
		    }
		 for (int i = 0; i <= ind; i++) {
		    if (rtp_content.info_Weather[i] != NULL) {
		        snprintf(text, sizeof(text), "%d.\t%s", ++lfd, rtp_content.info_Weather[i]);
		        Add(new cOsdItem(hk(text)), refresh);
		        }
		    }
		 break;
	case 5:	 snprintf(text, sizeof(text), "-- %s --", tr("Stockmarket"));
		 Add(new cOsdItem(hk(text)));
		 snprintf(text, sizeof(text), "%s", " ");
		 Add(new cOsdItem(hk(text)));
		 ind = rtp_content.info_StockIndex;
		 if (ind < (MAX_RTPC-1) && rtp_content.info_Stock[ind+1] != NULL) {
		    for (int i = ind+1; i < MAX_RTPC; i++) {
			if (rtp_content.info_Stock[i] != NULL) {
			    snprintf(text, sizeof(text), "%d.\t%s", ++lfd, rtp_content.info_Stock[i]);
			    Add(new cOsdItem(hk(text)));
			    }
			}
		    }
		 for (int i = 0; i <= ind; i++) {
		    if (rtp_content.info_Stock[i] != NULL) {
		        snprintf(text, sizeof(text), "%d.\t%s", ++lfd, rtp_content.info_Stock[i]);
		        Add(new cOsdItem(hk(text)), refresh);
		        }
		    }
		 break;
	case 6:	 snprintf(text, sizeof(text), "-- %s --", tr("Other"));
		 Add(new cOsdItem(hk(text)));
		 snprintf(text, sizeof(text), "%s", " ");
		 Add(new cOsdItem(hk(text)));
		 ind = rtp_content.info_OtherIndex;
		 if (ind < (MAX_RTPC-1) && rtp_content.info_Other[ind+1] != NULL) {
		    for (int i = ind+1; i < MAX_RTPC; i++) {
			if (rtp_content.info_Other[i] != NULL) {
			    snprintf(text, sizeof(text), "%d.\t%s", ++lfd, rtp_content.info_Other[i]);
			    Add(new cOsdItem(hk(text)));
			    }
			}
		    }
		 for (int i = 0; i <= ind; i++) {
		    if (rtp_content.info_Other[i] != NULL) {
		        snprintf(text, sizeof(text), "%d.\t%s", ++lfd, rtp_content.info_Other[i]);
		        Add(new cOsdItem(hk(text)), refresh);
		        }
		    }
		 break;
	}

    SetHelp(NULL, NULL , refresh ? tr("Refresh Off") :  tr("Refresh On"), tr("Back"));
}

void cRTplusList::Update(void)
{
    Clear();
    Load();
    Display();
}

eOSState cRTplusList::ProcessKey(eKeys Key)
{
    eOSState state = cOsdMenu::ProcessKey(Key);

    if (state == osUnknown) {
	switch (Key) {
	    case k0:	    Update();
			    break;
	    case kYellow:   refresh = (refresh) ? false : true;
			    Update();
			    break;
	    case kBack:
	    case kOk:	    
	    case kBlue:	    return osBack;
    	    default:	    state = osContinue;
	}
    }
    
    static int ct;
    if (refresh) {
	if (++ct >= 20) {
	    ct = 0;
	    Update();
	    }
	}
	
    return state;
}


//--------------- End -----------------------------------------------------------------
