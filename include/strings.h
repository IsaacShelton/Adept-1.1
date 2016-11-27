
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

int8_t to_byte(std::string str);
int16_t to_short(std::string str);
int32_t to_int(std::string str);
int64_t to_long(std::string str);
uint8_t to_ubyte(std::string str);
uint16_t to_ushort(std::string str);
uint32_t to_uint(std::string str);
uint64_t to_ulong(std::string str);
float to_float(std::string str);
double to_double(std::string str);

bool string_contains(std::string, std::string);
std::string string_get_until(std::string, std::string);
std::string string_get_until_or(std::string, std::string);
std::string string_iter_until(const std::string&, size_t&, char);
std::string string_iter_until_or(const std::string&, size_t&, std::string);
std::string string_itertest_until(const std::string&, size_t, char);
std::string string_itertest_until_or(const std::string&, size_t, std::string);
std::string string_delete_until(std::string, std::string);
std::string string_delete_until_or(std::string, std::string);
std::string string_delete_amount(std::string, int);
std::string string_get_until_last(std::string text, std::string character_set);

unsigned int string_count(std::string, std::string);
std::string string_replace(std::string, std::string, std::string);
std::string string_replace_all(std::string, std::string, std::string);

std::string string_kill_whitespace(std::string);
std::string string_kill_all_whitespace(std::string);
std::string string_kill_newline(std::string);
std::string string_flatten(std::string);
void string_iter_kill_whitespace(const std::string& code, size_t& i);

std::string delete_slash(std::string);
std::string string_upper(std::string);

std::string resource(std::string);
std::string resource_type(std::string);
bool is_identifier(std::string);
bool is_indent(std::string, size_t i = 0);

std::string filename_name(std::string);
std::string filename_path(std::string);
std::string filename_change_ext(std::string, std::string);
std::ifstream::pos_type file_size(std::string);

#endif // STRINGS_H_INCLUDED
