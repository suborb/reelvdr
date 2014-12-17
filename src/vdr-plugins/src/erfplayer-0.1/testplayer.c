#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <hdshm_user_structs.h>
#include "../reelbox/HdCommChannel.h"

#include "../reelbox/Reel.c"
#include "../reelbox/hdshmlib.c"
#include "../reelbox/hdchannel.c"
#include "../reelbox/HdCommChannel.c"

#define _esyslog(a...) { printf(a);printf("\n");  } 
#define _isyslog(a...) { printf(a);printf("\n");  }
#define _output(a...) { printf(a);printf("\r"); }
#include "rawplayer.c"

int main(int argc, char **argv) { 
	if (argc < 2) {
		Reel::HdCommChannel::Init();
		Reel::ReelBoxSetup lSetup;
		memset(&lSetup, 0, sizeof(lSetup));
		lSetup.sub_mode = 5; // dvi
		lSetup.resolution = 5; // 720p
		lSetup.display_type = 0;// 4:3
		lSetup.aspect = 2; // crop
		
		Reel::HdCommChannel::SetVideomode(&lSetup);
		Reel::HdCommChannel::Exit();
		return 0;
	} // if 
	printf("START\n");
	{
		ERFPlayer::RawPlayer lPlayer;
		lPlayer.Play(argv[1], 0);
		
		bool restore = false;	
		struct termios raw, orig;
		if (isatty(STDIN_FILENO) && !fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK)) {
			tcgetattr(STDIN_FILENO, &orig);
			raw = orig;
			raw.c_lflag &= ~(ICANON | ECHO);
			tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
			restore = true;
		} // if
			
		printf("(r)FR (f)FF (p)Pause (s)Stop (g)Play (<)Skip -10s (>)Skip +10s\n");
		char lKey = 0;
		while(lKey != 'q') {
			if(read(STDIN_FILENO, &lKey, 1) == 1) {
				switch (lKey) {
					case 'p': lPlayer.Pause(); break;
					case 'f': lPlayer.FF(); break;
					case 'r': lPlayer.FR(); break;
					case 's': lPlayer.Stop(); break;
					case 'g': lPlayer.Play(); break;
					case '>': lPlayer.Skip(10); break;
					case '<': lPlayer.Skip(-10); break;
					case 'a': 
				    	if (lPlayer.GetCurrentAudio()+1 < lPlayer.GetAudioCount())
				    		lPlayer.SetCurrentAudio(lPlayer.GetCurrentAudio()+1);
				    	else
				    		lPlayer.SetCurrentAudio(0);
			    		break;
				} // switch
			} // if
			::usleep(100000);
		} // while
		if(restore)
			tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
	}
	printf("DONE\n");
} // main
