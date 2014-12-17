#include "timecounter.h"

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
cTimeCounter::cTimeCounter()
{
    CleanStop();
}

////////////////////////////////////////////////////////////////////////////////
//clears isPaused, elapsed time and sets lastUpdatedTime to current time
//
void cTimeCounter::CleanStart()
{
    isPaused = false;
    lastUpdatedTime = time(0); // current time
    elapsedTime = 0;
}

void cTimeCounter::CleanStop()
{
    isPaused = true;
    lastUpdatedTime = time(0); // current time
    elapsedTime = 0;
}

////////////////////////////////////////////////////////////////////////////////
// usually called after Pause()
//
void cTimeCounter::Start()
{
    if (!isPaused) return;

    isPaused = false;
    lastUpdatedTime = time(0);
    // discard the rest of the time
}

////////////////////////////////////////////////////////////////////////////////
// updates elapsed time and then lastUpdatedTime = time(0) current time
//
void cTimeCounter::Update()
{
    if(isPaused) return;

    elapsedTime += time(0) - lastUpdatedTime;
    lastUpdatedTime = time(0);
}

////////////////////////////////////////////////////////////////////////////////
// sets isPaused
//
void cTimeCounter::Pause()
{
    Update(); //update elapsed time 
    isPaused = true;
}

////////////////////////////////////////////////////////////////////////////////
// returns elapased time (in Seconds)
//
unsigned long cTimeCounter::Seconds()
{
    Update();
    return elapsedTime;
}
