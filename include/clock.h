
#ifndef CLOCK_H_INCLUDED
#define CLOCK_H_INCLUDED

#include <chrono>

class Clock {
    public:
    std::chrono::time_point<std::chrono::steady_clock> start_time; // Program Start Time
    std::chrono::time_point<std::chrono::steady_clock> last_time;  // Last Finish Time

    void start(); // Stores current time into 'start'
    void remember(); // Stores current time into 'last'
    double since(); // Returns seconds since 'last'
    double since_start(); // Returns time since clock 'start'
    void print_since(std::string, std::string file); // Print time since 'last'
    void print_since_start(std::string, std::string file); // Print time since 'start'
};

#endif // CLOCK_H_INCLUDED
