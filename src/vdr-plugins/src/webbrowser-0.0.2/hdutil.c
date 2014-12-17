
#include <iostream>
#include "hdshmlib.h"
#include "hdshm_user_structs.h"
#include "hdutil.h"

static hdshm_area_t *hsa;
static hd_data_t *hdd;

int setalpha(int alpha)
{
    if (hd_init(0))
    {
        std::cerr << "Error: hdshm driver not loaded!" << std::endl;
        return -1;
    }
    if (!hsa)
        hsa = hd_get_area(HDID_HDA);
    if (hsa)
    {
        if (!hdd)
            hdd = (hd_data_t *) hsa->mapped;
        if (!hdd)
        {
            std::cerr << "Error: hsa not mapped!" << std::endl;
            return -1;
        }

        std::cout << "SetAlpha: " << alpha << std::endl;
        hdd->plane[2].alpha = alpha;
        hdd->plane[2].changed++;
        return 0;
    }
    std::cerr << "Error: hd_get_area failed!" << std::endl;
    return -1;
}

int setcenter(void)
{
    if (hd_init(0))
    {
        std::cerr << "Error: hdshm driver not loaded!" << std::endl;
        return -1;
    }
    if (!hsa)
        hsa = hd_get_area(HDID_HDA);
    if (hsa)
    {
        if (!hdd)
            hdd = (hd_data_t *) hsa->mapped;
        if (!hdd)
        {
            std::cerr << "Error: hsa not mapped!" << std::endl;
            return -1;
        }

        std::cout << "SetCenter " << std::endl;
        hdd->plane[2].ws = -1;
        hdd->plane[2].changed++;
        return 0;
    }
    std::cerr << "Error: hd_get_area failed!" << std::endl;
    return -1;
}

int setfull(void)
{
    if (hd_init(0))
    {
        std::cerr << "Error: hdshm driver not loaded!" << std::endl;
        return -1;
    }
    if (!hsa)
        hsa = hd_get_area(HDID_HDA);
    if (hsa)
    {
        if (!hdd)
            hdd = (hd_data_t *) hsa->mapped;
        if (!hdd)
        {
            std::cerr << "Error: hsa not mapped!" << std::endl;
            return -1;
        }

        std::cout << "SetCenter " << std::endl;
        hdd->plane[2].ws = 0;
        hdd->plane[2].hs = 0;
        hdd->plane[2].changed++;
        return 0;
    }
    std::cerr << "Error: hd_get_area failed!" << std::endl;
    return -1;
}

int oldAutoformat = -1;
int oldWidth = -1;
int oldHeight = -1;

bool switchToHDifAutoFormat(void)
{
    if (hd_init(0))
    {
        std::cerr << "Error: hdshm driver not loaded!" << std::endl;
        return -1;
    }
    if (!hsa)
        hsa = hd_get_area(HDID_HDA);
    if (hsa)
    {
        if (!hdd)
            hdd = (hd_data_t *) hsa->mapped;
        if (!hdd)
        {
            std::cerr << "Error: hsa not mapped!" << std::endl;
            return -1;
        }

        int auto_format = hdd->video_mode.auto_format;
        int width = hdd->video_mode.width;
        int height = hdd->video_mode.height;
        int cur_w = oldWidth = hdd->video_mode.current_w;
        int cur_h = oldHeight = hdd->video_mode.current_h;

        std::cout << "AutoFormat: " << auto_format << std::endl;
        std::cout << "w: " << width << "h: " << height << std::endl;
        std::cout << "c_w: " << cur_w << "c_h: " << cur_h << std::endl;

        if (auto_format && (width != cur_w || height != cur_h))
        {
            std::cout << "switching off autoformat" << std::endl;
            hdd->video_mode.auto_format = 0;
            hdd->video_mode.current_w = width;
            hdd->video_mode.current_h = height;
            hdd->video_mode.changed++;
            oldAutoformat = auto_format;
        }
        return 0;
    }
    std::cerr << "Error: hd_get_area failed!" << std::endl;
    return -1;
}

bool switchBack(void)
{
    if (hd_init(0))
    {
        std::cerr << "Error: hdshm driver not loaded!" << std::endl;
        return -1;
    }
    if (!hsa)
        hsa = hd_get_area(HDID_HDA);
    if (hsa)
    {
        if (!hdd)
            hdd = (hd_data_t *) hsa->mapped;
        if (!hdd)
        {
            std::cerr << "Error: hsa not mapped!" << std::endl;
            return -1;
        }

        if (oldAutoformat >= 0 && oldWidth >= 0 && oldHeight >= 0)
        {
            std::cout << "switching back to autoformat" << std::endl;
            hdd->video_mode.auto_format = oldAutoformat;
            hdd->video_mode.current_w = oldWidth;
            hdd->video_mode.current_h = oldHeight;
            hdd->video_mode.changed++;
            oldAutoformat = -1;
            oldWidth = -1;
            oldHeight = -1;
        }
        return 0;
    }
    std::cerr << "Error: hd_get_area failed!" << std::endl;
    return -1;
}
