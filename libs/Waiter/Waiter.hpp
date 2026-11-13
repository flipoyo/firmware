#ifndef WAITER_HPP
#define WAITER_HPP

#include <stdint.h>

#define nVersion 2.0.1

class Waiter
{
public:
    // Default constructor, no setup needed here
    Waiter();

    void sleepFor(long waiting_interval);
    
};

#endif