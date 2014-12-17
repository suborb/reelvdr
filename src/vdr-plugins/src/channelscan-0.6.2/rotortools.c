#include "rotortools.h"

#ifndef DEVICE_ATTRIBUTES
#include <sys/ioctl.h>
#endif
#include <vdr/device.h>
#include <vdr/plugin.h>


bool WaitForRotorMovement(int cardnr)
{
    /* wait until rotor has moved the dish */
#define DISH_MOVE_TIMEOUT 30
    time_t now = time(NULL);
    if (TunerIsRotor(cardnr))
    {
        rotorStatus rStat;
        /* give the rotor a chance to start moving */
        cCondWait::SleepMs(500);
        do
        {
            if (GetRotorStatus(cardnr, &rStat) == false)
            {
                //printf("error: GetRotorStatus failed on cardnr: %i\n", cardnr);
                return false;
            }
            /* sleep as long as rotor is moving */
            if (rStat != R_NOT_MOVING)
            {
                //printf("waiting for rotor... status: %i\n", rStat);
                cCondWait::SleepMs(333);
            }
        }
        while (rStat != R_NOT_MOVING && time(NULL) - now < DISH_MOVE_TIMEOUT);
    }
    return true;
}

void MoveRotor(int cardIndex, int source)
{
    // NetCeiver Rotor
    uchar cmd3 = 0, cmd4 = 0;
    if (1)
    {
        cPlugin *p = cPluginManager::GetPlugin("netcvrotor");
        if (p)
        {
            void *data[3];
            int cmds;
            data[0] = (void *)(cardIndex + 1);
            data[1] = (void *)(source);
            p->Service("netcvrotor.getDiseqc", &data);
            cmds = (int)data[2];
            if (cmds != -1)
            {
                cmd3 = cmds & 255;
                cmd4 = (cmds >> 8) & 255;
            }
        }
    }
}

bool IsWithinConfiguredBorders(int currentTuner, const cSource * source)
{
    int max = -1, min = -1;
    RotorGetMinMax(currentTuner + 1, &min, &max);
    /* normalize to zero */
    min -= 1800;
    max -= 1800;
    cString str = cSource::ToString(source->Code());
    const char *cstr = (char *)*str;
    int i1, i2;
    char c;

    /* Parse VDR's sources-String */
    if ((sscanf(cstr, "S%i%c", &i1, &c) == 2) && c != '.')
    {
        i1 *= 10;
        if (c == 'W')
            i1 *= -1;
        //printf("a) curTun: %i min: %i max: %i code: %i str: %s i1: %i c: %c\n", currentTuner+1, min, max, source->Code(), cstr, i1, c);
        if (i1 >= min && i1 <= max)
            return true;
    }
    else if (sscanf(cstr, "S%i.%i%c", &i1, &i2, &c) == 3)
    {
        i1 *= 10;
        i1 += i2;
        if (c == 'W')
            i1 *= -1;
        //printf("b) curTun: %i min: %i max: %i code: %i str: %s i1: %i i2: %i c: %c\n", currentTuner+1, min, max, source->Code(), cstr, i1, i2, c);
        if (i1 >= min && i1 <= max)
            return true;
    }
    return false;
}

bool GetRotorStatus(int cardIndex, rotorStatus * rStat)
{
    if (TunerIsRotor(cardIndex))
    {
        fe_status_t status;
        if (GetSignal(cardIndex, &status) == false)
        {
            //printf("Error: GetSignal failed\n");
            return false;
        }

        // GA: Demo only, this needs to be done much better...
        if ((status >> 12) & 7)
        {                       // !=0 -> special netceiver status delivered
            *(int *)rStat = (status >> 8) & 7;  // 0: rotor not moving, 1: move to cached position, 2: move to unknown pos, 3: autofocus
            return true;
        }                       //else
        //printf("error: status: %x\n", status);
    }
    return false;
}

#ifndef DEVICE_ATTRIBUTES
char const *const FRONTEND_DEVICE = "/dev/dvb/adapter%d/frontend%d";

bool GetSignal(int cardIndex, fe_status_t * status)
{
    char dev[256];

    ::sprintf(dev, FRONTEND_DEVICE, cardIndex, 0);
    int fe = open(dev, O_RDONLY | O_NONBLOCK);
    if (fe < 0)
    {
        printf("error: couldn't open %s\n", FRONTEND_DEVICE);
        return false;
    }
    if (::ioctl(fe, FE_READ_STATUS, status) == -1)
    {
        //printf("error: ioctl(FE_READ_STATUS) failed\n");
        return false;
    }
    close(fe);
    return true;
}
#else
bool GetSignal(int cardIndex, fe_status_t * status)
{
    cDevice *cdev = cDevice::GetDevice(cardIndex);
    if (cdev)
    {
        uint64_t v = 0;
        cdev->GetAttribute("fe.status", &v);
        *status = (fe_status_t) v;
        return true;
    }
    return false;
}
#endif

bool TunerIsRotor(int nrTuner)
{
    cPlugin *p = cPluginManager::GetPlugin("netcvrotor");
    if (p)
    {
        void *data[2];
        data[0] = (void *)(nrTuner);
        p->Service("netcvrotor.tunerIsRotor", &data);
        return (int)data[1];
    }
    return false;
}

bool RotorGetMinMax(int nrTuner, int *min, int *max)
{
    cPlugin *p = cPluginManager::GetPlugin("netcvrotor");
    if (p)
    {
        struct RotorRequestData data;
        data.tunerNr = nrTuner;
        p->Service("netcvrotor.getRotorData", &data);
        if (data.available)
        {
            *min = data.minpos;
            *max = data.maxpos;
        }
        else
        {
            *min = -1;
            *max = -1;
        }
        return true;
    }
    return false;
}
