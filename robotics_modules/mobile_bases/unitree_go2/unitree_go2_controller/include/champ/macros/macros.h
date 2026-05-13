#ifndef MACROS_H
#define MACROS_H

#ifdef __unix__
    #include <chrono> // Standard C++ time library

    // Replace GNU extension with standard C++ chrono implementation
    #define time_us() \
        std::chrono::duration_cast<std::chrono::microseconds>( \
            std::chrono::steady_clock::now().time_since_epoch() \
        ).count()
#else    
    // Original implementation for microcontrollers (e.g., Teensy/Arduino)
    #include <Arduino.h>
    #define time_us() micros()
#endif

#define SECONDS_TO_MICROS 1000000

inline float mapFloat(float x, float in_min, float in_max, float out_min, float out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

#endif
