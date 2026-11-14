Written in 2026 by Nabil El Houssein (26elhousse)

**Memory_monitor Library (`v1.0.0`)**

To be completely honest, I didn't work on this library myself, so what follows is simply my understanding of how these two tiny files (`Memory_monitor.hpp` and `Memory_monitor.cpp`) work after reading through the code.

This is a small debugging utility that provides a single function, `int freeMemory()`, to check how many bytes of free RAM are left on the Arduino at runtime (which is useful if you suspect `std::queue` or `String` objects are causing a memory leak and crashing the board).

**How `freeMemory()` works**

In C++ on a microcontroller, dynamic memory (`new`, `String`, `std::queue`) grows upward from the **heap**, while local variables inside functions grow downward from the **stack**. When the two collide, the Arduino crashes.

To measure the empty space remaining between the two:
1. `freeMemory()` declares a dummy local variable `char top;` right inside the function. Since local variables live on the stack, the memory address `&top` gives the current bottom edge of the stack.
2. Since our Arduino MKR WAN 1310 uses an ARM Cortex-M0+ processor (`__arm__` is defined), it calls `sbrk(0)` to get the current top edge of the heap (for older AVR Arduinos like the Uno, it falls back to `__brkval` or `__malloc_heap_start`).
3. It subtracts the heap address from the stack address (`&top - reinterpret_cast<char*>(sbrk(0))`) and returns the difference in bytes.

**Quick heads-up if you want to use it**

Right now, `Memory_monitor.hpp` only contains `#define nVersion 1.0.0` and doesn't actually declare `int freeMemory();`. So if you ever want to call `freeMemory()` inside `main.cpp` to print the available RAM to the serial monitor, you'll either need to add `int freeMemory();` into `Memory_monitor.hpp` first, or declare `extern int freeMemory();` at the top of the file where you call it.