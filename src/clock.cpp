
#include <iostream>
#include "../include/clock.h"

void Clock::start(){
    // Stores current time into 'start'
    start_time = std::chrono::steady_clock::now();
    last_time = std::chrono::steady_clock::now();
}

void Clock::remember(){
    // Stores current time into 'last'
    last_time = std::chrono::steady_clock::now();
}

double Clock::since(){
    // Returns seconds since 'last'
    std::chrono::time_point<std::chrono::steady_clock> finish = std::chrono::steady_clock::now();
    return (std::chrono::duration<double, std::milli> (finish - last_time).count())/1000;
}

double Clock::since_start(){
    // Returns time since clock 'start'
    std::chrono::time_point<std::chrono::steady_clock> finish = std::chrono::steady_clock::now();
    return (std::chrono::duration<double, std::milli> (finish - start_time).count())/1000;
}

void Clock::print_since(std::string message){
    int spaces = 18 - (int) message.length();
    std::string spaces_str;

    if(spaces >= 0) {
        for(int i = 0; i != spaces; i++) spaces_str += " ";
        std::cout << message << spaces_str << " (" << since() << "s)" << std::endl;
    } else {
        std::cout << message.substr(0, 15) << "... (" << since() << "s)" << std::endl;
    }
}

void Clock::print_since_start(std::string message){
    int spaces = 18 - (int) message.length();
    std::string spaces_str;

    if(spaces >= 0) {
        for(int i = 0; i != spaces; i++) spaces_str += " ";
        std::cout << message << spaces_str << " (" << since_start() << "s)" << std::endl;
    } else {
        std::cout << message.substr(0, 15) << "... (" << since_start() << "s)" << std::endl;
    }
}
