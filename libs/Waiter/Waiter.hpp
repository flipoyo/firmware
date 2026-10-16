#ifndef WAITER_HPP
#define WAITER_HPP

//  #include <cstdint>

#define nVersion 1.0.0

class Waiter
{
public:
    // Default constructor, no setup needed here
    Waiter();

    void sleepUntil(long desired_waiting_time);
    
private:
    uint32_t starting_time;
};

#endif