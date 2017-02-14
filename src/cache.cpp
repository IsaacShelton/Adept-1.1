
#include "../include/cache.h"

Program* CacheManager::newProgram(const std::string& filename){
    // Will create a new program if one with the same filename
    //   doesn't exist. If one with the same name is found, it
    //   will return NULL.

    for(size_t i = 0; i != cache.size(); i++){
        if(cache[i].filename == filename){
            return NULL;
        }
    }

    // The program wasn't found, so create a new one
    Program* created_program = new Program(this);
    cache.push_back( CacheEntry{filename, created_program} );
    return created_program;
}

Program* CacheManager::getProgram(const std::string& filename){
    // Returns pointer to the program with that filename,
    //   if none exists, NULL will be returned

    for(size_t i = 0; i != cache.size(); i++){
        if(cache[i].filename == filename){
            return cache[i].program;
        }
    }

    return NULL;
}

void CacheManager::free(){
    // Will free all cached programs

    for(size_t i = 0; i != cache.size(); i++){
        delete cache[i].program;
    }
}
