
#include <iostream>
#include "../include/strings.h"
#include "../include/statement.h"

Statement::Statement(){
    id = 0;
    data = NULL;
}
Statement::Statement(const Statement& other){
    id = other.id;

    switch(id){
    case STATEMENTID_DECLARE:
        data = new DeclareStatement( *(static_cast<DeclareStatement*>(other.data)) );
        break;
    case STATEMENTID_DECLAREAS:
        data = new DeclareAsStatement( *(static_cast<DeclareAsStatement*>(other.data)) );
        break;
    case STATEMENTID_RETURN:
        data = new ReturnStatement( *(static_cast<ReturnStatement*>(other.data)) );
        break;
    case STATEMENTID_ASSIGN:
        data = new AssignStatement( *(static_cast<AssignStatement*>(other.data)) );
        break;
    case STATEMENTID_ASSIGNMEMBER:
        data = new AssignMemberStatement( *(static_cast<AssignMemberStatement*>(other.data)) );
        break;
    case STATEMENTID_CALL:
        data = new CallStatement( *(static_cast<CallStatement*>(other.data)) );
        break;
    case STATEMENTID_IF:
    case STATEMENTID_WHILE:
        data = new ConditionalStatement( *(static_cast<ConditionalStatement*>(other.data)) );
        break;
    default:
        data = NULL;
    }
}
Statement::Statement(uint16_t i){
    id = i;
    data = NULL;
}
Statement::Statement(uint16_t i, void* d){
    id = i;
    data = d;
}
Statement::~Statement(){
    free();
}
void Statement::free(){
    switch(id){
    case STATEMENTID_DECLARE:
        delete static_cast<DeclareStatement*>(data);
        break;
    case STATEMENTID_DECLAREAS:
        delete static_cast<DeclareAsStatement*>(data);
        break;
    case STATEMENTID_RETURN:
        delete static_cast<ReturnStatement*>(data);
        break;
    case STATEMENTID_ASSIGN:
        delete static_cast<AssignStatement*>(data);
        break;
    case STATEMENTID_ASSIGNMEMBER:
        delete static_cast<AssignMemberStatement*>(data);
        break;
    case STATEMENTID_CALL:
        delete static_cast<CallStatement*>(data);
        break;
    case STATEMENTID_IF:
    case STATEMENTID_WHILE:
        delete static_cast<ConditionalStatement*>(data);
        break;
    }

    data = NULL;
}
void Statement::reset(){
    free();
    id = 0;
}
std::string Statement::toString(unsigned int indent){
    std::string str;

    switch(id){
    case STATEMENTID_NONE:
        break;
    case STATEMENTID_DECLARE:
        {
            DeclareStatement* extra = static_cast<DeclareStatement*>(data);
            for(unsigned int i = 0; i != indent; i++) str += "    ";

            str += extra->name + " " + extra->type;
            break;
        }
    case STATEMENTID_DECLAREAS:
        {
            DeclareAsStatement* extra = static_cast<DeclareAsStatement*>(data);
            for(unsigned int i = 0; i != indent; i++) str += "    ";

            str += extra->name + " " + extra->type + " = " + extra->value->toString();
            break;
        }
    case STATEMENTID_RETURN:
        {
             ReturnStatement* extra = static_cast<ReturnStatement*>(data);
             for(unsigned int i = 0; i != indent; i++) str += "    ";

             str += "return " + extra->value->toString();
             break;
        }
    case STATEMENTID_ASSIGN:
        {
            AssignStatement* extra = static_cast<AssignStatement*>(data);
            for(unsigned int i = 0; i != indent; i++) str += "    ";

            for(int i = 0; i != extra->loads; i++) str += "*";
            str += extra->name;
            for(size_t i = 0; i != extra->gep_loads.size(); i++) str += "[" + extra->gep_loads[i]->toString() + "]";
            str += " = " + extra->value->toString();
            break;
        }
    case STATEMENTID_ASSIGNMEMBER:
        {
            AssignMemberStatement* extra = static_cast<AssignMemberStatement*>(data);
            for(unsigned int i = 0; i != indent; i++) str += "    ";

            for(size_t i = 0; i != extra->path.size(); i++){
                str += extra->path[i].name;

                for(size_t j = 0; j != extra->path[i].gep_loads.size(); j++) str += "[" + extra->path[i].gep_loads[j]->toString() + "]";

                if(i + 1 != extra->path.size()) str += ":";
            }
            str += " = " + extra->value->toString();
            break;
        }
    case STATEMENTID_CALL:
        {
            CallStatement* extra = static_cast<CallStatement*>(data);

            for(unsigned int i = 0; i != indent; i++) str += "    ";
            str += extra->name + "(";

            for(size_t i = 0; i != extra->args.size(); i++){
                str += extra->args[i]->toString();
                if(i+1 != extra->args.size()) str += ", ";
            }

            str += ")";
            break;
        }
    case STATEMENTID_IF:
        {
            ConditionalStatement* extra = static_cast<ConditionalStatement*>(data);

            for(unsigned int i = 0; i != indent; i++) str += "    ";
            str += "if " + extra->condition->toString() + "{\n";

            for(size_t a = 0; a != extra->statements.size(); a++){
                str += extra->statements[a].toString(indent+1) + "\n";
            }

            for(unsigned int i = 0; i != indent; i++) str += "    ";
            str += "}";
        }
    case STATEMENTID_WHILE:
        {
            ConditionalStatement* extra = static_cast<ConditionalStatement*>(data);

            for(unsigned int i = 0; i != indent; i++) str += "    ";
            str += "while " + extra->condition->toString() + "{\n";

            for(size_t a = 0; a != extra->statements.size(); a++){
                str += extra->statements[a].toString(indent+1) + "\n";
            }

            for(unsigned int i = 0; i != indent; i++) str += "    ";
            str += "}";
        }
    }

    return str;
}

DeclareStatement::DeclareStatement(std::string n, std::string t){
    name = n;
    type = t;
}

DeclareAsStatement::DeclareAsStatement(const DeclareAsStatement& other){
    name = other.name;
    type = other.type;
    value = other.value->clone();
}
DeclareAsStatement::DeclareAsStatement(std::string n, std::string t, PlainExp* e){
    name = n;
    type = t;
    value = e;
}
DeclareAsStatement::~DeclareAsStatement(){
    delete value;
}

ReturnStatement::ReturnStatement(const ReturnStatement& other){
    value = (other.value == NULL) ? NULL : other.value->clone();
}
ReturnStatement::ReturnStatement(PlainExp* e){
    value = e;
}
ReturnStatement::~ReturnStatement(){
    delete value;
}

AssignStatement::AssignStatement(const AssignStatement& other){
    name = other.name;
    value = other.value->clone();
    loads = other.loads;
    gep_loads.resize(other.gep_loads.size());

    for(size_t i = 0; i != other.gep_loads.size(); i++){
        gep_loads[i] = other.gep_loads[i]->clone();
    }
}
AssignStatement::AssignStatement(std::string n, PlainExp* e, int l){
    name = n;
    value = e;
    loads = l;
}
AssignStatement::AssignStatement(std::string n, PlainExp* e, int l, const std::vector<PlainExp*>& g){
    name = n;
    value = e;
    loads = l;
    gep_loads = g;
}
AssignStatement::~AssignStatement(){
    for(PlainExp* expr : gep_loads) delete expr;
    delete value;
}

AssignMemberStatement::AssignMemberStatement(const AssignMemberStatement& other){
    value = other.value->clone();
    loads = other.loads;
    path.resize(other.path.size());

    for(size_t i = 0; i != other.path.size(); i++){
        path[i].name = other.path[i].name;
        for(size_t j = 0; j != other.path[i].gep_loads.size(); j++){
            path[i].gep_loads[j] = other.path[i].gep_loads[j]->clone();
        }
    }
}
AssignMemberStatement::AssignMemberStatement(const std::vector<std::string>& p, PlainExp* e, int l){
    path.resize(p.size());
    for(size_t i = 0; i != p.size(); i++) path[i] = AssignMemberPathNode{p[i], std::vector<PlainExp*>()};

    value = e;
    loads = l;
}
AssignMemberStatement::AssignMemberStatement(const std::vector<AssignMemberPathNode>& p, PlainExp* e, int l){
    path = p;
    value = e;
    loads = l;
}
AssignMemberStatement::~AssignMemberStatement(){
    for(AssignMemberPathNode& node : path){
        for(PlainExp* expr : node.gep_loads) delete expr;
    }
    delete value;
}

CallStatement::CallStatement(const CallStatement& other){
    name = other.name;
    for(PlainExp* exp : other.args) args.push_back( exp->clone() );
}
CallStatement::CallStatement(std::string n, const std::vector<PlainExp*>& a){
    name = n;
    args = a;
}
CallStatement::~CallStatement(){
    for(PlainExp* exp : args) delete exp;
}

ConditionalStatement::ConditionalStatement(const ConditionalStatement& other){
    condition = other.condition->clone();
    statements = other.statements;
}
ConditionalStatement::ConditionalStatement(PlainExp* c, const std::vector<Statement>& s){
    condition = c;
    statements = s;
}
ConditionalStatement::~ConditionalStatement(){
    delete condition;
}

