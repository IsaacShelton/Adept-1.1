
#include <unistd.h>
#include <iostream>
#include "../include/die.h"
#include "../include/type.h"
#include "../include/parse.h"
#include "../include/lexer.h"
#include "../include/errors.h"
#include "../include/strings.h"

int parse(Configuration& config, std::vector<Token>* tokens, Program& program){
    // NOTE: This function deletes 'tokens' after it is done parsing

    for(size_t i = 0; i != tokens->size(); i++){
        if(parse_token(config, *tokens, program, i) != 0) return 1;
    }

    delete tokens; // Free Tokens

    // Print Parser Time
    if(config.time and !config.silent){
        config.clock.print_since("Parser Finished");
        config.clock.remember();
    }
    return 0;
}

int parse_token(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i){
    static AttributeInfo attr_info(false);

    switch(tokens[i].id){
    case TOKENID_KEYWORD:
        if(parse_keyword(config, tokens, program, i, attr_info) != 0) return 1;
        break;
    case TOKENID_WORD:
        if(parse_word(config, tokens, program, i) != 0) return 1;
        break;
    case TOKENID_NEWLINE:
        break;
    default:
        die("Expected definition");
    }

    return 0;
}
int parse_word(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i){
    // a_word <unknown syntax follows>
    //   ^

    std::string name = tokens[i].getString();
    next_index(i, tokens.size());

    switch(tokens[i].id){
        default:
            die("Expected definition");
    }

    return 0;
}
int parse_keyword(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i, const AttributeInfo& attr_info){
    // a_keyword <unknown syntax follows>
    //     ^

    std::string keyword = tokens[i].getString();
    next_index(i, tokens.size());

    if(keyword == "def"){
        if(parse_function(config, tokens, program, i, attr_info) != 0) return 1;
    }
    else if(keyword == "foreign"){
        if(parse_external(config, tokens, program, i, attr_info) != 0) return 1;
    }
    else if(keyword == "type"){
        if(parse_structure(config, tokens, program, i, attr_info) != 0) return 1;
    }
    else if(keyword == "import"){
        if(tokens[i].id != TOKENID_WORD){
            die("Expected module name after import");
        }

        std::string name = tokens[i].getString();
        TokenList* import_tokens = new TokenList;
        Configuration* import_config = new Configuration(config);
        Program* import_program = new Program;
        std::string target_bc;
        std::string target_obj;

        import_config->silent = true;
        import_config->time = false;
        import_config->obj = true;

        std::string local_file = filename_path(config.filename) + name + ".adept";
        std::string public_file = "C:/Users/" + config.username + "/.adept/import/" + name + ".adept";

        if( access(local_file.c_str(), F_OK) != -1 ){
            name = local_file;
        }
        else if( access(public_file.c_str(), F_OK) != -1 ){
            name = public_file;
        }
        else {
            die( UNKNOWN_MODULE(name) );
        }

        import_config->filename = name;
        if( tokenize(*import_config, name, import_tokens) != 0 )        return 1;
        if( parse(*import_config, import_tokens, *import_program) != 0 ) return 1; // Deletes imported_tokens

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
        program.import_merge(*import_program);
    }
    else if(keyword == "public" or keyword == "private"){
        if(parse_attribute(config, tokens, program, --i) != 0) return 1;
    }
    else {
        die( UNEXPECTED_KEYWORD(keyword) );
    }

    return 0;
}
int parse_structure(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i, const AttributeInfo& attr_info){
    // type struct_name { ... }
    //          ^

    std::string name = tokens[i].getString();
    std::vector<Field> members;
    next_index(i, tokens.size());
    next_index(i, tokens.size());

    while(tokens[i].id != TOKENID_END){
        if(tokens[i].id == TOKENID_NEWLINE){
            next_index(i, tokens.size());
            continue;
        }
        else if(tokens[i].id != TOKENID_WORD){
            fail("Expected member");
            return 1;
        }

        std::string name = tokens[i].getString();
        std::string type;
        next_index(i, tokens.size());

        if(tokens[i].id != TOKENID_WORD){
            fail("Expected type name");
            return 1;
        }

        while(tokens[i].id != TOKENID_NEWLINE){
            switch(tokens[i].id){
            case TOKENID_MULTIPLY:
                type += "*";
                break;
            case TOKENID_WORD:
                type += tokens[i].getString();
                break;
            default:
                fail("Unexpected token in type");
                return 1;
            }
            next_index(i, tokens.size());
        }

        members.push_back( Field{name, type} );
    }

    program.structures.push_back( Structure{name, members, attr_info.is_public} );
    return 0;
}
int parse_function(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i, const AttributeInfo& attr_info){
    // def func_name() ret_type { <some code> }
    //         ^

    std::string name = tokens[i].getString();
    next_index(i, tokens.size());

    if(tokens[i].id != TOKENID_OPEN){
        std::cerr << "Expected open after function name" << std::endl;
        return 1;
    }
    next_index(i, tokens.size());

    std::vector<Field> arguments;
    std::string return_type;
    std::vector<Statement> statements;

    while(tokens[i].id != TOKENID_CLOSE){
        if(tokens[i].id != TOKENID_WORD){
            fail("Expected argument name");
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
            fail("Expected argument type");
            return 1;
        }

        type += tokens[i].getString();
        next_index(i, tokens.size());

        if(tokens[i].id != TOKENID_CLOSE) next_index(i, tokens.size());
        arguments.push_back( Field{name, type} );
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
            fail("Unexpected token in type");
            return 1;
        }
        next_index(i, tokens.size());
    }

    next_index(i, tokens.size());
    if(parse_block(config, tokens, program, statements, i) != 0) return 1;

    program.functions.push_back( Function{name, arguments, return_type, statements, attr_info.is_public} );
    return 0;
}
int parse_external(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i, const AttributeInfo& attr_info){
    // foreign func_name() ret_type
    //            ^

    std::string name = tokens[i].getString();
    next_index(i, tokens.size());

    if(tokens[i].id != TOKENID_OPEN){
        std::cerr << "Expected open after foreign function name" << std::endl;
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
            fail("Expected argument type");
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
            fail("Unexpected token in type");
            return 1;
        }
        next_index(i, tokens.size());
    }

    program.externs.push_back( External{name, arguments, return_type, attr_info.is_public} );
    return 0;
}
int parse_attribute(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i){
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

    if(parse_keyword(config, tokens, program, i, attr_info) != 0) return 1;
    return 0;
}

int parse_block(Configuration& config, std::vector<Token>& tokens, Program& program, std::vector<Statement>& statements, size_t& i){
    // { some code; some more code; }
    //    ^

    while(i != tokens.size() and tokens[i].id != TOKENID_END){
        switch(tokens[i].id){
        case TOKENID_NEWLINE:
            next_index(i, tokens.size());
            break;
        case TOKENID_KEYWORD:
            if(parse_block_keyword(config, tokens, program, statements, i, tokens[i].getString()) != 0) return -1;
            break;
        case TOKENID_WORD:
            if(parse_block_word(config, tokens, program, statements, i) != 0) return -1;
            break;
        case TOKENID_MULTIPLY: // multiply/pointer operator
            if(parse_block_dereference(config, tokens, program, statements, i) != 0) return -1;
            break;
        default:
            die("Expected Statement");
        }
    }

    return 0;
}
int parse_block_keyword(Configuration& config, std::vector<Token>& tokens, Program& program, std::vector<Statement>& statements, size_t& i, std::string keyword){
    // keyword <unknown syntax follows>
    //    ^

    if(keyword == "return"){
        PlainExp* expression;
        next_index(i, tokens.size());

        if(tokens[i].id == TOKENID_NEWLINE){
            statements.push_back( STATEMENT_RETURN(NULL) );
        }
        else {
            if(parse_expression(config, tokens, program, i, &expression) != 0) return 1;
            statements.push_back( STATEMENT_RETURN(expression) );
        }
    }
    else {
        die( UNEXPECTED_KEYWORD(keyword) );
    }

    return 0;
}
int parse_block_word(Configuration& config, std::vector<Token>& tokens, Program& program, std::vector<Statement>& statements, size_t& i){
    // word <unknown syntax follows>
    //  ^

    std::string name = tokens[i].getString();
    next_index(i, tokens.size());

    switch(tokens[i].id){
    case TOKENID_WORD:
    case TOKENID_MULTIPLY:
        // Variable Definition
        if(parse_block_variable_declaration(config, tokens, program, statements, i, name) != 0) return 1;
        break;
    case TOKENID_OPEN:
        // Function Call
        if(parse_block_call(config, tokens, program, statements, i, name) != 0) return 1;
        break;
    case TOKENID_ASSIGN:
    case TOKENID_BRACKET_OPEN:
        // Variable Assign
        if(parse_block_assign(config, tokens, program, statements, i, name, 0) != 0) return 1;
        break;
    case TOKENID_MEMBER:
        // Variable Assign
        if(parse_block_member_assign(config, tokens, program, statements, i, name) != 0) return 1;
        break;
    default:
        fail("Unexpected Operator after word in block");
        return 1;
    }

    return 0;
}
int parse_block_variable_declaration(Configuration& config, std::vector<Token>& tokens, Program& program, std::vector<Statement>& statements, size_t& i, std::string name){
    // name str = "Hello World"
    //       ^

    std::string type;
    while(tokens[i].id == TOKENID_MULTIPLY){
        type += "*";
        next_index(i, tokens.size());
    }

    if(tokens[i].id != TOKENID_WORD){
        die("Expected word in type");
    }

    type += tokens[i].getString();
    next_index(i, tokens.size());

    if(tokens[i].id == TOKENID_ASSIGN){
        PlainExp* expression;
        next_index(i, tokens.size());

        if(parse_expression(config, tokens, program, i, &expression) != 0) return 1;
        statements.push_back( STATEMENT_DECLAREAS(name, type, expression) );
    }
    else {
        statements.push_back( STATEMENT_DECLARE(name, type) );
    }
    return 0;
}
int parse_block_call(Configuration& config, std::vector<Token>& tokens, Program& program, std::vector<Statement>& statements, size_t& i, std::string name){
    // name(arg1, arg2)
    //     ^

    std::vector<PlainExp*> args;
    next_index(i, tokens.size());

    if(tokens[i].id != TOKENID_CLOSE) {
        while(true) {
            PlainExp* expression;

            if(parse_expression(config, tokens, program, i, &expression) != 0) return 1;

            args.push_back(expression);

            if(tokens[i].id == TOKENID_CLOSE) break;
            if(tokens[i].id != TOKENID_NEXT){
                std::cerr << "Expected ')' or ',' in argument list" << std::endl;
                return 1;
            }

            next_index(i, tokens.size());
        }
    }

    statements.push_back( STATEMENT_CALL(name, args) );
    next_index(i, tokens.size());
    return 0;
}
int parse_block_assign(Configuration& config, std::vector<Token>& tokens, Program& program, std::vector<Statement>& statements, size_t& i, std::string name, int loads){
    // name = 10 * 3 / 4   |   name[i] = 10 * 3 / 4
    //      ^                      ^

    PlainExp* expression;
    std::vector<PlainExp*> gep_loads;

    if(tokens[i].id == TOKENID_BRACKET_OPEN){
        PlainExp* int_expr;

        while(tokens[i].id == TOKENID_BRACKET_OPEN){
            next_index(i, tokens.size());
            if(parse_expression(config, tokens, program, i, &int_expr) != 0) return 1;

            if(tokens[i].id != TOKENID_BRACKET_CLOSE){
                die("Expected closing bracket ']'");
            }

            gep_loads.push_back(int_expr);
            next_index(i, tokens.size());
        }
    }

    next_index(i, tokens.size());
    if(parse_expression(config, tokens, program, i, &expression) != 0) return 1;
    statements.push_back( STATEMENT_ASSIGN(name, expression, loads, gep_loads) );
    return 0;
}
int parse_block_member_assign(Configuration& config, std::vector<Token>& tokens, Program& program, std::vector<Statement>& statements, size_t& i, std::string name){
    // name:member:member = 10 * 3 / 4
    //     ^

    std::vector<AssignMemberPathNode> path = { AssignMemberPathNode{name, std::vector<PlainExp*>()} };
    PlainExp* expression;

    while(tokens[i].id == TOKENID_MEMBER){
        next_index(i, tokens.size());
        if(tokens[i].id != TOKENID_WORD){
            std::cerr << "Expected word after ':' operator" << std::endl;
            return 1;
        }

        path.push_back( AssignMemberPathNode{tokens[i].getString(), std::vector<PlainExp*>()} );
        next_index(i, tokens.size());
    }

    if(tokens[i].id != TOKENID_ASSIGN){
        std::cerr << "Expected '=' operator after member of variable" << std::endl;
        return 1;
    }
    next_index(i, tokens.size());

    if(parse_expression(config, tokens, program, i, &expression) != 0) return 1;
    statements.push_back( STATEMENT_ASSIGNMEMBER(path, expression, 0) );
    return 0;
}
int parse_block_dereference(Configuration& config, std::vector<Token>& tokens, Program& program, std::vector<Statement>& statements, size_t& i){
    // *value = expression
    //    ^

    int deref_count = 0;
    std::string name;

    while(tokens[i].id == TOKENID_MULTIPLY){
        deref_count += 1;
        next_index(i, tokens.size());
    }

    if(tokens[i].id != TOKENID_WORD){
        die("Expected word after dereference operator");
    }

    name = tokens[i].getString();
    next_index(i, tokens.size());

    if(parse_block_assign(config, tokens, program, statements, i, name, deref_count) != 0) return 1;
    return 0;
}

int parse_expression(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i, PlainExp** expression){
    // 10 + 3 * 6
    // ^

    if(parse_expression_primary(config, tokens, program, i, expression) != 0) return 1;
    if(parse_expression_operator_right(config, tokens, program, i, 0, expression) != 0) return 1;
    return 0;
}
int parse_expression_primary(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i, PlainExp** expression){
    switch (tokens[i].id) {
    case TOKENID_WORD:
        next_index(i, tokens.size());
        if(tokens[i].id == TOKENID_OPEN){
            if(parse_expression_call(config, tokens, program, --i, expression) != 0) return 1;
        }
        else {
            *expression = new WordExp( tokens[i-1].getString() );
        }
        return 0;
    case TOKENID_ADDRESS:
        next_index(i, tokens.size());
        if(tokens[i].id != TOKENID_WORD){
            die("Expected word after address operator");
        }
        *expression = new AddrWordExp( tokens[i].getString() );
        next_index(i, tokens.size());
        return 0;
    case TOKENID_MULTIPLY: // Dereference
        {
            PlainExp* primary;
            next_index(i, tokens.size());

            if(parse_expression_primary(config, tokens, program, i, &primary) != 0) return 1;

            *expression = new LoadExp(primary);
            return 0;
        }
    case TOKENID_BYTE:
        *expression = new ByteExp( tokens[i].getByte() );
        next_index(i, tokens.size());
        return 0;
    case TOKENID_SHORT:
        *expression = new ShortExp( tokens[i].getShort() );
        next_index(i, tokens.size());
        return 0;
    case TOKENID_INT:
        *expression = new IntegerExp( tokens[i].getInt() );
        next_index(i, tokens.size());
        return 0;
    case TOKENID_LONG:
        *expression = new LongExp( tokens[i].getLong() );
        next_index(i, tokens.size());
        return 0;
    case TOKENID_UBYTE:
        *expression = new UnsignedByteExp( tokens[i].getUByte() );
        next_index(i, tokens.size());
        return 0;
    case TOKENID_USHORT:
        *expression = new UnsignedShortExp( tokens[i].getUShort() );
        next_index(i, tokens.size());
        return 0;
    case TOKENID_UINT:
        *expression = new UnsignedIntegerExp( tokens[i].getUInt() );
        next_index(i, tokens.size());
        return 0;
    case TOKENID_ULONG:
        *expression = new UnsignedLongExp( tokens[i].getULong() );
        next_index(i, tokens.size());
        return 0;
    case TOKENID_FLOAT:
        *expression = new FloatExp( tokens[i].getFloat() );
        next_index(i, tokens.size());
        return 0;
    case TOKENID_DOUBLE:
        *expression = new DoubleExp( tokens[i].getDouble() );
        next_index(i, tokens.size());
        return 0;
    case TOKENID_STRING:
        *expression = new StringExp( tokens[i].getString() );
        next_index(i, tokens.size());
        return 0;
    case TOKENID_OPEN:
        {
            next_index(i, tokens.size()); // eat (.
            if(parse_expression(config, tokens, program, i, expression) != 0) return 1;

            if (tokens[i].id != TOKENID_CLOSE){
                std::cerr << "Expected ')' after expression instead of " << tokens[i].toString()  << std::endl;
                return 1;
            }
            next_index(i, tokens.size()); // eat ).
        }
        return 0;
    default:
        std::cerr << "Unknown token in expression " << tokens[i].id << std::endl;
        return 1;
    }
}
int parse_expression_operator_right(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i, int precedence, PlainExp** left) {
    // If this is a binop, find its precedence.
    while(i != tokens.size()) {
        int token_precedence = tokens[i].getPrecedence();

        // If this is a binop that binds at least as tightly as the current binop,
        // consume it, otherwise we are done.
        if (token_precedence < precedence) return 0;

        // Okay, we know this is a binop.
        int operation = tokens[i].id;
        if(operation == TOKENID_CLOSE or operation == TOKENID_NEWLINE or operation == TOKENID_NEXT or operation == TOKENID_BRACKET_CLOSE) return 0;

        next_index(i, tokens.size());

        // Parse the primary expression after the binary operator.
        if(operation == TOKENID_MEMBER){
            if(tokens[i].id != TOKENID_WORD){
                std::cerr << "Expected word after ':' operator" << std::endl;
                return 1;
            }

            *left = new MemberExp(*left, tokens[i].getString());
            next_index(i, tokens.size());
        }
        else {
            PlainExp* right;
            if(parse_expression_primary(config, tokens, program, i, &right) != 0) return 1;

            // If BinOp binds less tightly with RHS than the operator after RHS, let
            // the pending operator take RHS as its LHS.
            int next_precedence = tokens[i].getPrecedence();
            if (token_precedence < next_precedence) {
                if(parse_expression_operator_right(config, tokens, program, i, token_precedence + 1, &right) != 0) return 1;
            }

            // Merge LHS/RHS.
            *left = new OperatorExp(operation, *left, right);
        }
    }

    return 0;
}
int parse_expression_call(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i, PlainExp** expression){
    // call(arg, arg, arg) + 132
    //  ^

    std::string name = tokens[i].getString();
    std::vector<PlainExp*> args;
    i++; // Safe because parse_expression_primary

    next_index(i, tokens.size());
    while(i != tokens.size() and tokens[i].id != TOKENID_CLOSE){
        PlainExp* arg;
        if(parse_expression(config, tokens, program, i, &arg) != 0) return 1;
        args.push_back(arg);

        if(tokens[i].id == TOKENID_NEXT) next_index(i, tokens.size());
    }

    *expression = new CallExp(name, args);
    next_index(i, tokens.size());
    return 0;
}
