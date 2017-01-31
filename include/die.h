
#ifndef DIE_H_INCLUDED
#define DIE_H_INCLUDED

#include <string>
#include <iostream>
#include "config.h"

#define die(a) { std::cerr << a << std::endl; return 1; }
#define next_index(i, s) { if(++i == s) { fail("Unexpected Statement Termination"); return 1; } }

void fail(const std::string& message);
void fail_filename(const Configuration& config, const std::string& message);

#endif // DIE_H_INCLUDED
