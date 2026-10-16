#ifndef WAITER_HPP
#define WAITER_HPP

#include <stdint.h>

#define nVersion 1.0.2

class Waiter
{
public:
    // Default constructor, no setup needed here
    Waiter();

    void sleepUntil(long desired_waiting_time);
    
};

#endif