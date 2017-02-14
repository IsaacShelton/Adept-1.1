
#ifndef ERRORS_H_INCLUDED
#define ERRORS_H_INCLUDED

#include <string>
#include <vector>

// General
#define SUICIDE                 ("An unintended part of this compiler was reached :\\\n    (line #" + to_str(__LINE__) + " in " + std::string(__FILE__) + ")")
#define FAILED_TO_CREATE(a)     ("Failed to create '" + std::string(a) + "'")
#define UNKNOWN_OPTION(a)       ("Unknown option '" + std::string(a) + "'")
#define UNKNOWN_MODULE(a)       ("Unknown module '" + std::string(a) + "'")
#define EXPRESSION_IS_VOID      ("Expression is void")
#define MAIN_IS_PRIVATE         ("The 'main' function is private")
#define FAILED_TO_GET_USERNAME  ("Failed to get current username")
#define FAILED_TO_OPEN_FILE(a)  ("Failed to open file '" + a + "'")

// Undefined
#define UNDECLARED_VARIABLE(a)  ("Undeclared variable '" + std::string(a) + "'")
#define UNDECLARED_TYPE(a)      ("Undeclared type '" + std::string(a) + "'")
#define UNDECLARED_FUNC(a)      ("Undeclared function '" + std::string(a) + "'")
#define UNDECLARED_STRUCT(a)    ("Undeclared struct '" + std::string(a) + "'")
#define UNDECLARED_CLASS(a)     ("Undeclared class '" + std::string(a) + "'")
#define UNDECLARED_MEMBER(a, b) ("Undeclared member '" + std::string(a) + "' in structure '" + std::string(b) + "'")
#define UNDECLARED_CONST(a)     ("Undeclared constant '$" + std::string(a) + "'")

// Expected
#define EXPECTED_NAME_OF_TYPE        ("Expected typename")
#define EXPECTED_DEFINITION          ("Expected definition of a function or type")

// Unexpected
#define UNEXPECTED_KEYWORD(a)        ("Unexpected keyword '" + std::string(a) + "'")
#define UNEXPECTED_KEYWORD_INEXPR(a) ("Unexpected keyword '" + std::string(a) + "' in expression")
#define UNEXPECTED_OPERATOR(a)       ("Unexpected operator: "  + std::string(a))

// Duplicate
#define DUPLICATE_FUNC(a)    ("Duplicate function definition '" + std::string(a) + "'")
#define DUPLICATE_METHOD(a)  ("Duplicate method definition '" + std::string(a) + "'")
#define DUPLICATE_STRUCT(a)  ("Duplicate type definition '" + std::string(a) + "'")
#define DUPLICATE_CLASS(a)  ("Duplicate class definition '" + std::string(a) + "'")
#define DUPLICATE_MEMBER(a)  ("Duplicate member definition '" + std::string(a) + "'")
#define DUPLICATE_VARIBLE(a) ("Duplicate variable definition '" + std::string(a) + "'")

// Validation
#define INCOMPATIBLE_TYPES_VAGUE ("Expression contains incompatible types")
#define INCOMPATIBLE_TYPES(a,b)  ("Expression contains incompatible types '" + std::string(a) + "' and '" + std::string(b) + "'")
#define INCOMPATIBLE_EXPRESSION  ("Expression type is incompatible")
#define INCOMPATIBLE_WITH_RETURN ("Expression type is incompatible with return type")

class ErrorHandler {
    public:
    std::string filename;
    size_t line;

    ErrorHandler();
    ErrorHandler(std::string file);

    // Errors with filename & line
    void warn(std::string);
    void panic(std::string);

    // Errors without filename & line
    void warn_plain(std::string);
    void panic_plain(std::string);

    // Error message templates
    void panic_undeclared_func(const std::string&, const std::vector<std::string>&);
    void panic_undeclared_method(const std::string&, const std::string&, const std::vector<std::string>&);
};

#endif // ERRORS_H_INCLUDED
