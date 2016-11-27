
#include <iostream>
#include "../include/statement.h"

Statement::Statement(){
    id = 0;
    data = NULL;
}
Statement::Statement(const Statement& other){
    id = other.id;

    switch(id){
    case 1:
        data = new DeclareStatement( *(static_cast<DeclareStatement*>(other.data)) );
        break;
    case 2:
        data = new DeclareAsStatement( *(static_cast<DeclareAsStatement*>(other.data)) );
        break;
    case 3:
        data = new ReturnStatement( *(static_cast<ReturnStatement*>(other.data)) );
        break;
    case 4:
        data = new AssignStatement( *(static_cast<AssignStatement*>(other.data)) );
        break;
    case 5:
        data = new AssignMemberStatement( *(static_cast<AssignMemberStatement*>(other.data)) );
        break;
    case 6:
        data = new CallStatement( *(static_cast<CallStatement*>(other.data)) );
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
    case 1:
        delete static_cast<DeclareStatement*>(data);
        break;
    case 2:
        delete static_cast<DeclareAsStatement*>(data);
        break;
    case 3:
        delete static_cast<ReturnStatement*>(data);
        break;
    case 4:
        delete static_cast<AssignStatement*>(data);
        break;
    case 5:
        delete static_cast<AssignMemberStatement*>(data);
        break;
    case 6:
        delete static_cast<CallStatement*>(data);
        break;
    }

    data = NULL;
}
void Statement::reset(){
    free();
    id = 0;
}
std::string Statement::toString(){
    std::string str;

    switch(id){
    case 0:
        break;
    case 1:
        {
            DeclareStatement* extra = static_cast<DeclareStatement*>(data);
            str = extra->name + " " + extra->type;
            break;
        }
    case 2:
        {
            DeclareAsStatement* extra = static_cast<DeclareAsStatement*>(data);
            str = extra->name + " " + extra->type + " = " + extra->value->toString();
            break;
        }
    case 3:
        {
             ReturnStatement* extra = static_cast<ReturnStatement*>(data);
             str = "return " + extra->value->toString();
             break;
        }
    case 4:
        {
            AssignStatement* extra = static_cast<AssignStatement*>(data);
            str = extra->name + " = " + extra->value->toString();
            break;
        }
    case 5:
        {
            AssignMemberStatement* extra = static_cast<AssignMemberStatement*>(data);
            for(size_t i = 0; i != extra->path.size(); i++){
                str += extra->path[i];
                if(i + 1 != extra->path.size()) str += ":";
            }
            str += " = " + extra->value->toString();
            break;
        }
    case 6:
        {
            CallStatement* extra = static_cast<CallStatement*>(data);
            str = extra->name + "(";
            for(size_t i = 0; i != extra->args.size(); i++){
                str += extra->args[i]->toString();
                if(i+1 != extra->args.size()) str += ", ";
            }
            str += ")";
            break;
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
}
AssignStatement::AssignStatement(std::string n, PlainExp* e){
    name = n;
    value = e;
}
AssignStatement::~AssignStatement(){
    delete value;
}

AssignMemberStatement::AssignMemberStatement(const AssignMemberStatement& other){
    path = other.path;
    value = other.value->clone();
}
AssignMemberStatement::AssignMemberStatement(const std::vector<std::string>& p, PlainExp* e){
    path = p;
    value = e;
}
AssignMemberStatement::~AssignMemberStatement(){
    delete value;
}

CallStatement::CallStatement(const CallStatement& other){
    name = other.name;
    for(PlainExp* exp : other.args) args.push_back( exp->clone() );
}
CallStatement::CallStatement(std::string n, std::vector<PlainExp*> a){
    name = n;
    args = a;
}
CallStatement::~CallStatement(){
    for(PlainExp* exp : args) delete exp;
}

