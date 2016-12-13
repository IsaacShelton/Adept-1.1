
#ifndef ERRORS_H_INCLUDED
#define ERRORS_H_INCLUDED

#include "die.h"

// General

#define FAILED_TO_CREATE(a)     ("ERROR: Failed to create '" + std::string(a) + "'")
#define UNKNOWN_OPTION(a)       ("ERROR: Unknown option '" + std::string(a) + "'")
#define UNKNOWN_MODULE(a)       ("ERROR: Unknown module '" + std::string(a) + "'")
#define EXPRESSION_IS_VOID      ("ERROR: Expression is void")
#define MAIN_IS_PRIVATE         ("ERROR: The 'main' function is private")

// Undefined

#define UNDECLARED_VARIABLE(a)  ("ERROR: Undeclared variable '" + std::string(a) + "'")
#define UNDECLARED_TYPE(a)      ("ERROR: Undeclared type '" + std::string(a) + "'")
#define UNDECLARED_FUNC(a)      ("ERROR: Undeclared function '" + std::string(a) + "'")
#define UNDECLARED_STRUCT(a)    ("ERROR: Undeclared struct '" + std::string(a) + "'")
#define UNDECLARED_MEMBER(a, b) ("ERROR: Undeclared member '" + std::string(a) + "' in structure '" + std::string(b) + "'")

// Unexpected

#define UNEXPECTED_KEYWORD(a)        ("ERROR: Unexpected keyword '" + std::string(a) + "'")
#define UNEXPECTED_KEYWORD_INEXPR(a) ("ERROR: Unexpected keyword '" + std::string(a) + "' in expression")

// Duplicate

#define DUPLICATE_FUNC(a)    ("ERROR: Duplicate function definition '" + std::string(a) + "'")
#define DUPLICATE_STRUCT(a)  ("ERROR: Duplicate type definition '" + std::string(a) + "'")
#define DUPLICATE_MEMBER(a)  ("ERROR: Duplicate member definition '" + std::string(a) + "'")
#define DUPLICATE_VARIBLE(a) ("ERROR: Duplicate variable definition '" + std::string(a) + "'")

// Validation

#define INCOMPATIBLE_TYPES_VAGUE ("ERROR: Expression contains incompatible types")
#define INCOMPATIBLE_TYPES(a,b)  ("ERROR: Expression contains incompatible types '" + std::string(a) + "' and '" + std::string(b) + "'")
#define INCOMPATIBLE_EXPRESSION  ("ERROR: Expression type is incompatible")
#define INCOMPATIBLE_WITH_RETURN ("ERROR: Expression type is incompatible with return type")

#endif // ERRORS_H_INCLUDED
