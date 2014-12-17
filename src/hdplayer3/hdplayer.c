#include "player.h"

#include "channel.h"
#include "osd.h"

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <syslog.h>
#include <execinfo.h>

#include <hdchannel.h>
#include <hdshm_user_structs.h>

hd_data_t volatile *hda = NULL;

int sock_stream1 = -1;
int chiprev=1;

// ----------------------------------------------------------------------------

static int init(void);
static int run(void);

// ----------------------------------------------------------------------------
void get_chipversion(void)
{
	if (!system("grep 'Revision  0' /proc/asicinfo"))
		chiprev=0;
}
// ----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    get_chipversion();

    if (!init())
    {
        return 1;
    }

    if (!run())
    {
        return 1;
    }
    return 0;
}

// ----------------------------------------------------------------------------

static int init(void)
{
    int area_size = ALIGN_UP(sizeof(hd_data_t), 2*4096);
    hdshm_area_t *area;

    if (hd_init(0) != 0)
    {
        fprintf(stderr, "ERROR: Unable to open hdshm device.\n");
        return 0;
    }

    area = hd_get_area(HDID_HDA);

    if(!area){
        printf("creating new hd-shm-area\n");
        hdshm_reset();
        area = hd_create_area(HDID_HDA, NULL, area_size, HDSHM_MEM_HD);
        //memset((void *)area, 0, area_size);
    }

    if (!area)
    {
        fprintf(stderr, "ERROR: Unable to create hda.\n");
        return 0;
    }

    hda = (hd_data_t volatile *)area->mapped;
    hda->osd_flush_count = 0;
    int i;
    for(i = 0; i<HDOSD_MAX_CACHED_FONTS; i++)
          hda->osd_cached_fonts[i] = 0;
    for(i = 0; i<HDOSD_MAX_IMAGES; i++)
          hda->osd_cached_images[i] = 0;

//    memset((void *)hda, 0, area_size);

    return create_channels();
}

// --------------------------------------------------------------------------
void signalhandler(int num)
{
	printf("Got signal %i\n",num);
	if (hda)
		hda->hdp_running=0;
	exit(-1);
}
void signalhandlerCrash(int num)
{
    syslog(LOG_INFO, "Got signal %i\n",num);
    if (hda)
        hda->hdp_running=0;
    //signal(num, SIG_DFL);
    void *array[15];
    int i, size = backtrace(array, 15);
    char **strings = backtrace_symbols(array, size);
    for(i=0; i<size;i++)
        syslog(LOG_INFO, " >%d: %s", i, strings[i]);
    free(strings);
    syslog(LOG_INFO, "Got signal %i DONE\n",num);
    exit(-1);
}
// --------------------------------------------------------------------------

static int run(void)
{
    player_t* pl[2];
    int res = 0;


    pl[0]= new_player(ch_stream1, ch_stream1_info, 0);
    pl[1]= new_player(ch_stream2, ch_stream2_info, 1);
    hd_channel_invalidate(ch_stream1, 1);
    hd_channel_invalidate(ch_stream2, 1);
    new_osd(ch_osd);
    start_thread();
    signal(SIGINT, signalhandler);
    signal(SIGQUIT, signalhandler);
    signal(SIGTERM, signalhandler);
    signal(SIGSEGV, signalhandlerCrash);
    signal(SIGBUS, signalhandlerCrash);

    hda->hdp_running=1;
    hda->active_player[0] = HD_PLAYER_MODE_LIVE; // Save mode
    hda->active_player[1] = HD_PLAYER_MODE_LIVE; // Save mode
    hda->osd_dont_touch&=~2; // Clear xine draw bit (prevent no fb osd if hdplayer crashes)
    /* TB: avoid busy-waiting */
    while ((res = run_player(pl[0])|run_player(pl[1]))) {
	    if(res==1) 
		    usleep(5*1000);
            if (hda->hdp_terminate==1)
		    break;
    };
    delete_player(pl[1]);
    delete_player(pl[0]);
    hda->hdp_terminate=0;
    printf("hdplayer clean exit %i\n",hda->hdp_terminate);

    return 0;
}

// ----------------------------------------------------------------------------

