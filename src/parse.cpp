
#include <iostream>
#include "../include/die.h"
#include "../include/type.h"
#include "../include/parse.h"

int parse(Configuration& config, std::vector<Token>* tokens, Program& program){

    for(size_t i = 0; i != tokens->size(); i++){
        if(parse_token(config, *tokens, program, i) != 0) return 1;
    }

    // Free Tokens
    delete tokens;

    // Print Parser Time
    config.clock.print_since("Parser Finished");
    config.clock.remember();
    return 0;
}

int parse_token(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i){

    switch(tokens[i].id){
    case TOKENID_KEYWORD:
        if(parse_keyword(config, tokens, program, i) != 0) return 1;
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
int parse_keyword(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i){
    // a_keyword <unknown syntax follows>
    //     ^

    std::string keyword = tokens[i].getString();
    next_index(i, tokens.size());

    if(keyword == "def"){
        if(parse_function(config, tokens, program, i) != 0) return 1;
    }
    else if(keyword == "extern"){
        if(parse_external(config, tokens, program, i) != 0) return 1;
    }
    else if(keyword == "type"){
        if(parse_structure(config, tokens, program, i) != 0) return 1;
    }
    else {
        std::cerr << "Unknown Keyword" << std::endl;
        return 1;
    }

    return 0;
}
int parse_structure(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i){
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

    program.structures.push_back( Structure{name, members} );
    return 0;
}
int parse_function(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i){
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

        if(tokens[i].id != TOKENID_WORD){
            fail("Expected argument type");
            return 1;
        }

        type = tokens[i].getString();
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

    program.functions.push_back( Function{name, arguments, return_type, statements} );
    return 0;
}
int parse_external(Configuration& config, std::vector<Token>& tokens, Program& program, size_t& i){
    // extern func_name() ret_type
    //            ^

    std::string name = tokens[i].getString();
    next_index(i, tokens.size());

    if(tokens[i].id != TOKENID_OPEN){
        std::cerr << "Expected open after external function name" << std::endl;
        return 1;
    }
    next_index(i, tokens.size());

    std::vector<std::string> arguments;
    std::string return_type;

    while(tokens[i].id != TOKENID_CLOSE){
        if(tokens[i].id != TOKENID_WORD){
            fail("Expected argument name");
            return 1;
        }

        std::string type = tokens[i].getString();
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

    program.externs.push_back( External{name, arguments, return_type} );
    return 0;
}

int parse_block(Configuration& config, std::vector<Token>& tokens, Program& program, std::vector<Statement>& statements, size_t& i){
    // { some code; some more code; }
    //    ^

    while(i != tokens.size() and tokens[i].id != TOKENID_END){
        switch(tokens[i].id){
        case TOKENID_KEYWORD:
            if(parse_block_keyword(config, tokens, program, statements, i, tokens[i].getString()) != 0) return -1;
            break;
        case TOKENID_WORD:
            if(parse_block_word(config, tokens, program, statements, i) != 0) return -1;
            break;
        default:
            next_index(i, tokens.size());
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

        if(parse_expression(config, tokens, program, i, &expression) != 0) return 1;
        statements.push_back( STATEMENT_RETURN(expression) );
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
        // Variable Definition
        if(parse_block_variable_declaration(config, tokens, program, statements, i, name) != 0) return 1;
        break;
    case TOKENID_OPEN:
        // Function Call
        if(parse_block_call(config, tokens, program, statements, i, name) != 0) return 1;
        break;
    case TOKENID_ASSIGN:
        // Variable Assign
        if(parse_block_assign(config, tokens, program, statements, i, name) != 0) return 1;
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

    std::string type = tokens[i].getString();
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
int parse_block_assign(Configuration& config, std::vector<Token>& tokens, Program& program, std::vector<Statement>& statements, size_t& i, std::string name){
    // name = 10 * 3 / 4
    //      ^

    PlainExp* expression;
    next_index(i, tokens.size());

    if(parse_expression(config, tokens, program, i, &expression) != 0) return 1;
    statements.push_back( STATEMENT_ASSIGN(name, expression) );
    return 0;
}
int parse_block_member_assign(Configuration& config, std::vector<Token>& tokens, Program& program, std::vector<Statement>& statements, size_t& i, std::string name){
    // name:member:member = 10 * 3 / 4
    //     ^

    std::vector<std::string> path = {name};
    PlainExp* expression;

    while(tokens[i].id == TOKENID_MEMBER){
        next_index(i, tokens.size());
        if(tokens[i].id != TOKENID_WORD){
            std::cerr << "Expected word after ':' operator" << std::endl;
            return 1;
        }

        path.push_back(tokens[i].getString());
        next_index(i, tokens.size());
    }

    if(tokens[i].id != TOKENID_ASSIGN){
        std::cerr << "Expected '=' operator after member of variable" << std::endl;
        return 1;
    }
    next_index(i, tokens.size());

    if(parse_expression(config, tokens, program, i, &expression) != 0) return 1;
    statements.push_back( STATEMENT_ASSIGNMEMBER(path, expression) );
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
    case TOKENID_INT:
        *expression = new IntegerExp( tokens[i].getInt() );
        next_index(i, tokens.size());
        return 0;
    case TOKENID_FLOAT:
        *expression = new DoubleExp( tokens[i].getFloat() );
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
        if(operation == TOKENID_CLOSE or operation == TOKENID_NEWLINE or operation == TOKENID_NEXT) return 0;

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
