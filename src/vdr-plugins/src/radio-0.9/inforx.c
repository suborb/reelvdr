/*
 * inetrx.c - part of radio.c, a plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <fstream>
#include "inforx.h"


void fill_rtpbuffer(char *text1, char *text2) {

    if (++rtp_content.rt_Index >= 2*MAX_RTPC) 
    	rtp_content.rt_Index = 0;
    asprintf(&rtp_content.radiotext[rtp_content.rt_Index], "%s", text1);
	snprintf(RT_Text[RT_Index], RT_MEL, "%s", rtp_content.radiotext[rtp_content.rt_Index]);
    RT_Index +=1; if (RT_Index >= S_RtOsdRows) RT_Index = 0;

    if (++rtp_content.rt_Index >= 2*MAX_RTPC) 
    	rtp_content.rt_Index = 0;
    asprintf(&rtp_content.radiotext[rtp_content.rt_Index], "%s", text2);
    snprintf(RT_Text[RT_Index], RT_MEL, "%s", rtp_content.radiotext[rtp_content.rt_Index]);
    RT_Index +=1; if (RT_Index >= S_RtOsdRows) RT_Index = 0;

    if (++rtp_content.item_Index >= MAX_RTPC)
		rtp_content.item_Index = 0;
    if (rtp_content.item_Index >= 0) {
    	rtp_content.item_Start[rtp_content.item_Index] = RTP_Starttime;
		asprintf(&rtp_content.item_Artist[rtp_content.item_Index], "%s", RTP_Artist);
		asprintf(&rtp_content.item_Title[rtp_content.item_Index], "%s", RTP_Title);
	   	}

	if ((S_Verbose & 0x0f) >= 1)
   		printf("radio-inforx: %s / %s\n", RTP_Artist, RTP_Title);
}

int info_request(int chantid, int chanapid) {

	char ident[80];
	char artist[RT_MEL], titel[RT_MEL];
	struct tm tm_store;
	static int repeat = 0;
	char temp[2][2*RT_MEL];

	char *command, *tempfile;
	asprintf(&command, "%s/radioinfo-%d-%d", ConfigDir, chantid, chanapid);
	asprintf(&tempfile, "%s/%s", DataDir, RadioInfofile);
	dsyslog("radio: inforxcall '%s'\n", command);
   	
	DoInfoReq = false;
	if (file_exists(command)) {
		if (file_exists(tempfile)) {	// delete tempfile
			char *delcmd;
			asprintf(&delcmd, "rm -f \"%s\"", tempfile);
			system(delcmd);
			free(delcmd);
			}
		asprintf(&command, "%s \"%s\"", command, tempfile);
		if (!system(command)) {			// execute script && read tempfile
			if (file_exists(tempfile)) {
			  	std::ifstream infile(tempfile);
			  	if (infile.is_open()) {
    				infile.getline(ident, 80);
		    		infile.getline(artist, RT_MEL);
    				infile.getline(titel, RT_MEL);
					}
				infile.close();
				DoInfoReq = true;
				}
			else
				esyslog("radio: ERROR inforx tempfile = %s\n", command);
			}
		else
			esyslog("radio: ERROR inforx command = %s\n", command);
		}
	free(command);
	free(tempfile);

	if (!DoInfoReq) {
		dsyslog("radio: inforxcall cancled !\n");
		return -1;
		}

	// Info empty?
	if (strcmp(artist, "") == 0 && strcmp(titel, "") == 0) {
		if (repeat < 700) {		// ~ 6x switch off
			repeat += 120;
			sprintf(RTP_Artist, "---");
			sprintf(RTP_Title, "---");
			RTP_Starttime = time(NULL);
			sprintf(temp[0], "%s  Error :-(", ident);
			struct tm *ts = localtime_r(&RTP_Starttime, &tm_store);
			sprintf(temp[1], "%02d:%02d  no Songinfo received !", ts->tm_hour, ts->tm_min);
			fill_rtpbuffer(temp[0], temp[1]);
			RT_MsgShow = true;
			RT_PlusShow = false;
    		(RT_Info > 0) ? : RT_Info = 1;
			radioStatusMsg();
			return 120;
			}
		else {
			RTP_Starttime = time(NULL);
			sprintf(temp[0], "%s  Error :-(", ident);
			struct tm *ts = localtime_r(&RTP_Starttime, &tm_store);
			sprintf(temp[1], "%02d:%02d  no Songinfo received, switching off !", ts->tm_hour, ts->tm_min);
			asprintf(&rtp_content.info_Url,  "%02d:%02d  no Songinfo, switched off!", ts->tm_hour, ts->tm_min);
			fill_rtpbuffer(temp[0], temp[1]);
			RT_MsgShow = true;
			RT_PlusShow = false;
    		(RT_Info > 0) ? : RT_Info = 1;
			radioStatusMsg();
			DoInfoReq = false;
			return 60;
			}
		}

	// Info
    xhtml2text(artist);
    xhtml2text(titel);
	if (strcmp(RTP_Artist, artist) != 0 || strcmp(RTP_Title, titel) != 0) {
	    snprintf(RTP_Artist, RT_MEL, artist);
		snprintf(RTP_Title, RT_MEL, titel);
		RTP_Starttime = time(NULL);
	    sprintf(temp[0], "%s  ok :-)", ident);
	    struct tm *ts = localtime_r(&RTP_Starttime, &tm_store);
	    snprintf(temp[1], 2*RT_MEL, "%02d:%02d  %s: %s", ts->tm_hour, ts->tm_min, RTP_Artist, RTP_Title);
		fill_rtpbuffer(temp[0], temp[1]);
		RT_MsgShow = RT_PlusShow = true;
		(RT_Info > 0) ? : RT_Info = 2;
		radioStatusMsg();
		repeat = 0;
	    return 90;
		}
	else {
		if (repeat < 1200) {		// ~20 Min. timeout
			repeat += 10;
	    	return 10;
			}
		else {
			sprintf(RTP_Artist, "---");
			sprintf(RTP_Title, "---");
			RTP_Starttime = time(NULL);
			sprintf(temp[0], "%s  Error :-(", ident);
			struct tm *ts = localtime_r(&RTP_Starttime, &tm_store);
			sprintf(temp[1], "%02d:%02d  no more different Songinfo received, switching off !", ts->tm_hour, ts->tm_min);
			asprintf(&rtp_content.info_Url,  "%02d:%02d  no more Songinfo, switched off!", ts->tm_hour, ts->tm_min);
			fill_rtpbuffer(temp[0], temp[1]);
			RT_MsgShow = true;
			RT_PlusShow = false;
   			(RT_Info > 0) ? : RT_Info = 1;
			radioStatusMsg();
			DoInfoReq = false;
			return 60;
			}
   		}
	
	return 10; 
}


// end
