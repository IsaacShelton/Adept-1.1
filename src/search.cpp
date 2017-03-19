
#include <string>
#include "../include/search.h"

int string_search(std::string names[], size_t array_size, std::string value){
    size_t first = 0;
    size_t last = array_size - 1;
    size_t middle;

    while (first <= last){
        middle = (first + last) / 2;

        if (names[middle] == value){
            return middle;
        }
        else if (names[middle] > value){
            last = middle - 1;
        }
        else {
            first = middle + 1;
        }
    }

    return -1;
}
