
#ifndef STRINGS_H_INCLUDED
#define STRINGS_H_INCLUDED

#include <string>
#include <fstream>
#include <stdlib.h>

std::string to_str(int8_t);
std::string to_str(int16_t);
std::string to_str(int32_t);
std::string to_str(int64_t);
std::string to_str(uint8_t);
std::string to_str(uint16_t);
std::string to_str(uint32_t);
std::string to_str(uint64_t);
std::string to_str(float);
std::string to_str(double);

int8_t to_byte(const std::string& str);
int16_t to_short(const std::string& str);
int32_t to_int(const std::string& str);
int64_t to_long(const std::string& str);
uint8_t to_ubyte(const std::string& str);
uint16_t to_ushort(const std::string& str);
uint32_t to_uint(const std::string& str);
uint64_t to_ulong(const std::string& str);
float to_float(const std::string& str);
double to_double(const std::string& str);

bool string_contains(const std::string&, const std::string&);
std::string string_get_until(const std::string&, const std::string&);
std::string string_get_until_or(const std::string&, const std::string&);
std::string string_iter_until(const std::string&, size_t&, char);
std::string string_iter_until_or(const std::string&, size_t&, const std::string&);
std::string string_itertest_until(const std::string&, size_t, char);
std::string string_itertest_until_or(const std::string&, size_t, const std::string&);
std::string string_delete_until(const std::string&, const std::string&);
std::string string_delete_until_or(const std::string&, const std::string&);
std::string string_delete_amount(const std::string&, int);
std::string string_get_until_last(const std::string& text, const std::string& character_set);

unsigned int string_count(const std::string&, const std::string&);
std::string string_replace(std::string, const std::string&, const std::string&);
std::string string_replace_all(std::string, const std::string&, const std::string&);

std::string string_kill_whitespace(std::string);
std::string string_kill_all_whitespace(std::string);
std::string string_kill_newline(std::string);
std::string string_flatten(std::string);
void string_iter_kill_whitespace(const std::string& code, size_t& i);

std::string delete_slash(std::string);
std::string string_upper(std::string);

std::string filename_name(std::string);
std::string filename_path(std::string);
std::string filename_change_ext(std::string, std::string);
std::ifstream::pos_type file_size(std::string);

#endif // STRINGS_H_INCLUDED
