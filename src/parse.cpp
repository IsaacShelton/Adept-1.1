
#include <unistd.h>
#include <iostream>
#include "llvm/Support/DynamicLibrary.h"

#include "../include/die.h"
#include "../include/type.h"
#include "../include/parse.h"
#include "../include/lexer.h"
#include "../include/errors.h"
#include "../include/strings.h"

int parse(Configuration& config, TokenList* tokens, Program& program, ErrorHandler& errors){
    // NOTE: This function deletes 'tokens' after it is done parsing

    for(size_t i = 0; i != tokens->size(); i++){
        if(parse_token(config, *tokens, program, i, errors) != 0) return 1;
    }

    delete tokens; // Free Tokens

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
        if(parse_word(config, tokens, program, i, errors) != 0) return 1;
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
int parse_word(Configuration& config, TokenList& tokens, Program& program, size_t& i, ErrorHandler& errors){
    // a_word <unknown syntax follows>
    //   ^

    std::string name = tokens[i].getString();
    next_index(i, tokens.size());

    switch(tokens[i].id){
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

    if(keyword == "def"){
        if(parse_function(config, tokens, program, i, attr_info, errors) != 0) return 1;
    }
    else if(keyword == "foreign"){
        if(parse_external(config, tokens, program, i, attr_info, errors) != 0) return 1;
    }
    else if(keyword == "public" or keyword == "private"){
        if(parse_attribute(config, tokens, program, --i, errors) != 0) return 1;
    }
    else if(keyword == "type"){
        if(parse_structure(config, tokens, program, i, attr_info, errors) != 0) return 1;
    }
    else if(keyword == "import"){
        if(parse_import(config, tokens, program, i, attr_info, errors) != 0) return 1;
    }
    else if(keyword == "link"){
        if(parse_lib(config, tokens, program, i, errors) != 0) return 1;
    }
    else if(keyword == "dynamic"){
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
    }
    else if(keyword == "constant"){
        if(parse_constant(config, tokens, program, i, attr_info, errors) != 0) return 1;
    }
    else {
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
    next_index(i, tokens.size());
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

        if(tokens[i].id != TOKENID_WORD and tokens[i].id != TOKENID_MULTIPLY){
            errors.panic(EXPECTED_NAME_OF_TYPE);
            return 1;
        }

        while(tokens[i].id == TOKENID_MULTIPLY){
            type += "*";
            next_index(i, tokens.size());
        }

        if(tokens[i].id != TOKENID_WORD){
            errors.panic(EXPECTED_NAME_OF_TYPE);
            return 1;
        }

        type += tokens[i].getString();
        next_index(i, tokens.size());
        members.push_back( Field{name, type} );

        while(tokens[i].id == TOKENID_NEWLINE){
            errors.line++;
            next_index(i, tokens.size());
        }
    }

    program.structures.push_back( Structure{name, members, attr_info.is_public} );
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

    while(tokens[i].id != TOKENID_CLOSE){
        if(tokens[i].id != TOKENID_WORD){
            errors.panic("Expected argument name");
            return 1;
        }

        std::string name = tokens[i].getString();
        std::string type;
        next_index(i, tokens.size());

        while(tokens[i].id == TOKENID_MULTIPLY){
            type += "*";
            next_index(i, tokens.size());
        }

        if(tokens[i].id != TOKENID_WORD){
            errors.panic("Expected argument type");
            return 1;
        }

        type += tokens[i].getString();
        next_index(i, tokens.size());

        if(tokens[i].id != TOKENID_CLOSE) next_index(i, tokens.size());
        arguments.push_back( Field{name, type} );
    }

    if(tokens[i].id == TOKENID_BEGIN){
        errors.panic("Expected function return type after function name");
        return 1;
    }
    next_index(i, tokens.size());

    while(tokens[i].id != TOKENID_BEGIN){
        switch(tokens[i].id){
        case TOKENID_MULTIPLY:
            return_type += "*";
            break;
        case TOKENID_WORD:
            return_type += tokens[i].getString();
            break;
        default:
            errors.panic( UNEXPECTED_OPERATOR(tokens[i].toString()) );
            return 1;
        }
        next_index(i, tokens.size());
    }

    if(return_type == ""){
        errors.panic("No return type specified for function '" + name + "'");
        return 1;
    }

    next_index(i, tokens.size());
    if(parse_block(config, tokens, program, statements, i, errors) != 0) return 1;

    program.functions.push_back( Function{name, arguments, return_type, statements, attr_info.is_public} );
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

        while(tokens[i].id == TOKENID_MULTIPLY){
            type += "*";
            next_index(i, tokens.size());
        }

        if(tokens[i].id != TOKENID_WORD){
            errors.panic(EXPECTED_NAME_OF_TYPE);
            return 1;
        }
        type += tokens[i].getString();
        next_index(i, tokens.size());

        if(tokens[i].id != TOKENID_CLOSE) next_index(i, tokens.size());
        arguments.push_back(type);
    }

    next_index(i, tokens.size());
    while(tokens[i].id != TOKENID_NEWLINE){
        switch(tokens[i].id){
        case TOKENID_MULTIPLY:
            return_type += "*";
            break;
        case TOKENID_WORD:
            return_type += tokens[i].getString();
            break;
        default:
            errors.panic( UNEXPECTED_OPERATOR(tokens[i].toString()) );
            return 1;
        }
        next_index(i, tokens.size());
    }

    if(return_type == ""){
        errors.panic("No return type specified for function '" + name + "'");
        return 1;
    }

    errors.line++;
    program.externs.push_back( External{name, arguments, return_type, attr_info.is_public, false} );
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
        else {
            // Not an attribute, continue on
            break;
        }
    }

    if(parse_keyword(config, tokens, program, i, attr_info, errors) != 0) return 1;
    return 0;
}
int parse_import(Configuration& config, TokenList& tokens, Program& program, size_t& i, const AttributeInfo& attr_info, ErrorHandler& errors){
    // import PackageName
    //             ^

    if(tokens[i].id != TOKENID_STRING){
        errors.panic("Expected module name after 'import'");
        return 1;
    }

    std::string name = tokens[i].getString();
    TokenList* import_tokens = new TokenList;
    Configuration* import_config = new Configuration(config);
    Program* import_program = new Program;
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
        return 1;
    }

    ErrorHandler error_handler(errors_name);
    import_config->filename = name;
    if( tokenize(*import_config, name, import_tokens, error_handler) != 0 )         return 1;
    if( parse(*import_config, import_tokens, *import_program, error_handler) != 0 ) return 1; // Deletes imported_tokens

    // Resolve imports
    std::string mangled_name = name;

    // TODO: Clean up name mangling code
    mangled_name = string_replace_all(mangled_name, ":",  "$1");
    mangled_name = string_replace_all(mangled_name, "/",  "$2");
    mangled_name = string_replace_all(mangled_name, "\\", "$3");
    mangled_name = string_replace_all(mangled_name, ".",  "$4");

    target_obj  = (config.obj)      ? filename_change_ext(name, "obj") : "C:/Users/" + config.username + "/.adept/obj/module_cache/" + mangled_name + ".o";
    target_bc   = (config.bytecode) ? filename_change_ext(name, "bc")  : "C:/Users/" + config.username + "/.adept/obj/module_cache/" + mangled_name + ".bc";

    program.imports.push_back( ModuleDependency(name, target_bc, target_obj, import_program, import_config) );
    if(program.import_merge(*import_program, attr_info.is_public) != 0) return 1;

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
    program.constants.push_back( Constant(name, value, attr_info.is_public) );
    i--;
    return 0;
}

int parse_block(Configuration& config, TokenList& tokens, Program& program, StatementList& statements, size_t& i, ErrorHandler& errors){
    // { some code; some more code; }
    //    ^

    while(tokens[i].id != TOKENID_END){
        switch(tokens[i].id){
        case TOKENID_NEWLINE:
            errors.line++;
            next_index(i, tokens.size());
            break;
        case TOKENID_KEYWORD:
            if(parse_block_keyword(config, tokens, program, statements, i, tokens[i].getString(), errors) != 0) return -1;
            break;
        case TOKENID_WORD:
            if(parse_block_word(config, tokens, program, statements, i, errors) != 0) return -1;
            break;
        case TOKENID_MULTIPLY: // multiply/pointer operator
            if(parse_block_dereference(config, tokens, program, statements, i, errors) != 0) return -1;
            break;
        default:
            errors.panic("Expected Statement, got token #" + to_str(tokens[i].id));
            return 1;
        }
    }

    return 0;
}
int parse_block_keyword(Configuration& config, TokenList& tokens, Program& program, StatementList& statements, size_t& i, std::string keyword, ErrorHandler& errors){
    // keyword <unknown syntax follows>
    //    ^

    if(keyword == "return"){
        PlainExp* expression;
        next_index(i, tokens.size());

        if(tokens[i].id == TOKENID_NEWLINE){
            statements.push_back( STATEMENT_RETURN(NULL, errors) );
        }
        else {
            if(parse_expression(config, tokens, program, i, &expression, errors) != 0) return 1;
            statements.push_back(std::move( STATEMENT_RETURN(expression, errors) ));
        }
    }
    else if(keyword == "if"){
        if(parse_block_conditional(config, tokens, program, statements, i, CONDITIONAL_IF, errors) != 0) return 1;
    }
    else if(keyword == "while"){
        if(parse_block_conditional(config, tokens, program, statements, i, CONDITIONAL_WHILE, errors) != 0) return 1;
    }
    else if(keyword == "unless"){
        if(parse_block_conditional(config, tokens, program, statements, i, CONDITIONAL_UNLESS, errors) != 0) return 1;
    }
    else if(keyword == "until"){
        if(parse_block_conditional(config, tokens, program, statements, i, CONDITIONAL_UNTIL, errors) != 0) return 1;
    }
    else {
        errors.panic( UNEXPECTED_KEYWORD(keyword) );
        return 1;
    }

    return 0;
}
int parse_block_word(Configuration& config, TokenList& tokens, Program& program, StatementList& statements, size_t& i, ErrorHandler& errors){
    // word <unknown syntax follows>
    //  ^

    std::string name = tokens[i].getString();
    next_index(i, tokens.size());

    switch(tokens[i].id){
    case TOKENID_WORD:
    case TOKENID_MULTIPLY:
        // Variable Definition
        if(parse_block_variable_declaration(config, tokens, program, statements, i, name, errors) != 0) return 1;
        break;
    case TOKENID_OPEN:
        // Function Call
        if(parse_block_call(config, tokens, program, statements, i, name, errors) != 0) return 1;
        break;
    case TOKENID_ASSIGN:
    case TOKENID_BRACKET_OPEN:
        // Variable Assign
        if(parse_block_assign(config, tokens, program, statements, i, name, 0, errors) != 0) return 1;
        break;
    case TOKENID_MEMBER:
        // Variable Assign
        if(parse_block_member_assign(config, tokens, program, statements, i, name, errors) != 0) return 1;
        break;
    default:
        errors.panic( UNEXPECTED_OPERATOR(tokens[i].toString()) );
        return 1;
    }

    return 0;
}
int parse_block_variable_declaration(Configuration& config, TokenList& tokens, Program& program, StatementList& statements, size_t& i, std::string name, ErrorHandler& errors){
    // name str = "Hello World"
    //       ^

    std::string type;
    while(tokens[i].id == TOKENID_MULTIPLY){
        type += "*";
        next_index(i, tokens.size());
    }

    if(tokens[i].id != TOKENID_WORD){
        errors.panic(EXPECTED_NAME_OF_TYPE);
        return 1;
    }

    type += tokens[i].getString();
    next_index(i, tokens.size());

    if(tokens[i].id == TOKENID_ASSIGN){
        PlainExp* expression;
        next_index(i, tokens.size());

        if(parse_expression(config, tokens, program, i, &expression, errors) != 0) return 1;
        statements.push_back( STATEMENT_DECLAREAS(name, type, expression, errors) );
    }
    else {
        statements.push_back( STATEMENT_DECLARE(name, type, errors) );
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

    statements.push_back( STATEMENT_CALL(name, args, errors) );
    next_index(i, tokens.size());
    return 0;
}
int parse_block_assign(Configuration& config, TokenList& tokens, Program& program, StatementList& statements, size_t& i, std::string name, int loads, ErrorHandler& errors){
    // name = 10 * 3 / 4   |   name[i] = 10 * 3 / 4
    //      ^                      ^

    PlainExp* expression;
    std::vector<PlainExp*> gep_loads;

    if(tokens[i].id == TOKENID_BRACKET_OPEN){
        PlainExp* int_expr;

        while(tokens[i].id == TOKENID_BRACKET_OPEN){
            next_index(i, tokens.size());
            if(parse_expression(config, tokens, program, i, &int_expr, errors) != 0) return 1;

            if(tokens[i].id != TOKENID_BRACKET_CLOSE){
                errors.panic("Expected closing bracket ']'");
                return 1;
            }

            gep_loads.push_back(int_expr);
            next_index(i, tokens.size());
        }
    }

    next_index(i, tokens.size());
    if(parse_expression(config, tokens, program, i, &expression, errors) != 0) return 1;
    statements.push_back( STATEMENT_ASSIGN(name, expression, loads, gep_loads, errors) );
    return 0;
}
int parse_block_member_assign(Configuration& config, TokenList& tokens, Program& program, StatementList& statements, size_t& i, std::string name, ErrorHandler& errors){
    // name:member:member = 10 * 3 / 4
    //     ^

    std::vector<AssignMemberPathNode> path = { AssignMemberPathNode{name, std::vector<PlainExp*>()} };
    PlainExp* expression;

    while(tokens[i].id == TOKENID_MEMBER){
        next_index(i, tokens.size());
        if(tokens[i].id != TOKENID_WORD){
            errors.panic("Expected word after ':' operator");
            return 1;
        }

        path.push_back( AssignMemberPathNode{tokens[i].getString(), std::vector<PlainExp*>()} );
        next_index(i, tokens.size());
    }

    if(tokens[i].id != TOKENID_ASSIGN){
        errors.panic("Expected '=' operator after member of variable");
        return 1;
    }
    next_index(i, tokens.size());

    if(parse_expression(config, tokens, program, i, &expression, errors) != 0) return 1;
    statements.push_back( STATEMENT_ASSIGNMEMBER(path, expression, 0, errors) );
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

    if(parse_block_assign(config, tokens, program, statements, i, name, deref_count, errors) != 0) return 1;
    return 0;
}
int parse_block_conditional(Configuration& config, TokenList& tokens, Program& program, StatementList& statements, size_t& i, uint16_t conditional_type, ErrorHandler& errors){
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
    if(parse_block(config, tokens, program, conditional_statements, i, errors) != 0) return 1;
    next_index(i, tokens.size());

    // Skip any newlines
    while(tokens[i].id == TOKENID_NEWLINE){
        errors.line++;
        next_index(i, tokens.size());
    }

    if(tokens[i].id == TOKENID_KEYWORD){
        if(tokens[i].getString() == "else"){
            // There is an else after the conditional
            StatementList else_statements;
            next_index(i, tokens.size());

            if(tokens[i].id == TOKENID_BEGIN){
                next_index(i, tokens.size());
                if(parse_block(config, tokens, program, else_statements, i, errors) != 0) return 1;
                next_index(i, tokens.size());
            }
            else if(tokens[i].id == TOKENID_KEYWORD){
                uint16_t else_conditional_type;
                std::string else_conditional = tokens[i].getString();

                if(else_conditional == "if"){
                    else_conditional_type = CONDITIONAL_IF;
                }
                else if(else_conditional == "unless"){
                    else_conditional_type = CONDITIONAL_UNLESS;
                }
                else {
                    errors.panic( UNEXPECTED_KEYWORD(else_conditional) );
                    return 1;
                }

                parse_block_conditional(config, tokens, program, else_statements, i, else_conditional_type, errors);
            }
            else {
                errors.panic("Expected '{' or 'if' after 'else'");
                return 1;
            }

            switch(conditional_type){
            case CONDITIONAL_IF:
                statements.push_back(std::move( STATEMENT_IFELSE(expression, conditional_statements, else_statements, errors) ));
                break;
            case CONDITIONAL_UNLESS:
                statements.push_back(std::move( STATEMENT_UNLESSELSE(expression, conditional_statements, else_statements, errors) ));
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
    case CONDITIONAL_IF:
        statements.push_back(std::move( STATEMENT_IF(expression, conditional_statements, errors) ));
        break;
    case CONDITIONAL_WHILE:
        statements.push_back(std::move( STATEMENT_WHILE(expression, conditional_statements, errors) ));
        break;
    case CONDITIONAL_UNLESS:
        statements.push_back(std::move( STATEMENT_UNLESS(expression, conditional_statements, errors) ));
        break;
    case CONDITIONAL_UNTIL:
        statements.push_back(std::move( STATEMENT_UNTIL(expression, conditional_statements, errors) ));
        break;
    default:
        errors.panic("Invalid conditional type");
        return 1;
    }

    return 0;
}

int parse_expression(Configuration& config, TokenList& tokens, Program& program, size_t& i, PlainExp** expression, ErrorHandler& errors){
    // 10 + 3 * 6
    // ^

    if(parse_expression_primary(config, tokens, program, i, expression, errors) != 0) return 1;
    if(parse_expression_operator_right(config, tokens, program, i, 0, expression, errors) != 0) return 1;
    return 0;
}
int parse_expression_primary(Configuration& config, TokenList& tokens, Program& program, size_t& i, PlainExp** expression, ErrorHandler& errors){
    switch (tokens[i].id) {
    case TOKENID_WORD:
        next_index(i, tokens.size());
        if(tokens[i].id == TOKENID_OPEN){
            if(parse_expression_call(config, tokens, program, --i, expression, errors) != 0) return 1;
        }
        else {
            *expression = new WordExp( tokens[i-1].getString(), errors );
        }
        return 0;
    case TOKENID_KEYWORD:
        {
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
                while(tokens[i].id == TOKENID_MULTIPLY){
                    target_typename += "*";
                    next_index(i, tokens.size());
                }
                if(tokens[i].id != TOKENID_WORD){
                    errors.panic(EXPECTED_NAME_OF_TYPE);
                    return 1;
                }
                target_typename += tokens[i].getString();
                next_index(i, tokens.size());

                if(tokens[i].id != TOKENID_OPEN){
                    errors.panic("Expected '(' after target cast type");
                    return 1;
                }
                PlainExp* cast_expr;
                next_index(i, tokens.size());
                if(parse_expression(config, tokens, program, i, &cast_expr, errors) != 0) return 1;
                next_index(i, tokens.size());
                *expression = new CastExp(cast_expr, target_typename, errors);
            } else {
                errors.panic( UNEXPECTED_KEYWORD_INEXPR(keyword) );
                return 1;
            }
        }
        return 0;
    case TOKENID_CONSTANT:
        {
            std::string const_name = tokens[i].getString();
            Constant constant;

            next_index(i, tokens.size());
            if(program.find_const(const_name, &constant) != 0){
                errors.panic( UNDECLARED_CONST(const_name) );
                return 1;
            }
            *expression = constant.value->clone();
        }
        return 0;
    case TOKENID_ADDRESS:
        next_index(i, tokens.size());
        if(tokens[i].id != TOKENID_WORD){
            errors.panic("Expected word after address operator");
            return 1;
        }
        *expression = new AddrWordExp( tokens[i].getString(), errors );
        next_index(i, tokens.size());
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
                errors.panic("Expected ')' after expression instead of " + tokens[i].toString());
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
    default:
        errors.panic("Unknown token in expression: " + tokens[i].toString());
        return 1;
    }
}
int parse_expression_operator_right(Configuration& config, TokenList& tokens, Program& program, size_t& i, int precedence, PlainExp** left, ErrorHandler& errors) {
    while(i != tokens.size()) {
        int token_precedence = tokens[i].getPrecedence();
        int operation;

        if (token_precedence < precedence) return 0;
        operation = tokens[i].id;

        if(operation == TOKENID_CLOSE or operation == TOKENID_NEWLINE or operation == TOKENID_NEXT
           or operation == TOKENID_BRACKET_CLOSE or operation == TOKENID_BEGIN or operation == TOKENID_END) return 0;

        if(operation == TOKENID_MEMBER){
            next_index(i, tokens.size());
            if(tokens[i].id != TOKENID_WORD){
                errors.panic("Expected word after ':' operator");
                return 1;
            }

            *left = new MemberExp(*left, tokens[i].getString(), errors);
            next_index(i, tokens.size());
        }
        else if(operation == TOKENID_WORD){
            PlainExp* right;
            std::string name = tokens[i].getString();

            next_index(i, tokens.size());
            if(parse_expression_primary(config, tokens, program, i, &right, errors) != 0) return 1;

            int next_precedence = tokens[i].getPrecedence();
            if (token_precedence < next_precedence) {
                if(parse_expression_operator_right(config, tokens, program, i, token_precedence + 1, &right, errors) != 0) return 1;
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

            ErrorHandler errors("idontexist.adept"); // TODO: Pass real ErrorHandler
            *left = new IndexLoadExp(*left, int_expr, errors);
            next_index(i, tokens.size());
        }
        else {
            PlainExp* right;
            next_index(i, tokens.size());
            if(parse_expression_primary(config, tokens, program, i, &right, errors) != 0) return 1;

            int next_precedence = tokens[i].getPrecedence();
            if (token_precedence < next_precedence) {
                if(parse_expression_operator_right(config, tokens, program, i, token_precedence + 1, &right, errors) != 0) return 1;
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
