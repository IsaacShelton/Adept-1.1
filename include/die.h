
#ifndef DIE_H_INCLUDED
#define DIE_H_INCLUDED

#define USE_ENSURES

#include <string>
#include <iostream>
#include "config.h"

#define FAIL_UNEXPECTED_TERMINATION { fail("Unexpected Statement Termination :\\ "); return 1; }
#define die(a) { std::cerr << a << std::endl; return 1; }
#define next_index(i, s) { if(++i == s) FAIL_UNEXPECTED_TERMINATION; }
#define next_index_else(i, s, err) { if(++i == s) { fail(err); return 1; } }

#ifdef USE_ENSURES
void _ensure(const char*, const char*, int);
#define ensure(e) ((e) ? (void)0 : _ensure(#e, __FILE__, __LINE__))
#else
#define ensure(e) ((void)0)
#endif // USE_ENSURES

void fail(const std::string& message);
void fail_filename(const Configuration& config, const std::string& message);
void fail_filename(const std::string&, const std::string& message);

#endif // DIE_H_INCLUDED
