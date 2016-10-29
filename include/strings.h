
#ifndef STRINGS_H_INCLUDED
#define STRINGS_H_INCLUDED

#include <string>
#include <fstream>
#include <stdlib.h>

std::string to_str(int8_t);
std::string to_str(int16_t);
std::string to_str(int32_t);
std::string to_str(int64_t);
std::string to_str(double);
double to_double(std::string str);
int to_int(std::string str);

bool string_contains(std::string, std::string);
std::string string_get_until(std::string, std::string);
std::string string_get_until_or(std::string, std::string);
std::string string_iter_until(std::string&, size_t&, char);
std::string string_iter_until_or(std::string&, size_t&, std::string);
std::string string_itertest_until(std::string&, size_t, char);
std::string string_itertest_until_or(std::string&, size_t, std::string);
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
