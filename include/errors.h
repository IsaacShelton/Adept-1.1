
#ifndef ERRORS_H_INCLUDED
#define ERRORS_H_INCLUDED

#include "die.h"

// General

#define FAILED_TO_CREATE(a)     ("Failed to create '" + std::string(a) + "'")
#define UNKNOWN_OPTION(a)       ("Unknown option '" + std::string(a) + "'")
#define UNKNOWN_MODULE(a)       ("Unknown module '" + std::string(a) + "'")
#define EXPRESSION_IS_VOID      ("Expression is void")

// Undefined

#define UNDECLARED_VARIABLE(a)  ("Undeclared variable '" + std::string(a) + "'")
#define UNDECLARED_TYPE(a)      ("Undeclared type '" + std::string(a) + "'")
#define UNDECLARED_FUNC(a)      ("Undeclared function '" + std::string(a) + "'")
#define UNDECLARED_STRUCT(a)    ("Undeclared struct '" + std::string(a) + "'")
#define UNDECLARED_MEMBER(a, b) ("Undeclared member '" + std::string(a) + "' in structure '" + std::string(b) + "'")

// Unexpected

#define UNEXPECTED_KEYWORD(a)   ("Unexpected keyword '" + std::string(a) + "'")

// Duplicate

#define DUPLICATE_FUNC(a)    ("Duplicate function definition '" + std::string(a) + "'")
#define DUPLICATE_STRUCT(a)  ("Duplicate type definition '" + std::string(a) + "'")
#define DUPLICATE_MEMBER(a)  ("Duplicate member definition '" + std::string(a) + "'")
#define DUPLICATE_VARIBLE(a) ("Duplicate variable definition '" + std::string(a) + "'")

// Validation

#define INCOMPATIBLE_TYPES_VAGUE ("Expression contains incompatible types")
#define INCOMPATIBLE_TYPES(a,b)  ("Expression contains incompatible types '" + std::string(a) + "' and '" + std::string(b) + "'")
#define INCOMPATIBLE_EXPRESSION  ("Expression type is incompatible")
#define INCOMPATIBLE_WITH_RETURN ("Expression type is incompatible with return type")

#endif // ERRORS_H_INCLUDED
