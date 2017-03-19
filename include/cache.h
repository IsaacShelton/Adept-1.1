
#ifndef CACHE_H_INCLUDED
#define CACHE_H_INCLUDED

#include <string>
#include <vector>
#include "program.h"

struct CacheEntry {
    std::string filename; // Must be absolute filename
    Program* program; // The syntax tree for that file
};

class CacheManager {
    public:
    std::vector<CacheEntry> cache;

    // Will create a new program if one with the same filename
    //   doesn't exist. If one with the same name is found, it
    //   will return NULL.
    Program* newProgram(const std::string&);

    // Returns pointer to the program with that filename,
    //   if none exists, NULL will be returned
    Program* getProgram(const std::string&);

    // Print all cache entries
    void print();

    // Will free all cached programs
    void free();
};

#endif // CACHE_H_INCLUDED
