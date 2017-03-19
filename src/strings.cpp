
#include <sstream>
#include <iostream>
#include <algorithm>
#include "../include/strings.h"

std::string to_str(int8_t dat){
    std::stringstream ss;
    ss << (int) dat;
    return ss.str();
}
std::string to_str(int16_t dat){
    std::stringstream ss;
    ss << dat;
    return ss.str();
}
std::string to_str(int32_t dat){
    std::stringstream ss;
    ss << dat;
    return ss.str();
}
std::string to_str(int64_t dat){
    std::stringstream ss;
    ss << dat;
    return ss.str();
}
std::string to_str(uint8_t dat){
    std::stringstream ss;
    ss << (int) dat;
    return ss.str();
}
std::string to_str(uint16_t dat){
    std::stringstream ss;
    ss << dat;
    return ss.str();
}
std::string to_str(uint32_t dat){
    std::stringstream ss;
    ss << dat;
    return ss.str();
}
std::string to_str(uint64_t dat){
    std::stringstream ss;
    ss << dat;
    return ss.str();
}
std::string to_str(float dat){
    std::stringstream ss;
    ss << dat;
    return ss.str();
}
std::string to_str(double dat){
    std::stringstream ss;
    ss << dat;
    return ss.str();
}

int8_t to_byte(const std::string& str){
    std::stringstream ss(str);
    int value;
    ss >> value;
    if (value > std::numeric_limits<int8_t>::max()) return 0;
    return (int8_t) value;
}
int16_t to_short(const std::string& str){
    int16_t val;
    if( ! (std::istringstream(str) >> val) ) val = 0;
    return val;
}
int32_t to_int(const std::string& str){
    int32_t val;
    if( ! (std::istringstream(str) >> val) ) val = 0;
    return val;
}
int64_t to_long(const std::string& str){
    int64_t val;
    if( ! (std::istringstream(str) >> val) ) val = 0;
    return val;
}
uint8_t to_ubyte(const std::string& str){
    std::stringstream ss(str);
    int value;
    ss >> value;
    if (value > std::numeric_limits<uint8_t>::max()) return 0;
    return (uint8_t) value;
}
uint16_t to_ushort(const std::string& str){
    uint16_t val;
    if( ! (std::istringstream(str) >> val) ) val = 0;
    return val;
}
uint32_t to_uint(const std::string& str){
    uint32_t val;
    if( ! (std::istringstream(str) >> val) ) val = 0;
    return val;
}
uint64_t to_ulong(const std::string& str){
    uint64_t val;
    if( ! (std::istringstream(str) >> val) ) val = 0;
    return val;
}
float to_float(const std::string& str){
    float val;
    if( ! (std::istringstream(str) >> val) ) val = 0;
    return val;
}
double to_double(const std::string& str){
    double val;
    if( ! (std::istringstream(str) >> val) ) val = 0;
    return val;
}

bool string_contains(const std::string& parent_string, const std::string& sub_string){
    if (parent_string.find(sub_string)==std::string::npos) { return false; }
    else return true;
}

std::string string_delete_until(const std::string& parent_string, const std::string& character){
    unsigned int index = 0;

    while(!( parent_string.substr(index,1)==character or index >= parent_string.length() )){
        index++;
    }

    if (index >= parent_string.length()) { return parent_string; }
    else return parent_string.substr(index,parent_string.length()-index);
}

//Gets text until character(s)
std::string string_get_until_or(const std::string& parent_string, const std::string& characters){
    unsigned int index = 0;

    while(!( string_contains(characters,parent_string.substr(index,1)) or index >= parent_string.length() )){
        index++;
    }

    if (index >= parent_string.length()){
        return parent_string;
    }
    else{
        return parent_string.substr(0,index);
    }
}

std::string string_iter_until(const std::string& code, size_t& i, char character){
    std::string content;
    while(code[i] != character and i < code.length()){
        content += code[i];
        i++;
    }
    return content;
}

std::string string_iter_until_or(const std::string& code, size_t& i, const std::string& characters){
    std::string content;
    while( i < code.length() and !string_contains(characters, code.substr(i, 1)) ){
        content += code[i];
        i++;
    }
    return content;
}

std::string string_itertest_until(const std::string& code, size_t i, char character){
    std::string content;
    while(code[i] != character and i < code.length()){
        content += code[i];
        i++;
    }
    return content;
}

std::string string_itertest_until_or(const std::string& code, size_t i, const std::string& characters){
    std::string content;
    while( i < code.length() and !string_contains(characters, code.substr(i, 1)) ){
        content += code[i];
        i++;
    }
    return content;
}


//Deletes text until character(s)
std::string string_delete_until_or(const std::string& parent_string, const std::string& characters){
    unsigned int index = 0;

    while(!( string_contains(characters,parent_string.substr(index,1)) or index >= parent_string.length() )){
        index++;
    }

    if (index >= parent_string.length()){
        return parent_string;
    }
    else{
        return parent_string.substr(index,parent_string.length()-index);
    }
}

std::string string_get_until_last(const std::string& text, const std::string& character_set){
    for(int i = text.length()-1; i >= 0; i--){
        if(text[i] == character_set[0]){
            return text.substr(i + 1, text.length()-i-1);
        }
    }
    return text;
}

//Deletes the amount of characters from the start of the string
std::string string_delete_amount(const std::string& str, int num){
    return str.substr(num,str.length()-num);
}

unsigned int string_count(const std::string& a, const std::string& character){
    unsigned int char_count = 0;
    for(unsigned int i = 0; i < a.length(); i++){
        if(a.substr(i,1)==character) char_count++;
    }
    return char_count;
}

//Replaces first string with new string
std::string string_replace(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return "";
    else
        return str.replace(start_pos, from.length(), to);
}

//Replaces all string(s) with new string
std::string string_replace_all(std::string str, const std::string& from, const std::string& to) {
    if(from.empty()) return "";

    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }

    return str;
}

//Kills spaces and tabs
std::string string_kill_whitespace(std::string str){
    int n = 0;

    while(!(str.substr(n,1)!=" " and str.substr(n,1)!="\t")){
        n++;
    }

    return string_delete_amount(str,n);
}

//Kills spaces, tabs, and newlines
std::string string_kill_all_whitespace(std::string str){
    int n = 0;

    while(!(str.substr(n,1)!=" " and str.substr(n,1)!="\t" and str.substr(n,1)!="\n"  and str.substr(n,1)!="\r")){
        n++;
    }

    return string_delete_amount(str,n);
}

//Kills newlines
std::string string_kill_newline(std::string str){
    int n = 0;

    while((str.substr(n,1)=="\n" and str.substr(n,1)!="\r")){
        n++;
    }

    return string_delete_amount(str,n);
}

// Remove all spaces from a string
std::string string_flatten(std::string str){
    for(unsigned int i = 0; i < str.length(); i++){
        if(str[i] == ' '){
            str.erase(str.begin() + i);
        }
    }

    return str;
}

void string_iter_kill_whitespace(const std::string& code, size_t& i){
    char character = code[i];
    while(!(character != ' ' and character != '\t')){
        character = code[++i];
    }
}

//Deletes Backslash if there is one
std::string delete_slash(std::string a){
    if (a.substr(0,1)=="\\" or a.substr(0,1)=="/"){
        return string_delete_amount(a, 1);
    }
    else {
        return a;
    }
}

//Turns the string uppercase
std::string string_upper(std::string a){
    std::transform(a.begin(), a.end(), a.begin(), ::toupper);
    return a;
}

//Checks to see if the string is an identifier
bool is_identifier(std::string what){
    char character;

    //Is it blank?
    if (what=="") return false;

    character = what[0];

    //Does it start with a number?
    if ( (int)character >= 48 and (int)character <= 57) return false;

    for(unsigned int i = 0; i < what.length(); i++){
        character = what[i];

        if( !((int)character>=48&&(int)character<=57)
        &&  !((int)character>=65&&(int)character<=90)
        &&  !((int)character>=97&&(int)character<=122)
        &&  !((int)character==95) ){
            return false;
        }
    }

    return true;
}

//Checks the see if the string is an indent character
bool is_indent(std::string what, size_t i){
    if(what.substr(i,1)=="\t" or what.substr(i,4)=="    ")
        return true;
    else
        return false;
}

//Gets the name of the file from path and filename
std::string filename_name(std::string a){
    if (a.find_last_of("\\/") == std::string::npos){
        return a;
    } else {
        return string_delete_amount(a.substr(a.find_last_of("\\/"), a.length() - a.find_last_of("\\/")),1);
    }
}

//Gets the path of the file from path and filename
std::string filename_path(std::string a){
    if (a.find_last_of("\\/") == std::string::npos){
        return "";
    } else {
        return a.substr(0, a.find_last_of("\\/")) + "/";
    }
}

//Gets the new filename from filename and new extension
std::string filename_change_ext(std::string filename, std::string ext_without_dot){
    std::string ext = string_get_until_last(filename,".");
    return filename.substr(0, filename.length()-ext.length()) + ext_without_dot;
}

std::ifstream::pos_type file_size(std::string size_filename){
    std::ifstream file_stream(size_filename.c_str(), std::ifstream::ate | std::ifstream::binary);
    unsigned int size_of_file = file_stream.tellg();
    file_stream.close();
    return size_of_file;
}
