#ifndef TIME_COUNTER_H
#define TIME_COUNTER_H

#include <time.h>


/// Counts number of seconds elapsed
//
class cTimeCounter
{
    
    private:
        unsigned long    elapsedTime; // the counter, in secs
        time_t           lastUpdatedTime;

        bool             isPaused;

    public:
        cTimeCounter();
        void Start();   // usually called after pause
        void Update();  // updates elapsed time and then lastUpdatedTime = time(0) current time
        void Pause();
        void CleanStart(); // clears isPaused, elapsed time and sets lastUpdatedTime to current time
        void CleanStop(); // sets isPaused, elapsed time = 0 and sets lastUpdatedTime to current time
        unsigned long Seconds(); // returns elapsed time in seconds

}; //end class

extern cTimeCounter timeCounter;

#endif
