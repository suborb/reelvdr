#include "player.h"

#include "channel.h"
#include "osd.h"

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include <hdchannel.h>
#include <hdshm_user_structs.h>

hd_data_t volatile *hda = NULL;

int sock_stream1 = -1;

// ----------------------------------------------------------------------------

static int init(void);
static int run(void);

// ----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
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

static int run(void)
{
    player_t *pl = new_player(NULL, ch_stream1, 0);
    int res = 0;

    new_osd(ch_osd);
    start_thread();

    /* TB: avoid busy-waiting */
    while (res = run_player(pl)) {
	    if(res==1) 
		    usleep(20);
	    if (hda->hdp_terminate==1)
		    break;
    };
    delete_player(pl);
    hda->hdp_terminate=0;
    printf("hdplayer clean exit %i\n",hda->hdp_terminate);

    return 0;
}

// ----------------------------------------------------------------------------

