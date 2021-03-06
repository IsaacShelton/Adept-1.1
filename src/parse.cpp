
#include <unistd.h>
#include <iostream>
#include <boost/filesystem.hpp>
#include "../include/die.h"
#include "../include/parse.h"
#include "../include/lexer.h"
#include "../include/errors.h"
#include "../include/search.h"
#include "../include/strings.h"
#include "../include/buildapi.h"
#include "../include/mangling.h"
#include "llvm/Support/DynamicLibrary.h"

int parse(Config& config, TokenList* tokens, Program& program, ErrorHandler& errors){
    if(config.time_verbose) config.time_verbose_clock.remember();

    // Generate standard type aliases
    program.generate_type_aliases();

    if(config.add_build_api){
        // Add build script api if its a build script
        build_add_api(&program);
    }

    AttributeInfo attr_info;

    for(size_t i = 0; i != tokens->size(); i++){
        switch((*tokens)[i].id){
        case TOKENID_KEYWORD:
            if(parse_keyword(config, *tokens, program, i, attr_info, errors) != 0) return 1;
            break;
        case TOKENID_WORD:
            if(parse_word(config, *tokens, program, i, attr_info, errors) != 0) return 1;
            break;
        case TOKENID_NEWLINE:
            attr_info.reset();
            errors.line++;
            break;
        default:
            errors.panic(EXPECTED_DEFINITION);
            return 1;
        }
    }

    // Print Parser Time
    if(config.time_verbose and !config.silent){
        config.time_verbose_clock.print_since("PARSER DONE", filename_name(config.filename));
        config.time_verbose_clock.remember();
    }
    return 0;
}

int parse_word(Config& config, TokenList& tokens, Program& program, size_t& i, const AttributeInfo& attr_info, ErrorHandler& errors){
    // a_word <unknown syntax follows>
    //   ^

    std::string name = tokens[i].getString();
    next_index(i, tokens.size());

    switch(tokens[i].id){
    case TOKENID_WORD:
    case TOKENID_KEYWORD:
    case TOKENID_MULTIPLY:
    case TOKENID_BRACKET_OPEN:
        if(parse_global(config, tokens, program, i, name, attr_info, errors) != 0) return 1;
        break;
    default:
        errors.panic(EXPECTED_DEFINITION);
        return 1;
    }

    return 0;
}
int parse_keyword(Config& config, TokenList& tokens, Program& program, size_t& i, const AttributeInfo& attr_info, ErrorHandler& errors){
    // a_keyword <unknown syntax follows>
    //     ^

    std::string keyword = tokens[i].getString();
    next_index(i, tokens.size());

    const size_t accepted_keywords_size = 12;
    const std::string accepted_keywords[] = {
        "alias", "constant", "def", "dynamic", "enum", "external", "foreign", "import", "link", "private", "public", "struct"
    };

    size_t string_index = string_search(accepted_keywords, accepted_keywords_size, keyword);

    switch(string_index){
    case 0: // alias
        if(parse_type_alias(config, tokens, program, i, attr_info, errors) != 0) return 1;
        break;
    case 1: // constant
        if(parse_constant(config, tokens, program, i, attr_info, errors) != 0) return 1;
        break;
    case 2: // def
        if(parse_function(config, tokens, program, i, attr_info, errors) != 0) return 1;
        break;
    case 3: { // dynamic
            if(tokens[i].id != TOKENID_STRING){
                errors.panic("Expected name of dynamic library after 'dynamic' keyword");
                return 1;
            }

            // Don't load dynamic libraries if we know this code won't run jit
            if(!config.load_dyn) return 0;

            std::string name = tokens[i].getString();
            std::string local_file = filename_path(config.filename) + name;
            std::string public_file = "C:/Users/" + config.username + "/.adept/lib/" + name;

            if( access(local_file.c_str(), F_OK) != -1 ){
                name = local_file;
            }
            else if( access(public_file.c_str(), F_OK) != -1 ){
                name = public_file;
            }
            else if( access(name.c_str(), F_OK) == -1 ){
                // File doesn't exist globally either
                errors.panic( UNKNOWN_MODULE(name) );
                return 1;
            }

            if( llvm::sys::DynamicLibrary::LoadLibraryPermanently(name.c_str()) ){
                errors.panic("Failed to load dynamic library '" + name + "'");
                return 1;
            }

            break;
        }
    case 4: // enum
        if(parse_enum(config, tokens, program, i, attr_info, errors) != 0) return 1;
        break;
    case 5: // external
        if(parse_attribute(config, tokens, program, --i, errors) != 0) return 1;
        break;
    case 6: // foreign
        if(parse_external(config, tokens, program, i, attr_info, errors) != 0) return 1;
        break;
    case 7: // import
        if(parse_import(config, tokens, program, i, attr_info, errors) != 0) return 1;
        break;
    case 8: // link
        if(parse_lib(config, tokens, program, i, errors) != 0) return 1;
        break;
    case 9: // private
    case 10: // public
        if(parse_attribute(config, tokens, program, --i, errors) != 0) return 1;
        break;
    case 11: // struct
        if(parse_struct(config, tokens, program, i, attr_info, errors) != 0) return 1;
        break;
    default:
        errors.panic( UNEXPECTED_KEYWORD(keyword) );
        return 1;
    }

    return 0;
}
int parse_enum(Config& config, TokenList& tokens, Program& program, size_t& i, const AttributeInfo& attr_info, ErrorHandler& errors){
    // enum enum_name { ... }
    //          ^

    std::string name = tokens[i].getString();
    std::vector<EnumField> fields;

    next_index(i, tokens.size());
    uint16_t token = tokens[i].id;

    if(token != TOKENID_BEGIN){
        errors.panic("Expected '{' or '=' after name of the type");
        return 1;
    }

    next_index(i, tokens.size());
    while(tokens[i].id == TOKENID_NEWLINE){
        errors.line++;
        next_index(i, tokens.size());
    }

    while(tokens[i].id != TOKENID_END){
        if(tokens[i].id != TOKENID_WORD){
            errors.panic("Expected enum field");
            return 1;
        }

        std::string field_name = tokens[i].getString();

        next_index(i, tokens.size());
        while(tokens[i].id == TOKENID_NEWLINE){
            errors.line++;
            next_index(i, tokens.size());
        }

        if(tokens[i].id != TOKENID_END){
            if(tokens[i].id != TOKENID_NEXT){
                errors.panic("Expected ',' after enum field");
                return 1;
            }
            next_index(i, tokens.size());
            while(tokens[i].id == TOKENID_NEWLINE){
                errors.line++;
                next_index(i, tokens.size());
            }
        }

        fields.push_back( EnumField(field_name, NULL, errors) );
    }

    program.enums.push_back( Enum(name, fields, attr_info.is_public, &program.origin_info) );
    return 0;
}
int parse_struct(Config& config, TokenList& tokens, Program& program, size_t& i, const AttributeInfo& attr_info, ErrorHandler& errors){
    // struct struct_name { ... }
    //             ^

    std::string name = tokens[i].getString();

    if(name == "llvm"){
        errors.panic("The name 'llvm' is reserved");
        return 1;
    }

    // Make sure that is hasn't been already declared
    for(size_t i = 0; i != program.structures.size(); i++){
        if(name == program.structures[i].name){
            errors.panic(DUPLICATE_STRUCT(name));
            return 1;
        }
    }

    for(size_t i = 0; i != program.type_aliases.size(); i++){
        if(name == program.type_aliases[i].alias){
            errors.panic(DUPLICATE_DEFINITION(name));
            return 1;
        }
    }

    // Allocate a new struct in 'program.structures'
    std::vector<Struct>* structures = &program.structures;
    structures->resize(structures->size()+1);

    // Store a reference to the created struct
    Struct* structure = &( (*structures)[structures->size()-1] );
    size_t struct_offset_plus_one = structures->size();

    // Fill in the struct data
    structure->name = name;
    structure->flags = (attr_info.is_public) ? STRUCT_PUBLIC : 0x00;
    structure->origin = &program.origin_info;

    // Mark struct packed if 'packed' attribue is specified
    if(attr_info.is_packed) structure->flags |= STRUCT_PACKED;

    // Skip over struct name and '{'
    next_index(i, tokens.size());
    if(tokens[i].id != TOKENID_BEGIN){
        errors.panic("Expected '{' after struct name");
        return 1;
    }
    next_index(i, tokens.size());

    // Parse leading newlines
    while(tokens[i].id == TOKENID_NEWLINE){
        errors.line++;
        next_index(i, tokens.size());
    }

    // Parse contents of the struct
    while(tokens[i].id != TOKENID_END){
        AttributeInfo member_attr(false);

        // Encountered a keyword inside struct definition
        if(tokens[i].id == TOKENID_KEYWORD){
            std::string keyword = tokens[i].getString();
            bool continue_loop = false;

            while(tokens[i].id == TOKENID_KEYWORD){
                keyword = tokens[i].getString();

                if(keyword == "def"){
                    // Parse the method
                    next_index(i, tokens.size());
                    if(parse_method(config, tokens, program, i, structure, struct_offset_plus_one, member_attr, errors) != 0) return 1;
                    next_index(i, tokens.size());

                    // Skip normal member stuff
                    continue_loop = true;
                    break;
                }
                else if(keyword == "public"){
                    member_attr.is_public = true;
                }
                else if(keyword == "private"){
                    member_attr.is_public = false;
                }
                else if(keyword == "static"){
                    member_attr.is_static = true;
                }
                else {
                    errors.panic( UNEXPECTED_KEYWORD(keyword) );
                    return 1;
                }

                next_index(i, tokens.size());
            }

            // Go to start of member loop
            if(continue_loop){
                while(tokens[i].id == TOKENID_NEWLINE){
                    errors.line++;
                    next_index(i, tokens.size());
                }
                continue;
            }
        }

        // Expect name of member after attributes
        if(tokens[i].id != TOKENID_WORD){
            errors.panic("Expected member name of struct");
            return 1;
        }

        std::string name = tokens[i].getString();
        std::string type;

        next_index(i, tokens.size());
        if(parse_type(config, tokens, program, i, type, errors) != 0) return 1;

        next_index(i, tokens.size());

        char member_flags = 0x00;
        if(member_attr.is_public) member_flags |= STRUCTFIELD_PUBLIC;
        if(member_attr.is_static) member_flags |= STRUCTFIELD_STATIC;
        structure->members.push_back( StructField{name, type, member_flags} );

        while(tokens[i].id == TOKENID_NEWLINE){
            errors.line++;
            next_index(i, tokens.size());
        }
    }

    return 0;
}
int parse_type_alias(Config& config, TokenList& tokens, Program& program, size_t& i, const AttributeInfo& attr_info, ErrorHandler& errors){
    // type type_alias = some_type
    //          ^

    std::string name = tokens[i].getString();
    std::vector<Field> members;

    if(name == "llvm"){
        errors.panic("The name 'llvm' is reserved");
        return 1;
    }

    // Make sure that is hasn't been already declared
    for(size_t i = 0; i != program.type_aliases.size(); i++){
        if(name == program.type_aliases[i].alias){
            errors.panic(DUPLICATE_DEFINITION(name));
            return 1;
        }
    }
    for(size_t i = 0; i != program.structures.size(); i++){
        if(name == program.structures[i].name){
            errors.panic(DUPLICATE_DEFINITION(name));
            return 1;
        }
    }

    next_index(i, tokens.size());
    uint16_t token = tokens[i].id;

    if(token != TOKENID_ASSIGN){
        errors.panic("Expected '=' after name of type alias");
        return 1;
    }

    next_index(i, tokens.size());
    std::string binding;

    if(parse_type(config, tokens, program, i, binding, errors) != 0) return 1;
    program.type_aliases.push_back( TypeAlias(name, binding, attr_info.is_public, &program.origin_info) );
    return 0;
}
int parse_function(Config& config, TokenList& tokens, Program& program, size_t& i, const AttributeInfo& attr_info, ErrorHandler& errors){
    // def func_name() ret_type { <some code> }   |   def func_name(name a_type) (one_type, two_type) { <some code> }
    //         ^

    bool multiple_return = false;
    std::string name = tokens[i].getString();
    next_index(i, tokens.size());

    if(tokens[i].id != TOKENID_OPEN){
        errors.panic("Expected open after function name");
        return 1;
    }
    next_index(i, tokens.size());

    std::vector<Field> arguments;
    std::string return_type;
    std::vector<std::string> extra_return_types;
    StatementList statements;
    StatementList defer_statements;
    bool is_variable_args = false;

    while(tokens[i].id != TOKENID_CLOSE){
        if(is_variable_args){
            errors.panic("Variable arguments can only be at the end of the argument list");
            return 1;
        }

        if(tokens[i].id != TOKENID_WORD){
            errors.panic("Expected argument name");
            return 1;
        }

        std::string name = tokens[i].getString();
        std::string type;
        next_index(i, tokens.size());

        if(tokens[i].id == TOKENID_ELLIPSIS){
            is_variable_args = true;
            next_index(i, tokens.size());
        }

        if(parse_type(config, tokens, program, i, type, errors) != 0) return 1;
        next_index(i, tokens.size());

        if(is_variable_args) type = "[]" + type;
        arguments.push_back( Field{name, type} );

        if(tokens[i].id == TOKENID_NEXT){
            next_index(i, tokens.size());
        } else if(tokens[i].id != TOKENID_CLOSE){
            errors.panic("Expected ',' or ')' after typename inside listing of multiple return types");
            return 1;
        }
    }

    if(tokens[i].id == TOKENID_BEGIN){
        errors.panic("Expected function return type after function name");
        return 1;
    }

    next_index(i, tokens.size());

    if(tokens[i].id == TOKENID_OPEN){
        multiple_return = true;
        next_index(i, tokens.size());

        while(tokens[i].id != TOKENID_CLOSE){
            return_type = "";
            if(parse_type(config, tokens, program, i, return_type, errors) != 0) return 1;
            extra_return_types.push_back(return_type);
            next_index(i, tokens.size());

            if(tokens[i].id == TOKENID_NEXT){
                next_index(i, tokens.size());
            } else if(tokens[i].id != TOKENID_CLOSE){
                errors.panic("Expected ',' or ')' after typename inside listing of multiple return types");
                return 1;
            }
        }

        if(extra_return_types.size() == 0){
            errors.panic("Must return at least one value when using multiple return types");
            return 1;
        }

        next_index(i, tokens.size());
    }
    else {
        if(parse_type(config, tokens, program, i, return_type, errors) != 0) return 1;
        next_index(i, tokens.size());
    }

    if(tokens[i].id != TOKENID_BEGIN){
        errors.panic("Expected block after function declaration");
        return 1;
    }
    next_index(i, tokens.size());

    if(parse_block(config, tokens, program, statements, defer_statements, i, errors) != 0) return 1;

    // If function not terminated, make sure to add defer statements
    if(statements.size() != 0){
        // If we haven't already terminated the function and dumped the defered statements, do it now
        if(!(statements[statements.size()-1]->flags & STMT_TERMINATOR)){
            for(size_t i = defer_statements.size(); i-- > 0;){
                statements.push_back( defer_statements[i]->clone() );
            }
        }
    }

    for(Statement* statement : defer_statements){
        // delete defered statements, they are no longer needed
        delete statement;
    }

    if(multiple_return){
        // return_type isn't used, the function will technically return void in the actual implementation
        program.functions.push_back( Function(name, arguments, extra_return_types, statements, attr_info.is_public, false, attr_info.is_stdcall, attr_info.is_external, is_variable_args, &program.origin_info) );
    }
    else {
        program.functions.push_back( Function(name, arguments, return_type, statements, attr_info.is_public, false, attr_info.is_stdcall, attr_info.is_external, is_variable_args, &program.origin_info) );
    }

    return 0;
}
int parse_method(Config& config, TokenList& tokens, Program& program, size_t& i, Struct* structure, size_t struct_offset_plus_one, const AttributeInfo& attr_info, ErrorHandler& errors){
    // def method_name() ret_type { <some code> }
    //           ^

    // NOTE: 'Struct* structure' should be a valid pointer when calling this function (if not a crash will occur)

    // NOTE: 'Struct* structure' should only used for adding the created method to the struct,
    //           if it is used for accessing things like members, not all of them may be present,
    //           or none of them at all

    bool multiple_return = false;
    std::string name = tokens[i].getString();
    next_index(i, tokens.size());

    if(tokens[i].id != TOKENID_OPEN){
        errors.panic("Expected open after method name");
        return 1;
    }
    next_index(i, tokens.size());

    std::vector<Field> arguments;
    std::string return_type;
    std::vector<std::string> extra_return_types;
    StatementList statements;
    StatementList defer_statements;
    bool is_variable_args = false;

    while(tokens[i].id != TOKENID_CLOSE){
        if(tokens[i].id != TOKENID_WORD){
            errors.panic("Expected argument name");
            return 1;
        }

        std::string name = tokens[i].getString();
        std::string type;
        next_index(i, tokens.size());

        if(tokens[i].id == TOKENID_ELLIPSIS){
            is_variable_args = true;
            next_index(i, tokens.size());
        }

        if(parse_type(config, tokens, program, i, type, errors) != 0) return 1;
        next_index(i, tokens.size());

        if(tokens[i].id != TOKENID_CLOSE) next_index(i, tokens.size());
        if(is_variable_args) type = "[]" + type;
        arguments.push_back( Field{name, type} );
        if(is_variable_args) break;
    }

    if(tokens[i].id == TOKENID_BEGIN){
        errors.panic("Expected method return type after method name");
        return 1;
    }

    next_index(i, tokens.size());

    if(tokens[i].id == TOKENID_OPEN){
        multiple_return = true;
        next_index(i, tokens.size());

        while(tokens[i].id != TOKENID_CLOSE){
            return_type = "";
            if(parse_type(config, tokens, program, i, return_type, errors) != 0) return 1;
            extra_return_types.push_back(return_type);
            next_index(i, tokens.size());

            if(tokens[i].id == TOKENID_NEXT){
                next_index(i, tokens.size());
            } else if(tokens[i].id != TOKENID_CLOSE){
                errors.panic("Expected ',' or ')' after typename inside listing of multiple return types");
                return 1;
            }
        }

        if(extra_return_types.size() == 0){
            errors.panic("Must return at least one value when using multiple return types");
            return 1;
        }

        next_index(i, tokens.size());
    }
    else {
        if(parse_type(config, tokens, program, i, return_type, errors) != 0) return 1;
        next_index(i, tokens.size());

        if(tokens[i].id == TOKENID_NEXT){
            errors.panic("When defining a method that returns multiple values, the return types must be surrounded by parentheses");
            return 1;
        }
    }

    if(return_type == ""){
        errors.panic("No return type specified for method '" + structure->name + "." + name + "'");
        return 1;
    }

    next_index(i, tokens.size());
    if(parse_block(config, tokens, program, statements, defer_statements, i, errors) != 0) return 1;

    // If method not terminated, make sure to add defer statements
    if(statements.size() != 0){
        // If we haven't already terminated the function and dumped the defered statements, do it now
        if(!(statements[statements.size()-1]->flags & STMT_TERMINATOR)){
            for(size_t i = defer_statements.size(); i-- > 0;){
                statements.push_back( defer_statements[i]->clone() );
            }
        }
    }

    for(Statement* statement : defer_statements){
        // delete defered statements, they are no longer needed
        delete statement;
    }

    if(multiple_return){
        Function created_method(name, arguments, extra_return_types, statements, attr_info.is_public, attr_info.is_static, attr_info.is_stdcall, false, is_variable_args, &program.origin_info);
        created_method.parent_struct_offset = struct_offset_plus_one;
        structure->methods.push_back( std::move(created_method) );
        return 0;
    }
    else {
        Function created_method(name, arguments, return_type, statements, attr_info.is_public, attr_info.is_static, attr_info.is_stdcall, false, is_variable_args, &program.origin_info);
        created_method.parent_struct_offset = struct_offset_plus_one;
        structure->methods.push_back( std::move(created_method) );
        return 0;
    }
}
int parse_external(Config& config, TokenList& tokens, Program& program, size_t& i, const AttributeInfo& attr_info, ErrorHandler& errors){
    // foreign func_name() ret_type
    //            ^

    std::string name = tokens[i].getString();
    next_index(i, tokens.size());

    if(tokens[i].id != TOKENID_OPEN){
        errors.panic("Expected open after foreign function name");
        return 1;
    }
    next_index(i, tokens.size());

    std::vector<std::string> arguments;
    std::string return_type;

    while(tokens[i].id != TOKENID_CLOSE){
        std::string type;

        if(parse_type(config, tokens, program, i, type, errors) != 0) return 1;
        next_index(i, tokens.size());

        if(tokens[i].id == TOKENID_NEXT){
            next_index(i, tokens.size());
        }
        else if(tokens[i].id != TOKENID_CLOSE){
            errors.panic("Expected ',' after type '" + type + "'");
        }

        arguments.push_back(type);
    }

    next_index(i, tokens.size());
    if(parse_type(config, tokens, program, i, return_type, errors) != 0) return 1;
    next_index(i, tokens.size());

    errors.line++;
    program.externs.push_back( External(name, arguments, return_type, attr_info.is_public, false, attr_info.is_stdcall, false, &program.origin_info) );
    return 0;
}
int parse_attribute(Config& config, TokenList& tokens, Program& program, size_t& i, ErrorHandler& errors){
    // <attribute> <unknown syntax follows>
    //      ^

    std::string keyword;
    AttributeInfo attr_info;

    while(tokens[i].id == TOKENID_KEYWORD){
        keyword = tokens[i].getString();

        if(keyword == "public"){
            attr_info.is_public = true;
            next_index(i, tokens.size());
        }
        else if(keyword == "private"){
            attr_info.is_public = false;
            next_index(i, tokens.size());
        }
        else if(keyword == "external"){
            attr_info.is_external = true;
            next_index(i, tokens.size());
        }
        else if(keyword == "static"){
            attr_info.is_static = true;
            next_index(i, tokens.size());
        }
        else if(keyword == "packed"){
            attr_info.is_packed = true;
            next_index(i, tokens.size());
        }
        else if(keyword == "stdcall"){
            attr_info.is_stdcall = true;
            next_index(i, tokens.size());
        }
        else {
            // Not an attribute, continue on
            break;
        }
    }

    switch(tokens[i].id){
    case TOKENID_KEYWORD:
        if(parse_keyword(config, tokens, program, i, attr_info, errors) != 0) return 1;
        break;
    case TOKENID_WORD:
        if(parse_word(config, tokens, program, i, attr_info, errors) != 0) return 1;
        break;
    default:
        errors.panic(EXPECTED_DEFINITION);
        return 1;
    }

    return 0;
}
int parse_import(Config& config, TokenList& tokens, Program& program, size_t& i, const AttributeInfo& attr_info, ErrorHandler& errors){
    // import PackageName
    //             ^

    using namespace boost::filesystem;

    if(tokens[i].id != TOKENID_STRING){
        errors.panic("Expected module name after 'import'");
        return 1;
    }

    std::string name = tokens[i].getString();
    TokenList* import_tokens;
    Config* import_config = new Config(config);
    Program* import_program;
    std::string target_bc;
    std::string target_obj;

    import_config->link = false;
    import_config->obj = false;
    import_config->bytecode = false;

    std::string local_file = filename_path(config.filename) + name;
    std::string public_file = "C:/Users/" + config.username + "/.adept/import/" + name;
    std::string errors_name;

    if( access(local_file.c_str(), F_OK) != -1 ){
        errors_name = filename_name(local_file);
        name = local_file;
    }
    else if( access(public_file.c_str(), F_OK) != -1 ){
        errors_name = "<adept import>/" + name;
        name = public_file;
    }
    else {
        errors.panic( UNKNOWN_MODULE(name) );
        delete import_config;
        return 1;
    }

    // Get the full filename of the source file that we want to compile
    std::string source_filename = absolute(path(name), current_path()).string();
    source_filename = string_replace_all(source_filename, "\\", "/");

    ErrorHandler error_handler(errors_name);
    import_config->filename = source_filename;
    import_program = program.parent_manager->newProgram(source_filename);

    if(import_program != NULL){
        import_tokens = new TokenList;

        if( tokenize(*import_config, source_filename, import_tokens, error_handler) != 0 ){
            free_tokens(*import_tokens);
            delete import_tokens;
            delete import_config;
            return 1;
        }

        if( parse(*import_config, import_tokens, *import_program, error_handler) != 0 ){
            delete import_config;
            return 1;
        }

        free_tokens(*import_tokens);
        delete import_tokens;
    }
    else {
        import_program = program.parent_manager->getProgram(source_filename);
    }

    std::string mangled_name = mangle_filename(source_filename);
    target_obj  = (config.obj)      ? (mangled_name + ".o") : "C:/Users/" + config.username + "/.adept/obj/tmp/" + mangled_name + ".o";
    target_bc   = (config.bytecode) ? (mangled_name + ".bc")  : "C:/Users/" + config.username + "/.adept/obj/tmp/" + mangled_name + ".bc";

    // Import the declarations
    if(program.import_merge(import_config, *import_program, attr_info.is_public, errors) != 0){
        delete import_config;
        return 1;
    }
    program.imports.push_back( ImportDependency(source_filename, attr_info.is_public) );

    // Exit if native dependency already exists
    for(size_t i = 0; i != program.dependencies.size(); i++){
        if(program.dependencies[i].target_obj == target_obj){
            delete import_config;
            return 0;
        }
    }

    bool is_nothing = (import_program->functions.size() == 0 and import_program->structures.size() == 0 and import_program->globals.size() == 0);
    program.dependencies.push_back( ModuleDependency(source_filename, target_bc, target_obj, import_config, is_nothing) );
    return 0;
}
int parse_lib(Config& config, TokenList& tokens, Program& program, size_t& i, ErrorHandler& errors){
    // link "filename.a"
    //           ^

    if(tokens[i].id != TOKENID_STRING){
        errors.panic("Expected library after 'link'");
        return 1;
    }

    std::string name = tokens[i].getString();
    std::string local_file = filename_path(config.filename) + name;
    std::string public_file = "C:/Users/" + config.username + "/.adept/lib/" + name;

    if( access(local_file.c_str(), F_OK) != -1 ){
        name = local_file;
    }
    else if( access(public_file.c_str(), F_OK) != -1 ){
        name = public_file;
    }
    else {
        errors.panic( UNKNOWN_MODULE(name) );
        return 1;
    }

    name = string_replace_all(name, "\\", "/");
    program.extra_libs.push_back(name);
    return 0;
}
int parse_constant(Config& config, TokenList& tokens, Program& program, size_t& i, const AttributeInfo& attr_info, ErrorHandler& errors){
    // constant $CONSTANT_NAME <expression>
    //                 ^

    if(tokens[i].id != TOKENID_CONSTANT){
        errors.panic("Expected constant name after 'constant' keyword");
        return 1;
    }

    PlainExp* value;
    std::string name = tokens[i].getString();
    next_index(i, tokens.size());

    if(parse_expression(config, tokens, program, i, &value, errors) != 0) return 1;
    program.constants.push_back( Constant(name, value, attr_info.is_public, &program.origin_info) );
    i--;
    return 0;
}
int parse_global(Config& config, TokenList& tokens, Program& program, size_t& i, const std::string& name, const AttributeInfo& attr_info, ErrorHandler& errors){
    // name str = "Hello World"
    //       ^

    std::string type;
    std::vector<Global>* globals = &program.globals;
    if(parse_type(config, tokens, program, i, type, errors) != 0) return 1;
    next_index(i, tokens.size());

    // Make sure that is hasn't been already declared
    for(size_t i = 0; i != globals->size(); i++){
        if(name == (*globals)[i].name){
            errors.panic(DUPLICATE_GLOBAL(name));
            return 1;
        }
    }

    if(tokens[i].id == TOKENID_ASSIGN){
        errors.panic("Global variables can't be initialized outside of a procedure");
        return 1;
    }
    else {
        // Safe because previously did 'next_index'
        i--;

        // Append the global to the list of globals
        globals->resize(globals->size() + 1);
        Global* global = &( (*globals)[globals->size()-1] );
        global->name = name;
        global->type = type;

        char global_flags = 0x00;
        if(attr_info.is_public) global_flags   |= GLOBAL_PUBLIC;
        if(attr_info.is_external) global_flags |= GLOBAL_EXTERNAL;
        global->flags = global_flags;
        global->origin = &program.origin_info;
        global->errors = errors;
    }

    return 0;
}
int parse_type(Config& config, TokenList& tokens, Program& program, size_t& i, std::string& output_type, ErrorHandler& errors){
    //  **some_type   |   def (int, int) int   |   another_type   |   [] int   |   *@TypeName
    //  ^                  ^                            ^             ^            ^

    std::string keyword;
    uint16_t prefix_token_id = tokens[i].id;

    while(prefix_token_id == TOKENID_MULTIPLY or prefix_token_id == TOKENID_BRACKET_OPEN){
        if(prefix_token_id == TOKENID_MULTIPLY){
            output_type += "*";
        }
        else if(prefix_token_id == TOKENID_BRACKET_OPEN){
            next_index(i, tokens.size());

            if(tokens[i].id == TOKENID_BRACKET_CLOSE){
                output_type += "[]";
            }
            else if(tokens[i].id == TOKENID_INT){
                int int_value = tokens[i].getInt();
                next_index(i, tokens.size());

                if(tokens[i].id != TOKENID_BRACKET_CLOSE){
                    errors.panic("Expected ']' after fixed array length in type");
                    return 1;
                }

                output_type += "[" + to_str(int_value) + "]";
            }
            else {
                errors.panic("Expected ']' or integer literal after '[' in type");
                return 1;
            }
        }

        next_index(i, tokens.size());
        prefix_token_id = tokens[i].id;
    }

    if(prefix_token_id == TOKENID_KEYWORD){
        keyword = tokens[i].getString();

        if(keyword == "def" or keyword == "stdcall"){
            if(parse_type_funcptr(config, tokens, program, i, output_type, keyword, errors) != 0) return 1;
        }
        else {
            errors.panic(UNEXPECTED_KEYWORD(keyword));
            return 1;
        }
    }
    else if(prefix_token_id == TOKENID_WORD){
        output_type += tokens[i].getString();
    }
    else {
        errors.panic(EXPECTED_NAME_OF_TYPE);
        return 1;
    }

    return 0;
}
int parse_type_funcptr(Config& config, TokenList& tokens, Program& program, size_t& i, std::string& output_type, const std::string& keyword, ErrorHandler& errors){
    // def (int, int) int     |     stdcall def (... string) void
    //  ^                              ^

    // If the function pointed to by required the stdcall call convention, add that to the typename
    if(keyword == "stdcall"){
        next_index(i, tokens.size());

        if(tokens[i].id != TOKENID_KEYWORD){
            errors.panic("Expected keyword after 'stdcall'");
            return 1;
        }

        if(tokens[i].getString() != "def"){
            errors.panic("Expected keyword 'def' after keyword 'stdcall'");
            return 1;
        }

        next_index(i, tokens.size());
        output_type += "stdcall def(";
    }
    else { // If not stdcall, the only other possible keyword is 'def'
        next_index(i, tokens.size());
        output_type += "def(";
    }

    if(tokens[i].id != TOKENID_OPEN){
        errors.panic("Expected '(' after keyword 'def' in function pointer type");
        return 1;
    }

    std::vector<std::string> arguments; // Arguments of function pointed to
    std::string return_type; // Return type of function pointed to
    std::string type; // Temp storage
    bool is_var_args = false; // Whether function pointed to is var args

    next_index(i, tokens.size());
    while(tokens[i].id != TOKENID_CLOSE){
        if(is_var_args){
            errors.panic("Variable arguments can only be at the end of the argument list");
            return 1;
        }

        if(tokens[i].id == TOKENID_ELLIPSIS){
            is_var_args = true;
            next_index(i, tokens.size());
        }

        type = "";
        if(parse_type(config, tokens, program, i, type, errors) != 0) return 1;
        next_index(i, tokens.size());

        if(tokens[i].id == TOKENID_NEXT){
            next_index(i, tokens.size());
        } else if(tokens[i].id != TOKENID_CLOSE) {
            errors.panic("Expected ',' or ')' after typename in function typename");
            return 1;
        }

        arguments.push_back(type);
    }

    for(size_t arg_index = 0; arg_index != arguments.size(); arg_index++){
        if(arg_index + 1 != arguments.size()){
            output_type += arguments[arg_index] + ", ";
        } else if(is_var_args){
            output_type += "..." + arguments[arg_index];
        }
        else {
            output_type += arguments[arg_index];
        }
    }

    next_index(i, tokens.size());
    if(tokens[i].id == TOKENID_OPEN){
        next_index(i, tokens.size());

        while(tokens[i].id != TOKENID_CLOSE){
            std::string tmp_return_type = "";
            if(parse_type(config, tokens, program, i, tmp_return_type, errors) != 0) return 1;
            next_index(i, tokens.size());

            if(tokens[i].id == TOKENID_NEXT){
                return_type += tmp_return_type + ", ";
                next_index(i, tokens.size());
            } else if(tokens[i].id != TOKENID_CLOSE){
                errors.panic("Expected ',' or ')' after typename inside listing of multiple return types");
                return 1;
            }
            else {
                return_type += tmp_return_type;
            }
        }

        if(return_type.length() == 0){
            errors.panic("Must return at least one value when using multiple return types");
            return 1;
        }

        output_type += ")(" + return_type + ")";
        return 0;
    }
    else {
        if(parse_type(config, tokens, program, i, return_type, errors) != 0) return 1;
        output_type += ") " + return_type;
        return 0;
    }
}

int parse_block(Config& config, TokenList& tokens, Program& program, StatementList& statements, StatementList& defer_statements, size_t& i, ErrorHandler& errors){
    // { some code; some more code; }
    //    ^

    while(tokens[i].id != TOKENID_END){
        switch(tokens[i].id){
        case TOKENID_NEWLINE:
            errors.line++;
            next_index(i, tokens.size());
            break;
        case TOKENID_KEYWORD:
            if(parse_block_keyword(config, tokens, program, statements, defer_statements, i, tokens[i].getString(), errors) != 0) return 1;
            break;
        case TOKENID_WORD:
            if(parse_block_word(config, tokens, program, statements, defer_statements, i, errors) != 0) return 1;
            break;
        case TOKENID_MULTIPLY: // multiply/pointer operator
            if(parse_block_dereference(config, tokens, program, statements, i, errors) != 0) return 1;
            break;
        case TOKENID_OPEN:
            if(parse_block_multireturn_call(config, tokens, program, statements, i , errors) != 0) return 1;
            break;
        default:
            errors.panic("Expected statement, received '" + tokens[i].syntax() + "'");
            return 1;
        }
    }

    return 0;
}
int parse_block_keyword(Config& config, TokenList& tokens, Program& program, StatementList& statements, StatementList& defer_statements, size_t& i, const std::string& keyword, ErrorHandler& errors){
    // keyword <unknown syntax follows>
    //    ^

    const size_t accepted_keywords_size = 11;
    const std::string accepted_keywords[] = {
        "break", "continue", "defer", "delete", "for", "if", "return", "switch", "unless", "until", "while"
    };

    size_t string_index = string_search(accepted_keywords, accepted_keywords_size, keyword);

    switch(string_index){
    case 0: { // break
        next_index(i, tokens.size());

        if(tokens[i].id != TOKENID_NEWLINE){
            errors.panic("Expected newline after 'break' keyword");
            return 1;
        }

        statements.push_back( new BreakStatement(errors) );
        break;
    }
    case 1: { // continue
        next_index(i, tokens.size());

        if(tokens[i].id != TOKENID_NEWLINE){
            errors.panic("Expected newline after 'continue' keyword");
            return 1;
        }

        statements.push_back( new ContinueStatement(errors) );
        break;
    }
    case 2: { // defer
        next_index(i, tokens.size());

        switch(tokens[i].id){
        case TOKENID_KEYWORD:
            // NOTE: 'defer defer <statement>' acts the same ways as 'defer <statement>' (Number of defers does not metter, maybe add a check here idk)
            if(parse_block_keyword(config, tokens, program, defer_statements, defer_statements, i, tokens[i].getString(), errors) != 0) return 1;
            break;
        case TOKENID_WORD:
            if(parse_block_word(config, tokens, program, defer_statements, defer_statements, i, errors) != 0) return 1;
            break;
        case TOKENID_MULTIPLY: // multiply/pointer operator
            if(parse_block_dereference(config, tokens, program, defer_statements, i, errors) != 0) return 1;
            break;
        default:
            errors.panic("Expected statement, received '" + tokens[i].syntax() + "'");
            return 1;
        }

        break;
    }
    case 3: { // delete
            PlainExp* expression;
            bool dangerous = false;

            next_index(i, tokens.size());
            if(tokens[i].id == TOKENID_KEYWORD){
                if(*(static_cast<std::string*>(tokens[i].data)) == "dangerous"){
                    dangerous = true;
                    next_index(i, tokens.size());
                }
            }

            if(parse_expression(config, tokens, program, i, &expression, errors) != 0) return 1;
            statements.push_back( new DeleteStatement(expression, dangerous, errors) );
            break;
        }
    case 4: { // for
        // TODO: Clean up this horrible mess of code

        StatementList for_condition_statements; // Will hold initialization statement and increament statement - [init ptr, incr ptr]
        StatementList for_statements;
        PlainExp* expression;

        next_index(i, tokens.size());
        switch(tokens[i].id){
        case TOKENID_NEWLINE:
            errors.line++;
            next_index(i, tokens.size());
            break;
        case TOKENID_KEYWORD:
            if(parse_block_keyword(config, tokens, program, for_condition_statements, defer_statements, i, tokens[i].getString(), errors) != 0) return 1;
            break;
        case TOKENID_WORD:
            if(parse_block_word(config, tokens, program, for_condition_statements, defer_statements, i, errors) != 0) return 1;
            break;
        case TOKENID_MULTIPLY: // multiply/pointer operator
            if(parse_block_dereference(config, tokens, program, for_condition_statements, i, errors) != 0) return 1;
            break;
        case TOKENID_NEXT: // Skip initialization statement if next token is found
            for_condition_statements.push_back(NULL);
            break;
        default:
            errors.panic("Expected statement, received '" + tokens[i].syntax() + "'");
            return 1;
        }

        next_index(i, tokens.size());
        if(for_condition_statements.size() > 1){
            errors.panic("Compound initialization statements currently not allowed");
            return 1;
        }

        if(parse_expression(config, tokens, program, i, &expression, errors) != 0) return 1;
        if(tokens[i].id != TOKENID_BEGIN) next_index(i, tokens.size());

        switch(tokens[i].id){
        case TOKENID_NEWLINE:
            errors.line++;
            next_index(i, tokens.size());
            break;
        case TOKENID_KEYWORD:
            if(parse_block_keyword(config, tokens, program, for_condition_statements, defer_statements, i, tokens[i].getString(), errors) != 0) return 1;
            break;
        case TOKENID_WORD:
            if(parse_block_word(config, tokens, program, for_condition_statements, defer_statements, i, errors) != 0) return 1;
            break;
        case TOKENID_MULTIPLY: // multiply/pointer operator
            if(parse_block_dereference(config, tokens, program, for_condition_statements, i, errors) != 0) return 1;
            break;
        case TOKENID_BEGIN: // Skip increament statement if begin token is found
            for_condition_statements.push_back(NULL);
            break;
        default:
            errors.panic("Expected statement, received '" + tokens[i].syntax() + "'");
            return 1;
        }

        if(for_condition_statements.size() > 2){
            errors.panic("Compound increament statements currently not allowed");
            return 1;
        }

        if(tokens[i].id != TOKENID_BEGIN){
            errors.panic("Expected '{' after 'for' statement conditions");
            return 1;
        }

        next_index(i, tokens.size());
        if(parse_block(config, tokens, program, for_statements, defer_statements, i, errors) != 0) return 1;
        next_index(i, tokens.size());

        // Skip any newlines
        while(tokens[i].id == TOKENID_NEWLINE){
            errors.line++;
            next_index_else(i, tokens.size(), errors.format("Expected '}' to close off block"));
        }

        statements.push_back( new ForStatement(for_condition_statements[0], expression, for_condition_statements[1], for_statements, errors) );
        break;
    }
    case 5: { // if
            next_index(i, tokens.size());

            if(tokens[i].id == TOKENID_KEYWORD){
                std::string next_keyword = tokens[i].getString();

                if(next_keyword == "while"){ // if-while-else
                    if(parse_block_conditional(config, tokens, program, statements, defer_statements, i, STATEMENTID_IFWHILEELSE, errors) != 0) return 1;
                } else if(next_keyword == "until"){ // if-until-else doesn't exist
                    errors.panic("if-until-else loops don't exist, did you mean to use an unless-until-else loop?");
                    return 1;
                } else { // Plain if or if-else
                    if(parse_block_conditional(config, tokens, program, statements, defer_statements, --i, STATEMENTID_IF, errors) != 0) return 1;
                }
            } else {
                if(parse_block_conditional(config, tokens, program, statements, defer_statements, --i, STATEMENTID_IF, errors) != 0) return 1;
            }

            break;
        }
    case 6: { // return
            PlainExp* expression;
            next_index(i, tokens.size());

            for(size_t i = defer_statements.size(); i-- > 0;){
                statements.push_back( defer_statements[i]->clone() );
            }

            if(tokens[i].id == TOKENID_NEWLINE){
                statements.push_back( new ReturnStatement(NULL, errors) );
            } else {
                if(parse_expression(config, tokens, program, i, &expression, errors) != 0) return 1;

                if(tokens[i].id == TOKENID_NEXT){
                    std::vector<PlainExp*> ret_expressions(1);
                    ret_expressions[0] = expression;

                    while(tokens[i].id == TOKENID_NEXT){
                        next_index(i, tokens.size());
                        if(parse_expression(config, tokens, program, i, &expression, errors) != 0) return 1;
                        ret_expressions.push_back(expression);
                    }

                    statements.push_back( new MultiReturnStatement(ret_expressions, errors) );
                }
                else statements.push_back( new ReturnStatement(expression, errors) );
            }

            break;
        }
    case 7: // switch
        if(parse_block_switch(config, tokens, program, statements, defer_statements, i, errors) != 0) return 1;
        break;
    case 8: { // unless
            next_index(i, tokens.size());

            if(tokens[i].id == TOKENID_KEYWORD){
                std::string next_keyword = tokens[i].getString();

                if(next_keyword == "until"){ // unless-until-else
                    if(parse_block_conditional(config, tokens, program, statements, defer_statements, i, STATEMENTID_UNLESSUNTILELSE, errors) != 0) return 1;
                } else if(next_keyword == "while"){ // unless-while-else doesn't exist
                    errors.panic("unless-while-else loops don't exist, did you mean to use an if-while-else loop?");
                    return 1;
                } else { // Plain unless or unless-else
                    if(parse_block_conditional(config, tokens, program, statements, defer_statements, --i, STATEMENTID_UNLESS, errors) != 0) return 1;
                }
            } else {
                if(parse_block_conditional(config, tokens, program, statements, defer_statements, --i, STATEMENTID_UNLESS, errors) != 0) return 1;
            }

            break;
        }
    case 9: // until
        if(parse_block_conditional(config, tokens, program, statements, defer_statements, i, STATEMENTID_UNTIL, errors) != 0) return 1;
        break;
    case 10: // while
        if(parse_block_conditional(config, tokens, program, statements, defer_statements, i, STATEMENTID_WHILE, errors) != 0) return 1;
        break;
    default:
        errors.panic( UNEXPECTED_KEYWORD(keyword) );
        return 1;
    }

    return 0;
}
int parse_block_word(Config& config, TokenList& tokens, Program& program, StatementList& statements, StatementList& defer_statements, size_t& i, ErrorHandler& errors){
    // word <unknown syntax follows>
    //  ^

    std::string name = tokens[i].getString();
    next_index(i, tokens.size());

    switch(tokens[i].id){
    case TOKENID_WORD:
    case TOKENID_MULTIPLY:
    case TOKENID_KEYWORD:
    case TOKENID_NEXT:
    case TOKENID_NOT:
        // Variable Definition
        if(parse_block_declaration(config, tokens, program, statements, defer_statements, i, name, errors) != 0) return 1;
        break;
    case TOKENID_OPEN:
        // Function Call
        if(parse_block_call(config, tokens, program, statements, i, name, errors) != 0) return 1;
        break;
    case TOKENID_ASSIGN:
    case TOKENID_ASSIGNADD:
    case TOKENID_ASSIGNSUB:
    case TOKENID_ASSIGNMUL:
    case TOKENID_ASSIGNDIV:
    case TOKENID_ASSIGNMOD:
        // Variable Assign
        if(parse_block_word_expression(config, tokens, program, statements, i, name, 0, errors) != 0) return 1;
        break;
    case TOKENID_MEMBER:
        if(parse_block_word_member(config, tokens, program, statements, i, name, errors) != 0) return 1;
        break;
    case TOKENID_BRACKET_OPEN:
        if(tokens[i].id == TOKENID_BRACKET_CLOSE){
            next_index(i, tokens.size());
            if(parse_block_declaration(config, tokens, program, statements, defer_statements, --i, name, errors) != 0) return 1;
        }
        else {
            // SPEED: This look ahead is kinda slow (7/31/17 -IS)
            // SYNTAX: Maybe change fixed-array declaration syntax so we don't have to do this crap (7/31/17 -IS)

            size_t peek = i;
            size_t balance = 0;

            while(tokens[peek].id == TOKENID_BRACKET_OPEN){
                next_index(peek, tokens.size());

                while(balance != 0 or tokens[peek].id != TOKENID_BRACKET_CLOSE){
                    if(tokens[peek].id == TOKENID_BRACKET_OPEN){
                        balance++;
                    }
                    else if(tokens[peek].id == TOKENID_BRACKET_CLOSE){
                        balance--;
                    }

                    next_index(peek, tokens.size());
                }

                next_index(peek, tokens.size());
            }

            next_index(peek, tokens.size());

            // SPEED: BADCODE: This check could be improved
            if( (tokens[peek-1].id == TOKENID_WORD or tokens[peek-1].id == TOKENID_MULTIPLY) and (tokens[peek].id == TOKENID_NEWLINE or tokens[peek].id == TOKENID_ASSIGN) ){
                // Fixed sized array declaration
                if(parse_block_declaration(config, tokens, program, statements, defer_statements, i, name, errors) != 0) return 1;
            }
            else {
                // Performing an operation on an array expression
                if(parse_block_word_expression(config, tokens, program, statements, i, name, 0, errors) != 0) return 1;
            }
        }
        break;
    default:
        errors.panic( UNEXPECTED_OPERATOR(tokens[i].syntax()) );
        return 1;
    }

    return 0;
}
int parse_block_declaration(Config& config, TokenList& tokens, Program& program, StatementList& statements, StatementList& defer_statements, size_t& i, const std::string& name, ErrorHandler& errors){
    // name str = "Hello World"   |   name, another_name str = "Greetings World"
    //       ^                            ^

    std::string type;
    std::vector<std::string> multiple_names;
    bool auto_delete = false;

    if(tokens[i].id == TOKENID_NEXT){
        multiple_names = { name };

        while(tokens[i].id == TOKENID_NEXT){
            next_index(i, tokens.size());

            if(tokens[i].id != TOKENID_WORD){
                errors.panic("Expected variable name after ',' in variable declaration");
                return 1;
            }

            multiple_names.push_back(tokens[i].getString());
            next_index(i, tokens.size());
        }
    }

    if(tokens[i].id == TOKENID_NOT){
        auto_delete = true;
        next_index(i, tokens.size());
    }

    if(parse_type(config, tokens, program, i, type, errors) != 0) return 1;
    next_index(i, tokens.size());

    if(auto_delete){
        if(!Program::is_pointer_typename(type) and !Program::is_array_typename(type) and type != "string"){
            errors.panic("The non-pointer type '" + type + "' can't be deleted later on in the program.\n    Only array and pointer types work with '!' deletion. (Aliases not supported yet)");
            return 1;
        }

        if(multiple_names.size() == 0){
            defer_statements.push_back( new DeleteStatement(new WordExp(name, errors), errors) );
        } else {
            for(const std::string& multiple_name : multiple_names){
                defer_statements.push_back( new DeleteStatement(new WordExp(multiple_name, errors), errors) );
            }
        }
    }

    if(tokens[i].id == TOKENID_ASSIGN){
        PlainExp* expression;

        next_index(i, tokens.size());
        if(parse_expression(config, tokens, program, i, &expression, errors) != 0) return 1;

        if(multiple_names.size() == 0){
            statements.push_back( new DeclareAssignStatement(name, type, expression, errors) );
        } else {
            statements.push_back( new MultiDeclareAssignStatement(multiple_names, type, expression, errors) );
        }
    }
    else {
        if(multiple_names.size() == 0){
            statements.push_back( new DeclareStatement(name, type, errors) );
        } else {
            statements.push_back( new MultiDeclareStatement(multiple_names, type, errors) );
        }
    }
    return 0;
}
int parse_block_call(Config& config, TokenList& tokens, Program& program, StatementList& statements, size_t& i, const std::string& name, ErrorHandler& errors){
    // name(arg1, arg2)
    //     ^

    std::vector<PlainExp*> args;
    next_index(i, tokens.size());

    if(tokens[i].id != TOKENID_CLOSE) {
        while(true) {
            PlainExp* expression;

            if(parse_expression(config, tokens, program, i, &expression, errors) != 0) return 1;

            args.push_back(expression);

            if(tokens[i].id == TOKENID_CLOSE) break;
            if(tokens[i].id != TOKENID_NEXT){
                errors.panic("Expected ')' or ',' in argument list");
                return 1;
            }

            next_index(i, tokens.size());
        }
    }

    statements.push_back( new CallStatement(name, args, errors) );
    next_index(i, tokens.size());
    return 0;
}
int parse_block_word_expression(Config& config, TokenList& tokens, Program& program, StatementList& statements, size_t& i, const std::string& name, int loads, ErrorHandler& errors){
    // name += 10 * 3 / 4   |   name[i] = 10 * 3 / 4   |   name.member *= 10 * 3 / 4   |   name.member.call()
    //      ^                       ^                          ^                               ^

    // NOTE: This function also parses assignment related things

    PlainExp* expression;
    PlainExp* location = new WordExp(name, errors);
    PlainExp* index_expression;

    for(int l = 0; l != loads; l++){
        location = new LoadExp(location, errors);
    }

    while(tokens[i].id == TOKENID_BRACKET_OPEN or tokens[i].id == TOKENID_MEMBER){
        switch(tokens[i].id){
        case TOKENID_BRACKET_OPEN:
            {
                next_index(i, tokens.size());
                if(parse_expression(config, tokens, program, i, &index_expression, errors) != 0) return 1;

                if(tokens[i].id != TOKENID_BRACKET_CLOSE){
                    errors.panic("Expected closing bracket ']'");
                    return 1;
                }

                location = new IndexLoadExp(location, index_expression, errors);
                next_index(i, tokens.size());
                break;
            }
        case TOKENID_MEMBER:
            {
                next_index(i, tokens.size());

                if(tokens[i].id != TOKENID_WORD){
                    errors.panic("Expected word after '.' operator");
                    return 1;
                }

                std::string word = tokens[i].getString();
                next_index(i, tokens.size());

                if(tokens[i].id == TOKENID_OPEN){
                    if(parse_block_member_call(config, tokens, program, statements, i, location, word, errors) != 0) return 1;
                    return 0;
                }
                else {
                    location = new MemberExp(location, word, errors);
                }

                break;
            }
        default:
            errors.panic(SUICIDE);
            return 1;
        }
    }

    // If no method call was found, assume this is an assignment statement
    uint16_t assignment_token = tokens[i].id;

    // Ensure assignment operator is valid
    switch(assignment_token){
    case TOKENID_ASSIGN:
    case TOKENID_ASSIGNADD:
    case TOKENID_ASSIGNSUB:
    case TOKENID_ASSIGNMUL:
    case TOKENID_ASSIGNDIV:
    case TOKENID_ASSIGNMOD:
        break;
    default:
        errors.panic("Expected assignment after mutable expression");
        return 1;
    }

    // Parse the expression that will be used by the operator
    next_index(i, tokens.size());
    if(parse_expression(config, tokens, program, i, &expression, errors) != 0) return 1;

    // Create the assignment operator variant
    switch(assignment_token){
    case TOKENID_ASSIGN:
        statements.push_back( new AssignStatement(location, expression, errors) );
        break;
    case TOKENID_ASSIGNADD:
        statements.push_back( new AdditionAssignStatement(location, expression, errors) );
        break;
    case TOKENID_ASSIGNSUB:
        statements.push_back( new SubtractionAssignStatement(location, expression, errors) );
        break;
    case TOKENID_ASSIGNMUL:
        statements.push_back( new MultiplicationAssignStatement(location, expression, errors) );
        break;
    case TOKENID_ASSIGNDIV:
        statements.push_back( new DivisionAssignStatement(location, expression, errors) );
        break;
    case TOKENID_ASSIGNMOD:
        statements.push_back( new ModulusAssignStatement(location, expression, errors) );
        break;
    default:
        errors.panic(SUICIDE); // This should never occur
        return 1;
    }

    return 0;
}
int parse_block_dereference(Config& config, TokenList& tokens, Program& program, StatementList& statements, size_t& i, ErrorHandler& errors){
    // *value = expression
    //    ^

    int deref_count = 0;
    std::string name;

    while(tokens[i].id == TOKENID_MULTIPLY){
        deref_count += 1;
        next_index(i, tokens.size());
    }

    if(tokens[i].id != TOKENID_WORD){
        errors.panic("Expected word after dereference operator");
        return 1;
    }

    name = tokens[i].getString();
    next_index(i, tokens.size());

    if(parse_block_word_expression(config, tokens, program, statements, i, name, deref_count, errors) != 0) return 1;
    return 0;
}
int parse_block_conditional(Config& config, TokenList& tokens, Program& program, StatementList& statements, StatementList& defer_statements, size_t& i, uint16_t conditional_type, ErrorHandler& errors){
    // conditional <condition> { ... }
    //      ^

    PlainExp* expression;
    StatementList conditional_statements;
    size_t original_defer_count = defer_statements.size();
    next_index(i, tokens.size());

    if(parse_expression(config, tokens, program, i, &expression, errors) != 0) return 1;
    if(tokens[i].id != TOKENID_BEGIN){
        errors.panic("Expected '{' after conditional");
        return 1;
    }

    next_index(i, tokens.size());
    if(parse_block(config, tokens, program, conditional_statements, defer_statements, i, errors) != 0) return 1;
    next_index(i, tokens.size());

    if(original_defer_count != defer_statements.size()){
        for(size_t i = defer_statements.size() - 1; i != original_defer_count - 1; i--){
            conditional_statements.push_back(defer_statements[i]);
        }
        defer_statements.resize(original_defer_count);
    }

    // Skip any newlines
    while(tokens[i].id == TOKENID_NEWLINE){
        errors.line++;
        next_index_else(i, tokens.size(), errors.format("Expected '}' to close off block"));
    }

    if(tokens[i].id == TOKENID_KEYWORD){
        if(tokens[i].getString() == "else"){
            // There is an else after the conditional
            StatementList else_statements;
            next_index(i, tokens.size());

            if(tokens[i].id == TOKENID_BEGIN){
                next_index(i, tokens.size());
                if(parse_block(config, tokens, program, else_statements, defer_statements, i, errors) != 0) return 1;
                next_index(i, tokens.size());

                if(original_defer_count != defer_statements.size()){
                    for(size_t i = defer_statements.size() - 1; i != original_defer_count - 1; i--){
                        else_statements.push_back(defer_statements[i]);
                    }
                    defer_statements.resize(original_defer_count);
                }
            }
            else if(tokens[i].id == TOKENID_KEYWORD){
                uint16_t else_conditional_type;
                std::string else_conditional = tokens[i].getString();

                if(else_conditional == "if"){
                    else_conditional_type = STATEMENTID_IF;
                }
                else if(else_conditional == "unless"){
                    else_conditional_type = STATEMENTID_UNLESS;
                }
                else {
                    errors.panic( UNEXPECTED_KEYWORD(else_conditional) );
                    return 1;
                }

                parse_block_conditional(config, tokens, program, else_statements, defer_statements, i, else_conditional_type, errors);
            }
            else {
                errors.panic("Expected '{' or 'if' after 'else'");
                return 1;
            }

            switch(conditional_type){
            case STATEMENTID_IF:
                statements.push_back( new IfElseStatement(expression, conditional_statements, else_statements, errors) );
                break;
            case STATEMENTID_UNLESS:
                statements.push_back( new UnlessElseStatement(expression, conditional_statements, else_statements, errors) );
                break;
            case STATEMENTID_IFWHILEELSE:
                statements.push_back( new IfWhileElseStatement(expression, conditional_statements, else_statements, errors) );
                break;
            case STATEMENTID_UNLESSUNTILELSE:
                statements.push_back( new UnlessUntilElseStatement(expression, conditional_statements, else_statements, errors) );
                break;
            default:
                errors.panic("Conditional doesn't support 'else' keyword");
                return 1;
            }

            return 0;
        }
    }

    // At this point, we have verified that there is no 'else'

    switch(conditional_type){
    case STATEMENTID_IF:
        statements.push_back( new IfStatement(expression, conditional_statements, errors) );
        break;
    case STATEMENTID_WHILE:
        statements.push_back( new WhileStatement(expression, conditional_statements, errors) );
        break;
    case STATEMENTID_UNLESS:
        statements.push_back( new UnlessStatement(expression, conditional_statements, errors) );
        break;
    case STATEMENTID_UNTIL:
        statements.push_back( new UntilStatement(expression, conditional_statements, errors) );
        break;
    default:
        errors.panic("Conditional requires 'else' keyword");
        return 1;
    }

    return 0;
}
int parse_block_word_member(Config& config, TokenList& tokens, Program& program, StatementList& statements, size_t& i, const std::string& name, ErrorHandler& errors){
    // word.<unknown syntax follows>
    //     ^

    size_t starting_i = i;
    next_index(i, tokens.size());

    if(tokens[i].id != TOKENID_WORD){
        errors.panic("Expected word after '.' operator");
        return 1;
    }

    std::string second_word = tokens[i].getString();
    next_index(i, tokens.size());

    switch(tokens[i].id){
    case TOKENID_OPEN:
        if(parse_block_member_call(config, tokens, program, statements, i, new WordExp(name, errors), second_word, errors) != 0) return 1;
        break;
    default:
        i -= 2;
        if(parse_block_word_expression(config, tokens, program, statements, i, name, 0, errors) != 0) return 1;
    }

    return 0;
}
int parse_block_member_call(Config& config, TokenList& tokens, Program& program, StatementList& statements, size_t& i, PlainExp* value, const std::string& func_name, ErrorHandler& errors){
    // a_variable.a_member.a_method ( ... )
    //                              ^

    std::vector<PlainExp*> args;
    next_index(i, tokens.size());

    if(tokens[i].id != TOKENID_CLOSE) {
        while(true) {
            PlainExp* expression;

            if(parse_expression(config, tokens, program, i, &expression, errors) != 0) return 1;
            args.push_back(expression);

            if(tokens[i].id == TOKENID_CLOSE) break;
            if(tokens[i].id != TOKENID_NEXT){
                errors.panic("Expected ')' or ',' in argument list");
                return 1;
            }

            next_index(i, tokens.size());
        }
    }

    statements.push_back( new MemberCallStatement(value, func_name, args, errors) );
    next_index(i, tokens.size());
    return 0;
}
int parse_block_switch(Config& config, TokenList& tokens, Program& program, StatementList& statements, StatementList& defer_statements, size_t& i, ErrorHandler& errors){
    // switch <condition> { .... }
    //    ^

    enum StatementContext : unsigned char {
        None = 0,
        Case = 1,
        Default = 2
    };

    PlainExp* switch_condition;
    PlainExp* case_condition;
    StatementContext statement_context = StatementContext::None;
    std::vector<SwitchStatement::Case> cases;
    ErrorHandler switch_errors = errors;
    ErrorHandler case_errors = errors;

    StatementList possible_statements;
    StatementList default_statements;

    next_index(i, tokens.size());
    if(parse_expression(config, tokens, program, i, &switch_condition, switch_errors) != 0) return 1;

    if(tokens[i].id != TOKENID_BEGIN){
        errors.panic("Expected '{' after switch condition");
        return 1;
    }

    next_index(i, tokens.size());

    while(tokens[i].id != TOKENID_END){
        switch(tokens[i].id){
        case TOKENID_NEWLINE:
            errors.line++;
            next_index(i, tokens.size());
            break;
        case TOKENID_KEYWORD: {
                std::string keyword = tokens[i].getString();

                if(keyword == "case"){
                    if(statement_context == StatementContext::Case){
                        cases.push_back( SwitchStatement::Case(case_condition, possible_statements, case_errors) );
                    } else if(statement_context == StatementContext::Default){
                        default_statements = possible_statements;
                    }

                    case_errors = errors;
                    next_index(i, tokens.size());
                    if(parse_expression(config, tokens, program, i, &case_condition, errors) != 0) return 1;

                    statement_context = StatementContext::Case;
                    possible_statements.clear();
                } else if(keyword == "default"){
                    if(statement_context == StatementContext::Case){
                        cases.push_back( SwitchStatement::Case(case_condition, possible_statements, case_errors) );
                    } else if(statement_context == StatementContext::Default){
                        default_statements = possible_statements;
                    }

                    if(default_statements.size() != 0){
                        errors.panic("Attempting to define a default case when a default case already exists");
                        return 1;
                    }

                    case_errors = errors;
                    statement_context = StatementContext::Default;
                    possible_statements.clear();
                    next_index(i, tokens.size());
                } else {
                    if(statement_context == StatementContext::None){
                        errors.panic("Statement must be part of a case because it's inside a switch statement");
                        return 1;
                    }

                    if(parse_block_keyword(config, tokens, program, possible_statements, defer_statements, i, keyword, errors) != 0) return 1;
                }
                break;
            }
        case TOKENID_WORD:
            if(statement_context == StatementContext::None){
                errors.panic("Statement must be part of a case because it's inside a switch statement");
                return 1;
            }

            if(parse_block_word(config, tokens, program, possible_statements, defer_statements, i, errors) != 0) return 1;
            break;
        case TOKENID_MULTIPLY: // multiply/pointer operator
            if(statement_context == StatementContext::None){
                errors.panic("Statement must be part of a case because it's inside a switch statement");
                return 1;
            }

            if(parse_block_dereference(config, tokens, program, possible_statements, i, errors) != 0) return 1;
            break;
        default:
            errors.panic("Expected statement, received '" + tokens[i].syntax() + "'");
            return 1;
        }
    }

    // Push back final case
    if(statement_context == StatementContext::Case){
        cases.push_back( SwitchStatement::Case(case_condition, possible_statements, case_errors) );
    } else if(statement_context == StatementContext::Default){
        default_statements = possible_statements;
    }

    statements.push_back( new SwitchStatement(switch_condition, cases, default_statements, switch_errors) );
    next_index(i, tokens.size());
    return 0;
}
int parse_block_multireturn_call(Config& config, TokenList& tokens, Program& program, StatementList& statements, size_t& i, ErrorHandler& errors){
    // (variable1, variable2) = functionCall(arg1, arg2, arg3)
    // ^

    // NOTE: Assumes initial token is '('

    std::string name;
    std::vector<PlainExp*> args;
    std::vector<std::string> result_variables;

    while(tokens[i].id != TOKENID_CLOSE){
        next_index(i, tokens.size());

        if(tokens[i].id == TOKENID_KEYWORD && tokens[i].getString() == "null"){
            result_variables.push_back("");
            next_index(i, tokens.size());
        }
        else if(tokens[i].id == TOKENID_WORD){
            result_variables.push_back(tokens[i].getString());
            next_index(i, tokens.size());
        }
        else {
            errors.panic("Expected variable name or null keyword inside listing of result variables");
            return 1;
        }

        if(tokens[i].id != TOKENID_NEXT and tokens[i].id != TOKENID_CLOSE){
            errors.panic("Expected ',' after variablename inside listing of result variables");
            return 1;
        }
    }

    next_index(i, tokens.size());
    if(tokens[i].id != TOKENID_ASSIGN){
        errors.panic("Expected assignment operator after listing of result variables");
        return 1;
    }

    next_index(i, tokens.size());

    if(tokens[i].id != TOKENID_WORD){
        errors.panic("Expected function name after assignment operator and listing of result variables");
        return 1;
    }

    name = tokens[i].getString();
    next_index(i, tokens.size());

    if(tokens[i].id == TOKENID_OPEN){
        next_index(i, tokens.size());

        if(tokens[i].id != TOKENID_CLOSE) {
            while(true) {
                PlainExp* expression;

                if(parse_expression(config, tokens, program, i, &expression, errors) != 0) return 1;

                args.push_back(expression);

                if(tokens[i].id == TOKENID_CLOSE) break;
                if(tokens[i].id != TOKENID_NEXT){
                    errors.panic("Expected ')' or ',' in argument list");
                    return 1;
                }

                next_index(i, tokens.size());
            }
        }

        statements.push_back( new MultiResultCallStatement(name, args, result_variables, errors) );
        next_index(i, tokens.size());
        return 0;
    }
    else if(tokens[i].id == TOKENID_MEMBER){
        errors.panic("Calling methods that return multiple values isn't suppported yet");
        return 1;
    }
    else {
        errors.panic("Expected '(' after function name");
        return 1;
    }
}

int parse_expression(Config& config, TokenList& tokens, Program& program, size_t& i, PlainExp** expression, ErrorHandler& errors){
    // 10 + 3 * 6
    // ^

    if(parse_expression_primary(config, tokens, program, i, expression, errors) != 0) return 1;
    if(parse_expression_operator_right(config, tokens, program, i, 0, expression, false, errors) != 0) return 1;
    return 0;
}
int parse_expression_primary(Config& config, TokenList& tokens, Program& program, size_t& i, PlainExp** expression, ErrorHandler& errors){
    while(tokens[i].id == TOKENID_NEWLINE){
        next_index(i, tokens.size());
        errors.line++;
    }

    // TODO: CLEANUP: Clean up code inside each case (probally refactor into different functions or something)
    switch (tokens[i].id) {
    case TOKENID_WORD:
        next_index(i, tokens.size());
        if(tokens[i].id == TOKENID_OPEN){
            if(parse_expression_call(config, tokens, program, --i, expression, errors) != 0) return 1;
        } else if(tokens[i].id == TOKENID_NAMESPACE){
            std::string name_space = tokens[i-1].getString();
            next_index(i, tokens.size());
            if(tokens[i].id != TOKENID_WORD){
                errors.panic("Expected identifier after '::' operator");
                return 1;
            }
            std::string member = tokens[i].getString();
            *expression = new NamespaceExp(name_space, member, errors);
            next_index(i, tokens.size());
        } else {
            *expression = new WordExp( tokens[i-1].getString(), errors );
        }
        return 0;
    case TOKENID_KEYWORD:
        {
            // TODO: Use string_search here
            // CLEANUP: Clean up this mess (this code is definitely r/badcode worthy)
            std::string keyword = tokens[i].getString();
            next_index(i, tokens.size());

            if(keyword == "true"){
                *expression = new BoolExp(true, errors);
            } else if(keyword == "false"){
                *expression = new BoolExp(false, errors);
            } else if(keyword == "null"){
                *expression = new NullExp(errors);
            } else if(keyword == "cast"){
                std::string target_typename;
                PlainExp* cast_expr;

                if(parse_type(config, tokens, program, i, target_typename, errors) != 0) return 1;
                next_index(i, tokens.size());

                if(tokens[i].id == TOKENID_OPEN){
                    next_index(i, tokens.size());
                    if(parse_expression(config, tokens, program, i, &cast_expr, errors) != 0) return 1;
                    next_index(i, tokens.size());
                }
                else {
                    if(parse_expression_primary(config, tokens, program, i, &cast_expr, errors) != 0) return 1;

                    if(tokens[i].id == TOKENID_MEMBER){
                        if(parse_expression_operator_right(config, tokens, program, i, 0, &cast_expr, true, errors) != 0) return 1;
                    }
                }

                *expression = new CastExp(cast_expr, target_typename, errors);
            } else if(keyword == "funcptr"){
                std::string function_name;
                std::vector<std::string> function_arguments;
                bool is_var_args = false;

                if(tokens[i].id != TOKENID_WORD){
                    errors.panic("Expected function name after 'funcptr' keyword");
                    return 1;
                }

                function_name = tokens[i].getString();
                next_index(i, tokens.size());

                if(tokens[i].id != TOKENID_OPEN){
                    errors.panic("Expected '(' after function name in 'funcptr' expression");
                    return 1;
                }

                next_index(i, tokens.size());
                while(tokens[i].id != TOKENID_CLOSE){
                    if(is_var_args){
                        errors.panic("Variable arguments can only be the end of the argument list");
                        return 1;
                    }

                    if(tokens[i].id == TOKENID_ELLIPSIS){
                        is_var_args = true;
                        next_index(i, tokens.size());
                    }

                    std::string type;
                    if(parse_type(config, tokens, program, i, type, errors) != 0) return 1;
                    function_arguments.push_back(is_var_args ? "..." + type : type);
                    next_index(i, tokens.size());

                    if(tokens[i].id == TOKENID_NEXT){
                        next_index(i, tokens.size());
                    } else if(tokens[i].id != TOKENID_CLOSE){
                        errors.panic("Expected ',' or ')' after typename in argument list");
                        return 1;
                    }
                }

                next_index(i, tokens.size());
                Token& peeked_token = tokens[i];

                if( peeked_token.id == TOKENID_WORD || peeked_token.id == TOKENID_MULTIPLY || peeked_token.id == TOKENID_BRACKET_OPEN || peeked_token.id == TOKENID_OPEN
                   || (peeked_token.id == TOKENID_KEYWORD && (peeked_token.getString() == "def" or peeked_token.getString() == "stdcall")) ){
                    errors.panic("Unexpected return type information after argument list of function '" + function_name + "'\n    The 'funcptr' keyword only takes in function arguments, not return type information");
                    return 1;
                }

                *expression = new FuncptrExp(function_name, function_arguments, errors);
            } else if(keyword == "sizeof"){
                std::string sizeof_typename;

                if(parse_type(config, tokens, program, i, sizeof_typename, errors) != 0){
                    errors.panic("Expected typename after 'sizeof' operator");
                    return 1;
                }

                next_index(i, tokens.size());
                *expression = new SizeofExp(sizeof_typename, errors);
            } else if(keyword == "new"){
                std::string new_typename;
                size_t amount = 1;
                size_t element_amount = 0;

                if(tokens[i].id == TOKENID_BRACKET_OPEN){
                    next_index(i, tokens.size());

                    if(tokens[i].id == TOKENID_INT){
                        int integer_value = tokens[i].getInt();

                        if(integer_value < 0){
                            errors.panic("Array length specified must be 0 or greater");
                            return 1;
                        }

                        element_amount = integer_value;
                        next_index(i, tokens.size());

                        if(tokens[i].id != TOKENID_BRACKET_CLOSE){
                            errors.panic("Expected ']' after integer value in [] after 'new' operator");
                            return 1;
                        }

                        next_index(i, tokens.size());
                    }
                    else {
                        i--;
                    }
                }

                if(parse_type(config, tokens, program, i, new_typename, errors) != 0){
                    errors.panic("Expected typename after 'new' keyword");
                    return 1;
                }
                next_index(i, tokens.size());

                if(tokens[i].id == TOKENID_MULTIPLY){
                    int new_amount;
                    next_index(i, tokens.size());

                    if(tokens[i].id == TOKENID_INT){
                        new_amount = tokens[i].getInt();

                        if(new_amount < 1){
                            errors.panic("Must allocate at least one object when using the 'new' operator");
                            return 1;
                        }

                        amount = new_amount;
                        next_index(i, tokens.size());
                        *expression = new AllocExp(new_typename, amount, element_amount, errors);
                    }
                    else if(tokens[i].id == TOKENID_OPEN){
                        PlainExp* amount_expression;
                        next_index(i, tokens.size());
                        parse_expression(config, tokens, program, i, &amount_expression, errors);
                        next_index(i, tokens.size());
                        *expression = new DynamicAllocExp(new_typename, amount_expression, element_amount, errors);
                    }
                    else {
                        errors.panic("Expected constant integer value or '(' after '*' in 'new' operator");
                        return 1;
                    }

                }
                else {
                    *expression = new AllocExp(new_typename, amount, element_amount, errors);
                }
            } else {
                errors.panic( UNEXPECTED_KEYWORD_INEXPR(keyword) );
                return 1;
            }
        }
        return 0;
    case TOKENID_CONSTANT:
        {
            *expression = new RetrieveConstantExp(tokens[i].getString(), errors);
            next_index(i, tokens.size());
        }
        return 0;
    case TOKENID_ADDRESS:
        {
            PlainExp* mutable_expression;

            next_index(i, tokens.size());
            if(parse_expression_primary(config, tokens, program, i, &mutable_expression, errors) != 0){
                errors.panic("Expected expression after '&' operator");
                return 1;
            }

            if(tokens[i].id == TOKENID_MEMBER){
                if(parse_expression_operator_right(config, tokens, program, i, 0, &mutable_expression, true, errors) != 0) return 1;
            }

            *expression = new AddrExp( mutable_expression, errors );
        }
        return 0;
    case TOKENID_MULTIPLY: // Dereference
        {
            PlainExp* primary;
            next_index(i, tokens.size());

            if(parse_expression_primary(config, tokens, program, i, &primary, errors) != 0) return 1;

            *expression = new LoadExp(primary, errors);
            return 0;
        }
    case TOKENID_BYTE:
        *expression = new ByteExp( tokens[i].getByte(), errors );
        next_index(i, tokens.size());
        return 0;
    case TOKENID_SHORT:
        *expression = new ShortExp( tokens[i].getShort(), errors );
        next_index(i, tokens.size());
        return 0;
    case TOKENID_INT:
        *expression = new IntegerExp( tokens[i].getInt(), errors );
        next_index(i, tokens.size());
        return 0;
    case TOKENID_LONG:
        *expression = new LongExp( tokens[i].getLong(), errors );
        next_index(i, tokens.size());
        return 0;
    case TOKENID_UBYTE:
        *expression = new UnsignedByteExp( tokens[i].getUByte(), errors );
        next_index(i, tokens.size());
        return 0;
    case TOKENID_USHORT:
        *expression = new UnsignedShortExp( tokens[i].getUShort(), errors );
        next_index(i, tokens.size());
        return 0;
    case TOKENID_UINT:
        *expression = new UnsignedIntegerExp( tokens[i].getUInt(), errors );
        next_index(i, tokens.size());
        return 0;
    case TOKENID_ULONG:
        *expression = new UnsignedLongExp( tokens[i].getULong(), errors );
        next_index(i, tokens.size());
        return 0;
    case TOKENID_HALF:
        *expression = new HalfExp( tokens[i].getFloat(), errors );
        next_index(i, tokens.size());
        return 0;
    case TOKENID_FLOAT:
        *expression = new FloatExp( tokens[i].getFloat(), errors );
        next_index(i, tokens.size());
        return 0;
    case TOKENID_DOUBLE:
        *expression = new DoubleExp( tokens[i].getDouble(), errors );
        next_index(i, tokens.size());
        return 0;
    case TOKENID_STRING:
        *expression = new StringExp( tokens[i].getString(), errors );
        next_index(i, tokens.size());
        return 0;
    case TOKENID_LENGTHSTRING:
        *expression = new LengthStringExp( tokens[i].getString(), errors );
        next_index(i, tokens.size());
        return 0;
    case TOKENID_USIZE:
        *expression = new UnsignedSizeExp( tokens[i].getULong(), errors );
        next_index(i, tokens.size());
        return 0;
    case TOKENID_NUMERIC_INT:
        *expression = new NumericIntegerExp( tokens[i].getULong(), errors );
        next_index(i, tokens.size());
       return 0;
    case TOKENID_OPEN:
        {
            next_index(i, tokens.size()); // eat (
            if(parse_expression(config, tokens, program, i, expression, errors) != 0) return 1;

            if (tokens[i].id != TOKENID_CLOSE){
                errors.panic("Expected ')' after expression, received '" + tokens[i].syntax() + "'");
                return 1;
            }
            next_index(i, tokens.size()); // eat )
        }
        return 0;
    case TOKENID_NOT:
        {
            PlainExp* not_expresion;
            next_index(i, tokens.size());
            if(parse_expression_primary(config, tokens, program, i, &not_expresion, errors) != 0) return 1;
            *expression = new NotExp(not_expresion, errors);
        }
        return 0;
    case TOKENID_BEGIN:
        {
            std::vector<PlainExp*> elements;
            next_index(i, tokens.size());

            while(tokens[i].id == TOKENID_NEWLINE){
                next_index(i, tokens.size());
                errors.line++;
            }

            while(tokens[i].id != TOKENID_END){
                PlainExp* element;
                if(parse_expression(config, tokens, program, i, &element, errors) != 0) return 1;
                elements.push_back(element);

                while(tokens[i].id == TOKENID_NEWLINE){
                    next_index(i, tokens.size());
                    errors.line++;
                }

                if(tokens[i].id == TOKENID_NEXT) next_index(i, tokens.size());

                while(tokens[i].id == TOKENID_NEWLINE){
                    next_index(i, tokens.size());
                    errors.line++;
                }
            }

            next_index(i, tokens.size());
            *expression = new ArrayDataExp(elements, errors);
        }
        return 0;
    default:
        errors.panic("Unexpected operator in expression: '" + tokens[i].syntax() + "'");
        return 1;
    }
}
int parse_expression_operator_right(Config& config, TokenList& tokens, Program& program, size_t& i, int precedence, PlainExp** left, bool keep_mutable, ErrorHandler& errors) {
    while(i != tokens.size()) {
        int token_precedence = tokens[i].getPrecedence();
        int operation;

        if (token_precedence < precedence) return 0;
        operation = tokens[i].id;

        if(operation == TOKENID_CLOSE or operation == TOKENID_NEWLINE or operation == TOKENID_NEXT
           or operation == TOKENID_BRACKET_CLOSE or operation == TOKENID_BEGIN or operation == TOKENID_END) return 0;

        // TODO: Clean up this messy code:
        if(operation == TOKENID_MEMBER){
            next_index(i, tokens.size());
            if(tokens[i].id != TOKENID_WORD){
                errors.panic("Expected word after '.' operator");
                return 1;
            }

            std::string name = tokens[i].getString();
            next_index(i, tokens.size());

            if(tokens[i].id == TOKENID_OPEN){
                // Member Call Expression - 'expression.method()'

                if(keep_mutable){
                    // If we want to keep the expression mutable, reverse our progress
                    i -= 2;
                    return 0;
                }

                std::vector<PlainExp*> args;
                next_index(i, tokens.size());

                if(tokens[i].id != TOKENID_CLOSE) {
                    while(true) {
                        PlainExp* expression;
                        if(parse_expression(config, tokens, program, i, &expression, errors) != 0) return 1;
                        args.push_back(expression);

                        if(tokens[i].id == TOKENID_CLOSE) break;
                        if(tokens[i].id != TOKENID_NEXT){
                            errors.panic("Expected ')' or ',' in argument list");
                            return 1;
                        }

                        next_index(i, tokens.size());
                    }
                }

                *left = new MemberCallExp(*left, name, args, errors);
                next_index(i, tokens.size());
            }
            else {
                // Member Expression - 'expression.member'
                *left = new MemberExp(*left, name, errors);
            }
        }
        else if(operation == TOKENID_WORD){
            if(keep_mutable) return 0;

            PlainExp* right;
            std::string name = tokens[i].getString();

            next_index(i, tokens.size());
            if(parse_expression_primary(config, tokens, program, i, &right, errors) != 0) return 1;

            int next_precedence = tokens[i].getPrecedence();
            if (token_precedence < next_precedence) {
                if(parse_expression_operator_right(config, tokens, program, i, token_precedence + 1, &right, false, errors) != 0) return 1;
            }

            std::vector<PlainExp*> args(2);
            args[0] = *left;
            args[1] = right;
            *left = new CallExp(name, args, errors);
        }
        else if(operation == TOKENID_BRACKET_OPEN){
            PlainExp* int_expr;

            next_index(i, tokens.size());
            if(parse_expression(config, tokens, program, i, &int_expr, errors) != 0) return 1;

            if(tokens[i].id != TOKENID_BRACKET_CLOSE){
                errors.panic("Expected closing bracket ']'");
                return 1;
            }

            *left = new IndexLoadExp(*left, int_expr, errors);
            next_index(i, tokens.size());
        }
        else {
            if(keep_mutable) return 0;

            PlainExp* right;
            next_index(i, tokens.size());
            if(parse_expression_primary(config, tokens, program, i, &right, errors) != 0) return 1;

            int next_precedence = tokens[i].getPrecedence();
            if (token_precedence < next_precedence) {
                if(parse_expression_operator_right(config, tokens, program, i, token_precedence + 1, &right, false, errors) != 0) return 1;
            }

            *left = new OperatorExp(operation, *left, right, errors);
        }
    }

    return 0;
}
int parse_expression_call(Config& config, TokenList& tokens, Program& program, size_t& i, PlainExp** expression, ErrorHandler& errors){
    // call(arg, arg, arg) + 132
    //  ^

    std::string name = tokens[i].getString();
    std::vector<PlainExp*> args;
    i++; // Safe because parse_expression_primary

    next_index(i, tokens.size());
    while(i != tokens.size() and tokens[i].id != TOKENID_CLOSE){
        PlainExp* arg;
        if(parse_expression(config, tokens, program, i, &arg, errors) != 0) return 1;
        args.push_back(arg);

        if(tokens[i].id == TOKENID_NEXT) next_index(i, tokens.size());
    }

    *expression = new CallExp(name, args, errors);
    next_index(i, tokens.size());
    return 0;
}
