/** A simple player based on gstreamer 
   plays one argument after the other. 

   MRL of the form: 
        file:///media/hd/video/myfile.mkv
        http://mystreamingserver.com/stream
*/
#include "GstreamerLib.h"

int main(int argc, char* argv[]) {

    Reel::GstLib gstlib;
    int i;
    int len, pos;

    if (argc<2) {
        g_print("no uris to play.\n %s file:///path/to/media/file\n", argv[0]);
        return 0;
    }

    // init gstreamer and make player
    gstlib.Init();

    i = 1;

    g_print("Ctrl-C to exit.\n");

    while (1) {

        gstlib.HandleBusMessages();

        if (gstlib.UriWanted()) { // playback ended OR Error
            g_print("UriWanted i = %d\n", i);
            
            if (i<argc) {
                // assume mrl/uri in proper format
                gstlib.PlayMrl(argv[i++]);
            }
            else // played all give mrls
                break;

        } // if

        sleep(1);

        gstlib.GetIndex(pos, len);

#include FILE_INCLUDE

    } // while

    return 0;
} // main()

