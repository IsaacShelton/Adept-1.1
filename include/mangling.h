
#ifndef MANGLING_H_INCLUDED
#define MANGLING_H_INCLUDED

#include <string>
#include <vector>
#include "program.h"

std::string mangle(const Function& func);
std::string mangle(const std::string&, const std::vector<std::string>&);
std::string mangle_filename(const std::string&);

#endif // MANGLING_H_INCLUDED
