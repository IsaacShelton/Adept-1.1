
#include <unistd.h>
#include <iostream>
#include <boost/filesystem.hpp>
#include "../include/die.h"
#include "../include/type.h"
#include "../include/build.h"
#include "../include/parse.h"
#include "../include/lexer.h"
#include "../include/errors.h"
#include "../include/search.h"
#include "../include/strings.h"
#include "../include/mangling.h"
#include "llvm/Support/DynamicLibrary.h"

int parse(Configuration& config, TokenList* tokens, Program& program, ErrorHandler& errors){
    if(config.time) config.clock.remember();

    // Generate standard type aliases
    program.generate_type_aliases();

    if(config.add_build_api){
        // Add build script api if its a build script
        build_add_api(&program);
    }

    for(size_t i = 0; i != tokens->size(); i++){
        // SPEED: parse_token code should probably be inlined here
        if(parse_token(config, *tokens, program, i, errors) != 0) return 1;
    }

    // Print Parser Time
    if(config.time and !config.silent){
        config.clock.print_since("PARSER DONE", filename_name(config.filename));
        config.clock.remember();
    }
    return 0;
}

int parse_token(Configuration& config, TokenList& tokens, Program& program, size_t& i, ErrorHandler& errors){
    AttributeInfo attr_info(false);

    switch(tokens[i].id){
    case TOKENID_KEYWORD:
        if(parse_keyword(config, tokens, program, i, attr_info, errors) != 0) return 1;
        break;
    case TOKENID_WORD:
        if(parse_word(config, tokens, program, i, attr_info, errors) != 0) return 1;
        break;
    case TOKENID_NEWLINE:
        errors.line++;
        break;
    default:
        errors.panic(EXPECTED_DEFINITION);
        return 1;
    }

    return 0;
}
int parse_word(Configuration& config, TokenList& tokens, Program& program, size_t& i, const AttributeInfo& attr_info, ErrorHandler& errors){
    // a_word <unknown syntax follows>
    //   ^

    std::string name = tokens[i].getString();
    next_index(i, tokens.size());

    switch(tokens[i].id){
    case TOKENID_WORD:
    case TOKENID_KEYWORD:
        if(parse_global(config, tokens, program, i, name, attr_info, errors) != 0) return 1;
        break;
    default:
        errors.panic(EXPECTED_DEFINITION);
        return 1;
    }

    return 0;
}
int parse_keyword(Configuration& config, TokenList& tokens, Program& program, size_t& i, const AttributeInfo& attr_info, ErrorHandler& errors){
    // a_keyword <unknown syntax follows>
    //     ^

    std::string keyword = tokens[i].getString();
    next_index(i, tokens.size());

    const size_t accepted_keywords_size = 11;
    const std::string accepted_keywords[] = {
        "class", "constant", "def", "dynamic", "enum", "foreign", "import", "link", "private", "public", "type"
    };

    size_t string_index = string_search(accepted_keywords, accepted_keywords_size, keyword);

    switch(string_index){
    case 0: // class
        if(parse_class(config, tokens, program, i, attr_info, errors) != 0) return 1;
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
    case 5: // foreign
        if(parse_external(config, tokens, program, i, attr_info, errors) != 0) return 1;
        break;
    case 6: // import
        if(parse_import(config, tokens, program, i, attr_info, errors) != 0) return 1;
        break;
    case 7: // link
        if(parse_lib(config, tokens, program, i, errors) != 0) return 1;
        break;
    case 8: // private
    case 9: // public
        if(parse_attribute(config, tokens, program, --i, errors) != 0) return 1;
        break;
    case 10: // type
        if(parse_structure(config, tokens, program, i, attr_info, errors) != 0) return 1;
        break;
    default:
        errors.panic( UNEXPECTED_KEYWORD(keyword) );
        return 1;
    }

    return 0;
}
int parse_structure(Configuration& config, TokenList& tokens, Program& program, size_t& i, const AttributeInfo& attr_info, ErrorHandler& errors){
    // type struct_name { ... }
    //          ^

    std::string name = tokens[i].getString();
    std::vector<Field> members;

    // Make sure that is hasn't been already declared
    for(size_t i = 0; i != program.structures.size(); i++){
        if(name == program.structures[i].name){
            errors.panic(DUPLICATE_STRUCT(name));
            return 1;
        }
    }
    for(size_t i = 0; i != program.classes.size(); i++){
        if(name == program.classes[i].name){
            errors.panic(DUPLICATE_DEFINITION(name));
            return 1;
        }
    }

    next_index(i, tokens.size());
    uint16_t token = tokens[i].id;

    if(token == TOKENID_ASSIGN){
        // Instead of treating this like a structure, parse it as a type alias
        if(parse_type_alias(config, tokens, program, i, name, attr_info, errors) != 0) return 1;
        return 0;
    } else if(token != TOKENID_BEGIN){
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
            errors.panic("Expected member of structure");
            return 1;
        }

        std::string name = tokens[i].getString();
        std::string type;

        next_index(i, tokens.size());
        if(parse_type(config, tokens, program, i, type, errors) != 0) return 1;

        next_index(i, tokens.size());
        members.push_back( Field{name, type} );

        while(tokens[i].id == TOKENID_NEWLINE){
            errors.line++;
            next_index(i, tokens.size());
        }
    }

    program.structures.push_back( Structure(name, members, attr_info.is_public, attr_info.is_packed, &program.origin_info) );
    return 0;
}
int parse_enum(Configuration& config, TokenList& tokens, Program& program, size_t& i, const AttributeInfo& attr_info, ErrorHandler& errors){
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
int parse_class(Configuration& config, TokenList& tokens, Program& program, size_t& i, const AttributeInfo& attr_info, ErrorHandler& errors){
    // class class_name { ... }
    //           ^

    std::string name = tokens[i].getString();

    // Make sure that is hasn't been already declared
    for(size_t i = 0; i != program.classes.size(); i++){
        if(name == program.classes[i].name){
            errors.panic(DUPLICATE_CLASS(name));
            return 1;
        }
    }
    for(size_t i = 0; i != program.structures.size(); i++){
        if(name == program.structures[i].name){
            errors.panic(DUPLICATE_DEFINITION(name));
            return 1;
        }
    }

    // Allocate a new class in 'program.classes'
    std::vector<Class>* classes = &program.classes;
    classes->resize(classes->size()+1);

    // Store a reference to the created class
    Class* klass = &( (*classes)[classes->size()-1] );
    size_t class_offset_plus_one = classes->size();

    // Fill in the class data
    klass->name = name;
    klass->is_public = attr_info.is_public;
    klass->is_imported = false;
    klass->origin = &program.origin_info;

    // Skip over class name and '{'
    next_index(i, tokens.size());
    if(tokens[i].id != TOKENID_BEGIN){
        errors.panic("Expected '{' after class name");
        return 1;
    }
    next_index(i, tokens.size());

    // Parse leading newlines
    while(tokens[i].id == TOKENID_NEWLINE){
        errors.line++;
        next_index(i, tokens.size());
    }

    // Parse contents of the class
    while(tokens[i].id != TOKENID_END){
        AttributeInfo member_attr(false, false);

        // Encountered a keyword inside class definition
        if(tokens[i].id == TOKENID_KEYWORD){
            std::string keyword = tokens[i].getString();
            bool continue_loop = false;

            while(tokens[i].id == TOKENID_KEYWORD){
                keyword = tokens[i].getString();

                if(keyword == "def"){
                    // Parse the method
                    next_index(i, tokens.size());
                    if(parse_method(config, tokens, program, i, klass, class_offset_plus_one, member_attr, errors) != 0) return 1;
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
            errors.panic("Expected member of structure");
            return 1;
        }

        std::string name = tokens[i].getString();
        std::string type;

        next_index(i, tokens.size());
        if(parse_type(config, tokens, program, i, type, errors) != 0) return 1;

        next_index(i, tokens.size());
        klass->members.push_back( ClassField{name, type, member_attr.is_public, member_attr.is_static} );

        while(tokens[i].id == TOKENID_NEWLINE){
            errors.line++;
            next_index(i, tokens.size());
        }
    }

    return 0;
}
int parse_type_alias(Configuration& config, TokenList& tokens, Program& program, size_t& i, const std::string& alias, const AttributeInfo& attr_info, ErrorHandler& errors){
    // type type_alias = some_type
    //                 ^

    next_index(i, tokens.size());
    std::string binding;

    if(parse_type(config, tokens, program, i, binding, errors) != 0) return 1;
    program.type_aliases.push_back( TypeAlias(alias, binding, attr_info.is_public, &program.origin_info) );
    return 0;
}
int parse_function(Configuration& config, TokenList& tokens, Program& program, size_t& i, const AttributeInfo& attr_info, ErrorHandler& errors){
    // def func_name() ret_type { <some code> }
    //         ^

    std::string name = tokens[i].getString();
    next_index(i, tokens.size());

    if(tokens[i].id != TOKENID_OPEN){
        errors.panic("Expected open after function name");
        return 1;
    }
    next_index(i, tokens.size());

    std::vector<Field> arguments;
    std::string return_type;
    StatementList statements;
    StatementList defer_statements;

    while(tokens[i].id != TOKENID_CLOSE){
        if(tokens[i].id != TOKENID_WORD){
            errors.panic("Expected argument name");
            return 1;
        }

        std::string name = tokens[i].getString();
        std::string type;
        next_index(i, tokens.size());

        if(parse_type(config, tokens, program, i, type, errors) != 0) return 1;
        next_index(i, tokens.size());

        if(tokens[i].id != TOKENID_CLOSE) next_index(i, tokens.size());
        arguments.push_back( Field{name, type} );
    }

    if(tokens[i].id == TOKENID_BEGIN){
        errors.panic("Expected function return type after function name");
        return 1;
    }
    next_index(i, tokens.size());

    if(parse_type(config, tokens, program, i, return_type, errors) != 0) return 1;
    next_index(i, tokens.size());

    next_index(i, tokens.size());
    if(parse_block(config, tokens, program, statements, defer_statements, i, true, errors) != 0) return 1;

    // If function not terminated, make sure to add defer statements
    if(statements.size() != 0){
        if(!statements[statements.size()-1]->isTerminator()){
            for(size_t i = defer_statements.size(); i-- > 0;){
                statements.push_back( defer_statements[i]->clone() );
            }
        }
    }

    for(Statement* statement : defer_statements){
        // delete defered statements, they are no longer needed
        delete statement;
    }

    program.functions.push_back( Function(name, arguments, return_type, statements, attr_info.is_public, false, attr_info.is_stdcall, &program.origin_info) );
    return 0;
}
int parse_method(Configuration& config, TokenList& tokens, Program& program, size_t& i, Class* klass, size_t class_offset_plus_one, const AttributeInfo& attr_info, ErrorHandler& errors){
    // def method_name() ret_type { <some code> }
    //           ^

    // NOTE: 'Class* klass' should be a valid pointer when calling this function (if not a crash will occur)

    // NOTE: 'Class* klass' should only used for adding the created method to the class,
    //           if it is used for accessing things like members, not all of them may be present,
    //           or none of them at all

    std::string name = tokens[i].getString();
    next_index(i, tokens.size());

    if(tokens[i].id != TOKENID_OPEN){
        errors.panic("Expected open after method name");
        return 1;
    }
    next_index(i, tokens.size());

    std::vector<Field> arguments;
    std::string return_type;
    StatementList statements;
    StatementList defer_statements;

    while(tokens[i].id != TOKENID_CLOSE){
        if(tokens[i].id != TOKENID_WORD){
            errors.panic("Expected argument name");
            return 1;
        }

        std::string name = tokens[i].getString();
        std::string type;
        next_index(i, tokens.size());

        if(parse_type(config, tokens, program, i, type, errors) != 0) return 1;
        next_index(i, tokens.size());

        if(tokens[i].id != TOKENID_CLOSE) next_index(i, tokens.size());
        arguments.push_back( Field{name, type} );
    }

    if(tokens[i].id == TOKENID_BEGIN){
        errors.panic("Expected method return type after method name");
        return 1;
    }

    next_index(i, tokens.size());
    if(parse_type(config, tokens, program, i, return_type, errors) != 0) return 1;
    next_index(i, tokens.size());

    if(return_type == ""){
        errors.panic("No return type specified for method '" + klass->name + "." + name + "'");
        return 1;
    }

    next_index(i, tokens.size());
    if(parse_block(config, tokens, program, statements, defer_statements, i, true, errors) != 0) return 1;

    // If method not terminated, make sure to add defer statements
    if(statements.size() != 0){
        if(!statements[statements.size()-1]->isTerminator()){
            for(size_t i = defer_statements.size(); i-- > 0;){
                statements.push_back( defer_statements[i]->clone() );
            }
        }
    }

    for(Statement* statement : defer_statements){
        // delete defered statements, they are no longer needed
        delete statement;
    }

    Function created_method(name, arguments, return_type, statements, attr_info.is_public, attr_info.is_static, attr_info.is_stdcall, &program.origin_info);
    created_method.parent_class_offset = class_offset_plus_one;
    klass->methods.push_back( std::move(created_method) );
    return 0;
}
int parse_external(Configuration& config, TokenList& tokens, Program& program, size_t& i, const AttributeInfo& attr_info, ErrorHandler& errors){
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

        if(tokens[i].id != TOKENID_CLOSE) next_index(i, tokens.size());
        arguments.push_back(type);
    }

    next_index(i, tokens.size());
    if(parse_type(config, tokens, program, i, return_type, errors) != 0) return 1;
    next_index(i, tokens.size());

    errors.line++;
    program.externs.push_back( External(name, arguments, return_type, attr_info.is_public, false, attr_info.is_stdcall, &program.origin_info) );
    return 0;
}
int parse_attribute(Configuration& config, TokenList& tokens, Program& program, size_t& i, ErrorHandler& errors){
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
int parse_import(Configuration& config, TokenList& tokens, Program& program, size_t& i, const AttributeInfo& attr_info, ErrorHandler& errors){
    // import PackageName
    //             ^

    using namespace boost::filesystem;

    if(tokens[i].id != TOKENID_STRING){
        errors.panic("Expected module name after 'import'");
        return 1;
    }

    std::string name = tokens[i].getString();
    TokenList* import_tokens;
    Configuration* import_config = new Configuration(config);
    Program* import_program;
    std::string target_bc;
    std::string target_obj;

    import_config->silent = config.silent;
    import_config->time = config.time;
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
    target_obj  = (config.obj)      ? (mangled_name + ".o") : "C:/Users/" + config.username + "/.adept/obj/module_cache/" + mangled_name + ".o";
    target_bc   = (config.bytecode) ? (mangled_name + ".bc")  : "C:/Users/" + config.username + "/.adept/obj/module_cache/" + mangled_name + ".bc";

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

    bool is_nothing = (import_program->functions.size() == 0 and import_program->classes.size() == 0 and import_program->globals.size() == 0);
    program.dependencies.push_back( ModuleDependency(source_filename, target_bc, target_obj, import_config, is_nothing) );
    return 0;
}
int parse_lib(Configuration& config, TokenList& tokens, Program& program, size_t& i, ErrorHandler& errors){
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
int parse_constant(Configuration& config, TokenList& tokens, Program& program, size_t& i, const AttributeInfo& attr_info, ErrorHandler& errors){
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
int parse_global(Configuration& config, TokenList& tokens, Program& program, size_t& i, std::string name, const AttributeInfo& attr_info, ErrorHandler& errors){
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
        global->is_public = attr_info.is_public;
        global->is_imported = false;
        global->origin = &program.origin_info;
        global->errors = errors;
    }
    return 0;
}
int parse_type(Configuration& config, TokenList& tokens, Program& program, size_t& i, std::string& output_type, ErrorHandler& errors){
    //  **some_type   |   def (int, int) int   |   another_type   |   [] int   |   *@TypeName
    //  ^                  ^                            ^             ^            ^

    uint16_t prefix_token_id = tokens[i].id;

    while(prefix_token_id == TOKENID_MULTIPLY or prefix_token_id == TOKENID_BRACKET_OPEN){
        if(prefix_token_id == TOKENID_MULTIPLY){
            output_type += "*";
        }
        else if(prefix_token_id == TOKENID_BRACKET_OPEN){
            next_index(i, tokens.size());
            if(tokens[i].id != TOKENID_BRACKET_CLOSE){
                errors.panic("Expected ']' after '[' in type");
                return 1;
            }
            output_type += "[]";
        }

        next_index(i, tokens.size());
        prefix_token_id = tokens[i].id;
    }

    if(prefix_token_id == TOKENID_KEYWORD){
        std::string keyword = tokens[i].getString();

        if(keyword == "stdcall"){
            // Function pointer type

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
            if(tokens[i].id != TOKENID_OPEN){
                errors.panic("Expected '(' after keyword 'def' in function pointer type");
                return 1;
            }

            next_index(i, tokens.size());
            output_type += "stdcall def(";

            std::vector<std::string> arguments;
            std::string return_type;

            while(tokens[i].id != TOKENID_CLOSE){
                std::string type;

                if(parse_type(config, tokens, program, i, type, errors) != 0) return 1;
                next_index(i, tokens.size());

                if(tokens[i].id != TOKENID_CLOSE) next_index(i, tokens.size());
                arguments.push_back(type);
            }

            next_index(i, tokens.size());
            if(parse_type(config, tokens, program, i, return_type, errors) != 0) return 1;

            for(size_t a = 0; a != arguments.size(); a++){
                output_type += arguments[a];
                if(a + 1 != arguments.size()) output_type += ", ";
            }
            output_type += ") " + return_type;
        }
        else if(keyword == "def"){
            // Function pointer type

            next_index(i, tokens.size());
            if(tokens[i].id != TOKENID_OPEN){
                errors.panic("Expected '(' after keyword 'def' in function pointer type");
                return 1;
            }

            next_index(i, tokens.size());
            output_type += "def(";

            std::vector<std::string> arguments;
            std::string return_type;

            while(tokens[i].id != TOKENID_CLOSE){
                std::string type;

                if(parse_type(config, tokens, program, i, type, errors) != 0) return 1;
                next_index(i, tokens.size());

                if(tokens[i].id != TOKENID_CLOSE) next_index(i, tokens.size());
                arguments.push_back(type);
            }

            next_index(i, tokens.size());
            if(parse_type(config, tokens, program, i, return_type, errors) != 0) return 1;

            for(size_t a = 0; a != arguments.size(); a++){
                output_type += arguments[a];
                if(a + 1 != arguments.size()) output_type += ", ";
            }
            output_type += ") " + return_type;
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

int parse_block(Configuration& config, TokenList& tokens, Program& program, StatementList& statements, StatementList& defer_statements, size_t& i, bool can_defer, ErrorHandler& errors){
    // { some code; some more code; }
    //    ^

    while(tokens[i].id != TOKENID_END){
        switch(tokens[i].id){
        case TOKENID_NEWLINE:
            errors.line++;
            next_index(i, tokens.size());
            break;
        case TOKENID_KEYWORD:
            if(parse_block_keyword(config, tokens, program, statements, defer_statements, i, tokens[i].getString(), can_defer, errors) != 0) return 1;
            break;
        case TOKENID_WORD:
            if(parse_block_word(config, tokens, program, statements, defer_statements, i, errors) != 0) return 1;
            break;
        case TOKENID_MULTIPLY: // multiply/pointer operator
            if(parse_block_dereference(config, tokens, program, statements, i, errors) != 0) return 1;
            break;
        default:
            errors.panic("Expected statement, received '" + tokens[i].syntax() + "'");
            return 1;
        }
    }

    return 0;
}
int parse_block_keyword(Configuration& config, TokenList& tokens, Program& program, StatementList& statements, StatementList& defer_statements, size_t& i, std::string keyword, bool can_defer, ErrorHandler& errors){
    // keyword <unknown syntax follows>
    //    ^

    const size_t accepted_keywords_size = 9;
    const std::string accepted_keywords[] = {
        "defer", "delete", "for", "if", "return", "switch", "unless", "until", "while"
    };

    size_t string_index = string_search(accepted_keywords, accepted_keywords_size, keyword);

    switch(string_index){
    case 0: { // defer
        next_index(i, tokens.size());

        if(!can_defer){
            errors.panic("Statements currently can't be defered while in conditionals");
            return 1;
        }

        switch(tokens[i].id){
        case TOKENID_KEYWORD:
            // NOTE: 'defer defer <statement>' acts the same ways as 'defer <statement>' (Number of defers does not metter, maybe add a check here idk)
            if(parse_block_keyword(config, tokens, program, defer_statements, defer_statements, i, tokens[i].getString(), true, errors) != 0) return 1;
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
    case 1: { // delete
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
    case 2: { // for
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
            if(parse_block_keyword(config, tokens, program, for_condition_statements, defer_statements, i, tokens[i].getString(), false, errors) != 0) return 1;
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
            if(parse_block_keyword(config, tokens, program, for_condition_statements, defer_statements, i, tokens[i].getString(), false, errors) != 0) return 1;
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
        if(parse_block(config, tokens, program, for_statements, defer_statements, i, false, errors) != 0) return 1;
        next_index(i, tokens.size());

        // Skip any newlines
        while(tokens[i].id == TOKENID_NEWLINE){
            errors.line++;
            next_index_else(i, tokens.size(), errors.format("Expected '}' to close off block"));
        }

        statements.push_back( new ForStatement(for_condition_statements[0], expression, for_condition_statements[1], for_statements, errors) );
        break;
    }
    case 3: { // if
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
    case 4: { // return
            PlainExp* expression;
            next_index(i, tokens.size());

            for(size_t i = defer_statements.size(); i-- > 0;){
                statements.push_back( defer_statements[i]->clone() );
            }

            if(tokens[i].id == TOKENID_NEWLINE){
                statements.push_back( new ReturnStatement(NULL, errors) );
            } else {
                if(parse_expression(config, tokens, program, i, &expression, errors) != 0) return 1;
                statements.push_back( new ReturnStatement(expression, errors) );
            }

            break;
        }
    case 5: // switch
        if(parse_block_switch(config, tokens, program, statements, defer_statements, i, errors) != 0) return 1;
        break;
    case 6: { // unless
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
    case 7: // until
        if(parse_block_conditional(config, tokens, program, statements, defer_statements, i, STATEMENTID_UNTIL, errors) != 0) return 1;
        break;
    case 8: // while
        if(parse_block_conditional(config, tokens, program, statements, defer_statements, i, STATEMENTID_WHILE, errors) != 0) return 1;
        break;
    default:
        errors.panic( UNEXPECTED_KEYWORD(keyword) );
        return 1;
    }

    return 0;
}
int parse_block_word(Configuration& config, TokenList& tokens, Program& program, StatementList& statements, StatementList& defer_statements, size_t& i, ErrorHandler& errors){
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
        if(parse_block_variable_declaration(config, tokens, program, statements, defer_statements, i, name, errors) != 0) return 1;
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
        next_index(i, tokens.size());
        if(tokens[i].id == TOKENID_BRACKET_CLOSE){
            if(parse_block_variable_declaration(config, tokens, program, statements, defer_statements, --i, name, errors) != 0) return 1;
        }
        else {
            if(parse_block_word_expression(config, tokens, program, statements, --i, name, 0, errors) != 0) return 1;
        }
        break;
    default:
        errors.panic( UNEXPECTED_OPERATOR(tokens[i].syntax()) );
        return 1;
    }

    return 0;
}
int parse_block_variable_declaration(Configuration& config, TokenList& tokens, Program& program, StatementList& statements, StatementList& defer_statements, size_t& i, std::string name, ErrorHandler& errors){
    // name str = "Hello World"   |   name, another_name str = "Greetings World"
    //       ^

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
        if(!Program::is_pointer_typename(type)){
            errors.panic("The non-pointer type '" + type + "' can't be deleted later on in the program.\n    Only pointer types work with '!' deletion.");
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
int parse_block_call(Configuration& config, TokenList& tokens, Program& program, StatementList& statements, size_t& i, std::string name, ErrorHandler& errors){
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
int parse_block_word_expression(Configuration& config, TokenList& tokens, Program& program, StatementList& statements, size_t& i, std::string name, int loads, ErrorHandler& errors){
    // name += 10 * 3 / 4   |   name[i] = 10 * 3 / 4   |   name.member *= 10 * 3 / 4   |   name.member.call()
    //      ^                      ^                          ^                              ^

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
int parse_block_dereference(Configuration& config, TokenList& tokens, Program& program, StatementList& statements, size_t& i, ErrorHandler& errors){
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
int parse_block_conditional(Configuration& config, TokenList& tokens, Program& program, StatementList& statements, StatementList& defer_statements, size_t& i, uint16_t conditional_type, ErrorHandler& errors){
    // conditional <condition> { ... }
    //      ^

    PlainExp* expression;
    StatementList conditional_statements;
    next_index(i, tokens.size());

    if(parse_expression(config, tokens, program, i, &expression, errors) != 0) return 1;
    if(tokens[i].id != TOKENID_BEGIN){
        errors.panic("Expected '{' after conditional");
        return 1;
    }

    next_index(i, tokens.size());
    if(parse_block(config, tokens, program, conditional_statements, defer_statements, i, false, errors) != 0) return 1;
    next_index(i, tokens.size());

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
                if(parse_block(config, tokens, program, else_statements, defer_statements, i, false, errors) != 0) return 1;
                next_index(i, tokens.size());
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
int parse_block_word_member(Configuration& config, TokenList& tokens, Program& program, StatementList& statements, size_t& i, std::string name, ErrorHandler& errors){
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
int parse_block_member_call(Configuration& config, TokenList& tokens, Program& program, StatementList& statements, size_t& i, PlainExp* value, std::string func_name, ErrorHandler& errors){
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
int parse_block_switch(Configuration& config, TokenList& tokens, Program& program, StatementList& statements, StatementList& defer_statements, size_t& i, ErrorHandler& errors){
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
                        cases.push_back( SwitchStatement::Case(case_condition, possible_statements) );
                    } else if(statement_context == StatementContext::Default){
                        default_statements = possible_statements;
                    }

                    next_index(i, tokens.size());
                    if(parse_expression(config, tokens, program, i, &case_condition, errors) != 0) return 1;

                    statement_context = StatementContext::Case;
                    possible_statements.clear();
                } else if(keyword == "default"){
                    if(statement_context == StatementContext::Case){
                        cases.push_back( SwitchStatement::Case(case_condition, possible_statements) );
                    } else if(statement_context == StatementContext::Default){
                        default_statements = possible_statements;
                    }

                    if(default_statements.size() != 0){
                        errors.panic("Attempting to define a default case when a default case already exists");
                        return 1;
                    }

                    statement_context = StatementContext::Default;
                    possible_statements.clear();
                    next_index(i, tokens.size());
                } else {
                    if(statement_context == StatementContext::None){
                        errors.panic("Statement must be part of a case because it's inside a switch statement");
                        return 1;
                    }

                    if(parse_block_keyword(config, tokens, program, possible_statements, defer_statements, i, keyword, false, errors) != 0) return 1;
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
        cases.push_back( SwitchStatement::Case(case_condition, possible_statements) );
    } else if(statement_context == StatementContext::Default){
        default_statements = possible_statements;
    }

    statements.push_back( new SwitchStatement(switch_condition, cases, default_statements, switch_errors) );
    next_index(i, tokens.size());
    return 0;
}

int parse_expression(Configuration& config, TokenList& tokens, Program& program, size_t& i, PlainExp** expression, ErrorHandler& errors){
    // 10 + 3 * 6
    // ^

    if(parse_expression_primary(config, tokens, program, i, expression, errors) != 0) return 1;
    if(parse_expression_operator_right(config, tokens, program, i, 0, expression, false, errors) != 0) return 1;
    return 0;
}
int parse_expression_primary(Configuration& config, TokenList& tokens, Program& program, size_t& i, PlainExp** expression, ErrorHandler& errors){
    while(tokens[i].id == TOKENID_NEWLINE){
        next_index(i, tokens.size());
        errors.line++;
    }

    // TODO: CLEAN: Clean up code inside each case (probally refactor into different functions or something)
    switch (tokens[i].id) {
    case TOKENID_WORD:
        next_index(i, tokens.size());
        if(tokens[i].id == TOKENID_OPEN){
            if(parse_expression_call(config, tokens, program, --i, expression, errors) != 0) return 1;
        }
        else if(tokens[i].id == TOKENID_NAMESPACE){
            std::string name_space = tokens[i-1].getString();
            next_index(i, tokens.size());
            if(tokens[i].id != TOKENID_WORD){
                errors.panic("Expected identifier after '::' operator");
                return 1;
            }
            std::string member = tokens[i].getString();
            *expression = new NamespaceExp(name_space, member, errors);
            next_index(i, tokens.size());
        }
        else {
            *expression = new WordExp( tokens[i-1].getString(), errors );
        }
        return 0;
    case TOKENID_KEYWORD:
        {
            // TODO: Use string_search here
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
                    std::string type;
                    if(parse_type(config, tokens, program, i, type, errors) != 0) return 1;
                    function_arguments.push_back(type);
                    next_index(i, tokens.size());

                    if(tokens[i].id == TOKENID_NEXT){
                        next_index(i, tokens.size());
                    }
                }

                next_index(i, tokens.size());
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

                    if(tokens[i].id != TOKENID_INT){
                        errors.panic("Expected integer value after '*' in 'new' operator");
                        return 1;
                    }

                    new_amount = tokens[i].getInt();

                    if(new_amount < 1){
                        errors.panic("Must allocate at least one object when using the 'new' operator");
                        return 1;
                    }

                    amount = new_amount;
                    next_index(i, tokens.size());
                }

                *expression = new AllocExp(new_typename, amount, element_amount, errors);
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

            *expression = new AddrWordExp( mutable_expression, errors );
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
int parse_expression_operator_right(Configuration& config, TokenList& tokens, Program& program, size_t& i, int precedence, PlainExp** left, bool keep_mutable, ErrorHandler& errors) {
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
int parse_expression_call(Configuration& config, TokenList& tokens, Program& program, size_t& i, PlainExp** expression, ErrorHandler& errors){
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
