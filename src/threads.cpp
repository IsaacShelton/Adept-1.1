
#include "../include/threads.h"

int threads_int_result(std::vector<std::future<int>>& futures){
    // Waits on all futures and returns 1 if any of them return
    // something other than zero

    // ASYNC: All futures sent to this function are resolved

    bool error_occured = false;
    for(std::future<int>& future : futures){
        if(future.get() != 0) error_occured = true;
    }

    if(error_occured) return 1;
    return 0;
}
